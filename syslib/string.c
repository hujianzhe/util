//
// Created by hujianzhe
//

#include "string.h"
#include <stdlib.h>

#ifdef	__cplusplus
extern "C" {
#endif

#if !defined(__FreeBSD__) && !defined(__APPLE__)
char *strnchr(const char* s, const char c, size_t n) {
	if (!s) {
		return NULL;
	}
	while (n && *s && *s != c) {
		--n;
		++s;
	}
	return n && *s == c ? (char*)s : NULL;
}

char *strnstr(const char* s1, const char* s2, size_t n) {
	if (s1 && s2 && n) {
		size_t s2len = strlen(s2);
		if (s2len <= n) {
			const char *end = s1 + n;
			while (*s1 && end - s1 >= s2len) {
				const char *bp = s1;
				const char *sp = s2;
				do {
					if (!*sp)
						return (char *) s1;
				} while (*bp++ == *sp++);
				++s1;
			}
		}
	}
	return NULL;
}
char* strlcpy(char* dst, const char* src, size_t size) {
	size_t i;
	for (i = 0; i < size; ++i) {
		dst[i] = src[i];
		if (0 == src[i])
			return dst;
	}
	if (i)
		dst[i - 1] = 0;
	return dst;
}
#endif

#if defined(_WIN32) || defined(_WIN64)
char* strndup(const char* s, size_t n) {
	char* p = (char*)malloc(n + 1);
	if (p) {
		size_t i;
		for (i = 0; i < n; ++i) {
			p[i] = s[i];
		}
		p[n] = 0;
	}
	return p;
}
#endif

void strtrim(const char* str, size_t len, char** newstr, size_t* newlen) {
	const char* end = str + len - 1;
	while (str < end) {
		if (*str == ' ' || *str == '\r' || *str == '\n' || *str == '\f' || *str == '\b' || *str == '\t' || *end == '\v')
			++str;
		else
			break;
	}
	while (end > str) {
		if (*end == ' ' || *end == '\r' || *end == '\n' || *end == '\f' || *end == '\b' || *end == '\t' || *end == '\v')
			--end;
		else
			break;
	}
	*newstr = (char*)str;
	*newlen = end + 1 - str;
}

unsigned int strhash(const char* str) {
	/* BKDR hash */
	unsigned int seed = 131;
	unsigned int hash = 0;
	while (*str) {
		hash = hash * seed + (*str++);
	}
	return hash & 0x7fffffff;
}

size_t strlen_utf8(const char* s) {
	size_t u8_len = 0;
	size_t byte_len = strlen(s);
	size_t i;
	for (i = 0; i < byte_len; ++i) {
		unsigned char c = s[i];
		if (c >= 0x80) {
			if (c < 0xc0 || c >= 0xf8) {/* not UTF-8 */
				u8_len = -1;
				break;
			}
			else if (c >= 0xc0 && c < 0xe0) {
				++i;
			}
			else if (c >= 0xe0 && c < 0xf0) {
				i += 2;
			}
			else if (c >= 0xf0 && c < 0xf8) {
				i += 3;
			}
		}
		++u8_len;
	}
	return u8_len;
}

size_t strcopy(char* dst, size_t dst_len, const char* src, size_t src_len) {
	size_t i;
	size_t len = dst_len > src_len ? src_len : dst_len;
	for (i = 0; i < len; ++i)
		dst[i] = src[i];
	if (len) {
		if (len < dst_len) {
			dst[len] = 0;
			return len;
		}
		else {
			dst[dst_len - 1] = 0;
			return dst_len - 1;
		}
	}
	return 0;
}

size_t strlen_safe(const char* s, size_t maxlen) {
	size_t len = 0;
	if (s) {
		for (; *s && len < maxlen; ++len);
	}
	return len;
}

#ifdef	__cplusplus
}
#endif
