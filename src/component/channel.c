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

static void channel_invalid(ChannelBase_t* base, int detach_error) {
	base->valid = 0;
	base->detach_error = detach_error;
}

static void free_halfconn(DgramHalfConn_t* halfconn) {
	if (INVALID_FD_HANDLE != halfconn->sockfd)
		socketClose(halfconn->sockfd);
	free(halfconn);
}

static void channel_set_timestamp(Channel_t* channel, long long timestamp_msec) {
	if (timestamp_msec <= 0)
		return;
	if (channel->_.event_msec <= 0 || channel->_.event_msec > timestamp_msec)
		channel->_.event_msec = timestamp_msec;
}

static void channel_set_heartbeat_timestamp(Channel_t* channel, long long timestamp_msec) {
	if (timestamp_msec <= 0)
		return;
	channel->m_heartbeat_msec = timestamp_msec;
	if (channel->heartbeat_timeout_sec > 0) {
		long long ts = channel->heartbeat_timeout_sec;
		ts *= 1000;
		ts += channel->m_heartbeat_msec;
		channel_set_timestamp(channel, ts);
	}
}

static unsigned char* merge_packet(List_t* list, unsigned int* mergelen) {
	unsigned char* ptr;
	ReactorPacket_t* packet;
	ListNode_t* cur, *next;
	unsigned int off;
	off = 0;
	for (cur = list->head; cur; cur = next) {
		packet = pod_container_of(cur, ReactorPacket_t, _.node);
		next = cur->next;
		off += packet->_.bodylen;
	}
	ptr = (unsigned char*)malloc(off);
	if (ptr) {
		*mergelen = off;
		off = 0;
		for (cur = list->head; cur; cur = next) {
			packet = pod_container_of(cur, ReactorPacket_t, _.node);
			next = cur->next;
			memcpy(ptr + off, packet->_.buf + packet->_.hdrlen, packet->_.bodylen);
			off += packet->_.bodylen;
			free(packet);
		}
	}
	return ptr;
}

static int channel_merge_packet_handler(Channel_t* channel, List_t* packetlist, long long timestamp_msec, ChannelInbufDecodeResult_t* decode_result) {
	ReactorPacket_t* packet = pod_container_of(packetlist->tail, ReactorPacket_t, _.node);
	if (NETPACKET_FIN == packet->_.type) {
		ListNode_t* cur, *next;
		for (cur = packetlist->head; cur; cur = next) {
			next = cur->next;
			free(pod_container_of(cur, ReactorPacket_t, _.node));
		}
		if (channel->_.has_recvfin)
			return 1;
		channel->_.has_recvfin = 1;
		if (channel->_.has_sendfin)
			return 1;
		channelSendFin(channel, timestamp_msec);
	}
	else {
		decode_result->bodyptr = merge_packet(packetlist, &decode_result->bodylen);
		if (!decode_result->bodyptr)
			return 0;
		channel->on_recv(channel, &channel->_.to_addr, decode_result);
		free(decode_result->bodyptr);
	}
	return 1;
}

static int channel_stream_recv_handler(Channel_t* channel, unsigned char* buf, int len, int off, long long timestamp_msec) {
	ChannelInbufDecodeResult_t decode_result;
	while (off < len) {
		memset(&decode_result, 0, sizeof(decode_result));
		channel->on_decode(channel, buf + off, len - off, &decode_result);
		if (decode_result.err)
			return -1;
		else if (decode_result.incomplete)
			return off;
		off += decode_result.decodelen;
		if (channel->flag & CHANNEL_FLAG_RELIABLE) {
			unsigned char pktype = decode_result.pktype;
			unsigned int pkseq = decode_result.pkseq;
			StreamTransportCtx_t* ctx = &channel->_.stream_ctx;
			if (streamtransportctxRecvCheck(ctx, pkseq, pktype)) {
				ReactorPacket_t* packet;
				if (pktype >= NETPACKET_STREAM_HAS_SEND_SEQ)
					channel->on_reply_ack(channel, pkseq, &channel->_.to_addr);
				packet = (ReactorPacket_t*)malloc(sizeof(ReactorPacket_t) + decode_result.bodylen);
				if (!packet)
					return -1;
				memset(packet, 0, sizeof(*packet));
				packet->_.type = pktype;
				packet->_.seq = pkseq;
				packet->_.bodylen = decode_result.bodylen;
				memcpy(packet->_.buf, decode_result.bodyptr, packet->_.bodylen);
				packet->_.buf[packet->_.bodylen] = 0;
				if (streamtransportctxCacheRecvPacket(ctx, &packet->_)) {
					List_t list;
					while (streamtransportctxMergeRecvPacket(ctx, &list)) {
						if (!channel_merge_packet_handler(channel, &list, timestamp_msec, &decode_result))
							return -1;
					}
				}
			}
			else if (NETPACKET_ACK == pktype) {
				NetPacket_t* ackpk = NULL;
				if (!streamtransportctxAckSendPacket(ctx, pkseq, &ackpk))
					return -1;
				free(pod_container_of(ackpk, ReactorPacket_t, _));
			}
			else if (pktype >= NETPACKET_STREAM_HAS_SEND_SEQ)
				channel->on_reply_ack(channel, pkseq, &channel->_.to_addr);
			else
				return -1;
		}
		else
			channel->on_recv(channel, &channel->_.to_addr, &decode_result);
	}
	channel->m_heartbeat_times = 0;
	channel_set_heartbeat_timestamp(channel, timestamp_msec);
	return off;
}

