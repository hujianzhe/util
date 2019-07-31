//
// Created by hujianzhe on 2019-7-11.
//

#include "../../inc/component/channel.h"
#include "../../inc/sysapi/error.h"
#include <stdlib.h>
#include <string.h>

typedef struct DgramHalfConn_t {
	ListNode_t node;
	unsigned char resend_times;
	long long resend_msec;
	FD_t sockfd;
	Sockaddr_t from_addr;
	unsigned short local_port;
	unsigned int len;
	unsigned char buf[1];
} DgramHalfConn_t;

#ifdef	__cplusplus
extern "C" {
#endif

/*******************************************************************************/

static void free_halfconn(DgramHalfConn_t* halfconn) {
	if (INVALID_FD_HANDLE != halfconn->sockfd)
		socketClose(halfconn->sockfd);
	free(halfconn);
}

static void channel_set_timestamp(Channel_t* channel, long long timestamp_msec) {
	if (timestamp_msec <= 0)
		return;
	if (channel->m_event_msec <= 0 || channel->m_event_msec > timestamp_msec)
		channel->m_event_msec = timestamp_msec;
	reactorSetEventTimestamp(channel->io->reactor, timestamp_msec);
}

static void channel_set_heartbeat_timestamp(Channel_t* channel, long long timestamp_msec) {
	if (timestamp_msec <= 0)
		return;
	channel->heartbeat_msec = timestamp_msec;
	if (channel->heartbeat_timeout_sec > 0) {
		long long ts = channel->heartbeat_timeout_sec;
		ts *= 1000;
		ts += channel->heartbeat_msec;
		channel_set_timestamp(channel, ts);
	}
}

static void channel_detach_handler(Channel_t* channel, int reason, long long timestamp_msec) {
	if (channel->m_detached)
		return;
	channel->m_detached = 1;
	channel->inactive_reason = reason;
	listRemoveNode(&channel->io->channel_list, &channel->node._);
	if (!channel->io->channel_list.head)
		reactorobjectInvalid(channel->io, timestamp_msec);
	channel->inactive(channel, reason);
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
		off += packet->bodylen;
	}
	ptr = (unsigned char*)malloc(off);
	if (ptr) {
		*mergelen = off;
		off = 0;
		for (cur = list->head; cur; cur = next) {
			packet = pod_container_of(cur, NetPacket_t, node);
			next = cur->next;
			memcpy(ptr + off, packet->buf + packet->hdrlen, packet->bodylen);
			off += packet->bodylen;
			free(packet);
		}
	}
	return ptr;
}

static int channel_merge_packet_handler(Channel_t* channel, List_t* packetlist, long long timestamp_msec, int* err, ChannelInbufDecodeResult_t* decode_result) {
	NetPacket_t* packet = pod_container_of(packetlist->tail, NetPacket_t, node);
	if (NETPACKET_FIN == packet->type) {
		ListNode_t* cur, *next;
		for (cur = packetlist->head; cur; cur = next) {
			next = cur->next;
			free(pod_container_of(cur, NetPacket_t, node));
		}
		if (channel->flag & CHANNEL_FLAG_STREAM) {
			if (channel->io->stream.has_recvfin)
				return 1;
			channel->io->stream.has_recvfin = 1;
			if (channel->io->stream.has_sendfin)
				*err = streamtransportctxAllSendPacketIsAcked(&channel->io->stream.ctx) ? CHANNEL_INACTIVE_NORMAL : CHANNEL_INACTIVE_UNSENT;
			else
				channelSendFin(channel, timestamp_msec);
		}
		else {
			if (channel->dgram.has_recvfin)
				return 1;
			channel->dgram.has_recvfin = 1;
			if (channel->dgram.has_sendfin)
				*err = CHANNEL_INACTIVE_NORMAL;
			else
				channelSendFin(channel, timestamp_msec);
		}
	}
	else {
		decode_result->bodyptr = merge_packet(packetlist, &decode_result->bodylen);
		if (!decode_result->bodyptr)
			return 0;
		channel->recv(channel, &channel->to_addr, decode_result);
		free(decode_result->bodyptr);
	}
	return 1;
}

