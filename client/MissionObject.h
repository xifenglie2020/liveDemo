#pragma once

#include <stdint.h>

typedef int32_t (*onMediaDataCallback)(void *userData, uint8_t streamid, uint8_t mediaType, uint8_t frameType, const uint8_t *data, int32_t size);
class MissionObject
{
public:
	MissionObject();
	~MissionObject();

	bool start(const char *targetUrl);
	void stop();
	void onResponse(uint32_t command, struct proto_base_t *msg, uint32_t msglen);
	void sendData(uint8_t streamid, uint8_t mediaType, uint8_t frameType, const uint8_t *data, int32_t size);
	
	void setCallback(onMediaDataCallback func, void *user) {
		m_callback = func;
		m_userData = user;
	}
private:
	static int32_t mediaCallback(int32_t id, uint64_t userdata, void *extend,
		uint8_t streamid, uint8_t mediaType, uint8_t frameType, const uint8_t *data, int32_t size);
private:
	int32_t m_selfRoom;
	int32_t m_selfId;
	int32_t m_localId;
	int32_t m_endpointRoom;
	int32_t m_endpointId;
	int32_t m_tranType;
	bool m_publisher;
	char m_destUrl[64];
	onMediaDataCallback m_callback;
	void *m_userData;
};

