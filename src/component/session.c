//
// Created by hujianzhe on 18-8-13.
//

#include "../../inc/sysapi/alloca.h"
#include "../../inc/sysapi/error.h"
#include "../../inc/sysapi/time.h"
#include "../../inc/component/session.h"
#include <stdlib.h>
#include <string.h>

enum {
	SESSION_SHUTDOWN_POST_MESSAGE = SESSION_CLOSE_MESSAGE + 1,
	SESSION_CLIENT_NET_RECONNECT_MESSAGE,
	SESSION_RECONNECT_RECOVERY_MESSAGE,
	SESSION_TRANSPORT_GRAB_ASYNC_REQ_MESSAGE,
	SESSION_TRANSPORT_GRAB_ASYNC_RET_MESSAGE,
	SESSION_REG_MESSAGE,
	SESSION_PACKET_MESSAGE
};
enum {
	HDR_SYN = 1,
	HDR_SYN_ACK,
	HDR_SYN_ACK_ACK,
	HDR_RECONNECT,
	HDR_RECONNECT_ACK,
	HDR_RECONNECT_ERR,
	HDR_FIN,
	HDR_FIN_ACK,
	HDR_DATA,
	HDR_ACK
};
enum {
	NO_STATUS = 0,
	LISTENED_STATUS,
	SYN_SENT_STATUS,
	SYN_RCVD_STATUS,
	RECONNECT_STATUS,
	ESTABLISHED_STATUS,
	ESTABLISHED_FIN_STATUS,
	FIN_WAIT_1_STATUS,
	FIN_WAIT_2_STATUS,
	CLOSE_WAIT_STATUS,
	TIME_WAIT_STATUS,
	LAST_ACK_STATUS,
	CLOSED_STATUS
};
#define	RELIABLE_STREAM_DATA_HDR_LEN	5
#define	RELIABLE_DGRAM_DATA_HDR_LEN		5
#define	HDR_DATA_END_FLAG				0x80
#define	MSL								30000

typedef struct Packet_t {
	SessionInternalMessage_t msg;
	Session_t* s;
	union {
		/* udp use */
		struct {
			unsigned char resendtimes;
			long long resend_timestamp_msec;
		};
		/* tcp use */
		struct {
			unsigned short type;
			unsigned short need_ack;
			size_t offset;
		};
	};
	unsigned int seq;
	size_t hdrlen;
	size_t len;
	unsigned char data[1];
} Packet_t;

typedef struct ReliableDgramHalfConnectPacket_t {
	ListNode_t m_listnode;
	FD_t sockfd;
	long long timestamp_msec;
	unsigned char resend_times;
	unsigned short local_port;
	struct sockaddr_storage peer_addr;
} ReliableDgramHalfConnectPacket_t;

typedef struct SessionTransportStatus_t {
	unsigned int m_cwndseq;
	unsigned int m_recvseq;
	unsigned int m_sendseq;
	List_t m_recvpacketlist;
	List_t m_sendpacketlist;
} SessionTransportStatus_t;

typedef struct SessionTransportGrabMsg_t {
	SessionInternalMessage_t msg;
	Session_t* s;
	SessionTransportStatus_t target_status;
	Mutex_t blocklock;
	unsigned int recvseq;
	unsigned int cwndseq;
	int success;
} SessionTransportGrabMsg_t;