static int channel_stream_recv_handler(Channel_t* channel, unsigned char* buf, int len, int off, long long timestamp_msec, int* err) {
	ChannelInbufDecodeResult_t decode_result;
	while (off < len) {
		memset(&decode_result, 0, sizeof(decode_result));
		channel->decode(channel, buf + off, len - off, &decode_result);
		if (decode_result.err)
			return -1;
		else if (decode_result.incomplete)
			return off;
		off += decode_result.decodelen;
		if (channel->flag & CHANNEL_FLAG_RELIABLE) {
			unsigned char pktype = decode_result.pktype;
			unsigned int pkseq = decode_result.pkseq;
			if (streamtransportctxRecvCheck(&channel->io->stream.ctx, pkseq, pktype)) {
				NetPacket_t* packet;
				if (pktype >= NETPACKET_STREAM_HAS_SEND_SEQ)
					channel->reply_ack(channel, pkseq, &channel->to_addr);
				packet = (NetPacket_t*)malloc(sizeof(NetPacket_t) + decode_result.bodylen);
				if (!packet)
					return -1;
				memset(packet, 0, sizeof(*packet));
				packet->type = pktype;
				packet->seq = pkseq;
				packet->bodylen = decode_result.bodylen;
				memcpy(packet->buf, decode_result.bodyptr, packet->bodylen);
				packet->buf[packet->bodylen] = 0;
				if (streamtransportctxCacheRecvPacket(&channel->io->stream.ctx, packet)) {
					List_t list;
					while (streamtransportctxMergeRecvPacket(&channel->io->stream.ctx, &list)) {
						if (!channel_merge_packet_handler(channel, &list, timestamp_msec, err, &decode_result))
							return -1;
					}
				}
			}
			else if (NETPACKET_ACK == pktype) {
				NetPacket_t* ackpk = NULL;
				if (!streamtransportctxAckSendPacket(&channel->io->stream.ctx, pkseq, &ackpk))
					return -1;
				free(ackpk);
				if (streamtransportctxAllSendPacketIsAcked(&channel->io->stream.ctx)) {
					NetPacket_t* packet = channel->finpacket;
					if (packet) {
						int res = reactorobjectSendStreamData(channel->io, packet->buf, packet->hdrlen + packet->bodylen, packet->type);
						if (res < 0)
							return -1;
						free(channel->finpacket);
						channel->finpacket = NULL;
					}
				}
			}
			else if (pktype >= NETPACKET_STREAM_HAS_SEND_SEQ)
				channel->reply_ack(channel, pkseq, &channel->to_addr);
			else
				return -1;
		}
		else
			channel->recv(channel, &channel->to_addr, &decode_result);
	}
	channel->m_heartbeat_times = 0;
	channel_set_heartbeat_timestamp(channel, timestamp_msec);
	return off;
}

static int channel_dgram_listener_handler(Channel_t* channel, unsigned char* buf, int len, long long timestamp_msec, const void* from_saddr) {
	ChannelInbufDecodeResult_t decode_result;
	unsigned char pktype;
	memset(&decode_result, 0, sizeof(decode_result));
	channel->decode(channel, buf, len, &decode_result);
	if (decode_result.err)
		return 1;
	else if (decode_result.incomplete)
		return 1;
	pktype = decode_result.pktype;
	if (NETPACKET_SYN == pktype) {
		DgramHalfConn_t* halfconn;
		ListNode_t* cur;
		for (cur = channel->dgram.ctx.recvpacketlist.head; cur; cur = cur->next) {
			halfconn = pod_container_of(cur, DgramHalfConn_t, node);
			if (sockaddrIsEqual(&halfconn->from_addr, from_saddr)) {
				socketWrite(channel->io->fd, halfconn->buf, halfconn->len, 0, &halfconn->from_addr, sockaddrLength(&halfconn->from_addr));
				break;
			}
		}
		if (!cur) {
			FD_t new_sockfd = INVALID_FD_HANDLE;
			halfconn = NULL;
			do {
				unsigned short local_port;
				Sockaddr_t local_saddr;
				unsigned int buflen;
				memcpy(&local_saddr, &channel->dgram.listen_addr, sockaddrLength(&channel->dgram.listen_addr));
				if (!sockaddrSetPort(&local_saddr.st, 0))
					break;
				new_sockfd = socket(channel->io->domain, channel->io->socktype, channel->io->protocol);
				if (INVALID_FD_HANDLE == new_sockfd)
					break;
				if (!socketBindAddr(new_sockfd, &local_saddr.sa, sockaddrLength(&local_saddr.sa)))
					break;
				if (!socketGetLocalAddr(new_sockfd, &local_saddr.st))
					break;
				if (!sockaddrDecode(&local_saddr.st, NULL, &local_port))
					break;
				if (!socketNonBlock(new_sockfd, TRUE))
					break;
				buflen = channel->hdrsize(channel, sizeof(local_port)) + sizeof(local_port);
				halfconn = (DgramHalfConn_t*)malloc(sizeof(DgramHalfConn_t) + buflen);
				if (!halfconn)
					break;
				halfconn->sockfd = new_sockfd;
				memcpy(&halfconn->from_addr, from_saddr, sockaddrLength(from_saddr));
				halfconn->local_port = local_port;
				halfconn->resend_times = 0;
				halfconn->resend_msec = timestamp_msec + channel->dgram.rto;
				halfconn->len = buflen;
				listInsertNodeBack(&channel->dgram.ctx.recvpacketlist, channel->dgram.ctx.recvpacketlist.tail, &halfconn->node);
				channel_set_timestamp(channel, halfconn->resend_msec);
				channel->encode(channel, halfconn->buf, sizeof(local_port), NETPACKET_SYN_ACK, 0);
				*(unsigned short*)(halfconn->buf + buflen - sizeof(local_port)) = htons(local_port);
				socketWrite(channel->io->fd, halfconn->buf, halfconn->len, 0, &halfconn->from_addr, sockaddrLength(&halfconn->from_addr));
			} while (0);
			if (!halfconn) {
				free(halfconn);
				if (INVALID_FD_HANDLE != new_sockfd)
					socketClose(new_sockfd);
				return 1;
			}
		}
	}
	else if (NETPACKET_ACK == pktype) {
		ListNode_t* cur, *next;
		for (cur = channel->dgram.ctx.recvpacketlist.head; cur; cur = next) {
			Sockaddr_t addr;
			DgramHalfConn_t* halfconn = pod_container_of(cur, DgramHalfConn_t, node);
			next = cur->next;
			if (!sockaddrIsEqual(&halfconn->from_addr, from_saddr))
				continue;
			if (socketRead(halfconn->sockfd, NULL, 0, 0, &addr.st))
				continue;
			channel->ack_halfconn(channel, halfconn->sockfd, &addr, timestamp_msec);
			listRemoveNode(&channel->dgram.ctx.recvpacketlist, cur);
			halfconn->sockfd = INVALID_FD_HANDLE;
			free_halfconn(halfconn);
		}
	}
	channel->m_heartbeat_times = 0;
	channel_set_heartbeat_timestamp(channel, timestamp_msec);
	return 1;
}

