#include "ContextSubscriber.h"
#include "App.h"

LOGGER_Declare("cs");

ContextSubscriber::ContextSubscriber()
{
}


ContextSubscriber::~ContextSubscriber()
{
}

void ContextSubscriber::createDecoderThread() {
	m_threadDecoderExit = false;
	std::thread thrd(&ContextSubscriber::onThreadDecoder, this);
	thrd.detach();
}

void ContextSubscriber::createRenderThread() {
	//不建立线程
	m_threadRenderExit = false;
	std::thread thrd(&ContextSubscriber::onThreadRender, this);
	thrd.detach();
}

bool ContextSubscriber::startShow(HWND hWndShow) {
	//m_wndCapture = hWndCapture;
#ifdef SAVE_FILE
	char filepathname[256];
	sprintf(filepathname, "./xtclient_%lld.h264", xt_tickcount64());
	m_pFile = fopen(filepathname,"wb");
#endif
	m_wndShow = hWndShow;
	m_threadDecoderExit = true;
	m_threadRenderExit = true;
	do {
		if (!m_stream.create()) {
			break;
		}
		m_threadRunnig = true;
		createDecoderThread();
		createRenderThread();
		return true;
	} while (0);
	stopShow();
	return false;
}

void ContextSubscriber::stopShow() {
	m_threadRunnig = false;
	//等待线程结束
	while (!m_threadDecoderExit) {
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	while (!m_threadRenderExit) {
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
#ifdef SAVE_FILE
	if (m_pFile) {
		FILE *p = m_pFile;
		m_pFile = NULL;
		fclose(p);
	}
#endif
	m_decoder.Destroy();
	m_render.Destroy();
	m_mediaFact.fini();
	m_stream.destroy();
}

void ContextSubscriber::onHeader(struct xp_media_header_t *mh) {
	uint16_t width;
	uint16_t height;
	uint16_t type;
	n2h_u16(mh->video.type, type);
	n2h_u16(mh->video.width, width);
	n2h_u16(mh->video.height, height);
	if (!m_mediaFact.init(16, ((uint32_t)width * height) * 2, 8)) {
		MessageBox(NULL, "ContextSubscriber::m_mediaFact.init failed", "", MB_OK);
		return;
	}
	if (!m_render.Create(m_wndShow, width, height)) {
		MessageBox(NULL, "ContextSubscriber::m_render.Create failed", "", MB_OK);
		return;
	}
	if (!m_decoder.Create(type, width, height, mh->video.fps, mh->video.gop)) {
		MessageBox(NULL, "ContextSubscriber::m_decoder.Create failed", "", MB_OK);
		return;
	}
}

void ContextSubscriber::onData(stream_frame_t *frame, const uint8_t *pdata, int32_t len) {
#ifdef SAVE_FILE
	if (m_pFile) {
		fwrite(pdata, 1, len, m_pFile);
	}
#endif
	m_stream.putPacket(frame, pdata, len);
}

void ContextSubscriber::onThreadDecoder() {
	struct temp_render_t {
		volatile bool &fRunning;
		MediaFrameHolder *holder;
		MediaFrameFactory *fact;
		//media_frame_header_t *mh;
	}data = { m_threadRunnig, &m_holder, &m_mediaFact };
	while (m_threadRunnig) {
		if (!m_stream.hasData()) {
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
			continue;
		}

		uint32_t dataLen;
		stream_frame_t *pframe;
		stream_frame_t frame;
		uint8_t *p = m_stream.getPacket(&pframe, &dataLen);
		memcpy(&frame, pframe, sizeof(stream_frame_t));
		//p -= 4; p[0] = 0; p[1] = 0; p[2] = 0; p[3] = 1; dataLen += 4;
		m_decoder.Decode(p, dataLen, frame.type, frame.seqNo, [](void *userdata, uint64_t timeMs, struct video_frame_data_t *frame) {
			temp_render_t *tr = (temp_render_t *)userdata;
			media_frame_header_t *mh;
			for (int i = 0; i < 4; i++) {
				mh = tr->fact->alloc();
				if (mh != NULL) {
					video_frame_data_t &video = mh->u.video;
					uint8_t *pdata = (uint8_t *)mh;
					pdata += sizeof(media_frame_header_t);
					mh->video = 1;
					mh->seqOrPts = timeMs;
					video.width = frame->width;
					video.height = frame->height;

					video.line[0] = frame->line[0];
					video.line[1] = frame->line[1];
					video.line[2] = frame->line[2];
					video.line[3] = frame->line[3];
					//只考虑YUV420
					video.data[0] = pdata;
					video.data[3] = pdata;
					memcpy(video.data[0], frame->data[0], frame->line[0] * frame->height);
					video.data[1] = pdata + frame->line[0] * frame->height;
					memcpy(video.data[1], frame->data[1], frame->line[1] * frame->height / 2);
					video.data[2] = video.data[1] + frame->line[1] * frame->height / 2;
					memcpy(video.data[2], frame->data[2], frame->line[2] * frame->height / 2);

					tr->holder->onFrame(mh);
					return;
				}
				std::this_thread::sleep_for(std::chrono::milliseconds(10));
				if (!tr->fRunning) {
					return;
				}
			}
		}, &data);
		m_stream.offsetPacket();
	}
	m_threadDecoderExit = true;
}

void ContextSubscriber::onThreadRender() {
	uint64_t startTime = 0;
	uint64_t frameCount = 0;
	while (m_threadRunnig) {
		media_frame_header_t *plists = m_holder.getFrameLists();
		if (plists == NULL) {
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
			continue;
		}
		do {
			uint64_t curTime = ::GetTickCount64();
			media_frame_header_t *p = plists;
			plists = plists->next;
			if (startTime == 0) {
				startTime = curTime - p->seqOrPts;
			}
			else {
				LOGGER_Debug("frame %lld delta time %lld, frame count %lld", p->seqOrPts, curTime - startTime, frameCount);
				if (p->seqOrPts > (curTime - startTime) + 10) {
					uint64_t delta = p->seqOrPts - (curTime - startTime) - 10;
					//LOGGER_Debug("frame %lld delta time %lld, %lld", p->seqOrPts, curTime - startTime, delta);
					//Sleep((DWORD)delta);
					std::this_thread::sleep_for(std::chrono::milliseconds(delta));
				}
			}
			frameCount++;
			m_render.Display(&p->u.video);
			m_mediaFact.free(p);
		} while (plists != NULL && m_threadRunnig);
	}
	m_threadRenderExit = true;
}
