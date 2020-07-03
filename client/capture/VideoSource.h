#pragma once

#include "../MediaFormat.h"
#include "../nodelist.h"
#include <mutex>

#define MAX_VIDEO_SOURCE_SINK_COUNT	4

class VideoCapture;
class VideoSource : public VideoCaptureSink
{
public:
	VideoSource();
	~VideoSource();

	virtual void onFrame(double dblSampleTime, uint8_t* pBuffer, int32_t lBufferSize);
	void setDevice(VideoCapture *dev) { m_pDevice = dev; }
	void setFactory(MediaFrameFactory *fact) { m_frameFact = fact; }
	//水平翻转
	void setMirror(bool f) { m_needMirror = f; }

	bool addSink(MediaSourceSink *sink);
	void delSink(MediaSourceSink *sink);

	int32_t getDataSize(int32_t width, int32_t height);
	//重置
	void reset();
protected:
	MediaFrameFactory *m_frameFact = NULL;
	VideoCapture *m_pDevice = NULL;
	bool m_needMirror = true;
	uint64_t m_seqNo = 0;
	MediaSourceSink *m_sinks[MAX_VIDEO_SOURCE_SINK_COUNT];
	std::mutex m_lock;
};

