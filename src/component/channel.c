//
// Created by hujianzhe on 2019-7-11.
//

#include "../../inc/component/channel.h"
#include "../../inc/sysapi/error.h"
#include <stdlib.h>
#include <string.h>

typedef struct DgramHalfConn_t {
	ListNode_t node;
	long long expire_time_msec;
	FD_t sockfd;
	Sockaddr_t from_addr;
	unsigned short local_port;
	unsigned int len;
	unsigned char buf[1];
} DgramHalfConn_t;

#ifdef	__cplusplus
extern "C" {
#endif

static const ChannelOutbufEncodeParam_t* netpacket2encodeparam(NetPacket_t* pkg, ChannelOutbufEncodeParam_t* param) {
	param->bodylen = pkg->bodylen;
	param->hdrlen = pkg->hdrlen;
	param->pkseq = pkg->seq;
	param->fragment_eof = pkg->fragment_eof;
	param->pktype = pkg->type;
	param->buf = pkg->buf;
	return param;
}

/*******************************************************************************/

static void channel_invalid(ChannelBase_t* base, int detach_error) {
	base->valid = 0;
	base->detach_error = detach_error;
}

static void free_halfconn(DgramHalfConn_t* halfconn) {
	if (INVALID_FD_HANDLE != halfconn->sockfd) {
		socketClose(halfconn->sockfd);
	}
	free(halfconn);
}

static void channel_set_timestamp(Channel_t* channel, long long timestamp_msec) {
	if (channel->_.event_msec > 0 && channel->_.event_msec <= timestamp_msec) {
		return;
	}
	channel->_.event_msec = timestamp_msec;
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

static int channel_recv_fin_handler(Channel_t* channel) {
	if (channel->_.has_recvfin) {
		return 1;
	}
	channel->_.has_recvfin = 1;

	if (channel->_.has_sendfin) {
		return 1;
	}
	if (_xchg32(&channel->m_has_sendfin, 1)) {
		return 1;
	}
	else {
		unsigned int hdrsz = channel->_.on_hdrsize(&channel->_, 0);
		ReactorPacket_t* fin_packet = reactorpacketMake(NETPACKET_FIN, hdrsz, 0);
		if (!fin_packet) {
			return 0;
		}
		channelbaseSendPacket(&channel->_, fin_packet);
		return 1;
	}
}

static int channel_merge_packet_handler(Channel_t* channel, List_t* packetlist, ChannelInbufDecodeResult_t* decode_result) {
	ReactorPacket_t* packet = pod_container_of(packetlist->tail, ReactorPacket_t, _.node);
	if (NETPACKET_FIN == packet->_.type) {
		ListNode_t* cur, *next;
		for (cur = packetlist->head; cur; cur = next) {
			next = cur->next;
			reactorpacketFree(pod_container_of(cur, ReactorPacket_t, _.node));
		}
		return channel_recv_fin_handler(channel);
	}
	else if (packetlist->head == packetlist->tail) {
		decode_result->bodyptr = packet->_.buf;
		decode_result->bodylen = packet->_.bodylen;
		channel->on_recv(channel, &channel->_.to_addr.sa, decode_result);
		reactorpacketFree(packet);
	}
	else {
		decode_result->bodyptr = merge_packet(packetlist, &decode_result->bodylen);
		if (!decode_result->bodyptr) {
			return 0;
		}
		channel->on_recv(channel, &channel->_.to_addr.sa, decode_result);
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
		if (decode_result.err) {
			return -1;
		}
		else if (decode_result.incomplete) {
			return off;
		}
		off += decode_result.decodelen;
		if (decode_result.ignore) {
			continue;
		}
		pktype = decode_result.pktype;
		if (0 == pktype) {
			channel->on_recv(channel, &channel->_.to_addr.sa, &decode_result);
		}
		else {
			unsigned int pkseq = decode_result.pkseq;
			StreamTransportCtx_t* ctx = &channel->_.stream_ctx;
			if (streamtransportctxRecvCheck(ctx, pkseq, pktype)) {
				if (channel->_.readcache_max_size > 0 &&
					channel->_.readcache_max_size < channel->_.stream_ctx.cache_recv_bytes + decode_result.bodylen)
				{
					return 0;
				}
				/*
				if (pktype >= NETPACKET_STREAM_HAS_SEND_SEQ)
					channel->on_reply_ack(channel, pkseq, &channel->_.to_addr.sa);
				*/
				if (ctx->recvlist.head || !decode_result.fragment_eof) {
					List_t list;
					ReactorPacket_t* packet = reactorpacketMake(pktype, 0, decode_result.bodylen);
					if (!packet) {
						return -1;
					}
					packet->_.seq = pkseq;
					packet->_.fragment_eof = decode_result.fragment_eof;
					memcpy(packet->_.buf, decode_result.bodyptr, decode_result.bodylen);
					streamtransportctxCacheRecvPacket(ctx, &packet->_);
					if (decode_result.fragment_eof) {
						while (streamtransportctxMergeRecvPacket(ctx, &list)) {
							if (!channel_merge_packet_handler(channel, &list, &decode_result)) {
								return -1;
							}
						}
					}
				}
				else {
					channel->on_recv(channel, &channel->_.to_addr.sa, &decode_result);
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
				channel->on_reply_ack(channel, pkseq, &channel->_.to_addr.sa);
			*/
			else {
				return -1;
			}
		}
	}
	return off;
}

static int channel_dgram_listener_handler(Channel_t* channel, unsigned char* buf, int len, long long timestamp_msec, const struct sockaddr* from_saddr) {
	ChannelInbufDecodeResult_t decode_result;
	unsigned char pktype;
	memset(&decode_result, 0, sizeof(decode_result));
	channel->on_decode(channel, buf, len, &decode_result);
	if (decode_result.err) {
		return 1;
	}
	else if (decode_result.incomplete) {
		return 1;
	}
	else if (decode_result.ignore) {
		return 1;
	}
	pktype = decode_result.pktype;
	if (NETPACKET_SYN == pktype) {
		DgramHalfConn_t* halfconn = NULL;
		ListNode_t* cur;
		for (cur = channel->_.dgram_ctx.recvlist.head; cur; cur = cur->next) {
			halfconn = pod_container_of(cur, DgramHalfConn_t, node);
			if (sockaddrIsEqual(&halfconn->from_addr.sa, from_saddr)) {
				break;
			}
		}
		if (cur) {
			sendto(channel->_.o->fd, (char*)halfconn->buf, halfconn->len, 0,
				&halfconn->from_addr.sa, sockaddrLength(&halfconn->from_addr.sa));
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
				socklen_t local_slen;
				unsigned int buflen, hdrlen;
				ChannelOutbufEncodeParam_t encode_param;
				memcpy(&local_saddr, &channel->_.listen_addr, sockaddrLength(&channel->_.listen_addr.sa));
				if (!sockaddrSetPort(&local_saddr.sa, 0)) {
					break;
				}
				o = channel->_.o;
				new_sockfd = socket(o->domain, o->socktype, o->protocol);
				if (INVALID_FD_HANDLE == new_sockfd) {
					break;
				}
				local_slen = sockaddrLength(&local_saddr.sa);
				if (bind(new_sockfd, &local_saddr.sa, local_slen)) {
					break;
				}
				if (getsockname(new_sockfd, &local_saddr.sa, &local_slen)) {
					break;
				}
				if (!sockaddrDecode(&local_saddr.sa, NULL, &local_port)) {
					break;
				}
				if (!socketNonBlock(new_sockfd, TRUE)) {
					break;
				}
				hdrlen = channel->_.on_hdrsize(&channel->_, sizeof(local_port));
				buflen = hdrlen + sizeof(local_port);
				halfconn = (DgramHalfConn_t*)malloc(sizeof(DgramHalfConn_t) + buflen);
				if (!halfconn) {
					break;
				}
				halfconn->sockfd = new_sockfd;
				memcpy(&halfconn->from_addr, from_saddr, sockaddrLength(from_saddr));
				halfconn->local_port = local_port;
				halfconn->expire_time_msec = timestamp_msec + channel->dgram.rto;
				halfconn->len = buflen;
				listPushNodeBack(&channel->_.dgram_ctx.recvlist, &halfconn->node);
				channel->dgram.m_halfconn_curwaitcnt++;
				channel_set_timestamp(channel, halfconn->expire_time_msec);

				encode_param.bodylen = sizeof(local_port);
				encode_param.hdrlen = hdrlen;
				encode_param.pkseq = 0;
				encode_param.fragment_eof = 1;
				encode_param.pktype = NETPACKET_SYN_ACK;
				encode_param.buf = halfconn->buf;
				channel->on_encode(channel, &encode_param);
				*(unsigned short*)(halfconn->buf + hdrlen) = htons(local_port);
				sendto(o->fd, (char*)halfconn->buf, halfconn->len, 0,
					&halfconn->from_addr.sa, sockaddrLength(&halfconn->from_addr.sa));
			} while (0);
			if (!halfconn) {
				free(halfconn);
				if (INVALID_FD_HANDLE != new_sockfd) {
					socketClose(new_sockfd);
				}
				return 1;
			}
		}
	}
	else if (NETPACKET_ACK == pktype) {
		ListNode_t* cur, *next;
		Sockaddr_t addr;
		socklen_t slen = sizeof(addr.st);
		for (cur = channel->_.dgram_ctx.recvlist.head; cur; cur = next) {
			FD_t connfd;
			DgramHalfConn_t* halfconn = pod_container_of(cur, DgramHalfConn_t, node);
			next = cur->next;
			if (!sockaddrIsEqual(&halfconn->from_addr.sa, from_saddr)) {
				continue;
			}
			connfd = halfconn->sockfd;
			if (socketRecvFrom(connfd, NULL, 0, 0, &addr.sa, &slen)) {
				continue;
			}
			listRemoveNode(&channel->_.dgram_ctx.recvlist, cur);
			channel->dgram.m_halfconn_curwaitcnt--;
			halfconn->sockfd = INVALID_FD_HANDLE;
			free_halfconn(halfconn);
			if (connect(connfd, &addr.sa, sockaddrLength(&addr.sa)) != 0) {
				socketClose(connfd);
				continue;
			}
			channel->_.on_ack_halfconn(&channel->_, connfd, &addr.sa, timestamp_msec);
		}
	}
	return 1;
}

static int channel_reliable_dgram_recv_handler(Channel_t* channel, unsigned char* buf, int len, long long timestamp_msec, const struct sockaddr* from_saddr) {
	ReactorPacket_t* packet;
	unsigned char pktype;
	unsigned int pkseq;
	int from_listen, from_peer;
	ChannelInbufDecodeResult_t decode_result;

	memset(&decode_result, 0, sizeof(decode_result));
	channel->on_decode(channel, buf, len, &decode_result);
	if (decode_result.err) {
		return 1;
	}
	else if (decode_result.incomplete) {
		return 1;
	}
	else if (decode_result.ignore) {
		return 1;
	}
	pktype = decode_result.pktype;
	if (0 == pktype) {
		channel->on_recv(channel, from_saddr, &decode_result);
		return 1;
	}
	pkseq = decode_result.pkseq;
	if (channel->_.flag & CHANNEL_FLAG_CLIENT) {
		from_listen = sockaddrIsEqual(&channel->_.connect_addr.sa, from_saddr);
	}
	else {
		from_listen = 0;
	}

	if (from_listen) {
		from_peer = 0;
	}
	else {
		from_peer = sockaddrIsEqual(&channel->_.to_addr.sa, from_saddr);
	}

	if (NETPACKET_SYN_ACK == pktype) {
		int i;
		if (!from_listen) {
			return 1;
		}
		if (decode_result.bodylen < sizeof(unsigned short)) {
			return 1;
		}
		if (channel->dgram.m_synpacket) {
			unsigned short port = *(unsigned short*)decode_result.bodyptr;
			port = ntohs(port);
			sockaddrSetPort(&channel->_.to_addr.sa, port);
			reactorpacketFree(channel->dgram.m_synpacket);
			channel->dgram.m_synpacket = NULL;
			if (~0 != channel->_.connected_times) {
				++channel->_.connected_times;
			}
			channel->_.disable_send = 0;
			if (channel->_.on_syn_ack) {
				channel->_.on_syn_ack(&channel->_, timestamp_msec);
			}
		}
		for (i = 0; i < 5; ++i) {
			channel->dgram.on_reply_ack(channel, 0, from_saddr);
			sendto(channel->_.o->fd, NULL, 0, 0,
				&channel->_.to_addr.sa, sockaddrLength(&channel->_.to_addr.sa));
		}
	}
	else if (!from_peer) {
		return 1;
	}
	else if (dgramtransportctxRecvCheck(&channel->_.dgram_ctx, pkseq, pktype)) {
		List_t list;
		if (channel->_.readcache_max_size > 0 &&
			channel->_.readcache_max_size < channel->_.dgram_ctx.cache_recv_bytes + decode_result.bodylen)
		{
			return 0;
		}
		channel->dgram.on_reply_ack(channel, pkseq, from_saddr);
		packet = reactorpacketMake(pktype, 0, decode_result.bodylen);
		if (!packet) {
			return 0;
		}
		packet->_.seq = pkseq;
		packet->_.fragment_eof = decode_result.fragment_eof;
		memcpy(packet->_.buf, decode_result.bodyptr, packet->_.bodylen);
		dgramtransportctxCacheRecvPacket(&channel->_.dgram_ctx, &packet->_);
		while (dgramtransportctxMergeRecvPacket(&channel->_.dgram_ctx, &list)) {
			if (!channel_merge_packet_handler(channel, &list, &decode_result)) {
				return 0;
			}
		}
	}
	else if (NETPACKET_ACK == pktype) {
		NetPacket_t* ackpkg;
		int cwndskip = dgramtransportctxAckSendPacket(&channel->_.dgram_ctx, pkseq, &ackpkg);
		if (!ackpkg) {
			return 1;
		}
		reactorpacketFree(pod_container_of(ackpkg, ReactorPacket_t, _));
		if (cwndskip) {
			ReactorObject_t* o = channel->_.o;
			const struct sockaddr* addr;
			int addrlen;
			ListNode_t* cur;
			if (o->m_connected) {
				addr = NULL;
				addrlen = 0;
			}
			else {
				addr = &channel->_.to_addr.sa;
				addrlen = sockaddrLength(addr);
			}
			for (cur = channel->_.dgram_ctx.sendlist.head; cur; cur = cur->next) {
				packet = pod_container_of(cur, ReactorPacket_t, _.node);
				if (!dgramtransportctxSendWindowHasPacket(&channel->_.dgram_ctx, &packet->_)) {
					break;
				}
				if (NETPACKET_FIN == packet->_.type) {
					channel->_.has_sendfin = 1;
				}
				if (packet->_.wait_ack && packet->_.resend_msec > timestamp_msec) {
					continue;
				}
				packet->_.wait_ack = 1;
				packet->_.resend_msec = timestamp_msec + channel->dgram.rto;
				channel_set_timestamp(channel, packet->_.resend_msec);
				sendto(o->fd, (char*)packet->_.buf, packet->_.hdrlen + packet->_.bodylen, 0, addr, addrlen);
			}
		}
	}
	else if (NETPACKET_NO_ACK_FRAGMENT == pktype) {
		channel->on_recv(channel, from_saddr, &decode_result);
	}
	else if (pktype >= NETPACKET_DGRAM_HAS_SEND_SEQ) {
		channel->dgram.on_reply_ack(channel, pkseq, from_saddr);
	}
	return 1;
}

static int on_read_stream(ChannelBase_t* base, unsigned char* buf, unsigned int len, unsigned int off, long long timestamp_msec, const struct sockaddr* from_addr) {
	Channel_t* channel = pod_container_of(base, Channel_t, _);
	int res_off = channel_stream_recv_handler(channel, buf, len, off, timestamp_msec);
	if (res_off < 0) {
		channel_invalid(base, REACTOR_ONREAD_ERR);
	}
	else if (base->has_recvfin && base->has_sendfin) {
		channel_invalid(base, 0);
	}
	return res_off;
}

static int on_read_dgram_listener(ChannelBase_t* base, unsigned char* buf, unsigned int len, unsigned int off, long long timestamp_msec, const struct sockaddr* from_addr) {
	Channel_t* channel = pod_container_of(base, Channel_t, _);
	channel_dgram_listener_handler(channel, buf, len, timestamp_msec, from_addr);
	return 1;
}

static int on_read_reliable_dgram(ChannelBase_t* base, unsigned char* buf, unsigned int len, unsigned int off, long long timestamp_msec, const struct sockaddr* from_addr) {
	Channel_t* channel = pod_container_of(base, Channel_t, _);
	if (!channel_reliable_dgram_recv_handler(channel, buf, len, timestamp_msec, from_addr)) {
		channel_invalid(base, REACTOR_ONREAD_ERR);
	}
	else if (base->has_recvfin && base->has_sendfin) {
		channel_invalid(base, 0);
	}
	return 1;
}

static int on_read_dgram(ChannelBase_t* base, unsigned char* buf, unsigned int len, unsigned int off, long long timestamp_msec, const struct sockaddr* from_addr) {
	Channel_t* channel = pod_container_of(base, Channel_t, _);
	ChannelInbufDecodeResult_t decode_result;

	memset(&decode_result, 0, sizeof(decode_result));
	channel->on_decode(channel, buf, len, &decode_result);
	if (decode_result.err) {
		return 1;
	}
	else if (decode_result.incomplete) {
		return 1;
	}
	else if (decode_result.ignore) {
		return 1;
	}
	channel->on_recv(channel, from_addr, &decode_result);
	return 1;
}

static int on_pre_send_stream(ChannelBase_t* base, ReactorPacket_t* packet, long long timestamp_msec) {
	Channel_t* channel = pod_container_of(base, Channel_t, _);
	packet->_.seq = streamtransportctxNextSendSeq(&base->stream_ctx, packet->_.type);
	if (channel->on_encode) {
		ChannelOutbufEncodeParam_t encode_param;
		netpacket2encodeparam(&packet->_, &encode_param);
		channel->on_encode(channel, &encode_param);
	}
	return 1;
}

static int on_pre_send_reliable_dgram(ChannelBase_t* base, ReactorPacket_t* packet, long long timestamp_msec) {
	Channel_t* channel = pod_container_of(base, Channel_t, _);
	packet->_.seq = dgramtransportctxNextSendSeq(&channel->_.dgram_ctx, packet->_.type);
	if (channel->on_encode) {
		ChannelOutbufEncodeParam_t encode_param;
		netpacket2encodeparam(&packet->_, &encode_param);
		channel->on_encode(channel, &encode_param);
	}
	if (dgramtransportctxCacheSendPacket(&channel->_.dgram_ctx, &packet->_)) {
		if (!dgramtransportctxSendWindowHasPacket(&channel->_.dgram_ctx, &packet->_)) {
			return 0;
		}
		if (NETPACKET_FIN == packet->_.type) {
			channel->_.has_sendfin = 1;
		}
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
	return 1;
}

static int on_pre_send_dgram(ChannelBase_t* base, ReactorPacket_t* packet, long long timestamp_msec) {
	Channel_t* channel = pod_container_of(base, Channel_t, _);
	if (channel->on_encode) {
		ChannelOutbufEncodeParam_t encode_param;
		netpacket2encodeparam(&packet->_, &encode_param);
		channel->on_encode(channel, &encode_param);
	}
	return 1;
}

static void on_exec_dgram_listener(ChannelBase_t* base, long long timestamp_msec) {
	Channel_t* channel = pod_container_of(base, Channel_t, _);
	ListNode_t* cur, *next;
	for (cur = channel->_.dgram_ctx.recvlist.head; cur; cur = next) {
		DgramHalfConn_t* halfconn = pod_container_of(cur, DgramHalfConn_t, node);
		next = cur->next;
		if (halfconn->expire_time_msec > timestamp_msec) {
			channel_set_timestamp(channel, halfconn->expire_time_msec);
			break;
		}
		listRemoveNode(&channel->_.dgram_ctx.recvlist, cur);
		channel->dgram.m_halfconn_curwaitcnt--;
		free_halfconn(halfconn);
	}
}

static void on_exec_reliable_dgram(ChannelBase_t* base, long long timestamp_msec) {
	Channel_t* channel = pod_container_of(base, Channel_t, _);
	ReactorObject_t* o = base->o;
	if (channel->dgram.m_synpacket) {
		ReactorPacket_t* packet = channel->dgram.m_synpacket;
		if (packet->_.resend_msec > timestamp_msec) {
			channel_set_timestamp(channel, packet->_.resend_msec);
		}
		else if (packet->_.resend_times >= channel->dgram.resend_maxtimes) {
			reactorpacketFree(channel->dgram.m_synpacket);
			channel->dgram.m_synpacket = NULL;
			channel_invalid(base, REACTOR_CONNECT_ERR);
			return;
		}
		else {
			sendto(o->fd, (char*)packet->_.buf, packet->_.hdrlen + packet->_.bodylen, 0,
				&channel->_.to_addr.sa, sockaddrLength(&channel->_.to_addr.sa));
			packet->_.resend_times++;
			packet->_.resend_msec = timestamp_msec + channel->dgram.rto;
			channel_set_timestamp(channel, packet->_.resend_msec);
		}
	}
	else {
		const struct sockaddr* addr;
		int addrlen;
		ListNode_t* cur;
		if (o->m_connected) {
			addr = NULL;
			addrlen = 0;
		}
		else {
			addr = &base->to_addr.sa;
			addrlen = sockaddrLength(addr);
		}
		for (cur = channel->_.dgram_ctx.sendlist.head; cur; cur = cur->next) {
			ReactorPacket_t* packet = pod_container_of(cur, ReactorPacket_t, _.node);
			if (!packet->_.wait_ack) {
				break;
			}
			if (packet->_.resend_msec > timestamp_msec) {
				channel_set_timestamp(channel, packet->_.resend_msec);
				continue;
			}
			if (packet->_.resend_times >= channel->dgram.resend_maxtimes) {
				int err = (NETPACKET_FIN != packet->_.type ? REACTOR_ZOMBIE_ERR : 0);
				channel_invalid(base, err);
				return;
			}
			sendto(o->fd, (char*)packet->_.buf, packet->_.hdrlen + packet->_.bodylen, 0, addr, addrlen);
			packet->_.resend_times++;
			packet->_.resend_msec = timestamp_msec + channel->dgram.rto;
			channel_set_timestamp(channel, packet->_.resend_msec);
		}
	}
}

static void on_free_dgram_listener(ChannelBase_t* base) {
	Channel_t* channel = pod_container_of(base, Channel_t, _);
	ListNode_t* cur, *next;
	for (cur = base->dgram_ctx.recvlist.head; cur; cur = next) {
		next = cur->next;
		free_halfconn(pod_container_of(cur, DgramHalfConn_t, node));
	}
	listInit(&base->dgram_ctx.recvlist);
	channel->dgram.m_halfconn_curwaitcnt = channel->dgram.halfconn_maxwaitcnt = 0;
}

static void on_free_reliable_dgram(ChannelBase_t* base) {
	Channel_t* channel = pod_container_of(base, Channel_t, _);
	reactorpacketFree(channel->dgram.m_synpacket);
	channel->dgram.m_synpacket = NULL;
}

static void on_reg_reliable_dgram_client(ChannelBase_t* base, long long timestamp_msec) {
	unsigned int hdrsize = base->on_hdrsize(base, 0);
	ReactorPacket_t* packet = reactorpacketMake(NETPACKET_SYN, hdrsize, 0);
	if (!packet) {
		channel_invalid(base, REACTOR_CONNECT_ERR);
		return;
	}
	channelbaseSendPacket(base, packet);
}

/*******************************************************************************/

Channel_t* reactorobjectOpenChannel(ReactorObject_t* o, unsigned short flag, unsigned int extra_sz, const struct sockaddr* addr) {
	Channel_t* channel = (Channel_t*)channelbaseOpen(sizeof(Channel_t) + extra_sz, flag, o, addr);
	if (!channel) {
		return NULL;
	}
	flag = channel->_.flag;
	if (flag & CHANNEL_FLAG_DGRAM) {
		if (flag & CHANNEL_FLAG_LISTEN) {
			channel->dgram.halfconn_maxwaitcnt = 200;
		}
		channel->dgram.rto = 200;
		channel->dgram.resend_maxtimes = 5;
		dgramtransportctxInit(&channel->_.dgram_ctx, 0);
	}
	else {
		streamtransportctxInit(&channel->_.stream_ctx, 0);
	}
	channel->m_initseq = 0;
	if (flag & CHANNEL_FLAG_STREAM) {
		channel->_.on_read = on_read_stream;
		channel->_.on_pre_send = on_pre_send_stream;
	}
	else if (flag & CHANNEL_FLAG_DGRAM) {
		if (flag & CHANNEL_FLAG_LISTEN) {
			channel->_.on_free = on_free_dgram_listener;
			channel->_.on_exec = on_exec_dgram_listener;
			channel->_.on_read = on_read_dgram_listener;
		}
		else if ((flag & CHANNEL_FLAG_CLIENT) || (flag & CHANNEL_FLAG_SERVER)) {
			channel->_.on_free = on_free_reliable_dgram;
			channel->_.on_exec = on_exec_reliable_dgram;
			channel->_.on_read = on_read_reliable_dgram;
			channel->_.on_pre_send = on_pre_send_reliable_dgram;
			if (flag & CHANNEL_FLAG_CLIENT) {
				channel->_.on_reg_proc = on_reg_reliable_dgram_client;
			}
		}
		else {
			channel->_.on_read = on_read_dgram;
			channel->_.on_pre_send = on_pre_send_dgram;
		}
	}
	return channel;
}

Channel_t* channelSendv(Channel_t* channel, const Iobuf_t iov[], unsigned int iovcnt, int pktype) {
	List_t pklist;
	if (NETPACKET_SYN == pktype) {
		if (!(channel->_.flag & CHANNEL_FLAG_CLIENT)) {
			return channel;
		}
		if (!(channel->_.flag & CHANNEL_FLAG_DGRAM) && 0 == channel->_.connected_times) {
			return channel;
		}
	}
	else if (NETPACKET_FIN == pktype) {
		if (_xchg32(&channel->m_has_sendfin, 1)) {
			return channel;
		}
	}
	else if (channel->m_has_sendfin) {
		return channel;
	}

	listInit(&pklist);
	if (!channelbaseShardDatas(&channel->_, pktype, iov, iovcnt, &pklist)) {
		return NULL;
	}
	channelbaseSendPacketList(&channel->_, &pklist);
	return channel;
}

Channel_t* channelSend(Channel_t* channel, const void* data, unsigned int len, int pktype) {
	Iobuf_t iov = iobufStaticInit(data, len);
	return channelSendv(channel, &iov, 1, pktype);
}

#ifdef	__cplusplus
}
#endif
