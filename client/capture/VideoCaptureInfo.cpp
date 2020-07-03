#include "VideoCaptureInfo.h"
#include "DeviceManager.h"
#include "../logger.h"

LOGGER_Declare("VideoCaptureInfo");

static struct {
	eVideoFormat fmt;
	const GUID *pguid;
	const char *desc;
}gCaptureFormats[] = {
	{ eVideoFormatMJPG, &MEDIASUBTYPE_MJPG, "MJPG" },
	{ eVideoFormatYUY2, &MEDIASUBTYPE_YUY2, "YUY2" },
	{ eVideoFormatI420, &WMMEDIASUBTYPE_I420, "I420" },
	{ eVideoFormatNV12, &MEDIASUBTYPE_NV12, "NV12" },
	//{ eVideoFormatYV12, &MEDIASUBTYPE_YV12, "YV12" },
	//{ eVideoFormatIYUV, &MEDIASUBTYPE_IYUV, "IYUV" },
	//{ eVideoFormatUYUV, &MEDIASUBTYPE_UYVY, "UYUV" },
	{ eVideoFormatNone, &MEDIASUBTYPE_None, "None" },
};

static int getFps(LONGLONG val) {
	int x = 10000000 / val;
	int x2 = 10000000 % val;
	return (x2 >= val / 2) ? (x + 1) : x;
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

VideoCaptureInfo::VideoCaptureInfo() {
}

void VideoCaptureInfo::clearInfo(video_capture_list_t *lst) {
	if (lst->name != NULL) {
		lst->captureLists = NULL;
		lst->captureCount = 0;
		delete[]lst->name;
		lst->name = NULL;
	}
}

void VideoCaptureInfo::cleanup() { 
	for (std::vector<video_capture_list_t>::iterator it = m_lists.begin(); 
		it != m_lists.end(); ++it) {
		clearInfo(&(*it));
	}
	m_lists.clear();
}

int VideoCaptureInfo::createCaptureLists(DeviceManager *mgr, const wchar_t *name) {
	video_capture_list_t lst = { 0 };
	int ret = buildCaptureInfo(mgr, name, &lst);
	if (ret >= 0) {
		m_lists.push_back(lst);
	}
	return ret;
}

int VideoCaptureInfo::checkedCreateCaptureLists(DeviceManager *mgr, const wchar_t *name) {
	video_capture_list_t *lst = find(name);
	return lst != NULL ? lst->captureCount : createCaptureLists(mgr, name);
}

video_capture_list_t *VideoCaptureInfo::find(const wchar_t *name) {
	for (std::vector<video_capture_list_t>::iterator it = m_lists.begin();
		it != m_lists.end(); ++it) {
		if (wcscmp(name, it->name) == 0) {
			return &(*it);
		}
	}
	return NULL;
}
video_capture_info_ext *VideoCaptureInfo::peer(video_capture_list_t *lst, int pos) {
	return pos >= lst->captureCount ? NULL : &lst->captureLists[pos];
}

video_capture_info_ext *VideoCaptureInfo::peer(const wchar_t *name, int pos) {
	video_capture_list_t *lst = find(name);
	return lst != NULL ? peer(lst, pos) : NULL;
}

int VideoCaptureInfo::enumCaptureInfo(const wchar_t *name, videoCaptureEnumCallbackFunc func, void *userdata) {
	video_capture_list_t *lst = find(name);
	return lst != NULL ? enumCaptureInfo2(lst, func, userdata) : -1;
}

int VideoCaptureInfo::enumCaptureInfo2(video_capture_list_t *lst, videoCaptureEnumCallbackFunc func, void *userdata) {
	for (int i = 0; i < lst->captureCount; i++) {
		func(userdata, i, lst->captureLists[i].desc,
			lst->captureLists[i].minFps, lst->captureLists[i].maxFps, 
			lst->captureLists[i].width, lst->captureLists[i].height);
	}
	return lst->captureCount;
}

static bool copyMt2Info(video_capture_info_ext *pci, AM_MEDIA_TYPE *pmt) {
	for (int i = 0; gCaptureFormats[i].fmt != eVideoFormatNone; i++) {
		if (*gCaptureFormats[i].pguid == pmt->subtype) {
			pci->fmt = gCaptureFormats[i].fmt;
		}
	}
	if (pmt->formattype == FORMAT_VideoInfo) {
		AM_MEDIA_TYPE *pmt2 = &pci->mt;
		memcpy(pmt2, pmt, sizeof(AM_MEDIA_TYPE));
		pmt2->pUnk = NULL;
		pmt2->pbFormat = pci->pbFormat;
		memcpy(pmt2->pbFormat, pmt->pbFormat, pmt->cbFormat);

		VIDEOINFOHEADER* h = reinterpret_cast<VIDEOINFOHEADER*>(pmt->pbFormat);
		pci->fps = getFps(h->AvgTimePerFrame);
		pci->width = h->bmiHeader.biWidth;
		pci->height = h->bmiHeader.biHeight;
	}
	else if (pmt->formattype == FORMAT_VideoInfo2) {
		AM_MEDIA_TYPE *pmt2 = &pci->mt;
		memcpy(pmt2, pmt, sizeof(AM_MEDIA_TYPE));
		pmt2->pUnk = NULL;
		pmt2->pbFormat = pci->pbFormat;
		memcpy(pmt2->pbFormat, pmt->pbFormat, pmt->cbFormat);

		VIDEOINFOHEADER2* h = reinterpret_cast<VIDEOINFOHEADER2*>(pmt->pbFormat);
		//pci->interlaced = h->dwInterlaceFlags&(AMINTERLACE_IsInterlaced | AMINTERLACE_DisplayModeBobOnly);
		pci->fps = getFps(h->AvgTimePerFrame);
		pci->width = h->bmiHeader.biWidth;
		pci->height = h->bmiHeader.biHeight;
	}
	else {
		return false;
	}
	return true;
}

int VideoCaptureInfo::buildCaptureInfo(DeviceManager *mgr, const wchar_t *name, video_capture_list_t *lst) {
	direct_show_base_graph_t g;
	int ret;
	IAMStreamConfig *pConfig = NULL;
	do {
		ret = createDirectShowGraph(mgr, name, &g);
		if (ret != 0) {
			break;
		}
		HRESULT hr = g.captureBuilder->FindInterface(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Video,
			g.baseFilter, IID_IAMStreamConfig, reinterpret_cast<void**>(&pConfig)); // 得到媒体控制接口
		if (FAILED(hr)) {
			LOGGER_Error("m_captureBuilder->FindInterface failed: %x", hr);
			ret = -2;	
			break;
		}

		int nCount = 0;
		int nSize = 0;
		hr = pConfig->GetNumberOfCapabilities(&nCount, &nSize);
		if (FAILED(hr)) {
			LOGGER_Error("pConfig->GetNumberOfCapabilities failed: %x", hr);
			ret = -3;
			break;
		}

		size_t nameSize = wcslen(name);
		nameSize += 8 - (nameSize & 7);
		size_t totalSize = nameSize * 2 + (nCount * sizeof(video_capture_info_ext));
		lst->name = (wchar_t *)malloc(totalSize);
		if (lst->name == NULL) {
			LOGGER_Error("malloc data %d failed", totalSize);
			ret = -3;
			break;
		}
		memset(lst->name, 0, totalSize);
		wcscpy(lst->name, name);
		lst->captureLists = (video_capture_info_ext *)&lst->name[nameSize];

		VIDEO_STREAM_CONFIG_CAPS caps = { 0 };
		int realCount = 0;
		for (int i = 0; i < nCount; ++i) {
			AM_MEDIA_TYPE* pmt = NULL;
			hr = pConfig->GetStreamCaps(i, &pmt, reinterpret_cast<BYTE*>(&caps));
			if (FAILED(hr)) {
				continue;
			}
			video_capture_info_ext *pci = &lst->captureLists[realCount];
			if (copyMt2Info(pci, pmt)) {
				pci->minFps = getFps(caps.MaxFrameInterval);
				pci->maxFps = getFps(caps.MinFrameInterval);
				realCount++;
			}
			myFreeMediaType(pmt);
			pmt = NULL;
		}

		lst->captureCount = realCount;
		ret = realCount;
	} while (0);

	if (pConfig != NULL) {
		pConfig->Release();
	}
	destroyDirectShowGraph(&g);
	return ret;
}


int createDirectShowGraph(DeviceManager *mgr, const wchar_t *deviceName, direct_show_base_graph_t *graph) {
	memset(graph, 0, sizeof(direct_show_base_graph_t));
	do {
		graph->baseFilter = mgr->createBaseFilter(deviceName);
		if (graph->baseFilter == NULL) {
			LOGGER_Error("createBaseFilter failed");
			break;
		}
		// Get the interface for DirectShow's GraphBuilder
		HRESULT hr = CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER,
			IID_IGraphBuilder, (void**)&graph->graphBuilder);
		if (FAILED(hr)) {
			LOGGER_Error("CoCreateInstance create CLSID_FilterGraph failed: %X", hr);
			break;
		}
		// 创建ICaptureGraphBuilder2接口
		hr = CoCreateInstance(CLSID_CaptureGraphBuilder2, NULL, CLSCTX_INPROC,
			IID_ICaptureGraphBuilder2, (void **)&graph->captureBuilder);
		if (FAILED(hr)) {
			LOGGER_Error("CoCreateInstance create CLSID_CaptureGraphBuilder2 failed: %X", hr);
			break;
		}
		graph->captureBuilder->SetFiltergraph(graph->graphBuilder);

		hr = graph->graphBuilder->AddFilter(graph->baseFilter, L"VideoCaptureFilter");
		if (FAILED(hr)) {
			LOGGER_Error("m_graphBuilder->AddFilter IBaseFilter faield: %X", hr);
			break;
		}
		return 0;
	} while (0);
	destroyDirectShowGraph(graph);
	return -1;
}

void destroyDirectShowGraph(direct_show_base_graph_t *graph) {
	SAFE_RELEASE(graph->baseFilter);
	SAFE_RELEASE(graph->captureBuilder);
	SAFE_RELEASE(graph->graphBuilder);
}




