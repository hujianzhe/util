//
// Created by hujianzhe
//

#include "string.h"

#ifdef	__cplusplus
extern "C" {
#endif

size_t strLength(const char* s) {
	if (!s)
		return 0;
	else {
		const char* p = s;
		while (*p)
			++p;
		return p - s;
	}
}

char* strChr(const char* s, size_t len, const char c) {
	while (len && *s && *s != c) {
		--len;
		++s;
	}
	return len && *s ? (char*)s : NULL;
}

char* strStr(const char* s1, size_t s1len, const char* s2, size_t s2len) {
	const char *p1, *p2;
	size_t i;
	if (-1 == s2len)
		for (p2 = s2, s2len = 0; *p2; ++p2, ++s2len);
	if (-1 == s1len)
		for (p1 = s1, s1len = 0; *p1; ++p1, ++s1len);
	for (i = 0; i + s2len <= s1len && s1[i]; ++i) {
		size_t len;
		for (p1 = s1 + i, p2 = s2, len = s2len; len && *p2 && *p1 && *p1 == *p2; ++p1, ++p2, --len);
		if ('\0' == *p2 || 0 == len)
			return (char*)(s1 + i);
	}
	return NULL;
}

char* strSplit(char** s, const char* delim) {
	int has_split = 0;
	char* ss = *s;
	char* sp = ss;
	while (*sp) {
		const char* p = delim;
		while (*p && *sp != *p)
			++p;
		if (*p) {
			*sp = '\0';
			has_split = 1;
		}
		else if (has_split)
			break;
		++sp;
	}
	*s = sp;
	return ss != sp ? ss : NULL;
}

void strTrim(const char* str, size_t len, char** newstr, size_t* newlen) {
	const char* end;
	if (-1 == len) {
		const char* p;
		for (p = str, len = 0; *p; ++p, ++len);
	}
	end = str + len;
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
	*newlen = end - str;
}

size_t strlenUTF8(const char* s) {
	size_t u8_len = 0;
	size_t byte_len = strLength(s);
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

char* strCopy(char* dst, size_t dst_len, const char* src, size_t src_len) {
	size_t i;
	size_t len = dst_len > src_len ? src_len : dst_len;
	if (-1 == len)
		return NULL;
	for (i = 0; i < len; ++i) {
		dst[i] = src[i];
		if (0 == dst[i]) {
			return dst;
		}
	}
	if (len) {
		if (len < dst_len) {
			dst[len] = 0;
			return dst;
		}
		else {
			dst[dst_len - 1] = 0;
			return dst;
		}
	}
	return dst;
}

int strCmp(const char* s1, const char* s2, size_t n) {
	if (0 == n || !s1 || !s2)
		return 0;
	if (-1 != n) {
		while (--n && *s1 && *s1 == *s2) {
			++s1;
			++s2;
		}
	}
	else {
		while (*s1 && *s1 == *s2) {
			++s1;
			++s2;
		}
	}
	return *s1 - *s2;
}

#ifdef	__cplusplus
}
#endif
