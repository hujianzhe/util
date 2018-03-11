//
// Created by hujianzhe on 18-1-8.
//

#ifndef UTIL_CPP_PROTOCOL_PACKET_WORKER_H
#define	UTIL_CPP_PROTOCOL_PACKET_WORKER_H

#include "../../c/syslib/socket.h"

namespace Util {
class PacketWorker {
public:
	PacketWorker(void) : m_userdata(NULL) {}
	virtual ~PacketWorker(void) {}

	void userdata(void* data) { m_userdata = data; }
	void* userdata(void) const { return m_userdata; }

	virtual int onParsePacket(unsigned char* buf, size_t buflen, struct sockaddr_storage* from) = 0;
	virtual bool onRecvPacket(unsigned char* data, size_t len, struct sockaddr_storage* from) { return true; }

private:
	void* m_userdata;
};
}

#endif // !UTIL_CPP_PROTOCOL_PACKET_WORKER_H
