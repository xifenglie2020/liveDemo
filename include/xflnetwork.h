
#ifndef _XflNetwork_HH
#define _XflNetwork_HH

/**
 * ��·�⺯����װ
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

#define XT_IO_EVENT_READ	1
#define XT_IO_EVENT_WRITE	(1<<1)

#ifdef __cplusplus
extern "C"{
#endif
	struct xt_io_event_t {
		int32_t socket;
		int32_t eventIn;
		int32_t eventOut;
		void *userData;
	};

	///2020-05-30���Ӽ򵥵����纯�������ⲿӦ��
	struct xt_network_t{
		//�汾�� ��ʼֵΪ1
		int32_t version;

		/* ����TCP socket
		* ���أ�socket XT_INVALID_HANDLE ��ʾʧ��
		*/
		int32_t (*create)();

		/* ���� socket
		* ������
		*   socket��tcp����
		* ���أ���
		*/
		void (*destroy)(int32_t socket);

		/* ����socket����ģʽ
		* ������
		*   socket��tcp����
		*   block: 1:����ģʽ 0������
		* ���أ�������
		*/
		int32_t (*set_blocking)(int32_t socket, int block);

		/* ����socket��������С
		* ������
		*   socket��tcp����
		*   send: ���ͻ�������С <= 0��ʾ������ ��Сֵ4K
		*   recv: ���ջ�������С <= 0��ʾ������ ��Сֵ4K
		* ���أ�������
		*/
		int32_t	(*set_bufsize)(int32_t socket, int send, int recv);

		/* tcp��ip �˿�
		* ������
		*   socket��socket
		*   ip��ipv4��ַ Ϊ�ձ�ʾ����������
		*   port��tcp�˿�
		* ���أ�������
		*/
		int32_t (*bind)(int32_t socket, const char *ip, int port);

		/* tcp ����
		* ������
		*   socket��socket
		* ���أ�������
		*/
		int32_t (*listen)(int32_t socket);

		/* tcp �����׽���
		* ������
		*   socket��socket
		*   ip����Ϊ�������öԷ�ip��ַ
		*   port���Է��˿�
		* ���أ����յ����׽���
		*/
		int32_t (*accept)(int32_t socket, char *ip, int *port);

		/* tcp���ӶԷ�
		* ������
		*   socket��socket
		*   ip��ipv4��ַ
		*   port��tcp�˿�
		* ���أ�������
		*/
		int32_t (*connect)(int32_t socket, const char *ip, int port);

		/* ��������
		* ������
		*   socket��
		*   buf��������
		*   size����������С
		* ���أ����յ������ݳ���
		*/
		int (*recv)(int32_t socket, char *buf, int size);

		/* ��������
		* ������
		*   socket��
		*   data������
		*   len�����ݳ���
		* ���أ��ѷ������ݳ���
		*/
		int (*send)(int32_t socket, const char *data, int len);

		/* ���io״̬
		* ������
		*   ioLists��socket�б�
		*   count���б���
		*   timeoutMs���ȴ��ĺ�����
		* ���أ�>0 ��ʾ������  0 û������ 
		*/
		int (*select)(xt_io_event_t *ioLists, int count, int timeoutMs);
	};

#ifdef __cplusplus
};
#endif


#endif