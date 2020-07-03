#pragma once

#include "NetBase.h"

class NetListener : public NetBase
{
public:
	NetListener();
	~NetListener();

	virtual void destroy();
	virtual void doRecv();

	int  create(int port);
};

