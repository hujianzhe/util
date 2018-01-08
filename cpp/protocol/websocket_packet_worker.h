//
// Created by hujianzhe on 16-9-11.
//

#ifndef	UTIL_CPP_PROTOCOL_WEBSOCKET_PACKET_WORKER_H
#define	UTIL_CPP_PROTOCOL_WEBSOCKET_PACKET_WORKER_H

#include "../nio/nio_packet_worker.h"
#include "websocket_frame.h"

namespace Util {
class WebsocketPacketWorker : public NioPacketWorker {
public:
	WebsocketPacketWorker(unsigned int frame_length_limit);

private:
	int onParsePacket(unsigned char* buf, size_t buflen, struct sockaddr_storage* from);
	virtual bool onRecvPacket(unsigned char* data, size_t len, struct sockaddr_storage* from) { return true; }

private:
	const unsigned int m_frameLengthLimit;
	bool m_hasHandshake;
};
}

#endif
