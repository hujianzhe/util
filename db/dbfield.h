//
// Created by hujianzhe on 17-9-24.
//

#ifndef UTIL_DB_DBFIELD_H
#define	UTIL_DB_DBFIELD_H

#include <stddef.h>

typedef char		tiny;
typedef	short		smallint;
typedef long long	bigint;

template <size_t LEN>
struct blob {
	blob(void) : len(0) {}

	bool operator != (const blob& v) {
		if (len != v.len) {
			return true;
		}
		else {
			size_t i;
			for (i = 0; i < len && i < v.len && data[i] == v.data[i]; ++i);
			return i != len || i != v.len;
		}
	}
	bool operator == (const blob& v) { return !operator != (v); }

	size_t len;
	unsigned char data[LEN];
};
template <size_t LEN>
struct blob<LEN>* memcpy(struct blob<LEN>* dst, const void* src, size_t n) {
	if (n > LEN) {
		return NULL;
	}
	for (size_t i = 0; i < n; ++i) {
		dst->data[i] = ((unsigned char*)src)[i];
	}
	if (n) {
		dst->len = n;
	}
	return dst;
}

template <size_t LEN>
class varchar {
public:
	varchar(void) { if (LEN > 0) m_data[0] = 0; }

	bool operator != (const varchar& v) {
		const char *src = m_data, *dst = v.m_data;
		while (*src && *dst && *src == *dst) { ++src; ++dst; }
		return *src - *dst;
	}
	bool operator == (const varchar& v) { return !operator != (v); }

	operator char* (void) { return m_data; }
	char& operator [] (size_t i) { return m_data[i]; }

private:
	char m_data[LEN];
};

#endif // !UTIL_DBFIELD_H