//
// Created by hujianzhe on 16-11-11.
//

#ifndef	UTIL_CPP_NIO_TCP_NIO_OBJECT_H
#define	UTIL_CPP_NIO_TCP_NIO_OBJECT_H

#include "../../c/datastruct/list.h"
#include "../../c/syslib/ipc.h"
#include "nio_object.h"

namespace Util {
class TcpNioObject : public NioObject {
public:
	TcpNioObject(FD_t fd, int domain);
	~TcpNioObject(void);

	bool reactorConnect(int family, const char* ip, unsigned short port, bool(*callback)(TcpNioObject*, bool));
	bool reactorConnect(struct sockaddr_storage* saddr, bool(*callback)(TcpNioObject*, bool));

protected:
	virtual int sendv(IoBuf_t* iov, unsigned int iovcnt, struct sockaddr_storage* saddr = NULL);

private:
	virtual int onWrite(void);

	int inbufRead(unsigned int nbytes, struct sockaddr_storage* saddr);
	void inbufRemove(unsigned int nbytes);
	virtual int read(void);

private:
	bool(*m_connectcallback)(TcpNioObject*, bool);
	volatile bool m_connecting;

	std::vector<unsigned char> m_inbuf;

	bool m_writeCommit;
	Mutex_t m_outbufMutex;
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