static int channel_dgram_recv_handler(Channel_t* channel, unsigned char* buf, int len, long long timestamp_msec, const void* from_saddr, int* err) {
	ChannelInbufDecodeResult_t decode_result;
	if (channel->flag & CHANNEL_FLAG_RELIABLE) {
		NetPacket_t* packet;
		unsigned char pktype;
		unsigned int pkseq;

		int from_peer = sockaddrIsEqual(&channel->to_addr, from_saddr);
		int from_listen;
		if (channel->flag & CHANNEL_FLAG_CLIENT)
			from_listen = sockaddrIsEqual(&channel->dgram.connect_addr, from_saddr);
		else
			from_listen = 0;
		if (!from_peer && !from_listen)
			return 1;

		memset(&decode_result, 0, sizeof(decode_result));
		channel->decode(channel, buf, len, &decode_result);
		if (decode_result.err)
			return 1;
		else if (decode_result.incomplete)
			return 1;
		pktype = decode_result.pktype;
		pkseq = decode_result.pkseq;
		if (dgramtransportctxRecvCheck(&channel->dgram.ctx, pkseq, pktype)) {
			channel->reply_ack(channel, pkseq, from_saddr);
			packet = (NetPacket_t*)malloc(sizeof(NetPacket_t) + decode_result.bodylen);
			if (!packet)
				return 0;
			memset(packet, 0, sizeof(*packet));
			packet->type = pktype;
			packet->seq = pkseq;
			packet->bodylen = decode_result.bodylen;
			memcpy(packet->buf, decode_result.bodyptr, packet->bodylen);
			packet->buf[packet->bodylen] = 0;
			if (dgramtransportctxCacheRecvPacket(&channel->dgram.ctx, packet)) {
				List_t list;
				while (dgramtransportctxMergeRecvPacket(&channel->dgram.ctx, &list)) {
					if (!channel_merge_packet_handler(channel, &list, timestamp_msec, err, &decode_result))
						return 0;
				}
			}
		}
		else if (NETPACKET_SYN_ACK == pktype) {
			if (!from_listen)
				return 1;
			if (decode_result.bodylen < sizeof(unsigned short))
				return 1;
			if (channel->dgram.synpacket) {
				unsigned short port = *(unsigned short*)decode_result.bodyptr;
				port = ntohs(port);
				sockaddrSetPort(&channel->to_addr.st, port);
				free(channel->dgram.synpacket);
				channel->dgram.synpacket = NULL;
				channel->reg(channel, 0, timestamp_msec);
			}
			channel->reply_ack(channel, 0, from_saddr);
			socketWrite(channel->io->fd, NULL, 0, 0, &channel->to_addr, sockaddrLength(&channel->to_addr));
		}
		else if (NETPACKET_ACK == pktype) {
			ListNode_t* cur;
			int cwndskip;
			packet = dgramtransportctxAckSendPacket(&channel->dgram.ctx, pkseq, &cwndskip);
			if (!packet)
				return 1;
			if (packet == channel->finpacket) {
				channel->finpacket = NULL;
				if (channel->dgram.has_recvfin)
					*err = CHANNEL_INACTIVE_NORMAL;
			}
			free(packet);
			if (cwndskip) {
				for (cur = channel->dgram.ctx.sendpacketlist.head; cur; cur = cur->next) {
					packet = pod_container_of(cur, NetPacket_t, node);
					if (!dgramtransportctxSendWindowHasPacket(&channel->dgram.ctx, packet))
						break;
					if (NETPACKET_FIN == packet->type)
						channel->dgram.has_sendfin = 1;
					if (packet->wait_ack && packet->resend_msec > timestamp_msec)
						continue;
					packet->wait_ack = 1;
					packet->resend_msec = timestamp_msec + channel->dgram.rto;
					channel_set_timestamp(channel, packet->resend_msec);
					socketWrite(channel->io->fd, packet->buf, packet->hdrlen + packet->bodylen, 0, &channel->to_addr, sockaddrLength(&channel->to_addr));
				}
			}
		}
		else if (NETPACKET_NO_ACK_FRAGMENT_EOF == pktype)
			channel->recv(channel, from_saddr, &decode_result);
		else if (pktype >= NETPACKET_DGRAM_HAS_SEND_SEQ)
			channel->reply_ack(channel, pkseq, from_saddr);
	}
	else {
		memset(&decode_result, 0, sizeof(decode_result));
		channel->decode(channel, buf, len, &decode_result);
		if (decode_result.err)
			return 0;
		else if (decode_result.incomplete)
			return 1;
		channel->recv(channel, from_saddr, &decode_result);
	}
	channel->m_heartbeat_times = 0;
	channel_set_heartbeat_timestamp(channel, timestamp_msec);
	return 1;
}

