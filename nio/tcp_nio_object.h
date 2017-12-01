//
// Created by hujianzhe on 16-11-11.
//

#ifndef	UTIL_NIO_TCP_NIO_OBJECT_H
#define	UTIL_NIO_TCP_NIO_OBJECT_H

#include "nio_object.h"
#include <functional>
#include <list>

namespace Util {
class TcpNioObject : public NioObject {
public:
	TcpNioObject(FD_HANDLE fd);
	~TcpNioObject(void);

	bool reactorConnect(int family, const char* ip, unsigned short port, const std::function<bool(NioObject*, bool)>& cb = NULL);
	bool reactorConnect(struct sockaddr_storage* saddr, const std::function<bool(NioObject*, bool)>& cb = NULL);

	virtual bool sendv(IO_BUFFER* iov, unsigned int iovcnt, struct sockaddr_storage* saddr = NULL);

private:
	bool onConnect(void);
	virtual bool onConnect(bool success);
	virtual int onWrite(void);

	int inbufRead(unsigned int nbytes, struct sockaddr_storage* saddr);
	void inbufRemove(unsigned int nbytes);
	IO_BUFFER inbuf(void);
	virtual int recv(void);

private:
	std::function<bool(NioObject*, bool)> m_connectCallback;
	volatile bool m_connecting;

	std::vector<unsigned char> m_inbuf;
	MUTEX m_outbufMutex;
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
