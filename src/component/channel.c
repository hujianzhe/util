//
// Created by hujianzhe on 2019-7-11.
//

#include "../../inc/component/channel.h"
#include <stdlib.h>

#ifdef	__cplusplus
extern "C" {
#endif

Channel_t* channelInit(Channel_t* channel, int flag, int initseq) {
	channel->flag = flag;
	channel->heartbeat_timeout_sec = 0;
	channel->zombie_timeout_sec = 0;
	channel->event_msec = 0;
	channel->inbuf = NULL;
	channel->inbuflen = 0;
	channel->inbufoff = 0;
	memset(&channel->peer_saddr, 0, sizeof(channel->peer_saddr));
	channel->peer_saddr.ss_family = AF_UNSPEC;
	if (channel->flag & CHANNEL_FLAG_RELIABLE) {
		if (flag & CHANNEL_FLAG_DGRAM) {
			dgramtransportctxInit(&channel->dgram.ctx, initseq);
			channel->dgram.send = NULL;
		}
		else {
			streamtransportctxInit(&channel->stream.ctx, initseq);
		}
	}
	channel->decode = NULL;
	channel->recv = NULL;
	channel->reply_ack = NULL;
	channel->heartbeat = NULL;
	channel->zombie = NULL;
	channel->shutdown = NULL;
	channel->has_recvfin = 0;
	channel->has_sendfin = 0;
	channel->m_lastactive_msec = 0;
	channel->m_heartbeat_msec = 0;
	return channel;
}

static void update_timestamp(long long* dst, long long ts) {
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

static int channel_stream_recv_handler(Channel_t* channel) {
	int ok = 1;
	while (channel->inbufoff < channel->inbuflen) {
		memset(&channel->decode_result, 0, sizeof(channel->decode_result));
		channel->decode(channel, channel->inbuf + channel->inbufoff, channel->inbuflen - channel->inbufoff);
		if (channel->decode_result.err) {
			ok = 0;
			break;
		}
		else if (channel->decode_result.incomplete)
			return 1;
		channel->inbufoff += channel->decode_result.decodelen;
		if (channel->flag & CHANNEL_FLAG_RELIABLE) {
			unsigned char pktype = channel->decode_result.pktype;
			unsigned int pkseq = channel->decode_result.pkseq;
			int res = streamtransportctxRecvCheck(&channel->stream.ctx, pkseq, pktype);
			if (res) {
				NetPacket_t* packet;
				channel->reply_ack(channel, pkseq);
				if (res < 0)
					continue;
				packet = (NetPacket_t*)malloc(sizeof(NetPacket_t) + channel->decode_result.bodylen);
				if (!packet) {
					ok = 0;
					break;
				}
				packet->type = pktype;
				packet->seq = pkseq;
				packet->len = channel->decode_result.bodylen;
				memcpy(packet->data, channel->decode_result.bodyptr, packet->len);
				packet->data[packet->len] = 0;
				if (streamtransportctxCacheRecvPacket(&channel->stream.ctx, packet)) {
					List_t list;
					while (streamtransportctxMergeRecvPacket(&channel->stream.ctx, &list)) {
						channel->decode_result.bodyptr = merge_packet(&list, &channel->decode_result.bodylen);
						if (!channel->decode_result.bodyptr) {
							ok = 0;
							break;
						}
						channel->recv(channel);
						free(channel->decode_result.bodyptr);
					}
				}
			}
			else if (NETPACKET_ACK == pktype) {
				NetPacket_t* ackpk = NULL;
				if (streamtransportctxAckSendPacket(&channel->stream.ctx, pkseq, &ackpk)) {
					free(ackpk);
				}
				else {
					ok = 0;
					break;
				}
			}
			else if (NETPACKET_NO_ACK_FRAGMENT == pktype) {
				channel->recv(channel);
			}
			else {
				ok = 0;
				break;
			}
		}
		else {
			channel->recv(channel);
		}
	}
	free(channel->inbuf);
	channel->inbuf = NULL;
	channel->inbuflen = 0;
	channel->inbufoff = 0;
	return ok;
}

static int channel_dgram_recv_handler(Channel_t* channel, long long timestamp_msec, const void* from_saddr) {
	memset(&channel->decode_result, 0, sizeof(channel->decode_result));
	channel->decode(channel, channel->inbuf, channel->inbuflen);
	if (channel->decode_result.err)
		return 0;
	else if (channel->decode_result.incomplete)
		return 1;
	if (channel->flag & CHANNEL_FLAG_RELIABLE) {
		NetPacket_t* packet;
		unsigned char pktype = channel->decode_result.pktype;
		unsigned int pkseq = channel->decode_result.pkseq;
		if (dgramtransportctxRecvCheck(&channel->dgram.ctx, pkseq, pktype)) {
			channel->reply_ack(channel, pkseq);
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
					packet = pod_container_of(list.tail, NetPacket_t, node);
					if (NETPACKET_FIN == packet->type) {
						ListNode_t* cur, *next;
						for (cur = list.head; cur; cur = next) {
							packet = pod_container_of(cur, NetPacket_t, node);
							next = cur->next;
							free(packet);
						}
						channel->has_recvfin = 1;
						channel->shutdown(channel);
					}
					else {
						channel->decode_result.bodyptr = merge_packet(&list, &channel->decode_result.bodylen);
						if (!channel->decode_result.bodyptr) {
							return 0;
						}
						channel->recv(channel);
						free(channel->decode_result.bodyptr);
					}
				}
			}
		}
		else if (NETPACKET_ACK == pktype) {
			int cwndskip;
			packet = dgramtransportctxAckSendPacket(&channel->dgram.ctx, pkseq, &cwndskip);
			if (packet) {
				free(packet);
				if (cwndskip) {
					ListNode_t* cur;
					for (cur = channel->dgram.ctx.sendpacketlist.head; cur; cur = cur->next) {
						packet = pod_container_of(cur, NetPacket_t, node);
						if (!dgramtransportctxSendWindowHasPacket(&channel->dgram.ctx, packet->seq))
							break;
						if (packet->wait_ack)
							continue;
						channel->dgram.send(channel, packet);
						packet->wait_ack = 1;
						packet->resend_times = 0;
						packet->resend_msec = timestamp_msec + channel->dgram.ctx.rto;
						update_timestamp(&channel->event_msec, packet->resend_msec);
					}
				}
			}
		}
		else if (NETPACKET_NO_ACK_FRAGMENT == pktype) {
			channel->recv(channel);
		}
		else {
			channel->reply_ack(channel, pkseq);
		}
	}
	else {
		channel->recv(channel);
	}
	return 1;
}

