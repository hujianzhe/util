//
// Created by hujianzhe on 16-12-1.
//

#include "../../c/syslib/file.h"
#include "../../c/syslib/error.h"
#include "tcplisten_nio_object.h"

namespace Util {
TcplistenNioObject::TcplistenNioObject(FD_t sockfd, int domain) :
	NioObject(sockfd, domain, SOCK_STREAM, 0),
	m_cbfunc(NULL),
	m_arg(0)
{
	m_isListen = true;
}

bool TcplistenNioObject::bindlisten(const char* ip, unsigned short port, REACTOR_ACCEPT_CALLBACK cbfunc, size_t arg) {
	struct sockaddr_storage saddr;
	if (!sock_Text2Addr(&saddr, m_domain, ip, port)) {
		return false;
	}
	return bindlisten(&saddr, cbfunc, arg);
}
bool TcplistenNioObject::bindlisten(const struct sockaddr_storage* saddr, REACTOR_ACCEPT_CALLBACK cbfunc, size_t arg) {
	if (!sock_BindSockaddr(m_fd, saddr)) {
		return false;
	}
	if (!sock_TcpListen(m_fd)) {
		return false;
	}
	m_cbfunc = cbfunc;
	m_arg = arg;
	return true;
}

bool TcplistenNioObject::accept(int msec) {
	struct sockaddr_storage saddr;
	FD_t connfd = sock_TcpAccept(m_fd, msec, &saddr);
	if (connfd != INVALID_FD_HANDLE) {
		if (m_cbfunc) {
			m_cbfunc(connfd, &saddr, m_arg);
		}
		return true;
	}
	if (error_code() == EWOULDBLOCK || error_code() == ENFILE || error_code() == EMFILE || error_code() == ECONNABORTED) {
		return true;
	}
	invalid();
	return false;
}

bool TcplistenNioObject::reactorAccept(void) {
	struct sockaddr_storage saddr;
	saddr.ss_family = m_domain;
	if (reactor_Commit(m_reactor, m_fd, REACTOR_ACCEPT, &m_readOl, &saddr)) {
		return true;
	}
	m_valid = false;
	return false;
}

int TcplistenNioObject::recv(void) {
	if (reactor_Accept(m_fd, m_readOl, m_cbfunc, m_arg)) {
		reactorAccept();
	}
	else {
		invalid();
	}
	return 0;
}
}
