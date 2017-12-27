//
// Created by hujianzhe on 16-9-11.
//

#include "websocket_nio_object.h"
#include "../protocol/websocket_protocol.h"

namespace Util {
WebsocketNioObject::WebsocketNioObject(FD_t fd, unsigned long long frame_length_limit) :
	TcpNioObject(fd),
	m_hasHandshake(false),
	m_frameLengthLimit(frame_length_limit)
{
}

unsigned long long WebsocketNioObject::frameLengthLimit(void) const { return m_frameLengthLimit; }

bool WebsocketNioObject::send(const void* data, unsigned int nbytes, struct sockaddr_storage* saddr) {
	WebSocketProtocol protocol(m_frameLengthLimit);
	size_t headlen = WebSocketProtocol::responseHeaderLength(nbytes);
	void* head = alloca(headlen);
	if (protocol.buildHeader((unsigned char*)head, nbytes)) {
		IoBuf_t iov[2];
		iobuffer_buf(iov + 0) = (char*)head;
		iobuffer_len(iov + 0) = headlen;
		iobuffer_buf(iov + 1) = (char*)data;
		iobuffer_len(iov + 1) = nbytes;
		return sendv(iov, sizeof(iov) / sizeof(iov[0]), saddr);
	}
	return false;
}
int WebsocketNioObject::onRead(IoBuf_t inbuf, struct sockaddr_storage* from, size_t transfer_bytes) {
	size_t offset = 0;
	WebSocketProtocol protocol(m_frameLengthLimit);
	unsigned char* data = (unsigned char*)iobuffer_buf(&inbuf);
	size_t data_len = iobuffer_len(&inbuf);
	if (m_hasHandshake) {
		do {
			int retcode = protocol.parseDataFrame(data + offset, data_len - offset);
			if (WebSocketProtocol::PARSE_OVERRANGE == retcode) {
				invalid();
				offset = data_len;
				break;
			}
			if (WebSocketProtocol::PARSE_INCOMPLETION == retcode) {
				break;
			}
			if (protocol.frameType() == WebSocketProtocol::FRAME_TYPE_CLOSE) {
				invalid();
				offset = data_len;
				break;
			}
			if (onRead(protocol.data(), protocol.dataLength(), from)) {
				offset += protocol.frameLength();
			}
			else {
				invalid();
				offset = data_len;
				break;
			}
		} while (1);
	}
	else {
		std::string response;
		int retcode = protocol.parseHandshake((char*)data, data_len, response);
		if (WebSocketProtocol::PARSE_OK == retcode) {
			if (Util::NioObject::send(response.data(), response.size(), from)) {
				offset = protocol.frameLength();
				m_hasHandshake = true;
			}
			else {
				invalid();
				offset = data_len;
			}
		}
		else if (WebSocketProtocol::PARSE_INVALID == retcode || WebSocketProtocol::PARSE_OVERRANGE == retcode) {
			invalid();
			offset = data_len;
		}
	}
	return offset;
}

bool WebsocketNioObject::onRead(unsigned char* data, size_t len, struct sockaddr_storage* from) {
	return true;
}
}
