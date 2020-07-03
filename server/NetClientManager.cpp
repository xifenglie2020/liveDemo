#include "NetClientManager.h"
#include "App.h"

NetClientManager::NetClientManager()
{
}

NetClientManager::~NetClientManager()
{
}

int NetClientManager::init(int count) {
	m_pClients = new NetClient[count];
	if (m_pClients) {
		m_size = count;
		return 0;
	}
	return -1;
}

void NetClientManager::fini() {
	if (m_pClients != NULL) {
		for (int i = 0; i < m_size; i++) {
			if (m_pClients[i].isUsed()) {
				m_pClients[i].destroy();
			}
		}
		delete[]m_pClients;
		m_pClients = NULL;
		m_size = 0;
	}
}

NetClient *NetClientManager::getClient() {
	if (m_pClients != NULL) {
		for (int i = 0; i < m_size; i++) {
			if (!m_pClients[i].isUsed()) {
				m_pClients[i].setUsed(true);
				return &m_pClients[i];
			}
		}
	}
	return NULL;
}

void NetClientManager::freeClient(NetClient *netClient) {
	if (netClient->isUsed()) {
		netClient->destroy();
		netClient->setUsed(false);
	}
}

void NetClientManager::checkTimeout(uint64_t &curTime) {
	if (m_pClients != NULL) {
		for (int i = 0; i < m_size; i++) {
			if (m_pClients[i].isUsed() && m_pClients[i].isTimeout(curTime)) {
				freeClient(&m_pClients[i]);
			}
		}
	}
}

NetClient *NetClientManager::find(const char *url) {
	if (m_pClients != NULL) {
		for (int i = 0; i < m_size; i++) {
			if (m_pClients[i].isUsed() && strcmp(url, m_pClients[i].url())==0 ) {
				return &m_pClients[i];
			}
		}
	}
	return NULL;
}

