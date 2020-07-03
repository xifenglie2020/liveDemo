#pragma once

#include <stdint.h>

//frameNo 为数据的序号
typedef void(*h26xEncoderCallback)(void *userData, uint8_t *pdata, int len, int frameType, uint64_t keyFrameNo);

//const int32_t encoder_header_remain_len = 16;

struct h26x_frame_info_t {
	int32_t type;
	int32_t code;	//
	int32_t len;	//包含code长度
	uint8_t *pdata;
};
extern int parseH26xFrames(bool h264, unsigned char *pdata, int datalen, h26x_frame_info_t *frames, int size);

class VideoEncoder
{
public:
	VideoEncoder();
	~VideoEncoder();

	bool start(int32_t width, int32_t height, int32_t fps=25);
	void stop();
	bool inputFrame(unsigned char *srcData, int width, int stride, int height,
		h26xEncoderCallback func, void *userData);

private:
	bool makeContext(int width, int height);
	bool checkSize(int width, int height);
private:
	struct AVCodec *m_codec;
	struct AVCodecContext *m_context;
	int32_t  m_fps;
	int32_t  m_pixelFormat;
	uint64_t m_frameNo;
	uint64_t m_encFrameNo;
	struct {
		uint8_t *pdata;
		int32_t size;
	}m_cache;
};
