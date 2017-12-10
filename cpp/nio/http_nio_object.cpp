//
// Created by hujianzhe on 17-5-29.
//

#include "http_nio_object.h"
#include <string.h>

namespace Util {
HttpNioObject::HttpNioObject(FD_HANDLE fd, size_t frame_length_limit) :
	TcpNioObject(fd),
	m_readbody(false),
	m_protocol(frame_length_limit)
{
}

size_t HttpNioObject::frameLengthLimit(void) const { return m_protocol.frameLengthLimit(); }

int HttpNioObject::onRead(IO_BUFFER inbuf, struct sockaddr_storage* from, size_t transfer_bytes) {
	size_t offset = 0;
	const char* data = (const char*)iobuffer_buf(&inbuf);
	size_t data_len = iobuffer_len(&inbuf);

	bool isok = true;
	do {
		if (m_readbody) {
			if (!strncmp(m_protocol.method(), "GET", 3)) {
				if (handle(from)) {
					m_readbody = false;
					if (onMessageEnd()) {
						break;
					}
					continue;
				}
				else {
					isok = false;
					break;
				}
			}
			else if (!strncmp(m_protocol.method(), "POST", 4)) {
				// Content-Length
				int ret = m_protocol.parseContentLengthBody(data + offset, data_len - offset);
				if (HttpProtocol::PARSE_OVERRANGE == ret || HttpProtocol::PARSE_INVALID == ret) {
					isok = false;
					break;
				}
				if (HttpProtocol::PARSE_INCOMPLETION == ret) {
					break;
				}
				if (HttpProtocol::PARSE_BODY_NOT_EXIST != ret) {
					if (handle(from)) {
						offset += m_protocol.frameLength();
						m_readbody = false;
						if (onMessageEnd()) {
							break;
						}
						continue;
					}
					else {
						isok = false;
						break;
					}
				}
				// Transfer-Encoding: chunked
				ret = m_protocol.parseNextChunkedBody(data + offset, data_len - offset);
				if (HttpProtocol::PARSE_OVERRANGE == ret || HttpProtocol::PARSE_INVALID == ret) {
					isok = false;
					break;
				}
				if (HttpProtocol::PARSE_INCOMPLETION == ret) {
					break;
				}
				if (HttpProtocol::PARSE_BODY_NOT_EXIST != ret) {
					if (0 == m_protocol.dataLength()) {
						offset += m_protocol.frameLength();
						m_readbody = false;
						if (onMessageEnd()) {
							break;
						}
						continue;
					}
					else if (handle(from)) {
						offset += m_protocol.frameLength();
						continue;
					}
					else {
						isok = false;
						break;
					}
				}
			}
			else {
				isok = false;
				break;
			}
		}
		else {
			int ret = m_protocol.parseHeader(data + offset, data_len - offset);
			if (HttpProtocol::PARSE_OVERRANGE == ret || HttpProtocol::PARSE_INVALID == ret) {
				isok = false;
				break;
			}
			if (HttpProtocol::PARSE_INCOMPLETION == ret) {
				break;
			}
			if (m_protocol.statusCode()) {
				ret = handleResponseHeader(m_protocol, from);
			}
			else {
				ret = handleRequestHeader(m_protocol, from);
			}
			if (ret) {
				offset += m_protocol.frameLength();
				m_readbody = true;
			}
			else {
				isok = false;
				break;
			}
		}
	} while (1);
	if (!isok) {
		invalid();
		offset = data_len;
	}
	return offset;
}

bool HttpNioObject::handleRequestHeader(const HttpProtocol& protocol, struct sockaddr_storage* from) { return true; }
bool HttpNioObject::handleGetRequest(const HttpProtocol& protocol, struct sockaddr_storage* from) { return true; }
bool HttpNioObject::handlePostRequestBody(const HttpProtocol& protocol, struct sockaddr_storage* from) { return true; }

bool HttpNioObject::handleResponseHeader(const HttpProtocol& protocol, struct sockaddr_storage* from) { return true; }
bool HttpNioObject::handleResponseBody(const HttpProtocol& protocol, struct sockaddr_storage* from) { return true; }

bool HttpNioObject::onMessageEnd(void) { return true; }

bool HttpNioObject::handle(struct sockaddr_storage* from) {
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
