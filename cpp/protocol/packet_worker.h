//
// Created by hujianzhe on 18-1-8.
//

#ifndef UTIL_CPP_PROTOCOL_PACKET_WORKER_H
#define	UTIL_CPP_PROTOCOL_PACKET_WORKER_H

struct sockaddr_storage;

namespace Util {
class PacketWorker {
public:
	virtual ~PacketWorker(void) {}

	virtual int onParsePacket(unsigned char* buf, size_t buflen, struct sockaddr_storage* from) = 0;
	virtual bool onRecvPacket(unsigned char* data, size_t len, struct sockaddr_storage* from) { return true; }
};
}

#endif // !UTIL_CPP_PROTOCOL_PACKET_WORKER_H
