#include "MediaFrameFactory.h"
#include "nodelist.h"

MediaFrameFactory::MediaFrameFactory()
{
	NODE_LIST_INIT(m_header, m_tailer);
	NODE_LIST_INIT(m_alloced_header, m_alloced_tailer);
}


MediaFrameFactory::~MediaFrameFactory()
{
}

bool MediaFrameFactory::init(int32_t maxCount, int32_t frameSize, int32_t createCount) {
	m_lock.lock();
	m_maxCount = maxCount;
	m_frameSize = frameSize;
	m_createCount = 0;
	NODE_LIST_INIT(m_header, m_tailer);
	if (createCount > 0) {
		for (int i = 0; i < createCount; i++) {
			media_frame_header_t *p = createOne();
			if (p == NULL) {
				m_lock.unlock();
				return false;
			}
			NODE_LIST_ADD_TAIL(m_tailer, p);
		}
	}
	m_lock.unlock();
	return true;
}

media_frame_header_t *MediaFrameFactory::createOne() {
	node_t *node = (node_t *)malloc(sizeof(node_t) + m_frameSize);
	if (node != NULL) {
		media_frame_header_t *p = (media_frame_header_t *)node->data;
		NODE_LIST_ADD_TAIL(m_alloced_tailer, node);
		p->creator = this;
		m_createCount++;
		return p;
	}
	return NULL;
}

void MediaFrameFactory::fini() {
	m_lock.lock();
	m_maxCount = 0;
	m_createCount = 0;
	NODE_LIST_INIT(m_header, m_tailer);
	while (m_alloced_header.next != &m_alloced_tailer) {
		node_t *p = m_alloced_header.next;
		NODE_LIST_REMOVE(p);
		::free(p);
	}
	m_lock.unlock();
}

media_frame_header_t *MediaFrameFactory::alloc() {
	media_frame_header_t *p = NULL;
	if (m_header.next != &m_tailer) {
		m_lock.lock();
		if (m_header.next != &m_tailer) {
			p = m_header.next;
			NODE_LIST_REMOVE(p);
		}
		m_lock.unlock();
	}
	else if (m_createCount < m_maxCount) {
		m_lock.lock();
		p = createOne();
		m_lock.unlock();
	}
	return p;
}

void MediaFrameFactory::free(media_frame_header_t *pdata) {
	m_lock.lock();
	if (m_maxCount > 0) {
		NODE_LIST_ADD_TAIL(m_tailer, pdata);
	}
	m_lock.unlock();
}

