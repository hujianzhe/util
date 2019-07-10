//
// Created by hujianzhe on 2019-7-10.
//

#include "../../inc/datastruct/transport_ctx.h"

#define seq1_before_seq2(seq1, seq2)	((int)(seq1 - seq2) < 0)

#ifdef	__cplusplus
extern "C" {
#endif

/*********************************************************************************/

TransportCtx_t* transportctxInit(TransportCtx_t* ctx) {
	ctx->m_cwndseq = ctx->m_recvseq = ctx->m_sendseq = ctx->m_ackseq = 0;
	ctx->m_recvnode = (struct ListNode_t*)0;
	listInit(&ctx->recvpacketlist);
	listInit(&ctx->sendpacketlist);
	return ctx;
}

int transportctxRecvCheck(TransportCtx_t* ctx, unsigned int seq) {
	if (seq1_before_seq2(seq, ctx->m_recvseq))
		return 0;
	else {
		ListNode_t* cur = ctx->m_recvnode ? ctx->m_recvnode : ctx->recvpacketlist.head;
		for (; cur; cur = cur->next) {
			NetPacket_t* pk = pod_container_of(cur, NetPacket_t, node);
			if (seq1_before_seq2(seq, pk->seq))
				break;
			else if (seq == pk->seq)
				return 0;
		}
		return 1;
	}
}

int transportctxCacheRecvPacket(TransportCtx_t* ctx, NetPacket_t* packet) {
	ListNode_t* cur = ctx->m_recvnode ? ctx->m_recvnode : ctx->recvpacketlist.head;
	for (; cur; cur = cur->next) {
		NetPacket_t* pk = pod_container_of(cur, NetPacket_t, node);
		if (seq1_before_seq2(packet->seq, pk->seq))
			break;
		else if (packet->seq == pk->seq)
			return 0;
	}
	if (cur)
		listInsertNodeFront(&ctx->recvpacketlist, cur, &packet->node._);
	else
		listInsertNodeBack(&ctx->recvpacketlist, ctx->recvpacketlist.tail, &packet->node._);

	cur = &packet->node._;
	while (cur) {
		packet = pod_container_of(cur, NetPacket_t, node);
		if (ctx->m_recvseq != packet->seq)
			break;
		ctx->m_recvseq++;
		ctx->m_recvnode = &packet->node._;
		cur = cur->next;
	}
	return 1;
}

int transportctxMergeRecvPacket(TransportCtx_t* ctx, List_t* list) {
	ListNode_t* cur;
	if (ctx->m_recvnode) {
		for (cur = ctx->recvpacketlist.head; cur && cur != ctx->m_recvnode->next; cur = cur->next) {
			NetPacket_t* packet = pod_container_of(cur, NetPacket_t, node);
			if (NETPACKET_FRAGMENT_EOF != packet->type && NETPACKET_END != packet->type)
				continue;
			*list = listSplitByTail(&ctx->recvpacketlist, cur);
			if (!ctx->recvpacketlist.head || ctx->m_recvnode == cur)
				ctx->m_recvnode = (struct ListNode_t*)0;
			return 1;
		}
	}
	return 0;
}

NetPacket_t* transportctxAckSendPacket(TransportCtx_t* ctx, unsigned int ackseq, int* cwndskip) {
	ListNode_t* cur, *next;
	*cwndskip = 0;
	if (seq1_before_seq2(ackseq, ctx->m_cwndseq))
		return (NetPacket_t*)0;
	for (cur = ctx->sendpacketlist.head; cur; cur = next) {
		NetPacket_t* packet = pod_container_of(cur, NetPacket_t, node);
		next = cur->next;
		if (packet->seq != ackseq)
			continue;
		if (!packet->wait_ack)
			break;
		if (seq1_before_seq2(ctx->m_ackseq, ackseq))
			ctx->m_ackseq = ackseq;
		listRemoveNode(&ctx->sendpacketlist, cur);
		if (packet->seq == ctx->m_cwndseq) {
			if (next) {
				packet = pod_container_of(next, NetPacket_t, node);
				ctx->m_cwndseq = packet->seq;
				*cwndskip = 1;
			}
			else
				ctx->m_cwndseq = ctx->m_ackseq + 1;
		}
		return packet;
	}
	return (NetPacket_t*)0;
}

void transportctxCacheSendPacket(TransportCtx_t* ctx, NetPacket_t* packet) {
	if (NETPACKET_ACK == packet->type)
		return;
	packet->wait_ack = 0;
	packet->seq = ctx->m_sendseq++;
	listInsertNodeBack(&ctx->sendpacketlist, ctx->sendpacketlist.tail, &packet->node._);
}

int transportctxSendWindowHasPacket(TransportCtx_t* ctx, unsigned int seq) {
	return seq >= ctx->m_cwndseq && seq - ctx->m_cwndseq < ctx->cwndsize;
}

/*********************************************************************************/

StreamTransportCtx_t* streamtransportctxInit(StreamTransportCtx_t* ctx) {
	ctx->m_recvseq = ctx->m_sendseq = ctx->m_cwndseq = 0;
	listInit(&ctx->recvpacketlist);
	listInit(&ctx->sendpacketlist);
	return ctx;
}

int streamtransportctxRecvCheck(StreamTransportCtx_t* ctx, unsigned int seq) {
	return seq1_before_seq2(seq, ctx->m_recvseq) || ctx->m_recvseq == seq;
}

int streamtransportctxCacheRecvPacket(StreamTransportCtx_t* ctx, NetPacket_t* packet) {
	if (seq1_before_seq2(packet->seq, ctx->m_recvseq))
		return 0;
	else if (packet->seq != ctx->m_recvseq)
		return 0;
	else if (NETPACKET_FRAGMENT == packet->type ||
		NETPACKET_FRAGMENT_EOF == packet->type ||
		NETPACKET_END == packet->type)
	{
		listInsertNodeBack(&ctx->recvpacketlist, ctx->recvpacketlist.tail, &packet->node._);
		ctx->m_recvseq++;
		return 1;
	}
	return 0;
}

int streamtransportctxMergeRecvPacket(StreamTransportCtx_t* ctx, List_t* list) {
	ListNode_t* cur;
	for (cur = ctx->recvpacketlist.head; cur; cur = cur->next) {
		NetPacket_t* packet = pod_container_of(cur, NetPacket_t, node);
		if (NETPACKET_FRAGMENT_EOF != packet->type && NETPACKET_END != packet->type)
			continue;
		*list = listSplitByTail(&ctx->recvpacketlist, cur);
		return 1;
	}
	return 0;
}

void streamtransportctxCacheSendPacket(StreamTransportCtx_t* ctx, NetPacket_t* packet) {
	if (NETPACKET_ACK == packet->type || NETPACKET_END == packet->type) {
		packet->seq = 0;
	}
	else {
		packet->wait_ack = 1;
		packet->seq = ctx->m_sendseq++;
	}
	listInsertNodeBack(&ctx->sendpacketlist, ctx->sendpacketlist.tail, &packet->node._);
}

int streamtransportctxAckSendPacket(StreamTransportCtx_t* ctx, unsigned int ackseq, NetPacket_t** ackpacket) {
	ListNode_t* cur, *next;
	*ackpacket = (NetPacket_t*)0;
	if (seq1_before_seq2(ackseq, ctx->m_cwndseq))
		return 1;
	if (ctx->m_cwndseq != ackseq)
		return 0;
	for (cur = ctx->sendpacketlist.head; cur; cur = next) {
		NetPacket_t* packet = pod_container_of(cur, NetPacket_t, node);
		next = cur->next;
		if (packet->off < packet->len)
			return 0;
		if (NETPACKET_ACK == packet->type || NETPACKET_END == packet->type)
			continue;
		if (packet->seq != ackseq)
			return 0;
		ctx->m_cwndseq++;
		listRemoveNode(&ctx->sendpacketlist, cur);
		*ackpacket = packet;
		return 1;
	}
	return 0;
}

#ifdef	__cplusplus
}
#endif
