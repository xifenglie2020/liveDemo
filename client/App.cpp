#include "App.h"

App theApp;

App::App()
{
}

App::~App()
{
}

extern int registerFfmpeg();
extern void unregisterFfmpeg();

bool App::startup() {
	int32_t ret;
	do {
		xt_init_params_t xip = { 0 };
		ret = logger_startup(NULL);
		m_netObject = new NetObject();

		HWND hWnd = m_wndMain.Create(NULL, "LianGui", WS_POPUP | WS_CLIPCHILDREN, 0L, 0, 0, 1024, 738);
		if (!IsWindow(hWnd)) {
			break;
		}
		ret = m_devMgr.startup();
		if (ret != 0) {
			break;
		}
		ret = registerFfmpeg();
		if (0 != ret) {
			break;
		}
		xip.udpPort = -1;
		ret = xt_startup_byfile(&xip, "./xtclient.conf");
		if (0 != ret) {
			break;
		}
		m_netWork = xt_network();
		m_wndMain.ShowWindow(true, false);
		//µÇÂ¼´°¿Ú
		//m_wndMain.PostMessage(WMUSER_PAGE_CTRL, MAIN_PAGE_CTRL_CREATE_PUBLISHER, 0);
		return true;
	} while (0);
	return false;
}

void App::cleanup() {
	m_devMgr.cleanup();
	if (::IsWindow(m_wndMain.GetHWND())) {
		m_wndMain.Close();
	}
	if (m_netObject != NULL) {
		m_netObject->logout();
		delete m_netObject;
		m_netObject = NULL;
	}
	xt_cleanup();
	unregisterFfmpeg();
	logger_cleanup();
}

