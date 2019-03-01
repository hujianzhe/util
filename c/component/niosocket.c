//
// Created by hujianzhe on 18-8-13.
//

#include "../syslib/error.h"
#include "niosocket.h"
#include <stdlib.h>
#include <string.h>

enum {
	NIO_SOCKET_USER_MESSAGE,
	NIO_SOCKET_CLOSE_MESSAGE,
	NIO_SOCKET_REG_MESSAGE,
	NIO_SOCKET_STREAM_WRITEABLE_MESSAGE,
	NIO_SOCKET_RELIABLE_MESSAGE,
	NIO_SOCKET_RELIABLE_ACK_MESSAGE
};

enum {
	HDR_DATA,
	HDR_ACK
};
#define	RELIABLE_HDR_LEN	5

typedef struct Packet_t {
	NioMsg_t msg;
	struct sockaddr_storage saddr;
	NioSocket_t* s;
	size_t offset;
	size_t len;
	unsigned char data[1];
} Packet_t;

typedef struct ReliableDataPacket_t {
	NioMsg_t msg;
	long long resend_timestamp_msec;
	unsigned int resendtimes;
	struct sockaddr_storage saddr;
	NioSocket_t* s;
	unsigned int seq;
	size_t len;
	unsigned char data[1];
} ReliableDataPacket_t;

typedef struct ReliableAckPacket_t {
	NioMsg_t msg;
	struct sockaddr_storage saddr;
	NioSocket_t* s;
	unsigned int seq;
} ReliableAckPacket_t;

