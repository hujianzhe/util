//
// Created by hujianzhe on 18-8-13.
//

#include "../syslib/error.h"
#include "niosocket.h"
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

static int reactor_socket_check_valid(NioSocket_t* s) {
	return s->valid &&
		!(s->timeout_second >= 0 && gmtimeSecond() - s->timeout_second > s->m_lastActiveTime);
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
	if (reactorCommit(&s->loop->reactor, s->fd, opcode, &s->m_readOl, &saddr))
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
					int len = s->read(s, s->m_inbuf + offset, s->m_inbuflen - offset, &saddr);
					if (0 == len)
						break;
					if (len < 0) {
						s->valid = 0;
						offset = s->m_inbuflen;
						break;
					}
					offset += len;
				}

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

			if (s->read(s, res ? buffer : NULL, res, &saddr) < 0) {
				s->valid = 0;
				break;
			}
		}
	}
}

typedef struct WaitSendData {
	list_node_t m_listnode;
	size_t offset;
	size_t len;
	unsigned char data[1];
} WaitSendData;

static int reactorsocket_write(NioSocket_t* s) {
	struct sockaddr_storage saddr;
	if (reactorCommit(&s->loop->reactor, s->fd, REACTOR_WRITE, &s->m_writeOl, &saddr))
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
		if (errnum)
			niosocketFree(s);
		else
			s->connect_callback = NULL;
		return;
	}
	if (!s->valid)
		return;

	mutexLock(&s->m_outbufMutex);
	do {
		list_node_t * cur;
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
				list_remove_node(&s->m_outbuflist, cur);
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
	mutexUnlock(&s->m_outbufMutex);
}

int niosocketSendv(NioSocket_t* s, IoBuf_t* iov, unsigned int iovcnt, struct sockaddr_storage* saddr) {
	if (!s->valid)
		return -1;
	if (!iov || !iovcnt)
		return 0;

	if (SOCK_STREAM == s->socktype) {
		int res = 0, sendbytes = -1;
		size_t nbytes = 0, i;
		for (i = 0; i < iovcnt; ++i) {
			nbytes += iobuffer_len(iov + i);
		}
		if (0 == nbytes) {
			return 0;
		}

		mutexLock(&s->m_outbufMutex);
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
					if (res >= iobuffer_len(iov + i))
						res -= iobuffer_len(iov + i);
					else {
						memcpy(wsd->data + off, ((char*)iobuffer_buf(iov + i)) + res, iobuffer_len(iov + i) - res);
						off += iobuffer_len(iov + i) - res;
						res = 0;
					}
				}
				list_insert_node_back(&s->m_outbuflist, s->m_outbuflist.tail, &wsd->m_listnode);
				if (!s->m_writeCommit) {
					s->m_writeCommit = 1;
					reactorsocket_write(s);
				}
			}
		} while (0);
		mutexUnlock(&s->m_outbufMutex);
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

static void *(*niosocket_malloc)(size_t) = malloc;
static void(*niosocket_free)(void*) = free;
void niosocketMemoryHook(void*(*p_malloc)(size_t), void(*p_free)(void*)) {
	niosocket_malloc = p_malloc;
	niosocket_free = p_free;
}

NioSocket_t* niosocketCreate(FD_t fd, int domain, int socktype, int protocol) {
	NioSocket_t* s = (NioSocket_t*)niosocket_malloc(sizeof(NioSocket_t));
	if (!s)
		return NULL;
	if (SOCK_STREAM == socktype) {
		if (!mutexCreate(&s->m_outbufMutex)) {
			niosocket_free(s);
			return NULL;
		}
	}
	if (INVALID_FD_HANDLE == fd) {
		fd = socket(domain, socktype, protocol);
		if (INVALID_FD_HANDLE == fd) {
			if (SOCK_STREAM == socktype)
				mutexClose(&s->m_outbufMutex);
			niosocket_free(s);
			return NULL;
		}
	}
	s->fd = fd;
	s->domain = domain;
	s->socktype = socktype;
	s->protocol = protocol;
	s->valid = 0;
	s->timeout_second = INFTIM;
	s->loop = NULL;
	s->accept_callback = NULL;
	s->connect_callback = NULL;
	s->read = NULL;
	s->close = NULL;
	s->m_hashnode.key = &s->fd;
	s->m_readOl = NULL;
	s->m_writeOl = NULL;
	s->m_lastActiveTime = 0;
	s->m_writeCommit = 0;
	s->m_inbuf = NULL;
	s->m_inbuflen = 0;
	list_init(&s->m_outbuflist);
	return s;
}

