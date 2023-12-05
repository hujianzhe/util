//
// Created by hujianzhe
//

#include "../../inc/component/reactor.h"
#include "../../inc/sysapi/io_overlapped.h"
#include "../../inc/sysapi/error.h"
#include "../../inc/sysapi/time.h"
#include "../../inc/sysapi/misc.h"
#include <stdlib.h>
#include <string.h>

enum {
	REACTOR_CHANNEL_REG_CMD = 1,
	REACTOR_CHANNEL_FREE_CMD,
	REACTOR_STREAM_SENDFIN_CMD,
	REACTOR_SEND_PACKET_CMD
};

typedef struct Reactor_t {
	/* private */
	long long m_event_msec;
	Nio_t m_nio;
	CriticalSection_t m_cmdlistlock;
	List_t m_cmdlist;
	List_t m_connect_endlist;
} Reactor_t;

ReactorPacket_t* reactorpacketMake(int pktype, unsigned int hdrlen, unsigned int bodylen, const struct sockaddr* addr, socklen_t addrlen) {
	ReactorPacket_t* pkg = (ReactorPacket_t*)malloc(sizeof(ReactorPacket_t) + hdrlen + bodylen);
	if (!pkg) {
		return NULL;
	}
	if (addr && addrlen > 0) {
		pkg->addr = (struct sockaddr*)malloc(addrlen);
		if (!pkg->addr) {
			free(pkg);
			return NULL;
		}
		pkg->addrlen = addrlen;
		memmove(pkg->addr, addr, addrlen);
	}
	else {
		pkg->addr = NULL;
		pkg->addrlen = 0;
	}
	pkg->cmd.type = REACTOR_SEND_PACKET_CMD;
	pkg->channel = NULL;
	pkg->_.type = pktype;
	pkg->_.wait_ack = 0;
	pkg->_.cached = 0;
	pkg->_.fragment_eof = 1;
	pkg->_.hdrlen = hdrlen;
	pkg->_.bodylen = bodylen;
	pkg->_.seq = 0;
	pkg->_.off = 0;
	pkg->_.resend_msec = 0;
	pkg->_.resend_times = 0;
	pkg->_.buf[hdrlen + bodylen] = 0;
	return pkg;
}

void reactorpacketFree(ReactorPacket_t* pkg) {
	if (pkg) {
		free(pkg->addr);
		free(pkg);
	}
}

/*****************************************************************************************/

static void reactor_set_event_timestamp(Reactor_t* reactor, long long timestamp_msec) {
	if (timestamp_msec <= 0) {
		return;
	}
	if (reactor->m_event_msec > 0 && reactor->m_event_msec <= timestamp_msec) {
		return;
	}
	reactor->m_event_msec = timestamp_msec;
}

static void channel_set_timestamp(ChannelBase_t* channel, long long timestamp_msec) {
	if (timestamp_msec <= 0) {
		return;
	}
	if (channel->event_msec > 0 && channel->event_msec <= timestamp_msec) {
		return;
	}
	channel->event_msec = timestamp_msec;
	reactor_set_event_timestamp(channel->reactor, timestamp_msec);
}

