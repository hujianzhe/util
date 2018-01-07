//
// Created by hujianzhe on 16-9-11.
//

#ifndef	UTIL_CPP_NIO_WEBSOCKET_NIO_OBJECT_H
#define	UTIL_CPP_NIO_WEBSOCKET_NIO_OBJECT_H

#include "tcp_nio_object.h"

namespace Util {
class WebsocketNioObject : public TcpNioObject {
public:
	WebsocketNioObject(FD_t fd, unsigned int frame_length_limit);

private:
	bool send(const void* data, unsigned int nbytes, struct sockaddr_storage* saddr = NULL);

	int onParsePacket(unsigned char* buf, size_t buflen, struct sockaddr_storage* from);
	virtual bool onRecvPacket(unsigned char* data, size_t len, struct sockaddr_storage* from) { return true; }

private:
	bool m_hasHandshake;
};
}

#endif
