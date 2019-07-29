//
// Created by hujianzhe on 2019-7-11.
//

#ifndef	UTIL_C_COMPONENT_CHANNEL_H
#define	UTIL_C_COMPONENT_CHANNEL_H

#include "../datastruct/transport_ctx.h"
#include "../sysapi/atomic.h"
#include "../sysapi/socket.h"

enum {
	CHANNEL_FLAG_CLIENT = 1 << 0,
	CHANNEL_FLAG_SERVER = 1 << 1,
	CHANNEL_FLAG_LISTEN = 1 << 2,
	CHANNEL_FLAG_STREAM = 1 << 3,
	CHANNEL_FLAG_DGRAM = 1 << 4,
	CHANNEL_FLAG_RELIABLE = 1 << 5
};

enum {
	CHANNEL_INACTIVE_NORMAL = 1,
	CHANNEL_INACTIVE_UNSENT,
	CHANNEL_INACTIVE_ZOMBIE,
	CHANNEL_INACTIVE_FATE
};

struct ReactorObject_t;
typedef struct Channel_t {
/* public */
	NetPacketListNode_t node;
	struct ReactorObject_t* io;
	int flag;
	unsigned int maxhdrsize;
	int heartbeat_timeout_sec;
	unsigned int heartbeat_maxtimes;
	long long heartbeat_msec;
	Sockaddr_t to_addr;
	NetPacket_t* finpacket;
	int inactive_reason;
	union {
		void(*ack_halfconn)(struct Channel_t* self, FD_t newfd, const void* peer_addr, long long ts_msec); /* listener use */
		void(*synack)(struct Channel_t* self, int err, long long ts_msec); /* client connect use */
	};
	struct {
		union {
			/* listener use */
			struct {
				Sockaddr_t listen_addr;
			};
			/* client connect use */
			struct {
				Sockaddr_t connect_addr;
				NetPacket_t* synpacket;
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
	struct {
		char err;
		char incomplete;
		char pktype;
		unsigned int pkseq;
		unsigned int decodelen;
		unsigned int bodylen;
		unsigned char* bodyptr;
	} decode_result;
	/* interface */
	void(*decode)(struct Channel_t* self, unsigned char* buf, size_t buflen);
	void(*recv)(struct Channel_t* self, const void* from_saddr);
	void(*reply_ack)(struct Channel_t* self, unsigned int seq, const void* to_saddr);
	void(*heartbeat)(struct Channel_t* self);
	int(*zombie)(struct Channel_t* self);
	unsigned int(*hdrsize)(struct Channel_t* self, unsigned int bodylen);
	void(*encode)(struct Channel_t* self, unsigned char* hdr, unsigned int bodylen, unsigned char pktype, unsigned int pkseq);
	void(*inactive)(struct Channel_t* self, int reason);
/* private */
	unsigned int m_heartbeat_times;
	Atom8_t m_ban_send;
} Channel_t;

#ifdef	__cplusplus
extern "C" {
#endif

__declspec_dll Channel_t* reactorobjectOpenChannel(struct ReactorObject_t* io, int flag, int initseq, const void* saddr);
__declspec_dll Channel_t* channelSend(Channel_t* channel, const void* data, unsigned int len, int no_ack);
__declspec_dll Channel_t* channelSendv(Channel_t* channel, const Iobuf_t iov[], unsigned int iovcnt, int no_ack);
__declspec_dll void channelSendFin(Channel_t* channel, long long timestamp_msec);
__declspec_dll void channelDestroy(Channel_t* channel);

#ifdef	__cplusplus
}
#endif

#endif