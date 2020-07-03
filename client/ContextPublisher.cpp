#include "ContextPublisher.h"
#include "App.h"
#include "ContextSubscriber.h"

LOGGER_Declare("publisher");

ContextPublisher::ContextPublisher()
{
	m_holder.setOwner(this);
}

ContextPublisher::~ContextPublisher()
{
}

void ContextPublisher::createEncoderThread() {
	m_threadEncoderExit = false;
	std::thread thrd(&ContextPublisher::onThreadEncoder, this);
	thrd.detach();
}

void ContextPublisher::createFileThread(int type, int fps, int width, int height) {
	m_threadEncoderExit = false;
	std::thread thrd(&ContextPublisher::onThreadFile, this, type, fps, width, height);
	thrd.detach();
}

void ContextPublisher::createSenderThread() {
	//不建立线程
	m_threadSenderExit = false;
	std::thread thrd(&ContextPublisher::onThreadSender, this);
	thrd.detach();
}

bool ContextPublisher::startCapture(const wchar_t *devname, HWND hWndCapture, int no, int fps) {
	//m_wndCapture = hWndCapture;
	m_threadEncoderExit = true;
	m_threadSenderExit = true;
	VideoCapture *cap = theApp.devMgr().createVideoCapture(devname, hWndCapture, no, fps);
	if (cap != NULL) {
		do {
			xp_media_header_t mediaHeader = { 0 };
			const video_capture_into_t *ci = cap->captureInfo();
			if (!m_mediaFact.init(24, m_videoSource.getDataSize(ci->width, ci->height), 24)) {
				break;
			}
			if (!m_stream.create()) {
				break;
			}

			m_videoSource.reset();
			m_videoSource.setDevice(cap);
			m_videoSource.setFactory(&m_mediaFact);
			m_videoSource.addSink(&m_holder);
			cap->setSink(&m_videoSource);

			if (!m_encoder.start(ci->width, ci->height, ci->fps)) {
				break;
			}
			m_threadRunnig = true;
			createEncoderThread();
			createSenderThread();
			m_videoCapture = cap;

			if (m_wndShow != NULL) {
				if (m_render.Create(m_wndShow, ci->width, ci->height)) {
					m_videoSource.addSink(&m_render); 
				}
			}

			h2n_u16(VIDEO_TYPE_H264,mediaHeader.video.type);
			mediaHeader.video.fps = ci->fps;
			mediaHeader.video.gop = ci->fps;
			h2n_u16(ci->width, mediaHeader.video.width);
			h2n_u16(ci->height, mediaHeader.video.height);
			if (m_rollback) {
				m_rollback->onHeader(&mediaHeader);
			}
			if (m_missionObject) {
				m_missionObject->sendData(getStreamId(), XT_MEDIA_TYPE_HEADER, 0/*no audio*/, 
					(uint8_t *)&mediaHeader, sizeof(mediaHeader));
			}

			if (0 != cap->start()) {
				break;
			}
			return true;
		} while (0);
		theApp.devMgr().destroyVideoCapture(cap);
		stopCapture();
	}
	return false;
}

bool ContextPublisher::startFile(const char *filename) {
	//m_wndCapture = hWndCapture;
	m_threadEncoderExit = true;
	m_threadSenderExit = true;
	do {
		struct file_params_t {
			char filename[512];
			int type;	//VIDEO_TYPE_H264
			int fps;
			//int gop;
			int width;
			int height;
		}fileinfo = { 0 };
		xp_media_header_t mediaHeader = { 0 };
		xt_ini_read(filename, [](void *userdata, const char *key, char *value)->int {
			file_params_t *fp = (file_params_t *)userdata;
			if (strcmp(key, "file") == 0) {
				strcpy(fp->filename, value);
			}
			else if (strcmp(key, "type") == 0) {
				fp->type = atoi(value);
			}
			else if (strcmp(key, "fps") == 0) {
				fp->fps = atoi(value);
			}
			//else if (strcmp(key, "gop") == 0) {
			//	fp->gop = atoi(value);
			//}
			else if (strcmp(key, "width") == 0) {
				fp->width = atoi(value);
			}
			else if (strcmp(key, "height") == 0) {
				fp->height = atoi(value);
			}
			return 0;
		}, &fileinfo);
		if (fileinfo.filename[0] == 0 ||
			fileinfo.fps <= 0 ||
			fileinfo.width <= 0 ||
			fileinfo.height <= 0
			) {
			break;
		}
		//if (fileinfo.gop < fileinfo.fps) {
		//	fileinfo.gop = fileinfo.fps;
		//}
		m_pFile = _fsopen(fileinfo.filename, "rb", SH_DENYWR);
		if (m_pFile == NULL) {
			break;
		}

		if (!m_stream.create()) {
			break;
		}

		m_threadRunnig = true;

		h2n_u16(fileinfo.type, mediaHeader.video.type);
		mediaHeader.video.fps = fileinfo.fps;
		mediaHeader.video.gop = 0;// fileinfo.gop;
		h2n_u16(fileinfo.width, mediaHeader.video.width);
		h2n_u16(fileinfo.height, mediaHeader.video.height);
		if (m_rollback) {
			m_rollback->onHeader(&mediaHeader);
		}
		if (m_missionObject) {
			m_missionObject->sendData(getStreamId(), XT_MEDIA_TYPE_HEADER, 0/*no audio*/,
				(uint8_t *)&mediaHeader, sizeof(mediaHeader));
		}
		createFileThread(fileinfo.type, fileinfo.fps, fileinfo.width, fileinfo.height);
		createSenderThread();

		return true;
	} while (0);
	stopCapture();
	return false;
}

