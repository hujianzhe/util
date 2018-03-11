//
// Created by hujianzhe on 17-7-17.
//

#ifndef UTIL_CPP_PROTOCOL_FIXEDFIELD_PACKET_WORKER_H
#define	UTIL_CPP_PROTOCOL_FIXEDFIELD_PACKET_WORKER_H

#include "packet_worker.h"

namespace Util {
class FixedFieldPacketWorker : public PacketWorker {
public:
	FixedFieldPacketWorker(size_t fixed_field_size);

	size_t fixedFieldSize(void) const { return m_fixedFieldSize; }

private:
	virtual int onParsePacket(unsigned char* buf, size_t buflen, struct sockaddr_storage* from);

private:
	const size_t m_fixedFieldSize;
};
}

#endif // !UTIL_CPP_PROTOCOL_FIXEDFIELD_PACKET_WORKER_H
