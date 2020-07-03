#include "MissionMgr.h"
#include "App.h"

LOGGER_Declare("mmgr");

mission_publisher_t *MissionMgr::allocPublisher() {
	return new mission_publisher_t;
}
void MissionMgr::freePublisher(mission_publisher_t *p) {
	delete p;
}
mission_subscriber_t *MissionMgr::allocSubscriber() {
	return new mission_subscriber_t;
}
void MissionMgr::freeSubscriber(mission_subscriber_t *p) {
	delete p;
}


MissionMgr::MissionMgr()
{
	NODE_LIST_INIT(m_header, m_tailer);
}

MissionMgr::~MissionMgr()
{
}

int  MissionMgr::create() {
	return 0;
}

void MissionMgr::destroy() {
	for (mission_publisher_t *p = first(); p != tailer(); ) {
		mission_publisher_t *mp = p;
		p = p->next;
		for (mission_subscriber_t *m = mp->header.next; m != &mp->tailer; ) {
			mission_subscriber_t *ms = m;
			m = m->next;
			freeSubscriber(ms);
		}
		freePublisher(mp);
	}
}

void MissionMgr::onClientClosed(NetClient *nc) {
	for (mission_publisher_t *p = first(); p != tailer(); ) {
		mission_publisher_t *mp = p;
		p = p->next;
		for (mission_subscriber_t *m = mp->header.next; m != &mp->tailer; ) {
			mission_subscriber_t *ms = m;
			m = m->next;
			if (ms->client == nc) {
				NODE_LIST_REMOVE(ms);
				closeSubscribe(ms);
				freeSubscriber(ms);
			}
		}
		if (mp->client == nc) {
			stopPublisher(nc, mp);
		}
	}
}

void MissionMgr::closeSubscribe(mission_subscriber_t *ms) {
	if (ms->selfId != XT_INVALID_HANDLE) {
		xt_destroy_object(ms->selfId);
	}
}

void MissionMgr::closePublisher(mission_publisher_t *mp) {
	if (mp->selfId != XT_INVALID_HANDLE) {
		xt_destroy_object(mp->selfId);
	}
	if (mp->selfRoom != XT_INVALID_HANDLE) {
		xt_destroy_room(mp->selfRoom);
	}
}


static int32_t transferProtoType(int32_t type) {
	switch (type) {
	case TRANSFER_TYPE_UDP:	return XT_NET_TYPE_UDP;
	default:	return XT_NET_TYPE_TCP;
	}
}
mission_publisher_t *MissionMgr::startPublisher(NetClient *nc, struct proto_publish_start_req_t *msg, uint32_t msglen) {
	mission_publisher_t *mp = allocPublisher();
	if (mp != NULL) {
		memset(mp, 0, sizeof(mission_publisher_t));
		NODE_LIST_INIT(mp->header, mp->tailer);
		mp->client = nc;
		n2h_u32(msg->room, mp->endpointRoom);
		n2h_u32(msg->id, mp->endpointId);
		//先只考虑直播 不考虑连麦等情况
		do {
			mp->selfRoom = xt_create_room(XT_ROOM_TYPE_LIVE, (uint64_t)mp);
			if (mp->selfRoom == XT_INVALID_HANDLE)	break;
			mp->selfId = xt_create_network_object(transferProtoType(msg->type), 1, (uint64_t)mp);
			if (mp->selfRoom == XT_INVALID_HANDLE) {
				xt_destroy_room(mp->selfRoom);
				break;
			}
			if (eXsuccess != xt_add_object_to_room(mp->selfId, mp->selfRoom)) {
				xt_destroy_object(mp->selfId);
				xt_destroy_room(mp->selfRoom);
				break;
			}
			LOGGER_Info("startPublisher endpoint room %d id %d self room %d id %d",
				mp->endpointRoom, mp->endpointId, mp->selfRoom, mp->selfId);
			//被动模式,等待对方来连接
			switch (msg->type) {
			case TRANSFER_TYPE_UDP:
				xt_network_set_ip4_udp(mp->selfId, mp->endpointRoom, mp->endpointId, NULL, NULL, 0, 1);
				break;
			default:
				xt_network_set_ip4_tcp(mp->selfId, mp->endpointRoom, mp->endpointId, NULL, 0, 1);
				break;
			}
			NODE_LIST_ADD_TAIL(m_tailer, mp);
			return mp;
		} while (0);
		freePublisher(mp);
	}
	return NULL;
}

void MissionMgr::stopPublisher(NetClient *nc, mission_publisher_t *mp) {
	//先移除
	NODE_LIST_REMOVE(mp);
	//通知所有的对象
	for (mission_subscriber_t *m = mp->header.next; m != &mp->tailer; ) {
		mission_subscriber_t *ms = m;
		m = m->next;
		ms->client->onPublisherClosed(ms);
		NODE_LIST_REMOVE(ms);
		closeSubscribe(ms);
		freeSubscriber(ms);
	}
	closePublisher(mp);
	freePublisher(mp);
}

mission_subscriber_t *MissionMgr::startSubscriber(NetClient *nc, mission_publisher_t *mpub,
	struct proto_subscribe_start_req_t *msg, uint32_t msglen) {
	mission_subscriber_t *ms = allocSubscriber();
	if (ms != NULL) {
		memset(ms, 0, sizeof(mission_subscriber_t));
		ms->publisher = mpub;
		ms->client = nc;
		n2h_u32(msg->room, ms->endpointRoom);
		n2h_u32(msg->id, ms->endpointId);
		do {
			ms->selfId = xt_create_network_object(transferProtoType(msg->type), 0, (uint64_t)ms);
			if (ms->selfId == XT_INVALID_HANDLE) {
				break;
			}
			if (eXsuccess != xt_add_object_to_room(ms->selfId, mpub->selfRoom)) {
				xt_destroy_object(ms->selfId);
				break;
			}
			LOGGER_Info("startSubscribe endpoint room %d id %d self room %d id %d", 
				ms->endpointRoom, ms->endpointId, mpub->selfRoom, ms->selfId);
			switch (msg->type) {
			case TRANSFER_TYPE_UDP:
				xt_network_set_ip4_udp(ms->selfId, ms->endpointRoom, ms->endpointId, NULL, NULL, 0, 0);
				break;
			default:
				xt_network_set_ip4_tcp(ms->selfId, ms->endpointRoom, ms->endpointId, NULL, 0, 0);
				break;
			}
			NODE_LIST_ADD_TAIL(mpub->tailer, ms);
			return ms;
		} while (0);
		freeSubscriber(ms);
	}
	return NULL;
}

void MissionMgr::stopSubscriber(NetClient *nc, proto_subscribe_stop_t *msg, uint32_t msglen) {
	int32_t room, id;
	n2h_u32(msg->cltRoom, room);
	n2h_u32(msg->cltId, id);
	for (mission_publisher_t *p = first(); p != tailer(); p = p->next) {
		for (mission_subscriber_t *ms = p->header.next; ms != &p->tailer; ms = ms->next) {
			if (ms->client == nc && ms->endpointRoom == room && ms->endpointId == id) {
				NODE_LIST_REMOVE(ms);
				closeSubscribe(ms);
				freeSubscriber(ms);
				return;
			}
		}
	}
}

