//
// Created by hujianzhe on 16-11-11.
//

#ifndef	UTIL_CPP_NIO_TCP_NIO_OBJECT_H
#define	UTIL_CPP_NIO_TCP_NIO_OBJECT_H

#include "../../c/datastruct/list.h"
#include "../../c/syslib/ipc.h"
#include "../cpp_compiler_define.h"
#include "nio_object.h"
#if __CPP_VERSION >= 2011
#include <functional>
#endif

namespace Util {
class TcpNioObject : public NioObject {
public:
	TcpNioObject(FD_t fd);
	~TcpNioObject(void);

#if __CPP_VERSION >= 2011
	bool reactorConnect(int family, const char* ip, unsigned short port, const std::function<bool(TcpNioObject*, bool)>& cb = nullptr);
	bool reactorConnect(struct sockaddr_storage* saddr, const std::function<bool(TcpNioObject*, bool)>& cb = nullptr);
#endif
	class ConnectFunctor {
	public:
		virtual ~ConnectFunctor(void) {}
		virtual bool operator() (TcpNioObject*, bool) = 0;
	protected:
		ConnectFunctor(void) {} // forbid create on stack memory
	};
	bool reactorConnect(int family, const char* ip, unsigned short port, ConnectFunctor* cb = NULL);
	bool reactorConnect(struct sockaddr_storage* saddr, ConnectFunctor* cb = NULL);

	virtual int sendv(IoBuf_t* iov, unsigned int iovcnt, struct sockaddr_storage* saddr = NULL);

private:
	bool onConnect(void);
	virtual int onWrite(void);

	int inbufRead(unsigned int nbytes, struct sockaddr_storage* saddr);
	void inbufRemove(unsigned int nbytes);
	virtual int recv(void);

private:
#if __CPP_VERSION >= 2011
	std::function<bool(TcpNioObject*, bool)> m_connectCallback;
#endif
	ConnectFunctor* m_connectCallbackFunctor;
	volatile bool m_connecting;

	std::vector<unsigned char> m_inbuf;
	Mutex_t m_outbufMutex;
	bool m_outbufMutexInitOk;
	bool m_writeCommit;
	struct WaitSendData {
		list_node_t m_listnode;
		size_t offset;
		size_t len;
		unsigned char data[1];
	};
	list_t m_outbuflist;
};
}

#endif
