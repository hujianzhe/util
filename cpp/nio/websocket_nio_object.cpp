//
// Created by hujianzhe on 16-9-11.
//

#include "websocket_nio_object.h"
#include "../protocol/websocket_frame.h"

namespace Util {
WebsocketNioObject::WebsocketNioObject(FD_t fd, unsigned int frame_length_limit) :
	TcpNioObject(fd, frame_length_limit),
	m_hasHandshake(false)
{
}

bool WebsocketNioObject::send(const void* data, unsigned int nbytes, struct sockaddr_storage* saddr) {
	WebSocketFrame protocol(m_frameLengthLimit);
	size_t headlen = WebSocketFrame::responseHeaderLength(nbytes);
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
int WebsocketNioObject::onParsePacket(unsigned char* buf, size_t buflen, struct sockaddr_storage* from) {
	WebSocketFrame protocol(m_frameLengthLimit);
	if (m_hasHandshake) {
		int retcode = protocol.parseDataFrame(buf, buflen);
		if (WebSocketFrame::PARSE_OVERRANGE == retcode) {
			return -1;
		}
		if (WebSocketFrame::PARSE_INCOMPLETION == retcode) {
			return 0;
		}
		if (protocol.frameType() == WebSocketFrame::FRAME_TYPE_CLOSE) {
			return -1;
		}
		if (!onRecvPacket(protocol.data(), protocol.dataLength(), from)) {
			return -1;
		}
		return protocol.frameLength();
	}
	else {
		std::string response;
		int retcode = protocol.parseHandshake((char*)buf, buflen, response);
		if (WebSocketFrame::PARSE_INVALID == retcode) {
			return -1;
		}
		if (WebSocketFrame::PARSE_OVERRANGE == retcode) {
			return -1;
		}
		if (WebSocketFrame::PARSE_INCOMPLETION == retcode) {
			return 0;
		}
		if (WebSocketFrame::PARSE_OK == retcode) {
			if (Util::NioObject::send(response.data(), response.size(), from)) {
				m_hasHandshake = true;
				return protocol.frameLength();
			}
		}
		return -1;
	}
}
}
