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

	bool reactorConnect(int family, const char* ip, unsigned short port, bool(*callback)(TcpNioObject*, int));
	bool reactorConnect(struct sockaddr_storage* saddr, bool(*callback)(TcpNioObject*, int));

protected:
	virtual int sendv(IoBuf_t* iov, unsigned int iovcnt, struct sockaddr_storage* saddr = NULL);

private:
	virtual int read(void);
	virtual int onWrite(void);

private:
	bool(*m_connectcallback)(TcpNioObject*, int);
	bool m_writeCommit;

	unsigned char* m_inbuf;
	size_t m_inbuflen;

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
