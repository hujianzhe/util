//
// Created by hujianzhe on 16-8-14.
//

#include "../../c/syslib/error.h"
#include "nio_object.h"
#include <stdlib.h>

namespace Util {
NioObject::NioObject(FD_t fd, int domain, int socktype, int protocol, bool islisten) :
	fd(fd),
	domain(domain),
	socktype(socktype),
	protocol(protocol),
	isListen(islisten),
	valid(false),
	m_readCommit(FALSE),
	m_readOl(NULL),
	m_writeOl(NULL),
	m_reactor(NULL),
	m_lastActiveTime(0),
	timeout_second(INFTIM),
	session(NULL),
	closemsg(NULL)
{
}

NioObject::~NioObject(void) {
	sock_Close(fd);
	free(m_readOl);
	free(m_writeOl);
}

bool NioObject::reg(Reactor_t* reactor) {
	if (!sock_NonBlock(fd, TRUE)) {
		return false;
	}
	if (!reactor_Reg(reactor, fd)) {
		return false;
	}
	m_reactor = reactor;
	valid = true;
	m_lastActiveTime = gmt_second();
	return true;
}

bool NioObject::reactorRead(void) {
	if (_xchg8(&m_readCommit, TRUE)) {
		return true;
	}
	struct sockaddr_storage saddr;
	int opcode;
	if (isListen) {
		opcode = REACTOR_ACCEPT;
		saddr.ss_family = domain;
	}
	else {
		opcode = REACTOR_READ;
	}
	if (reactor_Commit(m_reactor, fd, opcode, &m_readOl, &saddr)) {
		return true;
	}
	valid = false;
	return false;
}
bool NioObject::reactorWrite(void) {
	struct sockaddr_storage saddr;
	if (reactor_Commit(m_reactor, fd, REACTOR_WRITE, &m_writeOl, &saddr)) {
		return true;
	}
	valid = false;
	return false;
}

void NioObject::shutdownWaitAck(void) {
	if (!isListen) {
		if (SOCK_STREAM == socktype) {
			sock_Shutdown(fd, SHUT_WR);
			//reactorRead();
		}
	}
	valid = false;
}

int NioObject::onRead(void) {
	int res = read();
	if (valid) {
		m_lastActiveTime = gmt_second();
		_xchg8(&m_readCommit, FALSE);
		reactorRead();
	}
	return res;
}
}