static int reactorobject_recv_handler(ReactorObject_t* o, unsigned char* buf, unsigned int len, unsigned int off, long long timestamp_msec, const void* from_addr) {
	Channel_t* channel;
	if (!o->channel_list.head)
		return -1;
	else if (SOCK_STREAM == o->socktype) {
		int err = 0;
		channel = pod_container_of(o->channel_list.head, Channel_t, node);
		off = channel_stream_recv_handler(channel, buf, len, off, timestamp_msec, &err);
		if (off < 0)
			err = CHANNEL_INACTIVE_FATE;
		if (err)
			channel_detach_handler(channel, err, timestamp_msec);
		return off;
	}
	else {
		ListNode_t* cur, *next;
		for (cur = o->channel_list.head; cur; cur = next) {
			channel = pod_container_of(cur, Channel_t, node);
			next = cur->next;
			if (channel->flag & CHANNEL_FLAG_LISTEN)
				channel_dgram_listener_handler(channel, buf, len, timestamp_msec, from_addr);
			else {
				int err = 0;
				if (!channel_dgram_recv_handler(channel, buf, len, timestamp_msec, from_addr, &err))
					err = CHANNEL_INACTIVE_FATE;
				if (err)
					channel_detach_handler(channel, err, timestamp_msec);
			}
		}
		return 1;
	}
}

