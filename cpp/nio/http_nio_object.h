//
// Created by hujianzhe on 17-5-29.
//

#ifndef UTIL_CPP_NIO_HTTP_NIO_OBJECT_H
#define	UTIL_CPP_NIO_HTTP_NIO_OBJECT_H

#include "tcp_nio_object.h"
#include "../protocol/http_frame.h"

namespace Util {
class HttpNioObject : public TcpNioObject {
public:
	HttpNioObject(FD_t fd, size_t frame_length_limit);

private:
	int onParsePacket(unsigned char* buf, size_t buflen, struct sockaddr_storage* from);

	virtual bool handleRequestHeader(const HttpFrame& protocol, struct sockaddr_storage* from) { return true; }
	virtual bool handleGetRequest(const HttpFrame& protocol, struct sockaddr_storage* from) { return true; }
	virtual bool handlePostRequestBody(const HttpFrame& protocol, struct sockaddr_storage* from) { return true; }

	virtual bool handleResponseHeader(const HttpFrame& protocol, struct sockaddr_storage* from) { return true; }
	virtual bool handleResponseBody(const HttpFrame& protocol, struct sockaddr_storage* from) { return true; }

	virtual bool onMessageEnd(void) { return true; }

private:
	bool handle(struct sockaddr_storage* from);

private:
	bool m_readbody;
	HttpFrame m_protocol;
};
}

#endif // !UTIL_HTTP_NIO_OBJECT_H
