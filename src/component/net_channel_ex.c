//
// Created by hujianzhe on 2019-7-11.
//

#include "../../inc/component/net_reactor.h"
#include "../../inc/component/net_channel_ex.h"
#include "../../inc/sysapi/error.h"
#include <stdlib.h>
#include <string.h>

typedef struct DgramHalfConn_t {
	ListNode_t node;
	long long expire_time_msec;
	Sockaddr_t from_addr;
	socklen_t from_addrlen;
	NetPacket_t pkg;
} DgramHalfConn_t;

extern NetReactorPacket_t* reactorpacketMake(int pktype, unsigned int hdrlen, unsigned int bodylen, const struct sockaddr* addr, socklen_t addrlen);
extern void reactorpacketFree(NetReactorPacket_t* pkg);

/*******************************************************************************/

#ifdef	__cplusplus
extern "C" {
#endif

static void channel_invalid(NetChannel_t* base, short detach_error) {
	base->valid = 0;
	base->detach_error = detach_error;
}

static void free_halfconn(DgramHalfConn_t* halfconn) {
	free(halfconn);
}

static void channel_set_timestamp(NetChannel_t* channel, long long timestamp_msec) {
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
	NetReactorPacket_t* packet;
	ListNode_t* cur, *next;
	unsigned int off;
	off = 0;
	for (cur = list->head; cur; cur = next) {
		packet = pod_container_of(cur, NetReactorPacket_t, _.node);
		next = cur->next;
		off += packet->_.bodylen;
	}
	ptr = (unsigned char*)malloc(off);
	if (ptr) {
		*mergelen = off;
		off = 0;
		for (cur = list->head; cur; cur = next) {
			packet = pod_container_of(cur, NetReactorPacket_t, _.node);
			next = cur->next;
			memmove(ptr + off, packet->_.buf + packet->_.hdrlen, packet->_.bodylen);
			off += packet->_.bodylen;
			reactorpacketFree(packet);
		}
	}
	return ptr;
}

static int channel_merge_packet_handler(NetChannel_t* channel, List_t* packetlist, const struct sockaddr* from_addr, socklen_t addrlen) {
	NetChannelExData_t* rw = (NetChannelExData_t*)channel->m_ext_impl;
	NetReactorPacket_t* packet = pod_container_of(packetlist->tail, NetReactorPacket_t, _.node);
	if (NETPACKET_FIN == packet->_.type) {
		ListNode_t* cur, *next;
		for (cur = packetlist->head; cur; cur = next) {
			next = cur->next;
			reactorpacketFree(pod_container_of(cur, NetReactorPacket_t, _.node));
		}
		channel->has_recvfin = 1;
		NetChannel_send_fin(channel);
		return 1;
	}
	else if (packetlist->head == packetlist->tail) {
		rw->proc->on_recv(channel, packet->_.buf, packet->_.bodylen, from_addr, addrlen);
		reactorpacketFree(packet);
	}
	else {
		unsigned int bodylen;
		unsigned char* bodyptr = merge_packet(packetlist, &bodylen);
		if (!bodyptr) {
			return 0;
		}
		rw->proc->on_recv(channel, bodyptr, bodylen, from_addr, addrlen);
		free(bodyptr);
	}
	return 1;
}

static int on_read_stream(NetChannel_t* channel, unsigned char* buf, unsigned int len, long long timestamp_msec, const struct sockaddr* from_addr, socklen_t addrlen) {
	NetChannelExData_t* rw = (NetChannelExData_t*)channel->m_ext_impl;
	NetChannelInbufDecodeResult_t decode_result = { 0 };
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
			NetReactorPacket_t* packet;
			if (check_cache_overflow(ctx->cache_recv_bytes, decode_result.bodylen, channel->readcache_max_size)) {
				channel->detach_error = NET_REACTOR_CACHE_READ_OVERFLOW_ERR;
				return -1;
			}
			packet = reactorpacketMake(pktype, 0, decode_result.bodylen, NULL, 0);
			if (!packet) {
				return -1;
			}
			packet->_.seq = pkseq;
			packet->_.fragment_eof = decode_result.fragment_eof;
			memmove(packet->_.buf, decode_result.bodyptr, decode_result.bodylen);
			streamtransportctxCacheRecvPacket(ctx, &packet->_);
			if (!decode_result.fragment_eof) {
				return decode_result.decodelen;
			}
			while (streamtransportctxMergeRecvPacket(ctx, &list)) {
				if (!channel_merge_packet_handler(channel, &list, from_addr, addrlen)) {
					return -1;
				}
			}
			return decode_result.decodelen;
		}
	}
	rw->proc->on_recv(channel, decode_result.bodyptr, decode_result.bodylen, from_addr, addrlen);
	return decode_result.decodelen;
}

