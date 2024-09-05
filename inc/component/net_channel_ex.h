//
// Created by hujianzhe on 2019-7-11.
//

#ifndef	UTIL_C_COMPONENT_NET_CHANNEL_RW_H
#define	UTIL_C_COMPONENT_NET_CHANNEL_RW_H

#include "net_reactor.h"

typedef struct NetChannelInbufDecodeResult_t {
	char err;
	char incomplete;
	char fragment_eof;
	char pktype;
	char ignore;
	unsigned int pkseq;
	unsigned int decodelen;
	unsigned int bodylen;
	unsigned char* bodyptr;
} NetChannelInbufDecodeResult_t;

typedef struct NetChannelExProc_t {
	void(*on_decode)(struct NetChannel_t* channel, unsigned char* buf, size_t len, NetChannelInbufDecodeResult_t* result);
	void(*on_recv)(struct NetChannel_t* channel, unsigned char* bodyptr, size_t bodylen, const struct sockaddr* from_addr, socklen_t addrlen);
	void(*on_encode)(struct NetChannel_t* channel, NetPacket_t* packet);
	void(*on_reply_ack)(struct NetChannel_t* channel, unsigned int seq, const struct sockaddr* to_addr, socklen_t addrlen);
} NetChannelExProc_t;

typedef struct NetChannelExData_t {
/* public */
	struct {
		union {
			struct { /* listener use */
				int halfconn_maxwaitcnt;
				int m_halfconn_curwaitcnt;
			};
			struct { /* client or server use */
				int m_synpacket_status;
				NetPacket_t* m_synpacket;
			};
		};
		struct {
			unsigned short rto;
			unsigned char resend_maxtimes;
		};
	} dgram;

	const NetChannelExProc_t* proc;
} NetChannelExData_t;

typedef struct NetChannelExHookProc_t {
	int(*on_read)(NetChannel_t* channel, unsigned char* buf, unsigned int len, long long timestamp_msec, const struct sockaddr* from_addr, socklen_t addrlen);
	int(*on_pre_send)(NetChannel_t* channel, NetPacket_t* packet, long long timestamp_msec);
	void(*on_exec)(NetChannel_t* channel, long long timestamp_msec);
	void(*on_free)(NetChannel_t* channel);
} NetChannelExHookProc_t;

#ifdef	__cplusplus
extern "C" {
#endif

__declspec_dll const NetChannelExHookProc_t* NetChannelEx_get_hook(int channel_flag, int socktype);
__declspec_dll void NetChannelEx_init(NetChannel_t* channel, NetChannelExData_t* rw, const NetChannelExProc_t* rw_proc);

#ifdef	__cplusplus
}
#endif

#endif
