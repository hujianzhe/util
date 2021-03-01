//
// Created by hujianzhe
//

#include "../../inc/datastruct/memfunc.h"

#ifdef	__cplusplus
extern "C" {
#endif

int byteorderIsLE(void) {
	unsigned short v = 0x0001;
	return *((unsigned char*)&v);
}

unsigned short memBE16(unsigned short v) {
	if (byteorderIsLE()) {
		memReverse(&v, sizeof(v));
	}
	return v;
}

unsigned short memLE16(unsigned short v) {
	if (!byteorderIsLE()) {
		memReverse(&v, sizeof(v));
	}
	return v;
}

unsigned short BE16toNative(unsigned short v) {
	if (byteorderIsLE()) {
		memReverse(&v, sizeof(v));
	}
	return v;
}

unsigned short LE16toNative(unsigned short v) {
	if (!byteorderIsLE()) {
		memReverse(&v, sizeof(v));
	}
	return v;
}

unsigned int memBE32(unsigned int v) {
	if (byteorderIsLE()) {
		memReverse(&v, sizeof(v));
	}
	return v;
}

unsigned int memLE32(unsigned int v) {
	if (!byteorderIsLE()) {
		memReverse(&v, sizeof(v));
	}
	return v;
}

unsigned int BE32toNative(unsigned int v) {
	if (byteorderIsLE()) {
		memReverse(&v, sizeof(v));
	}
	return v;
}

unsigned int LE32toNative(unsigned int v) {
	if (!byteorderIsLE()) {
		memReverse(&v, sizeof(v));
	}
	return v;
}

unsigned long long memBE64(unsigned long long v) {
	if (byteorderIsLE()) {
		memReverse(&v, sizeof(v));
	}
	return v;
}

unsigned long long memLE64(unsigned long long v) {
	if (!byteorderIsLE()) {
		memReverse(&v, sizeof(v));
	}
	return v;
}

unsigned long long BE64toNative(unsigned long long v) {
	if (byteorderIsLE()) {
		memReverse(&v, sizeof(v));
	}
	return v;
}

unsigned long long LE64toNative(unsigned long long v) {
	if (!byteorderIsLE()) {
		memReverse(&v, sizeof(v));
	}
	return v;
}

void memSwap(void* p1, void* p2, ptrlen_t n) {
	unsigned char* _p1 = (unsigned char*)p1;
	unsigned char* _p2 = (unsigned char*)p2;
	while (n--) {
		unsigned char b = *_p1;
		*(_p1++) = *_p2;
		*(_p2++) = b;
	}
}

void* memCopy(void* dst, const void* src, ptrlen_t n) {
	if (n && dst != src) {
		if (dst < src) {
			ptrlen_t i;
			for (i = 0; i < n; ++i)
				((unsigned char*)dst)[i] = ((unsigned char*)src)[i];
		}
		else {
			for (; n; --n)
				((unsigned char*)dst)[n - 1] = ((unsigned char*)src)[n - 1];
		}
	}
	return dst;
}

unsigned char* memSkipByte(const void* p, ptrlen_t n, const unsigned char* delim, ptrlen_t dn) {
	const unsigned char* ptr = (const unsigned char*)p;
	while (n--) {
		ptrlen_t i = 0;
		for (i = 0; i < dn; ++i)
			if (*ptr == delim[i])
				break;
		if (i == dn)
			return (unsigned char*)ptr;
		++ptr;
	}
	return (unsigned char*)0;
}

void* memZero(void* p, ptrlen_t n) {
	unsigned char* _p = (unsigned char*)p;
	while (n--)
		*(_p++) = 0;
	return p;
}

void* memReverse(void* p, ptrlen_t len) {
	unsigned char* _p = (unsigned char*)p;
	ptrlen_t i, half_len = len >> 1;
	for (i = 0; i < half_len; ++i) {
		unsigned char temp = _p[i];
		_p[i] = _p[len - i - 1];
		_p[len - i - 1] = temp;
	}
	return p;
}

unsigned short memCheckSum16(const void* buffer, int len) {
	const unsigned short* pbuf = (const unsigned short*)buffer;
	unsigned int cksum = 0;
	while (len > 1) {
		cksum += *pbuf++;
		len -= sizeof(unsigned short);
	}
	if (len)
		cksum += *(unsigned char*)pbuf;
	cksum = (cksum >> 16) + (cksum & 0xFFFF);
	cksum += (cksum >> 16);
	return (unsigned short)(~cksum);
}

void* memSearch(const void* buf, ptrlen_t n, const void* s, ptrlen_t sn) {
	const unsigned char* pbuf = (const unsigned char*)buf, *ps = (const unsigned char*)s;
	ptrlen_t i, j;
	for (i = 0; i < n; ++i) {
		if (n - i < sn)
			break;
		for (j = 0; j < sn; ++j) {
			if (pbuf[j] != ps[j])
				break;
		}
		if (j == sn)
			return (void*)pbuf;
		pbuf++;
	}
	return (void*)0;
}

void* memSearchValue(const void* buf, ptrlen_t n, const void* d, ptrlen_t dn) {
	const unsigned char* pbuf = (const unsigned char*)buf, *ps = (const unsigned char*)d;
	ptrlen_t i = 0, j;
	while (i < n) {
		if (n - i < dn)
			break;
		for (j = 0; j < dn; ++j) {
			if (pbuf[j] != ps[j])
				break;
		}
		if (j == dn)
			return (void*)pbuf;
		i += dn;
		pbuf += dn;
	}
	return (void*)0;
}

char* strSkipByte(const char* s, const char* delim) {
	while (*s) {
		const char* p = delim;
		while (*p) {
			if (*p == *s)
				break;
			++p;
		}
		if (0 == *p)
			break;
		++s;
	}
	return (char*)s;
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

ptrlen_t strLenUtf8(const char* s, ptrlen_t s_bytelen) {
	ptrlen_t u8_len = 0;
	ptrlen_t i;
	for (i = 0; i < s_bytelen; ++i) {
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

static int alphabet_no_case_delta(const char c) {
	if (c >= 'a' && c <= 'z')
		return c - 'a';
	else if (c >= 'A' && c <= 'Z')
		return c - 'A';
	else
		return c - 'a';
}

int strCmpNoCase(const char* s1, const char* s2, ptrlen_t n) {
	if (!n || !s1 || !s2)
		return 0;
	if (-1 != n) {
		while (n-- && *s1) {
			if (*s1 != *s2 && alphabet_no_case_delta(*s1) != alphabet_no_case_delta(*s2))
				return *s1 - *s2;
			++s1;
			++s2;
		}
	}
	else {
		while (*s1) {
			if (*s1 != *s2 && alphabet_no_case_delta(*s1) != alphabet_no_case_delta(*s2))
				return *s1 - *s2;
			++s1;
			++s2;
		}
	}
	return 0;
}

#ifdef	__cplusplus
}
#endif
