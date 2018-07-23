//
// Created by hujianzhe on 16-8-14.
//

#include "../../c/syslib/error.h"
#include "../../c/syslib/time.h"
#include "nio_object.h"
#include <stdlib.h>

namespace Util {
NioObject::NioObject(FD_t fd, int domain, int socktype, int protocol) :
	m_fd(fd),
	m_domain(domain),
	m_socktype(-1 == socktype ? sock_Type(fd) : socktype),
	m_protocol(protocol),
	m_readOl(NULL),
	m_writeOl(NULL),
	m_reactor(NULL),
	m_valid(false),
	m_isListen(false),
	m_userdata(NULL),
	m_readCommit(FALSE),
	m_shutdown(1),
	m_createTime(gmt_second()),
	m_lastActiveTime(0),
	m_timeoutSecond(INFTIM)
{
}

NioObject::~NioObject(void) {
	sock_Close(m_fd);
	free(m_readOl);
	free(m_writeOl);
}

void NioObject::updateLastActiveTime(void) {
	m_lastActiveTime = gmt_second();
}

bool NioObject::reactorInit(Reactor_t* reactor) {
	if (!sock_NonBlock(m_fd, TRUE)) {
		return false;
	}
	if (!reactor_Reg(reactor, m_fd)) {
		return false;
	}
	m_reactor = reactor;
	m_valid = true;
	m_lastActiveTime = gmt_second();
	return true;
}

bool NioObject::reactorRead(void) {
	if (_xchg8(&m_readCommit, TRUE)) {
		return m_valid;
	}
	struct sockaddr_storage saddr;
	if (reactor_Commit(m_reactor, m_fd, REACTOR_READ, &m_readOl, &saddr)) {
		return true;
	}
	m_valid = false;
	return false;
}
bool NioObject::reactorWrite(void) {
	struct sockaddr_storage saddr;
	if (reactor_Commit(m_reactor, m_fd, REACTOR_WRITE, &m_writeOl, &saddr)) {
		return true;
	}
	m_valid = false;
	return false;
}

void NioObject::shutdownWaitAck(void) {
	if (!_xchg8(&m_shutdown, 0)) {
		return;
	}
	if (!m_isListen) {
		if (SOCK_STREAM == m_socktype) {
			sock_Shutdown(m_fd, SHUT_WR);
			//reactorRead();
		}
	}
	m_valid = false;
}

bool NioObject::checkTimeout(void) {
	return m_timeoutSecond >= 0 && gmt_second() - m_timeoutSecond > m_lastActiveTime;
}
bool NioObject::checkValid(void) {
	return m_valid && !checkTimeout();
}

int NioObject::onRead(void) {
	int res = recv();
	if (m_valid && !m_isListen) {
		_xchg8(&m_readCommit, FALSE);
		reactorRead();
	}
	return res;
}
}
