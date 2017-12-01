//
// Created by hujianzhe on 16-5-20.
//

#include "../syslib/encrypt.h"
#include "../syslib/socket.h"
#include "../syslib/string.h"
#include "../exception.h"
#include "websocket_protocol.h"

namespace Util {
int WebSocketProtocol::parseHandshake(char* data, size_t len, std::string& response) {
	const char* key;
	static const char WEB_SOCKET_MAGIC_KEY[] = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
	if (m_frameLengthLimit < len) {
		key = strnstr(data, "\r\n\r\n", m_frameLengthLimit);
		if (!key) {
			return PARSE_OVERRANGE;
		}
	}
	else {
		key = strnstr(data, "\r\n\r\n", len);
		if (!key) {
			return PARSE_INCOMPLETION;
		}
	}
	m_frameLength = key - data + 4;
	if (key = strnstr(data, "Sec-WebSocket-Key:", m_frameLength)) {
		for (key += 18; ' ' == *key; ++key);
	}
	else {
		return PARSE_INVALID;
	}
	std::string handle_shanke_key(key, strstr(key, "\r\n") - key);
	handle_shanke_key += WEB_SOCKET_MAGIC_KEY;
	unsigned char sha1_handle_key[CC_SHA1_DIGEST_LENGTH];
	char base64_handle_key[CC_SHA1_DIGEST_LENGTH * 3];
	CC_SHA1_CTX sha1_ctx;
	sha1_Init(&sha1_ctx);
	sha1_Update(&sha1_ctx, handle_shanke_key.data(), handle_shanke_key.size());
	sha1_Final(sha1_handle_key, &sha1_ctx);
	sha1_Clean(&sha1_ctx);
	base64_Encode(sha1_handle_key, sizeof(sha1_handle_key), base64_handle_key);
	response += "HTTP/1.1 101 Switching Protocols\r\n"
				"Upgrade: websocket\r\n"
				"Connection: Upgrade\r\n"
				"Sec-WebSocket-Accept: ";
	response += base64_handle_key;
	response += "\r\n\r\n";
	return PARSE_OK;
}

size_t WebSocketProtocol::responseHeaderLength(size_t datalen) {
	const size_t basic_header_len = 2;
	size_t header_len = 0;
	if (datalen < 126) {
		header_len = basic_header_len;
	}
	else if (datalen <= 0xffff) {
		header_len = basic_header_len + sizeof(unsigned short);
	}
	else {
		header_len = basic_header_len + sizeof(unsigned long long);
	}
	return header_len;
}

bool WebSocketProtocol::buildHeader(unsigned char* headbuf, size_t datalen) {
	const size_t basic_header_len = 2;
	size_t header_len = 0;

	headbuf[0] = m_frameType | (m_isFin ? 0x80 : 0);
	if (datalen < 126) {
		header_len = basic_header_len;
		headbuf[1] = datalen;
	}
	else if (datalen <= 0xffff) {
		header_len = basic_header_len + sizeof(unsigned short);
		headbuf[1] = 126;
		*(unsigned short*)&headbuf[basic_header_len] = htons(datalen);
	}
	else {
		header_len = basic_header_len + sizeof(unsigned long long);
		headbuf[1] = 127;
		*(unsigned long long*)&headbuf[basic_header_len] = htonll(datalen);
	}

	return header_len + datalen <= m_frameLengthLimit;
}

WebSocketProtocol::WebSocketProtocol(unsigned long long frame_length_limit, short frame_type, bool is_fin) :
	m_frameLengthLimit(frame_length_limit),
	m_isFin(is_fin),
	m_frameType(frame_type),
	m_payloadData(NULL),
	m_payloadLength(0),
	m_frameLength(0)
{
}

int WebSocketProtocol::parseDataFrame(unsigned char* data, size_t len) {
	size_t mask_len = 0;
	size_t ext_payload_filed_len = 0;
	const size_t header_size = 2;
	if (len < header_size) {
		return PARSE_INCOMPLETION;
	}
	bool is_fin = data[0] >> 7;
	/*
	if (!is_fin) {
		return false;
	}
	*/
	short frame_type = data[0] & 0x0f;
	/*
	if (FRAME_TYPE_CLOSE == frame_type) {
		return PARSE_OK;
	}
	*/
	unsigned long long payload_len = data[1] & 0x7f;
	if (data[1] >> 7) {
		mask_len = 4;
	}
	unsigned char* mask = NULL;
	unsigned char* payload_data = NULL;
	if (payload_len < 126) {
		if (len < header_size + ext_payload_filed_len + mask_len) {
			return PARSE_INCOMPLETION;
		}
	}
	else if (payload_len == 126) {
		ext_payload_filed_len = sizeof(unsigned short);
		if (len < header_size + ext_payload_filed_len + mask_len) {
			return PARSE_INCOMPLETION;
		}
		payload_len = ntohs(*(unsigned short*)&data[header_size]);
	}
	else if (payload_len == 127) {
		ext_payload_filed_len = sizeof(unsigned long long);
		if (len < header_size + ext_payload_filed_len + mask_len) {
			return PARSE_INCOMPLETION;
		}
		payload_len = ntohll(*(unsigned long long*)&data[header_size]);
	}
	if (mask_len) {
		mask = &data[header_size + ext_payload_filed_len];
	}
	if (m_frameLengthLimit < header_size + ext_payload_filed_len + mask_len + payload_len) {
		return PARSE_OVERRANGE;
	}
	if (len < header_size + ext_payload_filed_len + mask_len + payload_len) {
		return PARSE_INCOMPLETION;
	}
	payload_data = &data[header_size + ext_payload_filed_len + mask_len];
	if (mask) {
		for (unsigned long long i = 0; i < payload_len; ++i) {
			payload_data[i] ^= mask[i % 4];
		}
	}
	// save parse result
	m_isFin = is_fin;
	m_frameType = frame_type;
	m_payloadData = payload_len ? payload_data : NULL;
	m_payloadLength = payload_len;
	m_frameLength = header_size + ext_payload_filed_len + mask_len + payload_len;
	return PARSE_OK;
}
}
