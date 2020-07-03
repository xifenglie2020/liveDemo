
#ifndef _XflTransfer_HH
#define _XflTransfer_HH

/**
 * ��ý�崫���
 * ֧��tcp��udp��ϴ���
 * ��˾���������ڿƼ����޹�˾
 * ���ߣ�Shimin Gong
 * ����ʱ�䣺05/08/2020
 * ����֧�֣�13126484073@qq.com
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
		char bindip[32];	//��ip��ַ �����ʾ������
		int32_t tcpPort;	//tcp�����˿� 0��ʾ�����������ڿͻ��ˣ� 
		int32_t udpPort;	//udp�����˿� 0��ʾ������ 
							//-1��ʾʹ�����udp�˿ڣ��ͻ��˻���NATʱʹ�ã�
							//>0��ʾCSģʽ�������ˣ�����ǽ���˿�ӳ��ʱӦ�ſ�������1+2�������߳������Ķ˿�
	};
	struct xt_init_attrs_t{
		int32_t threadCount;	//�����߳��� Ĭ�� 4
		int32_t maxRooms;		//��󷿼��� Ĭ��16
		int32_t maxConnections;	//��������� Ĭ��128 ��������������ڴ���е��� ������ ����network and local
		int32_t sendCacheSize;	//�������ӷ��ͻ�������С Ĭ��1024KB
		int32_t recvCacheSize;	//�������ӽ��ջ�������С Ĭ��512KB	�����Ҫ��Ƶ����������� �����sendCacheSizeһ��
		int32_t udpSendSize;	//UDP ���ͻ�������С Ĭ��4096KB �������socket����������
		int32_t udpRecvSize;	//UDP ���ջ�������С Ĭ��4096KB	�������socket����������
		int32_t bandWidth;		//�������� ��λbit Ĭ��20Mbps 
		int32_t logLevel;		//��־����
	};

	//��ȡ�汾��Ϣ
	XTAPI(uint32_t) xt_version();

	//��ʼ��
	XTAPI(int32_t) xt_startup(xt_init_params_t *params, xt_init_attrs_t *attrs);

	//�������ļ���ʼ��
	XTAPI(int32_t) xt_startup_byfile(xt_init_params_t *params, const char *attrFile);

	//����ʼ��
	XTAPI(void) xt_cleanup();

	XTAPI(const xt_init_attrs_t *)xt_peek_init_attrs();

	XTAPI(const xt_init_params_t *)xt_peek_init_params();

#define XT_ROOM_TYPE_LIVE		1	//ʵʱ-�㲥
#define XT_ROOM_TYPE_MICROPHONE	2	//ֱ��-������ ����������
#define XT_ROOM_TYPE_MEETING	3	//����
#define XT_ROOM_TYPE_DOWNLOAD	4	//����	
	/* ��������
	 * ������
	 *   type:��������
	 *	 tag:�û��Զ������
	 *	 relationRoom: ��ʾ�·����relationRoom������ͬһ�����߳���
	 * ���أ�����id��
	 */
	XTAPI(int32_t) xt_create_room(int32_t type, uint64_t tag, int32_t relationRoom = XT_INVALID_HANDLE);

	/* ���ٷ���
	 * ������
	 *   room�������
	 */
	XTAPI(void) xt_destroy_room(int32_t room);

	/* ���·����û�����
	 * ������
	 *  room�������
	 *  tag:�û��Զ�������
	 */
	XTAPI(void) xt_update_room_tag(int32_t room, uint64_t tag);

	/* ��ȡ�����û�����
 	 * ������
	 *  room�������
	 * ���أ��û��Զ������
	 */
	XTAPI(uint64_t) xt_get_room_tag(int32_t room);
	
	/* ���÷�������
	* ������
	*  room�������
	*  key��������
	*  value������ֵ
	* "closeWhenPublishClosed" :  ��XT_ROOM_TYPE_LIVE XT_ROOM_TYPE_MICROPHONE XT_ROOM_TYPE_METTING��Ч
	*	    "1" ���ݷ����߹رչر������������������ 
	*		"0" ���رգ�Ĭ�ϣ� 
	* "notifyAllStatus" :  ��XT_ROOM_TYPE_LIVE��Ч ��������ȫ��֪ͨ
	*	    "1" ֪ͨ���ж��� ���ӶϿ��¼�
	*		"0" ��֪ͨ���ݷ����� ���ӶϿ��¼���Ĭ��)
	* ����: eXsuccess�ɹ� ����ʧ��
	*/
	XTAPI(int32_t) xt_set_room_env(int32_t room, const char *key, const char *value);
	
	//XTAPI(int32_t) xt_get_room_client_count(int32_t room);
	/*��ȡ�����¶�������
	* ������
	*   room�������
	* ����: ��������
	*/
	XTAPI(int32_t) xt_get_room_object_count(int32_t room);


