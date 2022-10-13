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
	void(*on_recv)(struct ChannelBase_t* channel, unsigned char* bodyptr, size_t bodylen, const struct sockaddr* from_addr);
	void(*on_encode)(struct ChannelBase_t* channel, NetPacket_t* packet);
	void(*on_reply_ack)(struct ChannelBase_t* channel, unsigned int seq, const struct sockaddr* to_addr);
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
	} dgram;

	ChannelBase_t* channel;
	const ChannelRWDataProc_t* proc;
} ChannelRWData_t;

typedef struct ChannelRWHookProc_t {
	int(*on_read)(ChannelRWData_t* rw, unsigned char* buf, unsigned int len, long long timestamp_msec, const struct sockaddr* from_addr);
	int(*on_pre_send)(ChannelRWData_t* rw, NetPacket_t* packet, long long timestamp_msec);
	void(*on_exec)(ChannelRWData_t* rw, long long timestamp_msec);
	void(*on_free)(ChannelRWData_t* rw);
} ChannelRWHookProc_t;

#ifdef	__cplusplus
extern "C" {
#endif

__declspec_dll void channelrwInitData(ChannelRWData_t* rw, unsigned short flag, const ChannelRWDataProc_t* proc);
__declspec_dll const ChannelRWHookProc_t* channelrwGetHookProc(unsigned short flag);
__declspec_dll void channelbaseUseRWData(ChannelBase_t* channel, ChannelRWData_t* rw);

#ifdef	__cplusplus
}
#endif

#endif
