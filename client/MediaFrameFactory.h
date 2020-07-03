#pragma once

#include "MediaFormat.h"
#include <mutex>

class MediaFrameFactory
{
public:
	MediaFrameFactory();
	~MediaFrameFactory();

	bool init(int32_t maxCount, int32_t frameSize, int32_t createCount);
	void fini();

	media_frame_header_t *alloc();
	void free(media_frame_header_t *pdata);

protected:
	media_frame_header_t *createOne();
protected:
	struct node_t {
		node_t *next;
		node_t *prev;
		char data[1];
	};
	int32_t m_frameSize = 0;
	int32_t m_maxCount = 0;
	int32_t m_createCount = 0;
	media_frame_header_t m_header;
	media_frame_header_t m_tailer;
	std::mutex m_lock;
	node_t m_alloced_header;
	node_t m_alloced_tailer;
};

