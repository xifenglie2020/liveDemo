#include "NetObject.h"
#include "App.h"

NetObject::NetObject()
{
	m_socket = XT_INVALID_HANDLE;
	memset(m_selfurl, 0, sizeof(m_selfurl));
	memset(m_centurl, 0, sizeof(m_centurl));
}

NetObject::~NetObject()
{
}

int NetObject::recv_data() {
	uint32_t msglen;
	int  recved, remain, total;
	char *pmsg = m_buffer;
	remain = NetObjectRecvCacheSize - m_recved;
	recved = theApp.xtNetwork()->recv(m_socket, &m_buffer[m_recved], remain);
	if (recved > 0) {
		total = recved + m_recved;
		while (total >= (int)sizeof(proto_base_t)) {
			proto_base_t *pheader = (proto_base_t *)pmsg;
			n2h_u16(pheader->len, msglen);
			if (msglen < sizeof(proto_base_t) || msglen > proto_max_msg_len) {
				//data error
				destroy(0);
				return -1;
			}
			if (total < msglen) {	//data length not enough
				break;
			}
			if(S_OK != ::SendMessage(theApp.mainWnd(), WMUSER_NET_MSG, msglen, (LPARAM)pheader)){
				destroy(0);
				return -1;
			}
			pmsg += msglen;
			total -= (int)msglen;
		}

		if (total > 0 && pmsg != m_buffer) {
			memcpy(m_buffer, pmsg, total);
		}
		m_recved = total;
	}
	else if (recved == 0) {
		destroy(0);
		return -1;
	}
	return 0;
}

bool NetObject::login(const char *ip, int port, const char *user, const char *pswd) {
	proto_login_req_t req;
	proto_login_rsp_t rsp;
	xt_network_t *xn = theApp.xtNetwork();
	int32_t s = xn->create();

	memset(m_selfurl, 0, sizeof(m_selfurl));
	memset(m_centurl, 0, sizeof(m_centurl));
	if (s != XT_INVALID_HANDLE) {
		memset(&req, 0, sizeof(req));
		memset(&rsp, 0, sizeof(rsp));
		do {
			int ret = xn->connect(s, ip, port);
			if (ret != eXsuccess) {
				break;
			}
			int len = sizeof(req);
			h2n_u16(PRO_CMD_LOGIN_REQ, req.cmd);
			h2n_u16(len, req.len);

			h2n_u16(1, req.ver);
			strncpy(req.user, user, sizeof(req.user));
			//md5 pswd...
			ret = xn->send(s, (const char *)&req, len);
			if (ret != len) {
				break;
			}
			len = sizeof(rsp);
			ret = xn->recv(s, (char *)&rsp, len);
			if(ret != len){
				break;
			}
			if (rsp.error != 0) {
				break;
			}
			m_recved = 0;
			m_lastSentTime = xt_tickcount64();
			m_socket = s;
			strcpy(m_selfurl, rsp.to);
			strcpy(m_centurl, rsp.from);
			return true;
		} while (0);
		xn->destroy(s);
	}
	return false;
}

void NetObject::logout() {
	memset(m_selfurl, 0, sizeof(m_selfurl));
	memset(m_centurl, 0, sizeof(m_centurl));
	if (m_socket != XT_INVALID_HANDLE) {
		sendCommand(PRO_CMD_LOGOUT);
		theApp.xtNetwork()->destroy(m_socket);
		m_socket = XT_INVALID_HANDLE;
	}
}

bool NetObject::sendCommand(uint32_t cmd) {
	proto_base_t req = { 0 };
	int len = (int)sizeof(req);
	h2n_u16(cmd, req.cmd);
	h2n_u16(len, req.len);
	strcpy(req.from, m_selfurl);
	strcpy(req.to, m_centurl);
	return sendData(&req, len);
}

void NetObject::step() {
	if (m_socket != XT_INVALID_HANDLE) {
		xt_network_t *xn = theApp.xtNetwork();
		xt_io_event_t ie;
		ie.socket = m_socket;
		ie.eventIn = XT_IO_EVENT_READ;
		xn->select(&ie, 1, 0);
		if (ie.eventOut & XT_IO_EVENT_READ) {
			recv_data();
		}
		else {
			uint64_t c = xt_tickcount64();
			if (c >= m_lastSentTime + 20000000) {
				proto_base_t req = { 0 };
				m_lastSentTime = c;
				if (!sendCommand(PRO_CMD_HEARTBEAT)) {
					destroy(0);
				}
			}
		}
	}
}

bool NetObject::sendData(void *data, int len) {
	m_lastSentTime = xt_tickcount64();
	int ret = theApp.xtNetwork()->send(m_socket, (const char *)data, len);
	return ret == len;
}
