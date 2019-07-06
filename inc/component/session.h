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
	NioReactor_t m_reactor;
	FD_t m_socketpair[2];
	void* m_readol;
	CriticalSection_t m_msglistlock;
	List_t m_msglist;
	List_t m_sockcloselist;
	Hashtable_t m_sockht;
	HashtableNode_t* m_sockht_bulks[2048];
	long long m_event_msec;
} SessionLoop_t;

enum {
	SESSION_USER_MESSAGE,
	SESSION_SHUTDOWN_MESSAGE,
	SESSION_CLOSE_MESSAGE,
};
typedef struct SessionInternalMsg_t {
	ListNode_t m_listnode;
	int type;
} SessionInternalMsg_t;

enum {
	SESSION_TRANSPORT_NOSIDE,
	SESSION_TRANSPORT_CLIENT,
	SESSION_TRANSPORT_SERVER,
	SESSION_TRANSPORT_LISTEN
};

typedef struct SessionDecodeResult_t {
	char err;
	char incomplete;
	int decodelen;
	int bodylen;
	unsigned char* bodyptr;
} SessionDecodeResult_t;

typedef struct Session_t {
/* public */
	FD_t fd;
	int domain;
	int socktype;
	int protocol;
	int heartbeat_timeout_sec;
	int keepalive_timeout_sec;
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
	int(*zombie)(struct Session_t* self);
	void(*accept)(struct Session_t* self, FD_t newfd, const struct sockaddr_storage* peeraddr);
	void(*decode)(unsigned char* buf, size_t buflen, SessionDecodeResult_t* result);
	void(*recv)(struct Session_t* self, const struct sockaddr_storage* addr, SessionDecodeResult_t* result);
	size_t(*hdrlen)(size_t bodylen);
	void(*encode)(unsigned char* hdrptr, size_t bodylen);
	void(*send_heartbeat_to_server)(struct Session_t* self);
	void(*shutdown)(struct Session_t* self);
	void(*close)(struct Session_t* self);
	void(*free)(struct Session_t*);
	SessionInternalMsg_t shutdownmsg;
	SessionInternalMsg_t closemsg;
/* private */
	volatile char m_valid;
	Atom16_t m_shutdownflag;
	Atom16_t m_reconnectrecovery;
	int m_connect_times;
	SessionInternalMsg_t m_regmsg;
	SessionInternalMsg_t m_shutdownpostmsg;
	SessionInternalMsg_t m_netreconnectmsg;
	SessionInternalMsg_t m_reconnectrecoverymsg;
	SessionInternalMsg_t m_closemsg;
	HashtableNode_t m_hashnode;
	SessionLoop_t* m_loop;
	void* m_readol;
	void* m_writeol;
	int m_writeol_has_commit;
	long long m_lastactive_msec;
	long long m_heartbeat_msec;
	unsigned char *m_inbuf;
	size_t m_inbufoffset;
	size_t m_inbuflen;
	unsigned short m_recvpacket_maxcnt;
	List_t m_recvpacketlist;
	List_t m_sendpacketlist;
	List_t m_sendpacketlist_bak;
	struct {
	/* public */
		struct sockaddr_storage peer_saddr;
		unsigned short mtu;
		unsigned short rto;
		unsigned char resend_maxtimes;
		unsigned char cwndsize;
		unsigned char enable;
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
		unsigned int m_cwndseq_bak;
		unsigned int m_recvseq_bak;
		unsigned int m_sendseq_bak;
	} reliable;
} Session_t;

#ifdef __cplusplus
extern "C" {
#endif

__declspec_dll Session_t* sessionCreate(Session_t* s, FD_t fd, int domain, int socktype, int protocol, int transport_side);
__declspec_dll void sessionManualClose(Session_t* s);
__declspec_dll void sessionShutdown(Session_t* s);
__declspec_dll Session_t* sessionSend(Session_t* s, const void* data, unsigned int len, const struct sockaddr* to, int tolen);
__declspec_dll Session_t* sessionSendv(Session_t* s, const Iobuf_t iov[], unsigned int iovcnt, const struct sockaddr* to, int tolen);
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
