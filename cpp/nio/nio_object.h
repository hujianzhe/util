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
class NioObject {
public:
	NioObject(FD_t fd, int domain, int socktype, int protocol, bool islisten);
	virtual ~NioObject(void);

	bool reg(Reactor_t* reactor);
	bool reactorRead(void);

	int onRead(void);
	virtual int onWrite(void) { return 0; }

	void shutdownWaitAck(void);

	bool checkTimeout(void) {
		return timeout_second >= 0 && gmt_second() - timeout_second > m_lastActiveTime;
	}
	bool checkValid(void) {
		return valid && !checkTimeout();
	}

	virtual int sendv(IoBuf_t* iov, unsigned int iovcnt, struct sockaddr_storage* saddr = NULL) = 0;
	int sendv(IoBuf_t iov, struct sockaddr_storage* saddr = NULL) { return sendv(&iov, 1, saddr); }
	virtual int sendpacket(IoBuf_t* iov, unsigned int iovcnt, struct sockaddr_storage* saddr = NULL) { return sendv(iov, iovcnt, saddr); }
	int sendpacket(IoBuf_t iov, struct sockaddr_storage* saddr = NULL) { return sendpacket(&iov, 1, saddr); }

private:
	NioObject(const NioObject& o) : fd(INVALID_FD_HANDLE), domain(AF_UNSPEC), socktype(-1), protocol(0), isListen(false) {}
	NioObject& operator=(const NioObject& o) { return *this; }

	virtual int read(void) = 0;

protected:
	virtual int onRead(unsigned char* buf, size_t len, struct sockaddr_storage* from) = 0; //{ return len > 0x7fffffff ? 0x7fffffff : (int)len; }
	bool reactorWrite(void);

public:
	const FD_t fd;
	const int domain;
	const int socktype;
	const int protocol;
	const bool isListen;
	volatile bool valid;
private:
	Atom8_t m_readCommit;
protected:
	void* m_readOl;
	void* m_writeOl;
	Reactor_t* m_reactor;
	time_t m_lastActiveTime;
public:
	volatile int timeout_second;
	void* session;// user define session structure
	void* closemsg;// user define message structrue
};
}

#endif //UTIL_NIO_OBJECT_H
