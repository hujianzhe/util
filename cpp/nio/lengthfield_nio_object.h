//
// Created by hujianzhe on 16-9-7.
//

#ifndef	UTIL_CPP_NIO_LENGTHFIELD_NIO_OBJECT_H
#define UTIL_CPP_NIO_LENGTHFIELD_NIO_OBJECT_H

#include "tcp_nio_object.h"

namespace Util {
class LengthFieldNioObject : public TcpNioObject {
public:
	LengthFieldNioObject(FD_t fd, short length_field_size, unsigned int frame_length_limit);

	short lengthFieldSize(void) const { return m_lengthFieldSize; }

private:
	int onParsePacket(unsigned char* buf, size_t buflen, struct sockaddr_storage* from);
	virtual bool onRecvPacket(unsigned char* data, size_t len, struct sockaddr_storage* from) { return true; }

private:
	const short m_lengthFieldSize;
};
}

#endif
