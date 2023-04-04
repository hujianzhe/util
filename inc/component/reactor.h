//
// Created by hujianzhe on 2019-07-14.
//

#ifndef	UTIL_C_COMPONENT_REACTOR_H
#define	UTIL_C_COMPONENT_REACTOR_H

#include "../sysapi/io.h"
#include "../sysapi/ipc.h"
#include "../sysapi/process.h"
#include "../sysapi/socket.h"
#include "../datastruct/list.h"
#include "../datastruct/hashtable.h"
#include "../datastruct/transport_ctx.h"

enum {
	REACTOR_REG_ERR = 1,
	REACTOR_IO_ERR,
	REACTOR_CONNECT_ERR,
	REACTOR_ZOMBIE_ERR,
	REACTOR_ONREAD_ERR
};
enum {
	CHANNEL_FLAG_CLIENT = 1 << 0,
	CHANNEL_FLAG_SERVER = 1 << 1,
	CHANNEL_FLAG_LISTEN = 1 << 2
};

typedef struct Reactor_t {
	/* private */
	long long m_event_msec;
	Thread_t m_runthread;
	Nio_t m_nio;
	CriticalSection_t m_cmdlistlock;
	List_t m_cmdlist;
	List_t m_invalidlist;
	List_t m_connect_endlist;
	Hashtable_t m_objht;
	HashtableNode_t* m_objht_bulks[2048];
} Reactor_t;

typedef struct ReactorCmd_t {
	struct ListNode_t _;
	int type;
} ReactorCmd_t;

struct ChannelBase_t;

typedef struct ReactorObject_t {
/* public */
	FD_t fd;
	int domain;
	int socktype;
	int protocol;
	int detach_error;
	int detach_timeout_msec;
	unsigned short dgram_read_fragment_size;
	struct {
		Sockaddr_t m_connect_addr;
		char m_listened;
		char inbuf_saved;
		short max_connect_timeout_sec;
		long long m_connect_end_msec;
		ListNode_t m_connect_endnode;
	} stream;
/* protected */
	int m_connected;
/* private */
	struct ChannelBase_t* m_channel;
	HashtableNode_t m_hashnode;
	ListNode_t m_invalidnode;
	char m_valid;
	char m_has_inserted;
	char m_has_detached;
	char m_readol_has_commit;
	char m_writeol_has_commit;
	void* m_readol;
	void* m_writeol;
	unsigned int m_io_event_mask;
	long long m_invalid_msec;
	unsigned char* m_inbuf;
	int m_inbufoff;
	int m_inbuflen;
	int m_inbufsize;
} ReactorObject_t;

struct ChannelBaseProc_t;
struct Session_t;

typedef struct ChannelBase_t {
	ReactorObject_t* o;
	Reactor_t* reactor;
	int socktype;
	Sockaddr_t to_addr;
	union {
		Sockaddr_t listen_addr;
		Sockaddr_t connect_addr;
	};
	union {
		StreamTransportCtx_t stream_ctx;
		DgramTransportCtx_t dgram_ctx;
	};
	struct {
		ReactorCmd_t m_stream_fincmd;
		char m_stream_delay_send_fin;
	};
	char has_recvfin;
	char has_sendfin;
	unsigned int heartbeat_maxtimes; /* client use, optional */
	int heartbeat_timeout_sec; /* optional */
	char valid;
	unsigned char write_fragment_with_hdr;
	unsigned short flag;
	int detach_error;
	long long event_msec;
	unsigned int write_fragment_size;
	unsigned int readcache_max_size;
	void* userdata; /* user use, library not use these field */
	const struct ChannelBaseProc_t* proc; /* user use, set your IO callback */
	struct Session_t* session; /* user use, set your logic session status */
	union {
		void(*on_ack_halfconn)(struct ChannelBase_t* self, FD_t newfd, const struct sockaddr* peer_addr, long long ts_msec); /* listener use */
		void(*on_syn_ack)(struct ChannelBase_t* self, long long timestamp_msec); /* client use, optional */
	};
/* private */
	long long m_heartbeat_msec;
	unsigned int m_heartbeat_times; /* client use */
	Atom32_t m_refcnt;
	char m_has_detached;
	char m_catch_fincmd;
	Atom8_t m_has_commit_fincmd;
	Atom8_t m_reghaspost;
	ReactorCmd_t m_regcmd;
	ReactorCmd_t m_freecmd;
} ChannelBase_t;

