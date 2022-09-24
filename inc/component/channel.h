//
// Created by hujianzhe on 2019-7-11.
//

#ifndef	UTIL_C_COMPONENT_CHANNEL_H
#define	UTIL_C_COMPONENT_CHANNEL_H

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
	void* userdata;
} ChannelInbufDecodeResult_t;

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
	ChannelBaseProc_t base_proc;
	void(*on_decode)(struct ChannelBase_t* channel, unsigned char* buf, size_t buflen, ChannelInbufDecodeResult_t* result);
	void(*on_recv)(struct ChannelBase_t* channel, const struct sockaddr* from_saddr, const ChannelInbufDecodeResult_t* result);
	void(*on_encode)(struct ChannelBase_t* channel, NetPacket_t* packet);
	void(*on_reply_ack)(struct ChannelBase_t* channel, unsigned int seq, const struct sockaddr* to_saddr);
} ChannelRWData_t;

#ifdef	__cplusplus
extern "C" {
#endif

__declspec_dll void channelrwdataInit(ChannelRWData_t* rw, unsigned short flag);
__declspec_dll void channelbaseUseRWData(ChannelBase_t* channel, const ChannelRWData_t* rw);

#ifdef	__cplusplus
}
#endif

#endif
