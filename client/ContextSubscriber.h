#pragma once

#include "decoder/VideoDecoder.h"
#include "MediaFrameFactory.h"
#include "D3D9Render.h"
#include "StreamBuffer.h"
#include "nodelist.h"

//#define SAVE_FILE

class ContextSubscriber
{
	class MediaFrameHolder{
	public:
		MediaFrameHolder() { NODE_LIST_INIT(m_header, m_tailer); }
		void setOwner(ContextSubscriber *p) { m_powner = p; }
		bool onFrame(media_frame_header_t *frame) {
			m_lock.lock();
			NODE_LIST_ADD_TAIL(m_tailer, frame);
			m_lock.unlock();
			return true;
		}
		media_frame_header_t *getFrameLists() {
			media_frame_header_t *p = NULL;
			if (m_header.next != &m_tailer) {
				m_lock.lock();
				p = m_header.next;
				m_tailer.prev->next = NULL;
				m_header.next = &m_tailer;
				m_tailer.prev = &m_header;
				m_lock.unlock();
			}
			return p;
		}
	private:
		ContextSubscriber *m_powner;
		media_frame_header_t m_header;
		media_frame_header_t m_tailer;
		std::mutex m_lock;
	};
public:
	ContextSubscriber();
	~ContextSubscriber();

	bool startShow(HWND hWndShow);
	void stopShow();

	void onHeader(struct xp_media_header_t *);
	void onData(stream_frame_t *frame, const uint8_t *pdata, int32_t len);
private:
	void createDecoderThread();
	void onThreadDecoder();
	void createRenderThread();
	void onThreadRender();
private:
#ifdef SAVE_FILE
	FILE *m_pFile = NULL;
#endif
	HWND m_wndShow = NULL;
	MediaFrameFactory m_mediaFact;
	VideoDecoder	m_decoder;
	CD3D9Render		m_render;
	StreamBuffer	m_stream;
	MediaFrameHolder m_holder;
	volatile bool m_threadRunnig = false;
	volatile bool m_threadDecoderExit = true;
	volatile bool m_threadRenderExit = true;
	uint32_t m_fps = 0;
	uint32_t m_gop = 0;
	uint64_t m_startTime = 0;
};

