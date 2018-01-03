//
// Created by hujianzhe on 17-7-17.
//

#include "fixedfield_nio_object.h"

namespace Util {
FixedFieldNioObject::FixedFieldNioObject(FD_t fd, size_t fixed_field_size) :
	TcpNioObject(fd),
	m_fixedFieldSize(fixed_field_size)
{
}

int FixedFieldNioObject::onRead(IoBuf_t inbuf, struct sockaddr_storage* from, size_t transfer_bytes) {
	size_t offset = 0;
	unsigned char* data = (unsigned char*)iobuffer_buf(&inbuf);
	size_t data_len = iobuffer_len(&inbuf);
	do {
		if (data_len - offset < m_fixedFieldSize) {
			break;
		}
		if (onRead(data + offset, m_fixedFieldSize, from)) {
			offset += m_fixedFieldSize;
		}
		else {
			invalid();
			offset = data_len;
			break;
		}
	} while (1);
	return offset;
}

bool FixedFieldNioObject::onRead(unsigned char* data, size_t len, struct sockaddr_storage* from) {
	return true;
}
}
