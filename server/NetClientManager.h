#pragma once

#include "NetClient.h"

class NetClientManager
{
public:
	NetClientManager();
	~NetClientManager();

	int  init(int count);
	void fini();

	NetClient *getClient();
	void freeClient(NetClient *);

	int  size() { return m_size; }
	NetClient *peer(int pos) { return &m_pClients[pos]; }

	void checkTimeout(uint64_t &curTime);

	NetClient *find(const char *url);
protected:
	int m_size = 0;
	NetClient *m_pClients = NULL;
};

