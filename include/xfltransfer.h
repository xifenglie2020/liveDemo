
#ifndef _XflTransfer_HH
#define _XflTransfer_HH

/**
 * 流媒体传输库
 * 支持tcp、udp混合传输
 * 公司：广州有孚科技有限公司
 * 作者：Shimin Gong
 * 更新时间：05/08/2020
 * 技术支持：13126484073@qq.com
 */

#ifdef _USE_STDINT_H
#include <stdint.h>
#else
typedef signed char        int8_t;
typedef short              int16_t;
typedef int                int32_t;
typedef long long          int64_t;
typedef unsigned char      uint8_t;
typedef unsigned short     uint16_t;
typedef unsigned int       uint32_t;
typedef unsigned long long uint64_t;
#endif

#if (defined WIN32 || defined WIN64 || defined _WIN64)
	//FOR windows
	#ifdef XTEXPORTS
	#define XTAPI(Type)		__declspec(dllexport)	Type
	#else
	#define XTAPI(Type)		__declspec(dllimport)	Type
	#endif
#else
	//FOR linux
	#if defined(XTEXPORTS) && defined(__GNUC__) && \
		((__GNUC__ >= 4) || (__GNUC__ == 3 && __GNUC_MINOR__ >= 3))
	#define XTAPI(Type)		__attribute__((visibility("default")))	Type
	#else
	#define XTAPI(Type)		Type
	#endif
#endif

#define XT_INVALID_HANDLE	(-1)

