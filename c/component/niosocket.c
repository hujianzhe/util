//
// Created by hujianzhe on 18-8-13.
//

#include "../syslib/error.h"
#include "niosocket.h"
#include <stdlib.h>
#include <string.h>

typedef struct WaitSendData {
	ListNode_t m_listnode;
	size_t offset;
	size_t len;
	unsigned char data[1];
} WaitSendData;

#ifdef __cplusplus
extern "C" {
#endif

void niosocketFree(NioSocket_t* s) {
	if (!s)
		return;

	socketClose(s->fd);
	reactorFreeOverlapped(s->m_readOl);
	reactorFreeOverlapped(s->m_writeOl);

	if (SOCK_STREAM == s->socktype) {
		criticalsectionClose(&s->m_outbufLock);
	}
	if (s->m_inbuf) {
		free(s->m_inbuf);
		s->m_inbuf = NULL;
		s->m_inbuflen = 0;
	}
	do {
		ListNode_t *cur, *next;
		for (cur = s->m_outbuflist.head; cur; cur = next) {
			next = cur->next;
			free(pod_container_of(cur, WaitSendData, m_listnode));
		}
	} while (0);

	s->free(s);
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
	if (reactorCommit(&s->loop->m_reactor, s->fd, opcode, s->m_readOl, &saddr))
		return 1;
	s->valid = 0;
	return 0;
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
					int len = s->decode_packet(s, s->m_inbuf + offset, s->m_inbuflen - offset, &saddr);
					if (0 == len)
						break;
					if (len < 0) {
						s->valid = 0;
						offset = s->m_inbuflen;
						break;
					}
					offset += len;
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
		struct sockaddr_storage saddr;
		while (1) {
			unsigned char buffer[0xffff];
			int res = socketRead(s->fd, buffer, sizeof(buffer), 0, &saddr);
			if (res < 0) {
				if (errnoGet() != EWOULDBLOCK)
					s->valid = 0;
				break;
			}
			else if (0 == res) {
				if (s->decode_packet(s, NULL, 0, &saddr) < 0) {
					s->valid = 0;
					break;
				}
			}
			else {
				int offset = 0, len = -1;
				while (offset < res) {
					len = s->decode_packet(s, buffer + offset, res - offset, &saddr);
					if (len < 0) {
						s->valid = 0;
						break;
					}
					offset += len;
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
	if (reactorCommit(&s->loop->m_reactor, s->fd, REACTOR_WRITE, s->m_writeOl, &saddr))
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

	criticalsectionEnter(&s->m_outbufLock);
	do {
		ListNode_t * cur;
		for (cur = s->m_outbuflist.head; cur; ) {
			WaitSendData* wsd = pod_container_of(cur, WaitSendData, m_listnode);
			int res = socketWrite(s->fd, wsd->data + wsd->offset, wsd->len - wsd->offset, 0, NULL);
			if (res < 0) {
				if (errnoGet() != EWOULDBLOCK) {
					s->valid = 0;
					break;
				}
				res = 0;
			}
			wsd->offset += res;
			if (wsd->offset >= wsd->len) {
				listRemoveNode(&s->m_outbuflist, cur);
				cur = cur->next;
				free(wsd);
			}
			else if (s->valid) {
				reactorsocket_write(s);
				break;
			}
		}
		if (!s->m_outbuflist.head) {
			s->m_writeCommit = 0;
		}
	} while (0);
	criticalsectionLeave(&s->m_outbufLock);
}

int niosocketSendv(NioSocket_t* s, Iobuf_t iov[], unsigned int iovcnt, struct sockaddr_storage* saddr) {
	if (!s->valid)
		return -1;
	if (!iov || !iovcnt)
		return 0;

	if (SOCK_STREAM == s->socktype) {
		int res = 0, sendbytes = -1;
		size_t nbytes = 0, i;
		for (i = 0; i < iovcnt; ++i) {
			nbytes += iobufLen(iov + i);
		}
		if (0 == nbytes) {
			return 0;
		}

		criticalsectionEnter(&s->m_outbufLock);
		do {
			if (!s->m_outbuflist.head) {
				res = socketWritev(s->fd, iov, iovcnt, 0, NULL);
				if (res < 0) {
					if (errnoGet() != EWOULDBLOCK) {
						s->valid = 0;
						break;
					}
					res = 0;
				}
			}
			sendbytes = res;
			if (res < nbytes) {
				unsigned int i;
				size_t off;
				WaitSendData* wsd = (WaitSendData*)malloc(sizeof(WaitSendData) + (nbytes - res));
				if (!wsd) {
					s->valid = 0;
					break;
				}
				wsd->len = nbytes - res;
				wsd->offset = 0;
				for (off = 0, i = 0; i < iovcnt; ++i) {
					if (res >= iobufLen(iov + i))
						res -= iobufLen(iov + i);
					else {
						memcpy(wsd->data + off, ((char*)iobufPtr(iov + i)) + res, iobufLen(iov + i) - res);
						off += iobufLen(iov + i) - res;
						res = 0;
					}
				}
				listInsertNodeBack(&s->m_outbuflist, s->m_outbuflist.tail, &wsd->m_listnode);
				if (!s->m_writeCommit) {
					s->m_writeCommit = 1;
					reactorsocket_write(s);
				}
			}
		} while (0);
		criticalsectionLeave(&s->m_outbufLock);

		return sendbytes;
	}
	else if (SOCK_DGRAM == s->socktype) {
		return socketWritev(s->fd, iov, iovcnt, 0, saddr);
	}
	return -1;
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
	if (SOCK_STREAM == socktype) {
		if (!criticalsectionCreate(&s->m_outbufLock)) {
			pfree(s);
			return NULL;
		}
	}
	if (INVALID_FD_HANDLE == fd) {
		fd = socket(domain, socktype, protocol);
		if (INVALID_FD_HANDLE == fd) {
			if (SOCK_STREAM == socktype)
				criticalsectionClose(&s->m_outbufLock);
			pfree(s);
			return NULL;
		}
	}
	s->fd = fd;
	s->domain = domain;
	s->socktype = socktype;
	s->protocol = protocol;
	s->valid = 1;
	s->timeout_second = INFTIM;
	s->loop = NULL;
	s->accept_callback = NULL;
	s->connect_callback = NULL;
	s->decode_packet = NULL;
	s->send_packet = niosocketSendv;
	s->close = NULL;
	s->free = pfree;
	s->m_hashnode.key = &s->fd;
	s->m_readOl = NULL;
	s->m_writeOl = NULL;
	s->m_lastActiveTime = 0;
	s->m_writeCommit = 0;
	s->m_inbuf = NULL;
	s->m_inbuflen = 0;
	listInit(&s->m_outbuflist);
	return s;
}

static int sockht_keycmp(struct HashtableNode_t* node, const void* key) {
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

void niosocketloopHandler(NioSocketLoop_t* loop) {
	NioEv_t e[4096];
	NioSocket_t* expire_sockets[512];
	int wait_msec = 1000;

	while (loop->valid) {
		int delta_msec;
		long long start_ts_msec = gmtimeMillisecond();
		int n = reactorWait(&loop->m_reactor, e, sizeof(e) / sizeof(e[0]), wait_msec);
		if (n < 0) {
			loop->valid = 0;
			break;
		}
		else if (n > 0) {
			time_t now_ts_sec = start_ts_msec / 1000;
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
					dataqueuePush(loop->m_msgdq, &s->m_msg.m_listnode);
			}
		}

		do {
			ListNode_t *cur, *next;
			criticalsectionEnter(&loop->m_msglistlock);
			for (cur = loop->m_msglist.head; cur; cur = next) {
				NioSocketMsg_t* message;
				next = cur->next;
				message = pod_container_of(cur, NioSocketMsg_t, m_listnode);
				if (NIO_SOCKET_CLOSE_MESSAGE == message->type) {
					NioSocket_t* s = pod_container_of(message, NioSocket_t, m_msg);
					niosocketFree(s);
				}
				else if (NIO_SOCKET_REG_MESSAGE == message->type) {
					NioSocket_t* s = pod_container_of(message, NioSocket_t, m_msg);
					int reg_ok = 0;
					do {
						if (!socketNonBlock(s->fd, TRUE))
							break;
						if (!reactorReg(&loop->m_reactor, s->fd))
							break;
						s->loop = loop;
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

						message->type = NIO_SOCKET_CLOSE_MESSAGE;
						hashtableReplaceNode(hashtableInsertNode(&loop->m_sockht, &s->m_hashnode), &s->m_hashnode);
						reg_ok = 1;
					} while (0);
					if (!reg_ok) {
						niosocketFree(s);
					}
				}
			}
			listInit(&loop->m_msglist);
			criticalsectionLeave(&loop->m_msglistlock);
		} while (0);

		if (n) {
			delta_msec = gmtimeMillisecond() - start_ts_msec;
			if (delta_msec < 0)
				delta_msec = wait_msec;
		}
		else
			delta_msec = wait_msec;

		if (delta_msec >= wait_msec) {
			List_t list;
			size_t i;
			size_t cnt = sockht_expire(&loop->m_sockht, expire_sockets, sizeof(expire_sockets) / sizeof(expire_sockets[0]));
			listInit(&list);
			for (i = 0; i < cnt; ++i) {
				listInsertNodeBack(&list, list.tail, &expire_sockets[i]->m_msg.m_listnode);
			}
			dataqueuePushList(loop->m_msgdq, &list);

			wait_msec = 1000;
		}
		else
			wait_msec -= delta_msec;
	}
}

NioSocketLoop_t* niosocketloopCreate(NioSocketLoop_t* loop, DataQueue_t* msgdq) {
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

	loop->valid = 1;
	loop->m_msgdq = msgdq;
	listInit(&loop->m_msglist);
	hashtableInit(&loop->m_sockht, loop->m_sockht_bulks, sizeof(loop->m_sockht_bulks) / sizeof(loop->m_sockht_bulks[0]),
			sockht_keycmp, sockht_keyhash);
	loop->initok = 1;
	return loop;
}

void niosocketloopAdd(NioSocketLoop_t* loop, NioSocket_t* s[], size_t n) {
	char c;
	size_t i;
	List_t list;
	listInit(&list);
	for (i = 0; i < n; ++i) {
		s[i]->m_msg.type = NIO_SOCKET_REG_MESSAGE;
		listInsertNodeBack(&list, list.tail, &s[i]->m_msg.m_listnode);
	}
	criticalsectionEnter(&loop->m_msglistlock);
	listMerge(&loop->m_msglist, &list);
	criticalsectionLeave(&loop->m_msglistlock);
	socketWrite(loop->m_socketpair[1], &c, sizeof(c), 0, NULL);
}

void niosocketloopDestroy(NioSocketLoop_t* loop) {
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

void niosocketmsgHandler(DataQueue_t* dq, int max_wait_msec, void (*user_msg_callback)(NioSocketMsg_t*, void*), void* arg) {
	ListNode_t* cur, *next;
	for (cur = dataqueuePop(dq, max_wait_msec, ~0); cur; cur = next) {
		NioSocketMsg_t* message = pod_container_of(cur, NioSocketMsg_t, m_listnode);
		next = cur->next;
		if (NIO_SOCKET_CLOSE_MESSAGE == message->type) {
			NioSocket_t* s = pod_container_of(message, NioSocket_t, m_msg);
			if (s->close)
				s->close(s);
			criticalsectionEnter(&s->loop->m_msglistlock);
			listInsertNodeBack(&s->loop->m_msglist, s->loop->m_msglist.tail, cur);
			criticalsectionLeave(&s->loop->m_msglistlock);
		}
		else if (NIO_SOCKET_USER_MESSAGE == message->type) {
			user_msg_callback(message, arg);
		}
	}
}

void niosocketmsgClean(DataQueue_t* dq, void(*deleter)(NioSocketMsg_t*)) {
	ListNode_t *cur = dataqueuePop(dq, 0, ~0);
	if (deleter) {
		ListNode_t *next;
		for (; cur; cur = next) {
			NioSocketMsg_t* message = pod_container_of(cur, NioSocketMsg_t, m_listnode);
			next = cur->next;
			if (NIO_SOCKET_USER_MESSAGE == message->type)
				deleter(message);
		}
	}
}

#ifdef __cplusplus
}
#endif
