//
// Created by hujianzhe on 2019-7-11.
//

#ifndef	UTIL_C_COMPONENT_CHANNEL_H
#define	UTIL_C_COMPONENT_CHANNEL_H

#include "../component/reactor.h"

typedef struct ChannelInbufDecodeResult_t {
	char err;
	char incomplete;
	char ignore;
	char pktype;
	unsigned int pkseq;
	unsigned int decodelen;
	unsigned int bodylen;
	unsigned char* bodyptr;
	void* userdata;
} ChannelInbufDecodeResult_t;

typedef struct Channel_t {
/* public */
	ChannelBase_t _;
	void* userdata; /* user use, library not use these field */
	int heartbeat_timeout_sec;
	unsigned int heartbeat_maxtimes; /* client use */
	struct {
		union {
			/* listener use */
			struct {
				int halfconn_maxwaitcnt;
				int m_halfconn_curwaitcnt;
			};
			/* client use */
			struct {
				ReactorPacket_t* m_synpacket;
			};
		};
		struct {
			unsigned short rto;
			unsigned char resend_maxtimes;
		};
		void(*on_reply_ack)(struct Channel_t* self, unsigned int seq, const void* to_saddr);
	} dgram;
	/* interface */
	void(*on_decode)(struct Channel_t* self, unsigned char* buf, size_t buflen, ChannelInbufDecodeResult_t* result);
	void(*on_recv)(struct Channel_t* self, const void* from_saddr, ChannelInbufDecodeResult_t* result);
	int(*on_heartbeat)(struct Channel_t* self, int heartbeat_times); /* optional */
	unsigned int(*on_hdrsize)(struct Channel_t* self, unsigned int bodylen);
	void(*on_encode)(struct Channel_t* self, unsigned char* hdr, unsigned int bodylen, unsigned char pktype, unsigned int pkseq);
/* private */
	long long m_heartbeat_msec;
	unsigned int m_initseq;
	unsigned int m_heartbeat_times;
	Atom32_t m_has_sendfin;
} Channel_t;

#ifdef	__cplusplus
extern "C" {
#endif

__declspec_dll Channel_t* reactorobjectOpenChannel(ReactorObject_t* io, unsigned short flag, unsigned int extra_sz, const void* saddr);
__declspec_dll Channel_t* channelEnableHeartbeat(Channel_t* channel, long long now_msec);
__declspec_dll List_t* channelShard(Channel_t* channel, const Iobuf_t iov[], unsigned int iovcnt, int pktype, List_t* packetlist);
__declspec_dll Channel_t* channelSend(Channel_t* channel, const void* data, unsigned int len, int pktype);
__declspec_dll Channel_t* channelSendv(Channel_t* channel, const Iobuf_t iov[], unsigned int iovcnt, int pktype);

#ifdef	__cplusplus
}
#endif

#endif