//
// Created by hujianzhe on 16-8-14.
//

#ifndef	UTIL_NIO_NIO_OBJECT_H
#define	UTIL_NIO_NIO_OBJECT_H

#include "../syslib/atomic.h"
#include "../syslib/io.h"
#include "../syslib/ipc.h"
#include "../syslib/socket.h"

#include <memory>
#include <string>
#include <vector>

namespace Util {
class NioWorker;
class NioObjectManager;
class NioObject : public std::enable_shared_from_this<NioObject> {
friend class NioObjectManager;
public:
	NioObject(FD_HANDLE fd, int socktype = -1);
	virtual ~NioObject(void);

	FD_HANDLE fd(void) const;
	int socktype(void) const;
	bool isListen(void) const;

	bool reactorInit(REACTOR* reactor);
	bool reactorRead(void);

	time_t createTime(void);
	time_t lastActiveTime(void);

	void invalid(void);

	void shutdownDirect(void);
	void shutdownWaitAck(void);

	void timeoutSecond(int sec);
	bool checkTimeout(void);
	bool checkValid(void);

	virtual bool sendv(IO_BUFFER* iov, unsigned int iovcnt, struct sockaddr_storage* saddr = NULL);
	virtual bool send(const void* data, unsigned int nbytes, struct sockaddr_storage* saddr = NULL);
	bool send(const std::vector<unsigned char>& data, struct sockaddr_storage* saddr = NULL);
	bool send(const std::vector<char>& data, struct sockaddr_storage* saddr = NULL);
	bool send(const std::string& str, struct sockaddr_storage* saddr = NULL);

private:
	NioObject(const NioObject& o);
	NioObject& operator=(const NioObject& o);

	virtual int recv(void);

	int onRead(void);
	virtual int onWrite(void);
	virtual void onRemove(void);

	time_t updateLastActiveTime(void);

protected:
	virtual int onRead(IO_BUFFER inbuf, struct sockaddr_storage* from, size_t transfer_bytes);
	bool reactorWrite(void);

protected:
	const FD_HANDLE m_fd;
	const int m_socktype;
	void* m_readOl;
	void* m_writeOl;
	REACTOR* m_reactor;
	volatile bool m_valid;
	bool m_isListen;

private:
	ATOM8 m_readCommit;
	ATOM8 m_shutdown;
	//
	time_t m_createTime;
	time_t m_lastActiveTime;
	int m_timeoutSecond;
};
}

#endif //UTIL_NIO_OBJECT_H