static int reactorobject_sendpacket_hook(ReactorObject_t* o, NetPacket_t* packet, long long timestamp_msec) {
	Channel_t* channel = (Channel_t*)packet->channel_object;
	if (SOCK_STREAM == o->socktype) {
		if (channel->flag & CHANNEL_FLAG_RELIABLE)
			packet->seq = streamtransportctxNextSendSeq(&o->stream.ctx, packet->type);
		if (packet->hdrlen)
			channel->encode(channel, packet->buf, packet->bodylen, packet->type, packet->seq);
		if (NETPACKET_FIN != packet->type)
			return 1;
		return streamtransportctxAllSendPacketIsAcked(&o->stream.ctx);
	}
	else {
		if (channel->flag & CHANNEL_FLAG_RELIABLE) {
			packet->seq = dgramtransportctxNextSendSeq(&channel->dgram.ctx, packet->type);
			if (packet->hdrlen)
				channel->encode(channel, packet->buf, packet->bodylen, packet->type, packet->seq);
			if (dgramtransportctxCacheSendPacket(&channel->dgram.ctx, packet)) {
				if (!dgramtransportctxSendWindowHasPacket(&channel->dgram.ctx, packet))
					return 0;
				if (NETPACKET_FIN == packet->type)
					channel->dgram.has_sendfin = 1;
				packet->wait_ack = 1;
				packet->resend_msec = timestamp_msec + channel->dgram.rto;
				channel_set_timestamp(channel, packet->resend_msec);
			}
			else if (NETPACKET_SYN == packet->type) {
				packet->wait_ack = 1;
				packet->resend_msec = timestamp_msec + channel->dgram.rto;
				channel_set_timestamp(channel, packet->resend_msec);
			}
			socketWrite(o->fd, packet->buf, packet->hdrlen + packet->bodylen, 0, &channel->to_addr, sockaddrLength(&channel->to_addr));
		}
		else {
			if (packet->hdrlen)
				channel->encode(channel, packet->buf, packet->bodylen, packet->type, packet->seq);
			socketWrite(o->fd, packet->buf, packet->hdrlen + packet->bodylen, 0, &channel->to_addr, sockaddrLength(&channel->to_addr));
		}
		return 0;
	}
}

int channel_event_handler(Channel_t* channel, long long timestamp_msec) {
/* heartbeat */
	if (channel->flag & CHANNEL_FLAG_CLIENT) {
		if (channel->heartbeat_msec > 0 &&
			channel->heartbeat_timeout_sec > 0 &&
			channel->heartbeat)
		{
			long long ts = channel->heartbeat_timeout_sec;
			ts *= 1000;
			ts += channel->heartbeat_msec;
			if (ts <= timestamp_msec) {
				if (channel->m_heartbeat_times < channel->heartbeat_maxtimes) {
					channel->heartbeat(channel, channel->m_heartbeat_times);
					channel->m_heartbeat_times++;
				}
				else if (channel->heartbeat(channel, channel->m_heartbeat_times))
					channel->m_heartbeat_times = 0;
				else
					return CHANNEL_INACTIVE_ZOMBIE;
				channel->heartbeat_msec = timestamp_msec;
				ts = channel->heartbeat_timeout_sec;
				ts *= 1000;
				ts += timestamp_msec;
			}
			channel_set_timestamp(channel, ts);
		}
	}
	else if (channel->heartbeat_timeout_sec > 0) {
		long long ts = channel->heartbeat_timeout_sec;
		ts *= 1000;
		ts += channel->heartbeat_msec;
		if (ts <= timestamp_msec)
			return CHANNEL_INACTIVE_ZOMBIE;
		channel_set_timestamp(channel, ts);
	}
/* reliable dgram resend packet */
	if ((channel->flag & CHANNEL_FLAG_DGRAM) &&
		(channel->flag & CHANNEL_FLAG_RELIABLE))
	{
		if (channel->flag & CHANNEL_FLAG_LISTEN) {
			ListNode_t* cur, *next;
			for (cur = channel->dgram.ctx.recvpacketlist.head; cur; cur = next) {
				DgramHalfConn_t* halfconn = pod_container_of(cur, DgramHalfConn_t, node);
				next = cur->next;
				if (halfconn->resend_msec > timestamp_msec) {
					channel_set_timestamp(channel, halfconn->resend_msec);
					continue;
				}
				if (halfconn->resend_times >= channel->dgram.resend_maxtimes) {
					listRemoveNode(&channel->dgram.ctx.recvpacketlist, cur);
					free_halfconn(halfconn);
					continue;
				}
				socketWrite(channel->io->fd, halfconn->buf, halfconn->len, 0, &halfconn->from_addr, sockaddrLength(&halfconn->from_addr));
				halfconn->resend_times++;
				halfconn->resend_msec = timestamp_msec + channel->dgram.rto;
				channel_set_timestamp(channel, halfconn->resend_msec);
			}
		}
		else if (channel->dgram.synpacket) {
			NetPacket_t* packet = channel->dgram.synpacket;
			if (packet->resend_msec > timestamp_msec)
				channel_set_timestamp(channel, packet->resend_msec);
			else if (packet->resend_times >= channel->dgram.resend_maxtimes) {
				free(channel->dgram.synpacket);
				channel->dgram.synpacket = NULL;
				channel->reg(channel, ETIMEDOUT, timestamp_msec);
				return CHANNEL_INACTIVE_UNSENT;
			}
			else {
				socketWrite(channel->io->fd, packet->buf, packet->hdrlen + packet->bodylen, 0, &channel->dgram.connect_addr, sockaddrLength(&channel->dgram.connect_addr));
				packet->resend_times++;
				packet->resend_msec = timestamp_msec + channel->dgram.rto;
				channel_set_timestamp(channel, packet->resend_msec);
			}
		}
		else {
			ListNode_t* cur;
			for (cur = channel->dgram.ctx.sendpacketlist.head; cur; cur = cur->next) {
				NetPacket_t* packet = pod_container_of(cur, NetPacket_t, node);
				if (!packet->wait_ack)
					break;
				if (packet->resend_msec > timestamp_msec) {
					channel_set_timestamp(channel, packet->resend_msec);
					continue;
				}
				if (packet->resend_times >= channel->dgram.resend_maxtimes)
					return NETPACKET_FIN != packet->type ? CHANNEL_INACTIVE_UNSENT : CHANNEL_INACTIVE_NORMAL;
				socketWrite(channel->io->fd, packet->buf, packet->hdrlen + packet->bodylen, 0, &channel->to_addr, sockaddrLength(&channel->to_addr));
				packet->resend_times++;
				packet->resend_msec = timestamp_msec + channel->dgram.rto;
				channel_set_timestamp(channel, packet->resend_msec);
			}
		}
	}
	return 0;
}

