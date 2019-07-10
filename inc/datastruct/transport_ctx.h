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
	unsigned int cwndseq;
	unsigned int recvseq;
	unsigned int sendseq;
	List_t sendpacketlist;
	List_t recvpacketlist;
	ListNode_t* m_recvnode;
	unsigned int m_ackseq;
} TransportCtx_t;

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
	char need_ack;
	char wait_ack;
	char resend_times;
	long long resend_msec;
	unsigned int seq;
	unsigned int off;
	unsigned int len;
	unsigned char data[1];
} NetPacket_t;

#ifdef	__cplusplus
extern "C" {
#endif

__declspec_dll int transportctxInsertRecvPacket(TransportCtx_t* ctx, NetPacket_t* packet);
__declspec_dll int transportctxMergeRecvPacket(TransportCtx_t* ctx, List_t* list);
__declspec_dll NetPacket_t* transportctxAckSendPacket(TransportCtx_t* ctx, unsigned int ackseq, int* cwndskip);
__declspec_dll void transportctxInsertSendPacket(TransportCtx_t* ctx, NetPacket_t* packet);
__declspec_dll int transportctxSendWindowHasPacket(TransportCtx_t* ctx, unsigned int seq);

#ifdef	__cplusplus
}
#endif

#endif