int channelRecvHandler(Channel_t* channel, long long timestamp_msec, const void* from_saddr) {
	int res;
	if (channel->flag & CHANNEL_FLAG_DGRAM)
		res = channel_dgram_recv_handler(channel, timestamp_msec, from_saddr);
	else
		res = channel_stream_recv_handler(channel);
	if (res) {
		channel->m_lastactive_msec = timestamp_msec;
		channel->m_heartbeat_msec = timestamp_msec;
		return 1;
	}
	return 0;
}

int channelEventHandler(Channel_t* channel, long long timestamp_msec) {
	if (timestamp_msec > channel->event_msec)
		return 1;
	channel->event_msec = 0;
	if (channel->zombie_timeout_sec > 0 &&
		channel->m_lastactive_msec > 0)
	{
		long long ts = channel->zombie_timeout_sec;
		ts *= 1000;
		ts += channel->m_lastactive_msec;
		if (ts <= timestamp_msec) {
			if (channel->zombie && channel->zombie(channel)) {
				channel->m_lastactive_msec = timestamp_msec;
				channel->m_heartbeat_msec = timestamp_msec;
				ts = channel->zombie_timeout_sec;
				ts *= 1000;
				ts += timestamp_msec;
			}
			else
				return 0;
		}
		update_timestamp(&channel->event_msec, ts);
	}
	if ((channel->flag & CHANNEL_FLAG_DGRAM) &&
		(channel->flag & CHANNEL_FLAG_RELIABLE))
	{
		int ok = 1;
		ListNode_t* cur;
		for (cur = channel->dgram.ctx.sendpacketlist.head; cur; cur = cur->next) {
			NetPacket_t* packet = pod_container_of(cur, NetPacket_t, node);
			if (!packet->wait_ack)
				break;
			if (packet->resend_times >= channel->dgram.ctx.resend_maxtimes) {
				if (!channel->has_sendfin && !channel->has_recvfin)
					ok = 0;
				break;
			}
			channel->dgram.send(channel, packet);
			packet->resend_times++;
			packet->resend_msec = timestamp_msec + channel->dgram.ctx.rto;
			update_timestamp(&channel->event_msec, packet->resend_msec);
		}
		if (!ok)
			return 0;
	}
	if (channel->flag & CHANNEL_FLAG_CLIENT) {
		if (channel->heartbeat &&
			channel->m_heartbeat_msec > 0 &&
			channel->heartbeat_timeout_sec > 0)
		{
			long long ts = channel->heartbeat_timeout_sec;
			ts *= 1000;
			ts += channel->m_heartbeat_msec;
			if (ts <= timestamp_msec) {
				channel->heartbeat(channel);
				channel->m_heartbeat_msec = timestamp_msec;
				ts = channel->heartbeat_timeout_sec;
				ts *= 1000;
				ts += timestamp_msec;
			}
			update_timestamp(&channel->event_msec, ts);
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
		channel_free_packetlist(&channel->dgram.ctx.recvpacketlist);
		channel_free_packetlist(&channel->dgram.ctx.sendpacketlist);
	}
	free(channel->inbuf);
	channel->inbuf = NULL;
	channel->inbuflen = 0;
	channel->inbufoff = 0;
}

#ifdef	__cplusplus
}
#endif