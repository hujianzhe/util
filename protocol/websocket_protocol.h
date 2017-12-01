//
// Created by hujianzhe on 16-5-20.
//

#ifndef UTIL_PROTOCOL_WEBSOCKET_PROTOCOL
#define UTIL_PROTOCOL_WEBSOCKET_PROTOCOL

#include <string>
#include <vector>

namespace Util {
class WebSocketProtocol {
public:
	enum {
		FRAME_TYPE_CONTINUE = 0,
		FRAME_TYPE_TEXT		= 1,
		FRAME_TYPE_BINARY	= 2,
		FRAME_TYPE_CLOSE	= 8,
		FRAME_TYPE_PING		= 9,
		FRAME_TYPE_PONG		= 10
	};
	enum {
		PARSE_OVERRANGE,
		PARSE_INCOMPLETION,
		PARSE_INVALID,
		PARSE_OK
	};
	int parseHandshake(char* data, size_t len, std::string& response);

	WebSocketProtocol(unsigned long long frame_length_limit, short frame_type = FRAME_TYPE_TEXT, bool is_fin = true);

	static size_t responseHeaderLength(size_t datalen);
	bool buildHeader(unsigned char* headbuf, size_t datalen);
	int parseDataFrame(unsigned char* data, size_t len);	

	bool isFin(void) const { return m_isFin; }
	short frameType(void) const { return m_frameType; }
	unsigned char* data(void) const { return m_payloadData; }
	unsigned long long dataLength(void) const { return m_payloadLength; }
	unsigned long long frameLength(void) const { return m_frameLength; }

private:
	const unsigned long long m_frameLengthLimit;

	bool m_isFin;
	short m_frameType;
	unsigned char* m_payloadData;
	unsigned long long m_payloadLength;
	unsigned long long m_frameLength;
};
}

#endif // UTIL_WEBSOCKET_PROTOCOL