typedef struct ChannelBaseProc_t {
	void(*on_reg)(struct ChannelBase_t* self, long long timestamp_msec); /* optional */
	void(*on_exec)(struct ChannelBase_t* self, long long timestamp_msec); /* optional */
	int(*on_read)(struct ChannelBase_t* self, unsigned char* buf, unsigned int len, long long timestamp_msec, const struct sockaddr* from_addr);
	unsigned int(*on_hdrsize)(struct ChannelBase_t* self, unsigned int bodylen); /* optional */
	int(*on_pre_send)(struct ChannelBase_t* self, NetPacket_t* packet, long long timestamp_msec); /* optional */
	void(*on_heartbeat)(struct ChannelBase_t* self, int heartbeat_times); /* client use, optional */
	void(*on_detach)(struct ChannelBase_t* self);
	void(*on_free)(struct ChannelBase_t* self); /* optional */
} ChannelBaseProc_t;

typedef struct ReactorPacket_t {
	ReactorCmd_t cmd;
	ChannelBase_t* channel;
	NetPacket_t _;
} ReactorPacket_t;

typedef struct Session_t {
	ChannelBase_t* channel_client;
	ChannelBase_t* channel_server;
	char* ident;
	void* userdata;
	/* interface */
	ChannelBase_t*(*do_connect_handshake)(struct Session_t*, int socktype, const char* ip, unsigned short port); /* optional */
	void(*on_disconnect)(struct Session_t*); /* optional */
} Session_t;

#ifdef	__cplusplus
extern "C" {
#endif

__declspec_dll Reactor_t* reactorInit(Reactor_t* reactor);
__declspec_dll void reactorWake(Reactor_t* reactor);
__declspec_dll int reactorHandle(Reactor_t* reactor, NioEv_t e[], int n, int wait_msec);
__declspec_dll void reactorDestroy(Reactor_t* reactor);

__declspec_dll ReactorPacket_t* reactorpacketMake(int pktype, unsigned int hdrlen, unsigned int bodylen);
__declspec_dll void reactorpacketFree(ReactorPacket_t* pkg);

__declspec_dll ChannelBase_t* channelbaseOpen(unsigned short channel_flag, const ChannelBaseProc_t* proc, FD_t fd, int socktype, const struct sockaddr* addr);
__declspec_dll ChannelBase_t* channelbaseAddRef(ChannelBase_t* channel);
__declspec_dll void channelbaseReg(Reactor_t* reactor, ChannelBase_t* channel);
__declspec_dll void channelbaseClose(ChannelBase_t* channel);

__declspec_dll ChannelBase_t* channelbaseSend(ChannelBase_t* channel, const void* data, size_t len, int pktype);
__declspec_dll ChannelBase_t* channelbaseSendv(ChannelBase_t* channel, const Iobuf_t iov[], unsigned int iovcnt, int pktype);
__declspec_dll ChannelBase_t* channelbaseSendFin(ChannelBase_t* channel);

__declspec_dll Session_t* initSession(Session_t* session);
__declspec_dll void sessionReplaceChannel(Session_t* session, ChannelBase_t* channel);
__declspec_dll void sessionDisconnect(Session_t* session);
__declspec_dll void sessionUnbindChannel(Session_t* session);
__declspec_dll ChannelBase_t* sessionChannel(Session_t* session);

#ifdef	__cplusplus
}
#endif

#endif
