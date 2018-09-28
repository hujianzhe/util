//
// Created by hujianzhe
//

#include "strings.h"

#ifdef	__cplusplus
extern "C" {
#endif

ptrlen_t strLen(const char* s) {
	if (!s)
		return 0;
	else {
		const char* p = s;
		while (*p)
			++p;
		return p - s;
	}
}

char* strChr(const char* s, ptrlen_t len, const char c) {
	if (!s)
		return (char*)0;
	while (len && *s && *s != c) {
		--len;
		++s;
	}
	return len && *s ? (char*)s : (char*)0;
}

char* strStr(const char* s1, ptrlen_t s1len, const char* s2, ptrlen_t s2len) {
	const char *p1, *p2;
	ptrlen_t i;
	if (!s1 || !s2)
		return (char*)0;
	if (-1 == s2len)
		for (p2 = s2, s2len = 0; *p2; ++p2, ++s2len);
	if (-1 == s1len)
		for (p1 = s1, s1len = 0; *p1; ++p1, ++s1len);
	for (i = 0; i + s2len <= s1len && s1[i]; ++i) {
		ptrlen_t len;
		for (p1 = s1 + i, p2 = s2, len = s2len; len && *p2 && *p1 && *p1 == *p2; ++p1, ++p2, --len);
		if ('\0' == *p2 || 0 == len)
			return (char*)(s1 + i);
	}
	return (char*)0;
}

char* strSplit(char** s, const char* delim) {
	int has_split;
	char *ss, *sp;
	if (!s || !*s)
		return (char*)0;
	has_split = 0;
	sp = ss = *s;
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
	return ss != sp ? ss : (char*)0;
}

void strTrim(const char* str, ptrlen_t len, char** newstr, ptrlen_t* newlen) {
	const char* end;
	if (!str) {
		*newstr = (char*)0;
		*newlen = 0;
		return;
	}
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

ptrlen_t strLenUtf8(const char* s) {
	ptrlen_t u8_len = 0;
	ptrlen_t byte_len = strLen(s);
	ptrlen_t i;
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

char* strCopy(char* dst, ptrlen_t dst_len, const char* src, ptrlen_t src_len) {
	ptrlen_t i;
	ptrlen_t len = dst_len > src_len ? src_len : dst_len;
	if (-1 == len)
		return (char*)0;
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

int strCmp(const char* s1, const char* s2, ptrlen_t n) {
	if (!n || !s1 || !s2)
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
