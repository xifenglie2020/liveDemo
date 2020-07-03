#include "NetListener.h"
#include "App.h"

LOGGER_Declare("listener");

NetListener::NetListener()
{
}

NetListener::~NetListener()
{
}

void NetListener::destroy() {
	if (m_socket != XT_INVALID_HANDLE) {
		theApp.xtNetwork()->destroy(m_socket);
		m_socket = XT_INVALID_HANDLE;
	}
}

void NetListener::doRecv() {
	xt_network_t *xn = theApp.xtNetwork();
	int32_t s = xn->accept(m_socket, NULL, NULL);
	if (s != XT_INVALID_HANDLE) {
		NetClient *client = theApp.netClientMgr().getClient();
		LOGGER_Info("socket %d connected and client %p", s, client);
		if (client == NULL) {
			xn->destroy(s);
		}
		else {
			client->create(s);
		}
	}
}


int  NetListener::create(int port) {
	int ret = -1;
	xt_network_t *xn = theApp.xtNetwork();
	int32_t s;
	do {
		s = xn->create();
		if (s == XT_INVALID_HANDLE) {
			break;
		}
		ret = xn->bind(s, NULL, port);
		if (eXsuccess != ret) {
			break;
		}
		ret = xn->listen(s);
		if (eXsuccess != ret) {
			break;
		}
		m_socket = s;
		return 0;
	} while (0);
	if (s != XT_INVALID_HANDLE) {
		xn->destroy(s);
	}
	return ret;
}
