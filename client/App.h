#pragma once

#include "logger.h"
#include "WindowMain.h"
#include "capture/DeviceManager.h"
#include "../protocol.h"
#include "xflnetwork.h"
#include "xfltransfer.h"
#include "xflerror.h"
#include "NetObject.h"
#include "MissionObject.h"

struct config_t {
	int netType;
	int capFile;
};
class App
{
public:
	App();
	~App();

	bool startup();
	void cleanup();

	HWND mainWnd() { return m_wndMain.GetHWND(); }
	DeviceManager &devMgr() { return m_devMgr; }
	xt_network_t *xtNetwork() { return m_netWork; }
	NetObject *netObject() { return m_netObject; }

	config_t &config() { return m_config; }

private:
	config_t	m_config;
	xt_network_t *m_netWork = NULL;
	CWindowMain m_wndMain;
	DeviceManager m_devMgr;
	NetObject	*m_netObject = NULL;
};

extern App theApp;

