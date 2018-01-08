//
// Created by hujianzhe on 17-7-17.
//

#ifndef UTIL_CPP_PROTOCOL_FIXEDFIELD_PACKET_WORKER_H
#define	UTIL_CPP_PROTOCOL_FIXEDFIELD_PACKET_WORKER_H

#include "../nio/nio_packet_worker.h"

namespace Util {
class FixedFieldPacketWorker : public NioPacketWorker {
public:
	FixedFieldPacketWorker(size_t fixed_field_size);

	size_t fixedFieldSize(void) const { return m_fixedFieldSize; }

private:
	int onParsePacket(unsigned char* buf, size_t buflen, struct sockaddr_storage* from);
	virtual bool onRecvPacket(unsigned char* data, size_t len, struct sockaddr_storage* from) { return true; }

private:
	const size_t m_fixedFieldSize;
};
}

#endif // !UTIL_CPP_PROTOCOL_FIXEDFIELD_PACKET_WORKER_H