#ifdef __cplusplus
extern "C" {
#endif

void niosocketFree(NioSocket_t* s) {
	ListNode_t *cur, *next;
	if (!s)
		return;

	socketClose(s->fd);
	reactorFreeOverlapped(s->m_readOl);
	reactorFreeOverlapped(s->m_writeOl);

	if (s->m_inbuf) {
		free(s->m_inbuf);
		s->m_inbuf = NULL;
		s->m_inbuflen = 0;
	}

	for (cur = s->m_recvpacketlist.head; cur; cur = next) {
		next = cur->next;
		free(pod_container_of(cur, Packet_t, msg.m_listnode));
	}
	for (cur = s->m_sendpacketlist.head; cur; cur = next) {
		next = cur->next;
		free(pod_container_of(cur, Packet_t, msg.m_listnode));
	}

	if (s->m_free)
		s->m_free(s);
}

static int reactorsocket_read(NioSocket_t* s) {
	struct sockaddr_storage saddr;
	int opcode;
	if (!s->valid)
		return 0;
	else if (s->accept_callback) {
		opcode = REACTOR_ACCEPT;
		saddr.ss_family = s->domain;
	}
	else {
		opcode = REACTOR_READ;
	}
	if (!s->m_readOl) {
		s->m_readOl = reactorMallocOverlapped(opcode);
		if (!s->m_readOl) {
			s->valid = 0;
			return 0;
		}
	}
	if (reactorCommit(&s->m_loop->m_reactor, s->fd, opcode, s->m_readOl, &saddr))
		return 1;
	s->valid = 0;
	return 0;
}

static int reactor_socket_reliable_read(NioSocket_t* s, unsigned char* buffer, int len, const struct sockaddr_storage* saddr) {
	unsigned char hdr_type = buffer[0];
	unsigned int seq = *(unsigned int*)(buffer + 1);
	if (HDR_ACK == hdr_type) {
		ReliableAckPacket_t* packet = (ReliableAckPacket_t*)malloc(sizeof(ReliableAckPacket_t));
		if (!packet) {
			s->valid = 0;
			return 0;
		}
		packet->msg.type = NIO_SOCKET_RELIABLE_ACK_MESSAGE;
		packet->saddr = *saddr;
		packet->s = s;
		packet->seq = ntohl(seq);
		dataqueuePush(&s->m_loop->m_sender->m_dq, &packet->msg.m_listnode);
	}
	else if (HDR_DATA == hdr_type) {
		ListNode_t* cur, *next;
		ReliableDataPacket_t* packet;
		unsigned char ack[RELIABLE_HDR_LEN];
		ack[0] = HDR_ACK;
		*(unsigned int*)(ack + 1) = seq;
		socketWrite(s->fd, ack, sizeof(ack), 0, saddr);

		seq = ntohl(seq);
		if (seq < s->reliable.m_recvseq)
			return 1;
		if (seq == s->reliable.m_recvseq) {
			for (cur = s->m_recvpacketlist.head; cur; cur = next) {
				NioMsg_t* msgptr;
				packet = pod_container_of(cur, ReliableDataPacket_t, msg.m_listnode);
				if (packet->seq != s->reliable.m_recvseq)
					break;
				next = cur->next;
				s->reliable.m_recvseq++;
				msgptr = NULL;
				if (s->decode_packet(s, packet->len ? packet->data : NULL, packet->len, &packet->saddr, &msgptr) < 0) {
					s->valid = 0;
					return 0;
				}
				if (msgptr) {
					msgptr->type = NIO_SOCKET_USER_MESSAGE;
					dataqueuePush(s->m_loop->m_msgdq, &msgptr->m_listnode);
				}
				free(packet);
			}
		}
		else {
			for (cur = s->m_recvpacketlist.head; cur; cur = cur->next) {
				packet = pod_container_of(cur, ReliableDataPacket_t, msg.m_listnode);
				if (packet->seq > seq)
					break;
				else if (packet->seq == seq)
					return 1;
			}
			packet = (ReliableDataPacket_t*)malloc(sizeof(ReliableDataPacket_t) + len - RELIABLE_HDR_LEN);
			if (!packet) {
				s->valid = 0;
				return 0;
			}
			packet->msg.type = NIO_SOCKET_USER_MESSAGE;
			packet->saddr = *saddr;
			packet->s = s;
			packet->seq = seq;
			packet->len = len - RELIABLE_HDR_LEN;
			memcpy(packet->data, buffer + RELIABLE_HDR_LEN, len - RELIABLE_HDR_LEN);
			if (cur)
				listInsertNodeFront(&s->m_recvpacketlist, cur, &packet->msg.m_listnode);
			else
				listInsertNodeBack(&s->m_recvpacketlist, s->m_recvpacketlist.tail, &packet->msg.m_listnode);
		}
	}
	return 1;
}

static void reactor_socket_do_read(NioSocket_t* s) {
	if (SOCK_STREAM == s->socktype) {
		if (s->accept_callback) {
			if (!reactorAccept(s->fd, s->m_readOl, s->accept_callback, s->accept_callback_arg)) {
				s->valid = 0;
			}
		}
		else {
			struct sockaddr_storage saddr;
			int res = socketTcpReadableBytes(s->fd);
			do {
				unsigned char *p;
				size_t offset = 0;

				if (res <= 0) {
					s->valid = 0;
					break;
				}

				p = (unsigned char*)realloc(s->m_inbuf, s->m_inbuflen + res);
				if (!p) {
					free(s->m_inbuf);
					s->m_inbuf = NULL;
					s->m_inbuflen = 0;
					s->valid = 0;
					break;
				}
				s->m_inbuf = p;
				res = socketRead(s->fd, s->m_inbuf + s->m_inbuflen, res, 0, &saddr);
				if (res <= 0) {
					free(s->m_inbuf);
					s->m_inbuf = NULL;
					s->m_inbuflen = 0;
					s->valid = 0;
					break;
				}
				else {
					s->m_inbuflen += res;
				}

				while (offset < s->m_inbuflen) {
					NioMsg_t* msgptr = NULL;
					int len = s->decode_packet(s, s->m_inbuf + offset, s->m_inbuflen - offset, &saddr, &msgptr);
					if (0 == len)
						break;
					if (len < 0) {
						s->valid = 0;
						offset = s->m_inbuflen;
						break;
					}
					offset += len;
					if (msgptr) {
						msgptr->type = NIO_SOCKET_USER_MESSAGE;
						dataqueuePush(s->m_loop->m_msgdq, &msgptr->m_listnode);
					}
				}

				if (offset) {
					if (offset >= s->m_inbuflen) {
						free(s->m_inbuf);
						s->m_inbuf = NULL;
						s->m_inbuflen = 0;
					}
					else {
						size_t n = offset, start;
						for (start = 0; offset < s->m_inbuflen; ++start, ++offset) {
							s->m_inbuf[start] = s->m_inbuf[offset];
						}
						s->m_inbuflen -= n;
					}
				}
			} while (0);
		}
	}
	else if (SOCK_DGRAM == s->socktype) {
		unsigned char buffer[0xffff];
		struct sockaddr_storage saddr;
		unsigned int readtimes;
		for (readtimes = 0; readtimes < 10; ++readtimes) {
			int res = socketRead(s->fd, buffer, sizeof(buffer), 0, &saddr);
			if (res < 0) {
				if (errnoGet() != EWOULDBLOCK)
					s->valid = 0;
				break;
			}
			else if (0 == res) {
				NioMsg_t* msgptr = NULL;
				if (s->decode_packet(s, NULL, 0, &saddr, &msgptr) < 0) {
					s->valid = 0;
					break;
				}
				if (msgptr) {
					msgptr->type = NIO_SOCKET_USER_MESSAGE;
					dataqueuePush(s->m_loop->m_msgdq, &msgptr->m_listnode);
				}
			}
			else if (s->reliable.enable) {
				if (res < RELIABLE_HDR_LEN)
					continue;
				if (!reactor_socket_reliable_read(s, buffer, res, &saddr))
					break;
			}
			else {
				int offset = 0, len = -1;
				while (offset < res) {
					NioMsg_t* msgptr = NULL;
					len = s->decode_packet(s, buffer + offset, res - offset, &saddr, &msgptr);
					if (len < 0) {
						s->valid = 0;
						break;
					}
					offset += len;
					if (msgptr) {
						msgptr->type = NIO_SOCKET_USER_MESSAGE;
						dataqueuePush(s->m_loop->m_msgdq, &msgptr->m_listnode);
					}
				}
				if (len < 0)
					break;
			}
		}
	}
}

static int reactorsocket_write(NioSocket_t* s) {
	struct sockaddr_storage saddr;
	if (!s->m_writeOl) {
		s->m_writeOl = reactorMallocOverlapped(REACTOR_WRITE);
		if (!s->m_writeOl) {
			s->valid = 0;
			return 0;
		}
	}
	if (reactorCommit(&s->m_loop->m_reactor, s->fd, REACTOR_WRITE, s->m_writeOl, &saddr))
		return 1;
	s->valid = 0;
	return 0;
}

static void reactor_socket_do_write(NioSocket_t* s) {
	if (SOCK_STREAM != s->socktype)
		return;
	if (s->connect_callback) {
		int errnum = reactorConnectCheckSuccess(s->fd) ? 0 : errnoGet();
		if (errnum) {
			s->valid = 0;
		}
		else if (!reactorsocket_read(s)) {
			errnum = errnoGet();
			s->valid = 0;
		}
		s->connect_callback(s, errnum);
		if (s->valid)
			s->connect_callback = NULL;
		return;
	}
	if (!s->valid)
		return;

	dataqueuePush(&s->m_loop->m_sender->m_dq, &s->m_sendmsg.m_listnode);
}

NioSocket_t* niosocketSend(NioSocket_t* s, const void* data, unsigned int len, const struct sockaddr_storage* saddr) {
	if (!s->valid)
		return NULL;
	if (!data || !len)
		return s;
	if (s->socktype != SOCK_STREAM && s->reliable.enable) {
		ReliableDataPacket_t* packet = (ReliableDataPacket_t*)malloc(sizeof(ReliableDataPacket_t) + RELIABLE_HDR_LEN + len);
		if (!packet)
			return NULL;
		packet->msg.type = NIO_SOCKET_RELIABLE_MESSAGE;
		if (saddr && SOCK_STREAM != s->socktype)
			packet->saddr = *saddr;
		else
			packet->saddr.ss_family = AF_UNSPEC;
		packet->s = s;
		packet->resendtimes = 0;
		packet->len = RELIABLE_HDR_LEN + len;
		memcpy(packet->data + RELIABLE_HDR_LEN, data, len);
		dataqueuePush(&s->m_loop->m_sender->m_dq, &packet->msg.m_listnode);
	}
	else {
		Packet_t* packet = (Packet_t*)malloc(sizeof(Packet_t) + len);
		if (!packet)
			return NULL;
		packet->msg.type = NIO_SOCKET_USER_MESSAGE;
		if (saddr && SOCK_STREAM != s->socktype)
			packet->saddr = *saddr;
		else
			packet->saddr.ss_family = AF_UNSPEC;
		packet->s = s;
		packet->offset = 0;
		packet->len = len;
		memcpy(packet->data, data, len);
		dataqueuePush(&s->m_loop->m_sender->m_dq, &packet->msg.m_listnode);
	}
	return s;
}

NioSocket_t* niosocketSendv(NioSocket_t* s, Iobuf_t iov[], unsigned int iovcnt, const struct sockaddr_storage* saddr) {
	size_t i, nbytes;
	if (!s->valid)
		return NULL;
	if (!iov || !iovcnt)
		return s;
	for (nbytes = 0, i = 0; i < iovcnt; ++i)
		nbytes += iobufLen(iov + i);
	if (0 == nbytes)
		return s;
	if (s->socktype != SOCK_STREAM && s->reliable.enable) {
		ReliableDataPacket_t* packet = (ReliableDataPacket_t*)malloc(sizeof(ReliableDataPacket_t) + RELIABLE_HDR_LEN + nbytes);
		if (!packet)
			return NULL;
		packet->msg.type = NIO_SOCKET_RELIABLE_MESSAGE;
		if (saddr)
			packet->saddr = *saddr;
		else
			packet->saddr.ss_family = AF_UNSPEC;
		packet->resendtimes = 0;
		packet->s = s;
		packet->len = RELIABLE_HDR_LEN + nbytes;
		for (nbytes = RELIABLE_HDR_LEN, i = 0; i < iovcnt; ++i) {
			memcpy(packet->data + nbytes, iobufPtr(iov + i), iobufLen(iov + i));
			nbytes += iobufLen(iov + i);
		}
		dataqueuePush(&s->m_loop->m_sender->m_dq, &packet->msg.m_listnode);
	}
	else {
		Packet_t* packet = (Packet_t*)malloc(sizeof(Packet_t) + nbytes);
		if (!packet)
			return NULL;
		packet->msg.type = NIO_SOCKET_USER_MESSAGE;
		if (saddr && SOCK_STREAM != s->socktype)
			packet->saddr = *saddr;
		else
			packet->saddr.ss_family = AF_UNSPEC;
		packet->s = s;
		packet->offset = 0;
		packet->len = nbytes;
		for (nbytes = 0, i = 0; i < iovcnt; ++i) {
			memcpy(packet->data + nbytes, iobufPtr(iov + i), iobufLen(iov + i));
			nbytes += iobufLen(iov + i);
		}
		dataqueuePush(&s->m_loop->m_sender->m_dq, &packet->msg.m_listnode);
	}
	return s;
}

void niosocketShutdown(NioSocket_t* s) {
	if (SOCK_STREAM == s->socktype && !s->accept_callback) {
		socketShutdown(s->fd, SHUT_WR);
		if (INFTIM == s->timeout_second)
			s->timeout_second = 5;
	}
	else {
		s->valid = 0;
	}
}

NioSocket_t* niosocketCreate(FD_t fd, int domain, int socktype, int protocol, NioSocket_t*(*pmalloc)(void), void(*pfree)(NioSocket_t*)) {
	NioSocket_t* s = pmalloc();
	if (!s)
		return NULL;
	if (INVALID_FD_HANDLE == fd) {
		fd = socket(domain, socktype, protocol);
		if (INVALID_FD_HANDLE == fd) {
			if (pfree)
				pfree(s);
			return NULL;
		}
	}
	if (!socketNonBlock(fd, TRUE)) {
		if (pfree)
			pfree(s);
		return NULL;
	}
	s->fd = fd;
	s->domain = domain;
	s->socktype = socktype;
	s->protocol = protocol;
	s->valid = 1;
	s->timeout_second = INFTIM;
	s->userdata = NULL;
	s->accept_callback = NULL;
	s->connect_callback = NULL;
	s->reg_callback = NULL;
	s->decode_packet = NULL;
	s->close = NULL;
	s->m_regmsg.type = NIO_SOCKET_REG_MESSAGE;
	s->m_closemsg.type = NIO_SOCKET_CLOSE_MESSAGE;
	s->m_sendmsg.type = NIO_SOCKET_STREAM_WRITEABLE_MESSAGE;
	s->m_hashnode.key = &s->fd;
	s->m_loop = NULL;
	s->m_free = pfree;
	s->m_readOl = NULL;
	s->m_writeOl = NULL;
	s->m_lastActiveTime = 0;
	s->m_inbuf = NULL;
	s->m_inbuflen = 0;
	listInit(&s->m_recvpacketlist);
	listInit(&s->m_sendpacketlist);
	s->reliable.enable = 0;
	s->reliable.rto = 4;
	s->reliable.m_recvseq = 0;
	s->reliable.m_sendseq = 0;
	return s;
}

static int sockht_keycmp(const struct HashtableNode_t* node, const void* key) {
	return pod_container_of(node, NioSocket_t, m_hashnode)->fd != *(FD_t*)key;
}

static unsigned int sockht_keyhash(const void* key) { return *(FD_t*)key; }

static size_t sockht_expire(Hashtable_t* ht, NioSocket_t* buf[], size_t n) {
	time_t now_ts_sec = gmtimeSecond();
	size_t i = 0;
	HashtableNode_t *cur, *next;
	for (cur = hashtableFirstNode(ht); cur; cur = next) {
		NioSocket_t* s;
		next = hashtableNextNode(cur);
		s = pod_container_of(cur, NioSocket_t, m_hashnode);
		if (s->valid && (s->timeout_second < 0 ||
			now_ts_sec - s->timeout_second < s->m_lastActiveTime))
		{
			continue;
		}
		s->valid = 0;
		if (i < n) {
			hashtableRemoveNode(ht, cur);
			buf[i++] = s;
		}
	}
	return i;
}

void nioloopHandler(NioLoop_t* loop, long long timestamp_msec, int wait_msec) {
	int n;
	NioEv_t e[4096];
	ListNode_t *cur, *next;
	if (wait_msec < 0)
		wait_msec = 1000;
	if (timestamp_msec < loop->m_checkexpire_msec)
		loop->m_checkexpire_msec = timestamp_msec;
	else if (timestamp_msec > loop->m_checkexpire_msec + 1000)
		wait_msec = 0;
	else if (timestamp_msec + wait_msec > loop->m_checkexpire_msec + 1000) {
		int expire_wait_msec = loop->m_checkexpire_msec + 1000 - timestamp_msec;
		if (expire_wait_msec < wait_msec)
			wait_msec = expire_wait_msec;
	}

	n = reactorWait(&loop->m_reactor, e, sizeof(e) / sizeof(e[0]), wait_msec);
	if (n < 0) {
		return;
	}
	else if (n > 0) {
		time_t now_ts_sec = timestamp_msec / 1000;
		int i;
		for (i = 0; i < n; ++i) {
			HashtableNode_t* find_node;
			NioSocket_t* s;
			FD_t fd;
			int event;
			void* ol;

			reactorResult(e + i, &fd, &event, &ol);
			if (fd == loop->m_socketpair[0]) {
				struct sockaddr_storage saddr;
				char c;
				socketRead(fd, &c, sizeof(c), 0, NULL);
				reactorCommit(&loop->m_reactor, fd, REACTOR_READ, loop->m_readOl, &saddr);
				continue;
			}
			find_node = hashtableSearchKey(&loop->m_sockht, &fd);
			if (!find_node)
				continue;
			s = pod_container_of(find_node, NioSocket_t, m_hashnode);

			s->m_lastActiveTime = now_ts_sec;
			switch (event) {
				case REACTOR_READ:
					reactor_socket_do_read(s);
					reactorsocket_read(s);
					break;
				case REACTOR_WRITE:
					reactor_socket_do_write(s);
					break;
				default:
					s->valid = 0;
			}
			if (s->valid)
				continue;

			hashtableRemoveNode(&loop->m_sockht, &s->m_hashnode);
			if (s->accept_callback || s->connect_callback)
				niosocketFree(s);
			else
				dataqueuePush(loop->m_msgdq, &s->m_closemsg.m_listnode);
		}
	}

	criticalsectionEnter(&loop->m_msglistlock);
	cur = loop->m_msglist.head;
	listInit(&loop->m_msglist);
	criticalsectionLeave(&loop->m_msglistlock);

	for (; cur; cur = next) {
		NioMsg_t* message;
		next = cur->next;
		message = pod_container_of(cur, NioMsg_t, m_listnode);
		if (NIO_SOCKET_CLOSE_MESSAGE == message->type) {
			NioSocket_t* s = pod_container_of(message, NioSocket_t, m_closemsg);
			niosocketFree(s);
		}
		else if (NIO_SOCKET_REG_MESSAGE == message->type) {
			NioSocket_t* s = pod_container_of(message, NioSocket_t, m_regmsg);
			int reg_ok = 0;
			do {
				if (!reactorReg(&loop->m_reactor, s->fd))
					break;
				s->m_loop = loop;
				s->m_lastActiveTime = gmtimeSecond();
				if (SOCK_STREAM == s->socktype && s->connect_callback) {
					if (!s->m_writeOl) {
						s->m_writeOl = reactorMallocOverlapped(REACTOR_CONNECT);
						if (!s->m_writeOl)
							break;
					}
					if (!reactorCommit(&loop->m_reactor, s->fd, REACTOR_CONNECT, s->m_writeOl, &s->connect_saddr))
						break;
				}
				else if (!reactorsocket_read(s))
					break;

				hashtableReplaceNode(hashtableInsertNode(&loop->m_sockht, &s->m_hashnode), &s->m_hashnode);
				dataqueuePush(&loop->m_sender->m_dq, &s->m_regmsg.m_listnode);
				reg_ok = 1;
			} while (0);
			if (s->reg_callback)
				s->reg_callback(s, reg_ok ? 0 : errnoGet());
			if (!reg_ok)
				niosocketFree(s);
		}
	}

	if (n)
		timestamp_msec = gmtimeMillisecond();
	else
		timestamp_msec += wait_msec;

	if (timestamp_msec < loop->m_checkexpire_msec) {
		loop->m_checkexpire_msec = timestamp_msec;
	}
	else if (timestamp_msec >= loop->m_checkexpire_msec + 1000) {
		NioSocket_t* expire_sockets[512];
		List_t list;
		size_t i;
		size_t cnt = sockht_expire(&loop->m_sockht, expire_sockets, sizeof(expire_sockets) / sizeof(expire_sockets[0]));
		listInit(&list);
		for (i = 0; i < cnt; ++i) {
			listInsertNodeBack(&list, list.tail, &expire_sockets[i]->m_closemsg.m_listnode);
		}
		dataqueuePushList(loop->m_msgdq, &list);
		loop->m_checkexpire_msec = timestamp_msec;
	}
}

NioLoop_t* nioloopCreate(NioLoop_t* loop, DataQueue_t* msgdq, NioSender_t* sender) {
	struct sockaddr_storage saddr;
	loop->initok = 0;

	if (!socketPair(SOCK_STREAM, loop->m_socketpair))
		return NULL;

	loop->m_readOl = reactorMallocOverlapped(REACTOR_READ);
	if (!loop->m_readOl) {
		socketClose(loop->m_socketpair[0]);
		socketClose(loop->m_socketpair[1]);
		return NULL;
	}

	if (!reactorCreate(&loop->m_reactor)) {
		reactorFreeOverlapped(loop->m_readOl);
		socketClose(loop->m_socketpair[0]);
		socketClose(loop->m_socketpair[1]);
		return NULL;
	}

	if (!socketNonBlock(loop->m_socketpair[0], TRUE) ||
		!socketNonBlock(loop->m_socketpair[1], TRUE) ||
		!reactorReg(&loop->m_reactor, loop->m_socketpair[0]) ||
		!reactorCommit(&loop->m_reactor, loop->m_socketpair[0], REACTOR_READ, loop->m_readOl, &saddr))
	{
		reactorFreeOverlapped(loop->m_readOl);
		socketClose(loop->m_socketpair[0]);
		socketClose(loop->m_socketpair[1]);
		reactorClose(&loop->m_reactor);
		return NULL;
	}

	if (!criticalsectionCreate(&loop->m_msglistlock)) {
		reactorFreeOverlapped(loop->m_readOl);
		socketClose(loop->m_socketpair[0]);
		socketClose(loop->m_socketpair[1]);
		reactorClose(&loop->m_reactor);
		return NULL;
	}

	loop->m_msgdq = msgdq;
	loop->m_sender = sender;
	listInit(&loop->m_msglist);
	hashtableInit(&loop->m_sockht,
		loop->m_sockht_bulks, sizeof(loop->m_sockht_bulks) / sizeof(loop->m_sockht_bulks[0]),
		sockht_keycmp, sockht_keyhash);
	loop->initok = 1;
	loop->m_checkexpire_msec = 0;
	return loop;
}

void nioloopReg(NioLoop_t* loop, NioSocket_t* s[], size_t n) {
	char c;
	size_t i;
	List_t list;
	listInit(&list);
	for (i = 0; i < n; ++i) {
		listInsertNodeBack(&list, list.tail, &s[i]->m_regmsg.m_listnode);
	}
	criticalsectionEnter(&loop->m_msglistlock);
	listMerge(&loop->m_msglist, &list);
	criticalsectionLeave(&loop->m_msglistlock);
	socketWrite(loop->m_socketpair[1], &c, sizeof(c), 0, NULL);
}

void nioloopDestroy(NioLoop_t* loop) {
	if (loop && loop->initok) {
		HashtableNode_t *cur, *next;
		reactorFreeOverlapped(loop->m_readOl);
		socketClose(loop->m_socketpair[0]);
		socketClose(loop->m_socketpair[1]);
		reactorClose(&loop->m_reactor);
		criticalsectionClose(&loop->m_msglistlock);
		for (cur = hashtableFirstNode(&loop->m_sockht); cur; cur = next) {
			next = hashtableNextNode(cur);
			niosocketFree(pod_container_of(cur, NioSocket_t, m_hashnode));
		}
	}
}

static void reactorsocket_real_send(NioSocket_t* s, Packet_t* packet) {
	int res, is_empty;
	if (!s->valid)
		return;
	res = 0;
	is_empty = !s->m_sendpacketlist.head;
	if (SOCK_STREAM != s->socktype || is_empty) {
		struct sockaddr_storage* saddrptr = (packet->saddr.ss_family != AF_UNSPEC ? &packet->saddr : NULL);
		res = socketWrite(s->fd, packet->data, packet->len, 0, saddrptr);
		if (res < 0) {
			if (errnoGet() != EWOULDBLOCK) {
				s->valid = 0;
				return;
			}
			res = 0;
		}
		else if (res >= packet->len) {
			if (NIO_SOCKET_USER_MESSAGE == packet->msg.type)
				free(packet);
			return;
		}
	}
	if (SOCK_STREAM == s->socktype) {
		packet->offset = res;
		listInsertNodeBack(&s->m_sendpacketlist, s->m_sendpacketlist.tail, &packet->msg.m_listnode);
		if (is_empty)
			reactorsocket_write(s);
	}
}

NioSender_t* niosenderCreate(NioSender_t* sender) {
	sender->initok = 0;
	if (!dataqueueInit(&sender->m_dq))
		return NULL;
	listInit(&sender->m_socklist);
	sender->m_resend_msec = 0;
	sender->initok = 1;
	return sender;
}

static void resend_packet(NioSender_t* sender, NioSocket_t* s, long long timestamp_msec) {
	ListNode_t* cur;
	for (cur = s->m_sendpacketlist.head; cur; cur = cur->next) {
		ReliableDataPacket_t* packet = pod_container_of(cur, ReliableDataPacket_t, msg.m_listnode);
		if (packet->resend_timestamp_msec <= timestamp_msec) {
			socketWrite(s->fd, packet->data, packet->len, 0, &packet->saddr);
			packet->resend_timestamp_msec = timestamp_msec + s->reliable.rto;
			packet->resendtimes++;
		}
		if (!sender->m_resend_msec || packet->resend_timestamp_msec < sender->m_resend_msec)
			sender->m_resend_msec = packet->resend_timestamp_msec;
	}
}

void niosenderHandler(NioSender_t* sender, long long timestamp_msec, int wait_msec) {
	ListNode_t *cur, *next;
	if (sender->m_resend_msec > timestamp_msec) {
		int resend_wait_msec = sender->m_resend_msec - timestamp_msec;
		if (resend_wait_msec < wait_msec || wait_msec < 0)
			wait_msec = resend_wait_msec;
	}
	else if (sender->m_resend_msec) {
		wait_msec = 0;
	}
	for (cur = dataqueuePop(&sender->m_dq, wait_msec, ~0); cur; cur = next) {
		NioMsg_t* msgbase = pod_container_of(cur, NioMsg_t, m_listnode);
		next = cur->next;
		if (NIO_SOCKET_REG_MESSAGE == msgbase->type) {
			NioSocket_t* s = pod_container_of(msgbase, NioSocket_t, m_regmsg);
			listInsertNodeBack(&sender->m_socklist, sender->m_socklist.tail, &s->m_senderlistnode);
		}
		else if (NIO_SOCKET_CLOSE_MESSAGE == msgbase->type) {
			NioSocket_t* s = pod_container_of(msgbase, NioSocket_t, m_closemsg);
			listRemoveNode(&sender->m_socklist, &s->m_senderlistnode);
			criticalsectionEnter(&s->m_loop->m_msglistlock);
			listInsertNodeBack(&s->m_loop->m_msglist, s->m_loop->m_msglist.tail, cur);
			criticalsectionLeave(&s->m_loop->m_msglistlock);
		}
		else if (NIO_SOCKET_RELIABLE_MESSAGE == msgbase->type) {
			ReliableDataPacket_t* packet = pod_container_of(msgbase, ReliableDataPacket_t, msg);
			NioSocket_t* s = packet->s;
			if (!s->valid)
				continue;
			packet->data[0] = HDR_DATA;
			*(unsigned int*)(packet->data + 1) = htonl(s->reliable.m_sendseq);
			packet->seq = s->reliable.m_sendseq++;
			if (socketWrite(s->fd, packet->data, packet->len, 0, &packet->saddr) < 0) {
				s->valid = 0;
				continue;
			}
			packet->resend_timestamp_msec = gmtimeMillisecond() + s->reliable.rto;
			if (!sender->m_resend_msec || sender->m_resend_msec > packet->resend_timestamp_msec)
				sender->m_resend_msec = packet->resend_timestamp_msec;
			listInsertNodeBack(&s->m_sendpacketlist, s->m_sendpacketlist.tail, &packet->msg.m_listnode);
		}
		else if (NIO_SOCKET_USER_MESSAGE == msgbase->type) {
			Packet_t* packet = pod_container_of(msgbase, Packet_t, msg);
			reactorsocket_real_send(packet->s, packet);
		}
		else if (NIO_SOCKET_STREAM_WRITEABLE_MESSAGE == msgbase->type) {
			NioSocket_t* s = pod_container_of(msgbase, NioSocket_t, m_sendmsg);
			ListNode_t* cur, *next;
			if (!s->valid)
				continue;
			for (cur = s->m_sendpacketlist.head; cur; cur = next) {
				int res;
				Packet_t* packet = pod_container_of(cur, Packet_t, msg.m_listnode);
				next = cur->next;
				res = socketWrite(s->fd, packet->data + packet->offset, packet->len - packet->offset, 0, NULL);
				if (res < 0) {
					if (errnoGet() != EWOULDBLOCK) {
						s->valid = 0;
						break;
					}
					res = 0;
				}
				packet->offset += res;
				if (packet->offset >= packet->len) {
					listRemoveNode(&s->m_sendpacketlist, cur);
					if (NIO_SOCKET_USER_MESSAGE == packet->msg.type)
						free(packet);
					continue;
				}
				else if (s->valid)
					reactorsocket_write(s);
				break;
			}
		}
		else if (NIO_SOCKET_RELIABLE_ACK_MESSAGE == msgbase->type) {
			ListNode_t* cur;
			ReliableAckPacket_t* packet = pod_container_of(msgbase, ReliableAckPacket_t, msg);
			unsigned int ack_seq = packet->seq;
			NioSocket_t* s = packet->s;
			free(packet);
			for (cur = s->m_sendpacketlist.head; cur; cur = cur->next) {
				ReliableDataPacket_t* packet = pod_container_of(cur, ReliableDataPacket_t, msg.m_listnode);
				if (ack_seq < packet->seq)
					break;
				if (packet->seq == ack_seq) {
					listRemoveNode(&s->m_sendpacketlist, cur);
					free(packet);
					break;
				}
			}
		}
	}
	timestamp_msec = gmtimeMillisecond();
	if (sender->m_resend_msec && timestamp_msec >= sender->m_resend_msec) {
		sender->m_resend_msec = 0;
		for (cur = sender->m_socklist.head; cur; cur = cur->next) {
			NioSocket_t* s = pod_container_of(cur, NioSocket_t, m_senderlistnode);
			if (s->socktype == SOCK_STREAM || !s->reliable.enable)
				continue;
			resend_packet(sender, s, timestamp_msec);
		}
	}
}

void niosenderDestroy(NioSender_t* sender) {
	if (sender && sender->initok) {
		ListNode_t *cur, *next;
		for (cur = dataqueuePop(&sender->m_dq, 0, ~0); cur; cur = next) {
			NioMsg_t* msgbase = pod_container_of(cur, NioMsg_t, m_listnode);
			next = cur->next;
			if (NIO_SOCKET_USER_MESSAGE == msgbase->type)
				free(pod_container_of(msgbase, Packet_t, msg));
		}
		dataqueueDestroy(&sender->m_dq, NULL);
	}
}

void niomsgHandler(DataQueue_t* dq, int max_wait_msec, void (*user_msg_callback)(NioMsg_t*, void*), void* arg) {
	ListNode_t* cur, *next;
	for (cur = dataqueuePop(dq, max_wait_msec, ~0); cur; cur = next) {
		NioMsg_t* message = pod_container_of(cur, NioMsg_t, m_listnode);
		next = cur->next;
		if (NIO_SOCKET_CLOSE_MESSAGE == message->type) {
			NioSocket_t* s = pod_container_of(message, NioSocket_t, m_closemsg);
			if (s->close)
				s->close(s);
			dataqueuePush(&s->m_loop->m_sender->m_dq, cur);
		}
		else if (NIO_SOCKET_USER_MESSAGE == message->type) {
			user_msg_callback(message, arg);
		}
	}
}

void niomsgClean(DataQueue_t* dq, void(*deleter)(NioMsg_t*)) {
	ListNode_t *cur = dataqueuePop(dq, 0, ~0);
	if (deleter) {
		ListNode_t *next;
		for (; cur; cur = next) {
			NioMsg_t* message = pod_container_of(cur, NioMsg_t, m_listnode);
			next = cur->next;
			if (NIO_SOCKET_USER_MESSAGE == message->type)
				deleter(message);
		}
	}
}

#ifdef __cplusplus
}
#endif
