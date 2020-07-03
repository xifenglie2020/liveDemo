#include "MissionObject.h"
#include "App.h"

LOGGER_Declare("mission");

MissionObject::MissionObject()
	: m_selfRoom(XT_INVALID_HANDLE)
	, m_selfId(XT_INVALID_HANDLE)
	, m_localId(XT_INVALID_HANDLE)
	, m_endpointRoom(XT_INVALID_HANDLE)
	, m_endpointId(XT_INVALID_HANDLE)
{
	m_destUrl[0] = 0;
}

MissionObject::~MissionObject()
{
}

static int32_t get_xt_transfer_type(int32_t type) {
	switch (type) {
	case TRANSFER_TYPE_UDP:
		return XT_NET_TYPE_UDP;
	default:
		return XT_NET_TYPE_TCP;
	}
}

bool MissionObject::start(const char *targetUrl) {
	m_tranType = theApp.config().netType;// TRANSFER_TYPE_UDP;
	if (targetUrl == NULL) {
		m_publisher = true;
		proto_publish_start_req_t req;
		int len = (int)sizeof(req);
		memset(&req, 0, sizeof(req));
		h2n_u16(PRO_CMD_PUBLISH_START_REQ, req.cmd);
		h2n_u16(len, req.len);
		strcpy(req.from, theApp.netObject()->selfUrl());
		strcpy(req.to, theApp.netObject()->centUrl());

		m_selfRoom = xt_create_room(XT_ROOM_TYPE_LIVE, (uint64_t)this);
		m_localId = xt_create_local_object(1, (uint64_t)this);
		xt_set_command_callback(m_localId, [](int32_t id, uint64_t userdata, uint8_t command, const uint8_t *data, int32_t size) {
			//状态接收
		}, (uint64_t)this);
		xt_add_object_to_room(m_localId, m_selfRoom);

		m_selfId = xt_create_network_object(get_xt_transfer_type(m_tranType), 0, (uint64_t)this);
		xt_add_object_to_room(m_selfId, m_selfRoom);

		req.type = m_tranType;
		h2n_u32(m_selfRoom, req.room);
		h2n_u32(m_selfId, req.id);
		theApp.netObject()->sendData(&req, len);
	}
	else {
		m_publisher = false;
		proto_subscribe_start_req_t req;
		int len = (int)sizeof(req);
		memset(&req, 0, sizeof(req));

		h2n_u16(PRO_CMD_SUBSCRIBE_START_REQ, req.cmd);
		h2n_u16(len, req.len);
		strcpy(req.from, theApp.netObject()->selfUrl());
		strcpy(req.to, targetUrl);

		m_selfRoom = xt_create_room(XT_ROOM_TYPE_LIVE, (uint64_t)this);
		m_localId = xt_create_local_object(0, (uint64_t)this);
		xt_set_command_callback(m_localId, [](int32_t id, uint64_t userdata, uint8_t command, const uint8_t *data, int32_t size) {
			//状态接收
		}, (uint64_t)this);
		xt_set_media_callback(m_localId, MissionObject::mediaCallback, (uint64_t)this);
		xt_add_object_to_room(m_localId, m_selfRoom);

		m_selfId = xt_create_network_object(get_xt_transfer_type(m_tranType), 1, (uint64_t)this);
		xt_add_object_to_room(m_selfId, m_selfRoom);

		req.type = m_tranType;
		h2n_u32(m_selfRoom, req.room);
		h2n_u32(m_selfId, req.id);
		theApp.netObject()->sendData(&req, len);
	}
	return true;
}

