//
// Created by hujianzhe on 17-5-29.
//

#ifndef UTIL_CPP_NIO_HTTP_NIO_OBJECT_H
#define	UTIL_CPP_NIO_HTTP_NIO_OBJECT_H

#include "tcp_nio_object.h"
#include "../protocol/http_protocol.h"

namespace Util {
class HttpNioObject : public TcpNioObject {
public:
	HttpNioObject(FD_HANDLE fd, size_t frame_length_limit);

	size_t frameLengthLimit(void) const;

private:
	int onRead(IO_BUFFER inbuf, struct sockaddr_storage* from, size_t transfer_bytes);

	virtual bool handleRequestHeader(const HttpProtocol& protocol, struct sockaddr_storage* from);
	virtual bool handleGetRequest(const HttpProtocol& protocol, struct sockaddr_storage* from);
	virtual bool handlePostRequestBody(const HttpProtocol& protocol, struct sockaddr_storage* from);

	virtual bool handleResponseHeader(const HttpProtocol& protocol, struct sockaddr_storage* from);
	virtual bool handleResponseBody(const HttpProtocol& protocol, struct sockaddr_storage* from);

	virtual bool onMessageEnd(void);

private:
	bool handle(struct sockaddr_storage* from);

private:
	bool m_readbody;
	HttpProtocol m_protocol;
};
}

#endif // !UTIL_HTTP_NIO_OBJECT_H
