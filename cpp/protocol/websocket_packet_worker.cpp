//
// Created by hujianzhe on 16-9-11.
//

#include "../nio/nio_object.h"
#include "websocket_packet_worker.h"

namespace Util {
WebsocketPacketWorker::WebsocketPacketWorker(unsigned int frame_length_limit) :
	m_frameLengthLimit(frame_length_limit),
	m_hasHandshake(false)
{
}

int WebsocketPacketWorker::onParsePacket(unsigned char* buf, size_t buflen, struct sockaddr_storage* from) {
	WebSocketFrame protocol(m_frameLengthLimit, Util::WebSocketFrame::FRAME_TYPE_BINARY);
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
			if (sendHandshakePacket(response, from)) {
				m_hasHandshake = true;
				return protocol.frameLength();
			}
		}
		return -1;
	}
}
}
