//
// Created by hujianzhe on 16-12-1.
//

#ifndef	UTIL_CPP_NIO_TCPLISTEN_NIO_OBJECT_H
#define	UTIL_CPP_NIO_TCPLISTEN_NIO_OBJECT_H

#include "nio_object.h"

namespace Util {
class TcplistenNioObject : public NioObject {
public:
	TcplistenNioObject(FD_t sockfd, int sa_family);

	bool bindlisten(unsigned short port, REACTOR_ACCEPT_CALLBACK cbfunc = NULL, size_t arg = 0);
	bool bindlisten(const char* ip, unsigned short port, REACTOR_ACCEPT_CALLBACK cbfunc = NULL, size_t arg = 0);
	bool bindlisten(const struct sockaddr_storage* saddr, REACTOR_ACCEPT_CALLBACK cbfunc = NULL, size_t arg = 0);

	bool accept(int msec);
	bool reactorAccept(void);

private:
	virtual int recv(void);

private:
	int m_saFamily;
	REACTOR_ACCEPT_CALLBACK m_cbfunc;
	size_t m_arg;
};
}

#endif
