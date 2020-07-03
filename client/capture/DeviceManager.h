#pragma once

#include "DeviceInclude.h"
#include "../MediaFormat.h"

class DeviceProps {
public:
	DeviceProps(IPropertyBag *bag) { m_pBag = bag; }
	//return -2:failed  -1:destSize too small  >=0:data length
	// Find the description or friendly name.
	int32_t getName(wchar_t *dest, int32_t destSize) {
		int32_t ret = getProp(dest, destSize, L"Description");
		if (ret == -2) {
			ret = getProp(dest, destSize, L"FriendlyName");
		}
		return ret;
	}
	int32_t getPath(wchar_t *dest, int32_t destSize) {
		return getProp(dest, destSize, L"DevicePath");
	}
	int32_t getProp(wchar_t *dest, int32_t destSize, const wchar_t *name);
private:
	DeviceProps() {}
	DeviceProps(DeviceProps &another) {}
	IPropertyBag *m_pBag;
};

//return !0 exit loop
typedef int(*deviceEnumCallbackFunc)(void *userdata, int32_t deviceNo, wchar_t *name, IMoniker *moniker, DeviceProps *prop);
class VideoCapture;
class VideoCaptureInfo;
class DeviceManager
{
public:
	DeviceManager();
	~DeviceManager();

	int32_t startup();
	void cleanup();
	
	int32_t enumVideoDevice(deviceEnumCallbackFunc func, void *userdata);
	int32_t enumVideoCaptureInfo(const wchar_t *name, videoCaptureEnumCallbackFunc func, void *userdata);
	VideoCapture *createVideoCapture(const wchar_t *name, HWND hWnd, int no = 0, int fps = 0);
	void destroyVideoCapture(VideoCapture *);

	//ÄÚ²¿Ë½ÓÐ
	IBaseFilter *createBaseFilter(const wchar_t *deviceName);
	VideoCaptureInfo *videoCaptureInfo() { return m_captureInfo; }
private:
	bool m_fInited = false;
	VideoCaptureInfo *m_captureInfo = NULL;
	ICreateDevEnum *m_devEnum = NULL;
};


