//
// Created by hujianzhe on 18-8-13.
//

#ifndef	UTIL_C_COMPONENT_SESSION_H
#define	UTIL_C_COMPONENT_SESSION_H

#include "../sysapi/atomic.h"
#include "../sysapi/io.h"
#include "../sysapi/ipc.h"
#include "../sysapi/process.h"
#include "../sysapi/socket.h"
#include "../datastruct/list.h"
#include "../datastruct/hashtable.h"

typedef struct SessionLoop_t {
	unsigned char m_initok;
	unsigned char m_runthreadhasbind;
	Thread_t m_runthread;
	Atom16_t m_wake;
	Nio_t m_reactor;
	FD_t m_socketpair[2];
	void* m_readol;
	CriticalSection_t m_msglistlock;
	List_t m_msglist;
	List_t m_closelist;
	Hashtable_t m_sockht;
	HashtableNode_t* m_sockht_bulks[2048];
	long long m_event_msec;
} SessionLoop_t;

enum {
	SESSION_USER_MESSAGE,
	SESSION_SHUTDOWN_MESSAGE,
	SESSION_CLOSE_MESSAGE
};

enum {
	SESSION_TRANSPORT_CLIENT = 1,
	SESSION_TRANSPORT_SERVER,
	SESSION_TRANSPORT_LISTEN
};

typedef ListNodeTemplateDeclare(int, type)	SessionInternalMessage_t;

typedef struct TransportCtx_t {
	/* public */
	void* io_object;
	struct sockaddr_storage peer_saddr;
	unsigned short mtu;
	unsigned short rto;
	unsigned char resend_maxtimes;
	unsigned char cwndsize;
	unsigned char reliable;
	struct {
		char err;
		char incomplete;
		int decodelen;
		int bodylen;
		unsigned char* bodyptr;
	} decode_result;
	void(*decode)(struct TransportCtx_t* self, unsigned char* buf, size_t buflen);
	void(*recv)(struct TransportCtx_t* self, const struct sockaddr_storage* addr);
	size_t(*hdrlen)(size_t bodylen);
	void(*encode)(unsigned char* hdrptr, size_t bodylen);
	void(*heartbeat)(struct TransportCtx_t* self);
	int(*zombie)(struct TransportCtx_t* self);
	int heartbeat_timeout_sec;
	int zombie_timeout_sec;
	/* private */
	unsigned char m_status;
	unsigned char m_synrcvd_times;
	unsigned char m_synsent_times;
	unsigned char m_reconnect_times;
	unsigned char m_fin_times;
	long long m_synrcvd_msec;
	long long m_synsent_msec;
	long long m_reconnect_msec;
	long long m_fin_msec;
	unsigned int m_cwndseq;
	unsigned int m_recvseq;
	unsigned int m_sendseq;
	unsigned int m_cwndseqbak;
	unsigned int m_recvseqbak;
	unsigned int m_sendseqbak;
	long long m_lastactive_msec;
	long long m_heartbeat_msec;
	unsigned char *m_inbuf;
	size_t m_inbufoff;
	size_t m_inbuflen;
	List_t m_recvpacketlist;
	List_t m_sendpacketlist;
	List_t m_sendpacketlistbak;
} TransportCtx_t;

typedef struct Session_t {
/* public */
	FD_t fd;
	int domain;
	int socktype;
	int protocol;
	int close_timeout_msec;
	int transport_side;
	unsigned int sessionid_len;
	const char* sessionid;
	void* userdata;
	union {
		struct sockaddr_storage local_listen_saddr;
		struct sockaddr_storage peer_listen_saddr;
	};
	union {
		void(*reg)(struct Session_t* self, int err);
		void(*connect)(struct Session_t* self, int err, int times, unsigned int self_recvseq, unsigned int self_cwndseq);
		const void* reg_or_connect;
	};
	void(*accept)(struct Session_t* self, FD_t newfd, const struct sockaddr_storage* peeraddr);
	void(*shutdown)(struct Session_t* self);
	void(*close)(struct Session_t* self);
	void(*free)(struct Session_t*);
	SessionInternalMessage_t shutdownmsg;
	SessionInternalMessage_t closemsg;
	TransportCtx_t ctx;
/* private */
	volatile char m_valid;
	Atom16_t m_shutdownflag;
	Atom16_t m_reconnectrecovery;
	int m_connect_times;
	SessionInternalMessage_t m_regmsg;
	SessionInternalMessage_t m_shutdownpostmsg;
	SessionInternalMessage_t m_netreconnectmsg;
	SessionInternalMessage_t m_reconnectrecoverymsg;
	SessionInternalMessage_t m_closemsg;
	HashtableNode_t m_hashnode;
	SessionLoop_t* m_loop;
	void* m_readol;
	void* m_writeol;
	int m_writeol_has_commit;
	unsigned short m_recvpacket_maxcnt;
} Session_t;

#ifdef __cplusplus
extern "C" {
#endif

__declspec_dll Session_t* sessionCreate(Session_t* s, FD_t fd, int domain, int socktype, int protocol, int transport_side);
__declspec_dll void sessionManualClose(Session_t* s);
__declspec_dll void sessionShutdown(Session_t* s);
__declspec_dll Session_t* sessionSend(Session_t* s, const void* data, unsigned int len, const void* to, int tolen);
__declspec_dll Session_t* sessionSendv(Session_t* s, const Iobuf_t iov[], unsigned int iovcnt, const void* to, int tolen);
__declspec_dll void sessionClientNetReconnect(Session_t* s);
__declspec_dll void sessionReconnectRecovery(Session_t* s);
__declspec_dll int sessionTransportGrab(Session_t* s, Session_t* target_s, unsigned int recvseq, unsigned int cwndseq);
__declspec_dll SessionLoop_t* sessionloopCreate(SessionLoop_t* loop);
__declspec_dll SessionLoop_t* sessionloopWake(SessionLoop_t* loop);
__declspec_dll int sessionloopHandler(SessionLoop_t* loop, NioEv_t e[], int n, long long timestamp_msec, int wait_msec);
__declspec_dll void sessionloopReg(SessionLoop_t* loop, Session_t* s[], size_t n);
__declspec_dll void sessionloopDestroy(SessionLoop_t* loop);

#ifdef __cplusplus
}
#endif

#endif