static void reactorobject_reg_handler(ReactorObject_t* o, int err, long long timestamp_msec) {
	ListNode_t* cur, *next;
	for (cur = o->channel_list.head; cur; cur = next) {
		Channel_t* channel = pod_container_of(cur, Channel_t, node);
		next = cur->next;
		if (!err &&
			(channel->flag & CHANNEL_FLAG_DGRAM) &&
			(channel->flag & CHANNEL_FLAG_RELIABLE) &&
			(channel->flag & CHANNEL_FLAG_CLIENT))
		{
			unsigned int hdrsize = channel->hdrsize(channel, 0);
			NetPacket_t* packet = (NetPacket_t*)malloc(sizeof(NetPacket_t) + hdrsize);
			if (packet) {
				memset(packet, 0, sizeof(*packet));
				packet->channel_object = channel;
				packet->type = NETPACKET_SYN;
				packet->hdrlen = hdrsize;
				packet->bodylen = 0;
				channel->dgram.synpacket = packet;
				reactorobjectSendPacket(channel->io, packet);
				continue;
			}
			err = errnoGet();
		}
		channel->reg(channel, err, timestamp_msec);
		if (err) {
			listRemoveNode(&o->channel_list, cur);
			channelDestroy(channel);
		}
		else
			channel_set_heartbeat_timestamp(channel, timestamp_msec);
	}
}

static void reactorobject_exec_channel(ReactorObject_t* o, long long timestamp_msec, long long ev_msec) {
	ListNode_t* cur, *next;
	for (cur = o->channel_list.head; cur; cur = next) {
		int inactive_reason;
		Channel_t* channel = pod_container_of(cur, Channel_t, node);
		next = cur->next;
		if (ev_msec < channel->m_event_msec)
			continue;
		channel->m_event_msec = 0;
		inactive_reason = channel_event_handler(channel, timestamp_msec);
		if (!inactive_reason)
			continue;
		channel_detach_handler(channel, inactive_reason, timestamp_msec);
	}
	if (!o->channel_list.head)
		reactorobjectInvalid(o, timestamp_msec);
}

