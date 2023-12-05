//
// Created by hujianzhe on 2019-07-14.
//

#ifndef	UTIL_C_COMPONENT_REACTOR_H
#define	UTIL_C_COMPONENT_REACTOR_H

#include "../sysapi/nio.h"
#include "../sysapi/ipc.h"
#include "../sysapi/socket.h"
#include "../datastruct/list.h"
#include "../datastruct/transport_ctx.h"

enum {
	REACTOR_REG_ERR = 1,
	REACTOR_IO_READ_ERR = 2,
	REACTOR_IO_WRITE_ERR = 3,
	REACTOR_IO_CONNECT_ERR = 4,
	REACTOR_IO_ACCEPT_ERR = 5,
	REACTOR_ZOMBIE_ERR = 6,
	REACTOR_CACHE_READ_OVERFLOW_ERR = 7,
	REACTOR_CACHE_WRITE_OVERFLOW_ERR = 8
};
enum {
	CHANNEL_SIDE_CLIENT = 1,
	CHANNEL_SIDE_SERVER = 2,
	CHANNEL_SIDE_LISTEN = 3
};

struct Reactor_t;
struct ChannelBase_t;
struct ChannelBaseProc_t;
struct Session_t;

typedef struct ReactorCmd_t {
	ListNode_t _;
	int type;
} ReactorCmd_t;

typedef struct ReactorObject_t {
/* public */
	NioFD_t niofd;
	int inbuf_maxlen;
	char inbuf_saved;
/* private */
	struct ChannelBase_t* m_channel;
	struct {
		long long m_connect_end_msec;
		ListNode_t m_connect_endnode;
	} stream;
	char m_connected;
	unsigned char* m_inbuf;
	int m_inbufoff;
	int m_inbuflen;
	int m_inbufsize;
} ReactorObject_t;

typedef struct ChannelBase_t {
/* public */
	ReactorObject_t* o;
	struct Reactor_t* reactor;
	int domain;
	int socktype;
	int protocol;
	Sockaddr_t to_addr;
	socklen_t to_addrlen;
	unsigned short heartbeat_timeout_sec; /* optional */
	unsigned short heartbeat_maxtimes; /* client use, optional */
	char has_recvfin;
	char has_sendfin;
	char valid;
	unsigned char write_fragment_with_hdr;
	unsigned short side; /* read-only, TCP must set this field */
	short detach_error;
	long long event_msec;
	unsigned int write_fragment_size;
	unsigned int readcache_max_size;
	unsigned int sendcache_max_size;
	void* userdata; /* user use, library not use these field */
	const struct ChannelBaseProc_t* proc; /* user use, set your IO callback */
	struct Session_t* session; /* user use, set your logic session status */
	union {
		struct { /* listener use */
			void(*on_ack_halfconn)(struct ChannelBase_t* self, FD_t newfd, const struct sockaddr* peer_addr, socklen_t addrlen, long long ts_msec);
			Sockaddr_t listen_addr;
			socklen_t listen_addrlen;
		};
		struct { /* client use */
			void(*on_syn_ack)(struct ChannelBase_t* self, long long timestamp_msec); /* optional */
			Sockaddr_t connect_addr;
			socklen_t connect_addrlen;
			unsigned short connect_timeout_sec; /* optional */
		};
	};
/* private */
	void* m_ext_impl; /* internal or other ext */
	union {
		struct {
			StreamTransportCtx_t stream_ctx;
			ReactorCmd_t m_stream_fincmd;
			char m_stream_delay_send_fin;
		};
		DgramTransportCtx_t dgram_ctx;
	};
	long long m_heartbeat_msec;
	unsigned short m_heartbeat_times; /* client use */
	Atom32_t m_refcnt;
	char m_has_detached;
	char m_catch_fincmd;
	Atom8_t m_has_commit_fincmd;
	Atom8_t m_reghaspost;
	ReactorCmd_t m_regcmd;
	ReactorCmd_t m_freecmd;
} ChannelBase_t;

typedef struct ChannelBaseProc_t {
	void(*on_exec)(struct ChannelBase_t* self, long long timestamp_msec); /* optional */
	int(*on_read)(struct ChannelBase_t* self, unsigned char* buf, unsigned int len, long long timestamp_msec, const struct sockaddr* from_addr, socklen_t addrlen);
	unsigned int(*on_hdrsize)(struct ChannelBase_t* self, unsigned int bodylen); /* optional */
	int(*on_pre_send)(struct ChannelBase_t* self, NetPacket_t* packet, long long timestamp_msec); /* optional */
	void(*on_heartbeat)(struct ChannelBase_t* self, int heartbeat_times); /* client use, optional */
	void(*on_detach)(struct ChannelBase_t* self);
	void(*on_free)(struct ChannelBase_t* self); /* optional */
} ChannelBaseProc_t;

typedef struct ReactorPacket_t {
	ReactorCmd_t cmd;
	ChannelBase_t* channel;
	struct sockaddr* addr;
	socklen_t addrlen;
	NetPacket_t _;
} ReactorPacket_t;

typedef struct Session_t {
	ChannelBase_t* channel;
	char* ident;
	void* userdata;
	ChannelBase_t*(*do_connect_handshake)(struct Session_t*, int socktype, const char* ip, unsigned short port); /* optional */
	void(*on_disconnect)(struct Session_t*); /* optional */
} Session_t;

#ifdef	__cplusplus
extern "C" {
#endif

__declspec_dll struct Reactor_t* reactorCreate(void);
__declspec_dll void reactorWake(struct Reactor_t* reactor);
__declspec_dll int reactorHandle(struct Reactor_t* reactor, NioEv_t e[], int n, int wait_msec);
__declspec_dll void reactorDestroy(struct Reactor_t* reactor);

__declspec_dll ChannelBase_t* channelbaseOpen(unsigned short side, const ChannelBaseProc_t* proc, int domain, int socktype, int protocol);
__declspec_dll ChannelBase_t* channelbaseOpenWithFD(unsigned short side, const ChannelBaseProc_t* proc, FD_t fd, int domain, int protocol);
__declspec_dll ChannelBase_t* channelbaseSetOperatorSockaddr(ChannelBase_t* channel, const struct sockaddr* op_addr, socklen_t op_addrlen);
__declspec_dll ChannelBase_t* channelbaseAddRef(ChannelBase_t* channel);
__declspec_dll void channelbaseReg(struct Reactor_t* reactor, ChannelBase_t* channel);
__declspec_dll void channelbaseCloseRef(ChannelBase_t* channel);

__declspec_dll void channelbaseSendFin(ChannelBase_t* channel);
__declspec_dll void channelbaseSend(ChannelBase_t* channel, const void* data, size_t len, int pktype, const struct sockaddr* to_addr, socklen_t to_addrlen);
__declspec_dll void channelbaseSendv(ChannelBase_t* channel, const Iobuf_t iov[], unsigned int iovcnt, int pktype, const struct sockaddr* to_addr, socklen_t to_addrlen);

__declspec_dll Session_t* sessionInit(Session_t* session);
__declspec_dll void sessionReplaceChannel(Session_t* session, ChannelBase_t* channel);

#ifdef	__cplusplus
}
#endif

#endif
