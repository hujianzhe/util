//
// Created by hujianzhe on 16-9-7.
//

#ifndef	UTIL_CPP_PROTOCOL_LENGTHFIELD_PACKET_WORKER_H
#define UTIL_CPP_PROTOCOL_LENGTHFIELD_PACKET_WORKER_H

#include "packet_worker.h"
#include "lengthfield_frame.h"

namespace Util {
class LengthFieldPacketWorker : public PacketWorker {
public:
	LengthFieldPacketWorker(unsigned short length_field_size, unsigned int frame_length_limit);

	unsigned short lengthFieldSize(void) const { return m_lengthFieldSize; }
	unsigned int frameLengthLimit(void) const { return m_frameLengthLimit; }

	virtual int onParsePacket(unsigned char* buf, size_t buflen, struct sockaddr_storage* from);

private:
	const unsigned short m_lengthFieldSize;
	const unsigned int m_frameLengthLimit;
};
}

#endif
