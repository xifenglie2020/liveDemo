#pragma once

#include <stdint.h>

#pragma pack(1)

//little endian
#define n2h_u16(net,host)	do{	\
	uint16_t __d, __v = (uint16_t)(net);	\
	char *__p = (char *)&__v;		\
	__d = __p[0] & 0xFF;				\
	host = ((__d) << 8)|(__p[1] & 0xFF);\
}while (0)
#define h2n_u16(host, net)	do{	\
	uint16_t __v = (uint16_t)(host);		\
	char *__p = (char *)&net;		\
	__p[0] = (__v >> 8) & 0xFF;			\
	__p[1] = __v & 0xFF;				\
}while (0)

#define n2h_u32(net,host)	do{	\
	uint32_t __d, __v = (uint32_t)(net);	\
	char *__p = (char *)&__v;		\
	__d = __p[0] & 0xFF;				\
	__d = (__d << 8) | (__p[1] & 0xFF);	\
	__d = (__d << 8) | (__p[2] & 0xFF);	\
	host = (__d << 8) | (__p[3] & 0xFF);\
}while (0)
#define h2n_u32(host, net)	do{	\
	uint32_t __v = (uint32_t)(host);		\
	char *__p = (char *)&net;		\
	__p[0] = (__v >> 24) & 0xFF;		\
	__p[1] = (__v >> 16) & 0xFF;		\
	__p[2] = (__v >> 8) & 0xFF;			\
	__p[3] = (__v)& 0xFF;				\
}while (0)

const uint32_t proto_url_len		= 32;
const uint32_t proto_name_len		= 64;
const uint32_t proto_max_msg_len	= 16 * 1024;

#define VIDEO_TYPE_H264		0
#define VIDEO_TYPE_H265		1
struct xp_media_header_t {
	uint32_t ver;
	struct video_t{
		uint16_t type;	//0  VIDEO_TYPE_H264
		uint8_t fps;
		uint8_t gop;
		uint16_t width;
		uint16_t height;
	}video;
	struct audio_t {
		char pad[16];
	}audio;
};

struct xp_video_keyframe_t {
	uint32_t seqNo;
};

#define TRANSFER_TYPE_TCP	0
#define TRANSFER_TYPE_UDP	1

struct proto_base_t{
	uint16_t len;
	uint16_t cmd;
	uint32_t crypt;	//加密类型
	char to[proto_url_len];
	char from[proto_url_len];
};

#define PRO_CMD_LOGIN_REQ	1
struct proto_login_req_t : proto_base_t {
	uint32_t ver;
	char user[32];
	char pswd[32];	
	char seed[32];
};

#define PRO_CMD_LOGIN_RSP	2
struct proto_login_rsp_t : proto_base_t {
	uint32_t ver;	//服务端版本号
	int32_t error;
};

#define PRO_CMD_LOGOUT		3
#define PRO_CMD_HEARTBEAT	4

#define PRO_CMD_STOPED		5	//停止
struct proto_stoped_t : proto_base_t {
	int32_t srvRoom;
	int32_t srvId;
	int32_t cltRoom;
	int32_t cltId;
	int32_t error;
};

#define PRO_CMD_QUERY_PUB_REQ	6

#define PRO_CMD_QUERY_PUB_RSP	7
struct proto_query_publishers_rsp_t : proto_base_t {
	int32_t start;
	int32_t count;
	struct item_t{
		char url[proto_url_len];
		char name[proto_name_len];
	}items[1];
};

#define PRO_CMD_LOCAL_END		99
#define PRO_CMD_PUBLISH_START_REQ	100
struct proto_publish_start_req_t : proto_base_t {
	int8_t  type;	// TRANSFER_TYPE_UDP 
	int8_t	pad1;
	int16_t pad2;
	int32_t room;
	int32_t id;
};

#define PRO_CMD_PUBLISH_START_RSP	101
struct proto_publish_start_rsp_t : proto_base_t {
	int32_t error;
	int8_t  type;	//tcp udp...
	int8_t  passive : 4;
	int8_t  retry : 4;	//重试
	int16_t pad2;
	int32_t srvRoom;
	int32_t srvId;
	int32_t cltRoom;
	int32_t cltId;
	int32_t srvPort;
	char	srvIp[32];
	char	nat[32];
};

#define PRO_CMD_PUBLISH_STOP		102
struct proto_publish_stop_t : proto_base_t {
	int32_t srvRoom;
	int32_t srvId;
	int32_t cltRoom;
	int32_t cltId;
};


#define PRO_CMD_SUBSCRIBE_START_REQ		110
struct proto_subscribe_start_req_t : proto_base_t {
	uint8_t type;	//tcp udp...
	uint8_t pad;
	int16_t pad2;
	int32_t room;
	int32_t id;
};

#define PRO_CMD_SUBSCRIBE_START_RSP		111
struct proto_subscribe_start_rsp_t : proto_base_t {
	int32_t error;
	uint8_t type;	//tcp udp...
	uint8_t passive : 4;
	uint8_t retry : 4;	//重试
	int16_t pad2;
	int32_t srvRoom;
	int32_t srvId;
	int32_t cltRoom;
	int32_t cltId;
	int32_t srvPort;
	char	srvIp[32];
	char	nat[32];
};

#define PRO_CMD_SUBSCRIBE_STOP			112
struct proto_subscribe_stop_t : proto_base_t {
	int32_t srvRoom;
	int32_t srvId;
	int32_t cltRoom;
	int32_t cltId;
};


#pragma pack()

