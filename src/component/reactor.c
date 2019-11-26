//
// Created by hujianzhe
//

#include "../../inc/component/reactor.h"
#include "../../inc/sysapi/error.h"
#include "../../inc/sysapi/time.h"
#include <stdlib.h>

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

typedef struct StreamClientSideReconnectCmd_t {
	ReactorCmd_t _;
	ReactorObject_t* src_o;
	ReactorObject_t* dst_o;
} StreamClientSideReconnectCmd_t;

typedef struct StreamServerSideReconnectCmd_t {
	ReactorCmd_t _;
	ReactorObject_t* src_o;
	ReactorObject_t* dst_o;
	Reactor_t* dst_r;
	unsigned int recvseq;
	unsigned int cwdnseq;
	ReactorPacket_t* replypacket;
} StreamServerSideReconnectCmd_t;

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
	channel->on_detach(channel);
}

static void after_call_channel_interface(Reactor_t* reactor, ChannelBase_t* channel) {
	if (channel->valid) {
		reactor_set_event_timestamp(reactor, channel->event_msec);
	}
	else {
		channel_detach_handler(channel, channel->detach_error, 0);
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

static void reactorobject_invalid_inner_handler(ReactorObject_t* o, long long now_msec) {
	if (o->m_has_detached)
		return;
	o->m_has_detached = 1;
	o->m_invalid_msec = now_msec;
	if (o->m_has_inserted) {
		o->m_has_inserted = 0;
		hashtableRemoveNode(&o->reactor->m_objht, &o->m_hashnode);
	}
	if (SOCK_STREAM == o->socktype)
		free_io_resource(o);

	allChannelDoAction(o, ChannelBase_t* channel,
		if (REACTOR_CONNECT_ERR == o->detach_error)
			channel->detach_error = o->detach_error;
		else if (REACTOR_IO_ERR == o->detach_error && 0 == channel->detach_error)
			channel->detach_error = o->detach_error;
		channel->m_has_detached = 1;
		channel->valid = 0;
		channel->on_detach(channel);
	);
	listInit(&o->m_channel_list);

	if (o->detach_timeout_msec <= 0) {
		if (SOCK_STREAM != o->socktype)
			free_io_resource(o);
		o->on_detach(o);
	}
	else {
		Reactor_t* reactor = o->reactor;
		listInsertNodeBack(&reactor->m_invalidlist, reactor->m_invalidlist.tail, &o->m_invalidnode);
		reactor_set_event_timestamp(reactor, o->m_invalid_msec + o->detach_timeout_msec);
	}
}

static void stream_server_side_reconnectcmd_failure_handler(StreamServerSideReconnectCmd_t* cmd) {
	ReactorObject_t* o = cmd->src_o;
	free(cmd);
	o->m_valid = 0;
	reactorobject_invalid_inner_handler(o, 0);
}

static int reactorobject_request_read(ReactorObject_t* o) {
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
	if (!nioCommit(&o->reactor->m_nio, o->fd, opcode, o->m_readol, &saddr.sa, sockaddrLength(&saddr)))
		return 0;
	o->m_readol_has_commit = 1;
	return 1;
}

static int reactorobject_request_write(ReactorObject_t* o) {
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
	if (!nioCommit(&o->reactor->m_nio, o->fd, NIO_OP_WRITE, o->m_writeol, &saddr.sa, sockaddrLength(&saddr)))
		return 0;
	o->m_writeol_has_commit = 1;
	return 1;
}

static int reactorobject_request_connect(ReactorObject_t* o) {
	o->stream.m_connected = 0;
	if (!o->m_writeol) {
		o->m_writeol = nioAllocOverlapped(NIO_OP_CONNECT, NULL, 0, 0);
		if (!o->m_writeol)
			return 0;
	}
	if (!nioCommit(&o->reactor->m_nio, o->fd, NIO_OP_CONNECT, o->m_writeol,
		&o->stream.connect_addr.sa, sockaddrLength(&o->stream.connect_addr)))
	{
		return 0;
	}
	o->m_writeol_has_commit = 1;
	return 1;
}

static int reactor_reg_object_check(Reactor_t* reactor, ReactorObject_t* o) {
	o->reactor = reactor;
	if (!nioReg(&reactor->m_nio, o->fd))
		return 0;
	if (SOCK_STREAM == o->socktype) {
		BOOL ret;
		if (!socketIsListened(o->fd, &ret))
			return 0;
		if (ret) {
			o->stream.m_listened = 1;
			if (!reactorobject_request_read(o))
				return 0;
		}
		else if (!socketIsConnected(o->fd, &ret))
			return 0;
		else if (ret) {
			o->stream.m_connected = 1;
			if (!reactorobject_request_read(o))
				return 0;
		}
		else if (!reactorobject_request_connect(o)) {
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
		if (!reactorobject_request_read(o))
			return 0;
	}
	return 1;
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

static int reactorobject_recvfin_handler(ReactorObject_t* o, long long timestamp_msec) {
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

static int reactorobject_sendfin_direct_handler(ReactorObject_t* o) {
	ChannelBase_t* channel = streamChannel(o);
	channel->stream_sendfinwait = 0;
	if (!channel->has_sendfin) {
		channel->has_sendfin = 1;
	}
	return channel->has_recvfin;
}

static int reactorobject_sendfin_check(ReactorObject_t* o) {
	ChannelBase_t* channel = streamChannel(o);
	if (channel->has_sendfin)
		return channel->has_recvfin;
	if (streamtransportctxSendCheckBusy(&channel->stream_ctx)) {
		channel->stream_sendfinwait = 1;
		return 0;
	}
	socketShutdown(o->fd, SHUT_WR);
	return reactorobject_sendfin_direct_handler(o);
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
				after_call_channel_interface(o->reactor, channel);
			);
			if (o->m_valid)
				continue;
		}
		reactorobject_invalid_inner_handler(o, now_msec);
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
		o->on_detach(o);
	}
}

static void reactor_stream_writeev(ReactorObject_t* o) {
	int busy = 0;
	List_t finishedlist;
	ListNode_t* cur, *next;
	ChannelBase_t* channel = streamChannel(o);
	StreamTransportCtx_t* ctxptr = &channel->stream_ctx;
	for (cur = ctxptr->sendpacketlist.head; cur; cur = cur->next) {
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
			if (NETPACKET_FIN == packet->type && reactorobject_sendfin_direct_handler(o)) {
				o->m_valid = 0;
			}
			continue;
		}
		if (reactorobject_request_write(o)) {
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
	if (!channel->stream_sendfinwait)
		return;
	socketShutdown(o->fd, SHUT_WR);
	if (reactorobject_sendfin_direct_handler(o))
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

static void reactor_readev(ReactorObject_t* o, long long timestamp_msec) {
	Sockaddr_t from_addr;
	if (SOCK_STREAM == o->socktype) {
		ChannelBase_t* channel;
		int res = socketTcpReadableBytes(o->fd);
		if (res < 0) {
			o->m_valid = 0;
			return;
		}
		else if (0 == res) {
			if (reactorobject_recvfin_handler(o, timestamp_msec) ||
				reactorobject_sendfin_check(o))
			{
				o->m_valid = 0;
			}
			return;
		}
		else {
			if (o->m_inbufsize < o->m_inbuflen + res) {
				unsigned char* ptr = (unsigned char*)realloc(o->m_inbuf, o->m_inbuflen + res);
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
				if (reactorobject_recvfin_handler(o, timestamp_msec) ||
					reactorobject_sendfin_check(o))
				{
					o->m_valid = 0;
				}
				return;
			}
			o->m_inbuflen += res;
			channel = streamChannel(o);
			res = channel->on_read(channel, o->m_inbuf, o->m_inbuflen, o->m_inbufoff, timestamp_msec, &from_addr);
			if (res < 0 || !o->m_valid) {
				o->m_valid = 0;
				o->detach_error = REACTOR_ONREAD_ERR;
				return;
			}
			o->m_inbufoff = res;
			if (o->m_inbufoff >= o->m_inbuflen)
				free_inbuf(o);
			channel = streamChannel(o);
			if (!channel)
				return;
			after_call_channel_interface(o->reactor, channel);
			if (!channel->stream_sendfinwait)
				return;
			if (channel->stream_ctx.sendpacket_all_acked)
				reactor_stream_writeev(o);
		}
	}
	else {
		unsigned int readtimes, readmaxtimes = 8;
		for (readtimes = 0; readtimes < readmaxtimes; ++readtimes) {
			int res;
			unsigned char* ptr;
			if (readtimes) {
				if (!o->m_inbuf) {
					o->m_inbuf = (unsigned char*)malloc(o->read_fragment_size);
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
			allChannelDoAction(o, ChannelBase_t* channel,
				channel->on_read(channel, ptr, res, 0, timestamp_msec, &from_addr);
				after_call_channel_interface(o->reactor, channel);
			);
		}
	}
}

static int reactor_stream_connect(ReactorObject_t* o, long long timestamp_msec) {
	if (o->m_writeol) {
		nioFreeOverlapped(o->m_writeol);
		o->m_writeol = NULL;
	}
	if (!nioConnectCheckSuccess(o->fd)) {
		return 0;
	}
	else if (!reactorobject_request_read(o)) {
		return 0;
	}
	else {
		ChannelBase_t* channel = streamChannel(o);
		o->stream.m_connected = 1;
		channel->on_syn_ack(channel, timestamp_msec);
		after_call_channel_interface(o->reactor, channel);
		return 1;
	}
}

static void stream_reset_packet_off(List_t* sendpacketlist) {
	ListNode_t* cur;
	for (cur = sendpacketlist->head; cur; cur = cur->next) {
		NetPacket_t* packet = pod_container_of(cur, NetPacket_t, node);
		if (0 == packet->off)
			break;
		packet->off = 0;
	}
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
			ReactorObject_t* o = channel->o;
			if (!channel->valid || !o->m_valid) {
				reactorpacketFree(packet);
				continue;
			}
			if (channel->on_pre_send) {
				int ok = channel->on_pre_send(channel, packet, timestamp_msec);
				after_call_channel_interface(o->reactor, channel);
				if (!ok)
					continue;
			}
			if (SOCK_STREAM == o->socktype) {
				StreamTransportCtx_t* ctx = &channel->stream_ctx;
				if (NETPACKET_FIN == packet->_.type && !ctx->sendpacket_all_acked) {
					if (o->stream.m_sys_has_recvfin) {
						o->m_valid = 0;
						reactorobject_invalid_inner_handler(o, timestamp_msec);
					}
					else {
						channel->stream_sendfinwait = 1;
						streamtransportctxCacheSendPacket(ctx, &packet->_);
					}
					continue;
				}
				if (!streamtransportctxSendCheckBusy(ctx)) {
					int res = socketWrite(o->fd, packet->_.buf, packet->_.hdrlen + packet->_.bodylen, 0, NULL, 0);
					if (res < 0) {
						if (errnoGet() != EWOULDBLOCK) {
							o->m_valid = 0;
							reactorobject_invalid_inner_handler(o, timestamp_msec);
							continue;
						}
						res = 0;
					}
					packet->_.off = res;
				}
				if (streamtransportctxCacheSendPacket(ctx, &packet->_)) {
					if (packet->_.off < packet->_.hdrlen + packet->_.bodylen) {
						if (!reactorobject_request_write(o)) {
							o->m_valid = 0;
							o->detach_error = REACTOR_IO_ERR;
							reactorobject_invalid_inner_handler(o, timestamp_msec);
						}
					}
					continue;
				}
				if (NETPACKET_FIN == packet->_.type && reactorobject_sendfin_direct_handler(o)) {
					o->m_valid = 0;
					reactorobject_invalid_inner_handler(o, timestamp_msec);
				}
			}
			else {
				socketWrite(o->fd, packet->_.buf, packet->_.hdrlen + packet->_.bodylen, 0,
					&channel->to_addr, sockaddrLength(&channel->to_addr));
				if (packet->_.cached)
					continue;
			}
			reactorpacketFree(packet);
			continue;
		}
		else if (REACTOR_STREAM_CLIENT_SIDE_RECONNECT_CMD == cmd->type) {
			StreamClientSideReconnectCmd_t* cmdex = pod_container_of(cmd, StreamClientSideReconnectCmd_t, _);
			ReactorObject_t* src_o = cmdex->src_o;
			ReactorObject_t* dst_o = cmdex->dst_o;
			ChannelBase_t* src_channel = streamChannel(src_o);
			ChannelBase_t* dst_channel = streamChannel(dst_o);
			free(cmdex);
			if (!src_o->m_valid || !dst_o->m_valid) {
				continue;
			}
			listRemoveNode(&src_o->m_channel_list, &src_channel->regcmd._);
			listRemoveNode(&dst_o->m_channel_list, &dst_channel->regcmd._);
			listPushNodeBack(&dst_o->m_channel_list, &src_channel->regcmd._);
			listPushNodeBack(&src_o->m_channel_list, &dst_channel->regcmd._);
			src_channel->o = dst_o;
			src_o->m_valid = 0;
			reactorobject_invalid_inner_handler(src_o, timestamp_msec);
			stream_reset_packet_off(&src_channel->stream_ctx.sendpacketlist);
			reactor_stream_writeev(dst_o);
			continue;
		}
		else if (REACTOR_STREAM_SERVER_SIDE_RECONNECT_CMD == cmd->type) {
			StreamServerSideReconnectCmd_t* cmdex = pod_container_of(cmd, StreamServerSideReconnectCmd_t, _);
			ReactorObject_t* src_o = cmdex->src_o;
			if (!src_o->m_valid) {
				free(cmdex);
				continue;
			}
			else if (cmdex->dst_r != reactor) {
				if (!reactor_unreg_object_check(reactor, cmdex->src_o)) {
					stream_server_side_reconnectcmd_failure_handler(cmdex);
					continue;
				}
				// TODO: notify source thread unreg ok ......
			}
			else {
				ReactorObject_t* dst_o = cmdex->dst_o;
				ChannelBase_t* dst_c;
				if (!dst_o->m_valid) {
					stream_server_side_reconnectcmd_failure_handler(cmdex);
					continue;
				}
				dst_c = streamChannel(dst_o);
				if (dst_c->stream_ctx.m_recvseq < cmdex->cwdnseq ||
					dst_c->stream_ctx.m_cwndseq > cmdex->recvseq)
				{
					stream_server_side_reconnectcmd_failure_handler(cmdex);
					continue;
				}
				if (!src_o->m_has_inserted) {
					src_o->m_has_inserted = 1;
					hashtableInsertNode(&reactor->m_objht, &src_o->m_hashnode);
					if (!reactorobject_request_read(src_o)) {
						stream_server_side_reconnectcmd_failure_handler(cmdex);
						continue;
					}
				}
				listRemoveNode(&dst_o->m_channel_list, &dst_c->regcmd._);
				dst_o->m_valid = 0;
				reactorobject_invalid_inner_handler(dst_o, timestamp_msec);

				listPushNodeBack(&src_o->m_channel_list, &dst_c->regcmd._);
				dst_c->o = src_o;
				stream_reset_packet_off(&dst_c->stream_ctx.sendpacketlist);
				if (cmdex->replypacket)
					listPushNodeFront(&dst_c->stream_ctx.sendpacketlist, &cmdex->replypacket->_.node);
				free(cmdex);
				reactor_stream_writeev(src_o);
			}
			continue;
		}
		else if (REACTOR_STREAM_SENDFIN_CMD == cmd->type) {
			ReactorObject_t* o = pod_container_of(cmd, ReactorObject_t, stream.sendfincmd);
			if (!o->m_valid || !streamChannel(o))
				continue;
			if (reactorobject_sendfin_check(o)) {
				o->m_valid = 0;
				reactorobject_invalid_inner_handler(o, timestamp_msec);
			}
			continue;
		}
		else if (REACTOR_OBJECT_REG_CMD == cmd->type) {
			ReactorObject_t* o = pod_container_of(cmd, ReactorObject_t, regcmd);
			if (!reactor_reg_object_check(reactor, o)) {
				o->m_valid = 0;
				o->detach_error = REACTOR_REG_ERR;
				reactorobject_invalid_inner_handler(o, timestamp_msec);
				continue;
			}
			else {
				HashtableNode_t* htnode = hashtableInsertNode(&reactor->m_objht, &o->m_hashnode);
				if (htnode != &o->m_hashnode) {
					ReactorObject_t* exist_o = pod_container_of(htnode, ReactorObject_t, m_hashnode);
					hashtableReplaceNode(htnode, &o->m_hashnode);
					exist_o->m_valid = 0;
					exist_o->m_has_inserted = 0;
					reactorobject_invalid_inner_handler(exist_o, timestamp_msec);
				}
				o->m_has_inserted = 1;
			}
			if (o->on_reg)
				o->on_reg(o, timestamp_msec);
			allChannelDoAction(o, ChannelBase_t* channel,
				if (channel->on_reg) {
					channel->on_reg(channel, timestamp_msec);
					after_call_channel_interface(o->reactor, channel);
				}
			);
			if (SOCK_STREAM != o->socktype || o->stream.m_listened || !o->stream.m_connected)
				continue;
			else {
				ChannelBase_t* channel = streamChannel(o);
				if (channel && channel->on_syn_ack) {
					channel->on_syn_ack(channel, timestamp_msec);
					after_call_channel_interface(o->reactor, channel);
				}
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
			free(channel);
			continue;
		}

		if (reactor->cmd_exec)
			reactor->cmd_exec(cmd);
		if (reactor->cmd_free)
			reactor->cmd_free(cmd);
	}
}

static int objht_keycmp(const void* node_key, const void* key) {
	return *(FD_t*)node_key != *(FD_t*)key;
}

static unsigned int objht_keyhash(const void* key) { return *(FD_t*)key; }

Reactor_t* reactorInit(Reactor_t* reactor) {
	Sockaddr_t saddr = { 0 };

	if (!socketPair(SOCK_STREAM, reactor->m_socketpair))
		return NULL;
	saddr.sa.sa_family = AF_INET;

	reactor->m_readol = nioAllocOverlapped(NIO_OP_READ, NULL, 0, 0);
	if (!reactor->m_readol) {
		socketClose(reactor->m_socketpair[0]);
		socketClose(reactor->m_socketpair[1]);
		return NULL;
	}

	if (!nioCreate(&reactor->m_nio)) {
		nioFreeOverlapped(reactor->m_readol);
		socketClose(reactor->m_socketpair[0]);
		socketClose(reactor->m_socketpair[1]);
		return NULL;
	}

	if (!socketNonBlock(reactor->m_socketpair[0], TRUE) ||
		!socketNonBlock(reactor->m_socketpair[1], TRUE) ||
		!nioReg(&reactor->m_nio, reactor->m_socketpair[0]) ||
		!nioCommit(&reactor->m_nio, reactor->m_socketpair[0], NIO_OP_READ, reactor->m_readol,
			(struct sockaddr*)&saddr, sockaddrLength(&saddr)))
	{
		nioFreeOverlapped(reactor->m_readol);
		socketClose(reactor->m_socketpair[0]);
		socketClose(reactor->m_socketpair[1]);
		nioClose(&reactor->m_nio);
		return NULL;
	}

	if (!criticalsectionCreate(&reactor->m_cmdlistlock)) {
		nioFreeOverlapped(reactor->m_readol);
		socketClose(reactor->m_socketpair[0]);
		socketClose(reactor->m_socketpair[1]);
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
	reactor->m_wake = 0;

	reactor->cmd_exec = NULL;
	reactor->cmd_free = NULL;
	return reactor;
}

void reactorWake(Reactor_t* reactor) {
	if (0 == _cmpxchg16(&reactor->m_wake, 1, 0)) {
		char c;
		socketWrite(reactor->m_socketpair[1], &c, sizeof(c), 0, NULL, 0);
	}
}

void reactorCommitCmd(Reactor_t* reactor, ReactorCmd_t* cmdnode) {
	if (REACTOR_OBJECT_REG_CMD == cmdnode->type) {
		ReactorObject_t* o = pod_container_of(cmdnode, ReactorObject_t, regcmd);
		if (_xchg8(&o->m_reghaspost, 1))
			return;
		o->reactor = reactor;
	}
	else if (REACTOR_STREAM_SENDFIN_CMD == cmdnode->type) {
		ReactorObject_t* o = pod_container_of(cmdnode, ReactorObject_t, stream.sendfincmd);
		if (SOCK_STREAM != o->socktype || _xchg8(&o->stream.m_sendfincmdhaspost, 1))
			return;
	}
	else if (REACTOR_OBJECT_FREE_CMD == cmdnode->type) {
		ReactorObject_t* o = pod_container_of(cmdnode, ReactorObject_t, freecmd);
		if (!o->m_reghaspost || !reactor) {
			reactorobject_free(o);
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
			if (!nioEventOverlappedCheck(e + i))
				continue;
			fd = nioEventFD(e + i);
			if (fd == reactor->m_socketpair[0]) {
				Sockaddr_t saddr;
				char c[512];
				socketRead(fd, c, sizeof(c), 0, NULL);
				saddr.sa.sa_family = AF_INET;
				nioCommit(&reactor->m_nio, fd, NIO_OP_READ, reactor->m_readol, (struct sockaddr*)&saddr, sockaddrLength(&saddr));
				_xchg16(&reactor->m_wake, 0);
				continue;
			}
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
						reactor_readev(o, timestamp_msec);
					if (o->m_valid && !reactorobject_request_read(o)) {
						o->m_valid = 0;
						o->detach_error = REACTOR_IO_ERR;
					}
					break;
				case NIO_OP_WRITE:
					o->m_writeol_has_commit = 0;
					if (SOCK_STREAM == o->socktype) {
						if (o->stream.m_connected)
							reactor_stream_writeev(o);
						else if (!reactor_stream_connect(o, timestamp_msec)) {
							o->m_valid = 0;
							o->detach_error = REACTOR_CONNECT_ERR;
						}
					}
					else if (o->on_writeev)
						o->on_writeev(o, timestamp_msec);
					break;
				default:
					o->m_valid = 0;
			}
			if (o->m_valid)
				continue;
			reactorobject_invalid_inner_handler(o, timestamp_msec);
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
	nioFreeOverlapped(reactor->m_readol);
	socketClose(reactor->m_socketpair[0]);
	socketClose(reactor->m_socketpair[1]);
	nioClose(&reactor->m_nio);
	criticalsectionClose(&reactor->m_cmdlistlock);
	do {
		ListNode_t* cur, *next;
		for (cur = reactor->m_cmdlist.head; cur; cur = next) {
			ReactorCmd_t* cmd = pod_container_of(cur, ReactorCmd_t, _);
			next = cur->next;
			if (REACTOR_OBJECT_FREE_CMD == cmd->type) {
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
			else if (REACTOR_INNER_CMD > cmd->type)
				continue;
			else if (reactor->cmd_free)
				reactor->cmd_free(cmd);
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
	o->reactor = NULL;
	o->detach_error = 0;
	o->detach_timeout_msec = 0;
	o->write_fragment_size = (SOCK_STREAM == o->socktype) ? ~0 : 548;
	if (SOCK_STREAM == socktype) {
		memset(&o->stream, 0, sizeof(o->stream));
		o->stream.sendfincmd.type = REACTOR_STREAM_SENDFIN_CMD;
		o->read_fragment_size = 1460;
	}
	else {
		o->read_fragment_size = 1464;
	}
	o->regcmd.type = REACTOR_OBJECT_REG_CMD;
	o->freecmd.type = REACTOR_OBJECT_FREE_CMD;
	o->on_reg = NULL;
	o->on_writeev = NULL;
	o->on_detach = NULL;

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
		pkg->_.buf[0] = 0;
	}
	return pkg;
}

void reactorpacketFree(ReactorPacket_t* pkg) {
	free(pkg);
}

ChannelBase_t* channelbaseOpen(size_t sz, ReactorObject_t* o, const void* addr) {
	ChannelBase_t* channel = (ChannelBase_t*)calloc(1, sz);
	if (!channel)
		return NULL;
	channel->o = o;
	channel->freecmd.type = REACTOR_CHANNEL_FREE_CMD;
	if (SOCK_STREAM == o->socktype) {
		memcpy(&o->stream.connect_addr, addr, sockaddrLength(addr));
		streamtransportctxInit(&channel->stream_ctx, 0);
	}
	memcpy(&channel->to_addr, addr, sockaddrLength(addr));
	memcpy(&channel->connect_addr, addr, sockaddrLength(addr));
	memcpy(&channel->listen_addr, addr, sockaddrLength(addr));
	channel->valid = 1;
	listPushNodeBack(&o->m_channel_list, &channel->regcmd._);
	return channel;
}

void channelbaseSendPacket(ChannelBase_t* channel, ReactorPacket_t* packet) {
	packet->cmd.type = REACTOR_SEND_PACKET_CMD;
	packet->channel = channel;
	reactorCommitCmd(channel->o->reactor, &packet->cmd);
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
	reactor_commit_cmdlist(channel->o->reactor, packetlist);
}

static ReactorPacket_t* make_packet(const void* buf, unsigned int len, int off, int pktype) {
	ReactorPacket_t* packet = (ReactorPacket_t*)malloc(sizeof(ReactorPacket_t) + len);
	if (packet) {
		memset(packet, 0, sizeof(*packet));
		memcpy(packet->_.buf, buf, len);
		packet->_.hdrlen = 0;
		packet->_.bodylen = len;
		packet->_.off = off;
		packet->_.type = pktype;
	}
	return packet;
}

int channelbaseSend(ChannelBase_t* channel, int pktype, const void* buf, unsigned int len, const void* addr) {
	ReactorObject_t* o = channel->o;
	if (SOCK_STREAM == o->socktype) {
		ReactorPacket_t* packet;
		if (threadEqual(o->reactor->m_runthread, threadSelf())) {
			int res = 0;
			if (!streamtransportctxSendCheckBusy(&channel->stream_ctx)) {
				res = socketWrite(o->fd, buf, len, 0, NULL, 0);
				if (res < 0) {
					if (errnoGet() != EWOULDBLOCK) {
						o->m_valid = 0;
						o->detach_error = REACTOR_IO_ERR;
						reactor_set_event_timestamp(o->reactor, gmtimeMillisecond());
						return -1;
					}
					res = 0;
				}
				if (res >= len) {
					if (NETPACKET_FIN == pktype && reactorobject_sendfin_direct_handler(o)) {
						o->m_valid = 0;
						reactor_set_event_timestamp(o->reactor, gmtimeMillisecond());
					}
					return res;
				}
			}
			packet = make_packet(buf, len, res, pktype);
			if (!packet)
				return -1;
			streamtransportctxCacheSendPacket(&channel->stream_ctx, &packet->_);
			if (!reactorobject_request_write(o)) {
				o->m_valid = 0;
				o->detach_error = REACTOR_IO_ERR;
				reactor_set_event_timestamp(o->reactor, gmtimeMillisecond());
				return -1;
			}
			return res;
		}
		else {
			packet = make_packet(buf, len, 0, pktype);
			if (!packet)
				return -1;
			channelbaseSendPacket(channel, packet);
			return 0;
		}
	}
	else {
		return socketWrite(o->fd, buf, len, 0, addr, sockaddrLength(addr));
	}
}

#ifdef	__cplusplus
}
#endif
