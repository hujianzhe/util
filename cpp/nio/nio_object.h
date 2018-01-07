//
// Created by hujianzhe on 16-8-14.
//

#ifndef	UTIL_CPP_NIO_NIO_OBJECT_H
#define	UTIL_CPP_NIO_NIO_OBJECT_H

#include "../../c/syslib/atomic.h"
#include "../../c/syslib/io.h"
#include "../../c/syslib/ipc.h"
#include "../../c/syslib/socket.h"

#include <memory>
#include <string>
#include <vector>

namespace Util {
class NioObjectManager;
class NioPacketWorker;
class NioObject : public std::enable_shared_from_this<NioObject> {
friend class NioObjectManager;
public:
	NioObject(FD_t fd, int socktype = -1);
	virtual ~NioObject(void);

	void userdata(void* userdata) { m_userdata = userdata; }
	void* userdata(void) { return m_userdata; }

	void packetWorker(NioPacketWorker* worker);

	FD_t fd(void) const { return m_fd; }
	int socktype(void) const { return m_socktype; }
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

	virtual bool sendv(IoBuf_t* iov, unsigned int iovcnt, struct sockaddr_storage* saddr = NULL);
	virtual bool send(const void* data, unsigned int nbytes, struct sockaddr_storage* saddr = NULL);
	bool send(const std::vector<unsigned char>& data, struct sockaddr_storage* saddr = NULL) { return send(data.empty() ? NULL : &data[0], data.size(), saddr); }
	bool send(const std::vector<char>& data, struct sockaddr_storage* saddr = NULL) { return send(data.empty() ? NULL : &data[0], data.size(), saddr); }
	bool send(const std::string& str, struct sockaddr_storage* saddr = NULL) { return send(str.empty() ? NULL : str.data(), str.size(), saddr); }

private:
	NioObject(const NioObject& o);
	NioObject& operator=(const NioObject& o);

	virtual int recv(void) { return 0; }

	int onRead(void);
	virtual int onWrite(void) { return 0; }
	virtual void onRemove(void) {}

	time_t updateLastActiveTime(void);

protected:
	bool reactorWrite(void);

protected:
	const FD_t m_fd;
	const int m_socktype;
	void* m_readOl;
	void* m_writeOl;
	Reactor_t* m_reactor;
	volatile bool m_valid;
	bool m_isListen;
	void* m_userdata;
	NioPacketWorker* m_packetWorker;

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
