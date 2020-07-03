#pragma once

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include "logger.h"
#include "../protocol.h"
#include "xflnetwork.h"
#include "xfltransfer.h"
#include "xflerror.h"
#include "nodelist.h"
#include "NetListener.h"
#include "NetClientManager.h"
#include "MissionMgr.h"

struct config_t {
	char mapip[32];
	int  listenPort;
	int  socketTimeout;
	char url[proto_url_len];
};

class App
{
public:
	App();
	~App();

	bool startup();
	void cleanup();

	int  service();

	config_t &config() { return m_config; }
	xt_network_t *xtNetwork() { return m_netWork; }
	NetClientManager &netClientMgr() { return m_netClientMgr; }
	MissionMgr	&missionMgr() { return m_missionMgr; }
private:
	xt_network_t *m_netWork = NULL;
	xt_io_event_t *m_ioEvents = NULL;
	config_t m_config;
	NetListener m_listener;
	NetClientManager m_netClientMgr;
	MissionMgr	m_missionMgr;
};

extern App theApp;

