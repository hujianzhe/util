//
// Created by hujianzhe on 2019-7-11.
//

#include "../../inc/component/channel.h"
#include <stdlib.h>
#include <string.h>

#ifdef	__cplusplus
extern "C" {
#endif

Channel_t* channelInit(Channel_t* channel, int flag, int initseq) {
	channel->flag = flag;
	channel->heartbeat_timeout_sec = 0;
	channel->heartbeat_maxtimes = 0;
	channel->heartbeat_msec = 0;
	channel->has_recvfin = 0;
	channel->has_sendfin = 0;
	channel->event_msec = 0;
	memset(&channel->dgram.listen_addr, 0, sizeof(channel->dgram.listen_addr));
	channel->dgram.listen_addr.sa.sa_family = AF_UNSPEC;
	memset(&channel->dgram.connect_addr, 0, sizeof(channel->dgram.connect_addr));
	channel->dgram.connect_addr.sa.sa_family = AF_UNSPEC;
	memset(&channel->to_addr, 0, sizeof(channel->to_addr));
	channel->to_addr.sa.sa_family = AF_UNSPEC;
	if (flag & CHANNEL_FLAG_DGRAM) {
		channel->dgram.synpacket = NULL;
		channel->dgram.free_halfconn = NULL;
		channel->dgram.reply_synack = NULL;
		channel->dgram.recv_syn = NULL;
		channel->dgram.ack_halfconn = NULL;
		channel->dgram.send = NULL;
		dgramtransportctxInit(&channel->dgram.ctx, initseq);
	}
	else {
		streamtransportctxInit(&channel->stream.ctx, initseq);
	}
	channel->decode = NULL;
	channel->recv = NULL;
	channel->reply_ack = NULL;
	channel->heartbeat = NULL;
	channel->zombie = NULL;
	channel->shutdown = NULL;

	channel->m_heartbeat_times = 0;
	return channel;
}

static void update_timestamp(long long* dst, long long ts) {
	if (ts <= 0)
		return;
	if (*dst <= 0 || *dst > ts)
		*dst = ts;
}

static unsigned char* merge_packet(List_t* list, unsigned int* mergelen) {
	unsigned char* ptr;
	NetPacket_t* packet;
	ListNode_t* cur, *next;
	unsigned int off;
	off = 0;
	for (cur = list->head; cur; cur = next) {
		packet = pod_container_of(cur, NetPacket_t, node);
		next = cur->next;
		off += packet->len;
	}
	ptr = (unsigned char*)malloc(off);
	if (ptr) {
		*mergelen = off;
		off = 0;
		for (cur = list->head; cur; cur = next) {
			packet = pod_container_of(cur, NetPacket_t, node);
			next = cur->next;
			memcpy(ptr + off, packet->data, packet->len);
			off += packet->len;
			free(packet);
		}
	}
	return ptr;
}

static int channel_merge_packet_handler(Channel_t* channel, List_t* packetlist) {
	NetPacket_t* packet = pod_container_of(packetlist->tail, NetPacket_t, node);
	if (NETPACKET_FIN == packet->type) {
		ListNode_t* cur, *next;
		for (cur = packetlist->head; cur; cur = next) {
			next = cur->next;
			free(pod_container_of(cur, NetPacket_t, node));
		}
		channel->has_recvfin = 1;
		if (channel->shutdown) {
			channel->shutdown(channel);
			channel->shutdown = NULL;
		}
	}
	else {
		channel->decode_result.bodyptr = merge_packet(packetlist, &channel->decode_result.bodylen);
		if (!channel->decode_result.bodyptr)
			return 0;
		channel->recv(channel, &channel->to_addr);
		free(channel->decode_result.bodyptr);
	}
	return 1;
}

static int channel_stream_recv_handler(Channel_t* channel, unsigned char* buf, int len, int off, long long timestamp_msec) {
	if (0 == len) {
		if (channel->shutdown) {
			channel->shutdown(channel);
			channel->shutdown = NULL;
		}
		return 0;
	}
	while (off < len) {
		memset(&channel->decode_result, 0, sizeof(channel->decode_result));
		channel->decode(channel, buf + off, len - off);
		if (channel->decode_result.err)
			return -1;
		else if (channel->decode_result.incomplete)
			return off;
		off += channel->decode_result.decodelen;
		if (channel->flag & CHANNEL_FLAG_RELIABLE) {
			unsigned char pktype = channel->decode_result.pktype;
			unsigned int pkseq = channel->decode_result.pkseq;
			int res = streamtransportctxRecvCheck(&channel->stream.ctx, pkseq, pktype);
			if (res) {
				NetPacket_t* packet;
				channel->reply_ack(channel, pkseq, &channel->to_addr);
				if (res < 0)
					continue;
				packet = (NetPacket_t*)malloc(sizeof(NetPacket_t) + channel->decode_result.bodylen);
				if (!packet)
					return -1;
				packet->type = pktype;
				packet->seq = pkseq;
				packet->len = channel->decode_result.bodylen;
				memcpy(packet->data, channel->decode_result.bodyptr, packet->len);
				packet->data[packet->len] = 0;
				if (streamtransportctxCacheRecvPacket(&channel->stream.ctx, packet)) {
					List_t list;
					while (streamtransportctxMergeRecvPacket(&channel->stream.ctx, &list)) {
						if (!channel_merge_packet_handler(channel, &list))
							return -1;
					}
				}
			}
			else if (NETPACKET_ACK == pktype) {
				NetPacket_t* ackpk = NULL;
				if (streamtransportctxAckSendPacket(&channel->stream.ctx, pkseq, &ackpk))
					free(ackpk);
				else
					return -1;
			}
			else if (NETPACKET_NO_ACK_FRAGMENT == pktype)
				channel->recv(channel, &channel->to_addr);
			else
				return -1;
		}
		else
			channel->recv(channel, &channel->to_addr);
	}

	channel->m_heartbeat_times = 0;
	channel->heartbeat_msec = timestamp_msec;
	if (channel->heartbeat_timeout_sec > 0) {
		long long ts = channel->heartbeat_timeout_sec;
		ts *= 1000;
		ts += channel->heartbeat_msec;
		update_timestamp(&channel->event_msec, ts);
	}

	return off;
}

static int channel_dgram_recv_handler(Channel_t* channel, unsigned char* buf, int len, long long timestamp_msec, const void* from_saddr) {
	if (channel->flag & CHANNEL_FLAG_RELIABLE) {
		NetPacket_t* packet;
		unsigned char pktype;
		unsigned int pkseq;
		int from_listen;
		int from_peer;
		if (!(channel->flag & CHANNEL_FLAG_LISTEN)) {
			if (channel->flag & CHANNEL_FLAG_CLIENT)
				from_listen = sockaddrIsEqual(&channel->dgram.connect_addr, from_saddr);
			else
				from_listen = 0;
			from_peer = sockaddrIsEqual(&channel->to_addr, from_saddr);
			if (!from_peer && !from_listen)
				return 1;
		}
		memset(&channel->decode_result, 0, sizeof(channel->decode_result));
		channel->decode(channel, buf, len);
		if (channel->decode_result.err)
			return 0;
		else if (channel->decode_result.incomplete)
			return 1;
		pktype = channel->decode_result.pktype;
		pkseq = channel->decode_result.pkseq;
		if (dgramtransportctxRecvCheck(&channel->dgram.ctx, pkseq, pktype)) {
			channel->reply_ack(channel, pkseq, from_saddr);
			packet = (NetPacket_t*)malloc(sizeof(NetPacket_t) + channel->decode_result.bodylen);
			if (!packet)
				return 0;
			packet->type = pktype;
			packet->seq = pkseq;
			packet->len = channel->decode_result.bodylen;
			memcpy(packet->data, channel->decode_result.bodyptr, packet->len);
			packet->data[packet->len] = 0;
			if (dgramtransportctxCacheRecvPacket(&channel->dgram.ctx, packet)) {
				List_t list;
				while (dgramtransportctxMergeRecvPacket(&channel->dgram.ctx, &list)) {
					if (!channel_merge_packet_handler(channel, &list))
						return 0;
				}
			}
		}
		else if (NETPACKET_SYN == pktype) {
			if (channel->flag & CHANNEL_FLAG_LISTEN) {
				DgramHalfConn_t* halfconn;
				ListNode_t* cur;
				for (cur = channel->dgram.ctx.recvpacketlist.head; cur; cur = cur->next) {
					halfconn = pod_container_of(cur, DgramHalfConn_t, node);
					if (sockaddrIsEqual(&halfconn->from_addr, from_saddr))
						break;
				}
				if (!cur) {
					halfconn = channel->dgram.recv_syn(from_saddr);
					if (!halfconn)
						return 1;
					halfconn->resend_times = 0;
					halfconn->resend_msec = timestamp_msec + channel->dgram.ctx.rto;
					memcpy(&halfconn->from_addr, from_saddr, sockaddrLength(from_saddr));
					listInsertNodeBack(&channel->dgram.ctx.recvpacketlist, channel->dgram.ctx.recvpacketlist.tail, &halfconn->node._);
					update_timestamp(&channel->event_msec, halfconn->resend_msec);
				}
				channel->dgram.reply_synack(halfconn);
			}
		}
		else if (NETPACKET_SYN_ACK == pktype) {
			if (channel->flag & CHANNEL_FLAG_LISTEN)
				return 1;
			if (!from_listen)
				return 1;
			if (channel->dgram.synpacket) {
				free(channel->dgram.synpacket);
				channel->dgram.synpacket = NULL;
				channel->recv(channel, from_saddr);
			}
			channel->reply_ack(channel, 0, from_saddr);
			channel->dgram.send(channel, NULL, &channel->to_addr);
		}
		else if (NETPACKET_ACK == pktype) {
			ListNode_t* cur;
			if (channel->flag & CHANNEL_FLAG_LISTEN) {
				ListNode_t* next;
				for (cur = channel->dgram.ctx.recvpacketlist.head; cur; cur = next) {
					DgramHalfConn_t* halfconn = pod_container_of(cur, DgramHalfConn_t, node);
					next = cur->next;
					if (!sockaddrIsEqual(&halfconn->from_addr, from_saddr))
						continue;
					if (!channel->dgram.ack_halfconn(halfconn))
						continue;
					listRemoveNode(&channel->dgram.ctx.recvpacketlist, &halfconn->node._);
					halfconn->sockfd = INVALID_FD_HANDLE;
					channel->dgram.free_halfconn(halfconn);
				}
			}
			else {
				int cwndskip;
				packet = dgramtransportctxAckSendPacket(&channel->dgram.ctx, pkseq, &cwndskip);
				if (!packet)
					return 1;
				free(packet);
				if (!cwndskip)
					return 1;
				for (cur = channel->dgram.ctx.sendpacketlist.head; cur; cur = cur->next) {
					packet = pod_container_of(cur, NetPacket_t, node);
					if (!dgramtransportctxSendWindowHasPacket(&channel->dgram.ctx, packet->seq))
						break;
					if (packet->wait_ack)
						continue;
					channel->dgram.send(channel, packet, &channel->to_addr);
					packet->wait_ack = 1;
					packet->resend_times = 0;
					packet->resend_msec = timestamp_msec + channel->dgram.ctx.rto;
					update_timestamp(&channel->event_msec, packet->resend_msec);
				}
			}
		}
		else if (NETPACKET_NO_ACK_FRAGMENT == pktype) {
			if (channel->flag & CHANNEL_FLAG_LISTEN)
				return 1;
			channel->recv(channel, from_saddr);
		}
		else {
			if (channel->flag & CHANNEL_FLAG_LISTEN)
				return 1;
			channel->reply_ack(channel, pkseq, from_saddr);
		}
	}
	else {
		memset(&channel->decode_result, 0, sizeof(channel->decode_result));
		channel->decode(channel, buf, len);
		if (channel->decode_result.err)
			return 0;
		else if (channel->decode_result.incomplete)
			return 1;
		channel->recv(channel, from_saddr);
	}

	channel->m_heartbeat_times = 0;
	channel->heartbeat_msec = timestamp_msec;
	if (channel->heartbeat_timeout_sec > 0) {
		long long ts = channel->heartbeat_timeout_sec;
		ts *= 1000;
		ts += channel->heartbeat_msec;
		update_timestamp(&channel->event_msec, ts);
	}

	return 1;
}

int channelRecvHandler(Channel_t* channel, unsigned char* buf, int len, int off, long long timestamp_msec, const void* from_saddr) {
	if (channel->flag & CHANNEL_FLAG_DGRAM)
		return channel_dgram_recv_handler(channel, buf, len, timestamp_msec, from_saddr);
	else
		return channel_stream_recv_handler(channel, buf, len, off, timestamp_msec);
}

int channelEventHandler(Channel_t* channel, long long timestamp_msec) {
	if (timestamp_msec > channel->event_msec)
		return 1;
	channel->event_msec = 0;
	if (channel->flag & CHANNEL_FLAG_CLIENT) {
		if (channel->heartbeat_msec > 0 &&
			channel->heartbeat_timeout_sec > 0)
		{
			long long ts = channel->heartbeat_timeout_sec;
			ts *= 1000;
			ts += channel->heartbeat_msec;
			if (ts <= timestamp_msec) {
				do {
					if (channel->m_heartbeat_times < channel->heartbeat_maxtimes) {
						channel->heartbeat(channel);
						channel->m_heartbeat_times++;
						break;
					}
					if (!channel->zombie || !channel->zombie(channel))
						return 0;
					channel->m_heartbeat_times = 0;
				} while (0);
				channel->heartbeat_msec = timestamp_msec;
				ts = channel->heartbeat_timeout_sec;
				ts *= 1000;
				ts += timestamp_msec;
			}
			update_timestamp(&channel->event_msec, ts);
		}
	}
	if ((channel->flag & CHANNEL_FLAG_DGRAM) &&
		(channel->flag & CHANNEL_FLAG_RELIABLE))
	{
		if (channel->flag & CHANNEL_FLAG_LISTEN) {
			ListNode_t* cur, *next;
			for (cur = channel->dgram.ctx.recvpacketlist.head; cur; cur = next) {
				DgramHalfConn_t* halfconn = pod_container_of(cur, DgramHalfConn_t, node);
				next = cur->next;
				if (halfconn->resend_msec > timestamp_msec) {
					update_timestamp(&channel->event_msec, halfconn->resend_msec);
					continue;
				}
				if (halfconn->resend_times >= channel->dgram.ctx.resend_maxtimes) {
					listRemoveNode(&channel->dgram.ctx.recvpacketlist, cur);
					channel->dgram.free_halfconn(halfconn);
				}
				channel->dgram.reply_synack(halfconn);
				halfconn->resend_times++;
				halfconn->resend_msec = timestamp_msec + channel->dgram.ctx.rto;
				update_timestamp(&channel->event_msec, halfconn->resend_msec);
			}
		}
		else {
			ListNode_t* cur;
			for (cur = channel->dgram.ctx.sendpacketlist.head; cur; cur = cur->next) {
				NetPacket_t* packet = pod_container_of(cur, NetPacket_t, node);
				if (!packet->wait_ack)
					break;
				if (packet->resend_msec > timestamp_msec) {
					update_timestamp(&channel->event_msec, packet->resend_msec);
					continue;
				}
				if (packet->resend_times >= channel->dgram.ctx.resend_maxtimes) {
					if (!channel->has_sendfin && !channel->has_recvfin)
						return 0;
					break;
				}
				channel->dgram.send(channel, packet, &channel->to_addr);
				packet->resend_times++;
				packet->resend_msec = timestamp_msec + channel->dgram.ctx.rto;
				update_timestamp(&channel->event_msec, packet->resend_msec);
			}
		}
	}
	return 1;
}

static void channel_free_packetlist(List_t* list) {
	ListNode_t* cur, *next;
	for (cur = list->head; cur; cur = next) {
		next = cur->next;
		free(pod_container_of(cur, NetPacket_t, node));
	}
	listInit(list);
}

void channelDestroy(Channel_t* channel) {
	if (channel->flag & CHANNEL_FLAG_STREAM) {
		channel_free_packetlist(&channel->stream.ctx.recvpacketlist);
		channel_free_packetlist(&channel->stream.ctx.sendpacketlist);
	}
	else {
		if (channel->flag & CHANNEL_FLAG_LISTEN) {
			ListNode_t* cur, *next;
			for (cur = channel->dgram.ctx.recvpacketlist.head; cur; cur = next) {
				next = cur->next;
				channel->dgram.free_halfconn(pod_container_of(cur, DgramHalfConn_t, node));
			}
			listInit(&channel->dgram.ctx.recvpacketlist);
		}
		else {
			free(channel->dgram.synpacket);
			channel->dgram.synpacket = NULL;
			channel_free_packetlist(&channel->dgram.ctx.recvpacketlist);
			channel_free_packetlist(&channel->dgram.ctx.sendpacketlist);
		}
	}
}

#ifdef	__cplusplus
}
#endif