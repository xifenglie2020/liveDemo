#include "VideoCapture.h"
#include "VideoCaptureInfo.h"
#include "DeviceManager.h"
#include "../logger.h"
#include <atlbase.h>
#include <atlcom.h>
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <d3d9.h>
#include <vmr9.h>

LOGGER_Declare("VideoCapture");

//#define SINK_FILTER_NAME	L"SinkFilter"

STDMETHODIMP VideoCapture::CSampleGrabberCallBack::QueryInterface(REFIID riid, void ** ppv) {
	if (riid == IID_ISampleGrabberCB || riid == IID_IUnknown) {
		*ppv = this;
		return NOERROR;
	}
	return E_NOINTERFACE;
}
STDMETHODIMP VideoCapture::CSampleGrabberCallBack::BufferCB(double dblSampleTime, BYTE* pBuffer, long lBufferSize) {
	if (!pBuffer) return E_POINTER;
	if (frameHandler) frameHandler->onFrame(dblSampleTime, pBuffer, lBufferSize);
	return 0;
}

static void myFreeMediaType(AM_MEDIA_TYPE *mt) {
	if (mt->cbFormat != 0) {
		CoTaskMemFree((PVOID)mt->pbFormat);
		mt->cbFormat = 0;
		mt->pbFormat = NULL;
	}
	if (mt->pUnk != NULL) {
		// Unecessary because pUnk should not be used, but safest.
		mt->pUnk->Release();
		mt->pUnk = NULL;
	}
}
static void myDeleteMediaType(AM_MEDIA_TYPE *mt) {
	myFreeMediaType(mt);
	CoTaskMemFree(mt);
}

VideoCapture::VideoCapture()
{
	memset(&m_graph, 0, sizeof(m_graph));
}


VideoCapture::~VideoCapture()
{
}

int VideoCapture::open(DeviceManager *pMgr, const wchar_t *deviceName, 
	struct video_capture_info_ext *pinfo, HWND hWnd, int no, int fps) {
	m_pDevMgr = pMgr;

	do {
		int ret = createDirectShowGraph(pMgr, deviceName, &m_graph);
		if (0 != ret) {
			break;
		}
		HRESULT hr;

		hr = useCaptureInfo(pinfo, no, fps);
		if (FAILED(hr)) {
			LOGGER_Error("useCaptureInfo failed: %X", hr);
			break;
		}

		hr = setupGrabber();
		if (FAILED(hr)) {
			break;
		}

		hr = setupWindowlessVMR();
		if (FAILED(hr)) {
			break;
		}
		hr = m_graph.graphBuilder->QueryInterface(IID_IMediaControl, (void**)&m_mediaControl);
		if (FAILED(hr)) {
			LOGGER_Error("m_graphBuilder->QueryInterface IID_IMediaControl failed: %X", hr);
			break;
		}

		hr = m_graph.graphBuilder->QueryInterface(IID_IVideoWindow, (LPVOID *)&m_videoWindow);
		if (FAILED(hr)) {
			LOGGER_Error("create video Window failed: %x", hr);
			break;
		}

		if (hWnd != NULL) {
			//设置视频捕捉窗口
			hr = setupHWND(hWnd);
			if (FAILED(hr)) {
				break;
			}
		}

		hr = m_mediaControl->Pause();
		if (FAILED(hr)) {
			LOGGER_Error("Failed to Pause the Capture device %X. Is it already occupied? ",hr);
			break;
		}
		LOGGER_Info("Capture device initialized.");
		return 0;
	} while (0);
	close();
	return -1;
}

