//
// Created by hujianzhe on 16-5-20.
//

#ifndef UTIL_CPP_PROTOCOL_LENGTHFIELD_FRAME_H
#define UTIL_CPP_PROTOCOL_LENGTHFIELD_FRAME_H

#include <stddef.h>
#include <vector>

namespace Util {
class LengthFieldFrame {
public:
	enum {
		LENGTH_FIELD_BYTE_SIZE	= 1,
		LENGTH_FIELD_WORD_SIZE	= 2,
		LENGTH_FIELD_DWORD_SIZE	= 4
	};
	enum {
		PARSE_OVERRANGE,
		PARSE_INCOMPLETION,
		PARSE_OK
	};

	LengthFieldFrame(short length_field_size, size_t frame_length_limit);

	bool buildHeader(void* headbuf, unsigned int datalen);
	int parseDataFrame(unsigned char* data, size_t len);

	short lengthFieldSize(void) const { return m_lengthFieldSize; }
	size_t framelengthLimit(void) const { return m_frameLengthLimit; }
	unsigned char* data(void) const { return m_data; }
	size_t dataLength(void) const { return m_dataLength; }
	size_t frameLength(void) const { return m_frameLength; }

private:
	const short m_lengthFieldSize;
	size_t m_frameLengthLimit;

	unsigned char* m_data;
	size_t m_dataLength;
	size_t m_frameLength;
};
}

#endif // UTIL_LENGTHFIELD_PROTOCOL_H
