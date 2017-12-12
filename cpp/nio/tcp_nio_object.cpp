//
// Created by hujianzhe on 16-9-7.
//

#include "../../c/syslib/error.h"
#include "tcp_nio_object.h"
#include <string.h>
#include <exception>
#include <stdexcept>

namespace Util {
TcpNioObject::TcpNioObject(FD_HANDLE fd) :
	NioObject(fd, SOCK_STREAM),
	m_connecting(false),
	m_outbufMutexInitOk(false),
	m_writeCommit(false)
{
	if (mutex_Create(&m_outbufMutex) != EXEC_SUCCESS) {
		throw std::logic_error("Util::TcpNioObject mutex_Create failure");
	}
	m_outbufMutexInitOk = true;
}
TcpNioObject::~TcpNioObject(void) {
	if (m_outbufMutexInitOk) {
		mutex_Close(&m_outbufMutex);
	}
}

bool TcpNioObject::reactorConnect(int family, const char* ip, unsigned short port, const std::function<bool(NioObject*, bool)>& cb) {
	struct sockaddr_storage saddr;
	if (sock_Text2Addr(&saddr, family, ip, port) != EXEC_SUCCESS) {
		return false;
	}
	m_connecting = true;
	m_connectCallback = cb;
	if (reactor_Commit(m_reactor, m_fd, REACTOR_CONNECT, &m_writeOl, &saddr) == EXEC_SUCCESS) {
		return true;
	}
	m_connecting = false;
	m_valid = false;
	return false;
}
bool TcpNioObject::reactorConnect(struct sockaddr_storage* saddr, const std::function<bool(NioObject*, bool)>& cb) {
	m_connecting = true;
	m_connectCallback = cb;
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
	try {
		if (m_connectCallback) {
			ok = m_connectCallback(this, reactor_ConnectCheckSuccess(m_fd));
			m_connectCallback = NULL;
		}
		else {
			ok = onConnect(reactor_ConnectCheckSuccess(m_fd));
		}
	}
	catch (...) {}
	return ok;
}
bool TcpNioObject::onConnect(bool success) { return success; }

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
IO_BUFFER TcpNioObject::inbuf(void) {
	IO_BUFFER __buf;
	iobuffer_len(&__buf) = m_inbuf.size();
	iobuffer_buf(&__buf) = m_inbuf.empty() ? NULL : (char*)&m_inbuf[0];
	return __buf;
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
		try {
			inbufRemove(onRead(inbuf(), &saddr, res));
		}
		catch (...) {
			m_valid = false;
			break;
		}
	} while (0);
	return res;
}

bool TcpNioObject::sendv(IO_BUFFER* iov, unsigned int iovcnt, struct sockaddr_storage* saddr) {
	if (!m_valid) {
		return false;
	}
	if (!iov || !iovcnt) {
		return true;
	}
	size_t nbytes = 0;
	for (unsigned int i = 0; i < iovcnt; ++i) {
		nbytes += iobuffer_len(iov + i);
	}
	if (0 == nbytes) {
		return true;
	}

	assert_true(mutex_Lock(&m_outbufMutex, TRUE) == EXEC_SUCCESS);
	do {
		int res = 0;
		if (m_outbuf.empty()) {
			res = sock_SendVec(m_fd, iov, iovcnt, 0, saddr);
			if (res < 0) {
				if (error_code() != EWOULDBLOCK) {
					m_valid = false;
					break;
				}
				res = 0;
			}
		}
		if (res < nbytes) {
			m_outbuf.push_back(__NioSendDataInfo());
			struct __NioSendDataInfo& nsdi = m_outbuf.back();
			if (saddr) {
				nsdi.saddr = *saddr;
			}
			else {
				nsdi.saddr.ss_family = AF_UNSPEC;
			}
			nsdi.offset = 0;
			nsdi.data.resize(nbytes - res);

			unsigned int i, off;
			for (off = 0, i = 0; i < iovcnt; ++i) {
				if (res >= iobuffer_len(iov + i)) {
					res -= iobuffer_len(iov + i);
				}
				else {
					memcpy(&nsdi.data[off], ((char*)iobuffer_buf(iov + i)) + res, iobuffer_len(iov + i) - res);
					off += iobuffer_len(iov + i) - res;
					res = 0;
				}
			}
			//
			if (!m_writeCommit) {
				m_writeCommit = true;
				reactorWrite();
			}
		}
	} while (0);
	assert_true(mutex_Unlock(&m_outbufMutex) == EXEC_SUCCESS);
	return m_valid;
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
	assert_true(mutex_Lock(&m_outbufMutex, TRUE) == EXEC_SUCCESS);
	for (std::list<struct __NioSendDataInfo>::iterator iter = m_outbuf.begin(); iter != m_outbuf.end(); ) {
		struct sockaddr_storage* saddr = (iter->saddr.ss_family != AF_UNSPEC ? &iter->saddr : NULL);
		int res = sock_Send(m_fd, &iter->data[0] + iter->offset, iter->data.size() - iter->offset, 0, saddr);
		if (res < 0) {
			if (error_code() != EWOULDBLOCK) {
				m_valid = false;
				break;
			}
			res = 0;
		}
		count += res;
		iter->offset += res;
		if (iter->offset >= iter->data.size()) {
			m_outbuf.erase(iter++);
		}
		else if (m_valid) {
			reactorWrite();
			break;
		}
	}
	if (m_outbuf.empty()) {
		m_writeCommit = false;
	}
	assert_true(mutex_Unlock(&m_outbufMutex) == EXEC_SUCCESS);
	return count;
}
}
