//
// Created by hujianzhe on 16-9-7.
//

#include "../../c/syslib/error.h"
#include "tcp_nio_object.h"
#include <stdlib.h>
#include <string.h>
#include <exception>
#include <stdexcept>

namespace Util {
TcpNioObject::TcpNioObject(FD_t fd, int domain) :
	NioObject(fd, domain, SOCK_STREAM, 0, false),
	m_connectcallback(NULL),
	m_writeCommit(false),
	m_inbuf(NULL),
	m_inbuflen(0)
{
	if (!mutex_Create(&m_outbufMutex)) {
		throw std::logic_error("Util::TcpNioObject mutex_Create failure");
	}
	list_init(&m_outbuflist);
}
TcpNioObject::~TcpNioObject(void) {
	mutex_Close(&m_outbufMutex);
	free(m_inbuf);
}

bool TcpNioObject::reactorConnect(int family, const char* ip, unsigned short port, bool(*callback)(TcpNioObject*, int)) {
	struct sockaddr_storage saddr;
	if (!sock_Text2Addr(&saddr, family, ip, port)) {
		return false;
	}
	return reactorConnect(&saddr, callback);
}
bool TcpNioObject::reactorConnect(struct sockaddr_storage* saddr, bool(*callback)(TcpNioObject*, int)) {
	m_connectcallback = callback;
	if (reactor_Commit(m_reactor, m_fd, REACTOR_CONNECT, &m_writeOl, saddr)) {
		return true;
	}
	m_connectcallback = NULL;
	m_valid = false;
	return false;
}

int TcpNioObject::read(void) {
	struct sockaddr_storage saddr;
	int res = sock_TcpReadableBytes(m_fd);
	do {
		if (res <= 0) {
			m_valid = false;
			break;
		}

		unsigned char* p = (unsigned char*)realloc(m_inbuf, m_inbuflen + res);
		if (!p) {
			free(m_inbuf);
			m_inbuf = NULL;
			m_inbuflen = 0;
			m_valid = false;
			break;
		}
		m_inbuf = p;
		res = sock_Recv(m_fd, m_inbuf + m_inbuflen, res, 0, &saddr);
		if (res <= 0) {
			free(m_inbuf);
			m_inbuf = NULL;
			m_inbuflen = 0;
			m_valid = false;
			break;
		}
		else {
			m_inbuflen += res;
		}

		size_t offset = 0;
		while (offset < m_inbuflen) {
			int len = onRead(m_inbuf + offset, m_inbuflen - offset, &saddr);
			if (0 == len) {
				break;
			}
			if (len < 0) {
				m_valid = false;
				offset = m_inbuflen;
				break;
			}
			offset += len;
		}

		if (offset >= m_inbuflen) {
			free(m_inbuf);
			m_inbuf = NULL;
			m_inbuflen = 0;
		}
		else {
			size_t n = offset;
			for (size_t start = 0; offset < m_inbuflen; ++start, ++offset) {
				m_inbuf[start] = m_inbuf[offset];
			}
			m_inbuflen -= n;
		}
	} while (0);
	return res;
}

int TcpNioObject::sendv(IoBuf_t* iov, unsigned int iovcnt, struct sockaddr_storage* saddr) {
	if (!m_valid) {
		return -1;
	}
	if (!iov || !iovcnt) {
		return 0;
	}
	size_t nbytes = 0;
	for (unsigned int i = 0; i < iovcnt; ++i) {
		nbytes += iobuffer_len(iov + i);
	}
	if (0 == nbytes) {
		return 0;
	}

	int res = 0;
	mutex_Lock(&m_outbufMutex);

	do {
		if (!m_outbuflist.head) {
			res = sock_SendVec(m_fd, iov, iovcnt, 0, NULL);
			if (res < 0) {
				if (error_code() != EWOULDBLOCK) {
					m_valid = false;
					res = -1;
					break;
				}
				res = 0;
			}
		}
		if (res < nbytes) {
			WaitSendData* wsd = (WaitSendData*)malloc(sizeof(WaitSendData) + (nbytes - res));
			if (!wsd) {
				m_valid = false;
				res = -1;
				break;
			}
			wsd->len = nbytes - res;
			wsd->offset = 0;

			unsigned int i;
			size_t off;
			for (off = 0, i = 0; i < iovcnt; ++i) {
				if (res >= iobuffer_len(iov + i)) {
					res -= iobuffer_len(iov + i);
				}
				else {
					memcpy(wsd->data + off, ((char*)iobuffer_buf(iov + i)) + res, iobuffer_len(iov + i) - res);
					off += iobuffer_len(iov + i) - res;
					res = 0;
				}
			}

			list_insert_node_back(&m_outbuflist, m_outbuflist.tail, &wsd->m_listnode);
			//
			if (!m_writeCommit) {
				m_writeCommit = true;
				reactorWrite();
			}
		}
	} while (0);

	mutex_Unlock(&m_outbufMutex);

	return res;
}
int TcpNioObject::onWrite(void) {
	int count = 0;
	m_lastActiveTime = gmt_second();

	if (m_connectcallback) {
		bool ok = false;
		try {
			ok = m_connectcallback(this, reactor_ConnectCheckSuccess(m_fd) ? 0 : error_code());
		}
		catch (...) {}
		m_connectcallback = NULL;
		if (!ok) {
			m_valid = false;
			return -1;
		}
		return 0;
	}
	if (!m_valid) {
		return -1;
	}

	mutex_Lock(&m_outbufMutex);

	for (list_node_t* iter = m_outbuflist.head; iter; ) {
		WaitSendData* wsd = pod_container_of(iter, WaitSendData, m_listnode);
		int res = sock_Send(m_fd, wsd->data + wsd->offset, wsd->len - wsd->offset, 0, NULL);
		if (res < 0) {
			if (error_code() != EWOULDBLOCK) {
				m_valid = false;
				count = -1;
				break;
			}
			res = 0;
		}
		count += res;
		wsd->offset += res;
		if (wsd->offset >= wsd->len) {
			list_remove_node(&m_outbuflist, iter);
			iter = iter->next;
			free(wsd);
		}
		else if (m_valid) {
			reactorWrite();
			break;
		}
	}
	if (!m_outbuflist.head) {
		m_writeCommit = false;
	}

	mutex_Unlock(&m_outbufMutex);

	return count;
}
}
