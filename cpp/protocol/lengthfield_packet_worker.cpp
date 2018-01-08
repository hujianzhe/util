//
// Created by hujianzhe on 16-9-7.
//

#include "lengthfield_packet_worker.h"
#include "lengthfield_frame.h"

namespace Util {
LengthFieldPacketWorker::LengthFieldPacketWorker(short length_field_size, unsigned int frame_length_limit) :
	m_lengthFieldSize(length_field_size),
	m_frameLengthLimit(frame_length_limit)
{
}

int LengthFieldPacketWorker::onParsePacket(unsigned char* buf, size_t buflen, struct sockaddr_storage* from) {
	LengthFieldFrame protocol(m_lengthFieldSize, m_frameLengthLimit);
	int retcode = protocol.parseDataFrame(buf, buflen);
	if (LengthFieldFrame::PARSE_OVERRANGE == retcode) {
		return -1;
	}
	if (LengthFieldFrame::PARSE_INCOMPLETION == retcode) {
		return 0;
	}
	if (!onRecvPacket(protocol.data(), protocol.dataLength(), from)) {
		return -1;
	}
	return (int)protocol.frameLength();
}
}
