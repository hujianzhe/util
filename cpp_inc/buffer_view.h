//
// Created by hujianzhe on 25-1-20
//

#ifndef	UTIL_CPP_BUFFER_VIEW_H
#define	UTIL_CPP_BUFFER_VIEW_H

#include <string.h>
#include <string>
#include <vector>

namespace util {
struct buffer_view {
	buffer_view() : m_ptr(NULL), m_length(0) {}
	buffer_view(const void* ptr, size_t length) {
		if (ptr && length) {
			m_ptr = (char*)ptr;
			m_length = length;
		}
		else {
			m_ptr = NULL;
			m_length = 0;
		}
	}

	bool valid(size_t off, size_t len) const {
		return off < m_length && len < m_length - off;
	}

	template <typename T>
	size_t read(size_t off, T* v) const {
		if (!valid(off, sizeof(T))) {
			return -1;
		}
		*v = *(T*)(m_ptr + off);
		return off + sizeof(T);
	}

	size_t read(size_t off, void* p, size_t len) const {
		if (!valid(off, len)) {
			return -1;
		}
		memcpy(p, m_ptr + off, len);
		return off + len;
	}

	size_t read(size_t off, std::string* v, size_t len) const {
		if (!valid(off, len)) {
			return -1;
		}
		v->assign(m_ptr + off, len);
		return off + len;
	}

	size_t read(size_t off, std::vector<char>* v, size_t len) const {
		if (!valid(off, len)) {
			return -1;
		}
		v->resize(len);
		memcpy(v->data(), m_ptr + off, len);
		return off + len;
	}

	size_t read(size_t off, std::vector<unsigned char>* v, size_t len) const {
		if (!valid(off, len)) {
			return -1;
		}
		v->resize(len);
		memcpy(v->data(), m_ptr + off, len);
		return off + len;
	}

	template <typename T>
	size_t write(size_t off, const T& v) const {
		if (!valid(off, sizeof(T))) {
			return -1;
		}
		*(T*)(m_ptr + off) = v;
		return off + sizeof(T);
	}

	size_t write(size_t off, const void* p, size_t len) const {
		if (!valid(off, len)) {
			return -1;
		}
		memcpy(m_ptr + off, p, len);
		return off + len;
	}

	size_t write(size_t off, const std::string& v) const {
		return write(off, v.data(), v.size());
	}

	size_t write(size_t off, const std::vector<char>& v) const {
		return write(off, v.data(), v.size());
	}

	size_t write(size_t off, const std::vector<unsigned char>& v) const {
		return write(off, v.data(), v.size());
	}

private:
	char* m_ptr;
	size_t m_length;
};
}

#endif