static int channel_dgram_listener_handler(Channel_t* channel, unsigned char* buf, int len, long long timestamp_msec, const void* from_saddr) {
	ChannelInbufDecodeResult_t decode_result;
	unsigned char pktype;
	memset(&decode_result, 0, sizeof(decode_result));
	channel->on_decode(channel, buf, len, &decode_result);
	if (decode_result.err)
		return 1;
	else if (decode_result.incomplete)
		return 1;
	pktype = decode_result.pktype;
	if (NETPACKET_SYN == pktype) {
		DgramHalfConn_t* halfconn = NULL;
		ListNode_t* cur;
		for (cur = channel->dgram.ctx.recvpacketlist.head; cur; cur = cur->next) {
			halfconn = pod_container_of(cur, DgramHalfConn_t, node);
			if (sockaddrIsEqual(&halfconn->from_addr, from_saddr))
				break;
		}
		if (cur) {
			socketWrite(channel->_.o->fd, halfconn->buf, halfconn->len, 0, &halfconn->from_addr, sockaddrLength(&halfconn->from_addr));
		}
		else if (channel->dgram.m_halfconn_curwaitcnt >= channel->dgram.halfconn_maxwaitcnt) {
			/* TODO return rst, now let client syn timeout */
		}
		else {
			FD_t new_sockfd = INVALID_FD_HANDLE;
			halfconn = NULL;
			do {
				ReactorObject_t* o;
				unsigned short local_port;
				Sockaddr_t local_saddr;
				unsigned int buflen;
				memcpy(&local_saddr, &channel->_.listen_addr, sockaddrLength(&channel->_.listen_addr));
				if (!sockaddrSetPort(&local_saddr.st, 0))
					break;
				o = channel->_.o;
				new_sockfd = socket(o->domain, o->socktype, o->protocol);
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
				buflen = channel->on_hdrsize(channel, sizeof(local_port)) + sizeof(local_port);
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
				channel->dgram.m_halfconn_curwaitcnt++;
				channel_set_timestamp(channel, halfconn->resend_msec);
				channel->on_encode(channel, halfconn->buf, sizeof(local_port), NETPACKET_SYN_ACK, 0);
				*(unsigned short*)(halfconn->buf + buflen - sizeof(local_port)) = htons(local_port);
				socketWrite(o->fd, halfconn->buf, halfconn->len, 0, &halfconn->from_addr, sockaddrLength(&halfconn->from_addr));
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
			FD_t connfd;
			DgramHalfConn_t* halfconn = pod_container_of(cur, DgramHalfConn_t, node);
			next = cur->next;
			if (!sockaddrIsEqual(&halfconn->from_addr, from_saddr))
				continue;
			connfd = halfconn->sockfd;
			if (socketRead(connfd, NULL, 0, 0, &addr.st))
				continue;
			listRemoveNode(&channel->dgram.ctx.recvpacketlist, cur);
			channel->dgram.m_halfconn_curwaitcnt--;
			halfconn->sockfd = INVALID_FD_HANDLE;
			free_halfconn(halfconn);
			channel->_.on_ack_halfconn(&channel->_, connfd, &addr, timestamp_msec);
		}
	}
	channel->m_heartbeat_times = 0;
	channel_set_heartbeat_timestamp(channel, timestamp_msec);
	return 1;
}

static int channel_dgram_recv_handler(Channel_t* channel, unsigned char* buf, int len, long long timestamp_msec, const void* from_saddr) {
	ChannelInbufDecodeResult_t decode_result;
	if (channel->flag & CHANNEL_FLAG_RELIABLE) {
		ReactorPacket_t* packet;
		unsigned char pktype;
		unsigned int pkseq;

		int from_listen, from_peer;
		if (channel->flag & CHANNEL_FLAG_CLIENT)
			from_listen = sockaddrIsEqual(&channel->_.connect_addr, from_saddr);
		else
			from_listen = 0;
		if (from_listen)
			from_peer = 0;
		else
			from_peer = sockaddrIsEqual(&channel->_.to_addr, from_saddr);
		if (!from_peer && !from_listen)
			return 1;

		memset(&decode_result, 0, sizeof(decode_result));
		channel->on_decode(channel, buf, len, &decode_result);
		if (decode_result.err)
			return 1;
		else if (decode_result.incomplete)
			return 1;
		pktype = decode_result.pktype;
		pkseq = decode_result.pkseq;
		if (from_listen) {
			if (NETPACKET_SYN_ACK != pktype)
				return 1;
			if (decode_result.bodylen < sizeof(unsigned short))
				return 1;
			if (channel->dgram.m_synpacket) {
				unsigned short port = *(unsigned short*)decode_result.bodyptr;
				port = ntohs(port);
				sockaddrSetPort(&channel->_.to_addr.st, port);
				free(channel->dgram.m_synpacket);
				channel->dgram.m_synpacket = NULL;
				channel->_.on_syn_ack(&channel->_, timestamp_msec);
			}
			channel->on_reply_ack(channel, 0, from_saddr);
			socketWrite(channel->_.o->fd, NULL, 0, 0, &channel->_.to_addr, sockaddrLength(&channel->_.to_addr));
		}
		else if (dgramtransportctxRecvCheck(&channel->dgram.ctx, pkseq, pktype)) {
			channel->on_reply_ack(channel, pkseq, from_saddr);
			packet = (ReactorPacket_t*)malloc(sizeof(ReactorPacket_t) + decode_result.bodylen);
			if (!packet)
				return 0;
			memset(packet, 0, sizeof(*packet));
			packet->_.type = pktype;
			packet->_.seq = pkseq;
			packet->_.bodylen = decode_result.bodylen;
			memcpy(packet->_.buf, decode_result.bodyptr, packet->_.bodylen);
			packet->_.buf[packet->_.bodylen] = 0;
			if (dgramtransportctxCacheRecvPacket(&channel->dgram.ctx, &packet->_)) {
				List_t list;
				while (dgramtransportctxMergeRecvPacket(&channel->dgram.ctx, &list)) {
					if (!channel_merge_packet_handler(channel, &list, timestamp_msec, &decode_result))
						return 0;
				}
			}
		}
		else if (NETPACKET_ACK == pktype) {
			ListNode_t* cur;
			int cwndskip;
			NetPacket_t* packetbase = dgramtransportctxAckSendPacket(&channel->dgram.ctx, pkseq, &cwndskip);
			if (!packetbase)
				return 1;
			free(pod_container_of(packetbase, ReactorPacket_t, _));
			if (cwndskip) {
				for (cur = channel->dgram.ctx.sendpacketlist.head; cur; cur = cur->next) {
					packet = pod_container_of(cur, ReactorPacket_t, _.node);
					if (!dgramtransportctxSendWindowHasPacket(&channel->dgram.ctx, &packet->_))
						break;
					if (NETPACKET_FIN == packet->_.type)
						channel->_.has_sendfin = 1;
					if (packet->_.wait_ack && packet->_.resend_msec > timestamp_msec)
						continue;
					packet->_.wait_ack = 1;
					packet->_.resend_msec = timestamp_msec + channel->dgram.rto;
					channel_set_timestamp(channel, packet->_.resend_msec);
					socketWrite(channel->_.o->fd, packet->_.buf, packet->_.hdrlen + packet->_.bodylen, 0,
						&channel->_.to_addr, sockaddrLength(&channel->_.to_addr));
				}
			}
		}
		else if (NETPACKET_NO_ACK_FRAGMENT_EOF == pktype)
			channel->on_recv(channel, from_saddr, &decode_result);
		else if (pktype >= NETPACKET_DGRAM_HAS_SEND_SEQ)
			channel->on_reply_ack(channel, pkseq, from_saddr);
	}
	else {
		memset(&decode_result, 0, sizeof(decode_result));
		channel->on_decode(channel, buf, len, &decode_result);
		if (decode_result.err)
			return 0;
		else if (decode_result.incomplete)
			return 1;
		channel->on_recv(channel, from_saddr, &decode_result);
	}
	channel->m_heartbeat_times = 0;
	channel_set_heartbeat_timestamp(channel, timestamp_msec);
	return 1;
}

static int on_read(ChannelBase_t* base, unsigned char* buf, unsigned int len, unsigned int off, long long timestamp_msec, const void* from_addr) {
	Channel_t* channel = pod_container_of(base, Channel_t, _);
	if (CHANNEL_FLAG_STREAM & channel->flag) {
		off = channel_stream_recv_handler(channel, buf, len, off, timestamp_msec);
		if (off < 0)
			channel_invalid(base, REACTOR_ONREAD_ERR);
		else if (base->has_recvfin && base->has_sendfin)
			channel_invalid(base, 0);
		return off;
	}
	else {
		if (channel->flag & CHANNEL_FLAG_LISTEN)
			channel_dgram_listener_handler(channel, buf, len, timestamp_msec, from_addr);
		else if (!channel_dgram_recv_handler(channel, buf, len, timestamp_msec, from_addr))
			channel_invalid(base, REACTOR_ONREAD_ERR);
		else if (base->has_recvfin && base->has_sendfin)
			channel_invalid(base, 0);
		return 1;
	}
}

static int on_pre_send(ChannelBase_t* base, ReactorPacket_t* packet, long long timestamp_msec) {
	Channel_t* channel = pod_container_of(base, Channel_t, _);
	if (CHANNEL_FLAG_STREAM & channel->flag) {
		if (channel->flag & CHANNEL_FLAG_RELIABLE)
			packet->_.seq = streamtransportctxNextSendSeq(&base->stream_ctx, packet->_.type);
		if (packet->_.hdrlen)
			channel->on_encode(channel, packet->_.buf, packet->_.bodylen, packet->_.type, packet->_.seq);
		return 1;
	}
	else {
		if (channel->flag & CHANNEL_FLAG_RELIABLE) {
			packet->_.seq = dgramtransportctxNextSendSeq(&channel->dgram.ctx, packet->_.type);
			if (packet->_.hdrlen)
				channel->on_encode(channel, packet->_.buf, packet->_.bodylen, packet->_.type, packet->_.seq);
			if (dgramtransportctxCacheSendPacket(&channel->dgram.ctx, &packet->_)) {
				if (!dgramtransportctxSendWindowHasPacket(&channel->dgram.ctx, &packet->_))
					return 0;
				if (NETPACKET_FIN == packet->_.type)
					channel->_.has_sendfin = 1;
				packet->_.wait_ack = 1;
				packet->_.resend_msec = timestamp_msec + channel->dgram.rto;
				channel_set_timestamp(channel, packet->_.resend_msec);
			}
		}
		else if (packet->_.hdrlen) {
			channel->on_encode(channel, packet->_.buf, packet->_.bodylen, packet->_.type, packet->_.seq);
		}
		return 1;
	}
}

static void on_exec(ChannelBase_t* base, long long timestamp_msec) {
	Channel_t* channel = pod_container_of(base, Channel_t, _);
/* heartbeat */
	if (channel->flag & CHANNEL_FLAG_CLIENT) {
		if (channel->m_heartbeat_msec > 0 &&
			channel->heartbeat_timeout_sec > 0 &&
			channel->on_heartbeat)
		{
			long long ts = channel->heartbeat_timeout_sec;
			ts *= 1000;
			ts += channel->m_heartbeat_msec;
			if (ts <= timestamp_msec) {
				if (channel->m_heartbeat_times < channel->heartbeat_maxtimes) {
					channel->on_heartbeat(channel, channel->m_heartbeat_times);
					channel->m_heartbeat_times++;
				}
				else if (channel->on_heartbeat(channel, channel->m_heartbeat_times))
					channel->m_heartbeat_times = 0;
				else {
					channel_invalid(base, REACTOR_ZOMBIE_ERR);
					return;
				}
				channel->m_heartbeat_msec = timestamp_msec;
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
		ts += channel->m_heartbeat_msec;
		if (ts <= timestamp_msec) {
			channel_invalid(base, REACTOR_ZOMBIE_ERR);
			return;
		}
		channel_set_timestamp(channel, ts);
	}
/* reliable dgram resend packet */
	if (!(channel->flag & CHANNEL_FLAG_DGRAM) ||
		!(channel->flag & CHANNEL_FLAG_RELIABLE))
	{
		return;
	}
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
				channel->dgram.m_halfconn_curwaitcnt--;
				free_halfconn(halfconn);
				continue;
			}
			socketWrite(channel->_.o->fd, halfconn->buf, halfconn->len, 0,
				&halfconn->from_addr, sockaddrLength(&halfconn->from_addr));
			halfconn->resend_times++;
			halfconn->resend_msec = timestamp_msec + channel->dgram.rto;
			channel_set_timestamp(channel, halfconn->resend_msec);
		}
	}
	else if (channel->dgram.m_synpacket) {
		ReactorPacket_t* packet = channel->dgram.m_synpacket;
		if (packet->_.resend_msec > timestamp_msec)
			channel_set_timestamp(channel, packet->_.resend_msec);
		else if (packet->_.resend_times >= channel->dgram.resend_maxtimes) {
			free(channel->dgram.m_synpacket);
			channel->dgram.m_synpacket = NULL;
			channel_invalid(base, REACTOR_CONNECT_ERR);
			return;
		}
		else {
			socketWrite(channel->_.o->fd, packet->_.buf, packet->_.hdrlen + packet->_.bodylen, 0,
				&channel->_.connect_addr, sockaddrLength(&channel->_.connect_addr));
			packet->_.resend_times++;
			packet->_.resend_msec = timestamp_msec + channel->dgram.rto;
			channel_set_timestamp(channel, packet->_.resend_msec);
		}
	}
	else {
		ListNode_t* cur;
		for (cur = channel->dgram.ctx.sendpacketlist.head; cur; cur = cur->next) {
			ReactorPacket_t* packet = pod_container_of(cur, ReactorPacket_t, _.node);
			if (!packet->_.wait_ack)
				break;
			if (packet->_.resend_msec > timestamp_msec) {
				channel_set_timestamp(channel, packet->_.resend_msec);
				continue;
			}
			if (packet->_.resend_times >= channel->dgram.resend_maxtimes) {
				int err = (NETPACKET_FIN != packet->_.type ? REACTOR_ZOMBIE_ERR : 0);
				channel_invalid(base, err);
				return;
			}
			socketWrite(channel->_.o->fd, packet->_.buf, packet->_.hdrlen + packet->_.bodylen, 0,
				&channel->_.to_addr, sockaddrLength(&channel->_.to_addr));
			packet->_.resend_times++;
			packet->_.resend_msec = timestamp_msec + channel->dgram.rto;
			channel_set_timestamp(channel, packet->_.resend_msec);
		}
	}
}

