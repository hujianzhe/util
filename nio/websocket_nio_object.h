//
// Created by hujianzhe on 16-9-11.
//

#ifndef	UTIL_NIO_WEBSOCKET_NIO_OBJECT_H
#define	UTIL_NIO_WEBSOCKET_NIO_OBJECT_H

#include "tcp_nio_object.h"

namespace Util {
class WebsocketNioObject : public TcpNioObject {
public:
	WebsocketNioObject(FD_HANDLE fd, unsigned long long frame_length_limit);

	unsigned long long frameLengthLimit(void) const;

private:
	bool send(const void* data, unsigned int nbytes, struct sockaddr_storage* saddr = NULL);
	int onRead(IO_BUFFER inbuf, struct sockaddr_storage* from, size_t transfer_bytes);
	virtual bool onRead(unsigned char* data, size_t len, struct sockaddr_storage* from);

private:
	bool m_hasHandshake;
	const unsigned long long m_frameLengthLimit;
};
}

#endif
