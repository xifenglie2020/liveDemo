#include "NetClient.h"
#include "App.h"

LOGGER_Declare("netclient");

NetClient::NetClient()
{
}

NetClient::~NetClient()
{
}

void NetClient::destroy() {
	//关闭业务逻辑
	theApp.missionMgr().onClientClosed(this);
	m_publisher = NULL;
	m_url[0] = 0;
	if (m_socket != XT_INVALID_HANDLE) {
		theApp.xtNetwork()->destroy(m_socket);
		m_socket = XT_INVALID_HANDLE;
	}
}

void NetClient::doRecv() {
	uint32_t msglen;
	int  recved, remain, total;
	char *pmsg = m_buffer;
	remain = NetObjectRecvCacheSize - m_recved;
	recved = theApp.xtNetwork()->recv(m_socket, &m_buffer[m_recved], remain);
	LOGGER_Info("client %p socket %d recved %d total %d", this, m_socket, recved, recved + m_recved);
	if (recved > 0) {
		total = recved + m_recved;
		while (total >= (int)sizeof(proto_base_t)) {
			proto_base_t *pheader = (proto_base_t *)pmsg;
			n2h_u16(pheader->len, msglen);
			if (msglen < sizeof(proto_base_t) || msglen > proto_max_msg_len) {
				//data error
				theApp.netClientMgr().freeClient(this);
				return;
			}
			if (total < msglen) {	//data length not enough
				break;
			}
			if (!processMessage(pheader, msglen)) {
				theApp.netClientMgr().freeClient(this);
				return;
			}
			pmsg += msglen;
			total -= (int)msglen;
		}

		if (total > 0 && pmsg != m_buffer) {
			memcpy(m_buffer, pmsg, total);
		}
		m_recved = total;
	}
	else /*if (recved == 0)*/ {	//直接关闭好了
		theApp.netClientMgr().freeClient(this);
	}
	return;
}

void NetClient::create(int32_t socket) {
	m_lastRecvTime = xt_tickcount64();
	m_socket = socket;
	m_recved = 0;
	m_checkCount = (m_checkCount + 1) & 0xFFFF;
	m_url[0] = 0;
}

bool NetClient::isTimeout(uint64_t &curTime) {
	return curTime > m_lastRecvTime + theApp.config().socketTimeout;
}

bool NetClient::processMessage(struct proto_base_t *msg, uint32_t msglen) {
	uint16_t command;
	n2h_u16(msg->cmd, command);
	LOGGER_Info("client %p process command %d len %d", this, command, msglen);
	m_lastRecvTime = xt_tickcount64();
	switch (command) {
	case PRO_CMD_LOGIN_REQ:
		return onLogin((proto_login_req_t *)msg, msglen);
		break;
	case PRO_CMD_LOGOUT:
		theApp.netClientMgr().freeClient(this);
		break;
	case PRO_CMD_QUERY_PUB_REQ:
		return onQueryPlayerList(msg, msglen);
		break;
	case PRO_CMD_PUBLISH_START_REQ:
		return onStartPublish((proto_publish_start_req_t *)msg, msglen);
		break;
	case PRO_CMD_PUBLISH_STOP:
		return onStopPublish((proto_publish_stop_t *)msg, msglen);
		break;
	case PRO_CMD_SUBSCRIBE_START_REQ:
		return onStartSubscribe((proto_subscribe_start_req_t *)msg, msglen);
		break;
	case PRO_CMD_SUBSCRIBE_STOP:
		return onStopSubscribe((proto_subscribe_stop_t *)msg, msglen);
		break;
	}
	return true;
}

void NetClient::onPublisherClosed(struct mission_subscriber_t *ms) {
	proto_stoped_t rsp;
	memset(&rsp, 0, sizeof(rsp));
	int len = (int)sizeof(rsp);
	h2n_u16(PRO_CMD_STOPED, rsp.cmd);
	h2n_u16(len, rsp.len);
	strcpy(rsp.from, theApp.config().url);
	strcpy(rsp.to, m_url);
	h2n_u32(ms->endpointRoom, rsp.cltRoom);
	h2n_u32(ms->endpointId, rsp.cltId);
	h2n_u32(ms->publisher->selfRoom, rsp.srvRoom);
	h2n_u32(ms->selfId, rsp.srvId);
	if (!sendData(&rsp, len)) {
		theApp.netClientMgr().freeClient(this);
	}
}

bool NetClient::sendData(void *data, int len) {
	return len == theApp.xtNetwork()->send(m_socket, (const char *)data, len);
}

bool NetClient::onLogin(struct proto_login_req_t *msg, uint32_t msglen) {
	proto_login_rsp_t rsp;
	memset(&rsp, 0, sizeof(rsp));
	int len = (int)sizeof(rsp);
	h2n_u16(PRO_CMD_LOGIN_RSP, rsp.cmd);
	h2n_u16(len, rsp.len);
	sprintf(m_url, "%04x#%lld", m_checkCount, (int64_t)this);
	strcpy(m_name, msg->user);
	strcpy(rsp.from, theApp.config().url);
	strcpy(rsp.to, m_url);
	LOGGER_Info("user %s logined url %s", msg->user, m_url);
	return sendData(&rsp, len);
}

