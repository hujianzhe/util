//
// Created by hujianzhe on 16-8-14.
//

#include "../../c/syslib/error.h"
#include "nio_object.h"
#include <stdlib.h>

namespace Util {
NioObject::NioObject(FD_t fd, int domain, int socktype, int protocol, bool islisten) :
	m_fd(fd),
	m_domain(domain),
	m_socktype(socktype),
	m_protocol(protocol),
	m_isListen(islisten),
	m_valid(false),
	m_readOl(NULL),
	m_writeOl(NULL),
	m_reactor(NULL),
	m_lastActiveTime(0),
	m_timeoutSecond(INFTIM),
	m_readCommit(FALSE),
	session(NULL),
	closemsg(NULL)
{
}

NioObject::~NioObject(void) {
	sock_Close(m_fd);
	free(m_readOl);
	free(m_writeOl);
}

bool NioObject::reg(Reactor_t* reactor) {
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
	int opcode;
	if (m_isListen) {
		opcode = REACTOR_ACCEPT;
		saddr.ss_family = m_domain;
	}
	else {
		opcode = REACTOR_READ;
	}
	if (reactor_Commit(m_reactor, m_fd, opcode, &m_readOl, &saddr)) {
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
	if (!m_isListen) {
		if (SOCK_STREAM == m_socktype) {
			sock_Shutdown(m_fd, SHUT_WR);
			//reactorRead();
		}
	}
	m_valid = false;
}

int NioObject::onRead(void) {
	int res = read();
	if (m_valid) {
		m_lastActiveTime = gmt_second();
		_xchg8(&m_readCommit, FALSE);
		reactorRead();
	}
	return res;
}
}
