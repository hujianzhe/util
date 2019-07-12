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
	CHANNEL_FLAG_STREAM		= 1 << 2,
	CHANNEL_FLAG_DGRAM		= 1 << 3,
	CHANNEL_FLAG_RELIABLE	= 1 << 4
};

typedef struct Channel_t {
	/* public */
	void* io_object;
	int flag;
	int heartbeat_timeout_sec;
	int zombie_timeout_sec;
	unsigned char has_recvfin;
	unsigned char has_sendfin;
	long long event_msec;
	unsigned char* inbuf;
	size_t inbuflen;
	size_t inbufoff;
	Sockaddr_t connect_saddr;
	Sockaddr_t to_saddr;
	union {
		struct {
			StreamTransportCtx_t ctx;
		} stream;
		struct {
			DgramTransportCtx_t ctx;
			NetPacket_t* synpacket;
			void(*send)(struct Channel_t* self, NetPacket_t* packet, const void* to_saddr);
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
	long long m_lastactive_msec;
	long long m_heartbeat_msec;
} Channel_t;

#ifdef	__cplusplus
extern "C" {
#endif

__declspec_dll Channel_t* channelInit(Channel_t* channel, int flag, int initseq);
__declspec_dll int channelRecvHandler(Channel_t* channel, long long timestamp_msec, const void* from_saddr);
__declspec_dll int channelEventHandler(Channel_t* channel, long long timestamp_msec);
__declspec_dll void channelDestroy(Channel_t* channel);

#ifdef	__cplusplus
}
#endif

#endif