void ContextPublisher::stopCapture() {
	m_threadRunnig = false;
	if (m_videoCapture != NULL) {
		m_videoCapture->setSink(NULL);
		m_videoCapture->stop();
		theApp.devMgr().destroyVideoCapture(m_videoCapture);
		m_videoCapture = NULL;
	}
	//等待线程结束
	while (!m_threadEncoderExit) {
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	while (!m_threadSenderExit) {
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	if (m_pFile != NULL) {
		fclose(m_pFile);
		m_pFile = NULL;
	}
	m_encoder.stop();
	m_render.Destroy();
	m_videoSource.reset();
	m_mediaFact.fini();
	m_stream.destroy();
}

void ContextPublisher::onThreadEncoder() {
	while (m_threadRunnig) {
		media_frame_header_t *plists = m_holder.getFrameLists();
		if (plists == NULL) {
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
			continue;
		}
		do {
			media_frame_header_t *p = plists;
			plists = plists->next;
			m_encoder.inputFrame(p->u.video.data[0], p->u.video.width, p->u.video.line[0], p->u.video.height,
				[](void *userData, uint8_t *pdata, int len, int frameType, uint64_t keyFrameNo) {
				ContextPublisher *pthis = (ContextPublisher *)userData;
				stream_frame_t frame;
				frame.stream = (uint8_t)pthis->getStreamId();
				frame.media = 0;	//video
				frame.type = frameType;
				frame.seqNo = (uint32_t)keyFrameNo;
				//LOGGER_Debug("encoder frame type %d len %d", frameType, len);
				pthis->streamBuffer().putPacket(&frame, pdata, len);
				//printf("got frame %d len %d\n", frameType, len);
			}, this);
			m_mediaFact.free(p);
		} while (plists != NULL && m_threadRunnig);
	}
	m_threadEncoderExit = true;
}

struct time_info_t {
	uint64_t startTime;
	uint64_t ptsTime;
	int64_t  frameCount;
	uint32_t stepTime;
};
static void waitTime(time_info_t &timeInfo) {
	uint64_t curTime = xt_tickcount64() / 1000;
	if (curTime - timeInfo.startTime + 5 < timeInfo.ptsTime) {
		std::this_thread::sleep_for(std::chrono::milliseconds(timeInfo.ptsTime - (curTime - timeInfo.startTime)));
	}
}
void ContextPublisher::onThreadFile(int type, int fps, int width, int height) {
	int dataSize = 1 * 1024 * 1024;
	int remain = 0;
	h26x_frame_info_t frames[256];
	bool f264 = type != VIDEO_TYPE_H265;
	uint8_t *pdata = new uint8_t[dataSize];
	time_info_t  timeInfo;
	timeInfo.stepTime = 1000 / fps;
	timeInfo.frameCount = -1;
	timeInfo.startTime = 0;// xt_tickcount64() / 1000;
	timeInfo.ptsTime = 0;
	
	while (m_threadRunnig) {
		//读取一帧数据 然后发送
		int recved = fread(pdata + remain, 1, dataSize - remain, m_pFile);
		bool fend = (recved < dataSize - remain) ? true : false;
		int  fc = parseH26xFrames(f264, pdata, remain + recved, frames, 256);
		LOGGER_Error("send parse frame count %d end %d", fc, fend ? 1 : 0);
		if (fc <= 1) {
			if (fend) {
				fseek(m_pFile, 0, SEEK_SET);
				remain = 0;
			}
			else if(fc <= 0){
				//保留最后4个字节
				memcpy(pdata, pdata + remain + recved - 4, 4);
				remain = 4;
			}
			else {
				memcpy(pdata, frames[0].pdata, frames[0].len);
				remain = frames[0].len;
			}
		}
		else{
			stream_frame_t frame;
			frame.stream = (uint8_t)getStreamId();
			frame.media = 0;	//video
			//LOGGER_Debug("encoder frame type %d len %d", frameType, len);

			for (int i = 0; i < fc - 1; i++) {
				frame.type = frames[i].type;
				switch (frames[i].type) {
				case XT_FRAME_TYPE_KEY:
					timeInfo.frameCount++;
					if (timeInfo.frameCount == 0) {
						timeInfo.startTime = xt_tickcount64() / 1000;
						timeInfo.ptsTime = 0;
					}
					else {
						timeInfo.ptsTime = timeInfo.frameCount * 1000 / fps;
						waitTime(timeInfo);
					}
					frame.seqNo = (uint32_t)timeInfo.frameCount;
					streamBuffer().putPacket(&frame, frames[i].pdata, frames[i].len);
					LOGGER_Error("send %d key frame %lld, i count %lld", frames[i].len, timeInfo.ptsTime, timeInfo.frameCount);
					break;
				case XT_FRAME_TYPE_EXTEND:
					streamBuffer().putPacket(&frame, frames[i].pdata, frames[i].len);
					LOGGER_Error("send extend frame");
					break;
				case XT_FRAME_TYPE_CORE:
					//sps pps
					if (frames[i + 1].type == XT_FRAME_TYPE_CORE) {
						streamBuffer().putPacket(&frame, frames[i].pdata, frames[i].len + frames[i + 1].len);
						LOGGER_Error("send %d core frame", frames[i].len + frames[i + 1].len);
						i++;
					}
					break;
				case XT_FRAME_TYPE_DATA:
				case XT_FRAME_TYPE_DATA_LOW:
					if (timeInfo.frameCount >= 0) {
						timeInfo.frameCount++;
						//frame.seqNo = (uint32_t)timeInfo.iFrameCount;
						timeInfo.ptsTime += timeInfo.stepTime;
						waitTime(timeInfo);
						streamBuffer().putPacket(&frame, frames[i].pdata, frames[i].len);
						LOGGER_Error("send %d data frame %lld", frames[i].len, timeInfo.ptsTime);
					}
					break;
				}
			}//end for

			//保留最后一帧不完整的数据
			if (fend) {
				fseek(m_pFile, 0, SEEK_SET);
				remain = 0;
			}
			else {
				memcpy(pdata, frames[fc - 1].pdata, frames[fc - 1].len);
				remain = frames[fc - 1].len;
			}
		}
	}
	m_threadEncoderExit = true;
	delete[]pdata;
}

void ContextPublisher::onThreadSender() {
	while (m_threadRunnig) {
		if (!m_stream.hasData()) {
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
			continue;
		}
		else {
			uint32_t dataLen;
			stream_frame_t *frame;
			uint8_t *p = m_stream.getPacket(&frame, &dataLen);
			//LOGGER_Debug("before send frame type %d len %d", frame->type, dataLen);
			//发送数据
			if (m_rollback) {
				m_rollback->onData(frame, p, dataLen);
			}
			if (m_missionObject) {
				if (frame->media == 0 && frame->type == XT_FRAME_TYPE_KEY) {
					uint8_t stream = frame->stream;
					uint8_t media = frame->media;
					p -= sizeof(xp_video_keyframe_t);
					dataLen += sizeof(xp_video_keyframe_t);
					{
						xp_video_keyframe_t *kf = (xp_video_keyframe_t *)p;
						h2n_u32(frame->seqNo, kf->seqNo);
					}
					m_missionObject->sendData(stream, media, XT_FRAME_TYPE_KEY, p, dataLen);
				}
				else {
					m_missionObject->sendData(frame->stream, frame->media, frame->type, p, dataLen);
				}
			}
			m_stream.offsetPacket();
		}
	}
	m_threadSenderExit = true;
}