void niosocketFree(NioSocket_t* s) {
	if (!s)
		return;
	if (SOCK_STREAM == s->socktype) {
		mutexClose(&s->m_outbufMutex);
		free(s->m_inbuf);
		s->m_inbuf = NULL;
	}
	socketClose(s->fd);
	free(s->m_readOl);
	free(s->m_writeOl);
	niosocket_free(s);
}

static int sockht_keycmp(struct hashtable_node_t* node, void* key) {
	return pod_container_of(node, NioSocket_t, m_hashnode)->fd != *(FD_t*)key;
}

static unsigned int sockht_keyhash(void* key) { return *(FD_t*)key; }

static size_t sockht_expire(hashtable_t* ht, NioSocket_t* buf[], size_t n) {
	size_t i = 0;
	hashtable_node_t *cur, *next;
	for (cur = hashtable_first_node(ht); cur; cur = next) {
		NioSocket_t* s;
		next = hashtable_next_node(cur);
		s = pod_container_of(cur, NioSocket_t, m_hashnode);
		if (reactor_socket_check_valid(s))
			continue;
		s->valid = 0;
		if (i < n) {
			hashtable_remove_node(ht, cur);
			buf[i++] = s;
		}
	}
	return i;
}

static unsigned int THREAD_CALL reactor_socket_loop_entry(void* arg) {
	NioEv_t e[4096];
	NioSocket_t* expire_sockets[512];
	NioSocketLoop_t* loop = (NioSocketLoop_t*)arg;

	while (loop->valid) {
		int i;
		int n = reactorWait(&loop->reactor, e, sizeof(e) / sizeof(e[0]), 20);
		if (n < 0)
			break;
		for (i = 0; i < n; ++i) {
			hashtable_node_t* find_node;
			NioSocket_t* s;
			FD_t fd;
			int event;
			void* ol;
			reactorResult(e + i, &fd, &event, &ol);
			find_node = hashtable_search_key(&loop->sockht, &fd);
			if (!find_node)
				continue;
			s = pod_container_of(find_node, NioSocket_t, m_hashnode);
			s->m_lastActiveTime = gmtimeSecond();
			switch (event) {
				case REACTOR_READ:
					reactor_socket_do_read(s);
					reactorsocket_read(s);
					break;
				case REACTOR_WRITE:
					reactor_socket_do_write(s);
					break;
			}
			if (reactor_socket_check_valid(s)) {
				continue;
			}
			hashtable_remove_node(&loop->sockht, &s->m_hashnode);
			s->valid = 0;
			dataqueuePush(loop->msgdq, &s->m_msg.m_listnode);
		}
		do {
			list_t list;
			size_t i;
			size_t cnt = sockht_expire(&loop->sockht, expire_sockets, sizeof(expire_sockets) / sizeof(expire_sockets[0]));
			list_init(&list);
			for (i = 0; i < cnt; ++i) {
				list_insert_node_back(&list, list.tail, &expire_sockets[i]->m_msg.m_listnode);
			}
			dataqueuePushList(loop->msgdq, &list);
		} while (0);
		do {
			list_node_t *cur, *next;
			for (cur = dataqueuePop(&loop->dq, 0, ~0); cur; cur = next) {
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
						if (!reactorReg(&loop->reactor, s->fd))
							break;
						s->loop = loop;
						s->valid = 1;
						s->m_lastActiveTime = gmtimeSecond();
						if (s->connect_callback) {
							if (!reactorCommit(&loop->reactor, s->fd, REACTOR_CONNECT, &s->m_writeOl, &s->connect_saddr))
								break;
						}
						else if (!reactorsocket_read(s)) {
							break;
						}
						message->type = NIO_SOCKET_CLOSE_MESSAGE;
						hashtable_replace_node(hashtable_insert_node(&loop->sockht, &s->m_hashnode), &s->m_hashnode);
						reg_ok = 1;
					} while (0);
					if (!reg_ok) {
						niosocketFree(s);
					}
				}
			}
		} while (0);
	}

	return 0;
}

