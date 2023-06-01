//
// Created by hujianzhe on 2019-7-10.
//

#ifndef	UTIL_C_DATASTRUCT_TRANSPORT_CTX_H
#define	UTIL_C_DATASTRUCT_TRANSPORT_CTX_H

#include "list.h"

/* reliable UDP use those packet type */
enum {
	NETPACKET_SYN = 1,				/* reliable UDP client connect use */
	NETPACKET_SYN_ACK,				/* reliable UDP listener use */
	NETPACKET_ACK,
	NETPACKET_NO_ACK_FRAGMENT,
	NETPACKET_FIN,
	NETPACKET_FRAGMENT
};
enum {
	NETPACKET_DGRAM_HAS_SEND_SEQ = NETPACKET_FIN
};

typedef struct NetPacket_t {
	ListNode_t node;
	char type;
	char wait_ack;
	char cached;
	char fragment_eof;
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
	unsigned int hdrlen;
	unsigned int bodylen;
	unsigned char buf[1];
} NetPacket_t;

typedef struct DgramTransportCtx_t {
	List_t recvlist;
	List_t sendlist;
	unsigned int cache_recv_bytes;
	unsigned int cache_send_bytes;
	unsigned char send_all_acked;
	unsigned char cwndsize;
	/* private */
	unsigned int m_sendseq;
	unsigned int m_recvseq;
	unsigned int m_cwndseq;
	unsigned int m_ackseq;
	ListNode_t* m_recvnode;
} DgramTransportCtx_t;

typedef struct StreamTransportCtx_t {
	List_t recvlist;
	List_t sendlist;
	unsigned int cache_recv_bytes;
	unsigned int cache_send_bytes;
} StreamTransportCtx_t;

#ifdef	__cplusplus
extern "C" {
#endif

__declspec_dll DgramTransportCtx_t* dgramtransportctxInit(DgramTransportCtx_t* ctx, unsigned int initseq);
__declspec_dll int dgramtransportctxRecvCheck(DgramTransportCtx_t* ctx, unsigned int seq, int pktype);
__declspec_dll void dgramtransportctxCacheRecvPacket(DgramTransportCtx_t* ctx, NetPacket_t* packet);
__declspec_dll int dgramtransportctxMergeRecvPacket(DgramTransportCtx_t* ctx, List_t* list);
__declspec_dll unsigned int dgramtransportctxNextSendSeq(DgramTransportCtx_t* ctx, int pktype);
__declspec_dll int dgramtransportctxCacheSendPacket(DgramTransportCtx_t* ctx, NetPacket_t* packet);
__declspec_dll int dgramtransportctxAckSendPacket(DgramTransportCtx_t* ctx, unsigned int ackseq, NetPacket_t** ackpacket);
__declspec_dll int dgramtransportctxSendWindowHasPacket(DgramTransportCtx_t* ctx, NetPacket_t* packet);

__declspec_dll StreamTransportCtx_t* streamtransportctxInit(StreamTransportCtx_t* ctx);
__declspec_dll void streamtransportctxCacheRecvPacket(StreamTransportCtx_t* ctx, NetPacket_t* packet);
__declspec_dll int streamtransportctxMergeRecvPacket(StreamTransportCtx_t* ctx, List_t* list);
__declspec_dll int streamtransportctxSendCheckBusy(StreamTransportCtx_t* ctx);
__declspec_dll int streamtransportctxCacheSendPacket(StreamTransportCtx_t* ctx, NetPacket_t* packet);
__declspec_dll void streamtransportctxRemoveCacheSendPacket(StreamTransportCtx_t* ctx, NetPacket_t* packet);

#ifdef	__cplusplus
}
#endif

#endif
