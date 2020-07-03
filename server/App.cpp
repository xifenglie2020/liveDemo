#include "App.h"

App theApp;

App::App()
{
}

App::~App()
{
}

bool App::startup() {
	int32_t ret;
	do {
		ret = logger_startup(NULL);

		struct iniparse_t{
			config_t cfg;
			xt_init_params_t xi;
		}inidata = { 0 };
		inidata.cfg.socketTimeout = 30 * 1000000;	//30s
		inidata.xi.udpPort = 19000;
		inidata.xi.tcpPort = 9000;
		xt_ini_read("./xtserver.app.conf", [](void *userdata, const char *key, char *value)->int {
			iniparse_t &iniData = *((iniparse_t *)userdata);
			if (strcmp(key, "ip.bind") == 0) {
				strcpy(iniData.xi.bindip, value);
			}
			else if (strcmp(key, "ip.map") == 0) {
				strcpy(iniData.cfg.mapip, value);
			}
			else if (strcmp(key, "port.tcp") == 0) {
				iniData.xi.tcpPort = atoi(value);
			}
			else if (strcmp(key, "port.udp") == 0) {
				iniData.xi.udpPort = atoi(value);
			}
			else if (strcmp(key, "port.listen") == 0) {
				iniData.cfg.listenPort = atoi(value);
			}
			else if (strcmp(key, "socket.timeout") == 0) {
				iniData.cfg.socketTimeout = atoi(value) * 1000000;
			}
			return 0;
		}, &inidata);
		if (inidata.cfg.mapip[0] == '\0') {
			if (inidata.xi.bindip[0] == '\0') {
				strcpy(inidata.xi.bindip, "127.0.0.1");
			}
			strcpy(inidata.cfg.mapip, inidata.xi.bindip);
		}
		//save it
		memcpy(&m_config, &inidata.cfg, sizeof(config_t));
		ret = xt_startup_byfile(&inidata.xi, "./xtserver.conf");
		if (0 != ret) {
			break;
		}
		m_netWork = xt_network();

		ret = m_missionMgr.create();
		if (0 != ret) {
			break;
		}

		ret = m_listener.create(m_config.listenPort);
		if (0 != ret) {
			break;
		}

		const xt_init_attrs_t *attrs = xt_peek_init_attrs();

		m_ioEvents = new xt_io_event_t[attrs->maxConnections+16];
		if (m_ioEvents == NULL) {
			break;
		}

		ret = m_netClientMgr.init(attrs->maxConnections);
		if (0 != ret) {
			break;
		}

		return true;
	} while (0);
	return false;
}


void App::cleanup() {
	m_listener.destroy();
	m_netClientMgr.fini();
	xt_cleanup();
	if (m_ioEvents != NULL) {
		delete []m_ioEvents;
		m_ioEvents = NULL;
	}
	m_missionMgr.destroy();
	logger_cleanup();
}

int  App::service() {
	while (true) {
		uint64_t curTime;
		int i, count = 0;
		m_ioEvents[count].socket = m_listener.getSocket();
		m_ioEvents[count].eventIn = XT_IO_EVENT_READ;
		m_ioEvents[count].eventOut = 0;
		m_ioEvents[count].userData = &m_listener;
		count++;
		for (i = 0; i < m_netClientMgr.size(); i++) {
			NetClient *p = m_netClientMgr.peer(i);
			if (p->isUsed()) {
				m_ioEvents[count].socket = p->getSocket();
				m_ioEvents[count].eventIn = XT_IO_EVENT_READ;
				m_ioEvents[count].userData = p;
				count++;
			}
		}
		m_netWork->select(m_ioEvents, count, 10);
		for (i = 0; i < count; i++) {
			if ((m_ioEvents[i].eventOut & XT_IO_EVENT_READ)) {
				NetBase *p = (NetBase *)m_ioEvents[i].userData;
				p->doRecv();
			}
		}
		//¼ì²é³¬Ê±
		curTime = xt_tickcount64();
		m_netClientMgr.checkTimeout(curTime);
	}
	return 0;
}


