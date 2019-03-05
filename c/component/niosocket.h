//
// Created by hujianzhe on 18-8-13.
//

#ifndef	UTIL_C_COMPONENT_NIOSOCKET_H
#define	UTIL_C_COMPONENT_NIOSOCKET_H

#include "../syslib/atomic.h"
#include "../syslib/io.h"
#include "../syslib/socket.h"
#include "../syslib/time.h"
#include "../datastruct/hashtable.h"
#include "dataqueue.h"

struct NioSender_t;
typedef struct NioLoop_t {
	unsigned char initok;
	Reactor_t m_reactor;
	FD_t m_socketpair[2];
	void* m_readOl;
	DataQueue_t* m_msgdq;
	struct NioSender_t* m_sender;
	CriticalSection_t m_msglistlock;
	List_t m_msglist;
	Hashtable_t m_sockht;
	HashtableNode_t* m_sockht_bulks[2048];
	long long m_checkexpire_msec;
} NioLoop_t;

typedef struct NioSender_t {
	unsigned char initok;
	DataQueue_t m_dq;
	List_t m_socklist;
	long long m_resend_msec;
} NioSender_t;

typedef struct NioMsg_t {
	ListNode_t m_listnode;
	int type;
} NioMsg_t;

typedef struct NioSocket_t {
	FD_t fd;
	int domain;
	int socktype;
	int protocol;
	volatile char valid;
	volatile int timeout_second;
	void* userdata;
	union {
		void* accept_callback_arg;
		struct sockaddr_storage connect_saddr;
	};
	void(*accept_callback)(FD_t, struct sockaddr_storage*, void*);
	void(*connect_callback)(struct NioSocket_t*, int);
	int(*decode_packet)(struct NioSocket_t*, unsigned char*, size_t, const struct sockaddr_storage*, NioMsg_t**);
	void(*reg_callback)(struct NioSocket_t*, int);
	void(*close)(struct NioSocket_t*);
/* private */
	Atom32_t m_shutdown;
	NioMsg_t m_regmsg;
	NioMsg_t m_shutdownmsg;
	NioMsg_t m_closemsg;
	NioMsg_t m_sendmsg;
	HashtableNode_t m_hashnode;
	ListNode_t m_senderlistnode;
	NioLoop_t* m_loop;
	void(*m_free)(struct NioSocket_t*);
	void* m_readOl;
	void* m_writeOl;
	time_t m_lastActiveTime;
	unsigned char *m_inbuf;
	size_t m_inbuflen;
	List_t m_recvpacketlist;
	List_t m_sendpacketlist;
	struct {
		unsigned int enable;
		unsigned int rto;
		unsigned int m_cwndseq;
		unsigned int m_cwndsize;
		unsigned int m_recvseq;
		unsigned int m_sendseq;
	} reliable;
} NioSocket_t;

#ifdef __cplusplus
extern "C" {
#endif

__declspec_dll NioSocket_t* niosocketCreate(FD_t fd, int domain, int type, int protocol, NioSocket_t*(*pmalloc)(void), void(*pfree)(NioSocket_t*));
__declspec_dll void niosocketFree(NioSocket_t* s);
__declspec_dll NioSocket_t* niosocketSend(NioSocket_t* s, const void* data, unsigned int len, const struct sockaddr_storage* saddr);
__declspec_dll NioSocket_t* niosocketSendv(NioSocket_t* s, Iobuf_t iov[], unsigned int iovcnt, const struct sockaddr_storage* saddr);
__declspec_dll void niosocketShutdown(NioSocket_t* s);
__declspec_dll NioLoop_t* nioloopCreate(NioLoop_t* loop, DataQueue_t* msgdq, NioSender_t* sender);
__declspec_dll void nioloopHandler(NioLoop_t* loop, long long timestamp_msec, int wait_msec);
__declspec_dll void nioloopReg(NioLoop_t* loop, NioSocket_t* s[], size_t n);
__declspec_dll void nioloopDestroy(NioLoop_t* loop);
__declspec_dll NioSender_t* niosenderCreate(NioSender_t* sender);
__declspec_dll void niosenderHandler(NioSender_t* sender, long long timestamp_msec, int wait_msec);
__declspec_dll void niosenderDestroy(NioSender_t* sender);
__declspec_dll void niomsgHandler(DataQueue_t* dq, int max_wait_msec, void (*user_msg_callback)(NioMsg_t*, void*), void* arg);
__declspec_dll void niomsgClean(DataQueue_t* dq, void(*deleter)(NioMsg_t*));

#ifdef __cplusplus
}
#endif

#endif