#define XT_NET_TYPE_TCP	0
#define XT_NET_TYPE_UDP	1
	/* ����network����
	* ������
	*   netType:��������  XT_NET_TYPE_TCP...
	*   publisher: ���ݷ����ߣ�ͨ���������ý����
	*	tag:�û��Զ�������
	* ���أ�����id
	*/
	XTAPI(int32_t) xt_create_network_object(int32_t netType, int32_t publisher, uint64_t tag);

	/* ����local����
	* ������
	*   publisher: ���ݷ����ߣ�ͨ�����󷢲�ý����
	*	tag:�û��Զ�������
	* ���أ�����id
	*/
	XTAPI(int32_t) xt_create_local_object(int32_t publisher, uint64_t tag);

	/* ���ٶ���
	* ������
	*   id: ����id
	* ���أ���
	*/
	XTAPI(void) xt_destroy_object(int32_t id);

	/* ���¶����û�����
	* ������
	*   id: ����id
	*   tag: �û�����
	* ���أ���
	*/
	XTAPI(void) xt_update_object_tag(int32_t id, uint64_t tag);

	/*��ȡ�����û�����
	* ������
	*   id������id
	* ����: �û�����
	*/
	XTAPI(uint64_t) xt_get_object_tag(int32_t id);

	/*��������뷿��
	* ������
	*   id������id
	*   room�������
	* ����: eXsuccess�ɹ� ����ʧ��
	*/
	XTAPI(int32_t) xt_add_object_to_room(int32_t id, int32_t room);

	/*��ȡ�����
	* network������ܵ��á�
	* ������
	*   id������id
	* ����: �����
	*/
	XTAPI(int32_t) xt_get_object_room(int32_t id);

	/*���öԶ�tcp��ַ
	* network������ܵ��á�
	* ������
	*   id������id
	*   targetRoom���Զ˷����
	*   targetId���Զ�object id
	*   ip������Զ��ǿͻ��� ����Ϊ��
	*   port������Զ��ǿͻ��� ����Ϊ0
	*   passive������ģʽ
	* ����: eXsuccess�ɹ� ����ʧ��
	*/
	XTAPI(int32_t) xt_network_set_ip4_tcp(int32_t id, int32_t targetRoom, int32_t targetId, const char *ip, int32_t port, int32_t passive = 0);

	/*���öԶ�udp��ַ 
	* network������ܵ��á�
	* ������
	*   id��object ����id
	*   targetRoom���Զ˷����
	*   targetId���Զ�object id
	*   natid: NATʱid�ţ�����ʹ�÷�����id�ţ����23�ֽڣ� ��NATΪ��
	*   ip��nat��Զ�ip��ַ������CSģʽ������Զ��ǿͻ��� ����Ϊ��
	*   port��nat��Զ�udp�˿ڣ�����CSģʽ������Զ��ǿͻ��� ����Ϊ0
	*   passive������ģʽ
	* ����: eXsuccess�ɹ� ����ʧ��
	*/
	XTAPI(int32_t) xt_network_set_ip4_udp(int32_t id, int32_t targetRoom, int32_t targetId, const char *natid, const char *ip, int32_t port, int32_t passive = 0);

	const int xt_max_command_data_len = 64;
#define XT_COMMAND_STATUS	31		//����״̬֪ͨ �ڲ����� data Ϊxm_command_status_notify_t
#	define XT_STATUS_OFF		0		//offline
#	define XT_STATUS_ON			1		//onbline
#	define XT_STATUS_DESTROIED	255		//publisher������ɾ��,Զ�������������رջ����ӳ�ʱ �����Ҫ����������Ҫ���½�������Ȼ���������̨������Ƶ
	struct xm_command_status_notify_t{
		int32_t version;
		int32_t id;		//����id
		int32_t roomId; //�����
		int32_t isPublisher; //�Ƿ�Ϊ���ݷ�����
		int32_t status; //XT_STATUS_OFF ... 
	};

	/*����֪ͨ����
	* �ص�������Ҫ���촦����Ҫ����ʱ���������ܵ��� xt_destroy_object xt_destroy_room ������
	* ������
	*   id������id
	*   userdata���û�����
	*   command������id
	*   data������
	*   size�����ݴ�С
	* ����: ��
	*/
	typedef void(*xt_command_callback)(int32_t id, uint64_t userdata, uint8_t command, const uint8_t *data, int32_t size);

	/* ��������ص� local����ſ��Խ���
	* ������
	*   id������id
	*   func���ص�����
	*   userdata���û�����
	* ����: eXsuccess�ɹ�  ����ʧ��
	*/
	XTAPI(int32_t) xt_set_command_callback(int32_t id, xt_command_callback func, uint64_t userdata);

	/* �������� local����ſ��Է���
	* ������
	*   id������id
	*   command������id  ��40-128��
	*   data������
	*   size�����ݴ�С  ���ܳ���xt_max_command_data_len
	* ����: eXsuccess�ɹ� ����ʧ��
	*/
	XTAPI(int32_t) xt_local_send_command(int32_t id, uint8_t command, const uint8_t *data, int32_t size);

