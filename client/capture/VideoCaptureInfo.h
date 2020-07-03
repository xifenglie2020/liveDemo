#pragma once

#include "DeviceInclude.h"
#include "../MediaFormat.h"
#include <vector>

struct video_capture_info_ext {
	eVideoFormat fmt;
	const char *desc;
	int minFps;
	int maxFps;
	int fps;
	int width;
	int height;
	AM_MEDIA_TYPE mt;
	BYTE pbFormat[sizeof(VIDEOINFOHEADER) + 64];
};

struct video_capture_list_t {
	wchar_t *name;
	int captureCount;
	video_capture_info_ext *captureLists;
};

class VideoCaptureInfo {
public:
	VideoCaptureInfo();
	~VideoCaptureInfo() { cleanup(); }

	//return count;
	int createCaptureLists(DeviceManager *mgr, const wchar_t *name);
	int checkedCreateCaptureLists(DeviceManager *mgr, const wchar_t *name);
	video_capture_list_t *find(const wchar_t *name);
	video_capture_info_ext *peer(video_capture_list_t *lst, int pos);
	video_capture_info_ext *peer(const wchar_t *name, int pos);
	int enumCaptureInfo(const wchar_t *name, videoCaptureEnumCallbackFunc func, void *userdata);
	void cleanup();

private:
	void clearInfo(video_capture_list_t *);
	int buildCaptureInfo(DeviceManager *mgr, const wchar_t *name, video_capture_list_t *lst);
	int enumCaptureInfo2(video_capture_list_t *lst, videoCaptureEnumCallbackFunc func, void *userdata);
private:
	std::vector<video_capture_list_t> m_lists;
};



