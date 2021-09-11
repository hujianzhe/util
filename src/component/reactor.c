//
// Created by hujianzhe
//

#include "../../inc/component/reactor.h"
#include "../../inc/sysapi/error.h"
#include "../../inc/sysapi/time.h"
#include <stdlib.h>
#include <string.h>

#define	streamChannel(o)\
(o->m_channel_list.head ? pod_container_of(o->m_channel_list.head, ChannelBase_t, regcmd._) : NULL)

#ifdef	__cplusplus
extern "C" {
#endif

static void reactor_set_event_timestamp(Reactor_t* reactor, long long timestamp_msec) {
	if (reactor->m_event_msec > 0 && reactor->m_event_msec <= timestamp_msec) {
		return;
	}
	reactor->m_event_msec = timestamp_msec;
}

static void channel_set_timestamp(ChannelBase_t* channel, long long timestamp_msec) {
	if (channel->event_msec > 0 && channel->event_msec <= timestamp_msec) {
		return;
	}
	channel->event_msec = timestamp_msec;
	reactor_set_event_timestamp(channel->reactor, timestamp_msec);
}

static long long channel_next_heartbeat_timestamp(ChannelBase_t* channel, long long timestamp_msec) {
	if (channel->heartbeat_timeout_sec > 0) {
		long long ts = channel->heartbeat_timeout_sec;
		ts *= 1000;
		ts += timestamp_msec;
		return ts;
	}
	return 0;
}

static void channel_detach_handler(ChannelBase_t* channel, int error, long long timestamp_msec) {
	ReactorObject_t* o;
	if (channel->m_has_detached) {
		return;
	}
	channel->m_has_detached = 1;
	channel->detach_error = error;
	o = channel->o;
	listRemoveNode(&o->m_channel_list, &channel->regcmd._);
	if (!o->m_channel_list.head) {
		o->m_valid = 0;
	}
	channel->o = NULL;
	channel->on_detach(channel);
}

static int after_call_channel_interface(ChannelBase_t* channel) {
	if (channel->valid) {
		reactor_set_event_timestamp(channel->reactor, channel->event_msec);
		return 1;
	}
	else {
		channel_detach_handler(channel, channel->detach_error, 0);
		return 0;
	}
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
	if (INVALID_FD_HANDLE != o->fd) {
		socketClose(o->fd);
		o->fd = INVALID_FD_HANDLE;
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
	if (channel->flag & CHANNEL_FLAG_STREAM) {
		packetlist_free_packet(&channel->stream_ctx.recvlist);
		packetlist_free_packet(&channel->stream_ctx.sendlist);
	}
	else {
		packetlist_free_packet(&channel->dgram_ctx.recvlist);
		packetlist_free_packet(&channel->dgram_ctx.sendlist);
	}
	packetlist_free_packet(&channel->m_cache_packet_list);
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

static void reactorobject_invalid_inner_handler(Reactor_t* reactor, ReactorObject_t* o, long long now_msec) {
	ListNode_t* cur, *next;
	if (o->m_has_detached) {
		return;
	}
	o->m_has_detached = 1;
	o->m_invalid_msec = now_msec;
	if (o->m_has_inserted) {
		o->m_has_inserted = 0;
		hashtableRemoveNode(&reactor->m_objht, &o->m_hashnode);
	}
	if (SOCK_STREAM == o->socktype) {
		if (o->stream.m_connect_end_msec > 0) {
			listRemoveNode(&reactor->m_connect_endlist, &o->stream.m_connect_endnode);
			o->stream.m_connect_end_msec = 0;
		}
		free_io_resource(o);
	}

	for (cur = o->m_channel_list.head; cur; cur = next) {
		ChannelBase_t* channel = pod_container_of(cur, ChannelBase_t, regcmd._);
		next = cur->next;
		if (REACTOR_CONNECT_ERR == o->detach_error) {
			channel->detach_error = o->detach_error;
		}
		else if (REACTOR_IO_ERR == o->detach_error && 0 == channel->detach_error) {
			channel->detach_error = o->detach_error;
		}
		channel->m_has_detached = 1;
		channel->o = NULL;
		channel->valid = 0;
		channel->on_detach(channel);
	}
	listInit(&o->m_channel_list);

	if (o->detach_timeout_msec <= 0) {
		reactorobject_free(o);
	}
	else {
		o->m_invalid_msec += o->detach_timeout_msec;
		listInsertNodeSorted(&reactor->m_invalidlist, &o->m_invalidnode, cmp_sorted_reactor_object_invalid);
		reactor_set_event_timestamp(reactor, o->m_invalid_msec);
	}
}

static int reactorobject_request_read(Reactor_t* reactor, ReactorObject_t* o) {
	Sockaddr_t saddr;
	int opcode;
	if (o->m_readol_has_commit) {
		return 1;
	}
	else if (SOCK_STREAM == o->socktype && o->stream.m_listened) {
		opcode = NIO_OP_ACCEPT;
	}
	else {
		opcode = NIO_OP_READ;
	}
	if (!o->m_readol) {
		o->m_readol = nioAllocOverlapped(opcode, NULL, 0, SOCK_STREAM != o->socktype ? o->read_fragment_size : 0);
		if (!o->m_readol) {
			return 0;
		}
	}
	saddr.sa.sa_family = o->domain;
	if (!nioCommit(&reactor->m_nio, o->fd, opcode, o->m_readol, &saddr.sa, sockaddrLength(&saddr.sa))) {
		return 0;
	}
	o->m_readol_has_commit = 1;
	return 1;
}

static int reactorobject_request_write(Reactor_t* reactor, ReactorObject_t* o) {
	Sockaddr_t saddr;
	if (!o->m_valid) {
		return 0;
	}
	else if (o->m_writeol_has_commit) {
		return 1;
	}
	else if (!o->m_writeol) {
		o->m_writeol = nioAllocOverlapped(NIO_OP_WRITE, NULL, 0, 0);
		if (!o->m_writeol) {
			return 0;
		}
	}
	saddr.sa.sa_family = o->domain;
	if (!nioCommit(&reactor->m_nio, o->fd, NIO_OP_WRITE, o->m_writeol, &saddr.sa, sockaddrLength(&saddr.sa))) {
		return 0;
	}
	o->m_writeol_has_commit = 1;
	return 1;
}

static int reactorobject_request_connect(Reactor_t* reactor, ReactorObject_t* o) {
	if (!o->m_writeol) {
		o->m_writeol = nioAllocOverlapped(NIO_OP_CONNECT, NULL, 0, 0);
		if (!o->m_writeol)
			return 0;
	}
	if (!nioCommit(&reactor->m_nio, o->fd, NIO_OP_CONNECT, o->m_writeol,
		&o->stream.m_connect_addr.sa, sockaddrLength(&o->stream.m_connect_addr.sa)))
	{
		return 0;
	}
	o->m_writeol_has_commit = 1;
	return 1;
}

static int reactor_reg_object_check(Reactor_t* reactor, ReactorObject_t* o, long long timestamp_msec) {
	if (!nioReg(&reactor->m_nio, o->fd)) {
		return 0;
	}
	if (SOCK_STREAM == o->socktype) {
		BOOL ret;
		ChannelBase_t* channel = streamChannel(o);
		if (channel->flag & CHANNEL_FLAG_LISTEN) {
			o->stream.m_listened = 1;
			if (!reactorobject_request_read(reactor, o)) {
				return 0;
			}
		}
		else if (!socketIsConnected(o->fd, &ret)) {
			return 0;
		}
		else if (ret) {
			o->m_connected = 1;
			channel->disable_send = 0;
			if (!reactorobject_request_read(reactor, o)) {
				return 0;
			}
		}
		else {
			o->m_connected = 0;
			channel->disable_send = 1;
			if (!reactorobject_request_connect(reactor, o)) {
				return 0;
			}
			if (o->stream.max_connect_timeout_sec > 0) {
				o->stream.m_connect_end_msec = o->stream.max_connect_timeout_sec;
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
	else if (SOCK_DGRAM == o->socktype) {
		ListNode_t* cur, *next;
		BOOL bval;
		if (!socketHasAddr(o->fd, &bval)) {
			return 0;
		}
		if (!bval) {
			Sockaddr_t local_addr;
			if (!sockaddrEncode(&local_addr.sa, o->domain, NULL, 0)) {
				return 0;
			}
			if (bind(o->fd, &local_addr.sa, sockaddrLength(&local_addr.sa))) {
				return 0;
			}
		}
		if (!reactorobject_request_read(reactor, o)) {
			return 0;
		}
		for (cur = o->m_channel_list.head; cur; cur = next) {
			ChannelBase_t* channel = pod_container_of(cur, ChannelBase_t, regcmd._);
			next = cur->next;
			if (channel->flag & CHANNEL_FLAG_CLIENT) {
				channel->disable_send = 1;
			}
		}
		o->m_connected = (socketHasPeerAddr(o->fd, &bval) && bval);
	}
	return 1;
}

static int reactor_reg_object(Reactor_t* reactor, ReactorObject_t* o, long long timestamp_msec) {
	if (reactor_reg_object_check(reactor, o, timestamp_msec)) {
		HashtableNode_t* htnode = hashtableInsertNode(&reactor->m_objht, &o->m_hashnode);
		if (htnode != &o->m_hashnode) {
			ReactorObject_t* exist_o = pod_container_of(htnode, ReactorObject_t, m_hashnode);
			hashtableReplaceNode(htnode, &o->m_hashnode);
			exist_o->m_valid = 0;
			exist_o->m_has_inserted = 0;
			reactorobject_invalid_inner_handler(reactor, exist_o, timestamp_msec);
		}
		o->m_has_inserted = 1;
		return 1;
	}
	return 0;
}

static void reactor_exec_object_reg_callback(Reactor_t* reactor, ReactorObject_t* o, long long timestamp_msec) {
	ChannelBase_t* channel;
	ListNode_t* cur, *next;
	for (cur = o->m_channel_list.head; cur; cur = next) {
		channel = pod_container_of(cur, ChannelBase_t, regcmd._);
		next = cur->next;
		channel->reactor = reactor;
		if (channel->on_reg) {
			channel->on_reg(channel, timestamp_msec);
			channel->on_reg = NULL;
			after_call_channel_interface(channel);
		}
	}
	if (SOCK_STREAM != o->socktype) {
		channel->m_heartbeat_msec = channel_next_heartbeat_timestamp(channel, timestamp_msec);
		channel_set_timestamp(channel, channel->m_heartbeat_msec);
		return;
	}
	if (o->stream.m_listened) {
		return;
	}
	if (!o->m_connected) {
		return;
	}
	channel = streamChannel(o);
	if (~0 != channel->connected_times) {
		++channel->connected_times;
	}
	if (channel->flag & CHANNEL_FLAG_CLIENT) {
		if (channel->on_syn_ack) {
			channel->on_syn_ack(channel, timestamp_msec);
			after_call_channel_interface(channel);
		}
		channel->m_heartbeat_msec = channel_next_heartbeat_timestamp(channel, timestamp_msec);
		channel_set_timestamp(channel, channel->m_heartbeat_msec);
	}
}

static void stream_recvfin_handler(ReactorObject_t* o) {
	ChannelBase_t* channel = streamChannel(o);
	channel->has_recvfin = 1;
	if (channel->has_sendfin) {
		o->m_valid = 0;
	}
	else {
		ReactorPacket_t* fin_pkg = reactorpacketMake(NETPACKET_FIN, 0, 0);
		if (!fin_pkg) {
			o->m_valid = 0;
			return;
		}
		channelbaseSendPacket(channel, fin_pkg);
	}
}

static int channel_heartbeat_handler(ChannelBase_t* channel, long long now_msec) {
	if (channel->m_heartbeat_msec <= 0) {
		return 1;
	}
	if (channel->m_heartbeat_msec > now_msec) {
		channel_set_timestamp(channel, channel->m_heartbeat_msec);
		return 1;
	}
	if (channel->flag & CHANNEL_FLAG_CLIENT) {
		int ok = 0;
		if (channel->m_heartbeat_times < channel->heartbeat_maxtimes) {
			ok = channel->on_heartbeat(channel, channel->m_heartbeat_times++);
		}
		else if (channel->on_heartbeat(channel, channel->m_heartbeat_times)) {
			ok = 1;
			channel->m_heartbeat_times = 0;
		}
		if (!ok) {
			return 0;
		}
		channel->m_heartbeat_msec = channel_next_heartbeat_timestamp(channel, now_msec);
		channel_set_timestamp(channel, channel->m_heartbeat_msec);
	}
	else if (channel->flag & CHANNEL_FLAG_SERVER) {
		return 0;
	}
	return 1;
}

static void reactor_exec_object(Reactor_t* reactor, long long now_msec) {
	HashtableNode_t *cur, *next;
	for (cur = hashtableFirstNode(&reactor->m_objht); cur; cur = next) {
		ReactorObject_t* o = pod_container_of(cur, ReactorObject_t, m_hashnode);
		next = hashtableNextNode(cur);
		if (o->m_valid) {
			ListNode_t* lcur, *lnext;
			for (lcur = o->m_channel_list.head; lcur; lcur = lnext) {
				ChannelBase_t* channel = pod_container_of(lcur, ChannelBase_t, regcmd._);
				lnext = lcur->next;
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
					channel_detach_handler(channel, REACTOR_ZOMBIE_ERR, now_msec);
					continue;
				}
				if (channel->on_exec) {
					channel->on_exec(channel, now_msec);
					after_call_channel_interface(channel);
				}
			}
			if (o->m_valid) {
				continue;
			}
		}
		reactorobject_invalid_inner_handler(reactor, o, now_msec);
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
		next = cur->next;
		if (o->stream.m_connect_end_msec > now_msec) {
			reactor_set_event_timestamp(reactor, o->stream.m_connect_end_msec);
			break;
		}
		o->m_valid = 0;
		o->detach_error = REACTOR_CONNECT_ERR;
		reactorobject_invalid_inner_handler(reactor, o, now_msec);
	}
}

static void reactor_stream_writeev(Reactor_t* reactor, ReactorObject_t* o) {
	List_t finishedlist;
	ListNode_t* cur, *next;
	ChannelBase_t* channel = streamChannel(o);
	StreamTransportCtx_t* ctxptr = &channel->stream_ctx;
	for (cur = ctxptr->sendlist.head; cur; cur = cur->next) {
		int res;
		NetPacket_t* packet = pod_container_of(cur, NetPacket_t, node);
		if (NETPACKET_FIN == packet->type) {
			packet->off = packet->hdrlen + packet->bodylen;
			channel->has_sendfin = 1;
			if (channel->has_recvfin)
				o->m_valid = 0;
			else {
				channel->m_stream_sendfinwait = 0;
				socketShutdown(o->fd, SHUT_WR);
			}
			break;
		}
		if (packet->off >= packet->hdrlen + packet->bodylen) {
			continue;
		}
		res = socketWrite(o->fd, packet->buf + packet->off, packet->hdrlen + packet->bodylen - packet->off, 0, NULL, 0);
		if (res < 0) {
			if (errnoGet() != EWOULDBLOCK) {
				o->m_valid = 0;
				o->detach_error = REACTOR_IO_ERR;
				return;
			}
			res = 0;
		}
		packet->off += res;
		if (packet->off >= packet->hdrlen + packet->bodylen) {
			continue;
		}
		if (reactorobject_request_write(reactor, o)) {
			break;
		}
		else {
			o->m_valid = 0;
			o->detach_error = REACTOR_IO_ERR;
			return;
		}
	}
	finishedlist = streamtransportctxRemoveFinishedSendPacket(ctxptr);
	for (cur = finishedlist.head; cur; cur = next) {
		next = cur->next;
		reactorpacketFree(pod_container_of(cur, ReactorPacket_t, _.node));
	}
}

static void reactor_stream_accept(ReactorObject_t* o, long long timestamp_msec) {
	ChannelBase_t* channel = streamChannel(o);
	Sockaddr_t saddr;
	FD_t connfd;
	for (connfd = nioAcceptFirst(o->fd, o->m_readol, &saddr.st);
		connfd != INVALID_FD_HANDLE;
		connfd = nioAcceptNext(o->fd, &saddr.st))
	{
		channel->on_ack_halfconn(channel, connfd, &saddr.sa, timestamp_msec);
	}
}

static void reactor_stream_readev(Reactor_t* reactor, ReactorObject_t* o, long long timestamp_msec) {
	Sockaddr_t from_addr;
	ChannelBase_t* channel;
	int res = socketTcpReadableBytes(o->fd);
	if (res < 0) {
		o->m_valid = 0;
		return;
	}
	else if (0 == res) {
		stream_recvfin_handler(o);
		return;
	}
	channel = streamChannel(o);
	if (channel->readcache_max_size > 0 && channel->readcache_max_size < o->m_inbuflen + res) {
		o->m_valid = 0;
		o->detach_error = REACTOR_ONREAD_ERR;
		return;
	}
	if (o->m_inbufsize < o->m_inbuflen + res) {
		unsigned char* ptr = (unsigned char*)realloc(o->m_inbuf, o->m_inbuflen + res + 1);
		if (!ptr) {
			o->m_valid = 0;
			return;
		}
		o->m_inbuf = ptr;
		o->m_inbufsize = o->m_inbuflen + res;
	}
	res = socketRead(o->fd, o->m_inbuf + o->m_inbuflen, res, 0, &from_addr.st);
	if (res < 0) {
		if (errnoGet() != EWOULDBLOCK) {
			o->m_valid = 0;
			o->detach_error = REACTOR_IO_ERR;
		}
		return;
	}
	else if (0 == res) {
		stream_recvfin_handler(o);
		return;
	}
	o->m_inbuflen += res;
	o->m_inbuf[o->m_inbuflen] = 0; /* convienent for text data */
	res = channel->on_read(channel, o->m_inbuf, o->m_inbuflen, o->m_inbufoff, timestamp_msec, &from_addr.sa);
	if (res > 0) {
		channel->m_heartbeat_times = 0;
		channel->m_heartbeat_msec = channel_next_heartbeat_timestamp(channel, timestamp_msec);
		channel_set_timestamp(channel, channel->m_heartbeat_msec);
	}
	if (res < 0 || !after_call_channel_interface(channel)) {
		o->m_valid = 0;
		o->detach_error = REACTOR_ONREAD_ERR;
		return;
	}
	o->m_inbufoff = res;
	if (o->m_inbufoff >= o->m_inbuflen) {
		if (o->stream.inbuf_saved) {
			o->m_inbufoff = 0;
			o->m_inbuflen = 0;
		}
		else {
			free_inbuf(o);
		}
	}
	if (!channel->m_stream_sendfinwait) {
		return;
	}
	if (channel->stream_ctx.send_all_acked) {
		reactor_stream_writeev(reactor, o);
	}
}

static void channel_cachepacket_send_proc(Reactor_t*, ChannelBase_t*, long long);
static void reactor_dgram_readev(Reactor_t* reactor, ReactorObject_t* o, long long timestamp_msec) {
	Sockaddr_t from_addr;
	unsigned int readtimes, readmaxtimes = 8;
	for (readtimes = 0; readtimes < readmaxtimes; ++readtimes) {
		int res;
		unsigned char* ptr;
		ListNode_t* cur, * next;
		if (readtimes) {
			if (!o->m_inbuf) {
				o->m_inbuf = (unsigned char*)malloc(o->read_fragment_size + 1);
				if (!o->m_inbuf) {
					o->m_valid = 0;
					return;
				}
				o->m_inbuflen = o->m_inbufsize = o->read_fragment_size;
			}
			ptr = o->m_inbuf;
			res = socketRead(o->fd, o->m_inbuf, o->m_inbuflen, 0, &from_addr.st);
		}
		else {
			Iobuf_t iov;
			if (0 == nioOverlappedReadResult(o->m_readol, &iov, &from_addr.st)) {
				++readmaxtimes;
				continue;
			}
			ptr = (unsigned char*)iobufPtr(&iov);
			res = iobufLen(&iov);
		}

		if (res < 0) {
			if (errnoGet() != EWOULDBLOCK) {
				o->m_valid = 0;
			}
			return;
		}
		ptr[res] = 0; /* convienent for text data */
		for (cur = o->m_channel_list.head; cur; cur = next) {
			ChannelBase_t* channel = pod_container_of(cur, ChannelBase_t, regcmd._);
			int disable_send = channel->disable_send;
			next = cur->next;
			channel->on_read(channel, ptr, res, 0, timestamp_msec, &from_addr.sa);
			if (!after_call_channel_interface(channel)) {
				continue;
			}
			if (disable_send && !channel->disable_send) {
				channel_cachepacket_send_proc(reactor, channel, timestamp_msec);
			}
			channel->m_heartbeat_times = 0;
			channel->m_heartbeat_msec = channel_next_heartbeat_timestamp(channel, timestamp_msec);
			channel_set_timestamp(channel, channel->m_heartbeat_msec);
		}
	}
}

static void reactor_packet_send_proc_stream(Reactor_t* reactor, ReactorPacket_t* packet, long long timestamp_msec) {
	int packet_allow_send;
	ChannelBase_t* channel = packet->channel;
	ReactorObject_t* o;
	StreamTransportCtx_t* ctx;

	if (!channel->valid) {
		if (!packet->_.cached) {
			reactorpacketFree(packet);
		}
		return;
	}
	o = channel->o;
	if (!o->m_connected) {
		packet_allow_send = 0;
	}
	else if (!channel->disable_send) {
		packet_allow_send = 1;
	}
	else {
		packet_allow_send = 0;
	}

	if (packet_allow_send && NETPACKET_FIN != packet->_.type) {
		if (channel->on_pre_send) {
			int continue_send = channel->on_pre_send(channel, packet, timestamp_msec);
			if (!after_call_channel_interface(channel)) {
				if (!packet->_.cached) {
					reactorpacketFree(packet);
				}
				if (!o->m_valid) {
					reactorobject_invalid_inner_handler(reactor, o, timestamp_msec);
				}
				return;
			}
			if (!continue_send) {
				return;
			}
		}
	}
	ctx = &channel->stream_ctx;
	packet->_.off = 0;
	if (NETPACKET_FIN == packet->_.type) {
		if (ctx->sendlist.head) {
			channel->m_stream_sendfinwait = 1;
			streamtransportctxCacheSendPacket(ctx, &packet->_);
		}
		else {
			channel->has_sendfin = 1;
			reactorpacketFree(packet);
			if (channel->has_recvfin) {
				o->m_valid = 0;
				reactorobject_invalid_inner_handler(reactor, o, timestamp_msec);
			}
			else {
				channel->m_stream_sendfinwait = 0;
				socketShutdown(o->fd, SHUT_WR);
			}
		}
		return;
	}
	if (!packet_allow_send) {
		listPushNodeBack(&channel->m_cache_packet_list, &packet->_.node);
		return;
	}
	if (!streamtransportctxSendCheckBusy(ctx)) {
		int res = socketWrite(o->fd, packet->_.buf, packet->_.hdrlen + packet->_.bodylen, 0, NULL, 0);
		if (res < 0) {
			if (errnoGet() != EWOULDBLOCK) {
				o->m_valid = 0;
				reactorobject_invalid_inner_handler(reactor, o, timestamp_msec);
				return;
			}
			res = 0;
		}
		packet->_.off = res;
	}
	if (streamtransportctxCacheSendPacket(ctx, &packet->_)) {
		if (packet->_.off >= packet->_.hdrlen + packet->_.bodylen) {
			return;
		}
		if (!reactorobject_request_write(reactor, o)) {
			o->m_valid = 0;
			o->detach_error = REACTOR_IO_ERR;
			reactorobject_invalid_inner_handler(reactor, o, timestamp_msec);
		}
		return;
	}
	reactorpacketFree(packet);
}

static void reactor_packet_send_proc_dgram(Reactor_t* reactor, ReactorPacket_t* packet, long long timestamp_msec) {
	int packet_allow_send;
	ChannelBase_t* channel = packet->channel;
	ReactorObject_t* o;
	struct sockaddr* paddr;
	int addrlen;

	if (!channel->valid) {
		if (!packet->_.cached) {
			reactorpacketFree(packet);
		}
		return;
	}
	o = channel->o;
	if (NETPACKET_SYN == packet->_.type || NETPACKET_SYN_ACK == packet->_.type) {
		packet_allow_send = 1;
	}
	else if (!channel->disable_send) {
		packet_allow_send = 1;
	}
	else {
		packet_allow_send = 0;
	}

	if (packet_allow_send) {
		if (channel->on_pre_send) {
			int continue_send = channel->on_pre_send(channel, packet, timestamp_msec);
			if (!after_call_channel_interface(channel)) {
				if (!packet->_.cached) {
					reactorpacketFree(packet);
				}
				if (!o->m_valid) {
					reactorobject_invalid_inner_handler(reactor, o, timestamp_msec);
				}
				return;
			}
			if (!continue_send) {
				return;
			}
		}
	}
	else {
		listPushNodeBack(&channel->m_cache_packet_list, &packet->_.node);
		return;
	}
	if (o->m_connected) {
		paddr = NULL;
		addrlen = 0;
	}
	else {
		paddr = &channel->to_addr.sa;
		addrlen = sockaddrLength(paddr);
	}
	socketWrite(o->fd, packet->_.buf, packet->_.hdrlen + packet->_.bodylen, 0, paddr, addrlen);
	if (packet->_.cached) {
		return;
	}
	reactorpacketFree(packet);
}

static void channel_cachepacket_send_proc(Reactor_t* reactor, ChannelBase_t* channel, long long timestamp_msec) {
	ListNode_t* cur, *next;
	List_t cache_list = channel->m_cache_packet_list;
	listInit(&channel->m_cache_packet_list);
	for (cur = cache_list.head; cur; cur = next) {
		ReactorPacket_t* packet = pod_container_of(cur, ReactorPacket_t, _.node);
		next = cur->next;
		if (channel->flag & CHANNEL_FLAG_STREAM) {
			reactor_packet_send_proc_stream(reactor, packet, timestamp_msec);
		}
		else {
			reactor_packet_send_proc_dgram(reactor, packet, timestamp_msec);
		}
	}
}

static int reactor_stream_connect(Reactor_t* reactor, ReactorObject_t* o, long long timestamp_msec) {
	ChannelBase_t* channel = streamChannel(o);
	if (o->m_writeol) {
		nioFreeOverlapped(o->m_writeol);
		o->m_writeol = NULL;
	}
	if (o->stream.m_connect_end_msec > 0) {
		listRemoveNode(&reactor->m_connect_endlist, &o->stream.m_connect_endnode);
		o->stream.m_connect_end_msec = 0;
	}
	if (!nioConnectCheckSuccess(o->fd)) {
		return 0;
	}
	if (!reactorobject_request_read(reactor, o)) {
		return 0;
	}
	o->m_connected = 1;
	if (~0 != channel->connected_times) {
		++channel->connected_times;
	}
	channel->disable_send = 0;
	if (channel->on_syn_ack) {
		channel->on_syn_ack(channel, timestamp_msec);
		if (!after_call_channel_interface(channel)) {
			return 0;
		}
	}
	channel->m_heartbeat_msec = channel_next_heartbeat_timestamp(channel, timestamp_msec);
	channel_set_timestamp(channel, channel->m_heartbeat_msec);
	channel_cachepacket_send_proc(reactor, channel, timestamp_msec);
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
			if (channel->flag & CHANNEL_FLAG_STREAM) {
				reactor_packet_send_proc_stream(reactor, packet, timestamp_msec);
			}
			else {
				reactor_packet_send_proc_dgram(reactor, packet, timestamp_msec);
			}
			continue;
		}
		else if (REACTOR_STREAM_SENDFIN_CMD == cmd->type) {
			ReactorObject_t* o;
			ChannelBase_t* channel = pod_container_of(cmd, ChannelBase_t, stream_sendfincmd);
			if (!channel->valid) {
				continue;
			}
			o = channel->o;
			channel->has_sendfin = 1;
			channel->m_stream_sendfinwait = 0;
			socketShutdown(o->fd, SHUT_WR);
			if (channel->has_recvfin) {
				o->m_valid = 0;
				reactorobject_invalid_inner_handler(reactor, o, timestamp_msec);
			}
			continue;
		}
		else if (REACTOR_OBJECT_REG_CMD == cmd->type) {
			ReactorObject_t* o = pod_container_of(cmd, ReactorObject_t, regcmd);
			if (!reactor_reg_object(reactor, o, timestamp_msec)) {
				o->m_valid = 0;
				o->detach_error = REACTOR_REG_ERR;
				reactorobject_invalid_inner_handler(reactor, o, timestamp_msec);
				continue;
			}
			reactor_exec_object_reg_callback(reactor, o, timestamp_msec);
			if (!o->m_valid) {
				reactorobject_invalid_inner_handler(reactor, o, timestamp_msec);
				continue;
			}
			continue;
		}
		else if (REACTOR_OBJECT_FREE_CMD == cmd->type) {
			ReactorObject_t* o = pod_container_of(cmd, ReactorObject_t, freecmd);
			reactorobject_free(o);
			continue;
		}
		else if (REACTOR_CHANNEL_FREE_CMD == cmd->type) {
			ChannelBase_t* channel = pod_container_of(cmd, ChannelBase_t, freecmd);
			channelobject_free(channel);
			continue;
		}
	}
}

static int objht_keycmp(const HashtableNodeKey_t* node_key, const HashtableNodeKey_t* key) {
	return *(FD_t*)(node_key->ptr) != *(FD_t*)(key->ptr);
}

static unsigned int objht_keyhash(const HashtableNodeKey_t* key) { return *(FD_t*)(key->ptr); }

Reactor_t* reactorInit(Reactor_t* reactor) {
	if (!nioCreate(&reactor->m_nio)) {
		return NULL;
	}
	if (!criticalsectionCreate(&reactor->m_cmdlistlock)) {
		nioClose(&reactor->m_nio);
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

void reactorCommitCmd(Reactor_t* reactor, ReactorCmd_t* cmdnode) {
	if (REACTOR_OBJECT_REG_CMD == cmdnode->type) {
		ReactorObject_t* o = pod_container_of(cmdnode, ReactorObject_t, regcmd);
		if (_xchg8(&o->m_reghaspost, 1)) {
			return;
		}
		else {
			ListNode_t* cur, *next;
			for (cur = o->m_channel_list.head; cur; cur = next) {
				ChannelBase_t* channel = pod_container_of(cur, ChannelBase_t, regcmd._);
				next = cur->next;
				channel->reactor = reactor;
			}
		}
	}
	else if (REACTOR_STREAM_SENDFIN_CMD == cmdnode->type) {
		ChannelBase_t* channel = pod_container_of(cmdnode, ChannelBase_t, stream_sendfincmd);
		if (_xchg8(&channel->m_stream_has_sendfincmd, 1)) {
			return;
		}
		reactor = channel->reactor;
		if (!reactor) {
			return;
		}
	}
	else if (REACTOR_OBJECT_FREE_CMD == cmdnode->type) {
		ReactorObject_t* o = pod_container_of(cmdnode, ReactorObject_t, freecmd);
		reactorobject_free(o);
		return;
	}
	else if (REACTOR_CHANNEL_FREE_CMD == cmdnode->type) {
		ChannelBase_t* channel = pod_container_of(cmdnode, ChannelBase_t, freecmd);
		if (_xadd32(&channel->refcnt, -1) > 1) {
			return;
		}
		reactor = channel->reactor;
		if (channel->on_free) {
			channel->on_free(channel);
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

int reactorHandle(Reactor_t* reactor, NioEv_t e[], int n, long long timestamp_msec, int wait_msec) {
	if (reactor->m_event_msec > timestamp_msec) {
		int checkexpire_wait_msec = reactor->m_event_msec - timestamp_msec;
		if (checkexpire_wait_msec < wait_msec || wait_msec < 0)
			wait_msec = checkexpire_wait_msec;
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
			HashtableNodeKey_t hkey;
			HashtableNode_t* find_node;
			ReactorObject_t* o;
			FD_t fd;
			if (!nioEventOverlappedCheck(&reactor->m_nio, e + i)) {
				continue;
			}
			fd = nioEventFD(e + i);
			hkey.ptr = &fd;
			find_node = hashtableSearchKey(&reactor->m_objht, hkey);
			if (!find_node) {
				continue;
			}
			o = pod_container_of(find_node, ReactorObject_t, m_hashnode);
			if (!o->m_valid) {
				continue;
			}
			switch (nioEventOpcode(e + i)) {
				case NIO_OP_READ:
				{
					o->m_readol_has_commit = 0;
					if (SOCK_STREAM == o->socktype) {
						if (o->stream.m_listened) {
							reactor_stream_accept(o, timestamp_msec);
						}
						else {
							reactor_stream_readev(reactor, o, timestamp_msec);
						}
					}
					else {
						reactor_dgram_readev(reactor, o, timestamp_msec);
					}
					if (o->m_valid && !reactorobject_request_read(reactor, o)) {
						o->m_valid = 0;
						o->detach_error = REACTOR_IO_ERR;
					}
					break;
				}
				case NIO_OP_WRITE:
				{
					o->m_writeol_has_commit = 0;
					if (SOCK_STREAM == o->socktype) {
						if (o->m_connected) {
							reactor_stream_writeev(reactor, o);
						}
						else if (!reactor_stream_connect(reactor, o, timestamp_msec)) {
							o->m_valid = 0;
							o->detach_error = REACTOR_CONNECT_ERR;
						}
					}
					break;
				}
				default:
				{
					o->m_valid = 0;
				}
			}
			if (o->m_valid) {
				continue;
			}
			reactorobject_invalid_inner_handler(reactor, o, timestamp_msec);
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
	nioClose(&reactor->m_nio);
	criticalsectionClose(&reactor->m_cmdlistlock);
	do {
		ListNode_t* cur, *next;
		for (cur = reactor->m_cmdlist.head; cur; cur = next) {
			ReactorCmd_t* cmd = pod_container_of(cur, ReactorCmd_t, _);
			next = cur->next;
			if (REACTOR_OBJECT_REG_CMD == cmd->type) {
				ReactorObject_t* o = pod_container_of(cmd, ReactorObject_t, regcmd);
				free(o);
			}
			else if (REACTOR_OBJECT_FREE_CMD == cmd->type) {
				ReactorObject_t* o = pod_container_of(cmd, ReactorObject_t, freecmd);
				reactorobject_free(o);
			}
			else if (REACTOR_CHANNEL_FREE_CMD == cmd->type) {
				ChannelBase_t* channel = pod_container_of(cmd, ChannelBase_t, freecmd);
				free(channel);
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
			next = hashtableNextNode(cur);
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
}

/*****************************************************************************************/

static void reactorobject_init_comm(ReactorObject_t* o) {
	o->detach_error = 0;
	o->regcmd.type = REACTOR_OBJECT_REG_CMD;
	o->freecmd.type = REACTOR_OBJECT_FREE_CMD;

	o->m_connected = 0;
	listInit(&o->m_channel_list);
	o->m_hashnode.key.ptr = &o->fd;
	o->m_reghaspost = 0;
	o->m_valid = 1;
	o->m_has_inserted = 0;
	o->m_has_detached = 0;
	o->m_readol_has_commit = 0;
	o->m_writeol_has_commit = 0;
	o->m_readol = NULL;
	o->m_writeol = NULL;
	o->m_invalid_msec = 0;
	o->m_inbuf = NULL;
	o->m_inbuflen = 0;
	o->m_inbufoff = 0;
	o->m_inbufsize = 0;
}

ReactorObject_t* reactorobjectOpen(FD_t fd, int domain, int socktype, int protocol) {
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
	o->fd = fd;
	o->domain = domain;
	o->socktype = socktype;
	o->protocol = protocol;
	o->detach_timeout_msec = 0;
	if (SOCK_STREAM == socktype) {
		memset(&o->stream, 0, sizeof(o->stream));
		o->stream.inbuf_saved = 1;
		o->read_fragment_size = 1460;
		o->write_fragment_size = ~0;
	}
	else {
		o->read_fragment_size = 1464;
		o->write_fragment_size = 548;
	}
	reactorobject_init_comm(o);
	return o;
}

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

void reactorpacketFreeList(List_t* pkglist) {
	if (pkglist) {
		ListNode_t* cur, *next;
		for (cur = pkglist->head; cur; cur = next) {
			next = cur->next;
			reactorpacketFree(pod_container_of(cur, ReactorPacket_t, cmd._));
		}
		listInit(pkglist);
	}
}

ChannelBase_t* channelbaseOpen(size_t sz, unsigned short flag, ReactorObject_t* o, const struct sockaddr* addr) {
	ChannelBase_t* channel;
	int sockaddrlen;
	if (SOCK_STREAM == o->socktype) {
		if (flag & CHANNEL_FLAG_DGRAM) {
			return NULL;
		}
		if (!(flag & CHANNEL_FLAG_CLIENT) &&
			!(flag & CHANNEL_FLAG_SERVER) &&
			!(flag & CHANNEL_FLAG_LISTEN))
		{
			return NULL;
		}
		flag |= CHANNEL_FLAG_STREAM;
	}
	else {
		if (flag & CHANNEL_FLAG_STREAM) {
			return NULL;
		}
		flag |= CHANNEL_FLAG_DGRAM;
	}
	sockaddrlen = sockaddrLength(addr);
	if (sockaddrlen <= 0) {
		return NULL;
	}
	channel = (ChannelBase_t*)calloc(1, sz);
	if (!channel) {
		return NULL;
	}
	channel->refcnt = 1;
	channel->flag = flag;
	channel->o = o;
	channel->freecmd.type = REACTOR_CHANNEL_FREE_CMD;
	if (flag & CHANNEL_FLAG_STREAM) {
		memcpy(&o->stream.m_connect_addr, addr, sockaddrlen);
		streamtransportctxInit(&channel->stream_ctx, 0);
		channel->stream_sendfincmd.type = REACTOR_STREAM_SENDFIN_CMD;
	}
	else {
		dgramtransportctxInit(&channel->dgram_ctx, 0);
	}
	memcpy(&channel->to_addr, addr, sockaddrlen);
	memcpy(&channel->connect_addr, addr, sockaddrlen);
	memcpy(&channel->listen_addr, addr, sockaddrlen);
	channel->valid = 1;
	channel->write_fragment_size = o->write_fragment_size;
	listInit(&channel->m_cache_packet_list);
	listPushNodeBack(&o->m_channel_list, &channel->regcmd._);
	return channel;
}

ChannelBase_t* channelbaseAddRef(ChannelBase_t* channel) {
	_xadd32(&channel->refcnt, 1);
	return channel;
}

void channelbaseSendPacket(ChannelBase_t* channel, ReactorPacket_t* packet) {
	packet->channel = channel;
	reactorCommitCmd(channel->reactor, &packet->cmd);
}

void channelbaseSendPacketList(ChannelBase_t* channel, List_t* packetlist) {
	ListNode_t* cur;
	if (!packetlist->head) {
		return;
	}
	for (cur = packetlist->head; cur; cur = cur->next) {
		ReactorPacket_t* packet = pod_container_of(cur, ReactorPacket_t, cmd._);
		packet->channel = channel;
	}
	reactor_commit_cmdlist(channel->reactor, packetlist);
}

#ifdef	__cplusplus
}
#endif
