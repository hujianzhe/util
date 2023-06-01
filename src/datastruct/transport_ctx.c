//
// Created by hujianzhe on 2019-7-10.
//

#include "../../inc/datastruct/transport_ctx.h"

#define seq1_before_seq2(seq1, seq2)	((int)(seq1 - seq2) < 0)

#ifdef	__cplusplus
extern "C" {
#endif

/*********************************************************************************/

DgramTransportCtx_t* dgramtransportctxInit(DgramTransportCtx_t* ctx, unsigned int initseq) {
	ctx->send_all_acked = 1;
	ctx->cwndsize = 1;
	ctx->cache_recv_bytes = 0;
	ctx->cache_send_bytes = 0;
	ctx->m_cwndseq = ctx->m_recvseq = ctx->m_sendseq = ctx->m_ackseq = initseq;
	ctx->m_recvnode = (struct ListNode_t*)0;
	listInit(&ctx->recvlist);
	listInit(&ctx->sendlist);
	return ctx;
}

int dgramtransportctxRecvCheck(DgramTransportCtx_t* ctx, unsigned int seq, int pktype) {
	ListNode_t* cur;
	if (pktype < NETPACKET_DGRAM_HAS_SEND_SEQ) {
		return 0;
	}
	if (seq1_before_seq2(seq, ctx->m_recvseq)) {
		return 0;
	}
	cur = ctx->m_recvnode ? ctx->m_recvnode : ctx->recvlist.head;
	for (; cur; cur = cur->next) {
		NetPacket_t* pk = pod_container_of(cur, NetPacket_t, node);
		if (seq1_before_seq2(seq, pk->seq)) {
			break;
		}
		if (seq == pk->seq) {
			return 0;
		}
	}
	return 1;
}

void dgramtransportctxCacheRecvPacket(DgramTransportCtx_t* ctx, NetPacket_t* packet) {
	ListNode_t* cur = ctx->m_recvnode ? ctx->m_recvnode : ctx->recvlist.head;
	for (; cur; cur = cur->next) {
		NetPacket_t* pk = pod_container_of(cur, NetPacket_t, node);
		if (seq1_before_seq2(packet->seq, pk->seq)) {
			break;
		}
	}
	if (cur) {
		listInsertNodeFront(&ctx->recvlist, cur, &packet->node);
	}
	else {
		listInsertNodeBack(&ctx->recvlist, ctx->recvlist.tail, &packet->node);
	}

	cur = &packet->node;
	while (cur) {
		packet = pod_container_of(cur, NetPacket_t, node);
		if (ctx->m_recvseq != packet->seq) {
			break;
		}
		ctx->m_recvseq++;
		ctx->m_recvnode = &packet->node;
		cur = cur->next;
	}
	packet->cached = 1;
	ctx->cache_recv_bytes += (packet->hdrlen + packet->bodylen);
}

int dgramtransportctxMergeRecvPacket(DgramTransportCtx_t* ctx, List_t* list) {
	ListNode_t* cur;
	if (!ctx->m_recvnode) {
		return 0;
	}
	for (cur = ctx->recvlist.head; cur && cur != ctx->m_recvnode->next; cur = cur->next) {
		NetPacket_t* packet = pod_container_of(cur, NetPacket_t, node);
		if (!packet->fragment_eof) {
			continue;
		}
		*list = listSplitByTail(&ctx->recvlist, cur);
		if (!ctx->recvlist.head || ctx->m_recvnode == cur) {
			ctx->m_recvnode = (struct ListNode_t*)0;
		}
		for (cur = list->head; cur; cur = cur->next) {
			packet = pod_container_of(cur, NetPacket_t, node);
			packet->cached = 0;
			ctx->cache_recv_bytes -= (packet->hdrlen + packet->bodylen);
		}
		return 1;
	}
	return 0;
}

unsigned int dgramtransportctxNextSendSeq(DgramTransportCtx_t* ctx, int pktype) {
	return pktype < NETPACKET_DGRAM_HAS_SEND_SEQ ? 0 : ctx->m_sendseq++;
}

int dgramtransportctxCacheSendPacket(DgramTransportCtx_t* ctx, NetPacket_t* packet) {
	if (packet->type < NETPACKET_DGRAM_HAS_SEND_SEQ) {
		return 0;
	}
	if (packet->type > NETPACKET_FIN) {
		ctx->send_all_acked = 0;
	}
	packet->wait_ack = 0;
	listInsertNodeBack(&ctx->sendlist, ctx->sendlist.tail, &packet->node);
	ctx->cache_send_bytes += (packet->hdrlen + packet->bodylen);
	packet->cached = 1;
	return 1;
}

int dgramtransportctxAckSendPacket(DgramTransportCtx_t* ctx, unsigned int ackseq, NetPacket_t** ackpacket) {
	ListNode_t* cur, *next;
	int cwndskip = 0;
	*ackpacket = (NetPacket_t*)0;
	if (seq1_before_seq2(ackseq, ctx->m_cwndseq)) {
		return cwndskip;
	}
	for (cur = ctx->sendlist.head; cur; cur = next) {
		NetPacket_t* packet = pod_container_of(cur, NetPacket_t, node);
		next = cur->next;
		if (packet->seq != ackseq) {
			continue;
		}
		if (!packet->wait_ack) {
			break;
		}
		if (seq1_before_seq2(ctx->m_ackseq, ackseq)) {
			ctx->m_ackseq = ackseq;
		}
		listRemoveNode(&ctx->sendlist, cur);
		ctx->cache_send_bytes -= (packet->hdrlen + packet->bodylen);
		if (packet->seq == ctx->m_cwndseq) {
			if (next) {
				NetPacket_t* next_packet = pod_container_of(next, NetPacket_t, node);
				ctx->m_cwndseq = next_packet->seq;
				cwndskip = 1;
			}
			else {
				ctx->m_cwndseq = ctx->m_ackseq + 1;
			}
		}
		if (!ctx->sendlist.head ||
			NETPACKET_FIN == pod_container_of(ctx->sendlist.head, NetPacket_t, node)->type)
		{
			ctx->send_all_acked = 1;
		}
		packet->cached = 0;
		*ackpacket = packet;
		return cwndskip;
	}
	return cwndskip;
}

int dgramtransportctxSendWindowHasPacket(DgramTransportCtx_t* ctx, NetPacket_t* packet) {
	if (NETPACKET_FIN == packet->type && ctx->sendlist.head != &packet->node) {
		return 0;
	}
	return packet->seq >= ctx->m_cwndseq && packet->seq - ctx->m_cwndseq < ctx->cwndsize;
}

/*********************************************************************************/

StreamTransportCtx_t* streamtransportctxInit(StreamTransportCtx_t* ctx) {
	ctx->cache_recv_bytes = 0;
	ctx->cache_send_bytes = 0;
	listInit(&ctx->recvlist);
	listInit(&ctx->sendlist);
	return ctx;
}

void streamtransportctxCacheRecvPacket(StreamTransportCtx_t* ctx, NetPacket_t* packet) {
	listInsertNodeBack(&ctx->recvlist, ctx->recvlist.tail, &packet->node);
	packet->cached = 1;
	ctx->cache_recv_bytes += (packet->hdrlen + packet->bodylen);
}

int streamtransportctxMergeRecvPacket(StreamTransportCtx_t* ctx, List_t* list) {
	ListNode_t* cur;
	for (cur = ctx->recvlist.head; cur; cur = cur->next) {
		NetPacket_t* packet = pod_container_of(cur, NetPacket_t, node);
		if (!packet->fragment_eof) {
			continue;
		}
		*list = listSplitByTail(&ctx->recvlist, cur);
		for (cur = list->head; cur; cur = cur->next) {
			packet = pod_container_of(cur, NetPacket_t, node);
			packet->cached = 0;
			ctx->cache_recv_bytes -= (packet->hdrlen + packet->bodylen);
		}
		return 1;
	}
	return 0;
}

int streamtransportctxSendCheckBusy(StreamTransportCtx_t* ctx) {
	if (ctx->sendlist.tail) {
		NetPacket_t* packet = pod_container_of(ctx->sendlist.tail, NetPacket_t, node);
		if (packet->off < packet->hdrlen + packet->bodylen) {
			return 1;
		}
	}
	return 0;
}

int streamtransportctxCacheSendPacket(StreamTransportCtx_t* ctx, NetPacket_t* packet) {
	if (packet->off >= packet->hdrlen + packet->bodylen) {
		return 0;
	}
	listInsertNodeBack(&ctx->sendlist, ctx->sendlist.tail, &packet->node);
	ctx->cache_send_bytes += (packet->hdrlen + packet->bodylen);
	packet->cached = 1;
	return 1;
}

void streamtransportctxRemoveCacheSendPacket(StreamTransportCtx_t* ctx, NetPacket_t* packet) {
	listRemoveNode(&ctx->sendlist, &packet->node);
	ctx->cache_send_bytes -= (packet->hdrlen + packet->bodylen);
	packet->cached = 0;
}

#ifdef	__cplusplus
}
#endif