#ifdef __cplusplus
extern "C"{
#endif
	typedef struct xt_network_t xt_network_t;

	struct xt_init_params_t{
		char bindip[32];	//绑定ip地址 不填表示绑定所有
		int32_t tcpPort;	//tcp监听端口 0表示不监听（用于客户端） 
		int32_t udpPort;	//udp监听端口 0表示不监听 
							//-1表示使用随机udp端口，客户端或者NAT时使用，
							//>0表示CS模式服务器端，防火墙、端口映射时应放开连续（1+2倍工作线程数）的端口
	};
	struct xt_init_attrs_t{
		int32_t threadCount;	//工作线程数 默认 4
		int32_t maxRooms;		//最大房间数 默认16
		int32_t maxConnections;	//最大连接数 默认128 根据网卡带宽和内存进行调整 无上限 包含network and local
		int32_t sendCacheSize;	//单个连接发送缓冲区大小 默认1024KB
		int32_t recvCacheSize;	//单个连接接收缓冲区大小 默认512KB	如果需要视频会议或者连麦 建议和sendCacheSize一致
		int32_t udpSendSize;	//UDP 发送缓冲区大小 默认4096KB 建议根据socket连接数设置
		int32_t udpRecvSize;	//UDP 接收缓冲区大小 默认4096KB	建议根据socket连接数设置
		int32_t bandWidth;		//带宽限制 单位bit 默认20Mbps 
		int32_t logLevel;		//日志级别
	};

	//获取版本信息
	XTAPI(uint32_t) xt_version();

	//初始化
	XTAPI(int32_t) xt_startup(xt_init_params_t *params, xt_init_attrs_t *attrs);

	//从配置文件初始化
	XTAPI(int32_t) xt_startup_byfile(xt_init_params_t *params, const char *attrFile);

	//反初始化
	XTAPI(void) xt_cleanup();

	XTAPI(const xt_init_attrs_t *)xt_peek_init_attrs();

	XTAPI(const xt_init_params_t *)xt_peek_init_params();

#define XT_ROOM_TYPE_LIVE		1	//实时-广播
#define XT_ROOM_TYPE_MICROPHONE	2	//直播-仅连麦 不回送数据
#define XT_ROOM_TYPE_MEETING	3	//会议
#define XT_ROOM_TYPE_DOWNLOAD	4	//下载	
	/* 创建房间
	 * 参数：
	 *   type:房间类型
	 *	 tag:用户自定义参数
	 *	 relationRoom: 表示新房间和relationRoom部署在同一工作线程上
	 * 返回：房间id号
	 */
	XTAPI(int32_t) xt_create_room(int32_t type, uint64_t tag, int32_t relationRoom = XT_INVALID_HANDLE);

	/* 销毁房间
	 * 参数：
	 *   room：房间号
	 */
	XTAPI(void) xt_destroy_room(int32_t room);

	/* 更新房间用户数据
	 * 参数：
	 *  room：房间号
	 *  tag:用户自定义数据
	 */
	XTAPI(void) xt_update_room_tag(int32_t room, uint64_t tag);

	/* 获取房间用户数据
 	 * 参数：
	 *  room：房间号
	 * 返回：用户自定义参数
	 */
	XTAPI(uint64_t) xt_get_room_tag(int32_t room);
	
	/* 设置房间属性
	* 参数：
	*  room：房间号
	*  key：参数名
	*  value：参数值
	* "closeWhenPublishClosed" :  对XT_ROOM_TYPE_LIVE XT_ROOM_TYPE_MICROPHONE XT_ROOM_TYPE_METTING有效
	*	    "1" 内容发布者关闭关闭整个房间的其他连接 
	*		"0" 不关闭（默认） 
	* "notifyAllStatus" :  对XT_ROOM_TYPE_LIVE有效 其余内容全部通知
	*	    "1" 通知所有对象 连接断开事件
	*		"0" 仅通知内容发布者 连接断开事件（默认)
	* 返回: eXsuccess成功 其他失败
	*/
	XTAPI(int32_t) xt_set_room_env(int32_t room, const char *key, const char *value);
	
	//XTAPI(int32_t) xt_get_room_client_count(int32_t room);
	/*获取房间下对象数量
	* 参数：
	*   room：房间号
	* 返回: 对象数量
	*/
	XTAPI(int32_t) xt_get_room_object_count(int32_t room);


#define XT_NET_TYPE_TCP	0
#define XT_NET_TYPE_UDP	1
	/* 创建network对象
	* 参数：
	*   netType:对象类型  XT_NET_TYPE_TCP...
	*   publisher: 内容发布者，通过对象接收媒体流
	*	tag:用户自定义数据
	* 返回：对象id
	*/
	XTAPI(int32_t) xt_create_network_object(int32_t netType, int32_t publisher, uint64_t tag);

	/* 创建local对象
	* 参数：
	*   publisher: 内容发布者，通过对象发布媒体流
	*	tag:用户自定义数据
	* 返回：对象id
	*/
	XTAPI(int32_t) xt_create_local_object(int32_t publisher, uint64_t tag);

	/* 销毁对象
	* 参数：
	*   id: 对象id
	* 返回：无
	*/
	XTAPI(void) xt_destroy_object(int32_t id);

	/* 更新对象用户数据
	* 参数：
	*   id: 对象id
	*   tag: 用户数据
	* 返回：无
	*/
	XTAPI(void) xt_update_object_tag(int32_t id, uint64_t tag);

	/*获取对象用户数据
	* 参数：
	*   id：对象id
	* 返回: 用户数据
	*/
	XTAPI(uint64_t) xt_get_object_tag(int32_t id);

	/*将对象加入房间
	* 参数：
	*   id：对象id
	*   room：房间号
	* 返回: eXsuccess成功 其他失败
	*/
	XTAPI(int32_t) xt_add_object_to_room(int32_t id, int32_t room);

	/*获取房间号
	* network对象才能调用。
	* 参数：
	*   id：对象id
	* 返回: 房间号
	*/
	XTAPI(int32_t) xt_get_object_room(int32_t id);

	/*设置对端tcp地址
	* network对象才能调用。
	* 参数：
	*   id：对象id
	*   targetRoom：对端房间号
	*   targetId：对端object id
	*   ip：如果对端是客户端 可以为空
	*   port：如果对端是客户端 可以为0
	*   passive：被动模式
	* 返回: eXsuccess成功 其他失败
	*/
	XTAPI(int32_t) xt_network_set_ip4_tcp(int32_t id, int32_t targetRoom, int32_t targetId, const char *ip, int32_t port, int32_t passive = 0);

	/*设置对端udp地址 
	* network对象才能调用。
	* 参数：
	*   id：object 对象id
	*   targetRoom：对端房间号
	*   targetId：对端object id
	*   natid: NAT时id号（建议使用服务器id号）最大23字节， 非NAT为空
	*   ip：nat或对端ip地址，对于CS模式，如果对端是客户端 可以为空
	*   port：nat或对端udp端口，对于CS模式，如果对端是客户端 可以为0
	*   passive：被动模式
	* 返回: eXsuccess成功 其他失败
	*/
	XTAPI(int32_t) xt_network_set_ip4_udp(int32_t id, int32_t targetRoom, int32_t targetId, const char *natid, const char *ip, int32_t port, int32_t passive = 0);

	const int xt_max_command_data_len = 64;
#define XT_COMMAND_STATUS	31		//连接状态通知 内部触发 data 为xm_command_status_notify_t
#	define XT_STATUS_OFF		0		//offline
#	define XT_STATUS_ON			1		//onbline
#	define XT_STATUS_DESTROIED	255		//publisher对象已删除,远方服务器主动关闭或连接超时 如果需要重连，则需要重新建立对象然后重新向后台请求视频
	struct xm_command_status_notify_t{
		int32_t version;
		int32_t id;		//对象id
		int32_t roomId; //房间号
		int32_t isPublisher; //是否为内容发布者
		int32_t status; //XT_STATUS_OFF ... 
	};

	/*命令通知函数
	* 回调函数需要尽快处理，不要做耗时操作，不能调用 xt_destroy_object xt_destroy_room 函数。
	* 参数：
	*   id：对象id
	*   userdata：用户数据
	*   command：命令id
	*   data：数据
	*   size：数据大小
	* 返回: 无
	*/
	typedef void(*xt_command_callback)(int32_t id, uint64_t userdata, uint8_t command, const uint8_t *data, int32_t size);

	/* 设置命令回调 local对象才可以接收
	* 参数：
	*   id：对象id
	*   func：回调函数
	*   userdata：用户数据
	* 返回: eXsuccess成功  其他失败
	*/
	XTAPI(int32_t) xt_set_command_callback(int32_t id, xt_command_callback func, uint64_t userdata);

	/* 发送命令 local对象才可以发送
	* 参数：
	*   id：对象id
	*   command：命令id  （40-128）
	*   data：数据
	*   size：数据大小  不能超过xt_max_command_data_len
	* 返回: eXsuccess成功 其他失败
	*/
	XTAPI(int32_t) xt_local_send_command(int32_t id, uint8_t command, const uint8_t *data, int32_t size);

#define XT_FRAME_TYPE_DATA_LOW	0	//丢失不影响的数据
#define XT_FRAME_TYPE_DATA		1	//普通数据
#define XT_FRAME_TYPE_USER		2	//用户数据
#define XT_FRAME_TYPE_USER2		3	//用户数据2
#define XT_FRAME_TYPE_EXTEND	4	//扩展帧 sei...
#define XT_FRAME_TYPE_KEY		5	//关键帧 i frame
#define XT_FRAME_TYPE_CORE		6	//核心帧 sps pps (sps pps打包在一起，不能分2个包)

	//媒体流的头部长度最大值
	const int xt_max_media_header_len = 128;
#define XT_MEDIA_TYPE_AUDIO		0	//音频流。
#define XT_MEDIA_TYPE_VIDEO		1	//视频流。
#define XT_MEDIA_TYPE_HEADER	2	//媒体流的头部信息，长度不能超过 xt_max_media_header_len
									//在发送数据流之前可以先发送该包，指定编码类型 帧率等信息。 此时frameType为1表示包含音频数据 0不包含
	
	/*媒体流通知函数
	 * 回调函数需要尽快处理，不要做耗时操作，不能调用 xt_destroy_object xt_destroy_room 函数。
	 * 参数：
	 *   id：对象id
	 *   userdata：用户数据
	 *   extend：扩展使用 目前无效
	 *   streamid：流id，内容主发布者streamid必须为0
	 *   mediaType：视频还是音频 XT_MEDIA_TYPE_AUDIO...
	 *   frameType：帧类型 XT_FRAME_TYPE_DATA...
	 *   data：数据
	 *   size：数据大小
	 * 返回: 正数 表示需要等待的微秒数 
	 *     比如返回500000
	 *       对于历史流 表示需要等待500000微秒再重发该数据。 
	 *       对于实时流 表示丢弃接下来500000微秒的数据 
	*/
	typedef int32_t(*xt_media_callback)(int32_t id, uint64_t userdata, void *extend, 
		uint8_t streamid, uint8_t mediaType, uint8_t frameType, const uint8_t *data, int32_t size);

	/* 设置媒体流回调 local对象才可以接收
	* 参数：
	*   id：object id
	*   func：回调函数
	*   userdata：用户数据
	* 返回: eXsuccess成功  其他失败
	*/
	XTAPI(int32_t) xt_set_media_callback(int32_t id, xt_media_callback func, uint64_t userdata);

	/* 发送媒体流 local对象才可以发送
	* 参数：
	*   id：对象id
	*   streamid：流id，内容主发布者streamid必须为0  
	*   mediaType：视频还是音频 XT_MEDIA_TYPE_AUDIO...
	*   frameType：帧类型 XT_FRAME_TYPE_DATA...
	*   data：数据
	*   size：数据大小
	* 返回: eXsuccess成功  eXretry稍后重发该包 其他失败
	*/
	XTAPI(int32_t) xt_local_send_frame(int32_t id,
		uint8_t streamid, uint8_t mediaType, uint8_t frameType, const uint8_t *data, int32_t size);

	/* 读取配置文件 
	 * 参数：
	 *   pathname：配置文件所在路径
	 *   key_value_callback：回调函数
	 *		格式为	key=value  
	 *				#表示注释
	 *		返回-1 表示停止解析
	 *   userdata：用户数据
	 * 返回: -1 表示停止读取 0 成功
	 */
	XTAPI(int32_t) xt_ini_read(const char *pathname, int (*key_value_callback)(void *userdata, const char *key, char *value), void *userdata);

	/* 获取当前CPU时钟
	 * 返回：微秒数
	 */
	XTAPI(uint64_t) xt_tickcount64();


	///2020-05-30增加简单的网络函数，简化外部应用
	XTAPI(xt_network_t *) xt_network();

#ifdef __cplusplus
};
#endif


#endif