HRESULT VideoCapture::setupGrabber() {
	ISampleGrabber *grabber;
	HRESULT hr = CoCreateInstance(CLSID_SampleGrabber, NULL, 
		CLSCTX_INPROC_SERVER, IID_ISampleGrabber, (void**)&grabber);
	if (FAILED(hr)) {
		LOGGER_Error("CoCreateInstance create SampleGrabber failed: %X, maybe qedit.dll is not registered?", hr);
		return hr;
	}
	do {
		CComQIPtr<IBaseFilter, &IID_IBaseFilter> grabBase(grabber);
		hr = m_graph.graphBuilder->AddFilter(grabBase, L"Grabber");
		if (FAILED(hr)) {
			LOGGER_Error("m_graphBuilder->AddFilter Grabber failed: %X", hr);
			break;
		}
		// try to render preview/capture pin
		hr = m_graph.captureBuilder->RenderStream(&PIN_CATEGORY_PREVIEW, &MEDIATYPE_Video, m_graph.baseFilter, grabBase, NULL);
		if (FAILED(hr)) {
			hr = m_graph.captureBuilder->RenderStream(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Video, m_graph.baseFilter, grabBase, NULL);
			if (FAILED(hr)) {
				LOGGER_Error("m_captureBuilder->RenderStream failed: %X", hr);
				break;
			}
		}
		hr = grabber->SetBufferSamples(FALSE);
		hr = grabber->SetOneShot(FALSE);
		hr = grabber->SetCallback(&m_callback, 1);
		if (FAILED(hr)) {
			LOGGER_Error("grabber->SetCallback failed: %X", hr);
			break;
		}
		m_grabber = grabber;
		return S_OK;
	} while (0);
	SAFE_RELEASE(grabber);
	return hr;
}

HRESULT VideoCapture::unsetGrabber() {
	SAFE_RELEASE(m_grabber);
	return S_OK;
}

void VideoCapture::setSink(VideoCaptureSink *sink) {
	m_callback.frameHandler = sink;
}

#if 0
int VideoCapture::setWindow(void *hWnd) {
	if (m_videoWindow == NULL) {
		m_hWnd = hWnd;
	}
	else if(m_hWnd != hWnd){
		HRESULT hr;
		hr = unsetHWND();
		if (hWnd != NULL) {
			hr = setupHWND(hWnd);
		}
		if (FAILED(hr)) {
			LOGGER_Error("set window %p failed: %x", hWnd, hr);
			return -1;
		}
		m_hWnd = hWnd;
	}
	return 0;
}
#endif

HRESULT VideoCapture::setupHWND(void *hWnd) {
	HRESULT hr;
	hr = m_videoWindow->put_Owner((OAHWND)hWnd);
	if (FAILED(hr)) {
		LOGGER_Error("m_videoWindow->put_Owner failed: %x", hr);
		return hr;
	}
	//DWORD style = GetWindowStyle((HWND)hWnd);
	//hr = m_videoWindow->put_WindowStyle(style/*WS_CHILD | WS_CLIPCHILDREN*/);
	hr = m_videoWindow->put_WindowStyle(WS_CHILD | WS_CLIPCHILDREN);
	if (FAILED(hr)) {
		LOGGER_Error("m_videoWindow->put_WindowStyle failed: %x", hr);
		return hr;
	}
#if 0	//不设置可见了
	hr = m_videoWindow->put_Visible(OATRUE);
	if (FAILED(hr)) {
		LOGGER_Error("m_videoWindow->put_Visible failed: %x", hr);
		return hr;
	}
#endif
	m_hWnd = hWnd;
	return hr;
}

HRESULT VideoCapture::unsetHWND() {
	if (m_videoWindow != NULL) {
		HRESULT hr = m_videoWindow->put_Visible(OAFALSE);
		hr = m_videoWindow->put_Owner(NULL);
		if (FAILED(hr)) {
			LOGGER_Error("m_videoWindow->put_Owner NULL failed: %x", hr);
			return hr;
		}
	}
	m_hWnd = NULL;
	return S_OK;
}

