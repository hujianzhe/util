//
// Created by hujianzhe on 16-11-11.
//

#ifndef	UTIL_CPP_NIO_TCP_NIO_OBJECT_H
#define	UTIL_CPP_NIO_TCP_NIO_OBJECT_H

#include "nio_object.h"
#include <functional>
#include <list>

namespace Util {
class TcpNioObject : public NioObject {
public:
	TcpNioObject(FD_t fd);
	~TcpNioObject(void);

	bool reactorConnect(int family, const char* ip, unsigned short port, const std::function<bool(TcpNioObject*, bool)>& cb = nullptr);
	bool reactorConnect(struct sockaddr_storage* saddr, const std::function<bool(TcpNioObject*, bool)>& cb = nullptr);

	virtual bool sendv(IoBuf_t* iov, unsigned int iovcnt, struct sockaddr_storage* saddr = NULL);

private:
	bool onConnect(void);
	virtual bool onConnect(bool success);
	virtual int onWrite(void);

	int inbufRead(unsigned int nbytes, struct sockaddr_storage* saddr);
	void inbufRemove(unsigned int nbytes);
	IoBuf_t inbuf(void);
	virtual int recv(void);

private:
	std::function<bool(TcpNioObject*, bool)> m_connectCallback;
	volatile bool m_connecting;

	std::vector<unsigned char> m_inbuf;
	Mutex_t m_outbufMutex;
	bool m_outbufMutexInitOk;
	bool m_writeCommit;
	struct __NioSendDataInfo {
		std::vector<unsigned char> data;
		size_t offset;
		struct sockaddr_storage saddr;
	};
	std::list<struct __NioSendDataInfo> m_outbuf;
};
}

#endif
