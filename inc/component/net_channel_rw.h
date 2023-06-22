//
// Created by hujianzhe on 2019-7-11.
//

#ifndef	UTIL_C_COMPONENT_NET_CHANNEL_RW_H
#define	UTIL_C_COMPONENT_NET_CHANNEL_RW_H

#include "reactor.h"

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
} ChannelInbufDecodeResult_t;

typedef struct ChannelRWDataProc_t {
	void(*on_decode)(struct ChannelBase_t* channel, unsigned char* buf, size_t len, ChannelInbufDecodeResult_t* result);
	void(*on_recv)(struct ChannelBase_t* channel, unsigned char* bodyptr, size_t bodylen, const struct sockaddr* from_addr, socklen_t addrlen);
	void(*on_encode)(struct ChannelBase_t* channel, NetPacket_t* packet);
	void(*on_reply_ack)(struct ChannelBase_t* channel, unsigned int seq, const struct sockaddr* to_addr, socklen_t addrlen);
} ChannelRWDataProc_t;

typedef struct ChannelRWData_t {
/* public */
	struct {
		union {
			/* listener use */
			struct {
				int halfconn_maxwaitcnt;
				int m_halfconn_curwaitcnt;
			};
			struct {
				int m_synpacket_status;
				NetPacket_t* m_synpacket;
			};
		};
		struct {
			unsigned short rto;
			unsigned char resend_maxtimes;
		};
	} dgram;

	const ChannelRWDataProc_t* proc;
} ChannelRWData_t;

typedef struct ChannelRWHookProc_t {
	int(*on_read)(ChannelBase_t* channel, unsigned char* buf, unsigned int len, long long timestamp_msec, const struct sockaddr* from_addr, socklen_t addrlen);
	int(*on_pre_send)(ChannelBase_t* channel, NetPacket_t* packet, long long timestamp_msec);
	void(*on_exec)(ChannelBase_t* channel, long long timestamp_msec);
	void(*on_free)(ChannelBase_t* channel);
} ChannelRWHookProc_t;

#ifdef	__cplusplus
extern "C" {
#endif

__declspec_dll void channelrwInitData(ChannelRWData_t* rw, int channel_flag, int socktype, const ChannelRWDataProc_t* proc);
__declspec_dll const ChannelRWHookProc_t* channelrwGetHookProc(int channel_flag, int socktype);
__declspec_dll void channelbaseUseRWData(ChannelBase_t* channel, ChannelRWData_t* rw);

#ifdef	__cplusplus
}
#endif

#endif
