//
// Created by hujianzhe on 25-1-20
//

#ifndef	UTIL_CPP_BUFFER_VIEW_H
#define	UTIL_CPP_BUFFER_VIEW_H

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
		*v = *(T*)(ptr + off);
		return off + sizeof(T);
	}

	size_t read(size_t off, void* p, size_t len) const {
		if (!valid(off, len)) {
			return -1;
		}
		memcpy(p, m_ptr + off, len);
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

private:
	char* m_ptr;
	size_t m_length;
};
}

#endif