static int channel_shared_data(Channel_t* channel, const Iobuf_t iov[], unsigned int iovcnt, int no_ack, List_t* packetlist) {
	unsigned int i, nbytes = 0;
	ReactorPacket_t* packet;
	for (i = 0; i < iovcnt; ++i)
		nbytes += iobufLen(iov + i);
	listInit(packetlist);
	if (!(channel->flag & CHANNEL_FLAG_RELIABLE))
		no_ack = 1;
	if (nbytes) {
		ListNode_t* cur;
		unsigned int off, iov_i, iov_off;
		unsigned int sharedsize = channel->_.o->write_fragment_size - channel->maxhdrsize;
		unsigned int sharedcnt = nbytes / sharedsize + (nbytes % sharedsize != 0);
		if (sharedcnt > 1 && no_ack && (CHANNEL_FLAG_DGRAM & channel->flag))
			return 0;
		packet = NULL;
		for (off = i = 0; i < sharedcnt; ++i) {
			unsigned int memsz = nbytes - off > sharedsize ? sharedsize : nbytes - off;
			unsigned int hdrsize = channel->on_hdrsize(channel, memsz);
			packet = (ReactorPacket_t*)malloc(sizeof(ReactorPacket_t) + hdrsize + memsz);
			if (!packet)
				break;
			memset(packet, 0, sizeof(*packet));
			packet->channel = &channel->_;
			packet->_.type = no_ack ? NETPACKET_NO_ACK_FRAGMENT : NETPACKET_FRAGMENT;
			packet->_.hdrlen = hdrsize;
			packet->_.bodylen = memsz;
			listInsertNodeBack(packetlist, packetlist->tail, &packet->cmd._);
			off += memsz;
		}
		if (packet) {
			packet->_.type = no_ack ? NETPACKET_NO_ACK_FRAGMENT_EOF : NETPACKET_FRAGMENT_EOF;
		}
		else {
			ListNode_t *next;
			for (cur = packetlist->head; cur; cur = next) {
				next = cur->next;
				free(pod_container_of(cur, ReactorPacket_t, cmd._));
			}
			listInit(packetlist);
			return 0;
		}
		iov_i = iov_off = 0;
		for (cur = packetlist->head; cur; cur = cur->next) {
			packet = pod_container_of(cur, ReactorPacket_t, cmd._);
			off = 0;
			while (iov_i < iovcnt) {
				unsigned int pkleftsize = packet->_.bodylen - off;
				unsigned int iovleftsize = iobufLen(iov + iov_i) - iov_off;
				if (iovleftsize > pkleftsize) {
					memcpy(packet->_.buf + packet->_.hdrlen + off, ((char*)iobufPtr(iov + iov_i)) + iov_off, pkleftsize);
					iov_off += pkleftsize;
					break;
				}
				else {
					memcpy(packet->_.buf + packet->_.hdrlen + off, ((char*)iobufPtr(iov + iov_i)) + iov_off, iovleftsize);
					iov_off = 0;
					iov_i++;
					off += iovleftsize;
				}
			}
		}
	}
	else {
		unsigned int hdrsize = channel->on_hdrsize(channel, 0);
		if (0 == hdrsize && (CHANNEL_FLAG_STREAM & channel->flag))
			return 1;
		packet = (ReactorPacket_t*)malloc(sizeof(ReactorPacket_t) + hdrsize);
		if (!packet)
			return 0;
		memset(packet, 0, sizeof(*packet));
		packet->channel = &channel->_;
		packet->_.type = no_ack ? NETPACKET_NO_ACK_FRAGMENT_EOF : NETPACKET_FRAGMENT_EOF;
		packet->_.hdrlen = hdrsize;
		listInsertNodeBack(packetlist, packetlist->tail, &packet->cmd._);
	}
	return 1;
}

