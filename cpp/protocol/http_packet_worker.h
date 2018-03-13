//
// Created by hujianzhe on 17-5-29.
//

#ifndef UTIL_CPP_PROTOCOL_HTTP_PACKET_WORKER_H
#define	UTIL_CPP_PROTOCOL_HTTP_PACKET_WORKER_H

#include "packet_worker.h"
#include "http_frame.h"

namespace Util {
class HttpPacketWorker : public PacketWorker {
public:
	HttpPacketWorker(size_t frame_length_limit);

	size_t frameLengthLimit(void) const { return m_protocol.frameLengthLimit(); }

	virtual int onParsePacket(unsigned char* buf, size_t buflen, struct sockaddr_storage* from);

private:
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

#endif // !UTIL_CPP_PROTOCOL_HTTP_PACKET_WORKER_H
