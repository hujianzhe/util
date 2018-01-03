//
// Created by hujianzhe on 16-9-7.
//

#ifndef	UTIL_CPP_NIO_LENGTHFIELD_NIO_OBJECT_H
#define UTIL_CPP_NIO_LENGTHFIELD_NIO_OBJECT_H

#include "tcp_nio_object.h"

namespace Util {
class LengthFieldNioObject : public TcpNioObject {
public:
	LengthFieldNioObject(FD_t fd, short length_field_size, size_t frame_length_limit);

	short lengthFieldSize(void) const { return m_lengthFieldSize; }
	size_t frameLengthLimit(void) const { return m_frameLengthLimit; }

private:
	int onRead(IoBuf_t inbuf, struct sockaddr_storage* from, size_t transfer_bytes);
	virtual bool onRead(unsigned char* data, size_t len, struct sockaddr_storage* from) { return true; }

private:
	const short m_lengthFieldSize;
	const size_t m_frameLengthLimit;
};
}

#endif