void MissionObject::stop() {
	if (m_selfId != XT_INVALID_HANDLE) {
		if (m_publisher) {
			proto_publish_stop_t req;
			int len = (int)sizeof(req);
			h2n_u16(PRO_CMD_PUBLISH_STOP, req.cmd);
			h2n_u16(len, req.len);
			strcpy(req.from, theApp.netObject()->selfUrl());
			strcpy(req.to, m_destUrl);
			h2n_u32(m_selfRoom, req.cltRoom);
			h2n_u32(m_selfId, req.cltId);
			h2n_u32(m_endpointRoom, req.srvRoom);
			h2n_u32(m_endpointId, req.srvId);
			theApp.netObject()->sendData(&req, len);
		}
		else {
			proto_subscribe_stop_t req;
			int len = (int)sizeof(req);
			h2n_u16(PRO_CMD_SUBSCRIBE_STOP, req.cmd);
			h2n_u16(len, req.len);
			h2n_u32(m_selfRoom, req.cltRoom);
			h2n_u32(m_selfId, req.cltId);
			h2n_u32(m_endpointRoom, req.srvRoom);
			h2n_u32(m_endpointId, req.srvId);
			strcpy(req.from, theApp.netObject()->selfUrl());
			strcpy(req.to, m_destUrl);
			theApp.netObject()->sendData(&req, len);
		}

		m_endpointId = XT_INVALID_HANDLE;
		m_endpointRoom = XT_INVALID_HANDLE;

		xt_destroy_object(m_selfId);
		m_selfId = XT_INVALID_HANDLE;
	}
	
	if (m_localId != XT_INVALID_HANDLE) {
		xt_destroy_object(m_localId);
		m_localId = XT_INVALID_HANDLE;
	}

	if (m_selfRoom != XT_INVALID_HANDLE) {
		xt_destroy_room(m_selfRoom);
		m_selfRoom = XT_INVALID_HANDLE;
	}
}

void MissionObject::onResponse(uint32_t command, struct proto_base_t *msg, uint32_t msglen) {
	switch (command) {
	case PRO_CMD_PUBLISH_START_RSP:
		do {
			proto_publish_start_rsp_t *rsp = (proto_publish_start_rsp_t *)msg;
			if (rsp->error != 0) {
				stop();
			}
			else {
				int32_t port;
				n2h_u32(rsp->srvRoom, m_endpointRoom);
				n2h_u32(rsp->srvId, m_endpointId);
				n2h_u32(rsp->srvPort, port);
				//if(rsp->error != 0)	...
				switch (m_tranType) {
				case TRANSFER_TYPE_UDP:
					xt_network_set_ip4_udp(m_selfId, m_endpointRoom, m_endpointId,
						(rsp->nat[0] != 0) ? rsp->nat : NULL, rsp->srvIp, port, rsp->passive);
					break;
				default:
					xt_network_set_ip4_tcp(m_selfId, m_endpointRoom, m_endpointId, rsp->srvIp, port, rsp->passive);
					break;
				}
			}
		} while (0);
		break;
	case PRO_CMD_SUBSCRIBE_START_RSP:
		do {
			proto_subscribe_start_rsp_t *rsp = (proto_subscribe_start_rsp_t *)msg;
			if (rsp->error != 0) {
				stop();
			}
			else {
				int32_t port;
				n2h_u32(rsp->srvRoom, m_endpointRoom);
				n2h_u32(rsp->srvId, m_endpointId);
				n2h_u32(rsp->srvPort, port);
				//LOGGER_Info("onStartSubscribe port %d endpoint room %d id %d self room %d id %d", 
				//	port, m_endpointRoom, m_endpointId, m_selfRoom, m_selfId);
				//if(rsp->error != 0)	...
				switch (m_tranType) {
				case TRANSFER_TYPE_UDP:
					xt_network_set_ip4_udp(m_selfId, m_endpointRoom, m_endpointId,
						(rsp->nat[0] != 0) ? rsp->nat : NULL, rsp->srvIp, port, rsp->passive);
					break;
				default:
					xt_network_set_ip4_tcp(m_selfId, m_endpointRoom, m_endpointId, rsp->srvIp, port, rsp->passive);
					break;
				}
			}
		} while (0);
		break;
	}
}

void MissionObject::sendData(uint8_t streamid, uint8_t mediaType, uint8_t frameType, const uint8_t *data, int32_t size) {
	//LOGGER_Debug("send frame type %d, len %d", frameType, size);
	xt_local_send_frame(m_localId, streamid, mediaType, frameType, data, size);
}

int32_t MissionObject::mediaCallback(int32_t id, uint64_t userdata, void *extend,
	uint8_t streamid, uint8_t mediaType, uint8_t frameType, const uint8_t *data, int32_t size) {
	MissionObject *pthis = (MissionObject *)userdata;
	if (pthis && pthis->m_callback) {
		//LOGGER_Debug("recv frame type %d, len %d", frameType, size);
		return pthis->m_callback(pthis->m_userData, streamid, mediaType, frameType, data, size);
	}
	return 0;
}

