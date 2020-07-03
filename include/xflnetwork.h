
#ifndef _XflNetwork_HH
#define _XflNetwork_HH

/**
 * 网路库函数封装
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

	///2020-05-30增加简单的网络函数，简化外部应用
	struct xt_network_t{
		//版本号 初始值为1
		int32_t version;

		/* 创建TCP socket
		* 返回：socket XT_INVALID_HANDLE 表示失败
		*/
		int32_t (*create)();

		/* 销毁 socket
		* 参数：
		*   socket：tcp连接
		* 返回：无
		*/
		void (*destroy)(int32_t socket);

		/* 设置socket阻塞模式
		* 参数：
		*   socket：tcp连接
		*   block: 1:阻塞模式 0非阻塞
		* 返回：错误码
		*/
		int32_t (*set_blocking)(int32_t socket, int block);

		/* 设置socket缓冲区大小
		* 参数：
		*   socket：tcp连接
		*   send: 发送缓冲区大小 <= 0表示不设置 最小值4K
		*   recv: 接收缓冲区大小 <= 0表示不设置 最小值4K
		* 返回：错误码
		*/
		int32_t	(*set_bufsize)(int32_t socket, int send, int recv);

		/* tcp绑定ip 端口
		* 参数：
		*   socket：socket
		*   ip：ipv4地址 为空表示绑定所有网卡
		*   port：tcp端口
		* 返回：错误码
		*/
		int32_t (*bind)(int32_t socket, const char *ip, int port);

		/* tcp 监听
		* 参数：
		*   socket：socket
		* 返回：错误码
		*/
		int32_t (*listen)(int32_t socket);

		/* tcp 接收套接字
		* 参数：
		*   socket：socket
		*   ip：不为空则设置对方ip地址
		*   port：对方端口
		* 返回：接收到的套接字
		*/
		int32_t (*accept)(int32_t socket, char *ip, int *port);

		/* tcp连接对方
		* 参数：
		*   socket：socket
		*   ip：ipv4地址
		*   port：tcp端口
		* 返回：错误码
		*/
		int32_t (*connect)(int32_t socket, const char *ip, int port);

		/* 接收数据
		* 参数：
		*   socket：
		*   buf：缓冲区
		*   size：缓冲区大小
		* 返回：接收到的数据长度
		*/
		int (*recv)(int32_t socket, char *buf, int size);

		/* 发送数据
		* 参数：
		*   socket：
		*   data：数据
		*   len：数据长度
		* 返回：已发送数据长度
		*/
		int (*send)(int32_t socket, const char *data, int len);

		/* 监测io状态
		* 参数：
		*   ioLists：socket列表
		*   count：列表长度
		*   timeoutMs：等待的毫秒数
		* 返回：>0 表示有数据  0 没有数据 
		*/
		int (*select)(xt_io_event_t *ioLists, int count, int timeoutMs);
	};

#ifdef __cplusplus
};
#endif


#endif