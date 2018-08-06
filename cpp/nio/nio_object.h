//
// Created by hujianzhe on 16-8-14.
//

#ifndef	UTIL_CPP_NIO_NIO_OBJECT_H
#define	UTIL_CPP_NIO_NIO_OBJECT_H

#include "../../c/syslib/atomic.h"
#include "../../c/syslib/io.h"
#include "../../c/syslib/socket.h"
#include "../../c/syslib/time.h"
#include <vector>

namespace Util {
class NioObjectManager;
class NioObject {
friend class NioObjectManager;
public:
	NioObject(FD_t fd, int domain, int socktype, int protocol, bool islisten);
	virtual ~NioObject(void);

	FD_t fd(void) const { return m_fd; }
	int domain(void) const { return m_domain; }
	int socktype(void) const { return m_socktype; }
	int protocol(void) const { return m_protocol; }
	bool isListen(void) const { return m_isListen; }

	bool reactorInit(Reactor_t* reactor);
	bool reactorRead(void);

	int onRead(void);
	virtual int onWrite(void) { return 0; }

	void invalid(void) { m_valid = false; }

	void shutdownWaitAck(void);

	void timeoutSecond(int sec) { m_timeoutSecond = sec; }
	bool checkTimeout(void) {
		return m_timeoutSecond >= 0 && gmt_second() - m_timeoutSecond > m_lastActiveTime;
	}
	bool checkValid(void) {
		return m_valid && !checkTimeout();
	}

	virtual int sendv(IoBuf_t* iov, unsigned int iovcnt, struct sockaddr_storage* saddr = NULL) = 0;
	int sendv(IoBuf_t iov, struct sockaddr_storage* saddr = NULL) { return sendv(&iov, 1, saddr); }
	virtual int sendpacket(IoBuf_t* iov, unsigned int iovcnt, struct sockaddr_storage* saddr = NULL) { return sendv(iov, iovcnt, saddr); }
	int sendpacket(IoBuf_t iov, struct sockaddr_storage* saddr = NULL) { return sendpacket(&iov, 1, saddr); }

private:
	NioObject(const NioObject& o) : m_fd(INVALID_FD_HANDLE), m_domain(AF_UNSPEC), m_socktype(-1), m_protocol(0), m_isListen(false) {}
	NioObject& operator=(const NioObject& o) { return *this; }

	virtual int read(void) = 0;

protected:
	virtual int onRead(unsigned char* buf, size_t len, struct sockaddr_storage* from) = 0; //{ return len > 0x7fffffff ? 0x7fffffff : (int)len; }
	bool reactorWrite(void);

protected:
	const FD_t m_fd;
	const int m_domain;
	const int m_socktype;
	const int m_protocol;
	const bool m_isListen;
	volatile bool m_valid;
	void* m_readOl;
	void* m_writeOl;
	Reactor_t* m_reactor;
	time_t m_lastActiveTime;
	int m_timeoutSecond;
private:
	Atom8_t m_readCommit;

public:
	void* session;// user define session structure
	void* closemsg;// user define message structrue
};
}

#endif //UTIL_NIO_OBJECT_H
