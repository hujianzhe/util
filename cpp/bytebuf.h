//
// Created by hujianzhe on 17-4-9.
//

#ifndef UTIL_CPP_BYTEBUF_H
#define	UTIL_CPP_BYTEBUF_H

#include <string>
#include <vector>

namespace Util {
class ByteBuf {
public:
	ByteBuf(void);
	~ByteBuf(void);

	void copy(const void* data, size_t len, bool is_string);
	void copy(const std::string& str);
	void copy(const std::vector<unsigned char>& data);
	void copy(const std::vector<char>& data);

	void ref(const void* data, size_t len);
	void ref(const std::string& str);
	void ref(const std::vector<unsigned char>& data);
	void ref(const std::vector<char>& data);

	void freeData(void);

public:
	unsigned char* data;
	size_t datalen;

private:
	size_t m_memlen;
	bool m_isRef;
};
}

#endif // !UTIL_BYTEBUF_H
