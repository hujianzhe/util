//
// Created by hujianzhe on 16-5-20.
//

#include "../syslib/socket.h"
#include "lengthfield_protocol.h"
#include <string.h>

namespace Util {
bool LengthFieldProtocol::buildHeader(void* headbuf, unsigned int datalen) {
	if (m_frameLengthLimit < datalen + m_lengthFieldSize) {
		return false;
	}
	switch (m_lengthFieldSize) {
		case LENGTH_FIELD_BYTE_SIZE:
			*(unsigned char*)headbuf = datalen;
			break;

		case LENGTH_FIELD_WORD_SIZE:
			*(unsigned short*)headbuf = htons(datalen);
			break;

		case LENGTH_FIELD_DWORD_SIZE:
			*(unsigned int*)headbuf = htonl(datalen);
			break;

		default:
			return false;
	}
	return true;
}

LengthFieldProtocol::LengthFieldProtocol(short length_field_size, size_t frame_length_limit) :
	m_lengthFieldSize(length_field_size),
	m_frameLengthLimit(frame_length_limit),
	m_data(NULL),
	m_dataLength(0),
	m_frameLength(0)
{
	switch (m_lengthFieldSize) {
		case LENGTH_FIELD_BYTE_SIZE:
			if (m_frameLengthLimit > 0xff) {
				m_frameLengthLimit = 0xff;
			}
			break;

		case LENGTH_FIELD_WORD_SIZE:
			if (m_frameLengthLimit > 0xffff) {
				m_frameLengthLimit = 0xffff;
			}
			break;

		case LENGTH_FIELD_DWORD_SIZE:
			if (m_frameLengthLimit > 0x7fffffff) {
				m_frameLengthLimit = 0x7fffffff;
			}
			break;

		default:
			m_frameLengthLimit = 0;
			return;
	}
}

int LengthFieldProtocol::parseDataFrame(unsigned char* data, size_t len) {
	if (m_lengthFieldSize > len) {
		return PARSE_INCOMPLETION;
	}
	size_t packet_length = 0;
	switch (m_lengthFieldSize) {
		case LENGTH_FIELD_BYTE_SIZE:
			packet_length = *data;
			break;

		case LENGTH_FIELD_WORD_SIZE:
			packet_length = ntohs(*(unsigned short*)data);
			break;

		case LENGTH_FIELD_DWORD_SIZE:
			packet_length = ntohl(*(unsigned int*)data);
			break;

		default:
			return PARSE_OVERRANGE;
	}
	if (m_lengthFieldSize + packet_length > m_frameLengthLimit) {
		return PARSE_OVERRANGE;
	}
	if (m_lengthFieldSize + packet_length > len) {
		return PARSE_INCOMPLETION;
	}
	// save parse result
	m_data = packet_length ? data + m_lengthFieldSize : NULL;
	m_dataLength = packet_length;
	m_frameLength = m_lengthFieldSize + packet_length;
	return PARSE_OK;
}
}