HRESULT VideoCapture::setupWindowlessVMR(){
	IBaseFilter* pVmr = NULL;
	IVMRWindowlessControl* pWc = NULL;
	// 创建VMR
	HRESULT hr = CoCreateInstance(CLSID_VideoMixingRenderer, NULL,
		CLSCTX_INPROC, IID_IBaseFilter, (void**)&pVmr);
	if (FAILED(hr)){
		LOGGER_Error("CoCreateInstance CLSID_VideoMixingRenderer failed: %x", hr);
		return hr;
	}
	// 把VMR添加到过滤器图表中
	hr = m_graph.graphBuilder->AddFilter(pVmr, L"Video Mixing Renderer");
	if (FAILED(hr)){
		LOGGER_Error("graphBuilder AddFilter Mixing Renderer failed: %x", hr);
		pVmr->Release();
		return hr;
	}

	// 设置显示模式
	IVMRFilterConfig* pConfig;
	hr = pVmr->QueryInterface(IID_IVMRFilterConfig, (void**)&pConfig);
	if (SUCCEEDED(hr)){
		hr = pConfig->SetRenderingMode(VMRMode_Windowless);
		pConfig->Release();
	}
	else {
		LOGGER_Error("pVmr->QueryInterface failed: %x", hr);
	}
	pVmr->Release();
	return hr;
}


void VideoCapture::onWndSizeChanged() {
	if (m_videoWindow != NULL && m_hWnd != NULL) {
		//让图像充满整个窗口
		RECT rc;
		::GetClientRect((HWND)m_hWnd, &rc);
		m_videoWindow->SetWindowPosition(0, 0, rc.right, rc.bottom);
	}
}


void VideoCapture::close() {
	stop();
	unsetHWND();
	SAFE_RELEASE(m_videoWindow);
	SAFE_RELEASE(m_mediaControl);
	unsetGrabber();
	destroyDirectShowGraph(&m_graph);
}


int VideoCapture::start() {
	if (m_mediaControl != NULL) {
		HRESULT hr = m_mediaControl->Run();
		if (SUCCEEDED(hr)) {
			return 0;
		}
		LOGGER_Error("m_mediaControl->Run failed: %x", hr);
	}
	return -1;
}

void VideoCapture::stop() {
	if (m_mediaControl != NULL) {
		m_mediaControl->StopWhenReady();
	}
	//if (m_videoWindow != NULL) {
	//	m_videoWindow->put_Visible(OAFALSE);
	//}
}

HRESULT VideoCapture::useCaptureInfo(struct video_capture_info_ext *pinfo, int no, int fps) {
	IAMStreamConfig *pConfig = NULL;
	HRESULT hr = m_graph.captureBuilder->FindInterface(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Video,
		m_graph.baseFilter, IID_IAMStreamConfig, (void **)&pConfig);
	if (FAILED(hr)) {
		LOGGER_Error("m_captureBuilder->FindInterface failed: %x", hr);
		return hr;
	}
	video_capture_info_ext ci;
	memcpy(&ci, pinfo, sizeof(ci));
	ci.mt.pbFormat = ci.pbFormat;
	AM_MEDIA_TYPE *mt = &ci.mt;
	if (fps > 0) {
		if (mt->formattype == FORMAT_VideoInfo) {
			VIDEOINFOHEADER *pHd = (VIDEOINFOHEADER *)mt->pbFormat;
			// 单位为100ns，所以每帧(10^7/p3)*100ns
			pHd->AvgTimePerFrame = 10000000 / fps;
			pHd->dwBitRate = pHd->bmiHeader.biSizeImage * 8 * fps;	// 图像传输率，单位bps
		}
		else if (mt->formattype == FORMAT_VideoInfo2) {
			VIDEOINFOHEADER2 *pHd = (VIDEOINFOHEADER2 *)mt->pbFormat;
			pHd->AvgTimePerFrame = 10000000 / fps;
			pHd->dwBitRate = pHd->bmiHeader.biSizeImage * 8 * fps;	// 图像传输率，单位bps
		}
		else {
			LOGGER_Error("unknown format");
			pConfig->Release();
			return E_INVALIDARG;
		}
	}
	hr = pConfig->SetFormat(mt);
	if (FAILED(hr)) {
		LOGGER_Error("pConfig->SetFormat failed: %x", hr);
		pConfig->Release();
		return hr;
	}
	pConfig->Release();

	//保存信息
	m_captureInfo.fmt = pinfo->fmt;
	m_captureInfo.deviceNo = no;
	m_captureInfo.fps = (fps > 0) ? fps : pinfo->fps;
	m_captureInfo.width = pinfo->width;
	m_captureInfo.height = pinfo->height;

	return S_OK;
}

