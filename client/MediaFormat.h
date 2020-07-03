#pragma once

#include <stdlib.h>
#include <stdint.h>

enum eVideoFormat {
	eVideoFormatNone,
	eVideoFormatMJPG,
	eVideoFormatYUY2,	//4:2:2  [Y0 U0 Y1 V0]  YUVY 
	eVideoFormatI420,
	eVideoFormatNV12,
	//eVideoFormatYV12,
	//eVideoFormatIYUV,
	//eVideoFormatUYUV,
};

struct video_capture_into_t {
	eVideoFormat fmt;
	int32_t fps;
	int32_t width;
	int32_t height;
	int32_t deviceNo;
};

class VideoCaptureSink {
public:
	virtual void onFrame(double dblSampleTime, uint8_t* pBuffer, int32_t lBufferSize) = 0;
};

typedef void(*videoCaptureEnumCallbackFunc)(void *userdata, int32_t no,
	const char *format, int32_t minFps, int32_t maxFps, int32_t width, int32_t height);


struct video_frame_data_t {
	int32_t width;
	int32_t height;
	uint8_t *data[4];	//y v u all  a r g b
	int32_t line[4];
};

//Ŀǰ�������ü�����
class MediaFrameFactory;
struct media_frame_header_t {
	media_frame_header_t *next;
	media_frame_header_t *prev;
	MediaFrameFactory *creator;
	int8_t  video;	//video or audio
	int8_t  type;	//XT_FRAME_TYPE_KEY ...
	int16_t stream;	//��id
	uint64_t seqOrPts;	//����� ���� pts
	union {
		video_frame_data_t video;
	}u;
};

class MediaSourceSink {
public:
	//���� true ��ʾframe�����Ѿ���ռ��
	virtual bool onFrame(double dblSampleTime, media_frame_header_t *frame) = 0;

};



