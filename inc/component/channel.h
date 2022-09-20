//
// Created by hujianzhe on 2019-7-11.
//

#ifndef	UTIL_C_COMPONENT_CHANNEL_H
#define	UTIL_C_COMPONENT_CHANNEL_H

#include "../component/reactor.h"
#include "../sysapi/atomic.h"

typedef struct ChannelInbufDecodeResult_t {
	char err;
	char incomplete;
	char fragment_eof;
	char pktype;
	char ignore;
	unsigned int pkseq;
	unsigned int decodelen;
	unsigned int bodylen;
	unsigned char* bodyptr;
	void* userdata;
} ChannelInbufDecodeResult_t;

typedef struct Channel_t {
/* public */
	ChannelBase_t _;
	struct {
		union {
			/* listener use */
			struct {
				int halfconn_maxwaitcnt;
				int m_halfconn_curwaitcnt;
			};
			/* client use */
			struct {
				int m_synpacket_doing;
				NetPacket_t* m_synpacket;
			};
		};
		struct {
			unsigned short rto;
			unsigned char resend_maxtimes;
		};
		void(*on_reply_ack)(ChannelBase_t* self, unsigned int seq, const struct sockaddr* to_saddr);
	} dgram;
	/* interface */
	void(*on_decode)(ChannelBase_t* self, unsigned char* buf, size_t buflen, ChannelInbufDecodeResult_t* result);
	void(*on_recv)(ChannelBase_t* self, const struct sockaddr* from_saddr, ChannelInbufDecodeResult_t* result);
	void(*on_encode)(ChannelBase_t* self, NetPacket_t* packet);
} Channel_t;

#ifdef	__cplusplus
extern "C" {
#endif

__declspec_dll Channel_t* reactorobjectOpenChannel(size_t sz, unsigned short flag, ReactorObject_t* io, const struct sockaddr* addr);

#ifdef	__cplusplus
}
#endif

#endif