static int channel_shared_data(Channel_t* channel, const Iobuf_t iov[], unsigned int iovcnt, int no_ack, List_t* packetlist) {
	unsigned int i, nbytes = 0;
	NetPacket_t* packet;
	for (i = 0; i < iovcnt; ++i)
		nbytes += iobufLen(iov + i);
	listInit(packetlist);
	if (!(channel->flag & CHANNEL_FLAG_RELIABLE))
		no_ack = 1;
	if (nbytes) {
		ListNode_t* cur;
		unsigned int off, iov_i, iov_off;
		unsigned int sharedsize = channel->io->write_fragment_size - channel->maxhdrsize;
		unsigned int sharedcnt = nbytes / sharedsize + (nbytes % sharedsize != 0);
		if (sharedcnt > 1 && no_ack && SOCK_DGRAM == channel->io->socktype)
			return 0;
		packet = NULL;
		for (off = i = 0; i < sharedcnt; ++i) {
			unsigned int memsz = nbytes - off > sharedsize ? sharedsize : nbytes - off;
			unsigned int hdrsize = channel->hdrsize(channel, memsz);
			packet = (NetPacket_t*)malloc(sizeof(NetPacket_t) + hdrsize + memsz);
			if (!packet)
				break;
			memset(packet, 0, sizeof(*packet));
			packet->channel_object = channel;
			packet->type = no_ack ? NETPACKET_NO_ACK_FRAGMENT : NETPACKET_FRAGMENT;
			packet->hdrlen = hdrsize;
			packet->bodylen = memsz;
			listInsertNodeBack(packetlist, packetlist->tail, &packet->node._);
			off += memsz;
		}
		if (packet) {
			packet->type = no_ack ? NETPACKET_NO_ACK_FRAGMENT_EOF : NETPACKET_FRAGMENT_EOF;
		}
		else {
			ListNode_t *next;
			for (cur = packetlist->head; cur; cur = next) {
				next = cur->next;
				free(pod_container_of(cur, NetPacket_t, node));
			}
			listInit(packetlist);
			return 0;
		}
		iov_i = iov_off = 0;
		for (cur = packetlist->head; cur; cur = cur->next) {
			packet = pod_container_of(cur, NetPacket_t, node);
			off = 0;
			while (iov_i < iovcnt) {
				unsigned int pkleftsize = packet->bodylen - off;
				unsigned int iovleftsize = iobufLen(iov + iov_i) - iov_off;
				if (iovleftsize > pkleftsize) {
					memcpy(packet->buf + packet->hdrlen + off, ((char*)iobufPtr(iov + iov_i)) + iov_off, pkleftsize);
					iov_off += pkleftsize;
					break;
				}
				else {
					memcpy(packet->buf + packet->hdrlen + off, ((char*)iobufPtr(iov + iov_i)) + iov_off, iovleftsize);
					iov_off = 0;
					iov_i++;
					off += iovleftsize;
				}
			}
		}
	}
	else {
		unsigned int hdrsize = channel->hdrsize(channel, 0);
		if (0 == hdrsize && SOCK_STREAM == channel->io->socktype)
			return 1;
		packet = (NetPacket_t*)malloc(sizeof(NetPacket_t) + hdrsize);
		if (!packet)
			return 0;
		memset(packet, 0, sizeof(*packet));
		packet->channel_object = channel;
		packet->type = no_ack ? NETPACKET_NO_ACK_FRAGMENT_EOF : NETPACKET_FRAGMENT_EOF;
		packet->hdrlen = hdrsize;
		listInsertNodeBack(packetlist, packetlist->tail, &packet->node._);
	}
	return 1;
}

void reactorobject_stream_accept(ReactorObject_t* o, FD_t newfd, const void* peeraddr, long long timestamp_msec) {
	Channel_t* channel = pod_container_of(o->channel_list.head, Channel_t, node);
	channel->ack_halfconn(channel, newfd, peeraddr, timestamp_msec);
}

static void reactorobject_stream_recvfin(ReactorObject_t* o, long long timestamp_msec) {
	Channel_t* channel;
	if (!o->channel_list.head)
		return;
	channel = pod_container_of(o->channel_list.head, Channel_t, node);
	if (!o->stream.has_sendfin) {
		channelSendFin(channel, timestamp_msec);
		return;
	}
	else {
		int reason;
		if (channel->flag & CHANNEL_FLAG_STREAM)
			reason = streamtransportctxAllSendPacketIsAcked(&o->stream.ctx) ? CHANNEL_INACTIVE_NORMAL : CHANNEL_INACTIVE_UNSENT;
		else
			reason = CHANNEL_INACTIVE_FATE;
		channel_detach_handler(channel, reason, timestamp_msec);
	}
}

static void reactorobject_stream_sendfin(ReactorObject_t* o, long long timestamp_msec) {
	Channel_t* channel;
	if (!o->channel_list.head)
		return;
	channel = pod_container_of(o->channel_list.head, Channel_t, node);
	if (!o->stream.has_recvfin)
		return;
	else {
		int reason;
		if (channel->flag & CHANNEL_FLAG_STREAM)
			reason = streamtransportctxAllSendPacketIsAcked(&o->stream.ctx) ? CHANNEL_INACTIVE_NORMAL : CHANNEL_INACTIVE_UNSENT;
		else
			reason = CHANNEL_INACTIVE_FATE;
		channel_detach_handler(channel, reason, timestamp_msec);
	}
}

/*******************************************************************************/