static void channel_next_heartbeat_timestamp(ChannelBase_t* channel, long long timestamp_msec) {
	if (channel->heartbeat_timeout_sec > 0) {
		long long ts = channel->heartbeat_timeout_sec;
		ts *= 1000;
		ts += timestamp_msec;
		channel->m_heartbeat_msec = ts;
		channel_set_timestamp(channel, ts);
	}
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

static int after_call_channel_interface(ChannelBase_t* channel) {
	if (channel->valid) {
		reactor_set_event_timestamp(channel->reactor, channel->event_msec);
		return 1;
	}
	return 0;
}

static void free_inbuf(ReactorObject_t* o) {
	if (o->m_inbuf) {
		free(o->m_inbuf);
		o->m_inbuf = NULL;
		o->m_inbuflen = 0;
		o->m_inbufoff = 0;
		o->m_inbufsize = 0;
	}
}

static void reactorobject_free(ReactorObject_t* o) {
	free_inbuf(o);
	free(o);
}

static void nio_free_niofd(NioFD_t* niofd) {
	reactorobject_free(pod_container_of(niofd, ReactorObject_t, niofd));
}

static void packetlist_free_packet(List_t* packetlist) {
	ListNode_t* cur, *next;
	for (cur = packetlist->head; cur; cur = next) {
		next = cur->next;
		reactorpacketFree(pod_container_of(cur, ReactorPacket_t, _.node));
	}
	listInit(packetlist);
}

static void channelobject_free(ChannelBase_t* channel) {
	if (SOCK_STREAM == channel->socktype) {
		packetlist_free_packet(&channel->stream_ctx.recvlist);
		packetlist_free_packet(&channel->stream_ctx.sendlist);
	}
	else {
		packetlist_free_packet(&channel->dgram_ctx.recvlist);
		packetlist_free_packet(&channel->dgram_ctx.sendlist);
	}
	free(channel);
}

static int cmp_sorted_reactor_object_stream_connect_timeout(ListNode_t* node, ListNode_t* new_node) {
	ReactorObject_t* o = pod_container_of(node, ReactorObject_t, stream.m_connect_endnode);
	ReactorObject_t* new_o = pod_container_of(new_node, ReactorObject_t, stream.m_connect_endnode);
	return o->stream.m_connect_end_msec > new_o->stream.m_connect_end_msec;
}

static void reactorobject_invalid_inner_handler(Reactor_t* reactor, ChannelBase_t* channel, long long now_msec) {
	ReactorObject_t* o;
	if (channel->m_has_detached) {
		return;
	}
	o = channel->o;
	o->m_channel = NULL;
	if (SOCK_STREAM == channel->socktype) {
		if (o->stream.m_connect_end_msec > 0) {
			listRemoveNode(&reactor->m_connect_endlist, &o->stream.m_connect_endnode);
			o->stream.m_connect_end_msec = 0;
		}
	}
	free_inbuf(o);
	niofdDelete(&reactor->m_nio, &o->niofd);

	channel->m_has_detached = 1;
	channel->o = NULL;
	channel->valid = 0;
	channel->proc->on_detach(channel);
}

static int reactor_reg_object_check(Reactor_t* reactor, ChannelBase_t* channel, ReactorObject_t* o, long long timestamp_msec) {
	struct sockaddr_storage saddr;
	socklen_t saddrlen = sizeof(saddr);
	if (SOCK_STREAM == channel->socktype) {
		if (CHANNEL_SIDE_LISTEN == channel->side) {
			if (getsockname(o->niofd.fd, (struct sockaddr*)&saddr, &saddrlen)) {
				return 0;
			}
			memmove(&channel->listen_addr, &saddr, saddrlen);
			channel->listen_addrlen = saddrlen;
			if (!nioCommit(&reactor->m_nio, &o->niofd, NIO_OP_ACCEPT, NULL, 0)) {
				return 0;
			}
		}
		else if (!socketIsConnected(o->niofd.fd, (struct sockaddr*)&saddr, &saddrlen)) {
			return 0;
		}
		else if (saddr.ss_family != AF_UNSPEC) {
			o->m_connected = 1;
			memmove(&channel->to_addr, &saddr, saddrlen);
			channel->to_addrlen = saddrlen;
			memmove(&channel->connect_addr, &saddr, saddrlen);
			channel->connect_addrlen = saddrlen;
			if (!nioCommit(&reactor->m_nio, &o->niofd, NIO_OP_READ, NULL, 0)) {
				return 0;
			}
		}
		else {
			o->m_connected = 0;
			if (!nioCommit(&reactor->m_nio, &o->niofd, NIO_OP_CONNECT, &channel->connect_addr.sa, channel->connect_addrlen)) {
				return 0;
			}
			if (channel->connect_timeout_sec > 0) {
				o->stream.m_connect_end_msec = channel->connect_timeout_sec;
				o->stream.m_connect_end_msec *= 1000;
				o->stream.m_connect_end_msec += timestamp_msec;
				listInsertNodeSorted(&reactor->m_connect_endlist, &o->stream.m_connect_endnode,
					cmp_sorted_reactor_object_stream_connect_timeout);
				reactor_set_event_timestamp(reactor, o->stream.m_connect_end_msec);
			}
			else {
				o->stream.m_connect_end_msec = 0;
			}
		}
	}
	else if (SOCK_DGRAM == channel->socktype) {
		if (!socketIsConnected(o->niofd.fd, (struct sockaddr*)&saddr, &saddrlen)) {
			return 0;
		}
		if (saddr.ss_family != AF_UNSPEC) {
			o->m_connected = 1;
			memmove(&channel->to_addr, &saddr, saddrlen);
			channel->to_addrlen = saddrlen;
		}
		else {
			o->m_connected = 0;
		}
		if (!nioCommit(&reactor->m_nio, &o->niofd, NIO_OP_READ, NULL, 0)) {
			return 0;
		}
	}
	return 1;
}

static int reactor_reg_object(Reactor_t* reactor, ChannelBase_t* channel, long long timestamp_msec) {
	ReactorObject_t* o = channel->o;
	if (reactor_reg_object_check(reactor, channel, o, timestamp_msec)) {
		return 1;
	}
	return 0;
}

static void reactor_exec_object_reg_callback(Reactor_t* reactor, ChannelBase_t* channel, long long timestamp_msec) {
	ReactorObject_t* o;
	channel->reactor = reactor;
	reactor_set_event_timestamp(reactor, channel->event_msec);
	if (SOCK_STREAM != channel->socktype) {
		channel_next_heartbeat_timestamp(channel, timestamp_msec);
		return;
	}
	if (CHANNEL_SIDE_LISTEN == channel->side) {
		return;
	}
	o = channel->o;
	if (!o->m_connected) {
		return;
	}
	if (CHANNEL_SIDE_CLIENT == channel->side) {
		if (channel->on_syn_ack) {
			channel->on_syn_ack(channel, timestamp_msec);
			if (!after_call_channel_interface(channel)) {
				return;
			}
		}
	}
	channel_next_heartbeat_timestamp(channel, timestamp_msec);
}

static void stream_sendfin_handler(Reactor_t* reactor, ChannelBase_t* channel) {
	StreamTransportCtx_t* ctx = &channel->stream_ctx;

	channel->m_catch_fincmd = 1;
	if (ctx->sendlist.head) {
		channel->m_stream_delay_send_fin = 1;
		return;
	}
	shutdown(channel->o->niofd.fd, SHUT_WR);
	channel->has_sendfin = 1;
	if (channel->has_recvfin) {
		channel->valid = 0;
	}
}

static void stream_recvfin_handler(Reactor_t* reactor, ChannelBase_t* channel, long long timestamp_msec) {
	channel->has_recvfin = 1;
	if (channel->has_sendfin) {
		channel->valid = 0;
		return;
	}
	if (_xchg8(&channel->m_has_commit_fincmd, 1)) {
		return;
	}
	stream_sendfin_handler(reactor, channel);
}

static int channel_heartbeat_handler(ChannelBase_t* channel, long long now_msec) {
	if (channel->m_heartbeat_msec <= 0) {
		return 1;
	}
	if (channel->m_heartbeat_msec > now_msec) {
		channel_set_timestamp(channel, channel->m_heartbeat_msec);
		return 1;
	}
	if (CHANNEL_SIDE_SERVER == channel->side) {
		return 0;
	}
	if (CHANNEL_SIDE_CLIENT == channel->side) {
		if (channel->m_heartbeat_times >= channel->heartbeat_maxtimes) {
			return 0;
		}
		if (channel->proc->on_heartbeat) {
			channel->proc->on_heartbeat(channel, channel->m_heartbeat_times);
		}
		channel->m_heartbeat_times++;
		channel_next_heartbeat_timestamp(channel, now_msec);
	}
	return 1;
}

static void reactor_exec_object(Reactor_t* reactor, long long now_msec) {
	NioFD_t* cur, *next;
	for (cur = reactor->m_nio.__alive_list_head; cur; cur = next) {
		ReactorObject_t* o = pod_container_of(cur, ReactorObject_t, niofd);
		ChannelBase_t* channel = o->m_channel;
		next = cur->__lnext;

		if (!channel->valid) {
			reactorobject_invalid_inner_handler(reactor, channel, now_msec);
			continue;
		}
		if (channel->event_msec <= 0) {
			continue;
		}
		if (now_msec < channel->event_msec) {
			reactor_set_event_timestamp(reactor, channel->event_msec);
			continue;
		}
		channel->event_msec = 0;
		if (!channel_heartbeat_handler(channel, now_msec)) {
			channel->valid = 0;
			channel->detach_error = REACTOR_ZOMBIE_ERR;
		}
		else if (channel->proc->on_exec) {
			channel->proc->on_exec(channel, now_msec);
			after_call_channel_interface(channel);
		}
		if (channel->valid) {
			continue;
		}
		reactorobject_invalid_inner_handler(reactor, channel, now_msec);
	}
}

static void reactor_exec_connect_timeout(Reactor_t* reactor, long long now_msec) {
	ListNode_t* cur, *next;
	for (cur = reactor->m_connect_endlist.head; cur; cur = next) {
		ReactorObject_t* o = pod_container_of(cur, ReactorObject_t, stream.m_connect_endnode);
		ChannelBase_t* channel;
		next = cur->next;
		if (o->stream.m_connect_end_msec > now_msec) {
			reactor_set_event_timestamp(reactor, o->stream.m_connect_end_msec);
			break;
		}
		channel = o->m_channel;
		channel->valid = 0;
		channel->detach_error = REACTOR_IO_CONNECT_ERR;
		reactorobject_invalid_inner_handler(reactor, channel, now_msec);
	}
}

static void reactor_stream_writeev(Reactor_t* reactor, ChannelBase_t* channel, ReactorObject_t* o) {
	ListNode_t* cur, *next;
	StreamTransportCtx_t* ctxptr = &channel->stream_ctx;
	Iobuf_t iov[8];
	size_t iov_cnt = 0;
	int res;
	size_t iov_wnd_bytes = 4096;

	for (cur = ctxptr->sendlist.head; cur && iov_cnt < sizeof(iov)/sizeof(iov[0]); cur = next) {
		NetPacket_t* packet = pod_container_of(cur, NetPacket_t, node);
		size_t pkg_left_bytes = packet->hdrlen + packet->bodylen - packet->off;
		next = cur->next;

		iobufPtr(iov + iov_cnt) = (char*)(packet->buf + packet->off);
		if (pkg_left_bytes >= iov_wnd_bytes) {
			iobufLen(iov + iov_cnt) = iov_wnd_bytes;
			iov_cnt++;
			iov_wnd_bytes = 0;
			break;
		}
		iobufLen(iov + iov_cnt) = pkg_left_bytes;
		iov_cnt++;
		iov_wnd_bytes -= pkg_left_bytes;
	}

	if (iov_cnt > 0) {
		res = socketWritev(o->niofd.fd, iov, iov_cnt, 0, NULL, 0);
		if (res < 0) {
			if (errnoGet() != EWOULDBLOCK) {
				channel->valid = 0;
				channel->detach_error = REACTOR_IO_WRITE_ERR;
				return;
			}
			res = 0;
		}
	}
	else {
		res = 0;
	}

	for (cur = ctxptr->sendlist.head; cur; cur = next) {
		NetPacket_t* packet = pod_container_of(cur, NetPacket_t, node);
		size_t pkg_left_bytes = packet->hdrlen + packet->bodylen - packet->off;
		next = cur->next;

		if (res >= pkg_left_bytes) {
			res -= pkg_left_bytes;
			streamtransportctxRemoveCacheSendPacket(ctxptr, packet);
			reactorpacketFree(pod_container_of(cur, ReactorPacket_t, _.node));
			continue;
		}
		packet->off += res;
		if (nioCommit(&reactor->m_nio, &o->niofd, NIO_OP_WRITE, NULL, 0)) {
			break;
		}
		channel->valid = 0;
		channel->detach_error = REACTOR_IO_WRITE_ERR;
		return;
	}
	if (ctxptr->sendlist.head) {
		return;
	}
	if (!channel->m_stream_delay_send_fin) {
		return;
	}
	shutdown(o->niofd.fd, SHUT_WR);
	channel->has_sendfin = 1;
	if (channel->has_recvfin) {
		channel->valid = 0;
	}
}

static void reactor_stream_accept(ChannelBase_t* channel, ReactorObject_t* o, long long timestamp_msec) {
	Sockaddr_t saddr;
	socklen_t slen = sizeof(saddr.st);
	FD_t connfd;
	for (connfd = nioAccept(&o->niofd, &saddr.sa, &slen);
		connfd != INVALID_FD_HANDLE;
		connfd = nioAccept(&o->niofd, &saddr.sa, &slen))
	{
		channel->on_ack_halfconn(channel, connfd, &saddr.sa, slen, timestamp_msec);
		slen = sizeof(saddr.st);
	}
}

static void reactor_stream_readev(Reactor_t* reactor, ChannelBase_t* channel, ReactorObject_t* o, long long timestamp_msec) {
	int res = socketTcpReadableBytes(o->niofd.fd);
	if (res < 0) {
		channel->valid = 0;
		channel->detach_error = REACTOR_IO_READ_ERR;
		return;
	}
	if (0 == res) {
		stream_recvfin_handler(reactor, channel, timestamp_msec);
		return;
	}
	if (check_cache_overflow(o->m_inbuflen, res, o->inbuf_maxlen)) {
		channel->valid = 0;
		channel->detach_error = REACTOR_CACHE_READ_OVERFLOW_ERR;
		return;
	}
	if (o->m_inbufsize < o->m_inbuflen + res) {
		unsigned char* ptr = (unsigned char*)realloc(o->m_inbuf, o->m_inbuflen + res + 1);
		if (!ptr) {
			channel->valid = 0;
			return;
		}
		o->m_inbuf = ptr;
		o->m_inbufsize = o->m_inbuflen + res;
	}
	res = recv(o->niofd.fd, (char*)(o->m_inbuf + o->m_inbuflen), res, 0);
	if (res < 0) {
		if (errnoGet() != EWOULDBLOCK) {
			channel->valid = 0;
			channel->detach_error = REACTOR_IO_READ_ERR;
		}
		return;
	}
	if (0 == res) {
		stream_recvfin_handler(reactor, channel, timestamp_msec);
		return;
	}
	o->m_inbuflen += res;
	o->m_inbuf[o->m_inbuflen] = 0; /* convienent for text data */
	while (o->m_inbufoff < o->m_inbuflen) {
		res = channel->proc->on_read(channel, o->m_inbuf + o->m_inbufoff, o->m_inbuflen - o->m_inbufoff,
				timestamp_msec, &channel->connect_addr.sa, channel->connect_addrlen);
		if (res < 0 || !after_call_channel_interface(channel)) {
			channel->valid = 0;
			return;
		}
		if (0 == res) {
			break;
		}
		o->m_inbufoff += res;
	}
	channel->m_heartbeat_times = 0;
	if (CHANNEL_SIDE_SERVER == channel->side) {
		channel_next_heartbeat_timestamp(channel, timestamp_msec);
	}
	if (o->m_inbufoff >= o->m_inbuflen) {
		if (o->inbuf_saved) {
			o->m_inbufoff = 0;
			o->m_inbuflen = 0;
		}
		else {
			free_inbuf(o);
		}
	}
	else if (o->m_inbufoff > 0 && o->m_inbuflen - o->m_inbufoff <= 512) { /* temp default 512 bytes */
		memmove(o->m_inbuf, o->m_inbuf + o->m_inbufoff, o->m_inbuflen - o->m_inbufoff);
		o->m_inbuflen -= o->m_inbufoff;
		o->m_inbufoff = 0;
	}
}

static void reactor_dgram_readev(Reactor_t* reactor, ChannelBase_t* channel, ReactorObject_t* o, long long timestamp_msec) {
	unsigned int readtimes;
	if (!o->m_inbuf) {
		o->m_inbuf = (unsigned char*)malloc((size_t)o->inbuf_maxlen + 1);
		if (!o->m_inbuf) {
			channel->valid = 0;
			channel->detach_error = REACTOR_IO_READ_ERR;
			return;
		}
		o->m_inbuflen = o->m_inbufsize = o->inbuf_maxlen;
	}
	for (readtimes = 0; readtimes < 8; ++readtimes) {
		int len, off;
		unsigned char* ptr;
		Sockaddr_t from_addr;
		socklen_t slen;

		slen = sizeof(from_addr.st);
		len = socketRecvFrom(o->niofd.fd, (char*)o->m_inbuf, o->m_inbuflen, 0, &from_addr.sa, &slen);
		if (len < 0) {
			if (errnoGet() != EWOULDBLOCK) {
				channel->valid = 0;
				channel->detach_error = REACTOR_IO_READ_ERR;
				return;
			}
			break;
		}
		ptr = o->m_inbuf;
		ptr[len] = 0; /* convienent for text data */
		off = 0;
		do { /* dgram can recv 0 bytes packet */
			int res = channel->proc->on_read(channel, ptr + off, len - off, timestamp_msec, &from_addr.sa, slen);
			if (res < 0) {
				channel->valid = 0;
				return;
			}
			if (!after_call_channel_interface(channel)) {
				return;
			}
			if (0 == res) {
				break;
			}
			off += res;
		} while (off < len);
	}
	if (readtimes > 0) {
		channel->m_heartbeat_times = 0;
		if (CHANNEL_SIDE_SERVER == channel->side) {
			channel_next_heartbeat_timestamp(channel, timestamp_msec);
		}
	}
	if (!o->inbuf_saved) {
		free_inbuf(o);
	}
}

static void reactor_packet_send_proc_stream(Reactor_t* reactor, ReactorPacket_t* packet, long long timestamp_msec) {
	int packet_allow_send;
	ChannelBase_t* channel = packet->channel;
	ReactorObject_t* o;
	StreamTransportCtx_t* ctx;

	if (!channel->valid || channel->m_catch_fincmd) {
		if (!packet->_.cached) {
			reactorpacketFree(packet);
		}
		return;
	}
	o = channel->o;
	if (!o->m_connected) {
		packet_allow_send = 0;
	}
	else {
		packet_allow_send = 1;
	}

	if (channel->proc->on_pre_send) {
		int continue_send = channel->proc->on_pre_send(channel, &packet->_, timestamp_msec);
		if (!after_call_channel_interface(channel)) {
			if (!packet->_.cached) {
				reactorpacketFree(packet);
			}
			return;
		}
		if (!continue_send) {
			if (!packet->_.cached) {
				reactorpacketFree(packet);
			}
			return;
		}
	}
	ctx = &channel->stream_ctx;
	packet->_.off = 0;
	if (packet_allow_send) {
		if (!streamtransportctxSendCheckBusy(ctx)) {
			int res = send(o->niofd.fd, (char*)packet->_.buf, packet->_.hdrlen + packet->_.bodylen, 0);
			if (res < 0) {
				if (errnoGet() != EWOULDBLOCK) {
					if (!packet->_.cached) {
						reactorpacketFree(packet);
					}
					channel->valid = 0;
					channel->detach_error = REACTOR_IO_WRITE_ERR;
					return;
				}
				res = 0;
			}
			packet->_.off = res;
		}
	}
	if (packet->_.off < packet->_.hdrlen + packet->_.bodylen) {
		if (check_cache_overflow(ctx->cache_send_bytes, packet->_.hdrlen + packet->_.bodylen, channel->sendcache_max_size)) {
			if (!packet->_.cached) {
				reactorpacketFree(packet);
			}
			channel->valid = 0;
			channel->detach_error = REACTOR_CACHE_WRITE_OVERFLOW_ERR;
			return;
		}
	}
	if (streamtransportctxCacheSendPacket(ctx, &packet->_)) {
		if (!nioCommit(&reactor->m_nio, &o->niofd, NIO_OP_WRITE, NULL, 0)) {
			channel->valid = 0;
			channel->detach_error = REACTOR_IO_WRITE_ERR;
		}
		return;
	}
	reactorpacketFree(packet);
}

static void reactor_packet_send_proc_dgram(Reactor_t* reactor, ReactorPacket_t* packet, long long timestamp_msec) {
	ChannelBase_t* channel = packet->channel;
	ReactorObject_t* o;
	struct sockaddr* paddr;
	int addrlen;

	if (!channel->valid || channel->m_catch_fincmd) {
		if (!packet->_.cached) {
			reactorpacketFree(packet);
		}
		return;
	}
	if (NETPACKET_FIN == packet->_.type) {
		channel->m_catch_fincmd = 1;
	}
	o = channel->o;
	if (channel->proc->on_pre_send) {
		int continue_send = channel->proc->on_pre_send(channel, &packet->_, timestamp_msec);
		if (!after_call_channel_interface(channel)) {
			if (!packet->_.cached) {
				reactorpacketFree(packet);
			}
			return;
		}
		if (!continue_send) {
			if (!packet->_.cached) {
				reactorpacketFree(packet);
			}
			return;
		}
	}
	if (o->m_connected) {
		paddr = NULL;
		addrlen = 0;
	}
	else {
		paddr = packet->addr;
		addrlen = packet->addrlen;
	}
	sendto(o->niofd.fd, (char*)packet->_.buf, packet->_.hdrlen + packet->_.bodylen, 0, paddr, addrlen);
	if (!packet->_.cached) {
		reactorpacketFree(packet);
	}
}

static int reactor_stream_connect(Reactor_t* reactor, ChannelBase_t* channel, ReactorObject_t* o, long long timestamp_msec) {
	if (o->stream.m_connect_end_msec > 0) {
		listRemoveNode(&reactor->m_connect_endlist, &o->stream.m_connect_endnode);
		o->stream.m_connect_end_msec = 0;
	}
	if (nioConnectUpdate(&o->niofd)) {
		return 0;
	}
	if (!nioCommit(&reactor->m_nio, &o->niofd, NIO_OP_READ, NULL, 0)) {
		return 0;
	}
	o->m_connected = 1;
	if (channel->on_syn_ack) {
		channel->on_syn_ack(channel, timestamp_msec);
		if (!after_call_channel_interface(channel)) {
			return 0;
		}
	}
	channel_next_heartbeat_timestamp(channel, timestamp_msec);
	reactor_stream_writeev(reactor, channel, o);
	return 1;
}

static void reactor_exec_cmdlist(Reactor_t* reactor, long long timestamp_msec) {
	ListNode_t* cur, *next;
	criticalsectionEnter(&reactor->m_cmdlistlock);
	cur = reactor->m_cmdlist.head;
	listInit(&reactor->m_cmdlist);
	criticalsectionLeave(&reactor->m_cmdlistlock);
	for (; cur; cur = next) {
		ReactorCmd_t* cmd = pod_container_of(cur, ReactorCmd_t, _);
		next = cur->next;
		if (REACTOR_SEND_PACKET_CMD == cmd->type) {
			ReactorPacket_t* packet = pod_container_of(cmd, ReactorPacket_t, cmd);
			ChannelBase_t* channel = packet->channel;
			if (SOCK_STREAM == channel->socktype) {
				reactor_packet_send_proc_stream(reactor, packet, timestamp_msec);
			}
			else {
				reactor_packet_send_proc_dgram(reactor, packet, timestamp_msec);
			}
			if (!channel->valid) {
				reactorobject_invalid_inner_handler(reactor, channel, timestamp_msec);
				continue;
			}
			continue;
		}
		else if (REACTOR_STREAM_SENDFIN_CMD == cmd->type) {
			ChannelBase_t* channel = pod_container_of(cmd, ChannelBase_t, m_stream_fincmd);
			if (!channel->valid) {
				continue;
			}
			stream_sendfin_handler(reactor, channel);
			if (!channel->valid) {
				reactorobject_invalid_inner_handler(reactor, channel, timestamp_msec);
			}
			continue;
		}
		else if (REACTOR_CHANNEL_REG_CMD == cmd->type) {
			ChannelBase_t* channel = pod_container_of(cmd, ChannelBase_t, m_regcmd);
			if (!reactor_reg_object(reactor, channel, timestamp_msec)) {
				channel->valid = 0;
				channel->detach_error = REACTOR_REG_ERR;
				reactorobject_invalid_inner_handler(reactor, channel, timestamp_msec);
				continue;
			}
			reactor_exec_object_reg_callback(reactor, channel, timestamp_msec);
			if (!channel->valid) {
				reactorobject_invalid_inner_handler(reactor, channel, timestamp_msec);
				continue;
			}
			continue;
		}
		else if (REACTOR_CHANNEL_FREE_CMD == cmd->type) {
			ChannelBase_t* channel = pod_container_of(cmd, ChannelBase_t, m_freecmd);
			channelobject_free(channel);
			continue;
		}
	}
}

static void reactor_commit_cmd(Reactor_t* reactor, ReactorCmd_t* cmdnode) {
	criticalsectionEnter(&reactor->m_cmdlistlock);
	listInsertNodeBack(&reactor->m_cmdlist, reactor->m_cmdlist.tail, &cmdnode->_);
	criticalsectionLeave(&reactor->m_cmdlistlock);
	reactorWake(reactor);
}

static void reactor_commit_cmdlist(Reactor_t* reactor, List_t* cmdlist) {
	criticalsectionEnter(&reactor->m_cmdlistlock);
	listAppend(&reactor->m_cmdlist, cmdlist);
	criticalsectionLeave(&reactor->m_cmdlistlock);
	reactorWake(reactor);
}

static void reactorpacketFreeList(List_t* pkglist) {
	if (pkglist) {
		ListNode_t* cur, *next;
		for (cur = pkglist->head; cur; cur = next) {
			next = cur->next;
			reactorpacketFree(pod_container_of(cur, ReactorPacket_t, cmd._));
		}
		listInit(pkglist);
	}
}

static void reactorobject_init_comm(ReactorObject_t* o, FD_t fd, int domain) {
	o->inbuf_maxlen = 0;
	o->inbuf_saved = 1;

	o->m_connected = 0;
	o->m_channel = NULL;
	o->m_inbuf = NULL;
	o->m_inbuflen = 0;
	o->m_inbufoff = 0;
	o->m_inbufsize = 0;
	niofdInit(&o->niofd, fd, domain);
}

static ReactorObject_t* reactorobjectOpen(FD_t fd, int domain, int socktype, int protocol) {
	int fd_create;
	ReactorObject_t* o = (ReactorObject_t*)malloc(sizeof(ReactorObject_t));
	if (!o) {
		return NULL;
	}
	if (INVALID_FD_HANDLE == fd) {
		fd = socket(domain, socktype, protocol);
		if (INVALID_FD_HANDLE == fd) {
			free(o);
			return NULL;
		}
		fd_create = 1;
	}
	else {
		fd_create = 0;
	}
	if (!socketNonBlock(fd, TRUE)) {
		if (fd_create) {
			socketClose(fd);
		}
		free(o);
		return NULL;
	}
	reactorobject_init_comm(o, fd, domain);
	if (SOCK_STREAM == socktype) {
		memset(&o->stream, 0, sizeof(o->stream));
	}
	else {
		o->inbuf_maxlen = 1464;
	}
	return o;
}

static List_t* channelbaseShardDatas(ChannelBase_t* channel, int pktype, const Iobuf_t iov[], unsigned int iovcnt, List_t* packetlist, const struct sockaddr* addr, socklen_t addrlen) {
	unsigned int i, nbytes = 0;
	unsigned int hdrsz;
	ReactorPacket_t* packet;
	List_t pklist;
	unsigned int(*fn_on_hdrsize)(ChannelBase_t*, unsigned int);

	listInit(&pklist);
	for (i = 0; i < iovcnt; ++i) {
		nbytes += iobufLen(iov + i);
	}
	fn_on_hdrsize = channel->proc->on_hdrsize;
	if (nbytes) {
		unsigned int off = 0;
		size_t iov_i = 0, iov_off = 0;
		unsigned int write_fragment_size = channel->write_fragment_size;
		unsigned int shardsz = write_fragment_size;

		hdrsz = (fn_on_hdrsize ? fn_on_hdrsize(channel, shardsz) : 0);
		if (shardsz <= hdrsz) {
			return NULL;
		}
		shardsz -= hdrsz;
		packet = NULL;
		while (off < nbytes) {
			unsigned int sz = nbytes - off;
			if (sz > shardsz) {
				sz = shardsz;
			}
			if (fn_on_hdrsize) {
				hdrsz = fn_on_hdrsize(channel, sz);
			}
			packet = reactorpacketMake(pktype, hdrsz, sz, addr, addrlen);
			if (!packet) {
				break;
			}
			packet->channel = channel;
			packet->_.fragment_eof = 0;
			listPushNodeBack(&pklist, &packet->cmd._);
			off += sz;
			iobufShardCopy(iov, iovcnt, &iov_i, &iov_off, packet->_.buf + packet->_.hdrlen, packet->_.bodylen);
			if (channel->write_fragment_with_hdr) {
				continue;
			}
			shardsz = write_fragment_size;
			hdrsz = 0;
			fn_on_hdrsize = NULL;
		}
		if (!packet) {
			reactorpacketFreeList(&pklist);
			return NULL;
		}
		packet->_.fragment_eof = 1;
	}
	else {
		hdrsz = (fn_on_hdrsize ? fn_on_hdrsize(channel, 0) : 0);
		if (SOCK_STREAM == channel->socktype) {
			if (0 == hdrsz) {
				return packetlist;
			}
			addr = NULL;
			addrlen = 0;
		}
		packet = reactorpacketMake(pktype, hdrsz, 0, addr, addrlen);
		if (!packet) {
			return NULL;
		}
		packet->channel = channel;
		listPushNodeBack(&pklist, &packet->cmd._);
	}
	listAppend(packetlist, &pklist);
	return packetlist;
}

static void channelbaseInit(ChannelBase_t* channel, unsigned short side, const ChannelBaseProc_t* proc, int domain, int socktype, int protocol) {
	channel->o = NULL;
	channel->reactor = NULL;
	channel->domain = domain;
	channel->socktype = socktype;
	channel->protocol = protocol;
	channel->to_addr.sa.sa_family = AF_UNSPEC;
	channel->to_addrlen = 0;
	channel->heartbeat_timeout_sec = 0;
	channel->heartbeat_maxtimes = 0;
	channel->has_recvfin = 0;
	channel->has_sendfin = 0;
	channel->valid = 1;
	channel->write_fragment_with_hdr = 0;
	channel->side = side;
	channel->detach_error = 0;
	channel->event_msec = 0;
	channel->readcache_max_size = 0;
	channel->sendcache_max_size = 0;
	channel->userdata = NULL;
	channel->proc = proc;
	channel->session = NULL;
	if (CHANNEL_SIDE_CLIENT == side) {
		channel->on_syn_ack = NULL;
		channel->connect_addr.sa.sa_family = AF_UNSPEC;
		channel->connect_addrlen = 0;
		channel->connect_timeout_sec = 0;
	}
	else if (CHANNEL_SIDE_LISTEN == side) {
		channel->on_ack_halfconn = NULL;
		channel->listen_addr.sa.sa_family = AF_UNSPEC;
		channel->listen_addrlen = 0;
	}
	if (SOCK_STREAM == socktype) {
		streamtransportctxInit(&channel->stream_ctx);
		channel->m_stream_fincmd.type = REACTOR_STREAM_SENDFIN_CMD;
		channel->m_stream_delay_send_fin = 0;
		channel->write_fragment_size = ~0;
	}
	else {
		dgramtransportctxInit(&channel->dgram_ctx, 0);
		channel->write_fragment_size = 548;
	}
	channel->m_ext_impl = NULL;
	channel->m_heartbeat_msec = 0;
	channel->m_heartbeat_times = 0;
	channel->m_refcnt = 1;
	channel->m_has_detached = 0;
	channel->m_catch_fincmd = 0;
	channel->m_has_commit_fincmd = 0;
	channel->m_reghaspost = 0;
	channel->m_regcmd.type = REACTOR_CHANNEL_REG_CMD;
	channel->m_freecmd.type = REACTOR_CHANNEL_FREE_CMD;
}

static void reactor_free_cmdlist(Reactor_t* reactor) {
	ListNode_t* lcur, *lnext;
	for (lcur = reactor->m_cmdlist.head; lcur; lcur = lnext) {
		ReactorCmd_t* cmd = pod_container_of(lcur, ReactorCmd_t, _);
		lnext = lcur->next;
		if (REACTOR_CHANNEL_REG_CMD == cmd->type) {
			ChannelBase_t* channel = pod_container_of(cmd, ChannelBase_t, m_regcmd);
			if (channel->proc->on_free) {
				channel->proc->on_free(channel);
			}
			reactorobject_free(channel->o);
			channelobject_free(channel);
		}
		else if (REACTOR_CHANNEL_FREE_CMD == cmd->type) {
			ChannelBase_t* channel = pod_container_of(cmd, ChannelBase_t, m_freecmd);
			channelobject_free(channel);
		}
		else if (REACTOR_SEND_PACKET_CMD == cmd->type) {
			reactorpacketFree(pod_container_of(cmd, ReactorPacket_t, cmd));
		}
	}
}

static void reactor_free_alive_objects(Reactor_t* reactor) {
	NioFD_t* cur, *next;
	for (cur = reactor->m_nio.__alive_list_head; cur; cur = next) {
		ReactorObject_t* o = pod_container_of(cur, ReactorObject_t, niofd);
		ChannelBase_t* channel = o->m_channel;
		next = cur->__lnext;

		if (!channel) {
			continue;
		}
		if (channel->proc->on_free) {
			channel->proc->on_free(channel);
		}
		channelobject_free(channel);
	}
}

/*****************************************************************************************/

#ifdef	__cplusplus
extern "C" {
#endif

Reactor_t* reactorCreate(void) {
	Reactor_t* reactor = (Reactor_t*)malloc(sizeof(Reactor_t));
	if (!reactor) {
		return NULL;
	}
	if (!nioCreate(&reactor->m_nio, nio_free_niofd)) {
		free(reactor);
		return NULL;
	}
	if (!criticalsectionCreate(&reactor->m_cmdlistlock)) {
		nioClose(&reactor->m_nio);
		free(reactor);
		return NULL;
	}
	listInit(&reactor->m_cmdlist);
	listInit(&reactor->m_connect_endlist);
	reactor->m_event_msec = 0;
	return reactor;
}

void reactorWake(Reactor_t* reactor) {
	nioWakeup(&reactor->m_nio);
}

int reactorHandle(Reactor_t* reactor, NioEv_t e[], int n, int wait_msec) {
	long long timestamp_msec = gmtimeMillisecond();
	if (reactor->m_event_msec > timestamp_msec) {
		int checkexpire_wait_msec = reactor->m_event_msec - timestamp_msec;
		if (checkexpire_wait_msec < wait_msec || wait_msec < 0) {
			wait_msec = checkexpire_wait_msec;
		}
	}
	else if (reactor->m_event_msec) {
		wait_msec = 0;
	}

	n = nioWait(&reactor->m_nio, e, n, wait_msec);
	if (n < 0) {
		return n;
	}
	else if (0 == n) {
		timestamp_msec += wait_msec;
	}
	else {
		int i;
		timestamp_msec = gmtimeMillisecond();
		for (i = 0; i < n; ++i) {
			NioFD_t* niofd;
			ReactorObject_t* o;
			ChannelBase_t* channel;
			int ev_mask;

			niofd = nioEventCheck(&reactor->m_nio, e + i, &ev_mask);
			if (!niofd) {
				continue;
			}
			o = pod_container_of(niofd, ReactorObject_t, niofd);
			channel = o->m_channel;
			if (!channel) {
				continue;
			}
			do {
				if (ev_mask & NIO_OP_READ) {
					if (SOCK_STREAM == channel->socktype) {
						if (CHANNEL_SIDE_LISTEN == channel->side) {
							reactor_stream_accept(channel, o, timestamp_msec);
						}
						else if (o->m_connected) {
							reactor_stream_readev(reactor, channel, o, timestamp_msec);
						}
						else {
							channel->valid = 0;
						}
					}
					else {
						reactor_dgram_readev(reactor, channel, o, timestamp_msec);
					}
					if (!channel->valid) {
						break;
					}
					if (SOCK_STREAM == channel->socktype && CHANNEL_SIDE_LISTEN == channel->side) {
						if (!nioCommit(&reactor->m_nio, &o->niofd, NIO_OP_ACCEPT, NULL, 0)) {
							channel->valid = 0;
							channel->detach_error = REACTOR_IO_ACCEPT_ERR;
							break;
						}
					}
					else if (!nioCommit(&reactor->m_nio, &o->niofd, NIO_OP_READ, NULL, 0)) {
						channel->valid = 0;
						channel->detach_error = REACTOR_IO_READ_ERR;
						break;
					}
				}
				if (ev_mask & NIO_OP_WRITE) {
					if (SOCK_STREAM != channel->socktype) {
						break;
					}
					if (o->m_connected) {
						reactor_stream_writeev(reactor, channel, o);
					}
					else if (!reactor_stream_connect(reactor, channel, o, timestamp_msec)) {
						channel->valid = 0;
						channel->detach_error = REACTOR_IO_CONNECT_ERR;
						break;
					}
				}
			} while (0);
			if (channel->valid) {
				continue;
			}
			reactorobject_invalid_inner_handler(reactor, channel, timestamp_msec);
		}
	}
	reactor_exec_cmdlist(reactor, timestamp_msec);
	if (reactor->m_event_msec > 0 && timestamp_msec >= reactor->m_event_msec) {
		reactor->m_event_msec = 0;
		reactor_exec_connect_timeout(reactor, timestamp_msec);
		reactor_exec_object(reactor, timestamp_msec);
	}
	return n;
}

void reactorDestroy(Reactor_t* reactor) {
	if (!reactor) {
		return;
	}
	criticalsectionClose(&reactor->m_cmdlistlock);
	reactor_free_cmdlist(reactor);
	reactor_free_alive_objects(reactor);
	nioClose(&reactor->m_nio);
	free(reactor);
}

ChannelBase_t* channelbaseOpen(unsigned short side, const ChannelBaseProc_t* proc, int domain, int socktype, int protocol) {
	ChannelBase_t* channel;
	ReactorObject_t* o;

	channel = (ChannelBase_t*)malloc(sizeof(ChannelBase_t));
	if (!channel) {
		return NULL;
	}
	o = reactorobjectOpen(INVALID_FD_HANDLE, domain, socktype, protocol);
	if (!o) {
		free(channel);
		return NULL;
	}
	channelbaseInit(channel, side, proc, domain, socktype, protocol);
	o->m_channel = channel;
	channel->o = o;
	if (SOCK_STREAM == socktype) {
		if (CHANNEL_SIDE_CLIENT == side || CHANNEL_SIDE_SERVER == side) { /* default disable Nagle */
			socketTcpNoDelay(o->niofd.fd, 1);
		}
	}
	return channel;
}

ChannelBase_t* channelbaseOpenWithFD(unsigned short side, const ChannelBaseProc_t* proc, FD_t fd, int domain, int protocol) {
	ChannelBase_t* channel;
	ReactorObject_t* o;
	int socktype;

	if (!socketGetType(fd, &socktype)) {
		return NULL;
	}
	channel = (ChannelBase_t*)malloc(sizeof(ChannelBase_t));
	if (!channel) {
		return NULL;
	}
	o = reactorobjectOpen(fd, domain, socktype, protocol);
	if (!o) {
		goto err;
	}
	channelbaseInit(channel, side, proc, domain, socktype, protocol);
	o->m_channel = channel;
	channel->o = o;
	if (SOCK_STREAM == socktype) {
		if (CHANNEL_SIDE_LISTEN == side) {
			channel->listen_addrlen = sizeof(struct sockaddr_storage);
			if (!socketHasAddr(fd, &channel->listen_addr.sa, &channel->listen_addrlen)) {
				goto err;
			}
		}
		else if (CHANNEL_SIDE_CLIENT == side || CHANNEL_SIDE_SERVER == side) { /* default disable Nagle */
			channel->connect_addrlen = sizeof(struct sockaddr_storage);
			if (!socketIsConnected(fd, &channel->connect_addr.sa, &channel->connect_addrlen)) {
				goto err;
			}
			memmove(&channel->to_addr, &channel->connect_addr, channel->connect_addrlen);
			channel->to_addrlen = channel->connect_addrlen;

			socketTcpNoDelay(fd, 1);
		}
	}
	return channel;
err:
	free(o);
	free(channel);
	return NULL;
}

ChannelBase_t* channelbaseSetOperatorSockaddr(ChannelBase_t* channel, const struct sockaddr* op_addr, socklen_t op_addrlen) {
	unsigned short side = channel->side;
	FD_t fd = channel->o->niofd.fd;
	if (CHANNEL_SIDE_LISTEN == side) {
		if (SOCK_STREAM == channel->socktype) {
			if (AF_UNSPEC == channel->listen_addr.sa.sa_family) {
				if (!socketTcpListen(fd, op_addr, op_addrlen, SOMAXCONN)) {
					return NULL;
				}
				memmove(&channel->listen_addr, op_addr, op_addrlen);
				channel->listen_addrlen = op_addrlen;
			}
			else if (listen(fd, SOMAXCONN)) {
				return NULL;
			}
		}
		else if (AF_UNSPEC == channel->listen_addr.sa.sa_family) {
			if (!socketBindAndReuse(fd, op_addr, op_addrlen)) {
				return NULL;
			}
			memmove(&channel->listen_addr, op_addr, op_addrlen);
			channel->listen_addrlen = op_addrlen;
		}
	}
	else {
		if (CHANNEL_SIDE_CLIENT == side || CHANNEL_SIDE_SERVER == side) {
			if (AF_UNSPEC == channel->connect_addr.sa.sa_family) {
				memmove(&channel->connect_addr, op_addr, op_addrlen);
				channel->connect_addrlen = op_addrlen;
			}
		}
		if (AF_UNSPEC == channel->to_addr.sa.sa_family) {
			memmove(&channel->to_addr, op_addr, op_addrlen);
			channel->to_addrlen = op_addrlen;
		}
	}
	return channel;
}

ChannelBase_t* channelbaseAddRef(ChannelBase_t* channel) {
	_xadd32(&channel->m_refcnt, 1);
	return channel;
}

void channelbaseReg(Reactor_t* reactor, ChannelBase_t* channel) {
	if (_xchg8(&channel->m_reghaspost, 1)) {
		return;
	}
	_xadd32(&channel->m_refcnt, 1);
	channel->reactor = reactor;
	reactor_commit_cmd(reactor, &channel->m_regcmd);
}

void channelbaseCloseRef(ChannelBase_t* channel) {
	Reactor_t* reactor;
	if (!channel) {
		return;
	}
	if (_xadd32(&channel->m_refcnt, -1) > 1) {
		return;
	}
	reactor = channel->reactor;
	if (channel->proc->on_free) {
		channel->proc->on_free(channel);
	}
	if (!reactor) {
		socketClose(channel->o->niofd.fd);
		reactorobject_free(channel->o);
		channelobject_free(channel);
		return;
	}
	reactor_commit_cmd(reactor, &channel->m_freecmd);
}

void channelbaseSendFin(ChannelBase_t* channel) {
	if (_xchg8(&channel->m_has_commit_fincmd, 1)) {
		return;
	}
	if (!channel->reactor) {
		return;
	}
	if (SOCK_STREAM == channel->socktype) {
		reactor_commit_cmd(channel->reactor, &channel->m_stream_fincmd);
		return;
	}
	else if (channel->to_addr.sa.sa_family != AF_UNSPEC) {
		ReactorPacket_t* packet;
		unsigned int hdrsize;
		if (channel->proc->on_hdrsize) {
			hdrsize = channel->proc->on_hdrsize(channel, 0);
		}
		else {
			hdrsize = 0;
		}
		packet = reactorpacketMake(NETPACKET_FIN, hdrsize, 0, &channel->to_addr.sa, channel->to_addrlen);
		if (!packet) {
			return;
		}
		packet->channel = channel;
		reactor_commit_cmd(channel->reactor, &packet->cmd);
	}
}

void channelbaseSend(ChannelBase_t* channel, const void* data, size_t len, int pktype, const struct sockaddr* to_addr, socklen_t to_addrlen) {
	if (NETPACKET_FIN == pktype) {
		channelbaseSendFin(channel);
	}
	else {
		Iobuf_t iov = iobufStaticInit(data, len);
		channelbaseSendv(channel, &iov, 1, pktype, to_addr, to_addrlen);
	}
}

void channelbaseSendv(ChannelBase_t* channel, const Iobuf_t iov[], unsigned int iovcnt, int pktype, const struct sockaddr* to_addr, socklen_t to_addrlen) {
	List_t pklist;
	if (NETPACKET_FIN == pktype) {
		channelbaseSendFin(channel);
		return;
	}
	if (!channel->valid || channel->m_has_commit_fincmd) {
		return;
	}
	listInit(&pklist);
	if (SOCK_STREAM == channel->socktype) {
		to_addr = NULL;
		to_addrlen = 0;
	}
	else if (channel->to_addr.sa.sa_family != AF_UNSPEC) {
		to_addr = &channel->to_addr.sa;
		to_addrlen = channel->to_addrlen;
	}
	if (!channelbaseShardDatas(channel, pktype, iov, iovcnt, &pklist, to_addr, to_addrlen)) {
		return;
	}
	if (listIsEmpty(&pklist)) {
		return;
	}
	reactor_commit_cmdlist(channel->reactor, &pklist);
}

Session_t* sessionInit(Session_t* session) {
	session->channel = NULL;
	session->ident = NULL;
	session->userdata = NULL;
	session->do_connect_handshake = NULL;
	session->on_disconnect = NULL;
	return session;
}

void sessionReplaceChannel(Session_t* session, ChannelBase_t* channel) {
	ChannelBase_t* old_channel = session->channel;
	if (old_channel == channel) {
		return;
	}
	if (old_channel) {
		old_channel->session = NULL;
	}
	if (channel) {
		channel->session = session;
	}
	session->channel = channel;
}

#ifdef	__cplusplus
}
#endif
