//
// Created by hujianzhe on 17-7-17.
//

#include "fixedfield_packet_worker.h"

namespace Util {
FixedFieldPacketWorker::FixedFieldPacketWorker(size_t fixed_field_size) :
	m_fixedFieldSize(fixed_field_size)
{
}

int FixedFieldPacketWorker::onParsePacket(unsigned char* buf, size_t buflen, struct sockaddr_storage* from) {
	if (buflen < m_fixedFieldSize) {
		return 0;
	}
	else {
		return onRecvPacket(buf, m_fixedFieldSize, from) ? (int)m_fixedFieldSize : -1;
	}
}
}
