//
// Created by hujianzhe on 18-8-13.
//

#ifndef	UTIL_C_COMPONMENT_NIOSOCKET_H
#define	UTIL_C_COMPONMENT_NIOSOCKET_H

#include "../syslib/io.h"
#include "../syslib/process.h"
#include "../syslib/socket.h"
#include "../syslib/time.h"
#include "../datastruct/hashtable.h"
#include "dataqueue.h"

struct NioSocket_t;
struct NioSocketLoop_t;
struct NioSocketMsg_t;

typedef struct NioSocketLoop_t {
	volatile char valid;
	Thread_t handle;
	Reactor_t reactor;
	DataQueue_t dq;
	DataQueue_t* msgdq;
	hashtable_t sockht;
	hashtable_node_t* sockht_bulks[2048];
} NioSocketLoop_t;

enum {
	SOCKET_CLOSE_MESSAGE,
	SOCKET_REG_MESSAGE,
	SOCKET_USER_MESSAGE
};
typedef struct NioSocketMsg_t {
	list_node_t m_listnode;
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
	REACTOR_ACCEPT_CALLBACK accept_callback;
	void (*connect_callback)(struct NioSocket_t*, int);
	union {
		size_t accept_callback_arg;
		struct sockaddr_storage connect_saddr;
	};
	int (*read)(struct NioSocket_t*, unsigned char*, size_t, struct sockaddr_storage*);
	void (*close)(struct NioSocket_t*);
	// private
	NioSocketMsg_t m_closemsg;
	NioSocketMsg_t m_addmsg;
	hashtable_node_t m_hashnode;
	void* m_readOl;
	void* m_writeOl;
	time_t m_lastActiveTime;
	char m_writeCommit;
	unsigned char *m_inbuf;
	size_t m_inbuflen;
	Mutex_t m_outbufMutex;
	list_t m_outbuflist;
} NioSocket_t;

#ifdef __cplusplus
extern "C" {
#endif

void niosocketMemoryHook(void*(*p_malloc)(size_t), void(*p_free)(void*));
NioSocket_t* niosocketCreate(int domain, int socktype, int protocol);
void niosocketFree(NioSocket_t* s);
int niosocketSendv(NioSocket_t* s, IoBuf_t* iov, unsigned int iovcnt, struct sockaddr_storage*);
void niosocketShutdown(NioSocket_t* s);
NioSocketLoop_t* niosocketloopCreate(NioSocketLoop_t* loop, DataQueue_t* msgdq);
void niosocketloopAdd(NioSocketLoop_t* loop, NioSocket_t* s[], size_t n);
void niosocketloopJoin(NioSocketLoop_t* loop);
void niomsgHandler(DataQueue_t* dq, int max_wait_msec, void (*user_msg_callback)(NioSocketMsg_t*));
void niomsgClean(DataQueue_t* dq, void(*deleter)(NioSocketMsg_t*));

#ifdef __cplusplus
}
#endif

#endif
