#pragma once

#include <stdlib.h>
#include <stdint.h>

class NetBase {
public:
	virtual void destroy() = 0;
	virtual void doRecv() = 0;
	int32_t getSocket() { return m_socket; }
protected:
	int32_t m_socket = -1;
};
