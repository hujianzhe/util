//
// Created by hujianzhe on 16-11-11.
//

#ifndef	UTIL_CPP_NIO_UDP_NIO_OBJECT_H
#define	UTIL_CPP_NIO_UDP_NIO_OBJECT_H

#include "nio_object.h"

namespace Util {
class UdpNioObject : public NioObject {
public:
	UdpNioObject(FD_t sockfd, int domain, unsigned short frame_length_limit);

	unsigned short frameLengthLimit(void) const { return m_frameLengthLimit; }

private:
	virtual int recv(void);

protected:
	virtual int sendv(IoBuf_t* iov, unsigned int iovcnt, struct sockaddr_storage* saddr = NULL);

protected:
	const unsigned short m_frameLengthLimit;
};
}

#endif