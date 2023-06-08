//
// Created by hujianzhe on 2019-7-11.
//

#include "../../inc/component/reactor.h"
#include "../../inc/component/net_channel_rw.h"
#include "../../inc/sysapi/error.h"
#include <stdlib.h>
#include <string.h>

typedef struct DgramHalfConn_t {
	ListNode_t node;
	long long expire_time_msec;
	FD_t sockfd;
	Sockaddr_t from_addr;
	unsigned short local_port;
	NetPacket_t syn_ack_pkg;
} DgramHalfConn_t;

extern ReactorPacket_t* reactorpacketMake(int pktype, unsigned int hdrlen, unsigned int bodylen);
extern void reactorpacketFree(ReactorPacket_t* pkg);

/*******************************************************************************/

#ifdef	__cplusplus
extern "C" {
#endif

static void channel_invalid(ChannelBase_t* base, short detach_error) {
	base->valid = 0;
	base->detach_error = detach_error;
}

static void free_halfconn(DgramHalfConn_t* halfconn) {
	if (INVALID_FD_HANDLE != halfconn->sockfd) {
		socketClose(halfconn->sockfd);
	}
	free(halfconn);
}

static void channel_set_timestamp(ChannelBase_t* channel, long long timestamp_msec) {
	if (channel->event_msec > 0 && channel->event_msec <= timestamp_msec) {
		return;
	}
	channel->event_msec = timestamp_msec;
}

static int check_cache_overflow(unsigned int already_cache_bytes, size_t add_bytes, unsigned int max_limit_bytes) {
	if (max_limit_bytes <= 0) {
		return 0;
	}
	if (max_limit_bytes < add_bytes) {
		return 1;
	}
	return already_cache_bytes > max_limit_bytes - add_bytes;
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

static int channel_merge_packet_handler(ChannelRWData_t* rw, List_t* packetlist) {
	ChannelBase_t* channel = rw->channel;
	ReactorPacket_t* packet = pod_container_of(packetlist->tail, ReactorPacket_t, _.node);
	if (NETPACKET_FIN == packet->_.type) {
		ListNode_t* cur, *next;
		for (cur = packetlist->head; cur; cur = next) {
			next = cur->next;
			reactorpacketFree(pod_container_of(cur, ReactorPacket_t, _.node));
		}
		channel->has_recvfin = 1;
		channelbaseSendFin(channel);
		return 1;
	}
	else if (packetlist->head == packetlist->tail) {
		rw->proc->on_recv(channel, packet->_.buf, packet->_.bodylen, &channel->to_addr.sa);
		reactorpacketFree(packet);
	}
	else {
		unsigned int bodylen;
		unsigned char* bodyptr = merge_packet(packetlist, &bodylen);
		if (!bodyptr) {
			return 0;
		}
		rw->proc->on_recv(channel, bodyptr, bodylen, &channel->to_addr.sa);
		free(bodyptr);
	}
	return 1;
}

static int on_read_stream(ChannelRWData_t* rw, unsigned char* buf, unsigned int len, long long timestamp_msec, const struct sockaddr* from_addr) {
	ChannelBase_t* channel = rw->channel;
	ChannelInbufDecodeResult_t decode_result = { 0 };
	unsigned char pktype;

	rw->proc->on_decode(channel, buf, len, &decode_result);
	if (decode_result.err) {
		return -1;
	}
	if (decode_result.incomplete) {
		return 0;
	}
	if (decode_result.ignore) {
		return decode_result.decodelen;
	}
	pktype = decode_result.pktype;
	if (pktype != 0) {
		unsigned int pkseq = decode_result.pkseq;
		StreamTransportCtx_t* ctx = &channel->stream_ctx;
		if (ctx->recvlist.head || !decode_result.fragment_eof) {
			List_t list;
			ReactorPacket_t* packet;
			if (check_cache_overflow(ctx->cache_recv_bytes, decode_result.bodylen, channel->readcache_max_size)) {
				channel->detach_error = REACTOR_CACHE_OVERFLOW_ERR;
				return -1;
			}
			packet = reactorpacketMake(pktype, 0, decode_result.bodylen);
			if (!packet) {
				return -1;
			}
			packet->_.seq = pkseq;
			packet->_.fragment_eof = decode_result.fragment_eof;
			memcpy(packet->_.buf, decode_result.bodyptr, decode_result.bodylen);
			streamtransportctxCacheRecvPacket(ctx, &packet->_);
			if (!decode_result.fragment_eof) {
				return decode_result.decodelen;
			}
			while (streamtransportctxMergeRecvPacket(ctx, &list)) {
				if (!channel_merge_packet_handler(rw, &list)) {
					return -1;
				}
			}
			return decode_result.decodelen;
		}
	}
	rw->proc->on_recv(channel, decode_result.bodyptr, decode_result.bodylen, from_addr);
	return decode_result.decodelen;
}

static int on_read_dgram_listener(ChannelRWData_t* rw, unsigned char* buf, unsigned int len, long long timestamp_msec, const struct sockaddr* from_saddr) {
	ChannelBase_t* channel = rw->channel;
	ChannelInbufDecodeResult_t decode_result = { 0 };
	unsigned char pktype;

	rw->proc->on_decode(channel, buf, len, &decode_result);
	if (decode_result.err) {
		return len;
	}
	if (decode_result.incomplete) {
		return 0;
	}
	if (decode_result.ignore) {
		return decode_result.decodelen;
	}
	pktype = decode_result.pktype;
	if (NETPACKET_SYN == pktype) {
		DgramHalfConn_t* halfconn = NULL;
		NetPacket_t* syn_ack_pkg;
		ListNode_t* cur;
		for (cur = channel->dgram_ctx.recvlist.head; cur; cur = cur->next) {
			halfconn = pod_container_of(cur, DgramHalfConn_t, node);
			if (sockaddrIsEqual(&halfconn->from_addr.sa, from_saddr)) {
				break;
			}
		}
		if (cur) {
			syn_ack_pkg = &halfconn->syn_ack_pkg;
			sendto(channel->o->fd, (char*)syn_ack_pkg->buf, syn_ack_pkg->hdrlen + syn_ack_pkg->bodylen, 0,
				&halfconn->from_addr.sa, sockaddrLength(&halfconn->from_addr.sa));
		}
		else if (rw->dgram.m_halfconn_curwaitcnt >= rw->dgram.halfconn_maxwaitcnt) {
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
				unsigned int buflen, hdrlen, t;

				memcpy(&local_saddr, &channel->listen_addr, sockaddrLength(&channel->listen_addr.sa));
				if (!sockaddrSetPort(&local_saddr.sa, 0)) {
					break;
				}
				o = channel->o;
				new_sockfd = socket(o->domain, channel->socktype, o->protocol);
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
				hdrlen = channel->proc->on_hdrsize(channel, sizeof(local_port));
				buflen = hdrlen + sizeof(local_port);
				halfconn = (DgramHalfConn_t*)malloc(sizeof(DgramHalfConn_t) + buflen);
				if (!halfconn) {
					break;
				}
				halfconn->sockfd = new_sockfd;
				memcpy(&halfconn->from_addr, from_saddr, sockaddrLength(from_saddr));
				halfconn->local_port = local_port;
				t = (rw->dgram.resend_maxtimes + 2) * rw->dgram.rto;
				halfconn->expire_time_msec = timestamp_msec + t;
				listPushNodeBack(&channel->dgram_ctx.recvlist, &halfconn->node);
				rw->dgram.m_halfconn_curwaitcnt++;
				channel_set_timestamp(channel, halfconn->expire_time_msec);

				syn_ack_pkg = &halfconn->syn_ack_pkg;
				memset(syn_ack_pkg, 0, sizeof(*syn_ack_pkg));
				syn_ack_pkg->type = NETPACKET_SYN_ACK;
				syn_ack_pkg->hdrlen = hdrlen;
				syn_ack_pkg->bodylen = sizeof(local_port);
				syn_ack_pkg->seq = 0;
				syn_ack_pkg->fragment_eof = 1;
				rw->proc->on_encode(channel, syn_ack_pkg);
				*(unsigned short*)(syn_ack_pkg->buf + hdrlen) = htons(local_port);
				sendto(o->fd, (char*)syn_ack_pkg->buf, syn_ack_pkg->hdrlen + syn_ack_pkg->bodylen, 0,
					&halfconn->from_addr.sa, sockaddrLength(&halfconn->from_addr.sa));
			} while (0);
			if (!halfconn) {
				free(halfconn);
				if (INVALID_FD_HANDLE != new_sockfd) {
					socketClose(new_sockfd);
				}
				return decode_result.decodelen;
			}
		}
	}
	else if (NETPACKET_ACK == pktype) {
		ListNode_t* cur, *next;
		Sockaddr_t addr;
		socklen_t slen = sizeof(addr.st);
		for (cur = channel->dgram_ctx.recvlist.head; cur; cur = next) {
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
			listRemoveNode(&channel->dgram_ctx.recvlist, cur);
			rw->dgram.m_halfconn_curwaitcnt--;
			halfconn->sockfd = INVALID_FD_HANDLE;
			free_halfconn(halfconn);
			if (connect(connfd, &addr.sa, sockaddrLength(&addr.sa)) != 0) {
				socketClose(connfd);
				continue;
			}
			channel->on_ack_halfconn(channel, connfd, &addr.sa, timestamp_msec);
		}
	}
	return decode_result.decodelen;
}

static void channel_reliable_dgram_continue_send(ChannelRWData_t* rw, long long timestamp_msec) {
	ChannelBase_t* channel = rw->channel;
	ReactorObject_t* o = channel->o;
	const struct sockaddr* addr;
	int addrlen;
	ListNode_t* cur;

	if (o->m_connected) {
		addr = NULL;
		addrlen = 0;
	}
	else {
		addr = &channel->to_addr.sa;
		addrlen = sockaddrLength(addr);
	}
	for (cur = channel->dgram_ctx.sendlist.head; cur; cur = cur->next) {
		NetPacket_t* packet = pod_container_of(cur, NetPacket_t, node);
		if (!dgramtransportctxSendWindowHasPacket(&channel->dgram_ctx, packet)) {
			break;
		}
		if (NETPACKET_FIN == packet->type) {
			channel->has_sendfin = 1;
		}
		if (packet->wait_ack && packet->resend_msec > timestamp_msec) {
			continue;
		}
		packet->wait_ack = 1;
		packet->resend_msec = timestamp_msec + rw->dgram.rto;
		channel_set_timestamp(channel, packet->resend_msec);
		sendto(o->fd, (char*)packet->buf, packet->hdrlen + packet->bodylen, 0, addr, addrlen);
	}
}

static int on_read_reliable_dgram(ChannelRWData_t* rw, unsigned char* buf, unsigned int len, long long timestamp_msec, const struct sockaddr* from_saddr) {
	ReactorPacket_t* packet;
	unsigned char pktype;
	unsigned int pkseq;
	int from_listen, from_peer;
	ChannelInbufDecodeResult_t decode_result = { 0 };
	ChannelBase_t* channel = rw->channel;

	rw->proc->on_decode(channel, buf, len, &decode_result);
	if (decode_result.err) {
		return -1;
	}
	if (decode_result.incomplete) {
		return 0;
	}
	if (decode_result.ignore) {
		return decode_result.decodelen;
	}
	pktype = decode_result.pktype;
	if (0 == pktype) {
		rw->proc->on_recv(channel, decode_result.bodyptr, decode_result.bodylen, from_saddr);
		return decode_result.decodelen;
	}
	pkseq = decode_result.pkseq;
	if (channel->flag & CHANNEL_FLAG_CLIENT) {
		from_listen = sockaddrIsEqual(&channel->connect_addr.sa, from_saddr);
	}
	else {
		from_listen = 0;
	}

	if (from_listen) {
		from_peer = 0;
	}
	else {
		from_peer = sockaddrIsEqual(&channel->to_addr.sa, from_saddr);
	}

	if (NETPACKET_SYN_ACK == pktype) {
		int i, on_syn_ack;
		if (!from_listen) {
			return decode_result.decodelen;
		}
		if (decode_result.bodylen < sizeof(unsigned short)) {
			return decode_result.decodelen;
		}
		on_syn_ack = 0;
		if (rw->dgram.m_synpacket_doing) {
			unsigned short port = *(unsigned short*)decode_result.bodyptr;
			port = ntohs(port);
			sockaddrSetPort(&channel->to_addr.sa, port);
			free(rw->dgram.m_synpacket);
			rw->dgram.m_synpacket = NULL;
			rw->dgram.m_synpacket_doing = 0;
			if (channel->on_syn_ack) {
				channel->on_syn_ack(channel, timestamp_msec);
			}
			on_syn_ack = 1;
		}
		for (i = 0; i < 5; ++i) {
			rw->proc->on_reply_ack(channel, 0, from_saddr);
			sendto(channel->o->fd, NULL, 0, 0,
				&channel->to_addr.sa, sockaddrLength(&channel->to_addr.sa));
		}
		if (on_syn_ack) {
			channel_reliable_dgram_continue_send(rw, timestamp_msec);
		}
	}
	else if (!from_peer) {
		return decode_result.decodelen;
	}
	else if (dgramtransportctxRecvCheck(&channel->dgram_ctx, pkseq, pktype)) {
		List_t list;
		if (check_cache_overflow(channel->dgram_ctx.cache_recv_bytes, decode_result.bodylen, channel->readcache_max_size)) {
			channel->detach_error = REACTOR_CACHE_OVERFLOW_ERR;
			return -1;
		}
		rw->proc->on_reply_ack(channel, pkseq, from_saddr);
		packet = reactorpacketMake(pktype, 0, decode_result.bodylen);
		if (!packet) {
			return -1;
		}
		packet->_.seq = pkseq;
		packet->_.fragment_eof = decode_result.fragment_eof;
		memcpy(packet->_.buf, decode_result.bodyptr, packet->_.bodylen);
		dgramtransportctxCacheRecvPacket(&channel->dgram_ctx, &packet->_);
		while (dgramtransportctxMergeRecvPacket(&channel->dgram_ctx, &list)) {
			if (!channel_merge_packet_handler(rw, &list)) {
				return -1;
			}
		}
	}
	else if (NETPACKET_ACK == pktype) {
		NetPacket_t* ackpkg;
		int cwndskip = dgramtransportctxAckSendPacket(&channel->dgram_ctx, pkseq, &ackpkg);
		if (!ackpkg) {
			return decode_result.decodelen;
		}
		reactorpacketFree(pod_container_of(ackpkg, ReactorPacket_t, _));
		if (cwndskip) {
			channel_reliable_dgram_continue_send(rw, timestamp_msec);
		}
	}
	else if (NETPACKET_NO_ACK_FRAGMENT == pktype) {
		rw->proc->on_recv(channel, decode_result.bodyptr, decode_result.bodylen, from_saddr);
	}
	else if (pktype >= NETPACKET_DGRAM_HAS_SEND_SEQ) {
		rw->proc->on_reply_ack(channel, pkseq, from_saddr);
	}
	return decode_result.decodelen;
}

static int on_pre_send_reliable_dgram(ChannelRWData_t* rw, NetPacket_t* packet, long long timestamp_msec) {
	ChannelBase_t* channel = rw->channel;
	DgramTransportCtx_t* ctx = &channel->dgram_ctx;
	packet->seq = dgramtransportctxNextSendSeq(ctx, packet->type);
	if (rw->proc->on_encode) {
		rw->proc->on_encode(channel, packet);
	}
	if (packet->type < NETPACKET_DGRAM_HAS_SEND_SEQ) {
		return 0 == rw->dgram.m_synpacket_doing;
	}
	if (check_cache_overflow(ctx->cache_send_bytes, packet->hdrlen + packet->bodylen, channel->sendcache_max_size)) {
		channel->valid = 0;
		channel->detach_error = REACTOR_CACHE_OVERFLOW_ERR;
		return 0;
	}
	dgramtransportctxCacheSendPacket(ctx, packet);
	if (rw->dgram.m_synpacket_doing) {
		return 0;
	}
	if (!dgramtransportctxSendWindowHasPacket(ctx, packet)) {
		return 0;
	}
	if (NETPACKET_FIN == packet->type) {
		channel->has_sendfin = 1;
	}
	packet->wait_ack = 1;
	packet->resend_msec = timestamp_msec + rw->dgram.rto;
	channel_set_timestamp(channel, packet->resend_msec);
	return 1;
}

static void on_exec_dgram_listener(ChannelRWData_t* rw, long long timestamp_msec) {
	ChannelBase_t* channel = rw->channel;
	ListNode_t* cur, *next;
	for (cur = channel->dgram_ctx.recvlist.head; cur; cur = next) {
		DgramHalfConn_t* halfconn = pod_container_of(cur, DgramHalfConn_t, node);
		next = cur->next;
		if (halfconn->expire_time_msec > timestamp_msec) {
			channel_set_timestamp(channel, halfconn->expire_time_msec);
			break;
		}
		listRemoveNode(&channel->dgram_ctx.recvlist, cur);
		rw->dgram.m_halfconn_curwaitcnt--;
		free_halfconn(halfconn);
	}
}

static void on_exec_reliable_dgram(ChannelRWData_t* rw, long long timestamp_msec) {
	ChannelBase_t* channel = rw->channel;
	ReactorObject_t* o = channel->o;
	const struct sockaddr* addr;
	int addrlen;
	ListNode_t* cur;

	if (rw->dgram.m_synpacket_doing) {
		NetPacket_t* packet = rw->dgram.m_synpacket;
		if (!packet) {
			unsigned int hdrlen = channel->proc->on_hdrsize(channel, 0);
			packet = (NetPacket_t*)calloc(1, sizeof(NetPacket_t) + hdrlen);
			if (!packet) {
				channel_invalid(channel, REACTOR_CONNECT_ERR);
				return;
			}
			packet->type = NETPACKET_SYN;
			packet->fragment_eof = 1;
			packet->cached = 1;
			packet->wait_ack = 1;
			packet->hdrlen = hdrlen;
			packet->bodylen = 0;
			if (rw->proc->on_encode) {
				rw->proc->on_encode(channel, packet);
			}
			rw->dgram.m_synpacket = packet;
		}
		if (packet->resend_msec > timestamp_msec) {
			channel_set_timestamp(channel, packet->resend_msec);
			return;
		}
		if (packet->resend_times >= rw->dgram.resend_maxtimes) {
			free(rw->dgram.m_synpacket);
			rw->dgram.m_synpacket = NULL;
			channel_invalid(channel, REACTOR_CONNECT_ERR);
			return;
		}
		sendto(o->fd, (char*)packet->buf, packet->hdrlen + packet->bodylen, 0,
			&channel->connect_addr.sa, sockaddrLength(&channel->connect_addr.sa));
		packet->resend_times++;
		packet->resend_msec = timestamp_msec + rw->dgram.rto;
		channel_set_timestamp(channel, packet->resend_msec);
		return;
	}

	if (o->m_connected) {
		addr = NULL;
		addrlen = 0;
	}
	else {
		addr = &channel->to_addr.sa;
		addrlen = sockaddrLength(addr);
	}
	for (cur = channel->dgram_ctx.sendlist.head; cur; cur = cur->next) {
		NetPacket_t* packet = pod_container_of(cur, NetPacket_t, node);
		if (!packet->wait_ack) {
			break;
		}
		if (packet->resend_msec > timestamp_msec) {
			channel_set_timestamp(channel, packet->resend_msec);
			continue;
		}
		if (packet->resend_times >= rw->dgram.resend_maxtimes) {
			int err = (NETPACKET_FIN != packet->type ? REACTOR_ZOMBIE_ERR : 0);
			channel_invalid(channel, err);
			return;
		}
		sendto(o->fd, (char*)packet->buf, packet->hdrlen + packet->bodylen, 0, addr, addrlen);
		packet->resend_times++;
		packet->resend_msec = timestamp_msec + rw->dgram.rto;
		channel_set_timestamp(channel, packet->resend_msec);
	}
}

static void on_free_dgram_listener(ChannelRWData_t* rw) {
	ChannelBase_t* channel = rw->channel;
	ListNode_t* cur, *next;
	for (cur = channel->dgram_ctx.recvlist.head; cur; cur = next) {
		next = cur->next;
		free_halfconn(pod_container_of(cur, DgramHalfConn_t, node));
	}
	listInit(&channel->dgram_ctx.recvlist);
}

static void on_free_reliable_dgram(ChannelRWData_t* rw) {
	free(rw->dgram.m_synpacket);
	rw->dgram.m_synpacket = NULL;
}

static int on_read(ChannelRWData_t* rw, unsigned char* buf, unsigned int len, long long timestamp_msec, const struct sockaddr* from_addr) {
	ChannelBase_t* channel = rw->channel;
	ChannelInbufDecodeResult_t decode_result = { 0 };

	rw->proc->on_decode(channel, buf, len, &decode_result);
	if (decode_result.err) {
		return -1;
	}
	if (decode_result.incomplete) {
		return 0;
	}
	if (decode_result.ignore) {
		return decode_result.decodelen;
	}
	rw->proc->on_recv(channel, decode_result.bodyptr, decode_result.bodylen, from_addr);
	return decode_result.decodelen;
}

static int on_pre_send(ChannelRWData_t* rw, NetPacket_t* packet, long long timestamp_msec) {
	if (rw->proc->on_encode) {
		rw->proc->on_encode(rw->channel, packet);
	}
	return 1;
}

/*******************************************************************************/

static ChannelRWHookProc_t hook_proc_dgram_listener = {
	on_read_dgram_listener,
	NULL,
	on_exec_dgram_listener,
	on_free_dgram_listener
};

static ChannelRWHookProc_t hook_proc_reliable_dgram = {
	on_read_reliable_dgram,
	on_pre_send_reliable_dgram,
	on_exec_reliable_dgram,
	on_free_reliable_dgram
};

static ChannelRWHookProc_t hook_proc_stream = {
	on_read_stream,
	on_pre_send,
	NULL,
	NULL
};

static ChannelRWHookProc_t hook_proc_normal = {
	on_read,
	on_pre_send,
	NULL,
	NULL
};

void channelrwInitData(ChannelRWData_t* rw, int channel_flag, int socktype, const ChannelRWDataProc_t* proc) {
	memset(rw, 0, sizeof(*rw));
	if (SOCK_DGRAM == socktype) {
		if (channel_flag & CHANNEL_FLAG_LISTEN) {
			rw->dgram.halfconn_maxwaitcnt = 200;
			rw->dgram.rto = 200;
			rw->dgram.resend_maxtimes = 5;
		}
		else if ((channel_flag & CHANNEL_FLAG_CLIENT) || (channel_flag & CHANNEL_FLAG_SERVER)) {
			if (channel_flag & CHANNEL_FLAG_CLIENT) {
				rw->dgram.m_synpacket_doing = 1;
			}
			rw->dgram.rto = 200;
			rw->dgram.resend_maxtimes = 5;
		}
	}
	rw->proc = proc;
}

const ChannelRWHookProc_t* channelrwGetHookProc(int channel_flag, int socktype) {
	if (SOCK_DGRAM == socktype) {
		if (channel_flag & CHANNEL_FLAG_LISTEN) {
			return &hook_proc_dgram_listener;
		}
		else if ((channel_flag & CHANNEL_FLAG_CLIENT) || (channel_flag & CHANNEL_FLAG_SERVER)) {
			return &hook_proc_reliable_dgram;
		}
	}
	else if (SOCK_STREAM == socktype) {
		return &hook_proc_stream;
	}
	return &hook_proc_normal;
}

void channelbaseUseRWData(ChannelBase_t* channel, ChannelRWData_t* rw) {
	if (SOCK_DGRAM == channel->socktype) {
		if (channel->flag & CHANNEL_FLAG_CLIENT) {
			channel->event_msec = 1;
		}
	}
	channel->write_fragment_with_hdr = 1;

	rw->channel = channel;
}

#ifdef	__cplusplus
}
#endif
