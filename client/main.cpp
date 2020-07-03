// testCapture.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "env.h"
#include "App.h"

int APIENTRY _tWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPTSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	CPaintManagerUI::SetInstance(hInstance);

	HRESULT hr = CoInitializeEx(
		NULL, COINIT_MULTITHREADED);  // Use COINIT_MULTITHREADED since Voice
	if (SUCCEEDED(hr)) {
		//hr = ::OleInitialize(NULL);
		//if (SUCCEEDED(hr)) {
			do {
				if (!theApp.startup()) {
					printf("App Init Failed\n");
					hr = -2;
					break;
				}
				CPaintManagerUI::MessageLoop();
				hr = 0;
			} while (0);
			theApp.cleanup();
		//}
		//::OleUninitialize();
	}
	::CoUninitialize();
	return (int)hr;

#if 0
	DeviceManager deviceManager;
	VideoCapture *videoCapture = NULL;

	do {
		int ret = deviceManager.startup();
		if (0 != ret) {
			break;
		}
		wchar_t devname[1024] = { 0 };
		deviceManager.enumVideoDevice([](void *userdata, int32_t no, wchar_t *name, IMoniker *moniker, DeviceProps *prop)->int {
			wchar_t *devname = (wchar_t *)userdata;
			if (devname[0] == '\0') {
				wcscpy(devname, name);
			}
			wprintf(L"device pos:%d name:%s\r\n", no, name);
			return 0;
		}, devname);
		if (devname[0] == 0) {
			wprintf(L"device is empry\r\n");
			break;
		}
		deviceManager.enumVideoCaptureInfo(devname, [](void *userdata, int32_t no,
			const char *format, int minFps, int maxFps, int width, int height){
			printf("no %2d: %s fps(%d-%d) %d*%d\r\n", no, format, minFps, maxFps, width, height);
		}, NULL);

		//videoCapture = deviceManager.createVideoCapture(devname);
		videoCapture = deviceManager.createVideoCapture(devname, 7, 25);
		if (videoCapture != NULL) {
			videoCapture->setWindow(GetConsoleWindow());
			ret = videoCapture->start();
			if (0 != ret) {
				wprintf(L"videoCapture start failed %d\r\n", ret);
				break;
			}

			while (true) {
				char c = getchar();
				if (c == 'q' || c == 'Q') {
					break;
				}
			}

			videoCapture->stop();
			deviceManager.destroyVideoCapture(videoCapture);
		}
	} while (0);
	deviceManager.cleanup();
    return 0;
#endif
}

