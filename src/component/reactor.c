//
// Created by hujianzhe
//

#include "../../inc/component/reactor.h"
#include "../../inc/sysapi/error.h"
#include "../../inc/sysapi/time.h"
#include <stdlib.h>
#include <string.h>

#define	allChannelDoAction(o, var_name, do_action)\
do {\
	ListNode_t* cur, *next;\
	for (cur = (o)->m_channel_list.head; cur; cur = next) {\
		var_name = pod_container_of(cur, ChannelBase_t, regcmd._);\
		next = cur->next;\
		do {\
			do_action\
		} while (0);\
	}\
} while (0)
#define	streamChannel(o)\
(o->m_channel_list.head ? pod_container_of(o->m_channel_list.head, ChannelBase_t, regcmd._) : NULL)

typedef struct ReuseCmd_t {
	ReactorCmd_t _;
	ChannelBase_t* channel;
	Sockaddr_t toaddr;
} ReuseCmd_t;

typedef struct ReuseFinishCmd_t {
	ReactorCmd_t _;
	ChannelBase_t* channel;
	ReactorPacket_t* retpkg;
} ReuseFinishCmd_t;

#ifdef	__cplusplus
extern "C" {
#endif

static void reactor_set_event_timestamp(Reactor_t* reactor, long long timestamp_msec) {
	if (timestamp_msec <= 0)
		return;
	else if (reactor->m_event_msec <= 0 || reactor->m_event_msec > timestamp_msec)
		reactor->m_event_msec = timestamp_msec;
}

static void channel_detach_handler(ChannelBase_t* channel, int error, long long timestamp_msec) {
	ReactorObject_t* o;
	if (channel->m_has_detached)
		return;
	channel->m_has_detached = 1;
	channel->detach_error = error;
	o = channel->o;
	listRemoveNode(&o->m_channel_list, &channel->regcmd._);
	if (!o->m_channel_list.head)
		o->m_valid = 0;
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

static void reactorobject_invalid_inner_handler(Reactor_t* reactor, ReactorObject_t* o, long long now_msec) {
	if (o->m_has_detached)
		return;
	o->m_has_detached = 1;
	o->m_invalid_msec = now_msec;
	if (o->m_has_inserted) {
		o->m_has_inserted = 0;
		hashtableRemoveNode(&reactor->m_objht, &o->m_hashnode);
	}
	if (SOCK_STREAM == o->socktype)
		free_io_resource(o);

	allChannelDoAction(o, ChannelBase_t* channel,
		if (REACTOR_CONNECT_ERR == o->detach_error)
			channel->detach_error = o->detach_error;
		else if (REACTOR_IO_ERR == o->detach_error && 0 == channel->detach_error)
			channel->detach_error = o->detach_error;
		channel->m_has_detached = 1;
		channel->o = NULL;
		channel->valid = 0;
		channel->on_detach(channel);
	);
	listInit(&o->m_channel_list);

	if (o->detach_timeout_msec <= 0) {
		if (SOCK_STREAM != o->socktype)
			free_io_resource(o);
		reactorobject_free(o);
	}
	else {
		listInsertNodeBack(&reactor->m_invalidlist, reactor->m_invalidlist.tail, &o->m_invalidnode);
		reactor_set_event_timestamp(reactor, o->m_invalid_msec + o->detach_timeout_msec);
	}
}

static int reactorobject_request_read(Reactor_t* reactor, ReactorObject_t* o) {
	Sockaddr_t saddr;
	int opcode;
	if (o->m_readol_has_commit)
		return 1;
	else if (SOCK_STREAM == o->socktype && o->stream.m_listened)
		opcode = NIO_OP_ACCEPT;
	else
		opcode = NIO_OP_READ;
	if (!o->m_readol) {
		o->m_readol = nioAllocOverlapped(opcode, NULL, 0, SOCK_STREAM != o->socktype ? o->read_fragment_size : 0);
		if (!o->m_readol)
			return 0;
	}
	saddr.sa.sa_family = o->domain;
	if (!nioCommit(&reactor->m_nio, o->fd, opcode, o->m_readol, &saddr.sa, sockaddrLength(&saddr)))
		return 0;
	o->m_readol_has_commit = 1;
	return 1;
}

static int reactorobject_request_write(Reactor_t* reactor, ReactorObject_t* o) {
	Sockaddr_t saddr;
	if (!o->m_valid)
		return 0;
	else if (o->m_writeol_has_commit)
		return 1;
	else if (!o->m_writeol) {
		o->m_writeol = nioAllocOverlapped(NIO_OP_WRITE, NULL, 0, 0);
		if (!o->m_writeol) {
			return 0;
		}
	}
	saddr.sa.sa_family = o->domain;
	if (!nioCommit(&reactor->m_nio, o->fd, NIO_OP_WRITE, o->m_writeol, &saddr.sa, sockaddrLength(&saddr)))
		return 0;
	o->m_writeol_has_commit = 1;
	return 1;
}

static int reactorobject_request_connect(Reactor_t* reactor, ReactorObject_t* o) {
	o->stream.m_connected = 0;
	if (!o->m_writeol) {
		o->m_writeol = nioAllocOverlapped(NIO_OP_CONNECT, NULL, 0, 0);
		if (!o->m_writeol)
			return 0;
	}
	if (!nioCommit(&reactor->m_nio, o->fd, NIO_OP_CONNECT, o->m_writeol,
		&o->stream.m_connect_addr.sa, sockaddrLength(&o->stream.m_connect_addr)))
	{
		return 0;
	}
	o->m_writeol_has_commit = 1;
	return 1;
}

static int reactor_reg_object_check(Reactor_t* reactor, ReactorObject_t* o) {
	if (!nioReg(&reactor->m_nio, o->fd))
		return 0;
	if (SOCK_STREAM == o->socktype) {
		BOOL ret;
		if (!socketIsListened(o->fd, &ret))
			return 0;
		if (ret) {
			o->stream.m_listened = 1;
			if (!reactorobject_request_read(reactor, o))
				return 0;
		}
		else if (!socketIsConnected(o->fd, &ret))
			return 0;
		else if (ret) {
			o->stream.m_connected = 1;
			streamChannel(o)->disable_send = 0;
			if (!reactorobject_request_read(reactor, o))
				return 0;
		}
		else {
			streamChannel(o)->disable_send = 1;
			if (!reactorobject_request_connect(reactor, o))
				return 0;
		}
	}
	else {
		BOOL bval;
		if (!socketHasAddr(o->fd, &bval))
			return 0;
		if (!bval) {
			Sockaddr_t local_addr;
			if (!sockaddrEncode(&local_addr.st, o->domain, NULL, 0))
				return 0;
			if (!socketBindAddr(o->fd, &local_addr.sa, sockaddrLength(&local_addr)))
				return 0;
		}
		if (!reactorobject_request_read(reactor, o))
			return 0;
		allChannelDoAction(o, ChannelBase_t* channel,
			if (channel->flag & CHANNEL_FLAG_CLIENT)
				channel->disable_send = 1;
		);
	}
	return 1;
}

static int reactor_reg_object(Reactor_t* reactor, ReactorObject_t* o, long long timestamp_msec) {
	if (reactor_reg_object_check(reactor, o)) {
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
	allChannelDoAction(o, channel,
		channel->reactor = reactor;
		if (channel->on_reg) {
			channel->on_reg(channel, timestamp_msec);
			channel->on_reg = NULL;
			after_call_channel_interface(channel);
		}
	);
	if (SOCK_STREAM != o->socktype || o->stream.m_listened || !o->stream.m_connected)
		return;
	channel = streamChannel(o);
	if (~0 != channel->connected_times) {
		++channel->connected_times;
	}
	if (channel->flag & CHANNEL_FLAG_CLIENT) {
		channel->on_syn_ack(channel, timestamp_msec);
		after_call_channel_interface(channel);
	}
}

static int reactor_unreg_object_check(Reactor_t* reactor, ReactorObject_t* o) {
	if (!nioUnReg(&reactor->m_nio, o->fd)) {
		return 0;
	}
	o->m_has_inserted = 0;
	hashtableRemoveNode(&reactor->m_objht, &o->m_hashnode);
	o->m_readol_has_commit = 0;
	o->m_writeol_has_commit = 0;
	return 1;
}

static void stream_default_sys_recvfin_handler(ChannelBase_t* channel, long long timestamp_msec) {
	if (_xchg8(&channel->m_stream_has_sendfincmd, 1))
		return;
	else {
		Reactor_t* reactor = channel->reactor;
		criticalsectionEnter(&reactor->m_cmdlistlock);
		listPushNodeBack(&reactor->m_cmdlist, &channel->stream_sendfincmd._);
		criticalsectionLeave(&reactor->m_cmdlistlock);
	}
}

static int stream_recvfin_handler(ReactorObject_t* o, long long timestamp_msec) {
	ChannelBase_t* channel = streamChannel(o);
	if (!channel->has_recvfin) {
		channel->has_recvfin = 1;
	}
	if (!o->stream.m_sys_has_recvfin) {
		o->stream.m_sys_has_recvfin = 1;
		if (channel->stream_on_sys_recvfin)
			channel->stream_on_sys_recvfin(channel, timestamp_msec);
	}
	return channel->has_sendfin;
}

static int stream_sendfin_direct_handler(ReactorObject_t* o) {
	ChannelBase_t* channel = streamChannel(o);
	channel->m_stream_sendfinwait = 0;
	if (!channel->has_sendfin) {
		channel->has_sendfin = 1;
	}
	return channel->has_recvfin;
}

static int stream_sendfin_check(ReactorObject_t* o) {
	ChannelBase_t* channel = streamChannel(o);
	if (channel->has_sendfin)
		return channel->has_recvfin;
	if (streamtransportctxSendCheckBusy(&channel->stream_ctx)) {
		channel->m_stream_sendfinwait = 1;
		return 0;
	}
	socketShutdown(o->fd, SHUT_WR);
	return stream_sendfin_direct_handler(o);
}

static void reactor_exec_object(Reactor_t* reactor, long long now_msec, long long ev_msec) {
	HashtableNode_t *cur, *next;
	for (cur = hashtableFirstNode(&reactor->m_objht); cur; cur = next) {
		ReactorObject_t* o = pod_container_of(cur, ReactorObject_t, m_hashnode);
		next = hashtableNextNode(cur);
		if (o->m_valid) {
			allChannelDoAction(o, ChannelBase_t* channel,
				if (now_msec < channel->event_msec) {
					reactor_set_event_timestamp(reactor, channel->event_msec);
					continue;
				}
				channel->event_msec = 0;
				channel->on_exec(channel, now_msec);
				after_call_channel_interface(channel);
			);
			if (o->m_valid)
				continue;
		}
		reactorobject_invalid_inner_handler(reactor, o, now_msec);
	}
}

static void reactor_exec_invalidlist(Reactor_t* reactor, long long now_msec) {
	ListNode_t* cur, *next;
	List_t invalidfreelist;
	listInit(&invalidfreelist);
	for (cur = reactor->m_invalidlist.head; cur; cur = next) {
		ReactorObject_t* o = pod_container_of(cur, ReactorObject_t, m_invalidnode);
		next = cur->next;
		if (o->m_invalid_msec + o->detach_timeout_msec > now_msec) {
			reactor_set_event_timestamp(reactor, o->m_invalid_msec + o->detach_timeout_msec);
			continue; /* not use timer heap, so continue,,,,this is temp... */
		}
		listRemoveNode(&reactor->m_invalidlist, cur);
		free_io_resource(o);
		listInsertNodeBack(&invalidfreelist, invalidfreelist.tail, cur);
	}
	for (cur = invalidfreelist.head; cur; cur = next) {
		ReactorObject_t* o = pod_container_of(cur, ReactorObject_t, m_invalidnode);
		next = cur->next;
		reactorobject_free(o);
	}
}

static void reactor_stream_writeev(Reactor_t* reactor, ReactorObject_t* o) {
	int busy = 0;
	List_t finishedlist;
	ListNode_t* cur, *next;
	ChannelBase_t* channel = streamChannel(o);
	StreamTransportCtx_t* ctxptr = &channel->stream_ctx;
	for (cur = ctxptr->sendlist.head; cur; cur = cur->next) {
		int res;
		NetPacket_t* packet = pod_container_of(cur, NetPacket_t, node);
		if (packet->off >= packet->hdrlen + packet->bodylen)
			continue;
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
			if (NETPACKET_FIN == packet->type && stream_sendfin_direct_handler(o)) {
				o->m_valid = 0;
			}
			else if (NETPACKET_SYN_ACK == packet->type) {
				break;
			}
			continue;
		}
		if (reactorobject_request_write(reactor, o)) {
			busy = 1;
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
	if (busy)
		return;
	if (!channel->m_stream_sendfinwait)
		return;
	socketShutdown(o->fd, SHUT_WR);
	if (stream_sendfin_direct_handler(o))
		o->m_valid = 0;
}

static void reactor_stream_accept(ReactorObject_t* o, long long timestamp_msec) {
	ChannelBase_t* channel = streamChannel(o);
	Sockaddr_t saddr;
	FD_t connfd;
	for (connfd = nioAcceptFirst(o->fd, o->m_readol, &saddr.st);
		connfd != INVALID_FD_HANDLE;
		connfd = nioAcceptNext(o->fd, &saddr.st))
	{
		channel->on_ack_halfconn(channel, connfd, &saddr, timestamp_msec);
	}
}

static void channel_cachepacket_send_proc(Reactor_t*, ChannelBase_t*, long long);
static void reactor_readev(Reactor_t* reactor, ReactorObject_t* o, long long timestamp_msec) {
	Sockaddr_t from_addr;
	if (SOCK_STREAM == o->socktype) {
		ChannelBase_t* channel;
		int res = socketTcpReadableBytes(o->fd);
		if (res < 0) {
			o->m_valid = 0;
			return;
		}
		else if (0 == res) {
			if (stream_recvfin_handler(o, timestamp_msec) ||
				stream_sendfin_check(o))
			{
				o->m_valid = 0;
			}
			return;
		}
		else {
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
				if (stream_recvfin_handler(o, timestamp_msec) ||
					stream_sendfin_check(o))
				{
					o->m_valid = 0;
				}
				return;
			}
			o->m_inbuflen += res;
			o->m_inbuf[o->m_inbuflen] = 0; /* convienent for text data */
			channel = streamChannel(o);
			res = channel->on_read(channel, o->m_inbuf, o->m_inbuflen, o->m_inbufoff, timestamp_msec, &from_addr);
			if (!after_call_channel_interface(channel) || res < 0) {
				o->m_valid = 0;
				o->detach_error = REACTOR_ONREAD_ERR;
				return;
			}
			o->m_inbufoff = res;
			if (o->m_inbufoff >= o->m_inbuflen)
				free_inbuf(o);
			if (!channel->m_stream_sendfinwait)
				return;
			if (channel->stream_ctx.send_all_acked)
				reactor_stream_writeev(reactor, o);
		}
	}
	else {
		unsigned int readtimes, readmaxtimes = 8;
		for (readtimes = 0; readtimes < readmaxtimes; ++readtimes) {
			int res;
			unsigned char* ptr;
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
				if (0 == nioOverlappedData(o->m_readol, &iov, &from_addr.st)) {
					++readmaxtimes;
					continue;
				}
				ptr = (unsigned char*)iobufPtr(&iov);
				res = iobufLen(&iov);
			}

			if (res < 0) {
				if (errnoGet() != EWOULDBLOCK)
					o->m_valid = 0;
				return;
			}
			ptr[res] = 0; /* convienent for text data */
			allChannelDoAction(o, ChannelBase_t* channel,
				int disable_send = channel->disable_send;
				channel->on_read(channel, ptr, res, 0, timestamp_msec, &from_addr);
				after_call_channel_interface(channel);
				if (disable_send && !channel->disable_send) {
					channel_cachepacket_send_proc(reactor, channel, timestamp_msec);
				}
			);
		}
	}
}

static ReactorObject_t* reactorobject_dup(ReactorObject_t* o);
static void reactor_stream_client_reuse_proc(Reactor_t* reactor, ReuseCmd_t* cmdex, long long timestamp_msec) {
	ChannelBase_t* src_channel;
	ReactorObject_t* src_o;
	ReactorObject_t* reconnect_o;
	int sockaddrlen;
	src_channel = cmdex->channel;
	src_o = src_channel->o;
	reconnect_o = reactorobject_dup(src_o);
	if (!reconnect_o) {
		return;
	}
	sockaddrlen = sockaddrLength(&cmdex->toaddr);
	if (sockaddrlen > 0) {
		memcpy(&src_channel->to_addr, &cmdex->toaddr, sockaddrlen);
		memcpy(&reconnect_o->stream.m_connect_addr, &cmdex->toaddr, sockaddrlen);
	}
	else {
		sockaddrlen = sockaddrLength(&src_channel->to_addr);
		memcpy(&reconnect_o->stream.m_connect_addr, &src_channel->to_addr, sockaddrlen);
	}
	_xchg8(&reconnect_o->m_reghaspost, 1);
	if (!reactor_reg_object(reactor, reconnect_o, timestamp_msec)) {
		reactorobject_free(reconnect_o);
		return;
	}

	src_channel->disable_send = 1;
	packetlist_free_packet(&src_channel->stream_ctx.recvlist);
	packetlist_free_packet(&src_channel->stream_ctx.sendlist);
	streamtransportctxInit(&src_channel->stream_ctx, 0);

	listRemoveNode(&src_o->m_channel_list, &src_channel->regcmd._);
	listPushNodeBack(&reconnect_o->m_channel_list, &src_channel->regcmd._);
	src_channel->o = reconnect_o;
	src_o->m_valid = 0;
	reactorobject_invalid_inner_handler(reactor, src_o, timestamp_msec);

	reactor_exec_object_reg_callback(reactor, reconnect_o, timestamp_msec);
	if (!reconnect_o->m_valid) {
		reactorobject_invalid_inner_handler(reactor, reconnect_o, timestamp_msec);
		return;
	}
}

static void reactor_dgram_reuse_proc(Reactor_t* reactor, ReuseCmd_t* cmdex, long long timestamp_msec) {
	ChannelBase_t* channel = cmdex->channel;
	int sockaddrlen = sockaddrLength(&cmdex->toaddr);
	if (sockaddrlen > 0) {
		memcpy(&channel->to_addr, &cmdex->toaddr, sockaddrlen);
	}
	channel->disable_send = 1;
	packetlist_free_packet(&channel->dgram_ctx.recvlist);
	packetlist_free_packet(&channel->dgram_ctx.sendlist);
	dgramtransportctxInit(&channel->dgram_ctx, 0);

	if (channel->flag & CHANNEL_FLAG_CLIENT) {
		ReactorObject_t* o = channel->o;
		if (~0 != channel->connected_times) {
			++channel->connected_times;
		}
		channel->on_syn_ack(channel, timestamp_msec);
		after_call_channel_interface(channel);
		if (!o->m_valid) {
			reactorobject_invalid_inner_handler(reactor, o, timestamp_msec);
		}
	}
}

static void reactor_packet_send_proc(Reactor_t* reactor, ReactorPacket_t* packet, long long timestamp_msec) {
	int packet_allow_send;
	ChannelBase_t* channel = packet->channel;
	ReactorObject_t* o;
	if (!channel->valid) {
		reactorpacketFree(packet);
		return;
	}
	o = channel->o;
	if (SOCK_STREAM == o->socktype && !o->stream.m_connected) {
		packet_allow_send = 0;
	}
	else if (!channel->disable_send ||
		NETPACKET_SYN == packet->_.type || NETPACKET_SYN_ACK == packet->_.type)
	{
		packet_allow_send = 1;
	}
	else {
		packet_allow_send = 0;
	}

	if (packet_allow_send) {
		if (channel->on_pre_send) {
			int continue_send = channel->on_pre_send(channel, packet, timestamp_msec);
			if (!after_call_channel_interface(channel)) {
				reactorpacketFree(packet);
				if (!o->m_valid)
					reactorobject_invalid_inner_handler(reactor, o, timestamp_msec);
				return;
			}
			if (!continue_send)
				return;
		}
	}
	if (SOCK_STREAM == o->socktype) {
		StreamTransportCtx_t* ctx = &channel->stream_ctx;
		if (NETPACKET_FIN == packet->_.type && !ctx->send_all_acked) {
			if (o->stream.m_sys_has_recvfin) {
				o->m_valid = 0;
				reactorobject_invalid_inner_handler(reactor, o, timestamp_msec);
			}
			else {
				channel->m_stream_sendfinwait = 1;
				streamtransportctxCacheSendPacket(ctx, &packet->_);
			}
			return;
		}
		packet->_.off = 0;
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
			if (packet->_.off >= packet->_.hdrlen + packet->_.bodylen)
				return;
			if (!reactorobject_request_write(reactor, o)) {
				o->m_valid = 0;
				o->detach_error = REACTOR_IO_ERR;
				reactorobject_invalid_inner_handler(reactor, o, timestamp_msec);
			}
			return;
		}
		if (NETPACKET_FIN == packet->_.type && stream_sendfin_direct_handler(o)) {
			o->m_valid = 0;
			reactorobject_invalid_inner_handler(reactor, o, timestamp_msec);
		}
	}
	else {
		if (!packet_allow_send) {
			listPushNodeBack(&channel->m_cache_packet_list, &packet->_.node);
			return;
		}
		socketWrite(o->fd, packet->_.buf, packet->_.hdrlen + packet->_.bodylen, 0,
			&channel->to_addr, sockaddrLength(&channel->to_addr));
		if (packet->_.cached)
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
		reactor_packet_send_proc(reactor, packet, timestamp_msec);
	}
}

static int reactor_stream_connect(Reactor_t* reactor, ReactorObject_t* o, long long timestamp_msec) {
	ChannelBase_t* channel = streamChannel(o);
	if (o->m_writeol) {
		nioFreeOverlapped(o->m_writeol);
		o->m_writeol = NULL;
	}
	do {
		if (!nioConnectCheckSuccess(o->fd))
			break;
		if (!reactorobject_request_read(reactor, o))
			break;
		o->stream.m_connected = 1;
		if (~0 != channel->connected_times) {
			++channel->connected_times;
		}
		channel->disable_send = 0;
		channel->on_syn_ack(channel, timestamp_msec);
		after_call_channel_interface(channel);
		channel_cachepacket_send_proc(reactor, channel, timestamp_msec);
		return 1;
	} while (0);
	return 0;
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
			reactor_packet_send_proc(reactor, packet, timestamp_msec);
			continue;
		}
		else if (REACTOR_REUSE_CMD == cmd->type) {
			ReuseCmd_t* cmdex = pod_container_of(cmd, ReuseCmd_t, _);
			ChannelBase_t* channel = cmdex->channel;
			if (channel->valid) {
				if (channel->flag & CHANNEL_FLAG_STREAM) {
					if (channel->flag & CHANNEL_FLAG_CLIENT)
						reactor_stream_client_reuse_proc(reactor, cmdex, timestamp_msec);
				}
				else
					reactor_dgram_reuse_proc(reactor, cmdex, timestamp_msec);
			}
			free(cmdex);
			continue;
		}
		else if (REACTOR_REUSE_FINISH_CMD == cmd->type) {
			ReuseFinishCmd_t* cmdex = pod_container_of(cmd, ReuseFinishCmd_t, _);
			ChannelBase_t* channel = cmdex->channel;
			ReactorPacket_t* retpkg = cmdex->retpkg;
			ReactorObject_t* o;
			free(cmdex);
			if (!channel->valid) {
				reactorpacketFree(retpkg);
				continue;
			}
			if (!channel->disable_send) {
				reactorpacketFree(retpkg);
				continue;
			}
			o = channel->o;
			channel->disable_send = 0;
			if (retpkg) {
				retpkg->channel = channel;
				listPushNodeFront(&channel->m_cache_packet_list, &retpkg->_.node);
			}
			channel_cachepacket_send_proc(reactor, channel, timestamp_msec);
			continue;
		}
		else if (REACTOR_STREAM_SENDFIN_CMD == cmd->type) {
			ReactorObject_t* o;
			ChannelBase_t* channel = pod_container_of(cmd, ChannelBase_t, stream_sendfincmd);
			if (!channel->valid)
				continue;
			o = channel->o;
			if (stream_sendfin_check(o)) {
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

static int objht_keycmp(const void* node_key, const void* key) {
	return *(FD_t*)node_key != *(FD_t*)key;
}

static unsigned int objht_keyhash(const void* key) { return *(FD_t*)key; }

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
	hashtableInit(&reactor->m_objht,
		reactor->m_objht_bulks, sizeof(reactor->m_objht_bulks) / sizeof(reactor->m_objht_bulks[0]),
		objht_keycmp, objht_keyhash);
	reactor->m_runthreadhasbind = 0;
	reactor->m_event_msec = 0;
	return reactor;
}

void reactorWake(Reactor_t* reactor) {
	nioWakeup(&reactor->m_nio);
}

void reactorCommitCmd(Reactor_t* reactor, ReactorCmd_t* cmdnode) {
	if (REACTOR_OBJECT_REG_CMD == cmdnode->type) {
		ReactorObject_t* o = pod_container_of(cmdnode, ReactorObject_t, regcmd);
		if (_xchg8(&o->m_reghaspost, 1))
			return;
		allChannelDoAction(o, ChannelBase_t* channel, channel->reactor = reactor;);
	}
	else if (REACTOR_STREAM_SENDFIN_CMD == cmdnode->type) {
		ChannelBase_t* channel = pod_container_of(cmdnode, ChannelBase_t, stream_sendfincmd);
		if (_xchg8(&channel->m_stream_has_sendfincmd, 1))
			return;
		reactor = channel->reactor;
		if (!reactor)
			return;
	}
	else if (REACTOR_OBJECT_FREE_CMD == cmdnode->type) {
		ReactorObject_t* o = pod_container_of(cmdnode, ReactorObject_t, freecmd);
		reactorobject_free(o);
		return;
	}
	else if (REACTOR_CHANNEL_FREE_CMD == cmdnode->type) {
		ChannelBase_t* channel = pod_container_of(cmdnode, ChannelBase_t, freecmd);
		if (_xadd32(&channel->refcnt, -1) > 1)
			return;
		if (channel->on_free)
			channel->on_free(channel);
		reactor = channel->reactor;
	}
	else if (REACTOR_REUSE_CMD == cmdnode->type) {
		ReuseCmd_t* cmd = pod_container_of(cmdnode, ReuseCmd_t, _);
		reactor = cmd->channel->reactor;
	}
	else if (REACTOR_REUSE_FINISH_CMD == cmdnode->type) {
		ReuseFinishCmd_t* cmd = pod_container_of(cmdnode, ReuseFinishCmd_t, _);
		reactor = cmd->channel->reactor;
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
	if (!reactor->m_runthreadhasbind) {
		reactor->m_runthread = threadSelf();
		reactor->m_runthreadhasbind = 1;
	}

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
			HashtableNode_t* find_node;
			ReactorObject_t* o;
			FD_t fd;
			if (!nioEventOverlappedCheck(&reactor->m_nio, e + i))
				continue;
			fd = nioEventFD(e + i);
			find_node = hashtableSearchKey(&reactor->m_objht, &fd);
			if (!find_node)
				continue;
			o = pod_container_of(find_node, ReactorObject_t, m_hashnode);
			if (!o->m_valid)
				continue;
			switch (nioEventOpcode(e + i)) {
				case NIO_OP_READ:
					o->m_readol_has_commit = 0;
					if (SOCK_STREAM == o->socktype && o->stream.m_listened)
						reactor_stream_accept(o, timestamp_msec);
					else
						reactor_readev(reactor, o, timestamp_msec);
					if (o->m_valid && !reactorobject_request_read(reactor, o)) {
						o->m_valid = 0;
						o->detach_error = REACTOR_IO_ERR;
					}
					break;
				case NIO_OP_WRITE:
					o->m_writeol_has_commit = 0;
					if (SOCK_STREAM == o->socktype) {
						if (o->stream.m_connected)
							reactor_stream_writeev(reactor, o);
						else if (!reactor_stream_connect(reactor, o, timestamp_msec)) {
							o->m_valid = 0;
							o->detach_error = REACTOR_CONNECT_ERR;
						}
					}
					break;
				default:
					o->m_valid = 0;
			}
			if (o->m_valid)
				continue;
			reactorobject_invalid_inner_handler(reactor, o, timestamp_msec);
		}
	}
	reactor_exec_cmdlist(reactor, timestamp_msec);
	if (reactor->m_event_msec > 0 && timestamp_msec >= reactor->m_event_msec) {
		long long ev_msec = reactor->m_event_msec;
		reactor->m_event_msec = 0;
		reactor_exec_object(reactor, timestamp_msec, ev_msec);
		reactor_exec_invalidlist(reactor, timestamp_msec);
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

	listInit(&o->m_channel_list);
	o->m_hashnode.key = &o->fd;
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

static ReactorObject_t* reactorobject_dup(ReactorObject_t* o) {
	ReactorObject_t* dup_o = (ReactorObject_t*)malloc(sizeof(ReactorObject_t));
	if (!dup_o)
		return NULL;
	dup_o->fd = socket(o->domain, o->socktype, o->protocol);
	if (INVALID_FD_HANDLE == dup_o->fd) {
		free(dup_o);
		return NULL;
	}
	if (!socketNonBlock(dup_o->fd, TRUE)) {
		socketClose(dup_o->fd);
		free(dup_o);
		return NULL;
	}
	dup_o->domain = o->domain;
	dup_o->socktype = o->socktype;
	dup_o->protocol = o->protocol;
	dup_o->detach_timeout_msec = o->detach_timeout_msec;
	dup_o->write_fragment_size = o->write_fragment_size;
	dup_o->read_fragment_size = o->read_fragment_size;
	if (SOCK_STREAM == dup_o->socktype) {
		memset(&dup_o->stream, 0, sizeof(dup_o->stream));
	}
	reactorobject_init_comm(dup_o);
	return dup_o;
}

ReactorObject_t* reactorobjectOpen(FD_t fd, int domain, int socktype, int protocol) {
	int fd_create;
	ReactorObject_t* o = (ReactorObject_t*)malloc(sizeof(ReactorObject_t));
	if (!o)
		return NULL;
	if (INVALID_FD_HANDLE == fd) {
		fd = socket(domain, socktype, protocol);
		if (INVALID_FD_HANDLE == fd) {
			free(o);
			return NULL;
		}
		fd_create = 1;
	}
	else
		fd_create = 0;
	if (!socketNonBlock(fd, TRUE)) {
		if (fd_create)
			socketClose(fd);
		free(o);
		return NULL;
	}
	o->fd = fd;
	o->domain = domain;
	o->socktype = socktype;
	o->protocol = protocol;
	o->detach_timeout_msec = 0;
	if (SOCK_STREAM == socktype) {
		memset(&o->stream, 0, sizeof(o->stream));
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

ReactorCmd_t* reactorNewReuseCmd(ChannelBase_t* channel, const void* to_addr) {
	ReuseCmd_t* cmd;
	int sockaddrlen;
	if ((channel->flag & CHANNEL_FLAG_SERVER) &&
		(channel->flag & CHANNEL_FLAG_STREAM))
	{
		return NULL;
	}
	sockaddrlen = sockaddrLength(to_addr);
	if (sockaddrlen < 0)
		return NULL;
	cmd = (ReuseCmd_t*)calloc(1, sizeof(ReuseCmd_t));
	if (!cmd)
	return NULL;
	cmd->_.type = REACTOR_REUSE_CMD;
	cmd->channel = channel;
	if (sockaddrlen > 0)
		memcpy(&cmd->toaddr, to_addr, sockaddrlen);
	else
		cmd->toaddr.sa.sa_family = AF_UNSPEC;
	return &cmd->_;
}

ReactorCmd_t* reactorNewReuseFinishCmd(ChannelBase_t* src_channel, ReactorPacket_t* retpkg) {
	ReuseFinishCmd_t* cmd = (ReuseFinishCmd_t*)calloc(1, sizeof(ReuseFinishCmd_t));
	if (cmd) {
		cmd->_.type = REACTOR_REUSE_FINISH_CMD;
		cmd->channel = src_channel;
		cmd->retpkg = retpkg;
		return &cmd->_;
	}
	return NULL;
}

ChannelBase_t* channelbaseOpen(size_t sz, unsigned short flag, ReactorObject_t* o, const void* addr) {
	ChannelBase_t* channel;
	int sockaddrlen;
	if (SOCK_STREAM == o->socktype) {
		if (flag & CHANNEL_FLAG_DGRAM)
			return NULL;
		if (!(flag & CHANNEL_FLAG_CLIENT) &&
			!(flag & CHANNEL_FLAG_SERVER) &&
			!(flag & CHANNEL_FLAG_LISTEN))
		{
			return NULL;
		}
		flag |= CHANNEL_FLAG_STREAM;
	}
	else {
		if (flag & CHANNEL_FLAG_STREAM)
			return NULL;
		flag |= CHANNEL_FLAG_DGRAM;
	}
	sockaddrlen = sockaddrLength(addr);
	if (sockaddrlen <= 0) {
		return NULL;
	}
	channel = (ChannelBase_t*)calloc(1, sz);
	if (!channel)
		return NULL;
	channel->flag = flag;
	channel->o = o;
	channel->refcnt = 1;
	channel->freecmd.type = REACTOR_CHANNEL_FREE_CMD;
	if (flag & CHANNEL_FLAG_STREAM) {
		memcpy(&o->stream.m_connect_addr, addr, sockaddrlen);
		streamtransportctxInit(&channel->stream_ctx, 0);
		channel->stream_sendfincmd.type = REACTOR_STREAM_SENDFIN_CMD;
		channel->stream_on_sys_recvfin = stream_default_sys_recvfin_handler;
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

void channelbaseSendPacket(ChannelBase_t* channel, ReactorPacket_t* packet) {
	packet->cmd.type = REACTOR_SEND_PACKET_CMD;
	packet->channel = channel;
	reactorCommitCmd(channel->reactor, &packet->cmd);
}

void channelbaseSendPacketList(ChannelBase_t* channel, List_t* packetlist) {
	ListNode_t* cur;
	if (!packetlist->head)
		return;
	for (cur = packetlist->head; cur; cur = cur->next) {
		ReactorPacket_t* packet = pod_container_of(cur, ReactorPacket_t, cmd);
		packet->cmd.type = REACTOR_SEND_PACKET_CMD;
		packet->channel = channel;
	}
	reactor_commit_cmdlist(channel->reactor, packetlist);
}

#ifdef	__cplusplus
}
#endif