#define XT_FRAME_TYPE_DATA_LOW	0	//��ʧ��Ӱ�������
#define XT_FRAME_TYPE_DATA		1	//��ͨ����
#define XT_FRAME_TYPE_USER		2	//�û�����
#define XT_FRAME_TYPE_USER2		3	//�û�����2
#define XT_FRAME_TYPE_EXTEND	4	//��չ֡ sei...
#define XT_FRAME_TYPE_KEY		5	//�ؼ�֡ i frame
#define XT_FRAME_TYPE_CORE		6	//����֡ sps pps (sps pps�����һ�𣬲��ܷ�2����)

	//ý������ͷ���������ֵ
	const int xt_max_media_header_len = 128;
#define XT_MEDIA_TYPE_AUDIO		0	//��Ƶ����
#define XT_MEDIA_TYPE_VIDEO		1	//��Ƶ����
#define XT_MEDIA_TYPE_HEADER	2	//ý������ͷ����Ϣ�����Ȳ��ܳ��� xt_max_media_header_len
									//�ڷ���������֮ǰ�����ȷ��͸ð���ָ���������� ֡�ʵ���Ϣ�� ��ʱframeTypeΪ1��ʾ������Ƶ���� 0������
	
	/*ý����֪ͨ����
	 * �ص�������Ҫ���촦����Ҫ����ʱ���������ܵ��� xt_destroy_object xt_destroy_room ������
	 * ������
	 *   id������id
	 *   userdata���û�����
	 *   extend����չʹ�� Ŀǰ��Ч
	 *   streamid����id��������������streamid����Ϊ0
	 *   mediaType����Ƶ������Ƶ XT_MEDIA_TYPE_AUDIO...
	 *   frameType��֡���� XT_FRAME_TYPE_DATA...
	 *   data������
	 *   size�����ݴ�С
	 * ����: ���� ��ʾ��Ҫ�ȴ���΢���� 
	 *     ���緵��500000
	 *       ������ʷ�� ��ʾ��Ҫ�ȴ�500000΢�����ط������ݡ� 
	 *       ����ʵʱ�� ��ʾ����������500000΢������� 
	*/
	typedef int32_t(*xt_media_callback)(int32_t id, uint64_t userdata, void *extend, 
		uint8_t streamid, uint8_t mediaType, uint8_t frameType, const uint8_t *data, int32_t size);

	/* ����ý�����ص� local����ſ��Խ���
	* ������
	*   id��object id
	*   func���ص�����
	*   userdata���û�����
	* ����: eXsuccess�ɹ�  ����ʧ��
	*/
	XTAPI(int32_t) xt_set_media_callback(int32_t id, xt_media_callback func, uint64_t userdata);

	/* ����ý���� local����ſ��Է���
	* ������
	*   id������id
	*   streamid����id��������������streamid����Ϊ0  
	*   mediaType����Ƶ������Ƶ XT_MEDIA_TYPE_AUDIO...
	*   frameType��֡���� XT_FRAME_TYPE_DATA...
	*   data������
	*   size�����ݴ�С
	* ����: eXsuccess�ɹ�  eXretry�Ժ��ط��ð� ����ʧ��
	*/
	XTAPI(int32_t) xt_local_send_frame(int32_t id,
		uint8_t streamid, uint8_t mediaType, uint8_t frameType, const uint8_t *data, int32_t size);

	/* ��ȡ�����ļ� 
	 * ������
	 *   pathname�������ļ�����·��
	 *   key_value_callback���ص�����
	 *		��ʽΪ	key=value  
	 *				#��ʾע��
	 *		����-1 ��ʾֹͣ����
	 *   userdata���û�����
	 * ����: -1 ��ʾֹͣ��ȡ 0 �ɹ�
	 */
	XTAPI(int32_t) xt_ini_read(const char *pathname, int (*key_value_callback)(void *userdata, const char *key, char *value), void *userdata);

	/* ��ȡ��ǰCPUʱ��
	 * ���أ�΢����
	 */
	XTAPI(uint64_t) xt_tickcount64();


	///2020-05-30���Ӽ򵥵����纯�������ⲿӦ��
	XTAPI(xt_network_t *) xt_network();

#ifdef __cplusplus
};
#endif


#endif