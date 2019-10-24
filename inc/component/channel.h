//
// Created by hujianzhe on 2019-7-11.
//

#ifndef	UTIL_C_COMPONENT_CHANNEL_H
#define	UTIL_C_COMPONENT_CHANNEL_H

#include "../component/reactor.h"

enum {
	CHANNEL_FLAG_CLIENT = 1 << 0,
	CHANNEL_FLAG_SERVER = 1 << 1,
	CHANNEL_FLAG_LISTEN = 1 << 2,
	CHANNEL_FLAG_STREAM = 1 << 3,
	CHANNEL_FLAG_DGRAM = 1 << 4,
	CHANNEL_FLAG_RELIABLE = 1 << 5
};

typedef struct ChannelInbufDecodeResult_t {
	char err;
	char incomplete;
	char pktype;
	unsigned int pkseq;
	unsigned int decodelen;
	unsigned int bodylen;
	unsigned char* bodyptr;
} ChannelInbufDecodeResult_t;

typedef struct Channel_t {
/* public */
	ReactorCmd_t node;
	ReactorObject_t* o;
	void* userdata;
	int flag;
	unsigned int maxhdrsize;
	int heartbeat_timeout_sec;
	unsigned int heartbeat_maxtimes; /* client use */
	unsigned int connected_times; /* client use */
	Sockaddr_t to_addr;
	int detach_error;
	ReactorCmd_t freecmd;
	struct {
		union {
			/* listener use */
			struct {
				Sockaddr_t listen_addr;
				int halfconn_maxwaitcnt;
				int m_halfconn_curwaitcnt;
			};
			/* client use */
			struct {
				Sockaddr_t connect_addr;
				ReactorPacket_t* m_synpacket;
			};
		};
		struct {
			unsigned short rto;
			unsigned char resend_maxtimes;
			char has_recvfin;
			char has_sendfin;
			DgramTransportCtx_t ctx;
		};
	} dgram;
	/* interface */
	void(*ack_halfconn)(struct Channel_t* self, FD_t newfd, const void* peer_addr, long long ts_msec); /* listener use */
	void(*syn_ack)(struct Channel_t* self, long long ts_msec); /* listener use */
	void(*reg)(struct Channel_t* self, long long timestamp_msec);
	void(*decode)(struct Channel_t* self, unsigned char* buf, size_t buflen, ChannelInbufDecodeResult_t* result);
	void(*recv)(struct Channel_t* self, const void* from_saddr, ChannelInbufDecodeResult_t* result);
	void(*reply_ack)(struct Channel_t* self, unsigned int seq, const void* to_saddr);
	int(*heartbeat)(struct Channel_t* self, int heartbeat_times); /* optional */
	unsigned int(*hdrsize)(struct Channel_t* self, unsigned int bodylen);
	void(*encode)(struct Channel_t* self, unsigned char* hdr, unsigned int bodylen, unsigned char pktype, unsigned int pkseq);
	void(*detach)(struct Channel_t* self);
/* private */
	long long m_event_msec;
	long long m_heartbeat_msec;
	unsigned int m_initseq;
	unsigned int m_heartbeat_times;
	char detached;
	Atom8_t m_ban_send;
} Channel_t;

#ifdef	__cplusplus
extern "C" {
#endif

__declspec_dll Channel_t* reactorobjectOpenChannel(ReactorObject_t* io, int flag, unsigned int initseq, const void* saddr);
__declspec_dll void channelSendFin(Channel_t* channel, long long timestamp_msec);
__declspec_dll Channel_t* channelSend(Channel_t* channel, const void* data, unsigned int len, int no_ack);
__declspec_dll Channel_t* channelSendv(Channel_t* channel, const Iobuf_t iov[], unsigned int iovcnt, int no_ack);
__declspec_dll void channelDestroy(Channel_t* channel);

#ifdef	__cplusplus
}
#endif

#endif