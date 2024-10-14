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
	NET_REACTOR_REG_ERR = 1,
	NET_REACTOR_IO_READ_ERR = 2,
	NET_REACTOR_IO_WRITE_ERR = 3,
	NET_REACTOR_IO_CONNECT_ERR = 4,
	NET_REACTOR_IO_ACCEPT_ERR = 5,
	NET_REACTOR_ZOMBIE_ERR = 6,
	NET_REACTOR_CACHE_READ_OVERFLOW_ERR = 7,
	NET_REACTOR_CACHE_WRITE_OVERFLOW_ERR = 8
};
enum {
	NET_CHANNEL_SIDE_CLIENT = 1,
	NET_CHANNEL_SIDE_SERVER = 2,
	NET_CHANNEL_SIDE_LISTEN = 3
};

struct NetReactor_t;
struct NetChannel_t;

typedef struct NetChannelSession_t {
	int type;
	struct NetChannel_t* channel;
} NetChannelSession_t;

typedef struct NetChannelProc_t {
	void(*on_exec)(struct NetChannel_t* self, long long timestamp_msec); /* optional */
	int(*on_read)(struct NetChannel_t* self, unsigned char* buf, unsigned int len, long long timestamp_msec, const struct sockaddr* from_addr, socklen_t addrlen);
	unsigned int(*on_hdrsize)(struct NetChannel_t* self, unsigned int bodylen); /* optional */
	int(*on_pre_send)(struct NetChannel_t* self, NetPacket_t* packet, long long timestamp_msec); /* optional */
	void(*on_heartbeat)(struct NetChannel_t* self, int heartbeat_times); /* client use, optional */
	void(*on_detach)(struct NetChannel_t* self);
	void(*on_free)(struct NetChannel_t* self); /* optional */
} NetChannelProc_t;

typedef struct NetReactorCmd_t {
	ListNode_t _;
	int type;
} NetReactorCmd_t;

typedef struct NetReactorPacket_t {
	NetReactorCmd_t cmd;
	struct NetChannel_t* channel;
	struct sockaddr* addr;
	socklen_t addrlen;
	NetPacket_t _;
} NetReactorPacket_t;

typedef struct NetReactorObject_t {
/* public */
	NioFD_t niofd;
	int inbuf_maxlen;
	char inbuf_saved;
/* private */
	struct NetChannel_t* m_channel;
	struct {
		long long m_connect_end_msec;
		ListNode_t m_connect_endnode;
	} stream;
	char m_connected;
	unsigned char* m_inbuf;
	int m_inbufoff;
	int m_inbuflen;
	int m_inbufsize;
} NetReactorObject_t;

typedef struct NetChannel_t {
/* public */
	NetReactorObject_t* o;
	struct NetReactor_t* reactor;
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
	const struct NetChannelProc_t* proc; /* user use, set your IO callback */
	NetChannelSession_t* session; /* user use, set your logic session status */
	union {
		struct { /* listener use */
			void(*on_ack_halfconn)(struct NetChannel_t* self, FD_t newfd, const struct sockaddr* peer_addr, socklen_t addrlen, long long ts_msec);
			Sockaddr_t listen_addr;
			socklen_t listen_addrlen;
		};
		struct { /* client use */
			void(*on_syn_ack)(struct NetChannel_t* self, long long timestamp_msec); /* optional */
			Sockaddr_t connect_addr;
			socklen_t connect_addrlen;
			int connect_timeout_msec; /* optional */
		};
	};
/* private */
	void* m_ext_impl; /* internal or other ext */
	union {
		struct {
			StreamTransportCtx_t stream_ctx;
			int stream_writeev_wnd_bytes;
			NetReactorCmd_t m_stream_fincmd;
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
	NetReactorCmd_t m_regcmd;
	NetReactorCmd_t m_freecmd;
} NetChannel_t;

#ifdef	__cplusplus
extern "C" {
#endif

__declspec_dll struct NetReactor_t* NetReactor_create(void);
__declspec_dll void NetReactor_wake(struct NetReactor_t* reactor);
__declspec_dll int NetReactor_handle(struct NetReactor_t* reactor, NioEv_t e[], int n, int wait_msec);
__declspec_dll void NetReactor_destroy(struct NetReactor_t* reactor);

__declspec_dll NetChannel_t* NetChannel_open(unsigned short side, const NetChannelProc_t* proc, int domain, int socktype, int protocol);
__declspec_dll NetChannel_t* NetChannel_open_with_fd(unsigned short side, const NetChannelProc_t* proc, FD_t fd, int domain, int protocol);
__declspec_dll NetChannel_t* NetChannel_set_operator_sockaddr(NetChannel_t* channel, const struct sockaddr* op_addr, socklen_t op_addrlen);
__declspec_dll NetChannel_t* NetChannel_add_ref(NetChannel_t* channel);
__declspec_dll void NetChannel_reg(struct NetReactor_t* reactor, NetChannel_t* channel);
__declspec_dll void NetChannel_close_ref(NetChannel_t* channel);

__declspec_dll void NetChannel_send_fin(NetChannel_t* channel);
__declspec_dll void NetChannel_send(NetChannel_t* channel, const void* data, size_t len, int pktype, const struct sockaddr* to_addr, socklen_t to_addrlen);
__declspec_dll void NetChannel_sendv(NetChannel_t* channel, const Iobuf_t iov[], unsigned int iovcnt, int pktype, const struct sockaddr* to_addr, socklen_t to_addrlen);

#ifdef	__cplusplus
}
#endif

#endif