static int on_read_dgram_listener(NetChannel_t* channel, unsigned char* buf, unsigned int len, long long timestamp_msec, const struct sockaddr* from_saddr, socklen_t addrlen) {
	NetChannelExData_t* rw = (NetChannelExData_t*)channel->m_ext_impl;
	NetChannelInbufDecodeResult_t decode_result = { 0 };
	NetReactorObject_t* o;
	unsigned char pktype;
	ListNode_t* cur;
	DgramHalfConn_t* halfconn;
	NetPacket_t* pkg;
	FD_t new_sockfd;
	unsigned int hdrlen;
	struct sockaddr_storage local_ss;
	socklen_t local_sslen;
	unsigned short local_port;

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
	if (NETPACKET_SYN != pktype) {
		return decode_result.decodelen;
	}
	o = channel->o;
	for (cur = channel->dgram_ctx.recvlist.head; cur; cur = cur->next) {
		DgramHalfConn_t* halfconn = pod_container_of(cur, DgramHalfConn_t, node);
		if (sockaddrIsEqual(&halfconn->from_addr.sa, from_saddr)) {
			pkg = &halfconn->pkg;
			sendto(o->niofd.fd, (char*)pkg->buf, pkg->hdrlen + pkg->bodylen, 0, from_saddr, addrlen);
			return decode_result.decodelen;
		}
	}
	if (rw->dgram.m_halfconn_curwaitcnt >= rw->dgram.halfconn_maxwaitcnt) {
		/* TODO return rst, now let client syn timeout */
		return decode_result.decodelen;
	}

	new_sockfd = socket(channel->listen_addr.sa.sa_family, channel->socktype, 0);
	if (INVALID_FD_HANDLE == new_sockfd) {
		return decode_result.decodelen;
	}
	memmove(&local_ss, &channel->listen_addr, channel->listen_addrlen);
	sockaddrSetPort((struct sockaddr*)&local_ss, 0);
	if (bind(new_sockfd, (struct sockaddr*)&local_ss, channel->listen_addrlen)) {
		socketClose(new_sockfd);
		return decode_result.decodelen;
	}
	local_sslen = sizeof(local_ss);
	if (getsockname(new_sockfd, (struct sockaddr*)&local_ss, &local_sslen)) {
		socketClose(new_sockfd);
		return decode_result.decodelen;
	}
	if (!sockaddrDecode((struct sockaddr*)&local_ss, NULL, &local_port)) {
		socketClose(new_sockfd);
		return decode_result.decodelen;
	}

	hdrlen = channel->proc->on_hdrsize(channel, sizeof(local_port));
	halfconn = (DgramHalfConn_t*)malloc(sizeof(DgramHalfConn_t) + hdrlen + sizeof(local_port));
	if (!halfconn) {
		socketClose(new_sockfd);
		return decode_result.decodelen;
	}
	pkg = &halfconn->pkg;
	memset(pkg, 0, sizeof(*pkg));
	pkg->type = NETPACKET_SYN_ACK;
	pkg->hdrlen = hdrlen;
	pkg->bodylen = sizeof(local_port);
	pkg->fragment_eof = 1;
	rw->proc->on_encode(channel, pkg);
	*(unsigned short*)(pkg->buf + hdrlen) = htons(local_port);
	sendto(o->niofd.fd, (char*)pkg->buf, pkg->hdrlen + pkg->bodylen, 0, from_saddr, addrlen);

	memmove(&halfconn->from_addr, from_saddr, addrlen);
	halfconn->from_addrlen = addrlen;
	halfconn->expire_time_msec = timestamp_msec + (unsigned int)rw->dgram.rto * 5;
	listPushNodeBack(&channel->dgram_ctx.recvlist, &halfconn->node);
	rw->dgram.m_halfconn_curwaitcnt++;
	channel_set_timestamp(channel, halfconn->expire_time_msec);

	channel->on_ack_halfconn(channel, new_sockfd, from_saddr, addrlen, timestamp_msec);
	return decode_result.decodelen;
}

