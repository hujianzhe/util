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

	size_t frameLengthLimit(void) const;

private:
	int onRead(IoBuf_t inbuf, struct sockaddr_storage* from, size_t transfer_bytes);

	virtual bool handleRequestHeader(const HttpFrame& protocol, struct sockaddr_storage* from);
	virtual bool handleGetRequest(const HttpFrame& protocol, struct sockaddr_storage* from);
	virtual bool handlePostRequestBody(const HttpFrame& protocol, struct sockaddr_storage* from);

	virtual bool handleResponseHeader(const HttpFrame& protocol, struct sockaddr_storage* from);
	virtual bool handleResponseBody(const HttpFrame& protocol, struct sockaddr_storage* from);

	virtual bool onMessageEnd(void);

private:
	bool handle(struct sockaddr_storage* from);

private:
	bool m_readbody;
	HttpFrame m_protocol;
};
}

#endif // !UTIL_HTTP_NIO_OBJECT_H
