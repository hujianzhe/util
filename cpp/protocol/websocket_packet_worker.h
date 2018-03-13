//
// Created by hujianzhe on 16-9-11.
//

#ifndef	UTIL_CPP_PROTOCOL_WEBSOCKET_PACKET_WORKER_H
#define	UTIL_CPP_PROTOCOL_WEBSOCKET_PACKET_WORKER_H

#include "packet_worker.h"
#include "websocket_frame.h"

namespace Util {
class WebsocketPacketWorker : public PacketWorker {
public:
	WebsocketPacketWorker(unsigned int frame_length_limit);

	unsigned int frameLengthLimit(void) const { return m_frameLengthLimit; }

	virtual int onParsePacket(unsigned char* buf, size_t buflen, struct sockaddr_storage* from);

private:
	virtual bool sendHandshakePacket(const std::string& data, struct sockaddr_storage* from) = 0;

private:
	const unsigned int m_frameLengthLimit;
	bool m_hasHandshake;
};
}

#endif