#ifdef __cplusplus
extern "C" {
#endif

#define seq1_before_seq2(seq1, seq2)	((int)(seq1 - seq2) < 0)

static void update_timestamp(long long* dst, long long timestamp) {
	if (*dst <= 0 || *dst > timestamp)
		*dst = timestamp;
}

static SessionLoop_t* sessionloop_exec_msg(SessionLoop_t* loop, ListNode_t* msgnode) {
	criticalsectionEnter(&loop->m_msglistlock);
	listInsertNodeBack(&loop->m_msglist, loop->m_msglist.tail, msgnode);
	criticalsectionLeave(&loop->m_msglistlock);
	sessionloopWake(loop);
	return loop;
}

static SessionLoop_t* sessionloop_exec_msglist(SessionLoop_t* loop, List_t* msglist) {
	criticalsectionEnter(&loop->m_msglistlock);
	listMerge(&loop->m_msglist, msglist);
	criticalsectionLeave(&loop->m_msglistlock);
	sessionloopWake(loop);
	return loop;
}

static int reactorsocket_read(Session_t* s) {
	struct sockaddr_storage saddr;
	int opcode;
	if (!s->m_valid)
		return 0;
	else if (SESSION_TRANSPORT_LISTEN == s->transport_side && SOCK_STREAM == s->socktype) {
		opcode = NIO_OP_ACCEPT;
	}
	else {
		opcode = NIO_OP_READ;
	}
	if (!s->m_readol) {
		s->m_readol = nioAllocOverlapped(opcode, NULL, 0, SOCK_STREAM != s->socktype ? 65000 : 0);
		if (!s->m_readol) {
			s->m_valid = 0;
			return 0;
		}
	}
	saddr.ss_family = s->domain;
	if (nioCommit(&s->m_loop->m_reactor, s->fd, opcode, s->m_readol,
		(struct sockaddr*)&saddr, sockaddrLength((struct sockaddr*)&saddr)))
	{
		return 1;
	}
	s->m_valid = 0;
	return 0;
}

static int reactorsocket_write(Session_t* s) {
	struct sockaddr_storage saddr;
	if (s->m_writeol_has_commit)
		return 1;
	if (!s->m_writeol) {
		s->m_writeol = nioAllocOverlapped(NIO_OP_WRITE, NULL, 0, 0);
		if (!s->m_writeol) {
			s->m_valid = 0;
			return 0;
		}
	}
	saddr.ss_family = s->domain;
	if (nioCommit(&s->m_loop->m_reactor, s->fd, NIO_OP_WRITE, s->m_writeol,
		(struct sockaddr*)&saddr, sockaddrLength((struct sockaddr*)&saddr)))
	{
		s->m_writeol_has_commit = 1;
		return 1;
	}
	s->m_valid = 0;
	return 0;
}

static void free_io_resource(Session_t* s) {
	if (INVALID_FD_HANDLE != s->fd) {
		socketClose(s->fd);
		s->fd = INVALID_FD_HANDLE;
	}
	if (s->m_readol) {
		nioFreeOverlapped(s->m_readol);
		s->m_readol = NULL;
	}
	if (s->m_writeol) {
		nioFreeOverlapped(s->m_writeol);
		s->m_writeol = NULL;
	}
}

static void free_inbuf(TransportCtx_t* ctx) {
	free(ctx->m_inbuf);
	ctx->m_inbuf = NULL;
	ctx->m_inbuflen = 0;
	ctx->m_inbufoff = 0;
}

static TransportCtx_t* reset_decode_result(TransportCtx_t* ctx) {
	ctx->decode_result.err = 0;
	ctx->decode_result.incomplete = 0;
	ctx->decode_result.decodelen = 0;
	ctx->decode_result.bodylen = 0;
	ctx->decode_result.bodyptr = NULL;
	return ctx;
}

static void clear_packetlist(List_t* packetlist) {
	ListNode_t* cur, *next;
	for (cur = packetlist->head; cur; cur = next) {
		next = cur->next;
		free(cur);
	}
	listInit(packetlist);
}

static void session_replace_transport(Session_t* s, SessionTransportStatus_t* target_status) {
	if (SOCK_STREAM == s->socktype) {
		Packet_t* packet;
		ListNode_t* cur, *next;
		for (cur = s->ctx.m_sendpacketlist.head; cur; cur = next) {
			packet = pod_container_of(cur, Packet_t, msg);
			next = cur->next;
			if (packet->offset >= packet->len) {
				listRemoveNode(&s->ctx.m_sendpacketlist, cur);
				free(packet);
			}
		}
		if (s->ctx.m_sendpacketlist.head) {
			for (cur = s->ctx.m_sendpacketlist.head->next; cur; cur = next) {
				next = cur->next;
				listRemoveNode(&s->ctx.m_sendpacketlist, cur);
				free(pod_container_of(cur, Packet_t, msg));
			}
		}
	}
	else {
		clear_packetlist(&s->ctx.m_sendpacketlist);
	}
	clear_packetlist(&s->ctx.m_recvpacketlist);
	s->ctx.m_recvseq = target_status->m_recvseq;
	s->ctx.m_sendseq = target_status->m_sendseq;
	s->ctx.m_cwndseq = target_status->m_cwndseq;
	s->ctx.m_recvpacketlist = target_status->m_recvpacketlist;
	listMerge(&s->ctx.m_sendpacketlist, &target_status->m_sendpacketlist);
	listInit(&target_status->m_recvpacketlist);
}

static void session_grab_transport(TransportCtx_t* ctx, SessionTransportStatus_t* target_status) {
	target_status->m_cwndseq = ctx->m_cwndseq;
	target_status->m_recvseq = ctx->m_recvseq;
	target_status->m_sendseq = ctx->m_sendseq;
	target_status->m_recvpacketlist = ctx->m_recvpacketlist;
	target_status->m_sendpacketlist = ctx->m_sendpacketlist;
	ctx->m_cwndseq = 0;
	ctx->m_recvseq = 0;
	ctx->m_sendseq = 0;
	listInit(&ctx->m_recvpacketlist);
	listInit(&ctx->m_sendpacketlist);
}

static int session_grab_transport_check(TransportCtx_t* ctx, unsigned int recvseq, unsigned int cwndseq) {
	if (seq1_before_seq2(recvseq, ctx->m_cwndseq))
		return 0;
	if (seq1_before_seq2(ctx->m_recvseq, cwndseq))
		return 0;
	return 1;
}

static void stream_send_packet(Session_t* s, Packet_t* packet) {
	int res;
	if (s->ctx.m_sendpacketlist.tail) {
		Packet_t* last_packet = pod_container_of(s->ctx.m_sendpacketlist.tail, Packet_t, msg);
		if (last_packet->offset < last_packet->len) {
			packet->offset = 0;
			listInsertNodeBack(&s->ctx.m_sendpacketlist, s->ctx.m_sendpacketlist.tail, &packet->msg._);
			return;
		}
	}
	res = socketWrite(s->fd, packet->data, packet->len, 0, NULL, 0);
	if (res < 0) {
		if (errnoGet() != EWOULDBLOCK) {
			s->m_valid = 0;
			free(packet);
			return;
		}
		res = 0;
	}
	else if (res >= packet->len && !packet->need_ack) {
		free(packet);
		return;
	}
	listInsertNodeBack(&s->ctx.m_sendpacketlist, s->ctx.m_sendpacketlist.tail, &packet->msg._);
	packet->offset = res;
	if (res < packet->len)
		reactorsocket_write(s);
}

static void stream_send_packet_continue(Session_t* s) {
	ListNode_t* cur, *next;
	if (s->m_writeol_has_commit) {
		return;
	}
	for (cur = s->ctx.m_sendpacketlist.head; cur; cur = next) {
		int res;
		Packet_t* packet = pod_container_of(cur, Packet_t, msg);
		next = cur->next;
		if (packet->offset >= packet->len) {
			continue;
		}
		res = socketWrite(s->fd, packet->data + packet->offset, packet->len - packet->offset, 0, NULL, 0);
		if (res < 0) {
			if (errnoGet() != EWOULDBLOCK) {
				s->m_valid = 0;
				break;
			}
			res = 0;
		}
		packet->offset += res;
		if (packet->offset < packet->len) {
			reactorsocket_write(s);
			break;
		}
		else if (!packet->need_ack) {
			listRemoveNode(&s->ctx.m_sendpacketlist, cur);
			free(packet);
		}
	}
}

static void reliable_stream_bak(TransportCtx_t* ctx) {
	ListNode_t* cur, *next;
	ctx->m_sendpacketlistbak = ctx->m_sendpacketlist;
	listInit(&ctx->m_sendpacketlist);
	for (cur = ctx->m_sendpacketlistbak.head; cur; cur = next) {
		Packet_t* packet = pod_container_of(cur, Packet_t, msg);
		unsigned char hdrtype = packet->type;
		next = cur->next;
		if (HDR_DATA != hdrtype) {
			listRemoveNode(&ctx->m_sendpacketlistbak, cur);
			free(packet);
		}
		else {
			packet->offset = 0;
		}
	}
	ctx->m_cwndseqbak = ctx->m_cwndseq;
	ctx->m_recvseqbak = ctx->m_recvseq;
	ctx->m_sendseqbak = ctx->m_sendseq;
}

static void reliable_stream_bak_recovery(TransportCtx_t* ctx) {
	ctx->m_recvseq = ctx->m_recvseqbak;
	ctx->m_sendseq = ctx->m_sendseqbak;
	ctx->m_cwndseq = ctx->m_cwndseqbak;
	ctx->m_sendpacketlist = ctx->m_sendpacketlistbak;
	listInit(&ctx->m_sendpacketlistbak);
}

static int reliable_stream_reply_ack(Session_t* s, unsigned int seq) {
	ListNode_t* cur;
	Packet_t* packet = NULL;
	size_t hdrlen = s->ctx.hdrlen ? s->ctx.hdrlen(RELIABLE_STREAM_DATA_HDR_LEN) : 0;
	unsigned int sizeof_ack = hdrlen + RELIABLE_STREAM_DATA_HDR_LEN;
	for (cur = s->ctx.m_sendpacketlist.head; cur; cur = cur->next) {
		packet = pod_container_of(cur, Packet_t, msg);
		if (packet->offset < packet->len)
			break;
	}
	if (!packet || s->ctx.m_sendpacketlist.tail == &packet->msg._) {
		int res;
		unsigned char* ack = (unsigned char*)alloca(sizeof_ack);
		if (hdrlen && s->ctx.encode) {
			s->ctx.encode(ack, RELIABLE_STREAM_DATA_HDR_LEN);
		}
		ack[hdrlen] = HDR_ACK;
		*(unsigned int*)(ack + hdrlen + 1) = htonl(seq);
		res = socketWrite(s->fd, ack, sizeof_ack, 0, NULL, 0);
		if (res < 0) {
			if (errnoGet() != EWOULDBLOCK) {
				s->m_valid = 0;
				return 0;
			}
			res = 0;
		}
		if (res < sizeof_ack) {
			packet = (Packet_t*)malloc(sizeof(Packet_t) + sizeof_ack);
			if (!packet) {
				s->m_valid = 0;
				return 0;
			}
			packet->msg.type = SESSION_PACKET_MESSAGE;
			packet->s = s;
			packet->type = HDR_ACK;
			packet->need_ack = 0;
			packet->seq = seq;
			packet->hdrlen = hdrlen;
			packet->offset = res;
			packet->len = sizeof_ack;
			memcpy(packet->data, ack, sizeof_ack);
			listInsertNodeBack(&s->ctx.m_sendpacketlist, s->ctx.m_sendpacketlist.tail, &packet->msg._);
			reactorsocket_write(s);
		}
	}
	else {
		Packet_t* ack_packet = (Packet_t*)malloc(sizeof(Packet_t) + sizeof_ack);
		if (!ack_packet) {
			s->m_valid = 0;
			return 0;
		}
		ack_packet->msg.type = SESSION_PACKET_MESSAGE;
		ack_packet->s = s;
		ack_packet->type = HDR_ACK;
		ack_packet->need_ack = 0;
		ack_packet->hdrlen = hdrlen;
		ack_packet->seq = seq;
		ack_packet->offset = 0;
		ack_packet->len = sizeof_ack;
		if (hdrlen && s->ctx.encode) {
			s->ctx.encode(ack_packet->data, RELIABLE_STREAM_DATA_HDR_LEN);
		}
		ack_packet->data[hdrlen] = HDR_ACK;
		*(unsigned int*)(ack_packet->data + hdrlen + 1) = htonl(seq);
		listInsertNodeBack(&s->ctx.m_sendpacketlist, &packet->msg._, &ack_packet->msg._);
	}
	return 1;
}

static void reliable_stream_do_ack(Session_t* s, unsigned int seq) {
	Packet_t* packet = NULL;
	ListNode_t* cur, *next;
	List_t freepacketlist;
	listInit(&freepacketlist);
	for (cur = s->ctx.m_sendpacketlist.head; cur; cur = next) {
		unsigned char pkg_hdr_type;
		unsigned int pkg_seq;
		next = cur->next;
		packet = pod_container_of(cur, Packet_t, msg);
		pkg_hdr_type = packet->type;
		if (HDR_DATA != pkg_hdr_type)
			continue;
		if (packet->offset < packet->len)
			break;
		pkg_seq = packet->seq;
		if (seq1_before_seq2(seq, pkg_seq))
			break;
		if (next) {
			packet = pod_container_of(next, Packet_t, msg);
			s->ctx.m_cwndseq = packet->seq;
		}
		else
			s->ctx.m_cwndseq++;
		listRemoveNode(&s->ctx.m_sendpacketlist, cur);
		listInsertNodeBack(&freepacketlist, freepacketlist.tail, cur);
		if (pkg_seq == seq)
			break;
	}
	if (freepacketlist.head) {
		for (cur = freepacketlist.head; cur; cur = next) {
			next = cur->next;
			free(pod_container_of(cur, Packet_t, msg));
		}
		stream_send_packet_continue(s);
	}
}

static int reliable_stream_data_packet_handler(Session_t* s, unsigned char* data, int len, const struct sockaddr_storage* saddr) {
	int offset = 0;
	while (offset < len) {
		s->ctx.decode(reset_decode_result(&s->ctx), data + offset, len - offset);
		if (s->ctx.decode_result.err)
			return -1;
		else if (s->ctx.decode_result.incomplete)
			return offset;
		offset += s->ctx.decode_result.decodelen;
		if (s->ctx.decode_result.bodylen < RELIABLE_STREAM_DATA_HDR_LEN) {
			continue;
		}
		else {
			int packet_is_valid = 1;
			unsigned char hdr_type = *s->ctx.decode_result.bodyptr & (~HDR_DATA_END_FLAG);
			if (HDR_RECONNECT == hdr_type) {
				if (ESTABLISHED_STATUS == s->ctx.m_status) {
					ListNode_t* cur, *next;
					s->ctx.m_status = RECONNECT_STATUS;
					s->ctx.m_heartbeat_msec = 0;
					for (cur = s->ctx.m_sendpacketlist.head; cur; cur = next) {
						Packet_t* packet = pod_container_of(cur, Packet_t, msg);
						next = cur->next;
						if (0 == packet->offset || packet->offset >= packet->len) {
							listRemoveNode(&s->ctx.m_sendpacketlist, cur);
							free(packet);
						}
						else
							packet->need_ack = 0;
					}
				}
			}
			else if (RECONNECT_STATUS == s->ctx.m_status) {
				continue;
			}
			else {
				unsigned int seq = *(unsigned int*)(s->ctx.decode_result.bodyptr + 1);
				seq = ntohl(seq);
				if (HDR_ACK == hdr_type) {
					packet_is_valid = 0;
					if (seq != s->ctx.m_cwndseq) {
						s->m_valid = 0;
						return -1;
					}
					reliable_stream_do_ack(s, seq);
				}
				else if (HDR_DATA == hdr_type) {
					if (seq1_before_seq2(seq, s->ctx.m_recvseq))
						packet_is_valid = 0;
					else if (seq == s->ctx.m_recvseq)
						++s->ctx.m_recvseq;
					else {
						s->m_valid = 0;
						return -1;
					}
					reliable_stream_reply_ack(s, seq);
				}
				else
					packet_is_valid = 0;
			}
			if (packet_is_valid) {
				s->ctx.decode_result.bodylen -= RELIABLE_STREAM_DATA_HDR_LEN;
				if (s->ctx.decode_result.bodylen > 0)
					s->ctx.decode_result.bodyptr += RELIABLE_STREAM_DATA_HDR_LEN;
				else
					s->ctx.decode_result.bodyptr = NULL;
				s->ctx.recv(&s->ctx, saddr);
			}
		}
	}
	return offset;
}

static int data_packet_handler(Session_t* s, unsigned char* data, int len, const struct sockaddr_storage* saddr) {
	if (len) {
		int offset = 0;
		while (offset < len) {
			s->ctx.decode(reset_decode_result(&s->ctx), data + offset, len - offset);
			if (s->ctx.decode_result.err)
				return -1;
			else if (s->ctx.decode_result.incomplete)
				return offset;
			offset += s->ctx.decode_result.decodelen;
			s->ctx.recv(&s->ctx, saddr);
		}
		return offset;
	}
	else if (SOCK_STREAM != s->socktype) {
		s->ctx.recv(reset_decode_result(&s->ctx), saddr);
	}
	return 0;
}

static void reliable_dgram_send_again(Session_t* s, long long timestamp_msec) {
	ListNode_t* cur;
	for (cur = s->ctx.m_sendpacketlist.head; cur; cur = cur->next) {
		Packet_t* packet = pod_container_of(cur, Packet_t, msg);
		if (packet->seq < s->ctx.m_cwndseq ||
			packet->seq - s->ctx.m_cwndseq >= s->ctx.cwndsize)
		{
			break;
		}
		socketWrite(s->fd, packet->data, packet->len, 0, &s->ctx.peer_saddr, sockaddrLength(&s->ctx.peer_saddr));
		packet->resendtimes = 0;
		packet->resend_timestamp_msec = timestamp_msec + s->ctx.rto;
		update_timestamp(&s->m_loop->m_event_msec, packet->resend_timestamp_msec);
	}
}

static int reliable_dgram_inner_packet_send(Session_t* s, const unsigned char* data, int len, const struct sockaddr_storage* saddr) {
	size_t hdrlen = s->ctx.hdrlen ? s->ctx.hdrlen(len) : 0;
	if (hdrlen && s->ctx.encode) {
		Iobuf_t iov[2] = {
			iobufStaticInit(alloca(hdrlen), hdrlen),
			iobufStaticInit(data, len)
		};
		s->ctx.encode((unsigned char*)iobufPtr(&iov[0]), len);
		return socketWritev(s->fd, iov, sizeof(iov) / sizeof(iov[0]), 0, saddr, sockaddrLength(saddr));
	}
	else {
		return socketWrite(s->fd, data, len, 0, saddr, sockaddrLength(saddr));
	}
}

static void reliable_dgram_packet_merge(Session_t* s, unsigned char* data, int len, const struct sockaddr_storage* saddr) {
	unsigned char hdr_data_end_flag = data[0] & HDR_DATA_END_FLAG;
	len -= RELIABLE_DGRAM_DATA_HDR_LEN;
	data += RELIABLE_DGRAM_DATA_HDR_LEN;
	if (!s->ctx.m_inbuf && hdr_data_end_flag) {
		reset_decode_result(&s->ctx);
		s->ctx.decode_result.bodyptr = data;
		s->ctx.decode_result.bodylen = len;
		s->ctx.recv(&s->ctx, saddr);
	}
	else {
		unsigned char* ptr = (unsigned char*)realloc(s->ctx.m_inbuf, s->ctx.m_inbuflen + len);
		if (ptr) {
			s->ctx.m_inbuf = ptr;
			memcpy(s->ctx.m_inbuf + s->ctx.m_inbuflen, data, len);
			s->ctx.m_inbuflen += len;
			if (!hdr_data_end_flag)
				return;
			else {
				reset_decode_result(&s->ctx);
				s->ctx.decode_result.bodyptr = s->ctx.m_inbuf;
				s->ctx.decode_result.bodylen = s->ctx.m_inbuflen;
				s->ctx.recv(&s->ctx, saddr);
			}
		}
		free_inbuf(&s->ctx);
	}
}

static void reliable_dgram_check_send_fin_packet(Session_t* s, long long timestamp_msec) {
	if (ESTABLISHED_FIN_STATUS != s->ctx.m_status && CLOSE_WAIT_STATUS != s->ctx.m_status)
		return;
	else if (s->ctx.m_sendpacketlist.head)
		return;
	else {
		unsigned char fin = HDR_FIN;
		reliable_dgram_inner_packet_send(s, &fin, sizeof(fin), &s->ctx.peer_saddr);
		s->ctx.m_fin_msec = timestamp_msec + s->ctx.rto;
		update_timestamp(&s->m_loop->m_event_msec, s->ctx.m_fin_msec);
		if (ESTABLISHED_FIN_STATUS == s->ctx.m_status)
			s->ctx.m_status = FIN_WAIT_1_STATUS;
		else if (CLOSE_WAIT_STATUS == s->ctx.m_status)
			s->ctx.m_status = LAST_ACK_STATUS;
	}
}

static void reliable_dgram_shutdown(Session_t* s, long long timestamp_msec) {
	if (SESSION_TRANSPORT_LISTEN == s->transport_side) {
		s->ctx.m_status = CLOSED_STATUS;
		s->ctx.m_lastactive_msec = timestamp_msec;
		s->m_valid = 0;
		update_timestamp(&s->m_loop->m_event_msec, s->ctx.m_lastactive_msec + s->close_timeout_msec);
	}
	else if (SESSION_TRANSPORT_CLIENT == s->transport_side || SESSION_TRANSPORT_SERVER == s->transport_side) {
		if (ESTABLISHED_STATUS != s->ctx.m_status)
			return;
		s->ctx.m_status = ESTABLISHED_FIN_STATUS;
		reliable_dgram_check_send_fin_packet(s, timestamp_msec);
		s->ctx.m_heartbeat_msec = 0;
	}
}

static void reliable_dgram_send_packet(Session_t* s, Packet_t* packet, long long timestamp_msec) {
	listInsertNodeBack(&s->ctx.m_sendpacketlist, s->ctx.m_sendpacketlist.tail, &packet->msg._);
	if (packet->seq >= s->ctx.m_cwndseq &&
		packet->seq - s->ctx.m_cwndseq < s->ctx.cwndsize)
	{
		socketWrite(s->fd, packet->data, packet->len, 0, &s->ctx.peer_saddr, sockaddrLength(&s->ctx.peer_saddr));
		packet->resend_timestamp_msec = timestamp_msec + s->ctx.rto;
		update_timestamp(&s->m_loop->m_event_msec, packet->resend_timestamp_msec);
	}
}

static void reliable_dgram_do_reconnect(Session_t* s) {
	unsigned char reconnect_pkg[9];
	reconnect_pkg[0] = HDR_RECONNECT;
	*(unsigned int*)(reconnect_pkg + 1) = htonl(s->ctx.m_recvseq);
	*(unsigned int*)(reconnect_pkg + 5) = htonl(s->ctx.m_cwndseq);
	reliable_dgram_inner_packet_send(s, reconnect_pkg, sizeof(reconnect_pkg), &s->ctx.peer_saddr);
}

static int reliable_dgram_recv_handler(Session_t* s, unsigned char* buffer, int len, const struct sockaddr_storage* saddr, long long timestamp_msec) {
	unsigned char hdr_type = buffer[0] & (~HDR_DATA_END_FLAG);
	if (HDR_SYN == hdr_type) {
		unsigned char syn_ack[3];
		if (s->m_shutdownflag)
			return 1;
		else if (LISTENED_STATUS == s->ctx.m_status) {
			ReliableDgramHalfConnectPacket_t* halfcon = NULL;
			ListNode_t* cur, *next;
			for (cur = s->ctx.m_recvpacketlist.head; cur; cur = next) {
				next = cur->next;
				halfcon = pod_container_of(cur, ReliableDgramHalfConnectPacket_t, m_listnode);
				if (!memcmp(&halfcon->peer_addr, saddr, sizeof(halfcon->peer_addr)))
					break;
				halfcon = NULL;
			}
			if (halfcon) {
				syn_ack[0] = HDR_SYN_ACK;
				*(unsigned short*)(syn_ack + 1) = htons(halfcon->local_port);
			}
			else {
				FD_t new_fd;
				IPString_t ipstr;
				unsigned short local_port;
				struct sockaddr_storage local_saddr = s->local_listen_saddr;
				if (!sockaddrSetPort(&local_saddr, 0))
					return 1;
				new_fd = socket(s->domain, s->socktype, s->protocol);
				if (new_fd == INVALID_FD_HANDLE)
					return 1;
				if (!socketBindAddr(new_fd, (struct sockaddr*)&local_saddr, sockaddrLength((struct sockaddr*)&local_saddr))) {
					socketClose(new_fd);
					return 1;
				}
				if (!socketGetLocalAddr(new_fd, &local_saddr)) {
					socketClose(new_fd);
					return 1;
				}
				if (!sockaddrDecode(&local_saddr, ipstr, &local_port)) {
					socketClose(new_fd);
					return 1;
				}
				if (!socketNonBlock(new_fd, TRUE)) {
					socketClose(new_fd);
					return 1;
				}
				halfcon = (ReliableDgramHalfConnectPacket_t*)malloc(sizeof(ReliableDgramHalfConnectPacket_t));
				if (!halfcon) {
					socketClose(new_fd);
					return 1;
				}
				halfcon->local_port = local_port;
				halfcon->resend_times = 0;
				halfcon->peer_addr = *saddr;
				halfcon->sockfd = new_fd;
				halfcon->timestamp_msec = timestamp_msec + s->ctx.rto;

				listInsertNodeBack(&s->ctx.m_recvpacketlist, s->ctx.m_recvpacketlist.tail, &halfcon->m_listnode);
				update_timestamp(&s->m_loop->m_event_msec, halfcon->timestamp_msec);

				syn_ack[0] = HDR_SYN_ACK;
				*(unsigned short*)(syn_ack + 1) = htons(local_port);
			}
			reliable_dgram_inner_packet_send(s, syn_ack, sizeof(syn_ack), saddr);
			s->ctx.m_lastactive_msec = timestamp_msec;
		}
		else if (SYN_RCVD_STATUS == s->ctx.m_status) {
			if (AF_UNSPEC == s->ctx.peer_saddr.ss_family) {
				s->ctx.peer_saddr = *saddr;
				s->ctx.m_synrcvd_msec = timestamp_msec + s->ctx.rto;
				update_timestamp(&s->m_loop->m_event_msec, s->ctx.m_synrcvd_msec);
			}
			else if (memcmp(&s->ctx.peer_saddr, saddr, sizeof(*saddr)))
				return 1;
			syn_ack[0] = HDR_SYN_ACK;
			reliable_dgram_inner_packet_send(s, syn_ack, 1, saddr);
			s->ctx.m_lastactive_msec = timestamp_msec;
		}
	}
	else if (HDR_SYN_ACK_ACK == hdr_type) {
		ListNode_t* cur, *next;
		if (s->m_shutdownflag)
			return 1;
		else if (LISTENED_STATUS == s->ctx.m_status) {
			struct sockaddr_storage peer_addr;
			for (cur = s->ctx.m_recvpacketlist.head; cur; cur = next) {
				ReliableDgramHalfConnectPacket_t* halfcon = pod_container_of(cur, ReliableDgramHalfConnectPacket_t, m_listnode);
				next = cur->next;
				if (memcmp(&halfcon->peer_addr, saddr, sizeof(halfcon->peer_addr)))
					continue;
				if (socketRead(halfcon->sockfd, NULL, 0, 0, &peer_addr))
					break;
				listRemoveNode(&s->ctx.m_recvpacketlist, cur);
				s->accept(s, halfcon->sockfd, &peer_addr);
				free(halfcon);
				break;
			}
			s->ctx.m_lastactive_msec = timestamp_msec;
		}
		else if (SYN_RCVD_STATUS == s->ctx.m_status) {
			if (memcmp(&s->ctx.peer_saddr, saddr, sizeof(*saddr)))
				return 1;
			s->ctx.m_lastactive_msec = timestamp_msec;
			s->ctx.m_heartbeat_msec = timestamp_msec;
			if (s->ctx.heartbeat_timeout_sec > 0) {
				update_timestamp(&s->m_loop->m_event_msec, s->ctx.m_heartbeat_msec + s->ctx.heartbeat_timeout_sec * 1000);
			}
			s->ctx.m_status = ESTABLISHED_STATUS;
		}
	}
	else if (HDR_SYN_ACK == hdr_type) {
		unsigned char syn_ack_ack;
		if (len < 3)
			return 1;
		if (memcmp(saddr, &s->peer_listen_saddr, sizeof(s->peer_listen_saddr)))
			return 1;
		if (SYN_SENT_STATUS != s->ctx.m_status &&
			ESTABLISHED_STATUS != s->ctx.m_status)
		{
			return 1;
		}
		syn_ack_ack = HDR_SYN_ACK_ACK;
		reliable_dgram_inner_packet_send(s, &syn_ack_ack, sizeof(syn_ack_ack), saddr);
		if (SYN_SENT_STATUS == s->ctx.m_status) {
			if (len >= 3) {
				unsigned short peer_port;
				peer_port = *(unsigned short*)(buffer + 1);
				peer_port = ntohs(peer_port);
				sockaddrSetPort(&s->ctx.peer_saddr, peer_port);
			}
			s->ctx.m_status = ESTABLISHED_STATUS;
			s->connect(s, 0, s->m_connect_times++, 0, 0);
			s->ctx.m_heartbeat_msec = timestamp_msec;
			if (s->ctx.heartbeat_timeout_sec > 0) {
				update_timestamp(&s->m_loop->m_event_msec, s->ctx.m_heartbeat_msec + s->ctx.heartbeat_timeout_sec * 1000);
			}
		}
		socketWrite(s->fd, NULL, 0, 0, &s->ctx.peer_saddr, sockaddrLength(&s->ctx.peer_saddr));
		s->ctx.m_lastactive_msec = timestamp_msec;
	}
	else if (HDR_RECONNECT == hdr_type) {
		unsigned int peer_recvseq, peer_cwndseq;
		if (len < 9)
			return 1;
		if (SESSION_TRANSPORT_SERVER != s->transport_side || ESTABLISHED_STATUS != s->ctx.m_status)
			return 1;
		peer_recvseq = ntohl(*(unsigned int*)(buffer + 1));
		peer_cwndseq = ntohl(*(unsigned int*)(buffer + 5));
		if (session_grab_transport_check(&s->ctx, peer_recvseq, peer_cwndseq)) {
			unsigned char reconnect_ack = HDR_RECONNECT_ACK;
			reliable_dgram_inner_packet_send(s, &reconnect_ack, sizeof(reconnect_ack), saddr);
			s->ctx.peer_saddr = *saddr;
			s->ctx.m_lastactive_msec = timestamp_msec;
			reliable_dgram_send_again(s, timestamp_msec);
		}
		else {
			unsigned char reconnect_err = HDR_RECONNECT_ERR;
			reliable_dgram_inner_packet_send(s, &reconnect_err, sizeof(reconnect_err), saddr);
		}
	}
	else if (HDR_RECONNECT_ACK == hdr_type) {
		if (SESSION_TRANSPORT_CLIENT != s->transport_side || RECONNECT_STATUS != s->ctx.m_status)
			return 1;
		if (memcmp(&s->ctx.peer_saddr, saddr, sizeof(*saddr)))
			return 1;
		s->ctx.m_status = ESTABLISHED_STATUS;
		_xchg16(&s->m_shutdownflag, 0);
		s->ctx.m_lastactive_msec = timestamp_msec;
		reliable_dgram_send_again(s, timestamp_msec);
		s->connect(s, 0, s->m_connect_times++, s->ctx.m_recvseq, s->ctx.m_cwndseq);
	}
	else if (HDR_RECONNECT_ERR == hdr_type) {
		if (SESSION_TRANSPORT_CLIENT != s->transport_side || RECONNECT_STATUS != s->ctx.m_status)
			return 1;
		if (memcmp(&s->ctx.peer_saddr, saddr, sizeof(*saddr)))
			return 1;
		s->m_valid = 0;
		s->ctx.m_status = TIME_WAIT_STATUS;
		s->connect(s, ECONNREFUSED, s->m_connect_times++, s->ctx.m_recvseq, s->ctx.m_cwndseq);
	}
	else if (HDR_FIN == hdr_type) {
		unsigned char fin_ack;
		if (memcmp(saddr, &s->ctx.peer_saddr, sizeof(*saddr)))
			return 1;
		else if (ESTABLISHED_STATUS == s->ctx.m_status) {
			fin_ack = HDR_FIN_ACK;
			reliable_dgram_inner_packet_send(s, &fin_ack, sizeof(fin_ack), &s->ctx.peer_saddr);
			s->ctx.m_status = CLOSE_WAIT_STATUS;
			s->ctx.m_lastactive_msec = timestamp_msec;
			s->ctx.m_heartbeat_msec = 0;
			reliable_dgram_check_send_fin_packet(s, timestamp_msec);
			_xchg16(&s->m_shutdownflag, 1);
			if (s->shutdown) {
				s->shutdown(s);
				s->shutdown = NULL;
			}
		}
		else if (FIN_WAIT_1_STATUS == s->ctx.m_status ||
				FIN_WAIT_2_STATUS == s->ctx.m_status)
		{
			fin_ack = HDR_FIN_ACK;
			reliable_dgram_inner_packet_send(s, &fin_ack, sizeof(fin_ack), &s->ctx.peer_saddr);
			s->ctx.m_status = TIME_WAIT_STATUS;
			s->ctx.m_lastactive_msec = timestamp_msec;
			s->m_valid = 0;
			_xchg16(&s->m_shutdownflag, 1);
			if (s->shutdown) {
				s->shutdown(s);
				s->shutdown = NULL;
			}
		}
	}
	else if (HDR_FIN_ACK == hdr_type) {
		if (memcmp(saddr, &s->ctx.peer_saddr, sizeof(*saddr)))
			return 1;
		else if (LAST_ACK_STATUS == s->ctx.m_status) {
			s->ctx.m_status = CLOSED_STATUS;
			s->ctx.m_lastactive_msec = timestamp_msec;
			s->m_valid = 0;
		}
		else if (FIN_WAIT_1_STATUS == s->ctx.m_status) {
			s->ctx.m_status = FIN_WAIT_2_STATUS;
			s->ctx.m_lastactive_msec = timestamp_msec;
		}
	}
	else if (HDR_ACK == hdr_type) {
		ListNode_t* cur;
		unsigned int seq, cwnd_skip, ack_valid;
		if (len < RELIABLE_DGRAM_DATA_HDR_LEN)
			return 1;
		if (ESTABLISHED_STATUS > s->ctx.m_status)
			return 1;
		if (memcmp(saddr, &s->ctx.peer_saddr, sizeof(*saddr)))
			return 1;

		s->ctx.m_lastactive_msec = timestamp_msec;
		s->ctx.m_heartbeat_msec = timestamp_msec;
		seq = *(unsigned int*)(buffer + 1);
		seq = ntohl(seq);
		cwnd_skip = 0;
		ack_valid = 0;

		for (cur = s->ctx.m_sendpacketlist.head; cur; cur = cur->next) {
			Packet_t* packet = pod_container_of(cur, Packet_t, msg);
			if (seq1_before_seq2(seq, packet->seq))
				break;
			if (seq == packet->seq) {
				ListNode_t* next = cur->next;
				listRemoveNode(&s->ctx.m_sendpacketlist, cur);
				free(packet);
				if (seq == s->ctx.m_cwndseq) {
					if (next) {
						packet = pod_container_of(next, Packet_t, msg);
						s->ctx.m_cwndseq = packet->seq;
						cwnd_skip = 1;
					}
					else
						++s->ctx.m_cwndseq;
				}
				ack_valid = 1;
				break;
			}
		}
		if (cwnd_skip) {
			for (cur = s->ctx.m_sendpacketlist.head; cur; cur = cur->next) {
				Packet_t* packet = pod_container_of(cur, Packet_t, msg);
				if (packet->seq < s->ctx.m_cwndseq ||
					packet->seq - s->ctx.m_cwndseq >= s->ctx.cwndsize)
				{
					break;
				}
				socketWrite(s->fd, packet->data, packet->len, 0, &s->ctx.peer_saddr, sockaddrLength(&s->ctx.peer_saddr));
				packet->resend_timestamp_msec = timestamp_msec + s->ctx.rto;
				update_timestamp(&s->m_loop->m_event_msec, packet->resend_timestamp_msec);
			}
		}
		if (ack_valid) {
			reliable_dgram_check_send_fin_packet(s, timestamp_msec);
		}
	}
	else if (HDR_DATA == hdr_type) {
		ListNode_t* cur, *next;
		Packet_t* packet;
		unsigned int seq;
		unsigned char ack[RELIABLE_DGRAM_DATA_HDR_LEN];
		if (len < RELIABLE_DGRAM_DATA_HDR_LEN)
			return 1;
		if (ESTABLISHED_STATUS > s->ctx.m_status)
			return 1;
		if (memcmp(saddr, &s->ctx.peer_saddr, sizeof(*saddr)))
			return 1;

		s->ctx.m_lastactive_msec = timestamp_msec;
		seq = *(unsigned int*)(buffer + 1);
		ack[0] = HDR_ACK;
		*(unsigned int*)(ack + 1) = seq;
		reliable_dgram_inner_packet_send(s, ack, sizeof(ack), saddr);

		seq = ntohl(seq);
		if (seq1_before_seq2(seq, s->ctx.m_recvseq))
			return 1;
		else if (seq == s->ctx.m_recvseq) {
			s->ctx.m_recvseq++;
			reliable_dgram_packet_merge(s, buffer, len, saddr);
			for (cur = s->ctx.m_recvpacketlist.head; cur; cur = next) {
				packet = pod_container_of(cur, Packet_t, msg);
				if (packet->seq != s->ctx.m_recvseq)
					break;
				next = cur->next;
				s->ctx.m_recvseq++;
				reliable_dgram_packet_merge(s, packet->data, packet->len, saddr);
				listRemoveNode(&s->ctx.m_recvpacketlist, cur);
				free(packet);
			}
		}
		else {
			for (cur = s->ctx.m_recvpacketlist.head; cur; cur = cur->next) {
				packet = pod_container_of(cur, Packet_t, msg);
				if (seq1_before_seq2(seq, packet->seq))
					break;
				else if (packet->seq == seq)
					return 1;
			}
			packet = (Packet_t*)malloc(sizeof(Packet_t) + len);
			if (!packet) {
				//s->valid = 0;
				return 0;
			}
			packet->msg.type = SESSION_PACKET_MESSAGE;
			packet->s = s;
			packet->seq = seq;
			packet->hdrlen = 0;
			packet->len = len;
			memcpy(packet->data, buffer, len);
			if (cur)
				listInsertNodeFront(&s->ctx.m_recvpacketlist, cur, &packet->msg._);
			else
				listInsertNodeBack(&s->ctx.m_recvpacketlist, s->ctx.m_recvpacketlist.tail, &packet->msg._);
		}
	}
	return 1;
}

static void reliable_dgram_update(SessionLoop_t* loop, Session_t* s, long long timestamp_msec) {
	if (LISTENED_STATUS == s->ctx.m_status) {
		ListNode_t* cur, *next;
		for (cur = s->ctx.m_recvpacketlist.head; cur; cur = next) {
			ReliableDgramHalfConnectPacket_t* halfcon = pod_container_of(cur, ReliableDgramHalfConnectPacket_t, m_listnode);
			next = cur->next;
			if (halfcon->timestamp_msec > timestamp_msec) {
				update_timestamp(&loop->m_event_msec, halfcon->timestamp_msec);
			}
			else if (halfcon->resend_times >= s->ctx.resend_maxtimes) {
				socketClose(halfcon->sockfd);
				listRemoveNode(&s->ctx.m_recvpacketlist, cur);
				free(halfcon);
			}
			else {
				unsigned char syn_ack[3];
				syn_ack[0] = HDR_SYN_ACK;
				*(unsigned short*)(syn_ack + 1) = htons(halfcon->local_port);
				reliable_dgram_inner_packet_send(s, syn_ack, sizeof(syn_ack), &halfcon->peer_addr);
				++halfcon->resend_times;
				halfcon->timestamp_msec = timestamp_msec + s->ctx.rto;
				update_timestamp(&loop->m_event_msec, halfcon->timestamp_msec);
			}
		}
	}
	else if (SYN_RCVD_STATUS == s->ctx.m_status) {
		if (AF_UNSPEC == s->ctx.peer_saddr.ss_family || s->m_shutdownflag) {
			return;
		}
		else if (s->ctx.m_synrcvd_msec > timestamp_msec) {
			update_timestamp(&loop->m_event_msec, s->ctx.m_synrcvd_msec);
		}
		else if (s->ctx.m_synrcvd_times >= s->ctx.resend_maxtimes) {
			s->ctx.m_synrcvd_times = 0;
			s->ctx.peer_saddr.ss_family = AF_UNSPEC;
		}
		else {
			unsigned char syn_ack = HDR_SYN_ACK;
			reliable_dgram_inner_packet_send(s, &syn_ack, 1, &s->ctx.peer_saddr);
			++s->ctx.m_synrcvd_times;
			s->ctx.m_synrcvd_msec = timestamp_msec + s->ctx.rto;
			update_timestamp(&loop->m_event_msec, s->ctx.m_synrcvd_msec);
		}
	}
	else if (SYN_SENT_STATUS == s->ctx.m_status) {
		if (s->ctx.m_synsent_msec > timestamp_msec) {
			update_timestamp(&loop->m_event_msec, s->ctx.m_synsent_msec);
		}
		else if (s->ctx.m_synsent_times >= s->ctx.resend_maxtimes) {
			s->ctx.m_status = TIME_WAIT_STATUS;
			s->ctx.m_lastactive_msec = timestamp_msec;
			s->m_valid = 0;
			s->shutdown = NULL;
			s->connect(s, ETIMEDOUT, s->m_connect_times++, 0, 0);
		}
		else {
			unsigned char syn = HDR_SYN;
			reliable_dgram_inner_packet_send(s, &syn, 1, &s->peer_listen_saddr);
			++s->ctx.m_synsent_times;
			s->ctx.m_synsent_msec = timestamp_msec + s->ctx.rto;
			update_timestamp(&loop->m_event_msec, s->ctx.m_synsent_msec);
		}
	}
	else if (FIN_WAIT_1_STATUS == s->ctx.m_status || LAST_ACK_STATUS == s->ctx.m_status) {
		if (s->ctx.m_fin_msec > timestamp_msec) {
			update_timestamp(&loop->m_event_msec, s->ctx.m_fin_msec);
		}
		else if (s->ctx.m_fin_times >= s->ctx.resend_maxtimes) {
			s->ctx.m_lastactive_msec = timestamp_msec;
			s->m_valid = 0;
		}
		else {
			unsigned char fin = HDR_FIN;
			reliable_dgram_inner_packet_send(s, &fin, 1, &s->ctx.peer_saddr);
			++s->ctx.m_fin_times;
			s->ctx.m_fin_msec = timestamp_msec + s->ctx.rto;
			update_timestamp(&loop->m_event_msec, s->ctx.m_fin_msec);
		}
	}
	else if (RECONNECT_STATUS == s->ctx.m_status) {
		if (s->ctx.m_reconnect_msec > timestamp_msec) {
			update_timestamp(&loop->m_event_msec, s->ctx.m_reconnect_msec);
		}
		else if (s->ctx.m_reconnect_times >= s->ctx.resend_maxtimes) {
			s->ctx.m_status = TIME_WAIT_STATUS;
			s->ctx.m_lastactive_msec = timestamp_msec;
			s->m_valid = 0;
			s->connect(s, ETIMEDOUT, s->m_connect_times++, s->ctx.m_recvseq, s->ctx.m_cwndseq);
		}
		else {
			reliable_dgram_do_reconnect(s);
			++s->ctx.m_reconnect_times;
			s->ctx.m_reconnect_msec = timestamp_msec + s->ctx.rto;
			update_timestamp(&loop->m_event_msec, s->ctx.m_reconnect_msec);
		}
	}
	else if (ESTABLISHED_STATUS == s->ctx.m_status ||
			ESTABLISHED_FIN_STATUS == s->ctx.m_status ||
			CLOSE_WAIT_STATUS == s->ctx.m_status)
	{
		ListNode_t* cur;
		for (cur = s->ctx.m_sendpacketlist.head; cur; cur = cur->next) {
			Packet_t* packet = pod_container_of(cur, Packet_t, msg);
			if (packet->seq < s->ctx.m_cwndseq ||
				packet->seq - s->ctx.m_cwndseq >= s->ctx.cwndsize)
			{
				break;
			}
			if (packet->resend_timestamp_msec > timestamp_msec) {
				update_timestamp(&loop->m_event_msec, packet->resend_timestamp_msec);
				continue;
			}
			if (packet->resendtimes >= s->ctx.resend_maxtimes) {
				s->ctx.m_lastactive_msec = timestamp_msec;
				if (ESTABLISHED_STATUS != s->ctx.m_status) {
					s->m_valid = 0;
				}
				break;
			}
			socketWrite(s->fd, packet->data, packet->len, 0, &s->ctx.peer_saddr, sockaddrLength(&s->ctx.peer_saddr));
			packet->resendtimes++;
			packet->resend_timestamp_msec = timestamp_msec + s->ctx.rto;
			update_timestamp(&loop->m_event_msec, packet->resend_timestamp_msec);
		}
	}
}

static void reactor_socket_do_read(Session_t* s, long long timestamp_msec) {
	if (SOCK_STREAM == s->socktype) {
		struct sockaddr_storage saddr;
		if (SESSION_TRANSPORT_LISTEN == s->transport_side) {
			FD_t connfd;
			for (connfd = nioAcceptFirst(s->fd, s->m_readol, &saddr);
				connfd != INVALID_FD_HANDLE;
				connfd = nioAcceptNext(s->fd, &saddr))
			{
				if (s->accept)
					s->accept(s, connfd, &saddr);
				else
					socketClose(connfd);
			}
			s->ctx.m_lastactive_msec = timestamp_msec;
		}
		else {
			unsigned char *ptr;
			int res = socketTcpReadableBytes(s->fd);
			if (res <= 0) {
				s->m_valid = 0;
				if (0 == res) {
					s->ctx.m_status = CLOSE_WAIT_STATUS;
					if (s->shutdown) {
						s->shutdown(s);
						s->shutdown = NULL;
					}
				}
				return;
			}
			ptr = (unsigned char*)realloc(s->ctx.m_inbuf, s->ctx.m_inbuflen + res);
			if (!ptr) {
				s->m_valid = 0;
				return;
			}
			s->ctx.m_inbuf = ptr;
			res = socketRead(s->fd, s->ctx.m_inbuf + s->ctx.m_inbuflen, res, 0, &saddr);
			if (res < 0) {
				if (errnoGet() != EWOULDBLOCK) {
					s->m_valid = 0;
				}
				return;
			}
			else if (res == 0) {
				s->m_valid = 0;
				s->ctx.m_status = CLOSE_WAIT_STATUS;
				if (s->shutdown) {
					s->shutdown(s);
					s->shutdown = NULL;
				}
				return;
			}
			else {
				int(*handler)(Session_t*, unsigned char*, int, const struct sockaddr_storage*);
				int decodelen;
				s->ctx.m_inbuflen += res;
				s->ctx.m_lastactive_msec = timestamp_msec;
				s->ctx.m_heartbeat_msec = timestamp_msec;
				if (s->ctx.reliable)
					handler = reliable_stream_data_packet_handler;
				else
					handler = data_packet_handler;
				decodelen = handler(s, s->ctx.m_inbuf + s->ctx.m_inbufoff, s->ctx.m_inbuflen - s->ctx.m_inbufoff, &saddr);
				if (decodelen < 0) {
					s->ctx.m_inbuflen = s->ctx.m_inbufoff;
				}
				else {
					s->ctx.m_inbufoff += decodelen;
					if (s->ctx.m_inbufoff >= s->ctx.m_inbuflen) {
						free_inbuf(&s->ctx);
					}
				}
			}
		}
	}
	else if (SOCK_DGRAM == s->socktype) {
		struct sockaddr_storage saddr;
		unsigned char buffer[0xffff], *p_data;
		unsigned int readtimes, readmaxtimes = s->m_recvpacket_maxcnt;
		for (readtimes = 0; readtimes < readmaxtimes; ++readtimes) {
			int res;
			if (0 == readtimes) {
				Iobuf_t iov;
				if (0 == nioOverlappedData(s->m_readol, &iov, &saddr)) {
					++readmaxtimes;
					continue;
				}
				res = iobufLen(&iov);
				p_data = (unsigned char*)iobufPtr(&iov);
			}
			else {
				p_data = buffer;
				res = socketRead(s->fd, buffer, sizeof(buffer), 0, &saddr);
			}

			if (res < 0) {
				if (errnoGet() != EWOULDBLOCK) {
					s->m_valid = 0;
					if (s->ctx.reliable) {
						s->ctx.m_lastactive_msec = timestamp_msec;
						update_timestamp(&s->m_loop->m_event_msec, s->ctx.m_lastactive_msec + s->close_timeout_msec);
					}
				}
				break;
			}
			else if (s->ctx.reliable) {
				if (TIME_WAIT_STATUS == s->ctx.m_status ||
					CLOSED_STATUS == s->ctx.m_status ||
					NO_STATUS == s->ctx.m_status)
				{
					break;
				}
				if (0 == res)
					continue;
				s->ctx.decode(reset_decode_result(&s->ctx), p_data, res);
				if (s->ctx.decode_result.err || s->ctx.decode_result.incomplete)
					continue;
				if (!reliable_dgram_recv_handler(s, s->ctx.decode_result.bodyptr, s->ctx.decode_result.bodylen, &saddr, timestamp_msec))
					break;
			}
			else {
				data_packet_handler(s, p_data, res, &saddr);
				s->ctx.m_lastactive_msec = timestamp_msec;
			}
		}
	}
}

static void reactor_socket_do_write(Session_t* s, long long timestamp_msec) {
	if (SOCK_STREAM != s->socktype || !s->m_valid)
		return;
	s->m_writeol_has_commit = 0;
	if (SYN_SENT_STATUS == s->ctx.m_status) {
		int err;
		if (s->m_writeol) {
			nioFreeOverlapped(s->m_writeol);
			s->m_writeol = NULL;
		}
		if (!nioConnectCheckSuccess(s->fd)) {
			err = errnoGet();
			s->m_valid = 0;
			s->ctx.m_status = TIME_WAIT_STATUS;
			s->shutdown = NULL;
		}
		else if (!reactorsocket_read(s)) {
			err = errnoGet();
			s->m_valid = 0;
			s->shutdown = NULL;
		}
		else {
			err = 0;
			s->ctx.m_lastactive_msec = timestamp_msec;
			if (s->m_connect_times) {
				if (s->sessionid_len > 0 && s->ctx.reliable)
					s->ctx.m_status = RECONNECT_STATUS;
				else
					s->ctx.m_status = ESTABLISHED_STATUS;
				_xchg16(&s->m_shutdownflag, 0);
			}
			else {
				s->ctx.m_status = ESTABLISHED_STATUS;
			}
			if (ESTABLISHED_STATUS == s->ctx.m_status) {
				s->ctx.m_heartbeat_msec = timestamp_msec;
				if (s->ctx.heartbeat_timeout_sec > 0)
					update_timestamp(&s->m_loop->m_event_msec, s->ctx.m_heartbeat_msec + s->ctx.heartbeat_timeout_sec * 1000);
			}
		}
		s->connect(s, err, s->m_connect_times++, s->ctx.m_recvseq, s->ctx.m_cwndseq);
	}
	else if (ESTABLISHED_STATUS == s->ctx.m_status ||
		RECONNECT_STATUS == s->ctx.m_status)
	{
		stream_send_packet_continue(s);
	}
}

Session_t* sessionSend(Session_t* s, const void* data, unsigned int len, const void* to, int tolen) {
	Iobuf_t iov = iobufStaticInit(data, len);
	return sessionSendv(s, &iov, 1, to, tolen);
}

Session_t* sessionSendv(Session_t* s, const Iobuf_t iov[], unsigned int iovcnt, const void* to, int tolen) {
	unsigned int i, nbytes;
	if (!s->m_valid || s->m_shutdownflag)
		return NULL;
	if (SOCK_STREAM != s->socktype && !s->ctx.reliable) {
		if (socketWritev(s->fd, iov, iovcnt, 0, to, tolen) < 0) {
			return NULL;
		}
		return s;
	}
	if (!iov || !iovcnt) {
		if (SOCK_STREAM == s->socktype)
			return s;
		iovcnt = 0;
		nbytes = 0;
	}
	else {
		for (nbytes = 0, i = 0; i < iovcnt; ++i)
			nbytes += iobufLen(iov + i);
		if (0 == nbytes) {
			if (SOCK_STREAM == s->socktype && !s->ctx.reliable && (!s->ctx.hdrlen || !s->ctx.hdrlen(0))) {
				return s;
			}
			iovcnt = 0;
		}
	}
	if (s->ctx.reliable) {
		if (SOCK_STREAM == s->socktype) {
			size_t hdrlen = s->ctx.hdrlen ? s->ctx.hdrlen(RELIABLE_STREAM_DATA_HDR_LEN + nbytes) : 0;
			Packet_t* packet = (Packet_t*)malloc(sizeof(Packet_t) + hdrlen + RELIABLE_STREAM_DATA_HDR_LEN + nbytes);
			if (!packet)
				return NULL;
			packet->msg.type = SESSION_PACKET_MESSAGE;
			packet->s = s;
			packet->type = HDR_DATA;
			packet->need_ack = 1;
			packet->seq = 0;
			packet->offset = 0;
			packet->hdrlen = hdrlen;
			packet->len = hdrlen + RELIABLE_STREAM_DATA_HDR_LEN + nbytes;
			packet->data[hdrlen] = HDR_DATA | HDR_DATA_END_FLAG;
			if (hdrlen && s->ctx.encode) {
				s->ctx.encode(packet->data, RELIABLE_STREAM_DATA_HDR_LEN + nbytes);
			}
			for (nbytes = 0, i = 0; i < iovcnt; ++i) {
				memcpy(packet->data + hdrlen + RELIABLE_STREAM_DATA_HDR_LEN + nbytes, iobufPtr(iov + i), iobufLen(iov + i));
				nbytes += iobufLen(iov + i);
			}
			sessionloop_exec_msg(s->m_loop, &packet->msg._);
		}
		else if (ESTABLISHED_STATUS == s->ctx.m_status) {
			Packet_t* packet = NULL;
			size_t hdrlen;
			if (nbytes) {
				unsigned int offset, packetlen, copy_off, i_off;
				List_t packetlist;
				listInit(&packetlist);
				for (i = i_off = offset = 0; offset < nbytes; offset += packetlen) {
					packetlen = nbytes - offset;
					hdrlen = s->ctx.hdrlen ? s->ctx.hdrlen(RELIABLE_DGRAM_DATA_HDR_LEN + packetlen) : 0;
					if (hdrlen + RELIABLE_DGRAM_DATA_HDR_LEN + packetlen > s->ctx.mtu) {
						packetlen = s->ctx.mtu - hdrlen - RELIABLE_DGRAM_DATA_HDR_LEN;
						hdrlen = s->ctx.hdrlen ? s->ctx.hdrlen(RELIABLE_DGRAM_DATA_HDR_LEN + packetlen) : 0;
					}
					packet = (Packet_t*)malloc(sizeof(Packet_t) + hdrlen + RELIABLE_DGRAM_DATA_HDR_LEN + packetlen);
					if (!packet)
						break;
					packet->msg.type = SESSION_PACKET_MESSAGE;
					packet->s = s;
					packet->resendtimes = 0;
					packet->hdrlen = hdrlen;
					packet->len = hdrlen + RELIABLE_DGRAM_DATA_HDR_LEN + packetlen;
					packet->data[hdrlen] = HDR_DATA;
					if (hdrlen && s->ctx.encode) {
						s->ctx.encode(packet->data, RELIABLE_DGRAM_DATA_HDR_LEN + packetlen);
					}
					copy_off = 0;
					while (i < iovcnt) {
						unsigned int copy_len;
						if (iobufLen(iov + i) - i_off > packetlen - copy_off) {
							copy_len = packetlen - copy_off;
							memcpy(packet->data + hdrlen + RELIABLE_DGRAM_DATA_HDR_LEN + copy_off, ((char*)iobufPtr(iov + i)) + i_off, copy_len);
							i_off += copy_len;
							break;
						}
						else {
							copy_len = iobufLen(iov + i) - i_off;
							memcpy(packet->data + hdrlen + RELIABLE_DGRAM_DATA_HDR_LEN + copy_off, ((char*)iobufPtr(iov + i)) + i_off, copy_len);
							copy_off += copy_len;
							i_off = 0;
							++i;
						}
					}
					listInsertNodeBack(&packetlist, packetlist.tail, &packet->msg._);
				}
				if (offset >= nbytes) {
					packet->data[packet->hdrlen] |= HDR_DATA_END_FLAG;
					sessionloop_exec_msglist(s->m_loop, &packetlist);
				}
				else {
					ListNode_t* cur, *next;
					for (cur = packetlist.head; cur; cur = next) {
						next = cur->next;
						free(cur);
					}
					return NULL;
				}
			}
			else {
				hdrlen = s->ctx.hdrlen ? s->ctx.hdrlen(RELIABLE_DGRAM_DATA_HDR_LEN) : 0;
				packet = (Packet_t*)malloc(sizeof(Packet_t) + hdrlen + RELIABLE_DGRAM_DATA_HDR_LEN);
				if (!packet)
					return NULL;
				packet->msg.type = SESSION_PACKET_MESSAGE;
				packet->s = s;
				packet->resendtimes = 0;
				packet->hdrlen = hdrlen;
				packet->len = hdrlen + RELIABLE_DGRAM_DATA_HDR_LEN;
				packet->data[hdrlen] = HDR_DATA | HDR_DATA_END_FLAG;
				if (hdrlen && s->ctx.encode) {
					s->ctx.encode(packet->data, RELIABLE_DGRAM_DATA_HDR_LEN);
				}
				sessionloop_exec_msg(s->m_loop, &packet->msg._);
			}
		}
	}
	else {
		size_t hdrlen = s->ctx.hdrlen ? s->ctx.hdrlen(nbytes) : 0;
		Packet_t* packet = (Packet_t*)malloc(sizeof(Packet_t) + hdrlen + nbytes);
		if (!packet)
			return NULL;
		packet->msg.type = SESSION_PACKET_MESSAGE;
		packet->s = s;
		packet->type = HDR_DATA;
		packet->need_ack = 0;
		packet->seq = 0;
		packet->offset = 0;
		packet->hdrlen = hdrlen;
		packet->len = hdrlen + nbytes;
		if (hdrlen && s->ctx.encode) {
			s->ctx.encode(packet->data, nbytes);
		}
		for (nbytes = 0, i = 0; i < iovcnt; ++i) {
			memcpy(packet->data + hdrlen + nbytes, iobufPtr(iov + i), iobufLen(iov + i));
			nbytes += iobufLen(iov + i);
		}
		sessionloop_exec_msg(s->m_loop, &packet->msg._);
	}
	return s;
}

void sessionClientNetReconnect(Session_t* s) {
	if (SESSION_TRANSPORT_CLIENT != s->transport_side ||
		(SOCK_DGRAM == s->socktype && !s->ctx.reliable))
	{
		return;
	}
	else if (_xchg16(&s->m_shutdownflag, 1))
		return;
	sessionloop_exec_msg(s->m_loop, &s->m_netreconnectmsg._);
}

void sessionReconnectRecovery(Session_t* s) {
	if (!s->ctx.reliable || SOCK_DGRAM == s->socktype)
		return;
	else if (SESSION_TRANSPORT_CLIENT == s->transport_side ||
			SESSION_TRANSPORT_SERVER == s->transport_side)
	{
		if (_xchg16(&s->m_reconnectrecovery, 1))
			return;
		sessionloop_exec_msg(s->m_loop, &s->m_reconnectrecoverymsg._);
	}
}

int sessionTransportGrab(Session_t* s, Session_t* target_s, unsigned int recvseq, unsigned int cwndseq) {
	if (threadEqual(s->m_loop->m_runthread, threadSelf()) &&
		threadEqual(target_s->m_loop->m_runthread, threadSelf()))
	{
		SessionTransportGrabMsg_t ts;
		if (!session_grab_transport_check(&target_s->ctx, recvseq, cwndseq))
			return 0;
		session_grab_transport(&target_s->ctx, &ts.target_status);
		session_replace_transport(s, &ts.target_status);
		return 1;
	}
	else {
		SessionTransportGrabMsg_t* ts = (SessionTransportGrabMsg_t*)malloc(sizeof(SessionTransportGrabMsg_t));
		if (!ts)
			return 0;
		else if (!mutexCreate(&ts->blocklock)) {
			free(ts);
			return 0;
		}
		else if (threadEqual(target_s->m_loop->m_runthread, threadSelf())) {
			if (!session_grab_transport_check(&target_s->ctx, recvseq, cwndseq))
				return 0;
			session_grab_transport(&target_s->ctx, &ts->target_status);
		}
		else {
			mutexLock(&ts->blocklock);
			ts->msg.type = SESSION_TRANSPORT_GRAB_ASYNC_REQ_MESSAGE;
			ts->s = target_s;
			ts->recvseq = recvseq;
			ts->cwndseq = cwndseq;
			sessionloop_exec_msg(target_s->m_loop, &ts->msg._);
			mutexLock(&ts->blocklock);
			mutexUnlock(&ts->blocklock);
			if (!ts->success) {
				mutexClose(&ts->blocklock);
				free(ts);
				return 0;
			}
			else if (threadEqual(s->m_loop->m_runthread, threadSelf())) {
				session_replace_transport(s, &ts->target_status);
				return 1;
			}
		}
		mutexLock(&ts->blocklock);
		ts->msg.type = SESSION_TRANSPORT_GRAB_ASYNC_RET_MESSAGE;
		ts->s = s;
		sessionloop_exec_msg(s->m_loop, &ts->msg._);
		mutexLock(&ts->blocklock);
		mutexUnlock(&ts->blocklock);
		mutexClose(&ts->blocklock);
		free(ts);
		return 1;
	}
}

void sessionShutdown(Session_t* s) {
	if (_xchg16(&s->m_shutdownflag, 1))
		return;
	sessionloop_exec_msg(s->m_loop, &s->m_shutdownpostmsg._);
}

static TransportCtx_t* netTransportInitCtx(TransportCtx_t* ctx) {
	ctx->io_object = NULL;
	ctx->peer_saddr.ss_family = AF_UNSPEC;
	ctx->mtu = 1464;
	ctx->rto = 200;
	ctx->resend_maxtimes = 5;
	ctx->cwndsize = 10;
	ctx->reliable = 0;
	ctx->decode = NULL;
	ctx->recv = NULL;
	ctx->hdrlen = NULL;
	ctx->encode = NULL;
	ctx->heartbeat = NULL;
	ctx->zombie = NULL;
	ctx->heartbeat_timeout_sec = 0;
	ctx->zombie_timeout_sec = 0;
	ctx->m_status = NO_STATUS;
	ctx->m_synrcvd_times = 0;
	ctx->m_synsent_times = 0;
	ctx->m_reconnect_times = 0;
	ctx->m_fin_times = 0;
	ctx->m_synrcvd_msec = 0;
	ctx->m_synsent_msec = 0;
	ctx->m_reconnect_msec = 0;
	ctx->m_fin_msec = 0;
	ctx->m_cwndseq = 0;
	ctx->m_recvseq = 0;
	ctx->m_sendseq = 0;
	ctx->m_cwndseqbak = 0;
	ctx->m_recvseqbak = 0;
	ctx->m_sendseqbak = 0;
	ctx->m_lastactive_msec = 0;
	ctx->m_heartbeat_msec = 0;
	ctx->m_inbuf = NULL;
	ctx->m_inbufoff = 0;
	ctx->m_inbuflen = 0;
	listInit(&ctx->m_recvpacketlist);
	listInit(&ctx->m_sendpacketlist);
	listInit(&ctx->m_sendpacketlistbak);
	return reset_decode_result(ctx);
}

Session_t* sessionCreate(Session_t* s, FD_t fd, int domain, int socktype, int protocol, int transport_side) {
	int call_malloc = 0;
	if (!s) {
		s = (Session_t*)malloc(sizeof(Session_t));
		if (!s)
			return NULL;
		call_malloc = 1;
	}
	if (INVALID_FD_HANDLE == fd) {
		fd = socket(domain, socktype, protocol);
		if (INVALID_FD_HANDLE == fd) {
			if (call_malloc)
				free(s);
			return NULL;
		}
	}
	if (!socketNonBlock(fd, TRUE)) {
		socketClose(fd);
		if (call_malloc)
			free(s);
		return NULL;
	}
	s->fd = fd;
	s->domain = domain;
	s->socktype = socktype;
	s->protocol = protocol;
	s->close_timeout_msec = 0;
	s->transport_side = transport_side;
	s->sessionid_len = 0;
	s->sessionid = NULL;
	s->userdata = NULL;
	s->local_listen_saddr.ss_family = AF_UNSPEC;
	s->peer_listen_saddr.ss_family = AF_UNSPEC;
	s->reg_or_connect = NULL;
	s->accept = NULL;
	s->shutdown = NULL;
	s->close = NULL;
	s->free = NULL;
	s->shutdownmsg.type = SESSION_SHUTDOWN_MESSAGE;
	s->closemsg.type = SESSION_CLOSE_MESSAGE;

	s->m_valid = 1;
	s->m_shutdownflag = 0;
	s->m_reconnectrecovery = 0;
	s->m_connect_times = 0;
	s->m_regmsg.type = SESSION_REG_MESSAGE;
	s->m_shutdownpostmsg.type = SESSION_SHUTDOWN_POST_MESSAGE;
	s->m_netreconnectmsg.type = SESSION_CLIENT_NET_RECONNECT_MESSAGE;
	s->m_reconnectrecoverymsg.type = SESSION_RECONNECT_RECOVERY_MESSAGE;
	s->m_closemsg.type = SESSION_CLOSE_MESSAGE;
	s->m_hashnode.key = &s->fd;
	s->m_loop = NULL;
	s->m_readol = NULL;
	s->m_writeol = NULL;
	s->m_writeol_has_commit = 0;
	s->m_recvpacket_maxcnt = 8;
	netTransportInitCtx(&s->ctx);
	s->ctx.io_object = s;
	return s;
}

static void session_free(Session_t* s) {
	free_io_resource(s);
	free_inbuf(&s->ctx);
	if (SESSION_TRANSPORT_LISTEN == s->transport_side &&
		SOCK_DGRAM == s->socktype && s->ctx.reliable)
	{
		ListNode_t* cur;
		for (cur = s->ctx.m_recvpacketlist.head; cur; cur = cur->next) {
			ReliableDgramHalfConnectPacket_t* halfcon = pod_container_of(cur, ReliableDgramHalfConnectPacket_t, m_listnode);
			socketClose(halfcon->sockfd);
		}
	}
	clear_packetlist(&s->ctx.m_recvpacketlist);
	clear_packetlist(&s->ctx.m_sendpacketlist);
	clear_packetlist(&s->ctx.m_sendpacketlistbak);
	if (s->free)
		s->free(s);
	else
		free(s);
}

void sessionManualClose(Session_t* s) {
	if (s->m_loop) {
		sessionloop_exec_msg(s->m_loop, &s->m_closemsg._);
	}
	else {
		session_free(s);
	}
}

static int sockht_keycmp(const void* node_key, const void* key) {
	return *(FD_t*)node_key != *(FD_t*)key;
}

static unsigned int sockht_keyhash(const void* key) { return *(FD_t*)key; }

static void closelist_update(SessionLoop_t* loop, long long timestamp_msec) {
	ListNode_t* cur, *next;
	List_t expirelist;
	listInit(&expirelist);
	for (cur = loop->m_closelist.head; cur; cur = next) {
		Session_t* s = pod_container_of(cur, Session_t, m_closemsg);
		next = cur->next;
		if (s->ctx.m_lastactive_msec + s->close_timeout_msec > timestamp_msec) {
			update_timestamp(&loop->m_event_msec, s->ctx.m_lastactive_msec + s->close_timeout_msec);
			continue;
		}
		s->ctx.m_status = NO_STATUS;
		free_io_resource(s);
		free_inbuf(&s->ctx);
		listRemoveNode(&loop->m_closelist, cur);
		listInsertNodeBack(&expirelist, expirelist.tail, cur);
	}
	for (cur = expirelist.head; cur; cur = next) {
		Session_t* s = pod_container_of(cur, Session_t, m_closemsg);
		next = cur->next;/* same as list remove node, avoid thread race. */
		if (s->close) {
			s->close(s);
			s->close = NULL;
		}
	}
}

static void sockht_update(SessionLoop_t* loop, long long timestamp_msec) {
	HashtableNode_t *cur, *next;
	for (cur = hashtableFirstNode(&loop->m_sockht); cur; cur = next) {
		Session_t* s = pod_container_of(cur, Session_t, m_hashnode);
		next = hashtableNextNode(cur);
		if (s->m_valid) {
			if (s->ctx.zombie_timeout_sec > 0 && s->ctx.m_lastactive_msec + s->ctx.zombie_timeout_sec * 1000 <= timestamp_msec) {
				if (ESTABLISHED_STATUS == s->ctx.m_status && s->ctx.zombie && s->ctx.zombie(&s->ctx)) {
					s->ctx.m_lastactive_msec = timestamp_msec;
					continue;
				}
				s->m_valid = 0;
				free_inbuf(&s->ctx);
				if (SOCK_STREAM == s->socktype || s->ctx.zombie_timeout_sec * 1000 >= s->close_timeout_msec)
					free_io_resource(s);
			}
			else {
				if (SESSION_TRANSPORT_CLIENT == s->transport_side && s->ctx.heartbeat &&
					s->ctx.m_heartbeat_msec > 0 && s->ctx.heartbeat_timeout_sec > 0)
				{
					if (s->ctx.m_heartbeat_msec + s->ctx.heartbeat_timeout_sec * 1000 <= timestamp_msec) {
						s->ctx.m_heartbeat_msec = timestamp_msec;
						s->ctx.heartbeat(&s->ctx);
					}
					update_timestamp(&loop->m_event_msec, s->ctx.m_heartbeat_msec + s->ctx.heartbeat_timeout_sec * 1000);
				}
				if (s->ctx.zombie_timeout_sec > 0)
					update_timestamp(&loop->m_event_msec, s->ctx.m_lastactive_msec + s->ctx.zombie_timeout_sec * 1000);
				if (SOCK_DGRAM == s->socktype && s->ctx.reliable) {
					reliable_dgram_update(loop, s, timestamp_msec);
					if (s->m_valid)
						continue;
				}
				else
					continue;
			}
		}
		hashtableRemoveNode(&loop->m_sockht, cur);
		listInsertNodeBack(&loop->m_closelist, loop->m_closelist.tail, &s->m_closemsg._);
		_xchg16(&s->m_shutdownflag, 1);
	}
}

int sessionloopHandler(SessionLoop_t* loop, NioEv_t e[], int n, long long timestamp_msec, int wait_msec) {
	ListNode_t *cur, *next;
	if (!loop->m_runthreadhasbind) {
		loop->m_runthread = threadSelf();
		loop->m_runthreadhasbind = 1;
	}
	if (loop->m_event_msec > timestamp_msec) {
		int checkexpire_wait_msec = loop->m_event_msec - timestamp_msec;
		if (checkexpire_wait_msec < wait_msec || wait_msec < 0)
			wait_msec = checkexpire_wait_msec;
	}
	else if (loop->m_event_msec) {
		wait_msec = 0;
	}

	n = nioWait(&loop->m_reactor, e, n, wait_msec);
	if (n < 0) {
		return n;
	}
	else if (n > 0) {
		int i;
		timestamp_msec = gmtimeMillisecond();
		for (i = 0; i < n; ++i) {
			HashtableNode_t* find_node;
			Session_t* s;
			FD_t fd;
			if (!nioEventOverlappedCheck(e + i))
				continue;
			fd = nioEventFD(e + i);
			if (fd == loop->m_socketpair[0]) {
				struct sockaddr_storage saddr;
				char c[512];
				socketRead(fd, c, sizeof(c), 0, NULL);
				saddr.ss_family = AF_INET;
				nioCommit(&loop->m_reactor, fd, NIO_OP_READ, loop->m_readol, (struct sockaddr*)&saddr, sockaddrLength((struct sockaddr*)&saddr));
				_xchg16(&loop->m_wake, 0);
				continue;
			}
			find_node = hashtableSearchKey(&loop->m_sockht, &fd);
			if (!find_node)
				continue;
			s = pod_container_of(find_node, Session_t, m_hashnode);
			if (!s->m_valid)
				continue;
			switch (nioEventOpcode(e + i)) {
				case NIO_OP_READ:
					reactor_socket_do_read(s, timestamp_msec);
					reactorsocket_read(s);
					break;
				case NIO_OP_WRITE:
					reactor_socket_do_write(s, timestamp_msec);
					break;
				default:
					s->m_valid = 0;
			}
			if (s->m_valid)
				continue;
			hashtableRemoveNode(&loop->m_sockht, &s->m_hashnode);
			_xchg16(&s->m_shutdownflag, 1);
			free_inbuf(&s->ctx);
			if (SOCK_STREAM == s->socktype) {
				free_io_resource(s);
			}
			if (s->close_timeout_msec > 0) {
				s->ctx.m_lastactive_msec = timestamp_msec;
				update_timestamp(&loop->m_event_msec, s->ctx.m_lastactive_msec + s->close_timeout_msec);
				listInsertNodeBack(&loop->m_closelist, loop->m_closelist.tail, &s->m_closemsg._);
			}
			else {
				s->ctx.m_status = NO_STATUS;
				if (s->close) {
					s->close(s);
					s->close = NULL;
				}
			}
		}
	}
	else {
		timestamp_msec += wait_msec;
	}

	criticalsectionEnter(&loop->m_msglistlock);
	cur = loop->m_msglist.head;
	listInit(&loop->m_msglist);
	criticalsectionLeave(&loop->m_msglistlock);

	for (; cur; cur = next) {
		SessionInternalMessage_t* message = (SessionInternalMessage_t*)cur;
		next = cur->next;
		if (SESSION_CLOSE_MESSAGE == message->type) {
			Session_t* s = pod_container_of(message, Session_t, m_closemsg);
			session_free(s);
		}
		else if (SESSION_SHUTDOWN_POST_MESSAGE == message->type) {
			Session_t* s = pod_container_of(message, Session_t, m_shutdownpostmsg);
			if (SOCK_STREAM == s->socktype) {
				if (SESSION_TRANSPORT_LISTEN == s->transport_side) {
					if (s->m_valid) {
						s->m_valid = 0;
						s->ctx.m_status = NO_STATUS;
						hashtableRemoveNode(&loop->m_sockht, &s->m_hashnode);
						if (s->close) {
							s->close(s);
							s->close = NULL;
						}
					}
				}
				else if (ESTABLISHED_STATUS == s->ctx.m_status) {
					s->ctx.m_status = ESTABLISHED_FIN_STATUS;
					s->ctx.m_heartbeat_msec = 0;
					socketShutdown(s->fd, SHUT_WR);
				}
			}
			else {
				reliable_dgram_shutdown(s, timestamp_msec);
			}
		}
		else if (SESSION_PACKET_MESSAGE == message->type) {
			Packet_t* packet = pod_container_of(message, Packet_t, msg);
			Session_t* s = packet->s;
			if (!s->m_valid ||
				(ESTABLISHED_STATUS != s->ctx.m_status && RECONNECT_STATUS != s->ctx.m_status))
			{
				free(packet);
				continue;
			}
			if (SOCK_STREAM == s->socktype) {
				if (s->ctx.reliable) {
					if (ESTABLISHED_STATUS == s->ctx.m_status) {
						packet->seq = s->ctx.m_sendseq++;
						*(unsigned int*)(packet->data + packet->hdrlen + 1) = htonl(packet->seq);
					}
					else {
						packet->type = HDR_RECONNECT;
						packet->need_ack = 0;
						packet->data[packet->hdrlen] = HDR_RECONNECT | HDR_DATA_END_FLAG;
					}
				}
				stream_send_packet(packet->s, packet);
			}
			else {
				packet->seq = s->ctx.m_sendseq++;
				*(unsigned int*)(packet->data + packet->hdrlen + 1) = htonl(packet->seq);
				reliable_dgram_send_packet(s, packet, timestamp_msec);
			}
		}
		else if (SESSION_CLIENT_NET_RECONNECT_MESSAGE == message->type) {
			Session_t* s = pod_container_of(message, Session_t, m_netreconnectmsg);
			if (!s->m_valid || SESSION_TRANSPORT_CLIENT != s->transport_side)
				continue;
			if (ESTABLISHED_STATUS != s->ctx.m_status && RECONNECT_STATUS != s->ctx.m_status)
				continue;
			if (SOCK_STREAM == s->socktype) {
				int ok;
				hashtableRemoveNode(&loop->m_sockht, &s->m_hashnode);
				free_io_resource(s);
				free_inbuf(&s->ctx);
				s->m_writeol_has_commit = 0;
				s->ctx.m_heartbeat_msec = 0;
				do {
					ok = 0;
					s->fd = socket(s->domain, s->socktype, s->protocol);
					if (INVALID_FD_HANDLE == s->fd)
						break;
					if (!nioReg(&loop->m_reactor, s->fd))
						break;
					if (!s->m_writeol) {
						s->m_writeol = nioAllocOverlapped(NIO_OP_CONNECT, NULL, 0, 0);
						if (!s->m_writeol)
							break;
					}
					if (!nioCommit(&loop->m_reactor, s->fd, NIO_OP_CONNECT, s->m_writeol,
						(struct sockaddr*)(&s->peer_listen_saddr), sockaddrLength((struct sockaddr*)(&s->peer_listen_saddr))))
					{
						break;
					}
					if (s->sessionid_len > 0 &&
						s->ctx.reliable &&
						ESTABLISHED_STATUS == s->ctx.m_status)
					{
						reliable_stream_bak(&s->ctx);
					}
					else {
						clear_packetlist(&s->ctx.m_sendpacketlist);
					}
					s->m_writeol_has_commit = 1;
					s->ctx.m_status = SYN_SENT_STATUS;
					s->ctx.m_lastactive_msec = timestamp_msec;
					hashtableReplaceNode(hashtableInsertNode(&loop->m_sockht, &s->m_hashnode), &s->m_hashnode);
					ok = 1;
				} while (0);
				if (!ok) {
					free_io_resource(s);
					clear_packetlist(&s->ctx.m_sendpacketlist);
					s->m_valid = 0;
					s->connect(s, errnoGet(), s->m_connect_times++, s->ctx.m_recvseq, s->ctx.m_cwndseq);

					if (s->close_timeout_msec > 0) {
						s->ctx.m_lastactive_msec = timestamp_msec;
						update_timestamp(&loop->m_event_msec, s->ctx.m_lastactive_msec + s->close_timeout_msec);
						listInsertNodeBack(&loop->m_closelist, loop->m_closelist.tail, &s->m_closemsg._);
					}
					else {
						s->ctx.m_status = NO_STATUS;
						if (s->close) {
							s->close(s);
							s->close = NULL;
						}
					}
				}
			}
			else if (ESTABLISHED_STATUS == s->ctx.m_status) {
				reliable_dgram_do_reconnect(s);
				s->ctx.m_lastactive_msec = timestamp_msec;
				s->ctx.m_heartbeat_msec = 0;
				s->ctx.m_status = RECONNECT_STATUS;
				s->ctx.m_reconnect_times = 0;
				s->ctx.m_reconnect_msec = timestamp_msec + s->ctx.rto;
				update_timestamp(&s->m_loop->m_event_msec, s->ctx.m_reconnect_msec);
			}
		}
		else if (SESSION_RECONNECT_RECOVERY_MESSAGE == message->type) {
			Session_t* s = pod_container_of(message, Session_t, m_reconnectrecoverymsg);
			_xchg16(&s->m_reconnectrecovery, 0);
			if (RECONNECT_STATUS != s->ctx.m_status)
				continue;
			s->ctx.m_status = ESTABLISHED_STATUS;
			if (SESSION_TRANSPORT_CLIENT == s->transport_side) {
				reliable_stream_bak_recovery(&s->ctx);
			}
			stream_send_packet_continue(s);
		}
		else if (SESSION_TRANSPORT_GRAB_ASYNC_REQ_MESSAGE == message->type) {
			SessionTransportGrabMsg_t* ts = pod_container_of(message, SessionTransportGrabMsg_t, msg);
			Session_t* s = ts->s;
			if (session_grab_transport_check(&s->ctx, ts->recvseq, ts->cwndseq)) {
				ts->success = 1;
				session_grab_transport(&s->ctx, &ts->target_status);
			}
			else {
				ts->success = 0;
			}
			mutexUnlock(&ts->blocklock);
		}
		else if (SESSION_TRANSPORT_GRAB_ASYNC_RET_MESSAGE == message->type) {
			SessionTransportGrabMsg_t* ts = pod_container_of(message, SessionTransportGrabMsg_t, msg);
			Session_t* s = ts->s;
			session_replace_transport(s, &ts->target_status);
			mutexUnlock(&ts->blocklock);
		}
		else if (SESSION_REG_MESSAGE == message->type) {
			Session_t* s = pod_container_of(message, Session_t, m_regmsg);
			int reg_ok = 0, immedinate_call = 0;
			do {
				if (!nioReg(&loop->m_reactor, s->fd))
					break;
				s->ctx.m_lastactive_msec = timestamp_msec;
				if (SOCK_STREAM == s->socktype) {
					if (SESSION_TRANSPORT_CLIENT == s->transport_side) {
						BOOL has_connected;
						s->ctx.peer_saddr = s->peer_listen_saddr;
						if (!socketIsConnected(s->fd, &has_connected))
							break;
						if (has_connected) {
							if (!reactorsocket_read(s))
								break;
							s->ctx.m_heartbeat_msec = timestamp_msec;
							s->ctx.m_status = ESTABLISHED_STATUS;
							immedinate_call = 1;
						}
						else {
							if (!s->m_writeol) {
								s->m_writeol = nioAllocOverlapped(NIO_OP_CONNECT, NULL, 0, 0);
								if (!s->m_writeol)
									break;
							}
							if (!nioCommit(&loop->m_reactor, s->fd, NIO_OP_CONNECT, s->m_writeol,
								(struct sockaddr*)(&s->peer_listen_saddr), sockaddrLength((struct sockaddr*)(&s->peer_listen_saddr))))
							{
								break;
							}
							s->m_writeol_has_commit = 1;
							s->ctx.m_status = SYN_SENT_STATUS;
						}
					}
					else {
						if (SESSION_TRANSPORT_LISTEN == s->transport_side) {
							BOOL has_listen;
							if (AF_UNSPEC == s->local_listen_saddr.ss_family) {
								if (!socketGetLocalAddr(s->fd, &s->local_listen_saddr))
									break;
							}
							if (!socketIsListened(s->fd, &has_listen))
								break;
							if (!has_listen && !socketTcpListen(s->fd))
								break;
							s->ctx.m_status = LISTENED_STATUS;
						}
						else {
							s->ctx.m_status = ESTABLISHED_STATUS;
							s->ctx.m_heartbeat_msec = timestamp_msec;
						}
						if (!reactorsocket_read(s))
							break;
						immedinate_call = 1;
					}
				}
				else {
					if (s->ctx.reliable) {
						if (SESSION_TRANSPORT_CLIENT == s->transport_side && s->peer_listen_saddr.ss_family != AF_UNSPEC) {
							unsigned char syn = HDR_SYN;
							reliable_dgram_inner_packet_send(s, &syn, 1, &s->peer_listen_saddr);
							s->ctx.m_status = SYN_SENT_STATUS;
							s->ctx.peer_saddr = s->peer_listen_saddr;
							s->ctx.m_synsent_msec = timestamp_msec + s->ctx.rto;
							update_timestamp(&loop->m_event_msec, s->ctx.m_synsent_msec);
						}
						else if (SESSION_TRANSPORT_LISTEN == s->transport_side) {
							if (AF_UNSPEC == s->local_listen_saddr.ss_family) {
								if (!socketGetLocalAddr(s->fd, &s->local_listen_saddr))
									break;
							}
							if (s->accept) {
								s->ctx.m_status = LISTENED_STATUS;
								if (s->m_recvpacket_maxcnt < 200)
									s->m_recvpacket_maxcnt = 200;
								immedinate_call = 1;
							}
							else {
								s->ctx.m_status = SYN_RCVD_STATUS;
							}
						}
						else {
							s->ctx.m_status = ESTABLISHED_STATUS;
							s->ctx.m_heartbeat_msec = timestamp_msec;
							immedinate_call = 1;
						}
						if (s->close_timeout_msec < MSL + MSL) {
							s->close_timeout_msec = MSL + MSL;
						}
					}
					else {
						s->ctx.m_status = ESTABLISHED_STATUS;
						immedinate_call = 1;
					}
					if (!reactorsocket_read(s))
						break;
				}
				hashtableReplaceNode(hashtableInsertNode(&loop->m_sockht, &s->m_hashnode), &s->m_hashnode);
				if (s->ctx.m_heartbeat_msec > 0 && s->ctx.heartbeat_timeout_sec > 0) {
					update_timestamp(&loop->m_event_msec, s->ctx.m_heartbeat_msec + s->ctx.heartbeat_timeout_sec * 1000);
				}
				if (s->ctx.zombie_timeout_sec > 0) {
					update_timestamp(&loop->m_event_msec, s->ctx.m_lastactive_msec + s->ctx.zombie_timeout_sec * 1000);
				}
				reg_ok = 1;
			} while (0);
			if (reg_ok) {
				if (immedinate_call && s->reg_or_connect) {
					if (SESSION_TRANSPORT_CLIENT == s->transport_side)
						s->connect(s, 0, s->m_connect_times++, 0, 0);
					else
						s->reg(s, 0);
				}
			}
			else {
				s->ctx.m_status = NO_STATUS;
				s->shutdown = NULL;
				if (s->reg_or_connect) {
					if (SESSION_TRANSPORT_CLIENT == s->transport_side)
						s->connect(s, errnoGet(), s->m_connect_times++, 0, 0);
					else
						s->reg(s, errnoGet());
				}
				if (s->close) {
					s->close(s);
					s->close = NULL;
				}
			}
		}
	}
	if (loop->m_event_msec && timestamp_msec >= loop->m_event_msec) {
		loop->m_event_msec = 0;
		sockht_update(loop, timestamp_msec);
		closelist_update(loop, timestamp_msec);
	}
	return n;
}

SessionLoop_t* sessionloopCreate(SessionLoop_t* loop) {
	struct sockaddr_storage saddr;
	loop->m_initok = 0;

	if (!socketPair(SOCK_STREAM, loop->m_socketpair))
		return NULL;
	saddr.ss_family = AF_INET;

	loop->m_readol = nioAllocOverlapped(NIO_OP_READ, NULL, 0, 0);
	if (!loop->m_readol) {
		socketClose(loop->m_socketpair[0]);
		socketClose(loop->m_socketpair[1]);
		return NULL;
	}

	if (!nioCreate(&loop->m_reactor)) {
		nioFreeOverlapped(loop->m_readol);
		socketClose(loop->m_socketpair[0]);
		socketClose(loop->m_socketpair[1]);
		return NULL;
	}

	if (!socketNonBlock(loop->m_socketpair[0], TRUE) ||
		!socketNonBlock(loop->m_socketpair[1], TRUE) ||
		!nioReg(&loop->m_reactor, loop->m_socketpair[0]) ||
		!nioCommit(&loop->m_reactor, loop->m_socketpair[0], NIO_OP_READ, loop->m_readol,
			(struct sockaddr*)&saddr, sockaddrLength((struct sockaddr*)&saddr)))
	{
		nioFreeOverlapped(loop->m_readol);
		socketClose(loop->m_socketpair[0]);
		socketClose(loop->m_socketpair[1]);
		nioClose(&loop->m_reactor);
		return NULL;
	}

	if (!criticalsectionCreate(&loop->m_msglistlock)) {
		nioFreeOverlapped(loop->m_readol);
		socketClose(loop->m_socketpair[0]);
		socketClose(loop->m_socketpair[1]);
		nioClose(&loop->m_reactor);
		return NULL;
	}

	listInit(&loop->m_msglist);
	listInit(&loop->m_closelist);
	hashtableInit(&loop->m_sockht,
		loop->m_sockht_bulks, sizeof(loop->m_sockht_bulks) / sizeof(loop->m_sockht_bulks[0]),
		sockht_keycmp, sockht_keyhash);
	loop->m_initok = 1;
	loop->m_runthreadhasbind = 0;
	loop->m_wake = 0;
	loop->m_event_msec = 0;
	return loop;
}

SessionLoop_t* sessionloopWake(SessionLoop_t* loop) {
	if (0 == _cmpxchg16(&loop->m_wake, 1, 0)) {
		char c;
		socketWrite(loop->m_socketpair[1], &c, sizeof(c), 0, NULL, 0);
	}
	return loop;
}

void sessionloopReg(SessionLoop_t* loop, Session_t* s[], size_t n) {
	size_t i;
	List_t list;
	listInit(&list);
	for (i = 0; i < n; ++i) {
		s[i]->m_loop = loop;
		listInsertNodeBack(&list, list.tail, &s[i]->m_regmsg._);
	}
	sessionloop_exec_msglist(loop, &list);
}

void sessionloopDestroy(SessionLoop_t* loop) {
	if (loop && loop->m_initok) {
		nioFreeOverlapped(loop->m_readol);
		socketClose(loop->m_socketpair[0]);
		socketClose(loop->m_socketpair[1]);
		nioClose(&loop->m_reactor);
		criticalsectionClose(&loop->m_msglistlock);
		do {
			ListNode_t* cur, *next;
			for (cur = loop->m_msglist.head; cur; cur = next) {
				SessionInternalMessage_t* msgbase = (SessionInternalMessage_t*)cur;
				next = cur->next;
				if (SESSION_PACKET_MESSAGE == msgbase->type)
					free(pod_container_of(cur, Packet_t, msg));
			}
		} while (0);
		do {
			HashtableNode_t *cur, *next;
			for (cur = hashtableFirstNode(&loop->m_sockht); cur; cur = next) {
				next = hashtableNextNode(cur);
				session_free(pod_container_of(cur, Session_t, m_hashnode));
			}
		} while (0);
	}
}

#ifdef __cplusplus
}
#endif
