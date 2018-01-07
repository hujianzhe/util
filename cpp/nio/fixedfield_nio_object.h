//
// Created by hujianzhe on 17-7-17.
//

#ifndef UTIL_CPP_NIO_FIXEDFIELD_NIO_OBJECT_H
#define	UTIL_CPP_NIO_FIXEDFIELD_NIO_OBJECT_H

#include "tcp_nio_object.h"

namespace Util {
class FixedFieldNioObject : public TcpNioObject {
public:
	FixedFieldNioObject(FD_t fd, size_t fixed_field_size);

	short fixedFieldSize(void) const { return m_fixedFieldSize; }

private:
	int onParsePacket(unsigned char* buf, size_t buflen, struct sockaddr_storage* from);
	virtual bool onRecvPacket(unsigned char* data, size_t len, struct sockaddr_storage* from) { return true; }

private:
	const size_t m_fixedFieldSize;
};
}

#endif // !UTIL_FIXEDFIELD_NIO_OBJECT_H
