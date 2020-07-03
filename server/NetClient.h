#pragma once

#include "NetBase.h"
#include "../protocol.h"

const int NetObjectRecvCacheSize = 1024 * 32;

class NetClient : public NetBase
{
public:
	NetClient();
	~NetClient();

	virtual void destroy();
	virtual void doRecv();

	bool isUsed() { return m_used; }
	void setUsed(bool f) { m_used = f; }

	void create(int32_t socket);
	
	bool isTimeout(uint64_t &curTime);

	struct mission_publisher_t *getPublisher() { return m_publisher; }

	const char *url() { return m_url; }
	const char *name() { return m_name; }
	bool isLogined() { return m_url[0] != '\0'; }

	bool sendData(void *data, int len);
	void onPublisherClosed(struct mission_subscriber_t *ms);
private:
	bool processMessage(struct proto_base_t *msg, uint32_t msglen);
	bool onLogin(struct proto_login_req_t *msg, uint32_t msglen);
	bool onQueryPlayerList(struct proto_base_t *msg, uint32_t msglen);
	bool onStartPublish(struct proto_publish_start_req_t *msg, uint32_t msglen);
	bool onStopPublish(struct proto_publish_stop_t *msg, uint32_t msglen);
	bool onStartSubscribe(struct proto_subscribe_start_req_t *msg, uint32_t msglen);
	bool onStopSubscribe(struct proto_subscribe_stop_t *msg, uint32_t msglen);

private:
	bool m_used = false;
	uint64_t m_lastRecvTime = 0;
	struct mission_publisher_t *m_publisher = NULL;
	char m_url[proto_url_len];
	char m_name[proto_name_len];
	int  m_checkCount = 0;
	int  m_recved = 0;
	char m_buffer[NetObjectRecvCacheSize];
};

