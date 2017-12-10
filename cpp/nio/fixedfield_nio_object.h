//
// Created by hujianzhe on 17-7-17.
//

#ifndef UTIL_CPP_NIO_FIXEDFIELD_NIO_OBJECT_H
#define	UTIL_CPP_NIO_FIXEDFIELD_NIO_OBJECT_H

#include "tcp_nio_object.h"

namespace Util {
class FixedFieldNioObject : public TcpNioObject {
public:
	FixedFieldNioObject(FD_HANDLE fd, size_t fixed_field_size);

	short lengthFieldSize(void) const;

private:
	int onRead(IO_BUFFER inbuf, struct sockaddr_storage* from, size_t transfer_bytes);
	virtual bool onRead(unsigned char* data, size_t len, struct sockaddr_storage* from);

private:
	const size_t m_fixedFieldSize;
};
}

#endif // !UTIL_FIXEDFIELD_NIO_OBJECT_H
