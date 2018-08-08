//
// Created by hujianzhe on 16-9-11.
//

#include "../nio/nio_object.h"
#include "websocket_packet_worker.h"
#include <stdlib.h>
#include <string.h>

namespace Util {
WebsocketPacketWorker::WebsocketPacketWorker(unsigned int frame_length_limit) :
	m_frameLengthLimit(frame_length_limit),
	m_hasHandshake(false),
	m_data(NULL),
	m_datalen(0)
{
}
WebsocketPacketWorker::~WebsocketPacketWorker(void) { free(m_data); }

int WebsocketPacketWorker::onParsePacket(unsigned char* buf, size_t buflen, struct sockaddr_storage* from) {
	WebSocketFrame protocol(m_frameLengthLimit, Util::WebSocketFrame::FRAME_TYPE_BINARY);
	if (m_hasHandshake) {
		int retcode = protocol.parseDataFrame(buf, buflen);
		if (WebSocketFrame::PARSE_OVERRANGE == retcode) {
			return -1;
		}
		else if (WebSocketFrame::PARSE_INCOMPLETION == retcode) {
			return 0;
		}
		if (protocol.frameType() == WebSocketFrame::FRAME_TYPE_CLOSE) {
			return -1;
		}
		else if (protocol.frameType() == WebSocketFrame::FRAME_TYPE_CONTINUE) {
			unsigned char* p = (unsigned char*)realloc(m_data, m_datalen + protocol.dataLength());
			if (!p) {
				free(m_data);
				m_data = NULL;
				m_datalen = 0;
				return -1;
			}
			m_data = p;
			memcpy(m_data + m_datalen, protocol.data(), protocol.dataLength());
			m_datalen += protocol.dataLength();
			return protocol.frameLength();
		}
		if (!onRecvPacket(m_data ? m_data : protocol.data(), m_data ? m_datalen : protocol.dataLength(), from)) {
			if (m_data) {
				free(m_data);
				m_data = NULL;
				m_datalen = 0;
			}
			return -1;
		}
		if (m_data) {
			free(m_data);
			m_data = NULL;
			m_datalen = 0;
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
