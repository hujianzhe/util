//
// Created by hujianzhe on 16-9-7.
//

#ifndef	UTIL_CPP_PROTOCOL_LENGTHFIELD_PACKET_WORKER_H
#define UTIL_CPP_PROTOCOL_LENGTHFIELD_PACKET_WORKER_H

#include "../nio/nio_packet_worker.h"
#include "lengthfield_frame.h"

namespace Util {
class LengthFieldPacketWorker : public NioPacketWorker {
public:
	LengthFieldPacketWorker(short length_field_size, unsigned int frame_length_limit);

	short lengthFieldSize(void) const { return m_lengthFieldSize; }

private:
	int onParsePacket(unsigned char* buf, size_t buflen, struct sockaddr_storage* from);
	virtual bool onRecvPacket(unsigned char* data, size_t len, struct sockaddr_storage* from) { return true; }

private:
	const short m_lengthFieldSize;
	const unsigned int m_frameLengthLimit;
};
}

#endif
