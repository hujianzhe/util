//
// Created by hujianzhe on 16-12-1.
//

#include "../../c/syslib/file.h"
#include "../../c/syslib/error.h"
#include "tcplisten_nio_object.h"

namespace Util {
TcplistenNioObject::TcplistenNioObject(FD_t sockfd, int domain) :
	NioObject(sockfd, domain, SOCK_STREAM, 0, true),
	m_cbfunc(NULL),
	m_arg(0)
{
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

int TcplistenNioObject::read(void) {
	if (!reactor_Accept(m_fd, m_readOl, m_cbfunc, m_arg)) {
		invalid();
	}
	return 0;
}
}
