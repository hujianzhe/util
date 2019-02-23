//
// Created by hujianzhe on 18-8-13.
//

#ifndef	UTIL_C_COMPONENT_NIOSOCKET_H
#define	UTIL_C_COMPONENT_NIOSOCKET_H

#include "../syslib/io.h"
#include "../syslib/socket.h"
#include "../syslib/time.h"
#include "../datastruct/hashtable.h"
#include "dataqueue.h"

typedef struct NioSocketLoop_t {
	unsigned char initok;
	Reactor_t m_reactor;
	FD_t m_socketpair[2];
	void* m_readOl;
	DataQueue_t* m_msgdq;
	DataQueue_t* m_senddq;
	CriticalSection_t m_msglistlock;
	List_t m_msglist;
	Hashtable_t m_sockht;
	HashtableNode_t* m_sockht_bulks[2048];
} NioSocketLoop_t;

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
	union {
		void* accept_callback_arg;
		struct sockaddr_storage connect_saddr;
	};
	void* userdata;
	void(*reg_callback)(struct NioSocket_t*, int);
	void(*accept_callback)(FD_t, struct sockaddr_storage*, void*);
	void(*connect_callback)(struct NioSocket_t*, int);
	int(*decode_packet)(struct NioSocket_t*, unsigned char*, size_t, const struct sockaddr_storage*, NioSocketMsg_t**);
	void(*close)(struct NioSocket_t*);
/* private */
	NioSocketMsg_t m_msg;
	NioSocketMsg_t m_sendmsg;
	HashtableNode_t m_hashnode;
	NioSocketLoop_t* m_loop;
	void(*m_free)(struct NioSocket_t*);
	void* m_readOl;
	void* m_writeOl;
	time_t m_lastActiveTime;
	unsigned char *m_inbuf;
	size_t m_inbuflen;
	List_t m_outbuflist;
} NioSocket_t;

#ifdef __cplusplus
extern "C" {
#endif

__declspec_dll NioSocket_t* niosocketCreate(FD_t fd, int domain, int type, int protocol, NioSocket_t*(*pmalloc)(void), void(*pfree)(NioSocket_t*));
__declspec_dll void niosocketFree(NioSocket_t* s);
__declspec_dll int niosocketSend(NioSocket_t* s, const void* data, unsigned int len, const struct sockaddr_storage* saddr);
__declspec_dll int niosocketSendv(NioSocket_t* s, Iobuf_t iov[], unsigned int iovcnt, const struct sockaddr_storage* saddr);
__declspec_dll void niosocketShutdown(NioSocket_t* s);
__declspec_dll NioSocketLoop_t* niosocketloopCreate(NioSocketLoop_t* loop, DataQueue_t* msgdq, DataQueue_t* senddq);
__declspec_dll void niosocketloopHandler(NioSocketLoop_t* loop, int* wait_msec);
__declspec_dll void niosocketloopReg(NioSocketLoop_t* loop, NioSocket_t* s[], size_t n);
__declspec_dll void niosocketloopDestroy(NioSocketLoop_t* loop);
__declspec_dll void niosocketsendHandler(DataQueue_t* dq, int max_wait_msec, size_t popcnt);
__declspec_dll void niosocketsendClean(DataQueue_t* dq);
__declspec_dll void niosocketmsgHandler(DataQueue_t* dq, int max_wait_msec, void (*user_msg_callback)(NioSocketMsg_t*, void*), void* arg);
__declspec_dll void niosocketmsgClean(DataQueue_t* dq, void(*deleter)(NioSocketMsg_t*));

#ifdef __cplusplus
}
#endif

#endif
