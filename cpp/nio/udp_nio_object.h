//
// Created by hujianzhe on 16-11-11.
//

#ifndef	UTIL_CPP_NIO_UDP_NIO_OBJECT_H
#define	UTIL_CPP_NIO_UDP_NIO_OBJECT_H

#include "nio_object.h"

namespace Util {
class UdpNioObject : public NioObject {
public:
	UdpNioObject(FD_t sockfd, unsigned short frame_length_limit);

	unsigned short frameLengthLimit(void) const { return m_frameLengthLimit; }

	virtual int recv(void);
	virtual int onParsePacket(unsigned char* buf, size_t buflen, struct sockaddr_storage* from) { return buflen; }
	virtual void onParseEmptyPacket(struct sockaddr_storage* from) {}

protected:
	const unsigned short m_frameLengthLimit;
};
}

#endif