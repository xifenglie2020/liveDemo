#pragma once

#include "DeviceInclude.h"
#include "../MediaFormat.h"

class VideoCapture
{
	class CSampleGrabberCallBack : public ISampleGrabberCB{
	public:
		VideoCaptureSink * frameHandler = NULL;
	public:
		CSampleGrabberCallBack() {}
		STDMETHODIMP_(ULONG) AddRef()  { return 2; }
		STDMETHODIMP_(ULONG) Release() { return 1; }
		STDMETHODIMP QueryInterface(REFIID riid, void ** ppv);
		STDMETHODIMP SampleCB(double SampleTime, IMediaSample * pSample) {return 0;}
		STDMETHODIMP BufferCB(double dblSampleTime, BYTE * pBuffer, long lBufferSize);
	};
	friend class DeviceManager;
public:
	VideoCapture();
	~VideoCapture();

	const video_capture_into_t *captureInfo() const { return &m_captureInfo; }
	//int  setWindow(void *hWnd);
	void onWndSizeChanged();
	void setSink(VideoCaptureSink *);

	//return 0 means ok
	int  start();
	void stop();

protected:
	//return 0 means ok
	int  open(DeviceManager *pMgr, const wchar_t *deviceName, 
		struct video_capture_info_ext *pinfo, HWND hWnd, int no, int fps);
	void close();

	HRESULT setupGrabber();
	HRESULT unsetGrabber();

	HRESULT setupHWND(void *hWnd);
	HRESULT unsetHWND();

	HRESULT setupWindowlessVMR();

	HRESULT useCaptureInfo(struct video_capture_info_ext *pinfo, int no, int fps);

private:
	DeviceManager *m_pDevMgr = NULL;
	direct_show_base_graph_t m_graph;
	IMediaControl*  m_mediaControl = NULL;
	ISampleGrabber* m_grabber = NULL;
	IVideoWindow*   m_videoWindow = NULL;
	void *m_hWnd = NULL;
	CSampleGrabberCallBack m_callback;
	video_capture_into_t  m_captureInfo;
};
