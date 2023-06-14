//
// Created by hujianzhe
//

#include "../../inc/component/reactor.h"
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
	List_t m_invalidlist;
	List_t m_connect_endlist;
	Hashtable_t m_objht;
	HashtableNode_t* m_objht_bulks[2048];
} Reactor_t;

ReactorPacket_t* reactorpacketMake(int pktype, unsigned int hdrlen, unsigned int bodylen) {
	ReactorPacket_t* pkg = (ReactorPacket_t*)malloc(sizeof(ReactorPacket_t) + hdrlen + bodylen);
	if (pkg) {
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
	}
	return pkg;
}

void reactorpacketFree(ReactorPacket_t* pkg) {
	free(pkg);
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

static void free_io_resource(ReactorObject_t* o) {
	free_inbuf(o);
	if (INVALID_FD_HANDLE != o->niofd.fd) {
		socketClose(o->niofd.fd);
		o->niofd.fd = INVALID_FD_HANDLE;
	}
	if (o->m_readol) {
		nioFreeOverlapped(o->m_readol);
		o->m_readol = NULL;
	}
	if (o->m_writeol) {
		nioFreeOverlapped(o->m_writeol);
		o->m_writeol = NULL;
	}
}

static void reactorobject_free(ReactorObject_t* o) {
	free_io_resource(o);
	free(o);
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

static int cmp_sorted_reactor_object_invalid(ListNode_t* node, ListNode_t* new_node) {
	ReactorObject_t* o = pod_container_of(node, ReactorObject_t, m_invalidnode);
	ReactorObject_t* new_o = pod_container_of(new_node, ReactorObject_t, m_invalidnode);
	return o->m_invalid_msec > new_o->m_invalid_msec;
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
	o->m_invalid_msec = now_msec;
	if (o->m_has_inserted) {
		o->m_has_inserted = 0;
		hashtableRemoveNode(&reactor->m_objht, &o->m_hashnode);
	}
	if (SOCK_STREAM == channel->socktype) {
		if (o->stream.m_connect_end_msec > 0) {
			listRemoveNode(&reactor->m_connect_endlist, &o->stream.m_connect_endnode);
			o->stream.m_connect_end_msec = 0;
		}
		free_io_resource(o);
	}

	channel->m_has_detached = 1;
	channel->o = NULL;
	channel->valid = 0;
	channel->proc->on_detach(channel);

	if (o->detach_timeout_msec <= 0) {
		reactorobject_free(o);
	}
	else {
		o->m_invalid_msec += o->detach_timeout_msec;
		listInsertNodeSorted(&reactor->m_invalidlist, &o->m_invalidnode, cmp_sorted_reactor_object_invalid);
		reactor_set_event_timestamp(reactor, o->m_invalid_msec);
	}
}

static int reactorobject_request_read(Reactor_t* reactor, ReactorObject_t* o, int domain) {
	if (o->m_readol_has_commit) {
		return 1;
	}
	if (!o->m_readol) {
		o->m_readol = nioAllocOverlapped(domain, NIO_OP_READ, NULL, 0, 0);
		if (!o->m_readol) {
			return 0;
		}
	}
	if (!nioCommit(&reactor->m_nio, &o->niofd, o->m_readol, NULL, 0)) {
		return 0;
	}
	o->m_readol_has_commit = 1;
	return 1;
}

static int reactorobject_request_stream_accept(Reactor_t* reactor, ReactorObject_t* o, int domain) {
	if (o->m_readol_has_commit) {
		return 1;
	}
	if (!o->m_readol) {
		o->m_readol = nioAllocOverlapped(domain, NIO_OP_ACCEPT, NULL, 0, 0);
		if (!o->m_readol) {
			return 0;
		}
	}
	if (!nioCommit(&reactor->m_nio, &o->niofd, o->m_readol, NULL, 0)) {
		return 0;
	}
	o->m_readol_has_commit = 1;
	return 1;
}

static int reactorobject_request_stream_write(Reactor_t* reactor, ReactorObject_t* o, int domain) {
	if (o->m_writeol_has_commit) {
		return 1;
	}
	if (!o->m_writeol) {
		o->m_writeol = nioAllocOverlapped(domain, NIO_OP_WRITE, NULL, 0, 0);
		if (!o->m_writeol) {
			return 0;
		}
	}
	if (!nioCommit(&reactor->m_nio, &o->niofd, o->m_writeol, NULL, 0)) {
		return 0;
	}
	o->m_writeol_has_commit = 1;
	return 1;
}

static int reactorobject_request_stream_connect(Reactor_t* reactor, ReactorObject_t* o, struct sockaddr* saddr, int saddrlen) {
	if (!o->m_writeol) {
		o->m_writeol = nioAllocOverlapped(saddr->sa_family, NIO_OP_CONNECT, NULL, 0, 0);
		if (!o->m_writeol) {
			return 0;
		}
	}
	if (!nioCommit(&reactor->m_nio, &o->niofd, o->m_writeol, saddr, saddrlen)) {
		return 0;
	}
	o->m_writeol_has_commit = 1;
	return 1;
}

static int reactor_reg_object_check(Reactor_t* reactor, ChannelBase_t* channel, ReactorObject_t* o, long long timestamp_msec) {
	if (SOCK_STREAM == channel->socktype) {
		BOOL ret;
		if (channel->flag & CHANNEL_FLAG_LISTEN) {
			if (!reactorobject_request_stream_accept(reactor, o, channel->listen_addr.sa.sa_family)) {
				return 0;
			}
		}
		else if (!socketIsConnected(o->niofd.fd, &ret)) {
			return 0;
		}
		else if (ret) {
			o->m_connected = 1;
			if (!reactorobject_request_read(reactor, o, channel->to_addr.sa.sa_family)) {
				return 0;
			}
		}
		else {
			o->m_connected = 0;
			if (!reactorobject_request_stream_connect(reactor, o, &channel->connect_addr.sa, sockaddrLength(&channel->connect_addr.sa))) {
				return 0;
			}
			if (o->stream_connect_timeout_sec > 0) {
				o->stream.m_connect_end_msec = o->stream_connect_timeout_sec;
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
		BOOL bval;
		if (!socketHasAddr(o->niofd.fd, &bval)) {
			return 0;
		}
		if (!bval) {
			Sockaddr_t local_addr;
			if (!sockaddrEncode(&local_addr.sa, channel->to_addr.sa.sa_family, NULL, 0)) {
				return 0;
			}
			if (bind(o->niofd.fd, &local_addr.sa, sockaddrLength(&local_addr.sa))) {
				return 0;
			}
		}
		if (!reactorobject_request_read(reactor, o, channel->to_addr.sa.sa_family)) {
			return 0;
		}
		o->m_connected = (socketIsConnected(o->niofd.fd, &bval) && bval);
	}
	return 1;
}

static int reactor_reg_object(Reactor_t* reactor, ChannelBase_t* channel, long long timestamp_msec) {
	ReactorObject_t* o = channel->o;
	if (reactor_reg_object_check(reactor, channel, o, timestamp_msec)) {
		HashtableNode_t* htnode = hashtableInsertNode(&reactor->m_objht, &o->m_hashnode);
		if (htnode != &o->m_hashnode) {
			ReactorObject_t* exist_o = pod_container_of(htnode, ReactorObject_t, m_hashnode);
			hashtableReplaceNode(&reactor->m_objht, htnode, &o->m_hashnode);
			exist_o->m_has_inserted = 0;
			reactorobject_invalid_inner_handler(reactor, exist_o->m_channel, timestamp_msec);
		}
		o->m_has_inserted = 1;
		return 1;
	}
	return 0;
}

static void reactor_exec_object_reg_callback(Reactor_t* reactor, ChannelBase_t* channel, long long timestamp_msec) {
	ReactorObject_t* o;

	channel->reactor = reactor;
	if (channel->proc->on_reg) {
		channel->proc->on_reg(channel, timestamp_msec);
		if (!after_call_channel_interface(channel)) {
			return;
		}
	}
	else {
		reactor_set_event_timestamp(reactor, channel->event_msec);
	}
	if (SOCK_STREAM != channel->socktype) {
		channel_next_heartbeat_timestamp(channel, timestamp_msec);
		return;
	}
	if (channel->flag & CHANNEL_FLAG_LISTEN) {
		return;
	}
	o = channel->o;
	if (!o->m_connected) {
		return;
	}
	if (channel->flag & CHANNEL_FLAG_CLIENT) {
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
	socketShutdown(channel->o->niofd.fd, SHUT_WR);
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
	if (channel->flag & CHANNEL_FLAG_SERVER) {
		return 0;
	}
	if (channel->flag & CHANNEL_FLAG_CLIENT) {
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
	HashtableNode_t *cur, *next;
	for (cur = hashtableFirstNode(&reactor->m_objht); cur; cur = next) {
		ReactorObject_t* o = pod_container_of(cur, ReactorObject_t, m_hashnode);
		ChannelBase_t* channel = o->m_channel;
		next = hashtableNextNode(cur);

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

static void reactor_exec_invalidlist(Reactor_t* reactor, long long now_msec) {
	ListNode_t* cur, *next;
	for (cur = reactor->m_invalidlist.head; cur; cur = next) {
		ReactorObject_t* o = pod_container_of(cur, ReactorObject_t, m_invalidnode);
		next = cur->next;
		if (o->m_invalid_msec > now_msec) {
			reactor_set_event_timestamp(reactor, o->m_invalid_msec);
			break;
		}
		listRemoveNode(&reactor->m_invalidlist, cur);
		reactorobject_free(o);
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
	for (cur = ctxptr->sendlist.head; cur; cur = next) {
		int res;
		NetPacket_t* packet = pod_container_of(cur, NetPacket_t, node);
		next = cur->next;

		res = send(o->niofd.fd, (char*)(packet->buf + packet->off), packet->hdrlen + packet->bodylen - packet->off, 0);
		if (res < 0) {
			if (errnoGet() != EWOULDBLOCK) {
				channel->valid = 0;
				channel->detach_error = REACTOR_IO_WRITE_ERR;
				return;
			}
			res = 0;
		}
		packet->off += res;
		if (packet->off >= packet->hdrlen + packet->bodylen) {
			streamtransportctxRemoveCacheSendPacket(ctxptr, packet);
			reactorpacketFree(pod_container_of(cur, ReactorPacket_t, _.node));
			continue;
		}
		if (reactorobject_request_stream_write(reactor, o, channel->to_addr.sa.sa_family)) {
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
	socketShutdown(o->niofd.fd, SHUT_WR);
	channel->has_sendfin = 1;
	if (channel->has_recvfin) {
		channel->valid = 0;
	}
}

static void reactor_stream_accept(ChannelBase_t* channel, ReactorObject_t* o, long long timestamp_msec) {
	Sockaddr_t saddr;
	socklen_t slen = sizeof(saddr.st);
	FD_t connfd;
	for (connfd = nioAcceptFirst(o->niofd.fd, o->m_readol, &saddr.sa, &slen);
		connfd != INVALID_FD_HANDLE;
		connfd = accept(o->niofd.fd, &saddr.sa, &slen))
	{
		channel->on_ack_halfconn(channel, connfd, &saddr.sa, timestamp_msec);
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
		res = channel->proc->on_read(channel, o->m_inbuf + o->m_inbufoff, o->m_inbuflen - o->m_inbufoff, timestamp_msec, &channel->to_addr.sa);
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
	if (channel->flag & CHANNEL_FLAG_SERVER) {
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
			int res = channel->proc->on_read(channel, ptr + off, len - off, timestamp_msec, &from_addr.sa);
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
		if (channel->flag & CHANNEL_FLAG_SERVER) {
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
		if (!reactorobject_request_stream_write(reactor, o, channel->to_addr.sa.sa_family)) {
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
		paddr = &channel->to_addr.sa;
		addrlen = sockaddrLength(paddr);
	}
	sendto(o->niofd.fd, (char*)packet->_.buf, packet->_.hdrlen + packet->_.bodylen, 0, paddr, addrlen);
	if (!packet->_.cached) {
		reactorpacketFree(packet);
	}
}

static int reactor_stream_connect(Reactor_t* reactor, ChannelBase_t* channel, ReactorObject_t* o, long long timestamp_msec) {
	if (o->m_writeol) {
		nioFreeOverlapped(o->m_writeol);
		o->m_writeol = NULL;
	}
	if (o->stream.m_connect_end_msec > 0) {
		listRemoveNode(&reactor->m_connect_endlist, &o->stream.m_connect_endnode);
		o->stream.m_connect_end_msec = 0;
	}
	if (!nioConnectCheckSuccess(o->niofd.fd)) {
		return 0;
	}
	if (!reactorobject_request_read(reactor, o, channel->to_addr.sa.sa_family)) {
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

static int objht_keycmp(const HashtableNodeKey_t* node_key, const HashtableNodeKey_t* key) {
	return *(FD_t*)(node_key->ptr) != *(FD_t*)(key->ptr);
}

static unsigned int objht_keyhash(const HashtableNodeKey_t* key) { return *(FD_t*)(key->ptr); }

static void reactorCommitCmd(Reactor_t* reactor, ReactorCmd_t* cmdnode) {
	if (REACTOR_CHANNEL_REG_CMD == cmdnode->type) {
		ChannelBase_t* channel = pod_container_of(cmdnode, ChannelBase_t, m_regcmd);
		if (_xchg8(&channel->m_reghaspost, 1)) {
			return;
		}
		channel->reactor = reactor;
	}
	else if (REACTOR_STREAM_SENDFIN_CMD == cmdnode->type) {
		ChannelBase_t* channel = pod_container_of(cmdnode, ChannelBase_t, m_stream_fincmd);
		if (_xchg8(&channel->m_has_commit_fincmd, 1)) {
			return;
		}
		reactor = channel->reactor;
		if (!reactor) {
			return;
		}
	}
	else if (REACTOR_CHANNEL_FREE_CMD == cmdnode->type) {
		ChannelBase_t* channel = pod_container_of(cmdnode, ChannelBase_t, m_freecmd);
		if (_xadd32(&channel->m_refcnt, -1) > 1) {
			return;
		}
		reactor = channel->reactor;
		if (channel->proc->on_free) {
			channel->proc->on_free(channel);
		}
		if (!reactor) {
			channelobject_free(channel);
			return;
		}
	}
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

static void reactorobject_init_comm(ReactorObject_t* o, FD_t fd) {
	o->detach_timeout_msec = 0;
	o->inbuf_maxlen = 0;
	o->inbuf_saved = 1;
	o->stream_connect_timeout_sec = 0;

	o->m_connected = 0;
	o->m_channel = NULL;
	o->m_hashnode.key.ptr = &o->niofd.fd;
	o->m_has_inserted = 0;
	o->m_readol_has_commit = 0;
	o->m_writeol_has_commit = 0;
	o->m_readol = NULL;
	o->m_writeol = NULL;
	o->m_invalid_msec = 0;
	o->m_inbuf = NULL;
	o->m_inbuflen = 0;
	o->m_inbufoff = 0;
	o->m_inbufsize = 0;
	memset(&o->niofd, 0, sizeof(o->niofd));
	o->niofd.fd = fd;
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
	if (SOCK_DGRAM == socktype) {
		if (!socketUdpConnectReset(fd)) {
			if (fd_create) {
				socketClose(fd);
			}
			free(o);
			return NULL;
		}
	}
	reactorobject_init_comm(o, fd);
	if (SOCK_STREAM == socktype) {
		memset(&o->stream, 0, sizeof(o->stream));
	}
	else {
		o->inbuf_maxlen = 1464;
	}
	return o;
}

static List_t* channelbaseShardDatas(ChannelBase_t* channel, int pktype, const Iobuf_t iov[], unsigned int iovcnt, List_t* packetlist) {
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
		unsigned int off = 0, iov_i = 0, iov_off = 0;
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
			packet = reactorpacketMake(pktype, hdrsz, sz);
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
		if (0 == hdrsz && SOCK_STREAM == channel->socktype) {
			return packetlist;
		}
		packet = reactorpacketMake(pktype, hdrsz, 0);
		if (!packet) {
			return NULL;
		}
		packet->channel = channel;
		listPushNodeBack(&pklist, &packet->cmd._);
	}
	listAppend(packetlist, &pklist);
	return packetlist;
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
	if (!nioCreate(&reactor->m_nio)) {
		free(reactor);
		return NULL;
	}
	if (!criticalsectionCreate(&reactor->m_cmdlistlock)) {
		nioClose(&reactor->m_nio);
		free(reactor);
		return NULL;
	}
	listInit(&reactor->m_cmdlist);
	listInit(&reactor->m_invalidlist);
	listInit(&reactor->m_connect_endlist);
	hashtableInit(&reactor->m_objht,
		reactor->m_objht_bulks, sizeof(reactor->m_objht_bulks) / sizeof(reactor->m_objht_bulks[0]),
		objht_keycmp, objht_keyhash);
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
					o->m_readol_has_commit = 0;
					if (SOCK_STREAM == channel->socktype) {
						if (channel->flag & CHANNEL_FLAG_LISTEN) {
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
					if (SOCK_STREAM == channel->socktype && (channel->flag & CHANNEL_FLAG_LISTEN)) {
						if (!reactorobject_request_stream_accept(reactor, o, channel->listen_addr.sa.sa_family)) {
							channel->valid = 0;
							channel->detach_error = REACTOR_IO_ACCEPT_ERR;
							break;
						}
					}
					else if (!reactorobject_request_read(reactor, o, channel->to_addr.sa.sa_family)) {
						channel->valid = 0;
						channel->detach_error = REACTOR_IO_READ_ERR;
						break;
					}
				}
				if (ev_mask & NIO_OP_WRITE) {
					o->m_writeol_has_commit = 0;
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
		reactor_exec_invalidlist(reactor, timestamp_msec);
		reactor_exec_object(reactor, timestamp_msec);
	}
	return n;
}

void reactorDestroy(Reactor_t* reactor) {
	if (!reactor) {
		return;
	}
	nioClose(&reactor->m_nio);
	criticalsectionClose(&reactor->m_cmdlistlock);
	do {
		ListNode_t* cur, *next;
		for (cur = reactor->m_cmdlist.head; cur; cur = next) {
			ReactorCmd_t* cmd = pod_container_of(cur, ReactorCmd_t, _);
			next = cur->next;
			if (REACTOR_CHANNEL_REG_CMD == cmd->type) {
				ChannelBase_t* channel = pod_container_of(cmd, ChannelBase_t, m_regcmd);
				if (channel->proc->on_free) {
					channel->proc->on_free(channel);
				}
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
	} while (0);
	do {
		HashtableNode_t *cur, *next;
		for (cur = hashtableFirstNode(&reactor->m_objht); cur; cur = next) {
			ReactorObject_t* o = pod_container_of(cur, ReactorObject_t, m_hashnode);
			ChannelBase_t* channel = o->m_channel;
			next = hashtableNextNode(cur);
			if (channel) {
				if (channel->proc->on_free) {
					channel->proc->on_free(channel);
				}
				channelobject_free(channel);
			}
			reactorobject_free(o);
		}
	} while (0);
	do {
		ListNode_t* cur, *next;
		for (cur = reactor->m_invalidlist.head; cur; cur = next) {
			ReactorObject_t* o = pod_container_of(cur, ReactorObject_t, m_invalidnode);
			next = cur->next;
			reactorobject_free(o);
		}
	} while (0);

	free(reactor);
}

ChannelBase_t* channelbaseOpen(unsigned short channel_flag, const ChannelBaseProc_t* proc, FD_t fd, int socktype, const struct sockaddr* addr) {
	ChannelBase_t* channel;
	int sockaddrlen;
	ReactorObject_t* o;

	sockaddrlen = sockaddrLength(addr);
	if (sockaddrlen <= 0) {
		return NULL;
	}

	channel = (ChannelBase_t*)calloc(1, sizeof(ChannelBase_t));
	if (!channel) {
		return NULL;
	}
	o = reactorobjectOpen(fd, addr->sa_family, socktype, 0);
	if (!o) {
		free(channel);
		return NULL;
	}
	o->m_channel = channel;
	channel->o = o;
	channel->flag = channel_flag;
	channel->socktype = socktype;
	channel->m_refcnt = 1;
	channel->m_regcmd.type = REACTOR_CHANNEL_REG_CMD;
	channel->m_freecmd.type = REACTOR_CHANNEL_FREE_CMD;
	if (SOCK_STREAM == socktype) {
		if ((channel_flag & CHANNEL_FLAG_CLIENT) || (channel_flag & CHANNEL_FLAG_SERVER)) { /* default disable Nagle */
			int on = 1;
			setsockopt(o->niofd.fd, IPPROTO_TCP, TCP_NODELAY, (char*)&on, sizeof(on));
		}
		streamtransportctxInit(&channel->stream_ctx);
		channel->m_stream_fincmd.type = REACTOR_STREAM_SENDFIN_CMD;
		channel->write_fragment_size = ~0;
	}
	else {
		dgramtransportctxInit(&channel->dgram_ctx, 0);
		channel->write_fragment_size = 548;
	}
	memmove(&channel->to_addr, addr, sockaddrlen);
	memmove(&channel->connect_addr, addr, sockaddrlen);
	memmove(&channel->listen_addr, addr, sockaddrlen);
	channel->valid = 1;
	channel->proc = proc;
	return channel;
}

ChannelBase_t* channelbaseAddRef(ChannelBase_t* channel) {
	_xadd32(&channel->m_refcnt, 1);
	return channel;
}

void channelbaseReg(Reactor_t* reactor, ChannelBase_t* channel) {
	reactorCommitCmd(reactor, &channel->m_regcmd);
}

void channelbaseClose(ChannelBase_t* channel) {
	reactorCommitCmd(NULL, &channel->m_freecmd);
}

void channelbaseSendFin(ChannelBase_t* channel) {
	if (SOCK_STREAM == channel->socktype) {
		reactorCommitCmd(channel->reactor, &channel->m_stream_fincmd);
		return;
	}
	if (0 == _xchg8(&channel->m_has_commit_fincmd, 1)) {
		ReactorPacket_t* packet;
		unsigned int hdrsize;
		if (channel->proc->on_hdrsize) {
			hdrsize = channel->proc->on_hdrsize(channel, 0);
		}
		else {
			hdrsize = 0;
		}
		packet = reactorpacketMake(NETPACKET_FIN, hdrsize, 0);
		if (!packet) {
			return;
		}
		packet->channel = channel;
		reactorCommitCmd(channel->reactor, &packet->cmd);
	}
}

void channelbaseSend(ChannelBase_t* channel, const void* data, size_t len, int pktype) {
	if (NETPACKET_FIN == pktype) {
		channelbaseSendFin(channel);
	}
	else {
		Iobuf_t iov = iobufStaticInit(data, len);
		channelbaseSendv(channel, &iov, 1, pktype);
	}
}

void channelbaseSendv(ChannelBase_t* channel, const Iobuf_t iov[], unsigned int iovcnt, int pktype) {
	List_t pklist;
	if (NETPACKET_FIN == pktype) {
		channelbaseSendFin(channel);
		return;
	}
	if (!channel->valid || channel->m_has_commit_fincmd) {
		return;
	}
	listInit(&pklist);
	if (!channelbaseShardDatas(channel, pktype, iov, iovcnt, &pklist)) {
		return;
	}
	if (listIsEmpty(&pklist)) {
		return;
	}
	reactor_commit_cmdlist(channel->reactor, &pklist);
}

Session_t* sessionInit(Session_t* session) {
	session->channel_client = NULL;
	session->channel_server = NULL;
	session->ident = NULL;
	session->userdata = NULL;
	session->do_connect_handshake = NULL;
	session->on_disconnect = NULL;
	return session;
}

void sessionReplaceChannel(Session_t* session, ChannelBase_t* channel) {
	ChannelBase_t* old_channel;
	if (channel->flag & CHANNEL_FLAG_CLIENT) {
		old_channel = session->channel_client;
		if (old_channel == channel) {
			return;
		}
		session->channel_client = channel;
	}
	else if (channel->flag & CHANNEL_FLAG_SERVER) {
		old_channel = session->channel_server;
		if (old_channel == channel) {
			return;
		}
		session->channel_server = channel;
	}
	else {
		return;
	}
	if (old_channel) {
		old_channel->session = NULL;
		channelbaseSendFin(old_channel);
	}
	if (channel) {
		channel->session = session;
	}
}

void sessionDisconnect(Session_t* session) {
	if (session->channel_client) {
		channelbaseSendFin(session->channel_client);
		session->channel_client->session = NULL;
		session->channel_client = NULL;
	}
	if (session->channel_server) {
		channelbaseSendFin(session->channel_server);
		session->channel_server->session = NULL;
		session->channel_server = NULL;
	}
}

void sessionUnbindChannel(Session_t* session) {
	if (session->channel_client) {
		session->channel_client->session = NULL;
		session->channel_client = NULL;
	}
	if (session->channel_server) {
		session->channel_server->session = NULL;
		session->channel_server = NULL;
	}
}

ChannelBase_t* sessionChannel(Session_t* session) {
	if (session->channel_client) {
		return session->channel_client;
	}
	else if (session->channel_server) {
		return session->channel_server;
	}
	else {
		return NULL;
	}
}

#ifdef	__cplusplus
}
#endif
