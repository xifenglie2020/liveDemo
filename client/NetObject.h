#pragma once

#include <stdint.h>
#include "../protocol.h"

const int NetObjectRecvCacheSize = 1024 * 64;
class NetObject
{
public:
	NetObject();
	~NetObject();

	bool login(const char *ip, int port, const char *user, const char *pswd);
	void logout();
	void step();
	bool isLogined() { return m_selfurl[0] != '\0'; }
	bool sendData(void *data, int len);
	bool sendCommand(uint32_t cmd);

	const char *selfUrl() { return m_selfurl; }
	const char *centUrl() { return m_centurl; }
protected:
	int  recv_data();
	void destroy(int appexit) {
		logout();
	}
private:
	int32_t m_socket;
	char m_selfurl[proto_url_len];
	char m_centurl[proto_url_len];
	uint64_t m_lastSentTime = 0;
	int  m_recved = 0;
	char m_buffer[NetObjectRecvCacheSize];
};