static void channel_reliable_dgram_continue_send(NetChannel_t* channel, long long timestamp_msec) {
	NetChannelExData_t* rw = (NetChannelExData_t*)channel->m_ext_impl;
	NetReactorObject_t* o = channel->o;
	const struct sockaddr* addr;
	int addrlen;
	ListNode_t* cur;

	if (o->m_connected) {
		addr = NULL;
		addrlen = 0;
	}
	else {
		addr = &channel->to_addr.sa;
		addrlen = channel->to_addrlen;
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
		sendto(o->niofd.fd, (char*)packet->buf, packet->hdrlen + packet->bodylen, 0, addr, addrlen);
	}
}

static int on_read_reliable_dgram(NetChannel_t* channel, unsigned char* buf, unsigned int len, long long timestamp_msec, const struct sockaddr* from_saddr, socklen_t addrlen) {
	NetChannelExData_t* rw = (NetChannelExData_t*)channel->m_ext_impl;
	unsigned char pktype;
	unsigned int pkseq;
	NetChannelInbufDecodeResult_t decode_result = { 0 };

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
		rw->proc->on_recv(channel, decode_result.bodyptr, decode_result.bodylen, from_saddr, addrlen);
		return decode_result.decodelen;
	}
	if (NETPACKET_SYN_ACK == pktype) {
		if (NET_CHANNEL_SIDE_CLIENT == channel->side) {
			if (1 == rw->dgram.m_synpacket_status) {
				NetReactorObject_t* o;
				NetPacket_t* synpkg;
				unsigned short port;
				if (!sockaddrIsEqual(&channel->connect_addr.sa, from_saddr)) {
					return decode_result.decodelen;
				}
				if (decode_result.bodylen < sizeof(port)) {
					return decode_result.decodelen;
				}
				port = *(unsigned short*)decode_result.bodyptr;
				port = ntohs(port);
				memmove(&channel->to_addr, from_saddr, addrlen);
				channel->to_addrlen = addrlen;
				sockaddrSetPort(&channel->to_addr.sa, port);
				o = channel->o;
				if (connect(o->niofd.fd, &channel->to_addr.sa, channel->to_addrlen)) {
					channel_invalid(channel, NET_REACTOR_IO_CONNECT_ERR);
					return decode_result.decodelen;
				}
				o->m_connected = 1;

				synpkg = rw->dgram.m_synpacket;
				synpkg->type = NETPACKET_SYN_ACK;
				if (rw->proc->on_encode) {
					rw->proc->on_encode(channel, synpkg);
				}
				sendto(o->niofd.fd, (char*)synpkg->buf, synpkg->hdrlen + synpkg->bodylen, 0, NULL, 0);
				rw->dgram.m_synpacket_status = 2;
				if (channel->on_syn_ack) {
					channel->on_syn_ack(channel, timestamp_msec);
				}
				channel_reliable_dgram_continue_send(channel, timestamp_msec);
			}
		}
		else if (NET_CHANNEL_SIDE_SERVER == channel->side) {
			if (1 == rw->dgram.m_synpacket_status) {
				NetReactorObject_t* o = channel->o;
				memmove(&channel->to_addr, from_saddr, addrlen);
				channel->to_addrlen = addrlen;
				if (!connect(o->niofd.fd, from_saddr, addrlen)) {
					o->m_connected = 1;
				}
				rw->dgram.m_synpacket_status = 0;
				channel_reliable_dgram_continue_send(channel, timestamp_msec);
			}
		}
		return decode_result.decodelen;
	}
	pkseq = decode_result.pkseq;
	if (dgramtransportctxRecvCheck(&channel->dgram_ctx, pkseq, pktype)) {
		NetReactorPacket_t* packet;
		List_t list;
		if (check_cache_overflow(channel->dgram_ctx.cache_recv_bytes, decode_result.bodylen, channel->readcache_max_size)) {
			channel->detach_error = NET_REACTOR_CACHE_READ_OVERFLOW_ERR;
			return -1;
		}
		rw->proc->on_reply_ack(channel, pkseq, from_saddr, addrlen);
		packet = reactorpacketMake(pktype, 0, decode_result.bodylen, NULL, 0);
		if (!packet) {
			return -1;
		}
		packet->_.seq = pkseq;
		packet->_.fragment_eof = decode_result.fragment_eof;
		memmove(packet->_.buf, decode_result.bodyptr, packet->_.bodylen);
		dgramtransportctxCacheRecvPacket(&channel->dgram_ctx, &packet->_);
		while (dgramtransportctxMergeRecvPacket(&channel->dgram_ctx, &list)) {
			if (!channel_merge_packet_handler(channel, &list, from_saddr, addrlen)) {
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
		reactorpacketFree(pod_container_of(ackpkg, NetReactorPacket_t, _));
		if (cwndskip) {
			channel_reliable_dgram_continue_send(channel, timestamp_msec);
		}
	}
	else if (NETPACKET_NO_ACK_FRAGMENT == pktype) {
		rw->proc->on_recv(channel, decode_result.bodyptr, decode_result.bodylen, from_saddr, addrlen);
	}
	else if (pktype >= NETPACKET_DGRAM_HAS_SEND_SEQ) {
		rw->proc->on_reply_ack(channel, pkseq, from_saddr, addrlen);
	}
	return decode_result.decodelen;
}

static int on_pre_send_reliable_dgram(NetChannel_t* channel, NetPacket_t* packet, long long timestamp_msec) {
	NetChannelExData_t* rw = (NetChannelExData_t*)channel->m_ext_impl;
	DgramTransportCtx_t* ctx = &channel->dgram_ctx;
	packet->seq = dgramtransportctxNextSendSeq(ctx, packet->type);
	if (rw->proc->on_encode) {
		rw->proc->on_encode(channel, packet);
	}
	if (packet->type < NETPACKET_DGRAM_HAS_SEND_SEQ) {
		return 1 != rw->dgram.m_synpacket_status;
	}
	if (check_cache_overflow(ctx->cache_send_bytes, packet->hdrlen + packet->bodylen, channel->sendcache_max_size)) {
		channel->valid = 0;
		channel->detach_error = NET_REACTOR_CACHE_WRITE_OVERFLOW_ERR;
		return 0;
	}
	dgramtransportctxCacheSendPacket(ctx, packet);
	if (1 == rw->dgram.m_synpacket_status) {
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

static void on_exec_dgram_listener(NetChannel_t* channel, long long timestamp_msec) {
	NetChannelExData_t* rw = (NetChannelExData_t*)channel->m_ext_impl;
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

static NetPacket_t* new_reliable_dgram_syn_packet(NetChannel_t* channel) {
	unsigned int hdrlen = channel->proc->on_hdrsize(channel, 0);
	NetPacket_t* packet = (NetPacket_t*)calloc(1, sizeof(NetPacket_t) + hdrlen);
	if (!packet) {
		return NULL;
	}
	packet->type = NETPACKET_SYN;
	packet->fragment_eof = 1;
	packet->cached = 1;
	packet->wait_ack = 1;
	packet->hdrlen = hdrlen;
	packet->bodylen = 0;
	return packet;
}

static void on_exec_reliable_dgram(NetChannel_t* channel, long long timestamp_msec) {
	NetChannelExData_t* rw = (NetChannelExData_t*)channel->m_ext_impl;
	NetReactorObject_t* o = channel->o;
	const struct sockaddr* addr;
	int addrlen;
	ListNode_t* cur;

	if (NET_CHANNEL_SIDE_CLIENT == channel->side) {
		if (1 == rw->dgram.m_synpacket_status) {
			NetPacket_t* packet = rw->dgram.m_synpacket;
			if (!packet) {
				packet = new_reliable_dgram_syn_packet(channel);
				if (!packet) {
					channel_invalid(channel, NET_REACTOR_IO_CONNECT_ERR);
					return;
				}
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
				channel_invalid(channel, NET_REACTOR_IO_CONNECT_ERR);
				return;
			}
			addr = &channel->connect_addr.sa;
			addrlen = channel->connect_addrlen;
			sendto(o->niofd.fd, (char*)packet->buf, packet->hdrlen + packet->bodylen, 0, addr, addrlen);
			packet->resend_times++;
			packet->resend_msec = timestamp_msec + rw->dgram.rto;
			channel_set_timestamp(channel, packet->resend_msec);
			return;
		}
		if (2 == rw->dgram.m_synpacket_status) {
			NetPacket_t* packet = rw->dgram.m_synpacket;
			if (packet->resend_msec > timestamp_msec) {
				channel_set_timestamp(channel, packet->resend_msec);
				return;
			}
			if (packet->resend_times >= rw->dgram.resend_maxtimes) {
				free(rw->dgram.m_synpacket);
				rw->dgram.m_synpacket = NULL;
				rw->dgram.m_synpacket_status = 0;
				return;
			}
			sendto(o->niofd.fd, (char*)packet->buf, packet->hdrlen + packet->bodylen, 0, NULL, 0);
			packet->resend_times++;
			packet->resend_msec = timestamp_msec + rw->dgram.rto;
			channel_set_timestamp(channel, packet->resend_msec);
			return;
		}
	}
	if (o->m_connected) {
		addr = NULL;
		addrlen = 0;
	}
	else {
		addr = &channel->to_addr.sa;
		addrlen = channel->to_addrlen;
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
			int err = (NETPACKET_FIN != packet->type ? NET_REACTOR_ZOMBIE_ERR : 0);
			channel_invalid(channel, err);
			return;
		}
		sendto(o->niofd.fd, (char*)packet->buf, packet->hdrlen + packet->bodylen, 0, addr, addrlen);
		packet->resend_times++;
		packet->resend_msec = timestamp_msec + rw->dgram.rto;
		channel_set_timestamp(channel, packet->resend_msec);
	}
}

static void on_free_dgram_listener(NetChannel_t* channel) {
	NetChannelExData_t* rw = (NetChannelExData_t*)channel->m_ext_impl;
	ListNode_t* cur, *next;
	for (cur = channel->dgram_ctx.recvlist.head; cur; cur = next) {
		next = cur->next;
		free_halfconn(pod_container_of(cur, DgramHalfConn_t, node));
	}
	listInit(&channel->dgram_ctx.recvlist);
}

static void on_free_reliable_dgram(NetChannel_t* channel) {
	NetChannelExData_t* rw = (NetChannelExData_t*)channel->m_ext_impl;
	free(rw->dgram.m_synpacket);
	rw->dgram.m_synpacket = NULL;
}

static int on_read(NetChannel_t* channel, unsigned char* buf, unsigned int len, long long timestamp_msec, const struct sockaddr* from_addr, socklen_t addrlen) {
	NetChannelExData_t* rw = (NetChannelExData_t*)channel->m_ext_impl;
	NetChannelInbufDecodeResult_t decode_result = { 0 };

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
	rw->proc->on_recv(channel, decode_result.bodyptr, decode_result.bodylen, from_addr, addrlen);
	return decode_result.decodelen;
}

static int on_pre_send(NetChannel_t* channel, NetPacket_t* packet, long long timestamp_msec) {
	NetChannelExData_t* rw = (NetChannelExData_t*)channel->m_ext_impl;
	if (rw->proc->on_encode) {
		rw->proc->on_encode(channel, packet);
	}
	return 1;
}

/*******************************************************************************/

static NetChannelExHookProc_t hook_proc_dgram_listener = {
	on_read_dgram_listener,
	NULL,
	on_exec_dgram_listener,
	on_free_dgram_listener
};

static NetChannelExHookProc_t hook_proc_reliable_dgram = {
	on_read_reliable_dgram,
	on_pre_send_reliable_dgram,
	on_exec_reliable_dgram,
	on_free_reliable_dgram
};

static NetChannelExHookProc_t hook_proc_stream = {
	on_read_stream,
	on_pre_send,
	NULL,
	NULL
};

static NetChannelExHookProc_t hook_proc_normal = {
	on_read,
	on_pre_send,
	NULL,
	NULL
};

const NetChannelExHookProc_t* NetChannelEx_get_hook(int channel_side, int socktype) {
	if (SOCK_STREAM == socktype) {
		return &hook_proc_stream;
	}
	if (SOCK_DGRAM == socktype) {
		if (NET_CHANNEL_SIDE_LISTEN == channel_side) {
			return &hook_proc_dgram_listener;
		}
		if (NET_CHANNEL_SIDE_CLIENT == channel_side || NET_CHANNEL_SIDE_SERVER == channel_side) {
			return &hook_proc_reliable_dgram;
		}
	}
	return &hook_proc_normal;
}

void NetChannelEx_init(NetChannel_t* channel, NetChannelExData_t* rw, const NetChannelExProc_t* rw_proc) {
	memset(rw, 0, sizeof(*rw));
	rw->proc = rw_proc;
	if (SOCK_DGRAM == channel->socktype) {
		if (NET_CHANNEL_SIDE_LISTEN == channel->side) {
			rw->dgram.m_halfconn_curwaitcnt = 0;
			rw->dgram.halfconn_maxwaitcnt = 200;
			rw->dgram.rto = 200;
		}
		else if (NET_CHANNEL_SIDE_SERVER == channel->side) {
			rw->dgram.m_synpacket_status = 1;
			rw->dgram.resend_maxtimes = 5;
			rw->dgram.rto = 200;
		}
		else if (NET_CHANNEL_SIDE_CLIENT == channel->side) {
			channel->event_msec = 1;
			rw->dgram.m_synpacket_status = 1;
			rw->dgram.resend_maxtimes = 5;
			rw->dgram.rto = 200;
		}
	}
	channel->write_fragment_with_hdr = 1;
	channel->m_ext_impl = rw;
}

#ifdef	__cplusplus
}
#endif
