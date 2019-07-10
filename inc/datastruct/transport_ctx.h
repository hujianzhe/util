//
// Created by hujianzhe on 2019-7-10.
//

#ifndef	UTIL_C_DATASTRUCT_TRANSPORT_CTX_H
#define	UTIL_C_DATASTRUCT_TRANSPORT_CTX_H

#include "list.h"

typedef struct TransportCtx_t {
	unsigned short mtu;
	unsigned short rto;
	unsigned char resend_maxtimes;
	unsigned char cwndsize;
	List_t sendpacketlist;
	List_t recvpacketlist;
	void* userdata;
	/* private */
	ListNode_t* m_recvnode;
	unsigned int m_cwndseq;
	unsigned int m_recvseq;
	unsigned int m_sendseq;
	unsigned int m_ackseq;
} TransportCtx_t;

typedef struct StreamTransportCtx_t {
	List_t recvpacketlist;
	List_t sendpacketlist;
	void* userdata;
	/* private */
	unsigned int m_recvseq;
	unsigned int m_sendseq;
	unsigned int m_cwndseq;
} StreamTransportCtx_t;

enum {
	NETPACKET_ACK = 1,
	NETPACKET_FRAGMENT,
	NETPACKET_FRAGMENT_EOF,
	NETPACKET_END
};

typedef ListNodeTemplateDeclare(int, type)	NetPacketListNode_t;
typedef struct NetPacket_t {
	NetPacketListNode_t node;
	void* userdata;
	char type;
	char wait_ack;
	union {
		/* dgram */
		struct {
			char resend_times;
			long long resend_msec;
		};
		/* stream */
		struct {
			unsigned int off;
		};
	};
	unsigned int seq;
	unsigned int len;
	unsigned char data[1];
} NetPacket_t;

#ifdef	__cplusplus
extern "C" {
#endif

__declspec_dll TransportCtx_t* transportctxInit(TransportCtx_t* ctx, unsigned int initseq);
__declspec_dll int transportctxRecvCheck(TransportCtx_t* ctx, unsigned int seq);
__declspec_dll int transportctxCacheRecvPacket(TransportCtx_t* ctx, NetPacket_t* packet);
__declspec_dll int transportctxMergeRecvPacket(TransportCtx_t* ctx, List_t* list);
__declspec_dll NetPacket_t* transportctxAckSendPacket(TransportCtx_t* ctx, unsigned int ackseq, int* cwndskip);
__declspec_dll void transportctxCacheSendPacket(TransportCtx_t* ctx, NetPacket_t* packet);
__declspec_dll int transportctxSendWindowHasPacket(TransportCtx_t* ctx, unsigned int seq);

__declspec_dll StreamTransportCtx_t* streamtransportctxInit(StreamTransportCtx_t* ctx, unsigned int initseq);
__declspec_dll int streamtransportctxRecvCheck(StreamTransportCtx_t* ctx, unsigned int seq);
__declspec_dll int streamtransportctxCacheRecvPacket(StreamTransportCtx_t* ctx, NetPacket_t* packet);
__declspec_dll int streamtransportctxMergeRecvPacket(StreamTransportCtx_t* ctx, List_t* list);
__declspec_dll void streamtransportctxCacheSendPacket(StreamTransportCtx_t* ctx, NetPacket_t* packet);
__declspec_dll int streamtransportctxAckSendPacket(StreamTransportCtx_t* ctx, unsigned int ackseq, NetPacket_t** ackpacket);

#ifdef	__cplusplus
}
#endif

#endif