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
	HDR_SYN,
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
	SessionInternalMsg_t msg;
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
	SessionInternalMsg_t msg;
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
	int need_wake;
	criticalsectionEnter(&loop->m_msglistlock);
	need_wake = !loop->m_msglist.head;
	listInsertNodeBack(&loop->m_msglist, loop->m_msglist.tail, msgnode);
	criticalsectionLeave(&loop->m_msglistlock);
	if (need_wake) {
		char c;
		socketWrite(loop->m_socketpair[1], &c, sizeof(c), 0, NULL, 0);
	}
	else {
		sessionloopWake(loop);
	}
	return loop;
}

static SessionLoop_t* sessionloop_exec_msglist(SessionLoop_t* loop, List_t* msglist) {
	int need_wake;
	criticalsectionEnter(&loop->m_msglistlock);
	need_wake = !loop->m_msglist.head;
	listMerge(&loop->m_msglist, msglist);
	criticalsectionLeave(&loop->m_msglistlock);
	if (need_wake) {
		char c;
		socketWrite(loop->m_socketpair[1], &c, sizeof(c), 0, NULL, 0);
	}
	else {
		sessionloopWake(loop);
	}
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
	if (nioreactorCommit(&s->m_loop->m_reactor, s->fd, opcode, s->m_readol,
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
	if (nioreactorCommit(&s->m_loop->m_reactor, s->fd, NIO_OP_WRITE, s->m_writeol,
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

static void free_inbuf(Session_t* s) {
	free(s->m_inbuf);
	s->m_inbuf = NULL;
	s->m_inbuflen = 0;
	s->m_inbufoffset = 0;
}

static SessionDecodeResult_t* reset_decode_result(SessionDecodeResult_t* result) {
	result->err = 0;
	result->incomplete = 0;
	result->decodelen = 0;
	result->bodylen = 0;
	result->bodyptr = NULL;
	return result;
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
		for (cur = s->m_sendpacketlist.head; cur; cur = next) {
			packet = pod_container_of(cur, Packet_t, msg.m_listnode);
			next = cur->next;
			if (packet->offset >= packet->len) {
				listRemoveNode(&s->m_sendpacketlist, cur);
				free(packet);
			}
		}
		if (s->m_sendpacketlist.head) {
			for (cur = s->m_sendpacketlist.head->next; cur; cur = next) {
				next = cur->next;
				listRemoveNode(&s->m_sendpacketlist, cur);
				free(cur);
			}
		}
	}
	else {
		clear_packetlist(&s->m_sendpacketlist);
	}
	clear_packetlist(&s->m_recvpacketlist);
	s->reliable.m_recvseq = target_status->m_recvseq;
	s->reliable.m_sendseq = target_status->m_sendseq;
	s->reliable.m_cwndseq = target_status->m_cwndseq;
	s->m_recvpacketlist = target_status->m_recvpacketlist;
	listMerge(&s->m_sendpacketlist, &target_status->m_sendpacketlist);
	listInit(&target_status->m_recvpacketlist);
}

static void session_grab_transport(Session_t* s, SessionTransportStatus_t* target_status) {
	target_status->m_cwndseq = s->reliable.m_cwndseq;
	target_status->m_recvseq = s->reliable.m_recvseq;
	target_status->m_sendseq = s->reliable.m_sendseq;
	target_status->m_recvpacketlist = s->m_recvpacketlist;
	target_status->m_sendpacketlist = s->m_sendpacketlist;
	s->reliable.m_cwndseq = 0;
	s->reliable.m_recvseq = 0;
	s->reliable.m_sendseq = 0;
	listInit(&s->m_recvpacketlist);
	listInit(&s->m_sendpacketlist);
}

static int session_grab_transport_check(Session_t* s, unsigned int recvseq, unsigned int cwndseq) {
	if (seq1_before_seq2(recvseq, s->reliable.m_cwndseq))
		return 0;
	if (seq1_before_seq2(s->reliable.m_recvseq, cwndseq))
		return 0;
	return 1;
}

static void stream_send_packet(Session_t* s, Packet_t* packet) {
	int res;
	if (s->m_sendpacketlist.tail) {
		Packet_t* last_packet = pod_container_of(s->m_sendpacketlist.tail, Packet_t, msg.m_listnode);
		if (last_packet->offset < last_packet->len) {
			packet->offset = 0;
			listInsertNodeBack(&s->m_sendpacketlist, s->m_sendpacketlist.tail, &packet->msg.m_listnode);
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
	listInsertNodeBack(&s->m_sendpacketlist, s->m_sendpacketlist.tail, &packet->msg.m_listnode);
	packet->offset = res;
	if (res < packet->len)
		reactorsocket_write(s);
}

static void stream_send_packet_continue(Session_t* s) {
	ListNode_t* cur, *next;
	if (s->m_writeol_has_commit) {
		return;
	}
	for (cur = s->m_sendpacketlist.head; cur; cur = next) {
		int res;
		Packet_t* packet = pod_container_of(cur, Packet_t, msg.m_listnode);
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
			listRemoveNode(&s->m_sendpacketlist, cur);
			free(packet);
		}
	}
}

static void reliable_stream_bak(Session_t* s) {
	ListNode_t* cur, *next;
	s->m_sendpacketlist_bak = s->m_sendpacketlist;
	listInit(&s->m_sendpacketlist);
	for (cur = s->m_sendpacketlist_bak.head; cur; cur = next) {
		Packet_t* packet = pod_container_of(cur, Packet_t, msg.m_listnode);
		unsigned char hdrtype = packet->type;
		next = cur->next;
		if (HDR_DATA != hdrtype) {
			listRemoveNode(&s->m_sendpacketlist_bak, cur);
			free(packet);
		}
		else {
			packet->offset = 0;
		}
	}
	s->reliable.m_cwndseq_bak = s->reliable.m_cwndseq;
	s->reliable.m_recvseq_bak = s->reliable.m_recvseq;
	s->reliable.m_sendseq_bak = s->reliable.m_sendseq;
}

static void reliable_stream_bak_recovery(Session_t* s) {
	s->reliable.m_recvseq = s->reliable.m_recvseq_bak;
	s->reliable.m_sendseq = s->reliable.m_sendseq_bak;
	s->reliable.m_cwndseq = s->reliable.m_cwndseq_bak;
	s->m_sendpacketlist = s->m_sendpacketlist_bak;
	listInit(&s->m_sendpacketlist_bak);
}

static int reliable_stream_reply_ack(Session_t* s, unsigned int seq) {
	ListNode_t* cur;
	Packet_t* packet = NULL;
	size_t hdrlen = s->hdrlen ? s->hdrlen(RELIABLE_STREAM_DATA_HDR_LEN) : 0;
	unsigned int sizeof_ack = hdrlen + RELIABLE_STREAM_DATA_HDR_LEN;
	for (cur = s->m_sendpacketlist.head; cur; cur = cur->next) {
		packet = pod_container_of(cur, Packet_t, msg.m_listnode);
		if (packet->offset < packet->len)
			break;
	}
	if (!packet || s->m_sendpacketlist.tail == &packet->msg.m_listnode) {
		int res;
		unsigned char* ack = (unsigned char*)alloca(sizeof_ack);
		if (hdrlen && s->encode) {
			s->encode(ack, RELIABLE_STREAM_DATA_HDR_LEN);
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
			listInsertNodeBack(&s->m_sendpacketlist, s->m_sendpacketlist.tail, &packet->msg.m_listnode);
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
		if (hdrlen && s->encode) {
			s->encode(ack_packet->data, RELIABLE_STREAM_DATA_HDR_LEN);
		}
		ack_packet->data[hdrlen] = HDR_ACK;
		*(unsigned int*)(ack_packet->data + hdrlen + 1) = htonl(seq);
		listInsertNodeBack(&s->m_sendpacketlist, &packet->msg.m_listnode, &ack_packet->msg.m_listnode);
	}
	return 1;
}

static void reliable_stream_do_ack(Session_t* s, unsigned int seq) {
	Packet_t* packet = NULL;
	ListNode_t* cur, *next;
	List_t freepacketlist;
	listInit(&freepacketlist);
	for (cur = s->m_sendpacketlist.head; cur; cur = next) {
		unsigned char pkg_hdr_type;
		unsigned int pkg_seq;
		next = cur->next;
		packet = pod_container_of(cur, Packet_t, msg.m_listnode);
		pkg_hdr_type = packet->type;
		if (HDR_DATA != pkg_hdr_type)
			continue;
		if (packet->offset < packet->len)
			break;
		pkg_seq = packet->seq;
		if (seq1_before_seq2(seq, pkg_seq))
			break;
		if (next) {
			packet = pod_container_of(next, Packet_t, msg.m_listnode);
			s->reliable.m_cwndseq = packet->seq;
		}
		else
			s->reliable.m_cwndseq++;
		listRemoveNode(&s->m_sendpacketlist, cur);
		listInsertNodeBack(&freepacketlist, freepacketlist.tail, cur);
		if (pkg_seq == seq)
			break;
	}
	if (freepacketlist.head) {
		for (cur = freepacketlist.head; cur; cur = next) {
			next = cur->next;
			free(pod_container_of(cur, Packet_t, msg.m_listnode));
		}
		stream_send_packet_continue(s);
	}
}

static int reliable_stream_data_packet_handler(Session_t* s, unsigned char* data, int len, const struct sockaddr_storage* saddr) {
	SessionDecodeResult_t decode_result;
	int offset = 0;
	while (offset < len) {
		s->decode(data + offset, len - offset, reset_decode_result(&decode_result));
		if (decode_result.err)
			return -1;
		else if (decode_result.incomplete)
			return offset;
		offset += decode_result.decodelen;
		if (decode_result.bodylen < RELIABLE_STREAM_DATA_HDR_LEN) {
			continue;
		}
		else {
			unsigned char hdr_type = *decode_result.bodyptr & (~HDR_DATA_END_FLAG);
			unsigned int seq = *(unsigned int*)(decode_result.bodyptr + 1);
			seq = ntohl(seq);
			if (HDR_ACK == hdr_type) {
				if (ESTABLISHED_STATUS == s->reliable.m_status)
					reliable_stream_do_ack(s, seq);
				continue;
			}
			else if (HDR_DATA == hdr_type || HDR_RECONNECT == hdr_type) {
				int packet_is_valid = 1;
				if (HDR_DATA == hdr_type) {
					if (RECONNECT_STATUS == s->reliable.m_status)
						packet_is_valid = 0;
					else {
						if (seq1_before_seq2(seq, s->reliable.m_recvseq))
							packet_is_valid = 0;
						else if (seq == s->reliable.m_recvseq)
							++s->reliable.m_recvseq;
						else {
							s->m_valid = 0;
							return -1;
						}
						reliable_stream_reply_ack(s, seq);
					}
				}
				else if (ESTABLISHED_STATUS == s->reliable.m_status) {
					ListNode_t* cur, *next;
					s->reliable.m_status = RECONNECT_STATUS;
					s->m_heartbeat_msec = 0;
					for (cur = s->m_sendpacketlist.head; cur; cur = next) {
						Packet_t* packet = pod_container_of(cur, Packet_t, msg.m_listnode);
						next = cur->next;
						if (packet->offset >= packet->len)
							free(packet);
						else
							packet->need_ack = 0;
					}
				}
				if (packet_is_valid) {
					decode_result.bodylen -= RELIABLE_STREAM_DATA_HDR_LEN;
					if (decode_result.bodylen > 0)
						decode_result.bodyptr += RELIABLE_STREAM_DATA_HDR_LEN;
					else
						decode_result.bodyptr = NULL;
					s->recv(s, saddr, &decode_result);
				}
			}
		}
	}
	return offset;
}

static int data_packet_handler(Session_t* s, unsigned char* data, int len, const struct sockaddr_storage* saddr) {
	SessionDecodeResult_t decode_result;
	if (len) {
		int offset = 0;
		while (offset < len) {
			s->decode(data + offset, len - offset, reset_decode_result(&decode_result));
			if (decode_result.err)
				return -1;
			else if (decode_result.incomplete)
				return offset;
			offset += decode_result.decodelen;
			s->recv(s, saddr, &decode_result);
		}
		return offset;
	}
	else if (SOCK_STREAM != s->socktype) {
		s->recv(s, saddr, reset_decode_result(&decode_result));
	}
	return 0;
}

static void reliable_dgram_send_again(Session_t* s, long long timestamp_msec) {
	ListNode_t* cur;
	for (cur = s->m_sendpacketlist.head; cur; cur = cur->next) {
		Packet_t* packet = pod_container_of(cur, Packet_t, msg.m_listnode);
		if (packet->seq < s->reliable.m_cwndseq ||
			packet->seq - s->reliable.m_cwndseq >= s->reliable.cwndsize)
		{
			break;
		}
		socketWrite(s->fd, packet->data, packet->len, 0, &s->reliable.peer_saddr, sockaddrLength(&s->reliable.peer_saddr));
		packet->resendtimes = 0;
		packet->resend_timestamp_msec = timestamp_msec + s->reliable.rto;
		update_timestamp(&s->m_loop->m_event_msec, packet->resend_timestamp_msec);
	}
}

static int reliable_dgram_inner_packet_send(Session_t* s, const unsigned char* data, int len, const struct sockaddr_storage* saddr) {
	size_t hdrlen = s->hdrlen ? s->hdrlen(len) : 0;
	if (hdrlen && s->encode) {
		Iobuf_t iov[2] = {
			iobufStaticInit(alloca(hdrlen), hdrlen),
			iobufStaticInit(data, len)
		};
		s->encode((unsigned char*)iobufPtr(&iov[0]), len);
		return socketWritev(s->fd, iov, sizeof(iov) / sizeof(iov[0]), 0, saddr, sockaddrLength(saddr));
	}
	else {
		return socketWrite(s->fd, data, len, 0, saddr, sockaddrLength(saddr));
	}
}

static void reliable_dgram_packet_merge(Session_t* s, unsigned char* data, int len, const struct sockaddr_storage* saddr) {
	SessionDecodeResult_t decode_result;
	unsigned char hdr_data_end_flag = data[0] & HDR_DATA_END_FLAG;
	len -= RELIABLE_DGRAM_DATA_HDR_LEN;
	data += RELIABLE_DGRAM_DATA_HDR_LEN;
	if (!s->m_inbuf && hdr_data_end_flag) {
		reset_decode_result(&decode_result);
		decode_result.bodyptr = data;
		decode_result.bodylen = len;
		s->recv(s, saddr, &decode_result);
	}
	else {
		unsigned char* ptr = (unsigned char*)realloc(s->m_inbuf, s->m_inbuflen + len);
		if (ptr) {
			s->m_inbuf = ptr;
			memcpy(s->m_inbuf + s->m_inbuflen, data, len);
			s->m_inbuflen += len;
			if (!hdr_data_end_flag)
				return;
			else {
				reset_decode_result(&decode_result);
				decode_result.bodyptr = s->m_inbuf;
				decode_result.bodylen = s->m_inbuflen;
				s->recv(s, saddr, &decode_result);
			}
		}
		free_inbuf(s);
	}
}

static void reliable_dgram_check_send_fin_packet(Session_t* s, long long timestamp_msec) {
	if (ESTABLISHED_FIN_STATUS != s->reliable.m_status && CLOSE_WAIT_STATUS != s->reliable.m_status)
		return;
	else if (s->m_sendpacketlist.head)
		return;
	else {
		unsigned char fin = HDR_FIN;
		reliable_dgram_inner_packet_send(s, &fin, sizeof(fin), &s->reliable.peer_saddr);
		s->reliable.m_fin_msec = timestamp_msec + s->reliable.rto;
		update_timestamp(&s->m_loop->m_event_msec, s->reliable.m_fin_msec);
		if (ESTABLISHED_FIN_STATUS == s->reliable.m_status)
			s->reliable.m_status = FIN_WAIT_1_STATUS;
		else if (CLOSE_WAIT_STATUS == s->reliable.m_status)
			s->reliable.m_status = LAST_ACK_STATUS;
	}
}

static void reliable_dgram_shutdown(Session_t* s, long long timestamp_msec) {
	if (SESSION_TRANSPORT_LISTEN == s->transport_side) {
		s->reliable.m_status = CLOSED_STATUS;
		s->m_lastactive_msec = timestamp_msec;
		s->m_valid = 0;
		update_timestamp(&s->m_loop->m_event_msec, s->m_lastactive_msec + s->close_timeout_msec);
	}
	else if (SESSION_TRANSPORT_CLIENT == s->transport_side || SESSION_TRANSPORT_SERVER == s->transport_side) {
		if (ESTABLISHED_STATUS != s->reliable.m_status)
			return;
		s->reliable.m_status = ESTABLISHED_FIN_STATUS;
		reliable_dgram_check_send_fin_packet(s, timestamp_msec);
		s->m_heartbeat_msec = 0;
	}
}

static void reliable_dgram_send_packet(Session_t* s, Packet_t* packet, long long timestamp_msec) {
	listInsertNodeBack(&s->m_sendpacketlist, s->m_sendpacketlist.tail, &packet->msg.m_listnode);
	if (packet->seq >= s->reliable.m_cwndseq &&
		packet->seq - s->reliable.m_cwndseq < s->reliable.cwndsize)
	{
		socketWrite(s->fd, packet->data, packet->len, 0, &s->reliable.peer_saddr, sockaddrLength(&s->reliable.peer_saddr));
		packet->resend_timestamp_msec = timestamp_msec + s->reliable.rto;
		update_timestamp(&s->m_loop->m_event_msec, packet->resend_timestamp_msec);
	}
}

static void reliable_dgram_do_reconnect(Session_t* s) {
	unsigned char reconnect_pkg[9];
	reconnect_pkg[0] = HDR_RECONNECT;
	*(unsigned int*)(reconnect_pkg + 1) = htonl(s->reliable.m_recvseq);
	*(unsigned int*)(reconnect_pkg + 5) = htonl(s->reliable.m_cwndseq);
	reliable_dgram_inner_packet_send(s, reconnect_pkg, sizeof(reconnect_pkg), &s->reliable.peer_saddr);
}

static int reliable_dgram_recv_handler(Session_t* s, unsigned char* buffer, int len, const struct sockaddr_storage* saddr, long long timestamp_msec) {
	unsigned char hdr_type = buffer[0] & (~HDR_DATA_END_FLAG);
	if (HDR_SYN == hdr_type) {
		unsigned char syn_ack[3];
		if (s->m_shutdownflag)
			return 1;
		else if (LISTENED_STATUS == s->reliable.m_status) {
			ReliableDgramHalfConnectPacket_t* halfcon = NULL;
			ListNode_t* cur, *next;
			for (cur = s->m_recvpacketlist.head; cur; cur = next) {
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
				halfcon->timestamp_msec = timestamp_msec + s->reliable.rto;

				listInsertNodeBack(&s->m_recvpacketlist, s->m_recvpacketlist.tail, &halfcon->m_listnode);
				update_timestamp(&s->m_loop->m_event_msec, halfcon->timestamp_msec);

				syn_ack[0] = HDR_SYN_ACK;
				*(unsigned short*)(syn_ack + 1) = htons(local_port);
			}
			reliable_dgram_inner_packet_send(s, syn_ack, sizeof(syn_ack), saddr);
			s->m_lastactive_msec = timestamp_msec;
		}
		else if (SYN_RCVD_STATUS == s->reliable.m_status) {
			if (AF_UNSPEC == s->reliable.peer_saddr.ss_family) {
				s->reliable.peer_saddr = *saddr;
				s->reliable.m_synrcvd_msec = timestamp_msec + s->reliable.rto;
				update_timestamp(&s->m_loop->m_event_msec, s->reliable.m_synrcvd_msec);
			}
			else if (memcmp(&s->reliable.peer_saddr, saddr, sizeof(*saddr)))
				return 1;
			syn_ack[0] = HDR_SYN_ACK;
			reliable_dgram_inner_packet_send(s, syn_ack, 1, saddr);
			s->m_lastactive_msec = timestamp_msec;
		}
	}
	else if (HDR_SYN_ACK_ACK == hdr_type) {
		ListNode_t* cur, *next;
		if (s->m_shutdownflag)
			return 1;
		else if (LISTENED_STATUS == s->reliable.m_status) {
			struct sockaddr_storage peer_addr;
			for (cur = s->m_recvpacketlist.head; cur; cur = next) {
				ReliableDgramHalfConnectPacket_t* halfcon = pod_container_of(cur, ReliableDgramHalfConnectPacket_t, m_listnode);
				next = cur->next;
				if (memcmp(&halfcon->peer_addr, saddr, sizeof(halfcon->peer_addr)))
					continue;
				if (socketRead(halfcon->sockfd, NULL, 0, 0, &peer_addr))
					break;
				listRemoveNode(&s->m_recvpacketlist, cur);
				s->accept(s, halfcon->sockfd, &peer_addr);
				free(halfcon);
				break;
			}
			s->m_lastactive_msec = timestamp_msec;
		}
		else if (SYN_RCVD_STATUS == s->reliable.m_status) {
			if (memcmp(&s->reliable.peer_saddr, saddr, sizeof(*saddr)))
				return 1;
			s->m_lastactive_msec = timestamp_msec;
			s->m_heartbeat_msec = timestamp_msec;
			if (s->heartbeat_timeout_sec > 0) {
				update_timestamp(&s->m_loop->m_event_msec, s->m_heartbeat_msec + s->heartbeat_timeout_sec * 1000);
			}
			s->reliable.m_status = ESTABLISHED_STATUS;
		}
	}
	else if (HDR_SYN_ACK == hdr_type) {
		unsigned char syn_ack_ack;
		if (len < 3)
			return 1;
		if (memcmp(saddr, &s->peer_listen_saddr, sizeof(s->peer_listen_saddr)))
			return 1;
		if (SYN_SENT_STATUS != s->reliable.m_status &&
			ESTABLISHED_STATUS != s->reliable.m_status)
		{
			return 1;
		}
		syn_ack_ack = HDR_SYN_ACK_ACK;
		reliable_dgram_inner_packet_send(s, &syn_ack_ack, sizeof(syn_ack_ack), saddr);
		if (SYN_SENT_STATUS == s->reliable.m_status) {
			if (len >= 3) {
				unsigned short peer_port;
				peer_port = *(unsigned short*)(buffer + 1);
				peer_port = ntohs(peer_port);
				sockaddrSetPort(&s->reliable.peer_saddr, peer_port);
			}
			s->reliable.m_status = ESTABLISHED_STATUS;
			s->connect(s, 0, s->m_connect_times++, 0, 0);
			s->m_heartbeat_msec = timestamp_msec;
			if (s->heartbeat_timeout_sec > 0) {
				update_timestamp(&s->m_loop->m_event_msec, s->m_heartbeat_msec + s->heartbeat_timeout_sec * 1000);
			}
		}
		socketWrite(s->fd, NULL, 0, 0, &s->reliable.peer_saddr, sockaddrLength(&s->reliable.peer_saddr));
		s->m_lastactive_msec = timestamp_msec;
	}
	else if (HDR_RECONNECT == hdr_type) {
		unsigned int peer_recvseq, peer_cwndseq;
		if (len < 9)
			return 1;
		if (SESSION_TRANSPORT_SERVER != s->transport_side || ESTABLISHED_STATUS != s->reliable.m_status)
			return 1;
		peer_recvseq = ntohl(*(unsigned int*)(buffer + 1));
		peer_cwndseq = ntohl(*(unsigned int*)(buffer + 5));
		if (session_grab_transport_check(s, peer_recvseq, peer_cwndseq)) {
			unsigned char reconnect_ack = HDR_RECONNECT_ACK;
			reliable_dgram_inner_packet_send(s, &reconnect_ack, sizeof(reconnect_ack), saddr);
			s->reliable.peer_saddr = *saddr;
			s->m_lastactive_msec = timestamp_msec;
			reliable_dgram_send_again(s, timestamp_msec);
		}
		else {
			unsigned char reconnect_err = HDR_RECONNECT_ERR;
			reliable_dgram_inner_packet_send(s, &reconnect_err, sizeof(reconnect_err), saddr);
		}
	}
	else if (HDR_RECONNECT_ACK == hdr_type) {
		if (SESSION_TRANSPORT_CLIENT != s->transport_side || RECONNECT_STATUS != s->reliable.m_status)
			return 1;
		if (memcmp(&s->reliable.peer_saddr, saddr, sizeof(*saddr)))
			return 1;
		s->reliable.m_status = ESTABLISHED_STATUS;
		_xchg16(&s->m_shutdownflag, 0);
		s->m_lastactive_msec = timestamp_msec;
		reliable_dgram_send_again(s, timestamp_msec);
		s->connect(s, 0, s->m_connect_times++, s->reliable.m_recvseq, s->reliable.m_cwndseq);
	}
	else if (HDR_RECONNECT_ERR == hdr_type) {
		if (SESSION_TRANSPORT_CLIENT != s->transport_side || RECONNECT_STATUS != s->reliable.m_status)
			return 1;
		if (memcmp(&s->reliable.peer_saddr, saddr, sizeof(*saddr)))
			return 1;
		s->m_valid = 0;
		s->reliable.m_status = TIME_WAIT_STATUS;
		s->connect(s, ECONNREFUSED, s->m_connect_times++, s->reliable.m_recvseq, s->reliable.m_cwndseq);
	}
	else if (HDR_FIN == hdr_type) {
		unsigned char fin_ack;
		if (memcmp(saddr, &s->reliable.peer_saddr, sizeof(*saddr)))
			return 1;
		else if (ESTABLISHED_STATUS == s->reliable.m_status) {
			fin_ack = HDR_FIN_ACK;
			reliable_dgram_inner_packet_send(s, &fin_ack, sizeof(fin_ack), &s->reliable.peer_saddr);
			s->reliable.m_status = CLOSE_WAIT_STATUS;
			s->m_lastactive_msec = timestamp_msec;
			s->m_heartbeat_msec = 0;
			reliable_dgram_check_send_fin_packet(s, timestamp_msec);
			_xchg16(&s->m_shutdownflag, 1);
			if (s->shutdown) {
				s->shutdown(s);
				s->shutdown = NULL;
			}
		}
		else if (FIN_WAIT_1_STATUS == s->reliable.m_status ||
				FIN_WAIT_2_STATUS == s->reliable.m_status)
		{
			fin_ack = HDR_FIN_ACK;
			reliable_dgram_inner_packet_send(s, &fin_ack, sizeof(fin_ack), &s->reliable.peer_saddr);
			s->reliable.m_status = TIME_WAIT_STATUS;
			s->m_lastactive_msec = timestamp_msec;
			s->m_valid = 0;
			_xchg16(&s->m_shutdownflag, 1);
			if (s->shutdown) {
				s->shutdown(s);
				s->shutdown = NULL;
			}
		}
	}
	else if (HDR_FIN_ACK == hdr_type) {
		if (memcmp(saddr, &s->reliable.peer_saddr, sizeof(*saddr)))
			return 1;
		else if (LAST_ACK_STATUS == s->reliable.m_status) {
			s->reliable.m_status = CLOSED_STATUS;
			s->m_lastactive_msec = timestamp_msec;
			s->m_valid = 0;
		}
		else if (FIN_WAIT_1_STATUS == s->reliable.m_status) {
			s->reliable.m_status = FIN_WAIT_2_STATUS;
			s->m_lastactive_msec = timestamp_msec;
		}
	}
	else if (HDR_ACK == hdr_type) {
		ListNode_t* cur;
		unsigned int seq, cwnd_skip, ack_valid;
		if (len < RELIABLE_DGRAM_DATA_HDR_LEN)
			return 1;
		if (ESTABLISHED_STATUS > s->reliable.m_status)
			return 1;
		if (memcmp(saddr, &s->reliable.peer_saddr, sizeof(*saddr)))
			return 1;

		s->m_lastactive_msec = timestamp_msec;
		s->m_heartbeat_msec = timestamp_msec;
		seq = *(unsigned int*)(buffer + 1);
		seq = ntohl(seq);
		cwnd_skip = 0;
		ack_valid = 0;

		for (cur = s->m_sendpacketlist.head; cur; cur = cur->next) {
			Packet_t* packet = pod_container_of(cur, Packet_t, msg.m_listnode);
			if (seq1_before_seq2(seq, packet->seq))
				break;
			if (seq == packet->seq) {
				ListNode_t* next = cur->next;
				listRemoveNode(&s->m_sendpacketlist, cur);
				free(packet);
				if (seq == s->reliable.m_cwndseq) {
					if (next) {
						packet = pod_container_of(next, Packet_t, msg.m_listnode);
						s->reliable.m_cwndseq = packet->seq;
						cwnd_skip = 1;
					}
					else
						++s->reliable.m_cwndseq;
				}
				ack_valid = 1;
				break;
			}
		}
		if (cwnd_skip) {
			for (cur = s->m_sendpacketlist.head; cur; cur = cur->next) {
				Packet_t* packet = pod_container_of(cur, Packet_t, msg.m_listnode);
				if (packet->seq < s->reliable.m_cwndseq ||
					packet->seq - s->reliable.m_cwndseq >= s->reliable.cwndsize)
				{
					break;
				}
				socketWrite(s->fd, packet->data, packet->len, 0, &s->reliable.peer_saddr, sockaddrLength(&s->reliable.peer_saddr));
				packet->resend_timestamp_msec = timestamp_msec + s->reliable.rto;
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
		if (ESTABLISHED_STATUS > s->reliable.m_status)
			return 1;
		if (memcmp(saddr, &s->reliable.peer_saddr, sizeof(*saddr)))
			return 1;

		s->m_lastactive_msec = timestamp_msec;
		seq = *(unsigned int*)(buffer + 1);
		ack[0] = HDR_ACK;
		*(unsigned int*)(ack + 1) = seq;
		reliable_dgram_inner_packet_send(s, ack, sizeof(ack), saddr);

		seq = ntohl(seq);
		if (seq1_before_seq2(seq, s->reliable.m_recvseq))
			return 1;
		else if (seq == s->reliable.m_recvseq) {
			s->reliable.m_recvseq++;
			reliable_dgram_packet_merge(s, buffer, len, saddr);
			for (cur = s->m_recvpacketlist.head; cur; cur = next) {
				packet = pod_container_of(cur, Packet_t, msg.m_listnode);
				if (packet->seq != s->reliable.m_recvseq)
					break;
				next = cur->next;
				s->reliable.m_recvseq++;
				reliable_dgram_packet_merge(s, packet->data, packet->len, saddr);
				listRemoveNode(&s->m_recvpacketlist, cur);
				free(packet);
			}
		}
		else {
			for (cur = s->m_recvpacketlist.head; cur; cur = cur->next) {
				packet = pod_container_of(cur, Packet_t, msg.m_listnode);
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
				listInsertNodeFront(&s->m_recvpacketlist, cur, &packet->msg.m_listnode);
			else
				listInsertNodeBack(&s->m_recvpacketlist, s->m_recvpacketlist.tail, &packet->msg.m_listnode);
		}
	}
	return 1;
}

static void reliable_dgram_update(SessionLoop_t* loop, Session_t* s, long long timestamp_msec) {
	if (LISTENED_STATUS == s->reliable.m_status) {
		ListNode_t* cur, *next;
		for (cur = s->m_recvpacketlist.head; cur; cur = next) {
			ReliableDgramHalfConnectPacket_t* halfcon = pod_container_of(cur, ReliableDgramHalfConnectPacket_t, m_listnode);
			next = cur->next;
			if (halfcon->timestamp_msec > timestamp_msec) {
				update_timestamp(&loop->m_event_msec, halfcon->timestamp_msec);
			}
			else if (halfcon->resend_times >= s->reliable.resend_maxtimes) {
				socketClose(halfcon->sockfd);
				listRemoveNode(&s->m_recvpacketlist, cur);
				free(halfcon);
			}
			else {
				unsigned char syn_ack[3];
				syn_ack[0] = HDR_SYN_ACK;
				*(unsigned short*)(syn_ack + 1) = htons(halfcon->local_port);
				reliable_dgram_inner_packet_send(s, syn_ack, sizeof(syn_ack), &halfcon->peer_addr);
				++halfcon->resend_times;
				halfcon->timestamp_msec = timestamp_msec + s->reliable.rto;
				update_timestamp(&loop->m_event_msec, halfcon->timestamp_msec);
			}
		}
	}
	else if (SYN_RCVD_STATUS == s->reliable.m_status) {
		if (AF_UNSPEC == s->reliable.peer_saddr.ss_family || s->m_shutdownflag) {
			return;
		}
		else if (s->reliable.m_synrcvd_msec > timestamp_msec) {
			update_timestamp(&loop->m_event_msec, s->reliable.m_synrcvd_msec);
		}
		else if (s->reliable.m_synrcvd_times >= s->reliable.resend_maxtimes) {
			s->reliable.m_synrcvd_times = 0;
			s->reliable.peer_saddr.ss_family = AF_UNSPEC;
		}
		else {
			unsigned char syn_ack = HDR_SYN_ACK;
			reliable_dgram_inner_packet_send(s, &syn_ack, 1, &s->reliable.peer_saddr);
			++s->reliable.m_synrcvd_times;
			s->reliable.m_synrcvd_msec = timestamp_msec + s->reliable.rto;
			update_timestamp(&loop->m_event_msec, s->reliable.m_synrcvd_msec);
		}
	}
	else if (SYN_SENT_STATUS == s->reliable.m_status) {
		if (s->reliable.m_synsent_msec > timestamp_msec) {
			update_timestamp(&loop->m_event_msec, s->reliable.m_synsent_msec);
		}
		else if (s->reliable.m_synsent_times >= s->reliable.resend_maxtimes) {
			s->reliable.m_status = TIME_WAIT_STATUS;
			s->m_lastactive_msec = timestamp_msec;
			s->m_valid = 0;
			s->shutdown = NULL;
			s->connect(s, ETIMEDOUT, s->m_connect_times++, 0, 0);
		}
		else {
			unsigned char syn = HDR_SYN;
			reliable_dgram_inner_packet_send(s, &syn, 1, &s->peer_listen_saddr);
			++s->reliable.m_synsent_times;
			s->reliable.m_synsent_msec = timestamp_msec + s->reliable.rto;
			update_timestamp(&loop->m_event_msec, s->reliable.m_synsent_msec);
		}
	}
	else if (FIN_WAIT_1_STATUS == s->reliable.m_status || LAST_ACK_STATUS == s->reliable.m_status) {
		if (s->reliable.m_fin_msec > timestamp_msec) {
			update_timestamp(&loop->m_event_msec, s->reliable.m_fin_msec);
		}
		else if (s->reliable.m_fin_times >= s->reliable.resend_maxtimes) {
			s->m_lastactive_msec = timestamp_msec;
			s->m_valid = 0;
		}
		else {
			unsigned char fin = HDR_FIN;
			reliable_dgram_inner_packet_send(s, &fin, 1, &s->reliable.peer_saddr);
			++s->reliable.m_fin_times;
			s->reliable.m_fin_msec = timestamp_msec + s->reliable.rto;
			update_timestamp(&loop->m_event_msec, s->reliable.m_fin_msec);
		}
	}
	else if (RECONNECT_STATUS == s->reliable.m_status) {
		if (s->reliable.m_reconnect_msec > timestamp_msec) {
			update_timestamp(&loop->m_event_msec, s->reliable.m_reconnect_msec);
		}
		else if (s->reliable.m_reconnect_times >= s->reliable.resend_maxtimes) {
			s->reliable.m_status = TIME_WAIT_STATUS;
			s->m_lastactive_msec = timestamp_msec;
			s->m_valid = 0;
			s->connect(s, ETIMEDOUT, s->m_connect_times++, s->reliable.m_recvseq, s->reliable.m_cwndseq);
		}
		else {
			reliable_dgram_do_reconnect(s);
			++s->reliable.m_reconnect_times;
			s->reliable.m_reconnect_msec = timestamp_msec + s->reliable.rto;
			update_timestamp(&loop->m_event_msec, s->reliable.m_reconnect_msec);
		}
	}
	else if (ESTABLISHED_STATUS == s->reliable.m_status ||
			ESTABLISHED_FIN_STATUS == s->reliable.m_status ||
			CLOSE_WAIT_STATUS == s->reliable.m_status)
	{
		ListNode_t* cur;
		for (cur = s->m_sendpacketlist.head; cur; cur = cur->next) {
			Packet_t* packet = pod_container_of(cur, Packet_t, msg.m_listnode);
			if (packet->seq < s->reliable.m_cwndseq ||
				packet->seq - s->reliable.m_cwndseq >= s->reliable.cwndsize)
			{
				break;
			}
			if (packet->resend_timestamp_msec > timestamp_msec) {
				update_timestamp(&loop->m_event_msec, packet->resend_timestamp_msec);
				continue;
			}
			if (packet->resendtimes >= s->reliable.resend_maxtimes) {
				s->m_lastactive_msec = timestamp_msec;
				if (ESTABLISHED_STATUS != s->reliable.m_status) {
					s->m_valid = 0;
				}
				break;
			}
			socketWrite(s->fd, packet->data, packet->len, 0, &s->reliable.peer_saddr, sockaddrLength(&s->reliable.peer_saddr));
			packet->resendtimes++;
			packet->resend_timestamp_msec = timestamp_msec + s->reliable.rto;
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
			s->m_lastactive_msec = timestamp_msec;
		}
		else {
			unsigned char *ptr;
			int res = socketTcpReadableBytes(s->fd);
			if (res <= 0) {
				s->m_valid = 0;
				if (0 == res) {
					s->reliable.m_status = CLOSE_WAIT_STATUS;
					if (s->shutdown) {
						s->shutdown(s);
						s->shutdown = NULL;
					}
				}
				return;
			}
			ptr = (unsigned char*)realloc(s->m_inbuf, s->m_inbuflen + res);
			if (!ptr) {
				s->m_valid = 0;
				return;
			}
			s->m_inbuf = ptr;
			res = socketRead(s->fd, s->m_inbuf + s->m_inbuflen, res, 0, &saddr);
			if (res < 0) {
				if (errnoGet() != EWOULDBLOCK) {
					s->m_valid = 0;
				}
				return;
			}
			else if (res == 0) {
				s->m_valid = 0;
				s->reliable.m_status = CLOSE_WAIT_STATUS;
				if (s->shutdown) {
					s->shutdown(s);
					s->shutdown = NULL;
				}
				return;
			}
			else {
				int(*handler)(Session_t*, unsigned char*, int, const struct sockaddr_storage*);
				int decodelen;
				s->m_inbuflen += res;
				s->m_lastactive_msec = timestamp_msec;
				s->m_heartbeat_msec = timestamp_msec;
				if (s->reliable.enable)
					handler = reliable_stream_data_packet_handler;
				else
					handler = data_packet_handler;
				decodelen = handler(s, s->m_inbuf + s->m_inbufoffset, s->m_inbuflen - s->m_inbufoffset, &saddr);
				if (decodelen < 0) {
					s->m_inbuflen = s->m_inbufoffset;
				}
				else {
					s->m_inbufoffset += decodelen;
					if (s->m_inbufoffset >= s->m_inbuflen) {
						free_inbuf(s);
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
					if (s->reliable.enable) {
						s->m_lastactive_msec = timestamp_msec;
						update_timestamp(&s->m_loop->m_event_msec, s->m_lastactive_msec + s->close_timeout_msec);
					}
				}
				break;
			}
			else if (s->reliable.enable) {
				SessionDecodeResult_t decode_result;
				if (TIME_WAIT_STATUS == s->reliable.m_status ||
					CLOSED_STATUS == s->reliable.m_status ||
					NO_STATUS == s->reliable.m_status)
				{
					break;
				}
				if (0 == res)
					continue;
				s->decode(p_data, res, reset_decode_result(&decode_result));
				if (decode_result.err || decode_result.incomplete)
					continue;
				if (!reliable_dgram_recv_handler(s, decode_result.bodyptr, decode_result.bodylen, &saddr, timestamp_msec))
					break;
			}
			else {
				data_packet_handler(s, p_data, res, &saddr);
				s->m_lastactive_msec = timestamp_msec;
			}
		}
	}
}

static void reactor_socket_do_write(Session_t* s, long long timestamp_msec) {
	if (SOCK_STREAM != s->socktype || !s->m_valid)
		return;
	s->m_writeol_has_commit = 0;
	if (SYN_SENT_STATUS == s->reliable.m_status) {
		int err;
		if (s->m_writeol) {
			nioFreeOverlapped(s->m_writeol);
			s->m_writeol = NULL;
		}
		if (!nioConnectCheckSuccess(s->fd)) {
			err = errnoGet();
			s->m_valid = 0;
			s->reliable.m_status = TIME_WAIT_STATUS;
			s->shutdown = NULL;
		}
		else if (!reactorsocket_read(s)) {
			err = errnoGet();
			s->m_valid = 0;
			s->shutdown = NULL;
		}
		else {
			err = 0;
			s->m_lastactive_msec = timestamp_msec;
			if (s->m_connect_times) {
				if (s->reliable.enable)
					s->reliable.m_status = RECONNECT_STATUS;
				else
					s->reliable.m_status = ESTABLISHED_STATUS;
				_xchg16(&s->m_shutdownflag, 0);
			}
			else {
				s->reliable.m_status = ESTABLISHED_STATUS;
			}
			if (ESTABLISHED_STATUS == s->reliable.m_status) {
				s->m_heartbeat_msec = timestamp_msec;
				if (s->heartbeat_timeout_sec > 0)
					update_timestamp(&s->m_loop->m_event_msec, s->m_heartbeat_msec + s->heartbeat_timeout_sec * 1000);
			}
		}
		s->connect(s, err, s->m_connect_times++, s->reliable.m_recvseq, s->reliable.m_cwndseq);
	}
	else if (ESTABLISHED_STATUS == s->reliable.m_status ||
		RECONNECT_STATUS == s->reliable.m_status)
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
	if (SOCK_STREAM != s->socktype && !s->reliable.enable) {
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
			if (SOCK_STREAM == s->socktype && !s->reliable.enable && (!s->hdrlen || !s->hdrlen(0))) {
				return s;
			}
			iovcnt = 0;
		}
	}
	if (s->reliable.enable) {
		if (SOCK_STREAM == s->socktype) {
			size_t hdrlen = s->hdrlen ? s->hdrlen(RELIABLE_STREAM_DATA_HDR_LEN + nbytes) : 0;
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
			if (hdrlen && s->encode) {
				s->encode(packet->data, RELIABLE_STREAM_DATA_HDR_LEN + nbytes);
			}
			for (nbytes = 0, i = 0; i < iovcnt; ++i) {
				memcpy(packet->data + hdrlen + RELIABLE_STREAM_DATA_HDR_LEN + nbytes, iobufPtr(iov + i), iobufLen(iov + i));
				nbytes += iobufLen(iov + i);
			}
			sessionloop_exec_msg(s->m_loop, &packet->msg.m_listnode);
		}
		else if (ESTABLISHED_STATUS == s->reliable.m_status) {
			Packet_t* packet;
			size_t hdrlen;
			if (nbytes) {
				unsigned int offset, packetlen, copy_off, i_off;
				List_t packetlist;
				listInit(&packetlist);
				for (i = i_off = offset = 0; offset < nbytes; offset += packetlen) {
					packetlen = nbytes - offset;
					hdrlen = s->hdrlen ? s->hdrlen(RELIABLE_DGRAM_DATA_HDR_LEN + packetlen) : 0;
					if (hdrlen + RELIABLE_DGRAM_DATA_HDR_LEN + packetlen > s->reliable.mtu) {
						packetlen = s->reliable.mtu - hdrlen - RELIABLE_DGRAM_DATA_HDR_LEN;
						hdrlen = s->hdrlen ? s->hdrlen(RELIABLE_DGRAM_DATA_HDR_LEN + packetlen) : 0;
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
					if (hdrlen && s->encode) {
						s->encode(packet->data, RELIABLE_DGRAM_DATA_HDR_LEN + packetlen);
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
					listInsertNodeBack(&packetlist, packetlist.tail, &packet->msg.m_listnode);
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
				hdrlen = s->hdrlen ? s->hdrlen(RELIABLE_DGRAM_DATA_HDR_LEN) : 0;
				packet = (Packet_t*)malloc(sizeof(Packet_t) + hdrlen + RELIABLE_DGRAM_DATA_HDR_LEN);
				if (!packet)
					return NULL;
				packet->msg.type = SESSION_PACKET_MESSAGE;
				packet->s = s;
				packet->resendtimes = 0;
				packet->hdrlen = hdrlen;
				packet->len = hdrlen + RELIABLE_DGRAM_DATA_HDR_LEN;
				packet->data[hdrlen] = HDR_DATA | HDR_DATA_END_FLAG;
				if (hdrlen && s->encode) {
					s->encode(packet->data, RELIABLE_DGRAM_DATA_HDR_LEN);
				}
				sessionloop_exec_msg(s->m_loop, &packet->msg.m_listnode);
			}
		}
	}
	else {
		size_t hdrlen = s->hdrlen ? s->hdrlen(nbytes) : 0;
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
		if (hdrlen && s->encode) {
			s->encode(packet->data, nbytes);
		}
		for (nbytes = 0, i = 0; i < iovcnt; ++i) {
			memcpy(packet->data + hdrlen + nbytes, iobufPtr(iov + i), iobufLen(iov + i));
			nbytes += iobufLen(iov + i);
		}
		sessionloop_exec_msg(s->m_loop, &packet->msg.m_listnode);
	}
	return s;
}

void sessionClientNetReconnect(Session_t* s) {
	if (SESSION_TRANSPORT_CLIENT != s->transport_side ||
		(SOCK_DGRAM == s->socktype && !s->reliable.enable))
	{
		return;
	}
	else if (_xchg16(&s->m_shutdownflag, 1))
		return;
	sessionloop_exec_msg(s->m_loop, &s->m_netreconnectmsg.m_listnode);
}

void sessionReconnectRecovery(Session_t* s) {
	if (!s->reliable.enable || SOCK_DGRAM == s->socktype)
		return;
	else if (SESSION_TRANSPORT_CLIENT == s->transport_side ||
			SESSION_TRANSPORT_SERVER == s->transport_side)
	{
		if (_xchg16(&s->m_reconnectrecovery, 1))
			return;
		sessionloop_exec_msg(s->m_loop, &s->m_reconnectrecoverymsg.m_listnode);
	}
}

int sessionTransportGrab(Session_t* s, Session_t* target_s, unsigned int recvseq, unsigned int cwndseq) {
	if (threadEqual(s->m_loop->m_runthread, threadSelf()) &&
		threadEqual(target_s->m_loop->m_runthread, threadSelf()))
	{
		SessionTransportGrabMsg_t ts;
		if (!session_grab_transport_check(target_s, recvseq, cwndseq))
			return 0;
		session_grab_transport(target_s, &ts.target_status);
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
			if (!session_grab_transport_check(target_s, recvseq, cwndseq))
				return 0;
			session_grab_transport(target_s, &ts->target_status);
		}
		else {
			mutexLock(&ts->blocklock);
			ts->msg.type = SESSION_TRANSPORT_GRAB_ASYNC_REQ_MESSAGE;
			ts->s = target_s;
			ts->recvseq = recvseq;
			ts->cwndseq = cwndseq;
			sessionloop_exec_msg(target_s->m_loop, &ts->msg.m_listnode);
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
		sessionloop_exec_msg(s->m_loop, &ts->msg.m_listnode);
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
	sessionloop_exec_msg(s->m_loop, &s->m_shutdownpostmsg.m_listnode);
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
	s->heartbeat_timeout_sec = 0;
	s->keepalive_timeout_sec = 0;
	s->close_timeout_msec = 0;
	s->transport_side = transport_side;
	s->sessionid_len = 0;
	s->sessionid = NULL;
	s->userdata = NULL;
	s->local_listen_saddr.ss_family = AF_UNSPEC;
	s->peer_listen_saddr.ss_family = AF_UNSPEC;
	s->reg_or_connect = NULL;
	s->zombie = NULL;
	s->accept = NULL;
	s->decode = NULL;
	s->recv = NULL;
	s->hdrlen = NULL;
	s->encode = NULL;
	s->send_heartbeat_to_server = NULL;
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
	s->m_lastactive_msec = 0;
	s->m_heartbeat_msec = 0;
	s->m_inbuf = NULL;
	s->m_inbufoffset = 0;
	s->m_inbuflen = 0;
	s->m_recvpacket_maxcnt = 8;
	listInit(&s->m_recvpacketlist);
	listInit(&s->m_sendpacketlist);
	listInit(&s->m_sendpacketlist_bak);

	s->reliable.peer_saddr.ss_family = AF_UNSPEC;
	s->reliable.mtu = 1464;
	s->reliable.rto = 200;
	s->reliable.resend_maxtimes = 5;
	s->reliable.cwndsize = 10;
	s->reliable.enable = 0;
	s->reliable.m_status = NO_STATUS;
	s->reliable.m_synrcvd_times = 0;
	s->reliable.m_synsent_times = 0;
	s->reliable.m_reconnect_times = 0;
	s->reliable.m_fin_times = 0;
	s->reliable.m_synrcvd_msec = 0;
	s->reliable.m_synsent_msec = 0;
	s->reliable.m_reconnect_msec = 0;
	s->reliable.m_fin_msec = 0;
	s->reliable.m_cwndseq = 0;
	s->reliable.m_recvseq = 0;
	s->reliable.m_sendseq = 0;
	s->reliable.m_cwndseq_bak = 0;
	s->reliable.m_recvseq_bak = 0;
	s->reliable.m_sendseq_bak = 0;
	return s;
}

static void session_free(Session_t* s) {
	free_io_resource(s);
	free_inbuf(s);
	if (SESSION_TRANSPORT_LISTEN == s->transport_side &&
		SOCK_DGRAM == s->socktype && s->reliable.enable)
	{
		ListNode_t* cur;
		for (cur = s->m_recvpacketlist.head; cur; cur = cur->next) {
			ReliableDgramHalfConnectPacket_t* halfcon = pod_container_of(cur, ReliableDgramHalfConnectPacket_t, m_listnode);
			socketClose(halfcon->sockfd);
		}
	}
	clear_packetlist(&s->m_recvpacketlist);
	clear_packetlist(&s->m_sendpacketlist);
	clear_packetlist(&s->m_sendpacketlist_bak);
	if (s->free)
		s->free(s);
	else
		free(s);
}

void sessionManualClose(Session_t* s) {
	if (s->m_loop) {
		sessionloop_exec_msg(s->m_loop, &s->m_closemsg.m_listnode);
	}
	else {
		session_free(s);
	}
}

static int sockht_keycmp(const void* node_key, const void* key) {
	return *(FD_t*)node_key != *(FD_t*)key;
}

static unsigned int sockht_keyhash(const void* key) { return *(FD_t*)key; }

static void sockcloselist_update(SessionLoop_t* loop, long long timestamp_msec) {
	ListNode_t* cur, *next;
	List_t expirelist;
	listInit(&expirelist);
	for (cur = loop->m_sockcloselist.head; cur; cur = next) {
		Session_t* s = pod_container_of(cur, Session_t, m_closemsg.m_listnode);
		next = cur->next;
		if (s->m_lastactive_msec + s->close_timeout_msec > timestamp_msec) {
			update_timestamp(&loop->m_event_msec, s->m_lastactive_msec + s->close_timeout_msec);
			continue;
		}
		s->reliable.m_status = NO_STATUS;
		free_io_resource(s);
		free_inbuf(s);
		listRemoveNode(&loop->m_sockcloselist, cur);
		listInsertNodeBack(&expirelist, expirelist.tail, cur);
	}
	for (cur = expirelist.head; cur; cur = next) {
		Session_t* s = pod_container_of(cur, Session_t, m_closemsg.m_listnode);
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
			if (s->keepalive_timeout_sec > 0 && s->m_lastactive_msec + s->keepalive_timeout_sec * 1000 <= timestamp_msec) {
				if (ESTABLISHED_STATUS == s->reliable.m_status && s->zombie && s->zombie(s)) {
					s->m_lastactive_msec = timestamp_msec;
					continue;
				}
				s->m_valid = 0;
				free_inbuf(s);
				if (SOCK_STREAM == s->socktype || s->keepalive_timeout_sec * 1000 >= s->close_timeout_msec)
					free_io_resource(s);
			}
			else {
				if (SESSION_TRANSPORT_CLIENT == s->transport_side && s->send_heartbeat_to_server &&
					s->m_heartbeat_msec > 0 && s->heartbeat_timeout_sec > 0)
				{
					if (s->m_heartbeat_msec + s->heartbeat_timeout_sec * 1000 <= timestamp_msec) {
						s->m_heartbeat_msec = timestamp_msec;
						s->send_heartbeat_to_server(s);
					}
					update_timestamp(&loop->m_event_msec, s->m_heartbeat_msec + s->heartbeat_timeout_sec * 1000);
				}
				if (s->keepalive_timeout_sec > 0)
					update_timestamp(&loop->m_event_msec, s->m_lastactive_msec + s->keepalive_timeout_sec * 1000);
				if (SOCK_DGRAM == s->socktype && s->reliable.enable) {
					reliable_dgram_update(loop, s, timestamp_msec);
					if (s->m_valid)
						continue;
				}
				else
					continue;
			}
		}
		hashtableRemoveNode(&loop->m_sockht, cur);
		listInsertNodeBack(&loop->m_sockcloselist, loop->m_sockcloselist.tail, &s->m_closemsg.m_listnode);
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

	n = nioreactorWait(&loop->m_reactor, e, n, wait_msec);
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
			if (!nioEventOverlapped(e + i))
				continue;
			fd = nioEventFD(e + i);
			if (fd == loop->m_socketpair[0]) {
				struct sockaddr_storage saddr;
				char c[512];
				socketRead(fd, c, sizeof(c), 0, NULL);
				saddr.ss_family = AF_INET;
				nioreactorCommit(&loop->m_reactor, fd, NIO_OP_READ, loop->m_readol, (struct sockaddr*)&saddr, sockaddrLength((struct sockaddr*)&saddr));
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
			free_inbuf(s);
			if (SOCK_STREAM == s->socktype) {
				free_io_resource(s);
			}
			if (s->close_timeout_msec > 0) {
				s->m_lastactive_msec = timestamp_msec;
				update_timestamp(&loop->m_event_msec, s->m_lastactive_msec + s->close_timeout_msec);
				listInsertNodeBack(&loop->m_sockcloselist, loop->m_sockcloselist.tail, &s->m_closemsg.m_listnode);
			}
			else {
				s->reliable.m_status = NO_STATUS;
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
		SessionInternalMsg_t* message;
		next = cur->next;
		message = pod_container_of(cur, SessionInternalMsg_t, m_listnode);
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
						s->reliable.m_status = NO_STATUS;
						hashtableRemoveNode(&loop->m_sockht, &s->m_hashnode);
						if (s->close) {
							s->close(s);
							s->close = NULL;
						}
					}
				}
				else if (ESTABLISHED_STATUS == s->reliable.m_status) {
					s->reliable.m_status = ESTABLISHED_FIN_STATUS;
					s->m_heartbeat_msec = 0;
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
				(ESTABLISHED_STATUS != s->reliable.m_status && RECONNECT_STATUS != s->reliable.m_status))
			{
				free(packet);
				continue;
			}
			if (SOCK_STREAM == s->socktype) {
				if (s->reliable.enable) {
					if (ESTABLISHED_STATUS == s->reliable.m_status) {
						packet->seq = s->reliable.m_sendseq++;
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
				packet->seq = s->reliable.m_sendseq++;
				*(unsigned int*)(packet->data + packet->hdrlen + 1) = htonl(packet->seq);
				reliable_dgram_send_packet(s, packet, timestamp_msec);
			}
		}
		else if (SESSION_CLIENT_NET_RECONNECT_MESSAGE == message->type) {
			Session_t* s = pod_container_of(message, Session_t, m_netreconnectmsg);
			if (!s->m_valid || SESSION_TRANSPORT_CLIENT != s->transport_side)
				continue;
			if (ESTABLISHED_STATUS != s->reliable.m_status && RECONNECT_STATUS != s->reliable.m_status)
				continue;
			if (SOCK_STREAM == s->socktype) {
				int ok;
				hashtableRemoveNode(&loop->m_sockht, &s->m_hashnode);
				free_io_resource(s);
				free_inbuf(s);
				s->m_writeol_has_commit = 0;
				s->m_heartbeat_msec = 0;
				do {
					ok = 0;
					s->fd = socket(s->domain, s->socktype, s->protocol);
					if (INVALID_FD_HANDLE == s->fd)
						break;
					if (!nioreactorReg(&loop->m_reactor, s->fd))
						break;
					if (!s->m_writeol) {
						s->m_writeol = nioAllocOverlapped(NIO_OP_CONNECT, NULL, 0, 0);
						if (!s->m_writeol)
							break;
					}
					if (!nioreactorCommit(&loop->m_reactor, s->fd, NIO_OP_CONNECT, s->m_writeol,
						(struct sockaddr*)(&s->peer_listen_saddr), sockaddrLength((struct sockaddr*)(&s->peer_listen_saddr))))
					{
						break;
					}
					if (s->reliable.enable && ESTABLISHED_STATUS == s->reliable.m_status) {
						reliable_stream_bak(s);
					}
					else {
						clear_packetlist(&s->m_sendpacketlist);
					}
					s->m_writeol_has_commit = 1;
					s->reliable.m_status = SYN_SENT_STATUS;
					s->m_lastactive_msec = timestamp_msec;
					hashtableReplaceNode(hashtableInsertNode(&loop->m_sockht, &s->m_hashnode), &s->m_hashnode);
					ok = 1;
				} while (0);
				if (!ok) {
					free_io_resource(s);
					clear_packetlist(&s->m_sendpacketlist);
					s->m_valid = 0;
					s->connect(s, errnoGet(), s->m_connect_times++, s->reliable.m_recvseq, s->reliable.m_cwndseq);

					if (s->close_timeout_msec > 0) {
						s->m_lastactive_msec = timestamp_msec;
						update_timestamp(&loop->m_event_msec, s->m_lastactive_msec + s->close_timeout_msec);
						listInsertNodeBack(&loop->m_sockcloselist, loop->m_sockcloselist.tail, &s->m_closemsg.m_listnode);
					}
					else {
						s->reliable.m_status = NO_STATUS;
						if (s->close) {
							s->close(s);
							s->close = NULL;
						}
					}
				}
			}
			else if (ESTABLISHED_STATUS == s->reliable.m_status) {
				reliable_dgram_do_reconnect(s);
				s->m_lastactive_msec = timestamp_msec;
				s->m_heartbeat_msec = 0;
				s->reliable.m_status = RECONNECT_STATUS;
				s->reliable.m_reconnect_times = 0;
				s->reliable.m_reconnect_msec = timestamp_msec + s->reliable.rto;
				update_timestamp(&s->m_loop->m_event_msec, s->reliable.m_reconnect_msec);
			}
		}
		else if (SESSION_RECONNECT_RECOVERY_MESSAGE == message->type) {
			Session_t* s = pod_container_of(message, Session_t, m_reconnectrecoverymsg);
			_xchg16(&s->m_reconnectrecovery, 0);
			if (RECONNECT_STATUS != s->reliable.m_status)
				continue;
			s->reliable.m_status = ESTABLISHED_STATUS;
			if (SESSION_TRANSPORT_CLIENT == s->transport_side) {
				reliable_stream_bak_recovery(s);
			}
			stream_send_packet_continue(s);
		}
		else if (SESSION_TRANSPORT_GRAB_ASYNC_REQ_MESSAGE == message->type) {
			SessionTransportGrabMsg_t* ts = pod_container_of(message, SessionTransportGrabMsg_t, msg);
			Session_t* s = ts->s;
			if (session_grab_transport_check(s, ts->recvseq, ts->cwndseq)) {
				ts->success = 1;
				session_grab_transport(s, &ts->target_status);
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
				if (!nioreactorReg(&loop->m_reactor, s->fd))
					break;
				s->m_lastactive_msec = timestamp_msec;
				if (SOCK_STREAM == s->socktype) {
					if (SESSION_TRANSPORT_CLIENT == s->transport_side) {
						BOOL has_connected;
						s->reliable.peer_saddr = s->peer_listen_saddr;
						if (!socketIsConnected(s->fd, &has_connected))
							break;
						if (has_connected) {
							if (!reactorsocket_read(s))
								break;
							s->m_heartbeat_msec = timestamp_msec;
							s->reliable.m_status = ESTABLISHED_STATUS;
							immedinate_call = 1;
						}
						else {
							if (!s->m_writeol) {
								s->m_writeol = nioAllocOverlapped(NIO_OP_CONNECT, NULL, 0, 0);
								if (!s->m_writeol)
									break;
							}
							if (!nioreactorCommit(&loop->m_reactor, s->fd, NIO_OP_CONNECT, s->m_writeol,
								(struct sockaddr*)(&s->peer_listen_saddr), sockaddrLength((struct sockaddr*)(&s->peer_listen_saddr))))
							{
								break;
							}
							s->m_writeol_has_commit = 1;
							s->reliable.m_status = SYN_SENT_STATUS;
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
							s->reliable.m_status = LISTENED_STATUS;
						}
						else {
							s->reliable.m_status = ESTABLISHED_STATUS;
							s->m_heartbeat_msec = timestamp_msec;
						}
						if (!reactorsocket_read(s))
							break;
						immedinate_call = 1;
					}
				}
				else {
					if (s->reliable.enable) {
						if (SESSION_TRANSPORT_CLIENT == s->transport_side && s->peer_listen_saddr.ss_family != AF_UNSPEC) {
							unsigned char syn = HDR_SYN;
							reliable_dgram_inner_packet_send(s, &syn, 1, &s->peer_listen_saddr);
							s->reliable.m_status = SYN_SENT_STATUS;
							s->reliable.peer_saddr = s->peer_listen_saddr;
							s->reliable.m_synsent_msec = timestamp_msec + s->reliable.rto;
							update_timestamp(&loop->m_event_msec, s->reliable.m_synsent_msec);
						}
						else if (SESSION_TRANSPORT_LISTEN == s->transport_side) {
							if (AF_UNSPEC == s->local_listen_saddr.ss_family) {
								if (!socketGetLocalAddr(s->fd, &s->local_listen_saddr))
									break;
							}
							if (s->accept) {
								s->reliable.m_status = LISTENED_STATUS;
								if (s->m_recvpacket_maxcnt < 200)
									s->m_recvpacket_maxcnt = 200;
								immedinate_call = 1;
							}
							else {
								s->reliable.m_status = SYN_RCVD_STATUS;
							}
						}
						else {
							s->reliable.m_status = ESTABLISHED_STATUS;
							s->m_heartbeat_msec = timestamp_msec;
							immedinate_call = 1;
						}
						if (s->close_timeout_msec < MSL + MSL) {
							s->close_timeout_msec = MSL + MSL;
						}
					}
					else {
						s->reliable.m_status = ESTABLISHED_STATUS;
						immedinate_call = 1;
					}
					if (!reactorsocket_read(s))
						break;
				}
				hashtableReplaceNode(hashtableInsertNode(&loop->m_sockht, &s->m_hashnode), &s->m_hashnode);
				if (s->m_heartbeat_msec > 0 && s->heartbeat_timeout_sec > 0) {
					update_timestamp(&loop->m_event_msec, s->m_heartbeat_msec + s->heartbeat_timeout_sec * 1000);
				}
				if (s->keepalive_timeout_sec > 0) {
					update_timestamp(&loop->m_event_msec, s->m_lastactive_msec + s->keepalive_timeout_sec * 1000);
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
				s->reliable.m_status = NO_STATUS;
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
		sockcloselist_update(loop, timestamp_msec);
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

	if (!nioreactorCreate(&loop->m_reactor)) {
		nioFreeOverlapped(loop->m_readol);
		socketClose(loop->m_socketpair[0]);
		socketClose(loop->m_socketpair[1]);
		return NULL;
	}

	if (!socketNonBlock(loop->m_socketpair[0], TRUE) ||
		!socketNonBlock(loop->m_socketpair[1], TRUE) ||
		!nioreactorReg(&loop->m_reactor, loop->m_socketpair[0]) ||
		!nioreactorCommit(&loop->m_reactor, loop->m_socketpair[0], NIO_OP_READ, loop->m_readol,
			(struct sockaddr*)&saddr, sockaddrLength((struct sockaddr*)&saddr)))
	{
		nioFreeOverlapped(loop->m_readol);
		socketClose(loop->m_socketpair[0]);
		socketClose(loop->m_socketpair[1]);
		nioreactorClose(&loop->m_reactor);
		return NULL;
	}

	if (!criticalsectionCreate(&loop->m_msglistlock)) {
		nioFreeOverlapped(loop->m_readol);
		socketClose(loop->m_socketpair[0]);
		socketClose(loop->m_socketpair[1]);
		nioreactorClose(&loop->m_reactor);
		return NULL;
	}

	listInit(&loop->m_msglist);
	listInit(&loop->m_sockcloselist);
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
		listInsertNodeBack(&list, list.tail, &s[i]->m_regmsg.m_listnode);
	}
	sessionloop_exec_msglist(loop, &list);
}

void sessionloopDestroy(SessionLoop_t* loop) {
	if (loop && loop->m_initok) {
		nioFreeOverlapped(loop->m_readol);
		socketClose(loop->m_socketpair[0]);
		socketClose(loop->m_socketpair[1]);
		nioreactorClose(&loop->m_reactor);
		criticalsectionClose(&loop->m_msglistlock);
		do {
			ListNode_t* cur, *next;
			for (cur = loop->m_msglist.head; cur; cur = next) {
				SessionInternalMsg_t* msgbase = pod_container_of(cur, SessionInternalMsg_t, m_listnode);
				next = cur->next;
				if (SESSION_PACKET_MESSAGE == msgbase->type)
					free(cur);
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
