#pragma once

#include <stdint.h>

class NetClient;
struct mission_subscriber_t {
	mission_subscriber_t *next;
	mission_subscriber_t *prev;
	struct mission_publisher_t *publisher;
	NetClient *client;
	int32_t	selfId;
	int32_t endpointRoom;
	int32_t endpointId;
};

struct mission_publisher_t{
	mission_publisher_t *next;
	mission_publisher_t *prev;
	NetClient *client;
	int32_t selfRoom;
	int32_t selfId;
	int32_t endpointRoom;
	int32_t endpointId;
	mission_subscriber_t header;
	mission_subscriber_t tailer;
};


class MissionMgr
{
public:
	MissionMgr();
	~MissionMgr();

	int  create();
	void destroy();

	mission_publisher_t *header() { return &m_header; }
	mission_publisher_t *tailer() { return &m_tailer; }
	mission_publisher_t *first()  { return m_header.next; }

	void onClientClosed(NetClient *nc);

	mission_publisher_t *startPublisher(NetClient *, struct proto_publish_start_req_t *msg, uint32_t msglen);
	void stopPublisher(NetClient *nc, mission_publisher_t *);

	mission_subscriber_t *startSubscriber(NetClient *,  mission_publisher_t *, struct proto_subscribe_start_req_t *msg, uint32_t msglen);
	void stopSubscriber(NetClient *nc, struct proto_subscribe_stop_t *msg, uint32_t msglen);

private:
	mission_publisher_t *allocPublisher();
	void freePublisher(mission_publisher_t *);
	mission_subscriber_t *allocSubscriber();
	void freeSubscriber(mission_subscriber_t *);

private:
	void closeSubscribe(mission_subscriber_t *);
	void closePublisher(mission_publisher_t *);
private:
	mission_publisher_t m_header;
	mission_publisher_t m_tailer;
};


