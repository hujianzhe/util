//
// Created by hujianzhe on 17-5-29.
//

#include "http_packet_worker.h"
#include <string.h>

namespace Util {
HttpPacketWorker::HttpPacketWorker(size_t frame_length_limit) :
	m_readbody(false),
	m_protocol(frame_length_limit)
{
}

int HttpPacketWorker::onParsePacket(unsigned char* buf, size_t buflen, struct sockaddr_storage* from) {
	if (m_readbody) {
		if (strncmp(m_protocol.method(), "POST", 4)) {
			return -1;
		}
		// Content-Length
		int ret = m_protocol.parseContentLengthBody((char*)buf, buflen);
		if (HttpFrame::PARSE_OVERRANGE == ret || HttpFrame::PARSE_INVALID == ret) {
			return -1;
		}
		if (HttpFrame::PARSE_INCOMPLETION == ret) {
			return 0;
		}
		if (HttpFrame::PARSE_BODY_NOT_EXIST != ret) {
			if (!handle(from)) {
				return -1;
			}
			m_readbody = false;
			onMessageEnd();
			return (int)m_protocol.frameLength();
		}
		// Transfer-Encoding: chunked
		ret = m_protocol.parseNextChunkedBody((char*)buf, buflen);
		if (HttpFrame::PARSE_OVERRANGE == ret || HttpFrame::PARSE_INVALID == ret) {
			return -1;
		}
		if (HttpFrame::PARSE_INCOMPLETION == ret) {
			return 0;
		}
		if (HttpFrame::PARSE_BODY_NOT_EXIST != ret) {
			if (0 == m_protocol.dataLength()) {
				m_readbody = false;
				onMessageEnd();
				return (int)m_protocol.frameLength();
			}
			return handle(from) ? (int)m_protocol.frameLength() : -1;
		}
		//
		return -1;
	}
	else {
		int ret = m_protocol.parseHeader((char*)buf, buflen);
		if (HttpFrame::PARSE_OVERRANGE == ret || HttpFrame::PARSE_INVALID == ret) {
			return -1;
		}
		if (HttpFrame::PARSE_INCOMPLETION == ret) {
			return 0;
		}
		if (m_protocol.statusCode()) {
			ret = handleResponseHeader(m_protocol, from);
			if (!ret) {
				return -1;
			}
			m_readbody = true;
			return (int)m_protocol.frameLength();
		}
		else {
			ret = handleRequestHeader(m_protocol, from);
			if (!ret) {
				return -1;
			}
			if (strncmp(m_protocol.method(), "GET", 3)) {
				m_readbody = true;
				return (int)m_protocol.frameLength();
			}
			if (handle(from)) {
				onMessageEnd();
				return (int)m_protocol.frameLength();
			}
			else {
				return -1;
			}
		}
	}
}

bool HttpPacketWorker::handle(struct sockaddr_storage* from) {
	if (m_protocol.statusCode()) {
		return handleResponseBody(m_protocol, from);
	}
	else {
		if (!strncmp(m_protocol.method(), "GET", 3)) {
			return handleGetRequest(m_protocol, from);
		}
		else if (!strncmp(m_protocol.method(), "POST", 4)) {
			return handlePostRequestBody(m_protocol, from);
		}
		else {
			return false;
		}
	}
}
}
