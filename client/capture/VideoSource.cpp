#include "VideoSource.h"
#include "VideoCapture.h"
#include "../MediaFrameFactory.h"
#include "libyuv.h"

VideoSource::VideoSource()
{
	memset(m_sinks, 0, sizeof(m_sinks));
}

VideoSource::~VideoSource()
{
}

void VideoSource::reset() {
	memset(m_sinks, 0, sizeof(m_sinks));
	m_needMirror = true;
	m_seqNo = 0;
}

int32_t VideoSource::getDataSize(int32_t width, int32_t height) {
	//转码为YUV
	int32_t size = width * height;
	return sizeof(media_frame_header_t) + size + (size >> 1) + 16;
}

bool VideoSource::addSink(MediaSourceSink *sink) {
	m_lock.lock();
	for (int i = 0; i < MAX_VIDEO_SOURCE_SINK_COUNT; i++) {
		if (m_sinks[i] == NULL) {
			m_sinks[i] = sink;
			m_lock.unlock();
			return true;
		}
	}
	m_lock.unlock();
	return false;
}

void VideoSource::delSink(MediaSourceSink *sink) {
	for (int i = 0; i < MAX_VIDEO_SOURCE_SINK_COUNT; i++) {
		if (m_sinks[i] == sink) {
			m_sinks[i] = NULL;
			break;
		}
	}
}

static int convertVideoToI420(video_frame_data_t &video, 
	const video_capture_into_t *pinfo, uint8_t* pBuffer, int32_t lBufferSize) {
	int ret;
	switch (pinfo->fmt) {
	case eVideoFormatMJPG:
		ret = libyuv::MJPGToI420(pBuffer, lBufferSize,
			video.data[0], video.line[0], video.data[1], video.line[1], video.data[2], video.line[2],
			pinfo->width, pinfo->height, pinfo->width, pinfo->height);
		break;
	case eVideoFormatYUY2:
		ret = libyuv::YUY2ToI420(pBuffer, pinfo->width,
			video.data[0], video.line[0], video.data[1], video.line[1], video.data[2], video.line[2],
			pinfo->width, pinfo->height);
		break;
	case eVideoFormatI420:
		do {
			uint32_t usize = pinfo->width * pinfo->height;
			uint8_t *su = pBuffer + usize;
			uint8_t *sv = su + (usize >> 2);
			ret = libyuv::I420Copy(pBuffer, pinfo->width, su, (pinfo->width >> 1), sv, (pinfo->width >> 1),
				video.data[0], video.line[0], video.data[1], video.line[1], video.data[2], video.line[2],
				pinfo->width, pinfo->height);
		} while (0);
		break;
	case eVideoFormatNV12:
		ret = libyuv::NV12ToI420(pBuffer, pinfo->width, pBuffer + pinfo->width*pinfo->height, pinfo->width / 2,
			video.data[0], video.line[0], video.data[1], video.line[1], video.data[2], video.line[2],
			pinfo->width, pinfo->height);
			break;
	default:
		ret = -1;
		break;
	}
	return ret;
}

static void I420MirrorRow(uint8_t *data, int size) {
	for (uint8_t *end = data + size - 1; data < end; data++,end--) {
		uint8_t c = *end;
		*end = *data;
		*data = c;
	}
}
static void I420MirrorData(uint8_t *data, int width, int height) {
	for (int i = 0; i < height; i++, data+=width) {
		I420MirrorRow(data, width);
	}
}

void VideoSource::onFrame(double dblSampleTime, uint8_t* pBuffer, int32_t lBufferSize) {
	media_frame_header_t *frame = (media_frame_header_t *)m_frameFact->alloc();
	if (frame != NULL) {
		int ret;
		const video_capture_into_t *pinfo = m_pDevice->captureInfo();
		video_frame_data_t &video = frame->u.video;

		//转成420
		frame->video = 1;
		frame->type = eVideoFormatI420;
		frame->stream = 0;
		frame->seqOrPts = m_seqNo++;
		video.width = pinfo->width;
		video.height = pinfo->height;

		video.data[0] = ((uint8_t*)frame) + sizeof(media_frame_header_t);
		video.data[1] = video.data[0] + pinfo->width * pinfo->height;  
		video.data[2] = video.data[1] + ((pinfo->width * pinfo->height) >> 2);
		video.data[3] = video.data[0];	//all
		video.line[0] = pinfo->width;
		video.line[1] = pinfo->width >> 1;
		video.line[2] = pinfo->width >> 1;
		video.line[3] = pinfo->width;

		ret = convertVideoToI420(video, pinfo, pBuffer, lBufferSize);
		if (ret >= 0) {
			if (m_needMirror) {
				I420MirrorData(video.data[0], video.line[0], video.height);
				I420MirrorData(video.data[1], video.line[1], video.height >> 1);
				I420MirrorData(video.data[2], video.line[2], video.height >> 1);
			}
			bool fret = false;
			MediaSourceSink *sink;
			for (int i = 0; i < MAX_VIDEO_SOURCE_SINK_COUNT; i++) {
				if ((sink = m_sinks[i]) != NULL) {
					if (sink->onFrame(dblSampleTime, frame)) {
						if (!fret)	fret = true;
					}
				}
			}
			if (fret) {
				return;
			}
		}
		m_frameFact->free(frame);
	}
}
