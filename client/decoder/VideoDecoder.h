#pragma once

#include <stdint.h>

struct raw_av_codec_params_t{
	uint64_t pts;
	uint64_t dts;
};

//#define TEST_WRITE_AUDIO_FILE

typedef void(*h26xDecodeDataCallback)(void *userdata, uint64_t timeMs, struct video_frame_data_t *);

class VideoDecoder
{
public:
	VideoDecoder(void);
	~VideoDecoder(void);

public:
	bool Create(int type, int width, int height, int fps, int gop);
	void Destroy();
	bool Decode(unsigned char *data, int len, int frameType, uint32_t frameNo,
		h26xDecodeDataCallback func, void *userdata);
private:
	struct AVCodec *m_avcodec;
	struct AVStream *m_avstream;
	//struct AVPacket *m_avpacket;
	int		m_width;
	int		m_height;
	int		m_fps;
	//int		m_gop;
	uint64_t m_lastPtsMs;
	uint64_t m_lastICount;
	uint64_t m_firstICount;
};