static void channel_stream_on_sys_recvfin(ChannelBase_t* base, long long timestamp_msec) {
	Channel_t* channel = pod_container_of(base, Channel_t, _);
	if (!base->has_sendfin)
		channelSendFin(pod_container_of(base, Channel_t, _), timestamp_msec);
	if (channel->flag & CHANNEL_FLAG_RELIABLE)
		base->has_sendfin = 1;
}

/*******************************************************************************/

Channel_t* reactorobjectOpenChannel(ReactorObject_t* o, int flag, unsigned int initseq, const void* saddr) {
	Channel_t* channel = (Channel_t*)channelbaseOpen(sizeof(Channel_t), o, saddr);
	if (!channel)
		return NULL;
	flag &= ~(CHANNEL_FLAG_DGRAM | CHANNEL_FLAG_STREAM);
	flag |= (SOCK_STREAM == o->socktype) ? CHANNEL_FLAG_STREAM : CHANNEL_FLAG_DGRAM;
	if (flag & CHANNEL_FLAG_DGRAM) {
		if (flag & CHANNEL_FLAG_LISTEN) {
			flag |= CHANNEL_FLAG_RELIABLE;
			channel->dgram.halfconn_maxwaitcnt = 200;
		}
		channel->dgram.rto = 200;
		channel->dgram.resend_maxtimes = 5;
		dgramtransportctxInit(&channel->dgram.ctx, initseq);
	}
	else {
		streamtransportctxInit(&channel->_.stream_ctx, initseq);
		channel->_.stream_on_sys_recvfin = channel_stream_on_sys_recvfin;
	}
	channel->flag = flag;
	channel->m_initseq = initseq;
	channel->_.on_exec = on_exec;
	channel->_.on_read = on_read;
	channel->_.on_pre_send = on_pre_send;
	return channel;
}

