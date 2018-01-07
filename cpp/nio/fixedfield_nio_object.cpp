//
// Created by hujianzhe on 17-7-17.
//

#include "fixedfield_nio_object.h"

namespace Util {
FixedFieldNioObject::FixedFieldNioObject(FD_t fd, size_t fixed_field_size) :
	TcpNioObject(fd, INT_MAX),
	m_fixedFieldSize(fixed_field_size)
{
}

int FixedFieldNioObject::onParsePacket(unsigned char* buf, size_t buflen, struct sockaddr_storage* from) {
	if (buflen < m_fixedFieldSize) {
		return 0;
	}
	else {
		return onRecvPacket(buf, m_fixedFieldSize, from) ? m_fixedFieldSize : -1;
	}
}
}