NioSocketLoop_t* niosocketloopCreate(NioSocketLoop_t* loop, DataQueue_t* msgdq) {
	if (!reactorCreate(&loop->reactor))
		return NULL;
	if (!dataqueueInit(&loop->dq)) {
		reactorClose(&loop->reactor);
		return NULL;
	}
	loop->valid = 1;
	loop->msgdq = msgdq;
	hashtable_init(&loop->sockht, loop->sockht_bulks, sizeof(loop->sockht_bulks) / sizeof(loop->sockht_bulks[0]),
			sockht_keycmp, sockht_keyhash);
	if (!threadCreate(&loop->handle, reactor_socket_loop_entry, loop)) {
		reactorClose(&loop->reactor);
		dataqueueDestroy(&loop->dq, NULL);
		return NULL;
	}
	return loop;
}

void niosocketloopAdd(NioSocketLoop_t* loop, NioSocket_t* s[], size_t n) {
	size_t i;
	list_t list;
	list_init(&list);
	for (i = 0; i < n; ++i) {
		s[i]->m_msg.type = NIO_SOCKET_REG_MESSAGE;
		list_insert_node_back(&list, list.tail, &s[i]->m_msg.m_listnode);
	}
	dataqueuePushList(&loop->dq, &list);
}

void niosocketloopJoin(NioSocketLoop_t* loop) {
	hashtable_node_t *cur, *next;
	threadJoin(loop->handle, NULL);
	reactorClose(&loop->reactor);
	dataqueueDestroy(&loop->dq, NULL);
	for (cur = hashtable_first_node(&loop->sockht); cur; cur = next) {
		next = cur->next;
		niosocketFree(pod_container_of(cur, NioSocket_t, m_hashnode));
	}
}

void niosocketmsgHandler(DataQueue_t* dq, int max_wait_msec, void (*user_msg_callback)(NioSocketMsg_t*)) {
	list_node_t* cur, *next;
	for (cur = dataqueuePop(dq, max_wait_msec, ~0); cur; cur = next) {
		NioSocketMsg_t* message = pod_container_of(cur, NioSocketMsg_t, m_listnode);
		next = cur->next;
		if (NIO_SOCKET_CLOSE_MESSAGE == message->type) {
			NioSocket_t* s = pod_container_of(message, NioSocket_t, m_msg);
			if (s->close)
				s->close(s);
			dataqueuePush(&s->loop->dq, &s->m_msg.m_listnode);
		}
		else if (NIO_SOCKET_USER_MESSAGE == message->type) {
			user_msg_callback(message);
		}
	}
}

void niosocketmsgClean(DataQueue_t* dq, void(*deleter)(NioSocketMsg_t*)) {
	list_node_t *cur, *next;
	for (cur = dataqueuePop(dq, 0, ~0); cur; cur = next) {
		NioSocketMsg_t* message = pod_container_of(cur, NioSocketMsg_t, m_listnode);
		next = cur->next;
		if (NIO_SOCKET_USER_MESSAGE == message->type)
			deleter(message);
	}
}

#ifdef __cplusplus
}
#endif
