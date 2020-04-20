//
// Created by hujianzhe on 2019-7-11.
//

#include "../../inc/component/channel.h"
#include "../../inc/sysapi/error.h"
#include "../../inc/sysapi/misc.h"
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
			reactorpacketFree(packet);
		}
	}
	return ptr;
}

static void channel_recv_fin_haneler(Channel_t* channel) {
	if (channel->_.has_recvfin)
		return;
	channel->_.has_recvfin = 1;
	if (channel->_.has_sendfin)
		return;
	channelSendv(channel, NULL, 0, NETPACKET_FIN);
}

static int channel_merge_packet_handler(Channel_t* channel, List_t* packetlist, ChannelInbufDecodeResult_t* decode_result) {
	ReactorPacket_t* packet = pod_container_of(packetlist->tail, ReactorPacket_t, _.node);
	if (NETPACKET_FIN == packet->_.type) {
		ListNode_t* cur, *next;
		for (cur = packetlist->head; cur; cur = next) {
			next = cur->next;
			reactorpacketFree(pod_container_of(cur, ReactorPacket_t, _.node));
		}
		channel_recv_fin_haneler(channel);
	}
	else if (packetlist->head == packetlist->tail) {
		decode_result->bodyptr = packet->_.buf;
		decode_result->bodylen = packet->_.bodylen;
		channel->on_recv(channel, &channel->_.to_addr, decode_result);
		reactorpacketFree(packet);
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
		unsigned char pktype;
		memset(&decode_result, 0, sizeof(decode_result));
		channel->on_decode(channel, buf + off, len - off, &decode_result);
		if (decode_result.err)
			return -1;
		else if (decode_result.incomplete)
			return off;
		off += decode_result.decodelen;
		pktype = decode_result.pktype;
		if (0 == pktype) {
			channel->on_recv(channel, &channel->_.to_addr, &decode_result);
		}
		else if (NETPACKET_SYN == pktype) {
			if (channel->_.flag & CHANNEL_FLAG_SERVER) {
				channel->on_recv(channel, &channel->_.to_addr, &decode_result);
			}
			else {
				continue;
			}
		}
		else if (NETPACKET_SYN_ACK == pktype) {
			if (channel->_.flag & CHANNEL_FLAG_CLIENT) {
				channel->on_recv(channel, &channel->_.to_addr, &decode_result);
			}
			else {
				continue;
			}
		}
		else {
			unsigned int pkseq = decode_result.pkseq;
			StreamTransportCtx_t* ctx = &channel->_.stream_ctx;
			if (streamtransportctxRecvCheck(ctx, pkseq, pktype)) {
				/*
				if (pktype >= NETPACKET_STREAM_HAS_SEND_SEQ)
					channel->on_reply_ack(channel, pkseq, &channel->_.to_addr);
				*/
				if (ctx->recvlist.head || NETPACKET_NO_ACK_FRAGMENT == pktype || NETPACKET_FRAGMENT == pktype) {
					List_t list;
					ReactorPacket_t* packet = reactorpacketMake(pktype, 0, decode_result.bodylen);
					if (!packet)
						return -1;
					packet->_.seq = pkseq;
					memcpy(packet->_.buf, decode_result.bodyptr, decode_result.bodylen);
					streamtransportctxCacheRecvPacket(ctx, &packet->_);
					while (streamtransportctxMergeRecvPacket(ctx, &list)) {
						if (!channel_merge_packet_handler(channel, &list, &decode_result))
							return -1;
					}
				}
				else if (NETPACKET_FIN == pktype) {
					channel_recv_fin_haneler(channel);
				}
				else {
					channel->on_recv(channel, &channel->_.to_addr, &decode_result);
				}
			}
			/*
			else if (NETPACKET_ACK == pktype) {
				NetPacket_t* ackpk = NULL;
				if (!streamtransportctxAckSendPacket(ctx, pkseq, &ackpk))
					return -1;
				reactorpacketFree(pod_container_of(ackpk, ReactorPacket_t, _));
			}
			else if (pktype >= NETPACKET_STREAM_HAS_SEND_SEQ)
				channel->on_reply_ack(channel, pkseq, &channel->_.to_addr);
			*/
			else
				return -1;
		}
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
		for (cur = channel->_.dgram_ctx.recvlist.head; cur; cur = cur->next) {
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
				listPushNodeBack(&channel->_.dgram_ctx.recvlist, &halfconn->node);
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
		for (cur = channel->_.dgram_ctx.recvlist.head; cur; cur = next) {
			Sockaddr_t addr;
			FD_t connfd;
			DgramHalfConn_t* halfconn = pod_container_of(cur, DgramHalfConn_t, node);
			next = cur->next;
			if (!sockaddrIsEqual(&halfconn->from_addr, from_saddr))
				continue;
			connfd = halfconn->sockfd;
			if (socketRead(connfd, NULL, 0, 0, &addr.st))
				continue;
			listRemoveNode(&channel->_.dgram_ctx.recvlist, cur);
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
	ReactorPacket_t* packet;
	unsigned char pktype;
	unsigned int pkseq;
	int from_listen, from_peer;
	ChannelInbufDecodeResult_t decode_result;

	memset(&decode_result, 0, sizeof(decode_result));
	channel->on_decode(channel, buf, len, &decode_result);
	if (decode_result.err)
		return 1;
	else if (decode_result.incomplete)
		return 1;
	pktype = decode_result.pktype;
	if (0 == pktype) {
		channel->on_recv(channel, from_saddr, &decode_result);
	}
	else if ((channel->_.flag & CHANNEL_FLAG_CLIENT) || (channel->_.flag & CHANNEL_FLAG_SERVER)) {
		pkseq = decode_result.pkseq;
		if (channel->_.flag & CHANNEL_FLAG_CLIENT)
			from_listen = sockaddrIsEqual(&channel->_.connect_addr, from_saddr);
		else
			from_listen = 0;
		if (from_listen)
			from_peer = 0;
		else
			from_peer = sockaddrIsEqual(&channel->_.to_addr, from_saddr);

		if (NETPACKET_SYN == pktype) {
			if (!(channel->_.flag & CHANNEL_FLAG_SERVER) || from_listen)
				return 1;
			channel->on_recv(channel, from_saddr, &decode_result);
		}
		else if (NETPACKET_SYN_ACK == pktype) {
			if (from_listen) {
				if (decode_result.bodylen < sizeof(unsigned short))
					return 1;
				if (channel->dgram.m_synpacket) {
					unsigned short port = *(unsigned short*)decode_result.bodyptr;
					port = ntohs(port);
					sockaddrSetPort(&channel->_.to_addr.st, port);
					reactorpacketFree(channel->dgram.m_synpacket);
					channel->dgram.m_synpacket = NULL;
					if (~0 != channel->_.connected_times) {
						++channel->_.connected_times;
					}
					channel->_.on_syn_ack(&channel->_, timestamp_msec);
				}
				channel->dgram.on_reply_ack(channel, 0, from_saddr);
				socketWrite(channel->_.o->fd, NULL, 0, 0, &channel->_.to_addr, sockaddrLength(&channel->_.to_addr));
			}
			else if (from_peer && channel->dgram.m_synpacket) {
				reactorpacketFree(channel->dgram.m_synpacket);
				channel->dgram.m_synpacket = NULL;
				if (channel->_.flag & CHANNEL_FLAG_CLIENT) {
					channel->on_recv(channel, from_saddr, &decode_result);
				}
				else {
					return 1;
				}
			}
		}
		else if (!from_peer || from_listen) {
			return 1;
		}
		else if (dgramtransportctxRecvCheck(&channel->_.dgram_ctx, pkseq, pktype)) {
			List_t list;
			channel->dgram.on_reply_ack(channel, pkseq, from_saddr);
			packet = reactorpacketMake(pktype, 0, decode_result.bodylen);
			if (!packet)
				return 0;
			packet->_.seq = pkseq;
			memcpy(packet->_.buf, decode_result.bodyptr, packet->_.bodylen);
			dgramtransportctxCacheRecvPacket(&channel->_.dgram_ctx, &packet->_);
			while (dgramtransportctxMergeRecvPacket(&channel->_.dgram_ctx, &list)) {
				if (!channel_merge_packet_handler(channel, &list, &decode_result))
					return 0;
			}
		}
		else if (NETPACKET_ACK == pktype) {
			NetPacket_t* ackpkg;
			int cwndskip = dgramtransportctxAckSendPacket(&channel->_.dgram_ctx, pkseq, &ackpkg);
			if (!ackpkg)
				return 1;
			reactorpacketFree(pod_container_of(ackpkg, ReactorPacket_t, _));
			if (cwndskip) {
				ListNode_t* cur;
				for (cur = channel->_.dgram_ctx.sendlist.head; cur; cur = cur->next) {
					packet = pod_container_of(cur, ReactorPacket_t, _.node);
					if (!dgramtransportctxSendWindowHasPacket(&channel->_.dgram_ctx, &packet->_))
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
			channel->dgram.on_reply_ack(channel, pkseq, from_saddr);
	}
	else {
		channel->on_recv(channel, from_saddr, &decode_result);
	}
	channel->m_heartbeat_times = 0;
	channel_set_heartbeat_timestamp(channel, timestamp_msec);
	return 1;
}

static int on_read(ChannelBase_t* base, unsigned char* buf, unsigned int len, unsigned int off, long long timestamp_msec, const void* from_addr) {
	Channel_t* channel = pod_container_of(base, Channel_t, _);
	if (CHANNEL_FLAG_STREAM & channel->_.flag) {
		off = channel_stream_recv_handler(channel, buf, len, off, timestamp_msec);
		if (off < 0)
			channel_invalid(base, REACTOR_ONREAD_ERR);
		else if (base->has_recvfin && base->has_sendfin)
			channel_invalid(base, 0);
		return off;
	}
	else {
		if (channel->_.flag & CHANNEL_FLAG_LISTEN)
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
	if (CHANNEL_FLAG_STREAM & channel->_.flag) {
		packet->_.seq = streamtransportctxNextSendSeq(&base->stream_ctx, packet->_.type);
		channel->on_encode(channel, packet->_.buf, packet->_.bodylen, packet->_.type, packet->_.seq);
		return 1;
	}
	else {
		packet->_.seq = dgramtransportctxNextSendSeq(&channel->_.dgram_ctx, packet->_.type);
		channel->on_encode(channel, packet->_.buf, packet->_.bodylen, packet->_.type, packet->_.seq);
		if (dgramtransportctxCacheSendPacket(&channel->_.dgram_ctx, &packet->_)) {
			if (!dgramtransportctxSendWindowHasPacket(&channel->_.dgram_ctx, &packet->_))
				return 0;
			if (NETPACKET_FIN == packet->_.type)
				channel->_.has_sendfin = 1;
		}
		else if (NETPACKET_SYN == packet->_.type) {
			channel->dgram.m_synpacket = packet;
			packet->_.cached = 1;
			packet->_.wait_ack = 1;
			packet->_.resend_msec = timestamp_msec + channel->dgram.rto;
			channel_set_timestamp(channel, packet->_.resend_msec);
		}
		if (packet->_.type >= NETPACKET_DGRAM_HAS_SEND_SEQ) {
			packet->_.wait_ack = 1;
			packet->_.resend_msec = timestamp_msec + channel->dgram.rto;
			channel_set_timestamp(channel, packet->_.resend_msec);
		}
	}
	return 1;
}

static void on_exec(ChannelBase_t* base, long long timestamp_msec) {
	Channel_t* channel = pod_container_of(base, Channel_t, _);
/* heartbeat */
	if (channel->_.flag & CHANNEL_FLAG_CLIENT) {
		if (channel->m_heartbeat_msec > 0 &&
			channel->heartbeat_timeout_sec > 0 &&
			channel->on_heartbeat)
		{
			long long ts = channel->heartbeat_timeout_sec;
			ts *= 1000;
			ts += channel->m_heartbeat_msec;
			if (ts <= timestamp_msec) {
				int ok = 0;
				if (channel->m_heartbeat_times < channel->heartbeat_maxtimes) {
					ok = channel->on_heartbeat(channel, channel->m_heartbeat_times++);
				}
				else if (channel->on_heartbeat(channel, channel->m_heartbeat_times)) {
					ok = 1;
					channel->m_heartbeat_times = 0;
				}
				if (!ok) {
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
	if (!(channel->_.flag & CHANNEL_FLAG_DGRAM))
	{
		return;
	}
	if (channel->_.flag & CHANNEL_FLAG_LISTEN) {
		ListNode_t* cur, *next;
		for (cur = channel->_.dgram_ctx.recvlist.head; cur; cur = next) {
			DgramHalfConn_t* halfconn = pod_container_of(cur, DgramHalfConn_t, node);
			next = cur->next;
			if (halfconn->resend_msec > timestamp_msec) {
				channel_set_timestamp(channel, halfconn->resend_msec);
				continue;
			}
			if (halfconn->resend_times >= channel->dgram.resend_maxtimes) {
				listRemoveNode(&channel->_.dgram_ctx.recvlist, cur);
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
			reactorpacketFree(channel->dgram.m_synpacket);
			channel->dgram.m_synpacket = NULL;
			channel_invalid(base, REACTOR_CONNECT_ERR);
			return;
		}
		else {
			socketWrite(channel->_.o->fd, packet->_.buf, packet->_.hdrlen + packet->_.bodylen, 0,
				&channel->_.to_addr, sockaddrLength(&channel->_.to_addr));
			packet->_.resend_times++;
			packet->_.resend_msec = timestamp_msec + channel->dgram.rto;
			channel_set_timestamp(channel, packet->_.resend_msec);
		}
	}
	else {
		ListNode_t* cur;
		for (cur = channel->_.dgram_ctx.sendlist.head; cur; cur = cur->next) {
			ReactorPacket_t* packet = pod_container_of(cur, ReactorPacket_t, _.node);
			if (!packet->_.wait_ack)
				break;
			if (packet->_.resend_msec > timestamp_msec) {
				channel_set_timestamp(channel, packet->_.resend_msec);
				continue;
			}
			if (packet->_.resend_times >= channel->dgram.resend_maxtimes) {
				if (channel->_.flag & CHANNEL_FLAG_CLIENT) {
					if (channel->on_heartbeat(channel, channel->heartbeat_maxtimes)) {
						channel->m_heartbeat_times = 0;
						channel_set_heartbeat_timestamp(channel, timestamp_msec);
					}
					else {
						int err = (NETPACKET_FIN != packet->_.type ? REACTOR_ZOMBIE_ERR : 0);
						channel_invalid(base, err);
					}
				}
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

static List_t* channel_shared_data(Channel_t* channel, const Iobuf_t iov[], unsigned int iovcnt, List_t* packetlist) {
	unsigned int i, nbytes = 0;
	ReactorPacket_t* packet;
	for (i = 0; i < iovcnt; ++i)
		nbytes += iobufLen(iov + i);
	listInit(packetlist);
	if (nbytes) {
		unsigned int off = 0, iov_i = 0, iov_off = 0;
		unsigned int sharedsize = channel->_.write_fragment_size;
		sharedsize -= channel->on_hdrsize(channel, sharedsize);
		packet = NULL;
		while (off < nbytes) {
			unsigned int memsz = nbytes - off > sharedsize ? sharedsize : nbytes - off;
			unsigned int hdrsize = channel->on_hdrsize(channel, memsz);
			packet = reactorpacketMake(0, hdrsize, memsz);
			if (!packet)
				break;
			listInsertNodeBack(packetlist, packetlist->tail, &packet->cmd._);
			off += memsz;
			iobufSharedCopy(iov, iovcnt, &iov_i, &iov_off, packet->_.buf + packet->_.hdrlen, packet->_.bodylen);
		}
		if (!packet) {
			ListNode_t *cur, *next;
			for (cur = packetlist->head; cur; cur = next) {
				next = cur->next;
				reactorpacketFree(pod_container_of(cur, ReactorPacket_t, cmd._));
			}
			listInit(packetlist);
			return NULL;
		}
	}
	else {
		unsigned int hdrsize = channel->on_hdrsize(channel, 0);
		if (0 == hdrsize && (CHANNEL_FLAG_STREAM & channel->_.flag))
			return packetlist;
		packet = reactorpacketMake(0, hdrsize, 0);
		if (!packet)
			return NULL;
		listInsertNodeBack(packetlist, packetlist->tail, &packet->cmd._);
	}
	return packetlist;
}

/*
static void channel_stream_on_sys_recvfin(ChannelBase_t* base, long long timestamp_msec) {
	Channel_t* channel = pod_container_of(base, Channel_t, _);
	channelSendv(channel, NULL, 0, NETPACKET_FIN);
}
*/

/*******************************************************************************/

Channel_t* reactorobjectOpenChannel(ReactorObject_t* o, unsigned short flag, unsigned int initseq, const void* saddr) {
	Channel_t* channel = (Channel_t*)channelbaseOpen(sizeof(Channel_t), flag, o, saddr);
	if (!channel)
		return NULL;
	flag = channel->_.flag;
	if (flag & CHANNEL_FLAG_DGRAM) {
		if (flag & CHANNEL_FLAG_LISTEN) {
			channel->dgram.halfconn_maxwaitcnt = 200;
		}
		channel->dgram.rto = 200;
		channel->dgram.resend_maxtimes = 5;
		dgramtransportctxInit(&channel->_.dgram_ctx, initseq);
	}
	else {
		streamtransportctxInit(&channel->_.stream_ctx, initseq);
		//channel->_.stream_on_sys_recvfin = channel_stream_on_sys_recvfin;
	}
	channel->m_initseq = initseq;
	channel->_.on_exec = on_exec;
	channel->_.on_read = on_read;
	channel->_.on_pre_send = on_pre_send;
	return channel;
}

Channel_t* channelEnableHeartbeat(Channel_t* channel, long long now_msec) {
	Thread_t t = channel->_.reactor->m_runthread;
	if (threadEqual(t, threadSelf()))
		channel_set_heartbeat_timestamp(channel, now_msec);
	return channel;
}

Channel_t* channelSend(Channel_t* channel, const void* data, unsigned int len, int pktype) {
	Iobuf_t iov = iobufStaticInit(data, len);
	return channelSendv(channel, &iov, 1, pktype);
}

List_t* channelSharedData(Channel_t* channel, const Iobuf_t iov[], unsigned int iovcnt, int pktype, List_t* packetlist) {
	ListNode_t* cur, *next;
	if (!channel_shared_data(channel, iov, iovcnt, packetlist))
		return NULL;
	switch (pktype) {
		case NETPACKET_SYN:
		case NETPACKET_SYN_ACK:
		case NETPACKET_FIN:
		case NETPACKET_ACK:
		{
			for (cur = packetlist->head; cur; cur = cur->next) {
				if (cur != packetlist->head) {
					for (cur = packetlist->head; cur; cur = next) {
						next = cur->next;
						reactorpacketFree(pod_container_of(cur, ReactorPacket_t, cmd._));
					}
					return NULL;
				}
				else {
					ReactorPacket_t* packet = pod_container_of(cur, ReactorPacket_t, cmd._);
					packet->_.type = pktype;
				}
			}
			break;
		}
		case NETPACKET_NO_ACK_FRAGMENT:
		case NETPACKET_FRAGMENT:
		{
			ReactorPacket_t* packet = NULL;
			int no_ack;
			if (channel->_.flag & CHANNEL_FLAG_STREAM)
				no_ack = 1;
			else if ((channel->_.flag & CHANNEL_FLAG_CLIENT) || (channel->_.flag & CHANNEL_FLAG_SERVER))
				no_ack = (pktype != NETPACKET_FRAGMENT && pktype != NETPACKET_FRAGMENT_EOF);
			else
				no_ack = 1;
			for (cur = packetlist->head; cur; cur = cur->next) {
				packet = pod_container_of(cur, ReactorPacket_t, cmd._);
				packet->_.type = (no_ack ? NETPACKET_NO_ACK_FRAGMENT : NETPACKET_FRAGMENT);
			}
			if (packet)
				packet->_.type = (no_ack ? NETPACKET_NO_ACK_FRAGMENT_EOF : NETPACKET_FRAGMENT_EOF);
			break;
		}
		default:
		{
			for (cur = packetlist->head; cur; cur = next) {
				next = cur->next;
				reactorpacketFree(pod_container_of(cur, ReactorPacket_t, cmd._));
			}
			return NULL;
		}
	}
	return packetlist;
}

Channel_t* channelSendv(Channel_t* channel, const Iobuf_t iov[], unsigned int iovcnt, int pktype) {
	List_t packetlist;
	if (NETPACKET_SYN == pktype) {
		if (!(channel->_.flag & CHANNEL_FLAG_CLIENT))
			return channel;
		if (!(channel->_.flag & CHANNEL_FLAG_DGRAM) && 0 == channel->_.connected_times)
			return channel;
	}
	else if (NETPACKET_FIN == pktype) {
		if (_xchg32(&channel->m_has_sendfin, 1))
			return channel;
	}
	else if (channel->m_has_sendfin)
		return channel;

	if (!channelSharedData(channel, iov, iovcnt, pktype, &packetlist))
		return NULL;
	channelbaseSendPacketList(&channel->_, &packetlist);
	return channel;
}

void channelDestroy(Channel_t* channel) {
	if (channel->_.flag & CHANNEL_FLAG_DGRAM) {
		if (channel->_.flag & CHANNEL_FLAG_LISTEN) {
			ListNode_t* cur, *next;
			for (cur = channel->_.dgram_ctx.recvlist.head; cur; cur = next) {
				next = cur->next;
				free_halfconn(pod_container_of(cur, DgramHalfConn_t, node));
			}
			listInit(&channel->_.dgram_ctx.recvlist);
			channel->dgram.m_halfconn_curwaitcnt = channel->dgram.halfconn_maxwaitcnt = 0;
		}
		else {
			reactorpacketFree(channel->dgram.m_synpacket);
			channel->dgram.m_synpacket = NULL;
		}
	}
}

#ifdef	__cplusplus
}
#endif