bool NetClient::onQueryPlayerList(struct proto_base_t *msg, uint32_t msglen) {
	int  maxCountPerPacket = 1 + 
		(proto_max_msg_len - sizeof(proto_query_publishers_rsp_t)) / sizeof(proto_query_publishers_rsp_t::item_t);
	char buffer[proto_max_msg_len];
	proto_query_publishers_rsp_t *rsp = (proto_query_publishers_rsp_t *)buffer;
	int start = 0;
	int count = 0;
	for (mission_publisher_t *p = theApp.missionMgr().first(); p != theApp.missionMgr().tailer(); p = p->next) {
		if (p->client != NULL) {
			strcpy(rsp->items[count].url, p->client->url());
			strcpy(rsp->items[count].name, p->client->name());
			count++;
			if (count >= maxCountPerPacket) {
				//send response.
				int len = sizeof(proto_query_publishers_rsp_t) - 
					sizeof(proto_query_publishers_rsp_t::item_t) + 
					count * sizeof(proto_query_publishers_rsp_t::item_t);
				h2n_u16(PRO_CMD_QUERY_PUB_RSP, rsp->cmd);
				h2n_u16(len, rsp->len);
				strcpy(rsp->from, theApp.config().url);
				strcpy(rsp->to, msg->from);
				h2n_u32(start, rsp->start);
				h2n_u32(count, rsp->count);
				if (!sendData(rsp, len)) {
					return false;
				}
				start += maxCountPerPacket;
				count = 0;
			}
		}
	}
	if (start == 0 || count > 0) {
		int len = sizeof(proto_query_publishers_rsp_t) -
			sizeof(proto_query_publishers_rsp_t::item_t) +
			count * sizeof(proto_query_publishers_rsp_t::item_t);
		h2n_u16(PRO_CMD_QUERY_PUB_RSP, rsp->cmd);
		h2n_u16(len, rsp->len);
		strcpy(rsp->from, theApp.config().url);
		strcpy(rsp->to, msg->from);
		h2n_u32(start, rsp->start);
		h2n_u32(count, rsp->count);
		if (!sendData(rsp, len)) {
			return false;
		}
	}

	return true;
}

bool NetClient::onStartPublish(struct proto_publish_start_req_t *msg, uint32_t msglen) {
	proto_publish_start_rsp_t rsp;
	memset(&rsp, 0, sizeof(rsp));
	mission_publisher_t *mp = theApp.missionMgr().startPublisher(this, msg, msglen);
	if (mp == NULL) {
		h2n_u32(eXoutofMemory, rsp.error);
	}
	else {
		const xt_init_params_t *xpa = xt_peek_init_params();
		m_publisher = mp;
		rsp.passive = 1;
		h2n_u32(mp->selfRoom, rsp.srvRoom);
		h2n_u32(mp->selfId, rsp.srvId);
		switch (msg->type) {
		case TRANSFER_TYPE_UDP:
			h2n_u32(xpa->udpPort, rsp.srvPort);
			break;
		default:
			h2n_u32(xpa->tcpPort, rsp.srvPort);
			break;
		}
		strcpy(rsp.srvIp, theApp.config().mapip);
	}
	rsp.type = msg->type;
	rsp.cltRoom = msg->room;
	rsp.cltId = msg->id;

	int len = (int)sizeof(rsp);
	h2n_u16(PRO_CMD_PUBLISH_START_RSP, rsp.cmd);
	h2n_u16(len, rsp.len);
	strcpy(rsp.from, theApp.config().url);
	strcpy(rsp.to, msg->from);
	return sendData(&rsp, len);
}

bool NetClient::onStopPublish(struct proto_publish_stop_t *msg, uint32_t msglen) {
	if (m_publisher) {
		theApp.missionMgr().stopPublisher(this, m_publisher);
		m_publisher = NULL;
	}
	return true;
}

bool NetClient::onStartSubscribe(struct proto_subscribe_start_req_t *msg, uint32_t msglen) {
	proto_subscribe_start_rsp_t rsp;
	memset(&rsp, 0, sizeof(rsp));
	mission_subscriber_t *ms;
	NetClient *nc = theApp.netClientMgr().find(msg->to);
	if (nc == NULL || nc->getPublisher() == NULL) {
		h2n_u32(eXnotFound, rsp.error);
	}
	else if ((ms = theApp.missionMgr().startSubscriber(this, nc->getPublisher(), msg, msglen)) == NULL) {
		h2n_u32(eXoutofMemory, rsp.error);
	}
	else {
		const xt_init_params_t *xpa = xt_peek_init_params();
		h2n_u32(ms->publisher->selfRoom, rsp.srvRoom);
		h2n_u32(ms->selfId, rsp.srvId);
		switch (msg->type) {
		case TRANSFER_TYPE_UDP:
			h2n_u32(xpa->udpPort, rsp.srvPort);
			break;
		default:
			h2n_u32(xpa->tcpPort, rsp.srvPort);
			break;
		}
		strcpy(rsp.srvIp, theApp.config().mapip);
	}
	rsp.type = msg->type;
	rsp.cltRoom = msg->room;
	rsp.cltId = msg->id;
	int len = (int)sizeof(rsp);
	h2n_u16(PRO_CMD_SUBSCRIBE_START_RSP, rsp.cmd);
	h2n_u16(len, rsp.len);
	strcpy(rsp.from, theApp.config().url);
	strcpy(rsp.to, msg->from);
	return sendData(&rsp, len);
}

bool NetClient::onStopSubscribe(struct proto_subscribe_stop_t *msg, uint32_t msglen) {
	theApp.missionMgr().stopSubscriber(this, msg, msglen);
	return true;
}