Channel_t* channelEnableHeartbeat(Channel_t* channel, long long now_msec) {
	Thread_t t = channel->_.o->reactor->m_runthread;
	if (threadEqual(t, threadSelf()))
		channel_set_heartbeat_timestamp(channel, now_msec);
	return channel;
}

Channel_t* channelSendSyn(Channel_t* channel, long long timestamp_msec) {
	Thread_t t = channel->_.o->reactor->m_runthread;
	if (!threadEqual(t, threadSelf()))
		return channel;
	if ((channel->flag & CHANNEL_FLAG_CLIENT) &&
		(channel->flag & CHANNEL_FLAG_DGRAM) &&
		(channel->flag & CHANNEL_FLAG_RELIABLE))
	{
		unsigned int hdrsize = channel->on_hdrsize(channel, 0);
		ReactorPacket_t* packet = (ReactorPacket_t*)malloc(sizeof(ReactorPacket_t) + hdrsize);
		if (!packet) {
			channel_invalid(&channel->_, REACTOR_CONNECT_ERR);
			return NULL;
		}
		memset(packet, 0, sizeof(*packet));
		packet->channel = &channel->_;
		packet->_.type = NETPACKET_SYN;
		packet->_.hdrlen = hdrsize;
		packet->_.bodylen = 0;
		channel->dgram.m_synpacket = packet;

		if (packet->_.hdrlen)
			channel->on_encode(channel, packet->_.buf, 0, NETPACKET_SYN, 0);
		packet->_.wait_ack = 1;
		packet->_.resend_msec = timestamp_msec + channel->dgram.rto;
		channel_set_timestamp(channel, packet->_.resend_msec);
		socketWrite(channel->_.o->fd, packet->_.buf, packet->_.hdrlen + packet->_.bodylen, 0,
			&channel->_.to_addr, sockaddrLength(&channel->_.to_addr));
	}
	return channel;
}

