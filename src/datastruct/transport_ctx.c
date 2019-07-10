//
// Created by hujianzhe on 2019-7-10.
//

#include "../../inc/datastruct/transport_ctx.h"

#define seq1_before_seq2(seq1, seq2)	((int)(seq1 - seq2) < 0)

#ifdef	__cplusplus
extern "C" {
#endif

int transportctxInsertRecvPacket(TransportCtx_t* ctx, NetPacket_t* packet) {
	if (seq1_before_seq2(packet->seq, ctx->recvseq))
		return 0;
	else {
		ListNode_t* cur = ctx->recvnode ? ctx->recvnode : ctx->recvpacketlist.head;
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
			if (ctx->recvseq != packet->seq)
				break;
			ctx->recvseq++;
			ctx->recvnode = &packet->node._;
			cur = cur->next;
		}
		return 1;
	}
}

int transportctxMergeRecvPacket(TransportCtx_t* ctx, List_t* list) {
	ListNode_t* cur;
	if (ctx->recvnode) {
		for (cur = ctx->recvpacketlist.head; cur && cur != ctx->recvnode->next; cur = cur->next) {
			NetPacket_t* packet = pod_container_of(cur, NetPacket_t, node);
			if (NETPACKET_FRAGMENT_EOF != packet->type && NETPACKET_END != packet->type)
				continue;
			*list = listSplitByTail(&ctx->recvpacketlist, cur);
			if (!ctx->recvpacketlist.head || ctx->recvnode == cur)
				ctx->recvnode = (struct ListNode_t*)0;
			return 1;
		}
	}
	return 0;
}

NetPacket_t* transportctxAckSendPacket(TransportCtx_t* ctx, unsigned int ackseq, int* cwndskip) {
	ListNode_t* cur, *next;
	*cwndskip = 0;
	if (seq1_before_seq2(ackseq, ctx->cwndseq))
		return (NetPacket_t*)0;
	for (cur = ctx->sendpacketlist.head; cur; cur = next) {
		NetPacket_t* packet = pod_container_of(cur, NetPacket_t, node);
		next = cur->next;
		if (packet->seq != ackseq)
			continue;
		listRemoveNode(&ctx->sendpacketlist, cur);
		if (packet->seq == ctx->cwndseq) {
			if (next) {
				packet = pod_container_of(next, NetPacket_t, node);
				ctx->cwndseq = packet->seq;
				*cwndskip = 1;
			}
			else
				ctx->cwndseq++;
		}
		return packet;
	}
	return (NetPacket_t*)0;
}

void transportctxInsertSendPacket(TransportCtx_t* ctx, NetPacket_t* packet) {
	packet->need_ack = 1;
	packet->resend_times = 0;
	packet->resend_msec = 0;
	packet->seq = ctx->sendseq++;
	packet->off = 0;
	listInsertNodeBack(&ctx->sendpacketlist, ctx->sendpacketlist.tail, &packet->node._);
}

int transportctxSendWindowHasPacket(TransportCtx_t* ctx, unsigned int seq) {
	return seq >= ctx->cwndseq && seq - ctx->cwndseq < ctx->cwndsize;
}

#ifdef	__cplusplus
}
#endif
