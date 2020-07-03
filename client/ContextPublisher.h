#pragma once

#include "capture/DeviceInclude.h"
#include "capture/VideoCapture.h"
#include "capture/VideoSource.h"
#include "encoder/VideoEncoder.h"
#include "MediaFrameFactory.h"
#include "D3D9Render.h"
#include "StreamBuffer.h"

class ContextSubscriber;
class MissionObject;
class ContextPublisher
{
	class MediaSinkHolder : public MediaSourceSink {
	public:
		MediaSinkHolder() { NODE_LIST_INIT(m_header, m_tailer); }
		void setOwner(ContextPublisher *p) { m_powner = p; }
		virtual bool onFrame(double dblSampleTime, media_frame_header_t *frame) {
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
		ContextPublisher *m_powner;
		media_frame_header_t m_header;
		media_frame_header_t m_tailer;
		std::mutex m_lock;
	};
public:
	ContextPublisher();
	~ContextPublisher();

	void setMissionObject(MissionObject *p) { m_missionObject = p; }
	void setRollback(ContextSubscriber *p) { m_rollback = p; }
	void setWindowShow(HWND hShow) {m_wndShow = hShow;}
	void setStreamId(int32_t id) { m_streamId = id; }
	int32_t getStreamId() { return m_streamId; }
	StreamBuffer &streamBuffer() { return m_stream; }

	bool startCapture(const wchar_t *devname, HWND hWndCapture, int no=0, int fps=25);
	bool startFile(const char *filename);
	void stopCapture();

private:
	void createEncoderThread();
	void onThreadEncoder();
	void createFileThread(int type, int fps, int width, int height);
	void onThreadFile(int type, int fps, int width, int height);
	void createSenderThread();
	void onThreadSender();
private:
	//HWND m_wndCapture = NULL;
	HWND m_wndShow = NULL;
	FILE *m_pFile = NULL;
	ContextSubscriber *m_rollback = NULL;
	MissionObject	*m_missionObject = NULL;
	VideoCapture	*m_videoCapture = NULL;
	VideoSource		m_videoSource;
	MediaFrameFactory m_mediaFact;
	VideoEncoder	m_encoder;
	StreamBuffer	m_stream;
	MediaSinkHolder m_holder;
	CD3D9Render		m_render;
	int32_t m_streamId = 0;
	volatile bool m_threadRunnig = false;
	volatile bool m_threadEncoderExit = true;
	volatile bool m_threadSenderExit = true;
};

