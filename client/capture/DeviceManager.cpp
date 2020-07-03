#include "DeviceManager.h"
#include "VideoCapture.h"
#include "VideoCaptureInfo.h"
#include "../logger.h"

LOGGER_Declare("DeviceManager");

int32_t DeviceProps::getProp(wchar_t *dest, int32_t destSize, const wchar_t *name) {
	int32_t ret;
	VARIANT var;
	VariantInit(&var);
	HRESULT hr = m_pBag->Read(name, &var, 0);
	if (FAILED(hr)) {
		ret = -2;
	}
	else {
		int32_t size = (int32_t)wcslen(var.bstrVal);
		if (size >= destSize) {
			ret = -1;
		}
		memcpy(dest, var.bstrVal, (size + 1) * sizeof(wchar_t));
		ret = size;
	}
	VariantClear(&var);
	return ret;
}

DeviceManager::DeviceManager()
{
}

DeviceManager::~DeviceManager()
{
}

int32_t DeviceManager::startup() {
	if (!m_fInited) {
#ifndef COINIT_CALLED
		HRESULT hr = CoInitializeEx(
			NULL, COINIT_MULTITHREADED);  // Use COINIT_MULTITHREADED since Voice
		if (FAILED(hr)) {
			// Avoid calling CoUninitialize() since CoInitializeEx() failed.
			if (hr == RPC_E_CHANGED_MODE) {
				// Calling thread has already initialized COM to be used in a
				// single-threaded apartment (STA). We are then prevented from using STA.
				// Details: hr = 0x80010106 <=> "Cannot change thread mode after it is
				// set".
			}
			LOGGER_Error("Failed to CoInitializeEx, error %X.", hr);
			return -1;
		}
#endif
		m_fInited = true;
	}
	if (m_captureInfo == NULL) {
		m_captureInfo = new VideoCaptureInfo();
		if (m_captureInfo == NULL) {
			LOGGER_Error("Create Capture Info Out of memory");
			return -1;
		}
	}
	if (m_devEnum == NULL) {
		HRESULT hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC,
			IID_ICreateDevEnum, (void**)&m_devEnum);
		if (hr != NOERROR) {
			LOGGER_Error("Failed to enumerate CLSID_SystemDeviceEnum, error %X.", hr);
			return -1;
		}
	}
	return 0;
}

void DeviceManager::cleanup() {
	if (m_devEnum != NULL) {
		m_devEnum->Release();
		m_devEnum = NULL;
	}
	if (m_captureInfo != NULL) {
		m_captureInfo->cleanup();
		delete m_captureInfo;
		m_captureInfo = NULL;
	}
	if (m_fInited) {
		m_fInited = false;
#ifndef COINIT_CALLED
		CoUninitialize();
#endif
	}
}


int32_t DeviceManager::enumVideoDevice(deviceEnumCallbackFunc func, void *userdata) {
	if (m_devEnum == NULL) {
		return -1;
	}
	// enumerate all video capture devices
	IEnumMoniker *dsMonikerDevEnum = NULL;
	HRESULT hr = m_devEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory,
		&dsMonikerDevEnum, 0);
	if (hr != NOERROR) {
		LOGGER_Error("Failed to enumerate CLSID_VideoInputDeviceCategory, error %X. No webcam exist?", hr);
		return -2;
	}
	dsMonikerDevEnum->Reset();
	ULONG cFetched;
	IMoniker* pM;
	int32_t ret = 0, index = 0;
	while (ret == 0 && S_OK == dsMonikerDevEnum->Next(1, &pM, &cFetched)) {
		IPropertyBag* pBag;
		hr = pM->BindToStorage(0, 0, IID_IPropertyBag, (void**)&pBag);
		if (S_OK == hr) {
			wchar_t name[512];
			DeviceProps prop(pBag);
			if (prop.getName(name, 512) >= 0 &&
				wcsstr(name, (L"(VFW)")) == NULL &&
				_wcsnicmp(name, (L"Google Camera Adapter"), 21) != 0) {
				ret = func(userdata, index++, name, pM, &prop);
			}
			pBag->Release();
			pM->Release();
		}
	}
	dsMonikerDevEnum->Release();
	return ret;
}

IBaseFilter *DeviceManager::createBaseFilter(const wchar_t *deviceName) {
	struct check_filter_t {
		const wchar_t *deviceName;
		IBaseFilter *pFilter;
	}cd = { deviceName,NULL };
	enumVideoDevice([](void *userdata, int32_t no, wchar_t *name, IMoniker *moniker, DeviceProps *prop)->int {
		check_filter_t &cd = *((check_filter_t*)userdata);
		if (wcscmp(name, cd.deviceName) == 0) {
			HRESULT hr = moniker->BindToObject(0, 0, IID_IBaseFilter, (void**)&cd.pFilter);
			if (FAILED(hr)) {
				LOGGER_Error("moniker->BindToObject failed: %x", hr);
			}
			return -1;
		}
		return 0;
	}, &cd);
	return cd.pFilter;
}

int32_t DeviceManager::enumVideoCaptureInfo(const wchar_t *name, videoCaptureEnumCallbackFunc func, void *userdata) {
	int ret = m_captureInfo->checkedCreateCaptureLists(this, name);
	if (ret > 0) {
		return m_captureInfo->enumCaptureInfo(name, func, userdata);
	}
	return ret;
}

VideoCapture *DeviceManager::createVideoCapture(const wchar_t *name, HWND hWnd, int no, int fps) {
	int ret = m_captureInfo->checkedCreateCaptureLists(this, name);
	if (ret > 0) {
		if (no < 0)	no = 0;
		if(no < ret){
			VideoCapture *pCapture;
			video_capture_info_ext *pext = m_captureInfo->peer(name, no);
			pCapture = new VideoCapture();
			ret = pCapture->open(this, name, pext, hWnd, no, fps);
			if (ret == 0) {
				return pCapture;
			}
			delete pCapture;
			LOGGER_Error("createVideoCapture open failed %d", ret);
		}
		else {
			LOGGER_Error("createVideoCapture params no %d invalid max(%d)", no, ret);
		}
	}
	return NULL;
}

void DeviceManager::destroyVideoCapture(VideoCapture *p) {
	p->close();
	delete p;
}

