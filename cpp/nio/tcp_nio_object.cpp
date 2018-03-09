//
// Created by hujianzhe on 16-9-7.
//

#include "../../c/syslib/error.h"
#include "tcp_nio_object.h"
#include "nio_packet_worker.h"
#include <string.h>
#include <exception>
#include <stdexcept>

namespace Util {
TcpNioObject::TcpNioObject(FD_t fd, unsigned int frame_length_limit) :
	NioObject(fd, SOCK_STREAM),
	m_frameLengthLimit(frame_length_limit),
	m_connectCallbackFunctor(NULL),
	m_connecting(false),
	m_outbufMutexInitOk(false),
	m_writeCommit(false)
{
	if (mutex_Create(&m_outbufMutex) != EXEC_SUCCESS) {
		throw std::logic_error("Util::TcpNioObject mutex_Create failure");
	}
	m_outbufMutexInitOk = true;
	list_init(&m_outbuflist);
}
TcpNioObject::~TcpNioObject(void) {
	if (m_outbufMutexInitOk) {
		mutex_Close(&m_outbufMutex);
	}
}

#if __CPP_VERSION >= 2011
bool TcpNioObject::reactorConnect(int family, const char* ip, unsigned short port, const std::function<bool(TcpNioObject*, bool)>& cb) {
	struct sockaddr_storage saddr;
	if (sock_Text2Addr(&saddr, family, ip, port) != EXEC_SUCCESS) {
		return false;
	}
	return reactorConnect(&saddr, cb);
}
bool TcpNioObject::reactorConnect(struct sockaddr_storage* saddr, const std::function<bool(TcpNioObject*, bool)>& cb) {
	m_connecting = true;
	m_connectCallback = cb;
	if (reactor_Commit(m_reactor, m_fd, REACTOR_CONNECT, &m_writeOl, saddr) == EXEC_SUCCESS) {
		return true;
	}
	m_connecting = false;
	m_valid = false;
	return false;
}
#endif
bool TcpNioObject::reactorConnect(int family, const char* ip, unsigned short port, ConnectFunctor* cb) {
	struct sockaddr_storage saddr;
	if (sock_Text2Addr(&saddr, family, ip, port) != EXEC_SUCCESS) {
		return false;
	}
	return reactorConnect(&saddr, cb);
}
bool TcpNioObject::reactorConnect(struct sockaddr_storage* saddr, ConnectFunctor* cb) {
	m_connecting = true;
	m_connectCallbackFunctor = cb;
	if (reactor_Commit(m_reactor, m_fd, REACTOR_CONNECT, &m_writeOl, saddr) == EXEC_SUCCESS) {
		return true;
	}
	m_connecting = false;
	m_valid = false;
	return false;
}

bool TcpNioObject::onConnect(void) {
	m_connecting = false;
	bool ok = false;
	bool connect_ok = reactor_ConnectCheckSuccess(m_fd);
	try {
#if __CPP_VERSION >= 2011
		if (m_connectCallback) {
			ok = m_connectCallback(this, connect_ok);
			m_connectCallback = nullptr;//std::function<bool(NioObject*, bool)>();
		}
		else if (m_connectCallbackFunctor) {
#else
		if (m_connectCallbackFunctor) {
#endif
			ok = (*m_connectCallbackFunctor)(this, connect_ok);
			delete m_connectCallbackFunctor;
			m_connectCallbackFunctor = NULL;
		}
		else {
			ok = onConnect(connect_ok);
		}
	}
	catch (...) {}
	return ok;
}

int TcpNioObject::inbufRead(unsigned int nbytes, struct sockaddr_storage* saddr) {
	size_t offset = m_inbuf.size();
	m_inbuf.resize(offset + nbytes);
	int res = sock_Recv(m_fd, &m_inbuf[offset], nbytes, 0, saddr);
	if (res > 0) {
		m_inbuf.resize(offset + res);
	}
	return res;
}
void TcpNioObject::inbufRemove(unsigned int nbytes) {
	if (m_inbuf.size() <= nbytes) {
		std::vector<unsigned char>().swap(m_inbuf);
	}
	else {
		m_inbuf.erase(m_inbuf.begin(), m_inbuf.begin() + nbytes);
	}
}

int TcpNioObject::recv(void) {
	struct sockaddr_storage saddr;
	int res = sock_TcpReadableBytes(m_fd);
	do {
		if (res <= 0) {
			m_valid = false;
			break;
		}
		res = inbufRead(res, &saddr);
		if (res <= 0) {
			m_valid = false;
			break;
		}
		size_t offset = 0;
		while (offset < m_inbuf.size()) {
			ssize_t len = m_packetWorker->onParsePacket(&m_inbuf[offset], m_inbuf.size() - offset, &saddr);
			if (0 == len) {
				break;
			}
			if (len < 0) {
				m_valid = false;
				offset = m_inbuf.size();
				break;
			}
			offset += len;
		}
		inbufRemove(offset);
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
			res = sock_SendVec(m_fd, iov, iovcnt, 0, saddr);
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
	if (m_connecting) {
		if (!onConnect()) {
			m_valid = false;
			return 0;
		}
	}
	if (!m_valid) {
		return 0;
	}

	mutex_Lock(&m_outbufMutex);

	for (list_node_t* iter = m_outbuflist.head; iter; ) {
		WaitSendData* wsd = field_container(iter, WaitSendData, m_listnode);
		int res = sock_Send(m_fd, wsd->data + wsd->offset, wsd->len - wsd->offset, 0, NULL);
		if (res < 0) {
			if (error_code() != EWOULDBLOCK) {
				m_valid = false;
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