void channelSendFin(Channel_t* channel, long long timestamp_msec) {
	if (_xchg8(&channel->m_ban_send, 1))
		return;
	else if (channel->flag & CHANNEL_FLAG_RELIABLE) {
		unsigned int hdrsize = channel->on_hdrsize(channel, 0);
		ReactorPacket_t* packet = (ReactorPacket_t*)malloc(sizeof(ReactorPacket_t) + hdrsize);
		if (!packet)
			return;
		memset(packet, 0, sizeof(*packet));
		packet->_.hdrlen = hdrsize;
		packet->_.type = NETPACKET_FIN;
		packet->_.bodylen = 0;
		channelbaseSendPacket(&channel->_, packet);
	}
	else if (channel->flag & CHANNEL_FLAG_STREAM) {
		ReactorObject_t* o = channel->_.o;
		reactorCommitCmd(o->reactor, &o->stream.sendfincmd);
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
	channelbaseSendPacketList(&channel->_, &packetlist);
	return channel;
}

static void channel_free_packetlist(List_t* list) {
	ListNode_t* cur, *next;
	for (cur = list->head; cur; cur = next) {
		next = cur->next;
		free(pod_container_of(cur, ReactorPacket_t, _.node));
	}
	listInit(list);
}

void channelDestroy(Channel_t* channel) {
	if (channel->flag & CHANNEL_FLAG_STREAM) {
		channel_free_packetlist(&channel->_.stream_ctx.recvpacketlist);
		channel_free_packetlist(&channel->_.stream_ctx.sendpacketlist);
	}
	else if (channel->flag & CHANNEL_FLAG_LISTEN) {
		ListNode_t* cur, *next;
		for (cur = channel->dgram.ctx.recvpacketlist.head; cur; cur = next) {
			next = cur->next;
			free_halfconn(pod_container_of(cur, DgramHalfConn_t, node));
		}
		listInit(&channel->dgram.ctx.recvpacketlist);
		channel->dgram.m_halfconn_curwaitcnt = channel->dgram.halfconn_maxwaitcnt = 0;
	}
	else {
		free(channel->dgram.m_synpacket);
		channel->dgram.m_synpacket = NULL;
		channel_free_packetlist(&channel->dgram.ctx.recvpacketlist);
		channel_free_packetlist(&channel->dgram.ctx.sendpacketlist);
	}
}

#ifdef	__cplusplus
}
#endif
