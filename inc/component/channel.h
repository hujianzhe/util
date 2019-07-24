//
// Created by hujianzhe on 2019-7-11.
//

#ifndef	UTIL_C_COMPONENT_CHANNEL_H
#define	UTIL_C_COMPONENT_CHANNEL_H

#include "../datastruct/transport_ctx.h"
#include "../sysapi/socket.h"

enum {
	CHANNEL_FLAG_CLIENT = 1 << 0,
	CHANNEL_FLAG_SERVER = 1 << 1,
	CHANNEL_FLAG_LISTEN = 1 << 2,
	CHANNEL_FLAG_STREAM = 1 << 3,
	CHANNEL_FLAG_DGRAM = 1 << 4,
	CHANNEL_FLAG_RELIABLE = 1 << 5
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
	unsigned char has_recvfin;
	unsigned char has_sendfin;
	Sockaddr_t to_addr;
	struct {
		union {
			struct {
				Sockaddr_t listen_addr;
				void(*reply_synack)(FD_t listenfd, unsigned short newport, const void* from_addr); /* listener use */
				void(*ack_halfconn)(FD_t newfd, const void* peer_addr); /* listener use */
			};
			struct {
				Sockaddr_t connect_addr;
				NetPacket_t* synpacket; /* client connect use */
				NetPacket_t* finpacket;
				void(*resend_err)(struct Channel_t* self, NetPacket_t* packet);
				unsigned short rto;
				unsigned char resend_maxtimes;
			};
		};
		DgramTransportCtx_t ctx;
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
	void(*shutdown)(struct Channel_t* self);
	unsigned int(*hdrsize)(struct Channel_t* self, unsigned int bodylen);
	void(*encode)(struct Channel_t* self, unsigned char* hdr, unsigned int bodylen, unsigned char pktype, unsigned int pkseq);
/* private */
	unsigned int m_heartbeat_times;
} Channel_t;

#ifdef	__cplusplus
extern "C" {
#endif

__declspec_dll Channel_t* reactorobjectOpenChannel(struct ReactorObject_t* io, int flag, int initseq);
__declspec_dll int channelSharedData(Channel_t* channel, const Iobuf_t iov[], unsigned int iovcnt, int no_ack, List_t* packetlist);
__declspec_dll int channelSendPacket(Channel_t* channel, NetPacket_t* packet);
__declspec_dll int channelSendPacketList(Channel_t* channel, List_t* packetlist);
__declspec_dll void channelShutdown(Channel_t* channel, long long timestamp_msec);
__declspec_dll void channelDestroy(Channel_t* channel);

#ifdef	__cplusplus
}
#endif

#endif