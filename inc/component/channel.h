//
// Created by hujianzhe on 2019-7-11.
//

#ifndef	UTIL_C_COMPONENT_CHANNEL_H
#define	UTIL_C_COMPONENT_CHANNEL_H

#include "../datastruct/transport_ctx.h"
#include "../sysapi/socket.h"

enum {
	CHANNEL_FLAG_CLIENT		= 1 << 0,
	CHANNEL_FLAG_SERVER		= 1 << 1,
	CHANNEL_FLAG_LISTEN		= 1 << 2,
	CHANNEL_FLAG_STREAM		= 1 << 3,
	CHANNEL_FLAG_DGRAM		= 1 << 4,
	CHANNEL_FLAG_RELIABLE	= 1 << 5
};

typedef struct DgramHalfConn_t {
	NetPacketListNode_t node;
	unsigned char resend_times;
	long long resend_msec;
	FD_t sockfd;
	Sockaddr_t from_addr;
	unsigned short local_port;
} DgramHalfConn_t;

typedef struct Channel_t {
	NetPacketListNode_t node;
	/* public */
	void* io_object;
	int flag;
	int heartbeat_timeout_sec;
	unsigned int heartbeat_maxtimes;
	long long heartbeat_msec;
	unsigned char has_recvfin;
	unsigned char has_sendfin;
	long long event_msec;
	Sockaddr_t to_addr;
	union {
		struct {
			StreamTransportCtx_t ctx;
		} stream;
		struct {
			union {
				struct {
					Sockaddr_t listen_addr;
					void(*send_synack)(struct Channel_t* self, DgramHalfConn_t* halfconn); /* listener use */
					DgramHalfConn_t*(*recv_syn)(struct Channel_t* self, const void* from_saddr); /* listener use */
					int(*ack_halfconn)(struct Channel_t* self, DgramHalfConn_t* halfconn); /* listener use */
					void(*free_halfconn)(struct Channel_t* self, DgramHalfConn_t* halfconn); /* listener use */
				};
				struct {
					Sockaddr_t connect_addr;
					NetPacket_t* synpacket; /* client connect use */
					void(*send)(struct Channel_t* self, NetPacket_t* packet, const void* to_saddr);
				};
			};
			DgramTransportCtx_t ctx;
		} dgram;
	};
	struct {
		char err;
		char incomplete;
		char pktype;
		unsigned int pkseq;
		unsigned int decodelen;
		unsigned int bodylen;
		unsigned char* bodyptr;
	} decode_result;
	void(*decode)(struct Channel_t* self, unsigned char* buf, size_t buflen);
	void(*recv)(struct Channel_t* self, const void* from_saddr);
	void(*reply_ack)(struct Channel_t* self, unsigned int seq, const void* to_saddr);
	void(*heartbeat)(struct Channel_t* self);
	int(*zombie)(struct Channel_t* self);
	void(*shutdown)(struct Channel_t* self);
	/* private */
	unsigned int m_heartbeat_times;
} Channel_t;

#ifdef	__cplusplus
extern "C" {
#endif

__declspec_dll Channel_t* channelInit(Channel_t* channel, int flag, int initseq);
__declspec_dll int channelRecvHandler(Channel_t* channel, unsigned char* buf, int len, int off, long long timestamp_msec, const void* from_saddr);
__declspec_dll int channelEventHandler(Channel_t* channel, long long timestamp_msec);
__declspec_dll void channelDestroy(Channel_t* channel);

#ifdef	__cplusplus
}
#endif

#endif