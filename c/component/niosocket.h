//
// Created by hujianzhe on 18-8-13.
//

#ifndef	UTIL_C_COMPONENT_NIOSOCKET_H
#define	UTIL_C_COMPONENT_NIOSOCKET_H

#include "../syslib/io.h"
#include "../syslib/process.h"
#include "../syslib/socket.h"
#include "../syslib/time.h"
#include "../datastruct/hashtable.h"
#include "dataqueue.h"

typedef struct NioSocketLoop_t {
	volatile char valid;
	unsigned char initok;
	Thread_t m_handle;
	Reactor_t m_reactor;
	FD_t m_socketpair[2];
	void* m_readOl;
	DataQueue_t m_dq;
	DataQueue_t* m_msgdq;
	Hashtable_t m_sockht;
	HashtableNode_t* m_sockht_bulks[2048];
} NioSocketLoop_t;

enum {
	NIO_SOCKET_USER_MESSAGE,
	NIO_SOCKET_CLOSE_MESSAGE,
	NIO_SOCKET_REG_MESSAGE
};
typedef struct NioSocketMsg_t {
	ListNode_t m_listnode;
	int type;
} NioSocketMsg_t;
typedef struct NioSocket_t {
	FD_t fd;
	int domain;
	int socktype;
	int protocol;
	volatile char valid;
	volatile int timeout_second;
	NioSocketLoop_t* loop;
	void (*accept_callback)(FD_t, struct sockaddr_storage*, void*);
	void (*connect_callback)(struct NioSocket_t*, int);
	union {
		void* accept_callback_arg;
		struct sockaddr_storage connect_saddr;
	};
	int(*decode_packet)(struct NioSocket_t*, unsigned char*, size_t, struct sockaddr_storage*);
	int(*send_packet)(struct NioSocket_t*, Iobuf_t[], unsigned int, struct sockaddr_storage*);
	void(*close)(struct NioSocket_t*);
	void(*free)(struct NioSocket_t*);
/* private */
	NioSocketMsg_t m_msg;
	HashtableNode_t m_hashnode;
	void* m_readOl;
	void* m_writeOl;
	time_t m_lastActiveTime;
	char m_writeCommit;
	unsigned char *m_inbuf;
	size_t m_inbuflen;
	CriticalSection_t m_outbufLock;
	List_t m_outbuflist;
} NioSocket_t;

#ifdef __cplusplus
extern "C" {
#endif

UTIL_LIBAPI NioSocket_t* niosocketCreate(FD_t fd, int domain, int type, int protocol,
	NioSocket_t*(*pmalloc)(void), void(*pfree)(NioSocket_t*));
UTIL_LIBAPI void niosocketFree(NioSocket_t* s);
UTIL_LIBAPI int niosocketSendv(NioSocket_t* s, Iobuf_t iov[], unsigned int iovcnt, struct sockaddr_storage*);
UTIL_LIBAPI void niosocketShutdown(NioSocket_t* s);
UTIL_LIBAPI NioSocketLoop_t* niosocketloopCreate(NioSocketLoop_t* loop, DataQueue_t* msgdq);
UTIL_LIBAPI void niosocketloopAdd(NioSocketLoop_t* loop, NioSocket_t* s[], size_t n);
UTIL_LIBAPI void niosocketloopJoin(NioSocketLoop_t* loop);
UTIL_LIBAPI void niosocketmsgHandler(DataQueue_t* dq, int max_wait_msec, void (*user_msg_callback)(NioSocketMsg_t*, void*), void* arg);
UTIL_LIBAPI void niosocketmsgClean(DataQueue_t* dq, void(*deleter)(NioSocketMsg_t*));

#ifdef __cplusplus
}
#endif

#endif
