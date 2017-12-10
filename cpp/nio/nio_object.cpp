//
// Created by hujianzhe on 16-8-14.
//

#include "../../c/syslib/error.h"
#include "../../c/syslib/time.h"
#include "nio_object.h"

namespace Util {
NioObject::NioObject(const NioObject& o) : m_fd(INVALID_FD_HANDLE), m_socktype(-1) {}
NioObject& NioObject::operator=(const NioObject& o) { return *this; }
NioObject::NioObject(FD_HANDLE fd, int socktype) :
	m_fd(fd),
	m_socktype(-1 == socktype ? sock_Type(fd) : socktype),
	m_readOl(NULL),
	m_writeOl(NULL),
	m_reactor(NULL),
	m_valid(false),
	m_isListen(false),
	m_readCommit(FALSE),
	m_shutdown(1),
	m_createTime(gmt_Second()),
	m_lastActiveTime(0),
	m_timeoutSecond(INFTIM)
{
}

NioObject::~NioObject(void) {
	sock_Close(m_fd);
	free(m_readOl);
	free(m_writeOl);
}

FD_HANDLE NioObject::fd(void) const { return m_fd; }
int NioObject::socktype(void) const { return m_socktype; }
bool NioObject::isListen(void) const { return m_isListen; }

bool NioObject::reactorInit(REACTOR* reactor) {
	if (sock_NonBlock(m_fd, TRUE) != EXEC_SUCCESS) {
		return false;
	}
	if (reactor_Reg(reactor, m_fd) != EXEC_SUCCESS) {
		return false;
	}
	m_reactor = reactor;
	m_valid = true;
	m_lastActiveTime = gmt_Second();
	return true;
}

bool NioObject::reactorRead(void) {
	if (_xchg8(&m_readCommit, TRUE)) {
		return m_valid;
	}
	struct sockaddr_storage saddr;
	if (reactor_Commit(m_reactor, m_fd, REACTOR_READ, &m_readOl, &saddr) == EXEC_SUCCESS) {
		return true;
	}
	m_valid = false;
	return false;
}
bool NioObject::reactorWrite(void) {
	struct sockaddr_storage saddr;
	if (reactor_Commit(m_reactor, m_fd, REACTOR_WRITE, &m_writeOl, &saddr) == EXEC_SUCCESS) {
		return true;
	}
	m_valid = false;
	return false;
}

time_t NioObject::createTime(void) { return m_createTime; }
time_t NioObject::lastActiveTime(void) { return m_lastActiveTime; }
time_t NioObject::updateLastActiveTime(void) {
	m_lastActiveTime = gmt_Second();
	return m_lastActiveTime;
}

void NioObject::invalid(void) { m_valid = false; }

void NioObject::shutdownDirect(void) {
	if (!_xchg8(&m_shutdown, 0)) {
		return;
	}
	m_valid = false;
	onRemove();
}
void NioObject::shutdownWaitAck(void) {
	if (!m_isListen) {
		if (SOCK_STREAM == m_socktype) {
			sock_Shutdown(m_fd, SHUT_WR);
			reactorRead();
		}
	}
	shutdownDirect();
}

void NioObject::timeoutSecond(int sec) { m_timeoutSecond = sec; }
bool NioObject::checkTimeout(void) {
	return m_timeoutSecond >= 0 && gmt_Second() - m_timeoutSecond > m_lastActiveTime;
}
bool NioObject::checkValid(void) {
	if (m_valid && checkTimeout()) {
		m_valid = false;
		return false;
	}
	return m_valid;
}

bool NioObject::sendv(IO_BUFFER* iov, unsigned int iovcnt, struct sockaddr_storage* saddr) {
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

	if (sock_SendVec(m_fd, iov, iovcnt, 0, saddr) < 0) {
		if (error_code() != EWOULDBLOCK) {
			m_valid = false;
		}
	}
	return m_valid;
}

bool NioObject::send(const void* data, unsigned int nbytes, struct sockaddr_storage* saddr) {
	IO_BUFFER iov;
	iobuffer_buf(&iov) = (char*)data;
	iobuffer_len(&iov) = nbytes;
	return sendv(&iov, 1, saddr);
}
bool NioObject::send(const std::vector<unsigned char>& data, struct sockaddr_storage* saddr) {
	if (data.empty()) {
		return true;
	}
	return send(&data[0], data.size(), saddr);
}
bool NioObject::send(const std::vector<char>& data, struct sockaddr_storage* saddr) {
	if (data.empty()) {
		return true;
	}
	return send(&data[0], data.size(), saddr);
}
bool NioObject::send(const std::string& str, struct sockaddr_storage* saddr) {
	if (str.empty()) {
		return true;
	}
	return send(str.data(), str.size(), saddr);
}

int NioObject::recv(void) { return 0; }
int NioObject::onRead(void) {
	int res = recv();
	if (m_valid && !m_isListen) {
		_xchg8(&m_readCommit, FALSE);
		reactorRead();
	}
	return res;
}
int NioObject::onRead(IO_BUFFER inbuf, struct sockaddr_storage* from, size_t transfer_bytes) { return transfer_bytes; }
int NioObject::onWrite(void) { return 0; }
void NioObject::onRemove(void) {}
}