Channel_t* reactorobjectOpenChannel(ReactorObject_t* io, int flag, int initseq, const void* saddr) {
	Channel_t* channel;
	if (SOCK_STREAM == io->socktype && io->channel_list.head)
		return pod_container_of(io->channel_list.head, Channel_t, node);
	channel = (Channel_t*)calloc(1, sizeof(Channel_t));
	if (!channel)
		return NULL;
	channel->io = io;
	flag &= ~(CHANNEL_FLAG_DGRAM | CHANNEL_FLAG_STREAM);
	flag |= (SOCK_STREAM == io->socktype) ? CHANNEL_FLAG_STREAM : CHANNEL_FLAG_DGRAM;
	if (flag & CHANNEL_FLAG_DGRAM) {
		if (flag & CHANNEL_FLAG_LISTEN) {
			flag |= CHANNEL_FLAG_RELIABLE;
			memcpy(&channel->dgram.listen_addr, saddr, sockaddrLength(saddr));
		}
		else if (flag & CHANNEL_FLAG_CLIENT)
			memcpy(&channel->dgram.connect_addr, saddr, sockaddrLength(saddr));
		channel->dgram.rto = 200;
		channel->dgram.resend_maxtimes = 5;
		dgramtransportctxInit(&channel->dgram.ctx, initseq);
	}
	else {
		streamtransportctxInit(&channel->io->stream.ctx, initseq);
		if (flag & CHANNEL_FLAG_LISTEN)
			io->stream.accept = reactorobject_stream_accept;
		else if (flag & CHANNEL_FLAG_CLIENT)
			memcpy(&io->stream.connect_addr, saddr, sockaddrLength(saddr));
		io->stream.recvfin = reactorobject_stream_recvfin;
		io->stream.sendfin = reactorobject_stream_sendfin;
	}
	channel->flag = flag;
	channel->inactivecmd.type = REACTOR_CHANNEL_INACTIVE_CMD;
	memcpy(&channel->to_addr, saddr, sockaddrLength(saddr));
	io->reg = reactorobject_reg_handler;
	io->exec = reactorobject_exec_channel;
	io->preread = reactorobject_recv_handler;
	io->sendpacket_hook = reactorobject_sendpacket_hook;
	listInsertNodeBack(&io->channel_list, io->channel_list.tail, &channel->node._);
	return channel;
}

void channelSendFin(Channel_t* channel, long long timestamp_msec) {
	if (_xchg8(&channel->m_ban_send, 1))
		return;
	else if (channel->flag & CHANNEL_FLAG_RELIABLE) {
		NetPacket_t* packet = channel->finpacket;
		if (!packet) {
			unsigned int hdrsize = channel->hdrsize(channel, 0);
			packet = (NetPacket_t*)malloc(sizeof(NetPacket_t) + hdrsize);
			if (!packet)
				return;
			memset(packet, 0, sizeof(*packet));
			packet->type = NETPACKET_FIN;
			packet->hdrlen = hdrsize;
			packet->bodylen = 0;
			channel->finpacket = packet;
		}
		packet->channel_object = channel;
		reactorobjectSendPacket(channel->io, packet);
	}
	else if (channel->flag & CHANNEL_FLAG_STREAM) {
		reactorCommitCmd(channel->io->reactor, &channel->io->stream.sendfincmd);
	}
}

Channel_t* channelSend(Channel_t* channel, const void* data, unsigned int len, int no_ack) {
	Iobuf_t iov = iobufStaticInit(data, len);
	return channelSendv(channel, &iov, 1, no_ack);
}

Channel_t* channelSendv(Channel_t* channel, const Iobuf_t iov[], unsigned int iovcnt, int no_ack) {
	List_t packetlist;
	if (channel->m_ban_send)
		return NULL;
	if (!channel_shared_data(channel, iov, iovcnt, no_ack, &packetlist))
		return NULL;
	reactorobjectSendPacketList(channel->io, &packetlist);
	return channel;
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
		channel_free_packetlist(&channel->io->stream.ctx.recvpacketlist);
		channel_free_packetlist(&channel->io->stream.ctx.sendpacketlist);
	}
	else if (channel->flag & CHANNEL_FLAG_LISTEN) {
		ListNode_t* cur, *next;
		for (cur = channel->dgram.ctx.recvpacketlist.head; cur; cur = next) {
			next = cur->next;
			free_halfconn(pod_container_of(cur, DgramHalfConn_t, node));
		}
		listInit(&channel->dgram.ctx.recvpacketlist);
	}
	else {
		free(channel->dgram.synpacket);
		channel->dgram.synpacket = NULL;
		channel_free_packetlist(&channel->dgram.ctx.recvpacketlist);
		channel_free_packetlist(&channel->dgram.ctx.sendpacketlist);
	}
	free(channel);
}

#ifdef	__cplusplus
}
#endif
