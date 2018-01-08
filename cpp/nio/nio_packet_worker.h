//
// Created by hujianzhe on 18-1-8.
//

#ifndef UTIL_CPP_NIO_NIO_PACKET_WORKER_H
#define	UTIL_CPP_NIO_NIO_PACKET_WORKER_H

#include "../../c/syslib/socket.h"

namespace Util {
class NioObject;
class NioPacketWorker {
friend class NioObject;
public:
	static NioPacketWorker* defaultWorker(void);
	NioPacketWorker(void) { m_object = NULL; }
	virtual ~NioPacketWorker(void) {}
	virtual void release(void) {}

	NioObject* object(void) { return m_object; }

	virtual int onParsePacket(unsigned char* buf, size_t buflen, struct sockaddr_storage* from) {
		return buflen > 0x7fffffff ? 0x7fffffff : (int)buflen;
	}
	virtual void onParseEmptyPacket(struct sockaddr_storage* from) {}
	virtual bool onRecvPacket(unsigned char* data, size_t len, struct sockaddr_storage* from) { return true; }

private:
	NioObject* m_object;
};
}

#endif // !UTIL_CPP_NIO_NIO_PACKET_WORKER_H
