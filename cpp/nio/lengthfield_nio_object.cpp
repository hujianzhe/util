//
// Created by hujianzhe on 16-9-7.
//

#include "lengthfield_nio_object.h"
#include "../protocol/lengthfield_frame.h"

namespace Util {
LengthFieldNioObject::LengthFieldNioObject(FD_t fd, short length_field_size, size_t frame_length_limit) :
	TcpNioObject(fd),
	m_lengthFieldSize(length_field_size),
	m_frameLengthLimit(frame_length_limit)
{
}

int LengthFieldNioObject::onRead(IoBuf_t inbuf, struct sockaddr_storage* from, size_t transfer_bytes) {
	size_t offset = 0;
	unsigned char* data = (unsigned char*)iobuffer_buf(&inbuf);
	size_t data_len = iobuffer_len(&inbuf);
	LengthFieldFrame protocol(m_lengthFieldSize, m_frameLengthLimit);
	do {
		int retcode = protocol.parseDataFrame(data + offset, data_len - offset);
		if (LengthFieldFrame::PARSE_OVERRANGE == retcode) {
			invalid();
			offset = data_len;
			break;
		}
		if (LengthFieldFrame::PARSE_INCOMPLETION == retcode) {
			break;
		}
		if (onRead(protocol.data(), protocol.dataLength(), from)) {
			offset += protocol.frameLength();
		}
		else {
			invalid();
			offset = data_len;
			break;
		}
	} while (1);
	return offset;
}
}
