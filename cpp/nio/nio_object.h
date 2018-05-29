//
// Created by hujianzhe on 16-8-14.
//

#ifndef	UTIL_CPP_NIO_NIO_OBJECT_H
#define	UTIL_CPP_NIO_NIO_OBJECT_H

#include "../../c/syslib/atomic.h"
#include "../../c/syslib/io.h"
#include "../../c/syslib/socket.h"

#include <memory>
#include <string>
#include <vector>

namespace Util {
class NioObjectManager;
class NioObject : public std::enable_shared_from_this<NioObject> {
friend class NioObjectManager;
public:
	NioObject(FD_t fd, int domain, int socktype, int protocol);
	virtual ~NioObject(void);

	void userdata(void* userdata) { m_userdata = userdata; }
	void* userdata(void) const { return m_userdata; }

	FD_t fd(void) const { return m_fd; }
	int domain(void) const { return m_domain; }
	int socktype(void) const { return m_socktype; }
	int protocol(void) const { return m_protocol; }
	bool isListen(void) const { return m_isListen; }

	bool reactorInit(Reactor_t* reactor);
	bool reactorRead(void);

	time_t createTime(void) { return m_createTime; }
	time_t lastActiveTime(void) { return m_lastActiveTime; }

	void invalid(void) { m_valid = false; }

	void shutdownDirect(void);
	void shutdownWaitAck(void);

	void timeoutSecond(int sec) { m_timeoutSecond = sec; }
	bool checkTimeout(void);
	bool checkValid(void);

	virtual int send(IoBuf_t* iov, unsigned int iovcnt, struct sockaddr_storage* saddr = NULL) { return sendv(iov, iovcnt, saddr); }
	int send(const void* data, unsigned int nbytes, struct sockaddr_storage* saddr = NULL);
	int send(const std::vector<unsigned char>& data, struct sockaddr_storage* saddr = NULL) { return send(data.empty() ? NULL : &data[0], data.size(), saddr); }
	int send(const std::vector<char>& data, struct sockaddr_storage* saddr = NULL) { return send(data.empty() ? NULL : &data[0], data.size(), saddr); }
	int send(const std::string& str, struct sockaddr_storage* saddr = NULL) { return send(str.empty() ? NULL : str.data(), str.size(), saddr); }
	virtual bool sendmsg(const void* msg, struct sockaddr_storage* saddr = NULL) { return true; }

private:
	NioObject(const NioObject& o) : m_fd(INVALID_FD_HANDLE), m_domain(AF_UNSPEC), m_socktype(-1), m_protocol(0) {}
	NioObject& operator=(const NioObject& o) { return *this; }

	virtual int recv(void) { return 0; }

	int onRead(void);
	virtual int onWrite(void) { return 0; }
	virtual void onRemove(void) {}

	time_t updateLastActiveTime(void);

protected:
	virtual int sendv(IoBuf_t* iov, unsigned int iovcnt, struct sockaddr_storage* saddr = NULL);
	virtual int onRead(unsigned char* buf, size_t len, struct sockaddr_storage* from) = 0; //{ return len > 0x7fffffff ? 0x7fffffff : (int)len; }
	bool reactorWrite(void);

protected:
	const FD_t m_fd;
	const int m_domain;
	const int m_socktype;
	const int m_protocol;
	void* m_readOl;
	void* m_writeOl;
	Reactor_t* m_reactor;
	volatile bool m_valid;
	bool m_isListen;
	void* m_userdata;

private:
	Atom8_t m_readCommit;
	Atom8_t m_shutdown;
	//
	time_t m_createTime;
	time_t m_lastActiveTime;
	int m_timeoutSecond;
};
}

#endif //UTIL_NIO_OBJECT_H
