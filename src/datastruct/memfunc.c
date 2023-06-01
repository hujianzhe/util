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

unsigned short memToBE16(unsigned short v) {
	if (byteorderIsLE()) {
		memReverse(&v, sizeof(v));
	}
	return v;
}

unsigned short memToLE16(unsigned short v) {
	if (!byteorderIsLE()) {
		memReverse(&v, sizeof(v));
	}
	return v;
}

unsigned short memFromBE16(unsigned short v) {
	if (byteorderIsLE()) {
		memReverse(&v, sizeof(v));
	}
	return v;
}

unsigned short memFromLE16(unsigned short v) {
	if (!byteorderIsLE()) {
		memReverse(&v, sizeof(v));
	}
	return v;
}

unsigned int memToBE32(unsigned int v) {
	if (byteorderIsLE()) {
		memReverse(&v, sizeof(v));
	}
	return v;
}

unsigned int memToLE32(unsigned int v) {
	if (!byteorderIsLE()) {
		memReverse(&v, sizeof(v));
	}
	return v;
}

unsigned int memFromBE32(unsigned int v) {
	if (byteorderIsLE()) {
		memReverse(&v, sizeof(v));
	}
	return v;
}

unsigned int memFromLE32(unsigned int v) {
	if (!byteorderIsLE()) {
		memReverse(&v, sizeof(v));
	}
	return v;
}

unsigned long long memToBE64(unsigned long long v) {
	if (byteorderIsLE()) {
		memReverse(&v, sizeof(v));
	}
	return v;
}

unsigned long long memToLE64(unsigned long long v) {
	if (!byteorderIsLE()) {
		memReverse(&v, sizeof(v));
	}
	return v;
}

unsigned long long memFromBE64(unsigned long long v) {
	if (byteorderIsLE()) {
		memReverse(&v, sizeof(v));
	}
	return v;
}

unsigned long long memFromLE64(unsigned long long v) {
	if (!byteorderIsLE()) {
		memReverse(&v, sizeof(v));
	}
	return v;
}

int memBitCheck(char* arr, UnsignedPtr_t bit_idx) {
	return (arr[bit_idx >> 3] >> (bit_idx & 7)) & 1;
}

void memBitSet(char* arr, UnsignedPtr_t bit_idx) {
	arr[bit_idx >> 3] |= (1 << (bit_idx & 7));
}

void memBitUnset(char* arr, UnsignedPtr_t bit_idx) {
	arr[bit_idx >> 3] &= ~(1 << (bit_idx & 7));
}

void memSwap(void* p1, void* p2, UnsignedPtr_t n) {
	unsigned char* _p1 = (unsigned char*)p1;
	unsigned char* _p2 = (unsigned char*)p2;
	while (n--) {
		unsigned char b = *_p1;
		*(_p1++) = *_p2;
		*(_p2++) = b;
	}
}

void* memCopy(void* dst, const void* src, UnsignedPtr_t n) {
	if (n && dst != src) {
		if (dst < src) {
			UnsignedPtr_t i;
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

unsigned char* memSkipByte(const void* p, UnsignedPtr_t n, const unsigned char* delim, UnsignedPtr_t dn) {
	const unsigned char* ptr = (const unsigned char*)p;
	while (n--) {
		UnsignedPtr_t i = 0;
		for (i = 0; i < dn; ++i)
			if (*ptr == delim[i])
				break;
		if (i == dn)
			return (unsigned char*)ptr;
		++ptr;
	}
	return (unsigned char*)0;
}

void* memZero(void* p, UnsignedPtr_t n) {
	unsigned char* _p = (unsigned char*)p;
	while (n--)
		*(_p++) = 0;
	return p;
}

void* memReverse(void* p, UnsignedPtr_t len) {
	unsigned char* _p = (unsigned char*)p;
	UnsignedPtr_t i, half_len = len >> 1;
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

void* memSearch(const void* buf, UnsignedPtr_t n, const void* s, UnsignedPtr_t sn) {
	const unsigned char* pbuf = (const unsigned char*)buf, *ps = (const unsigned char*)s;
	UnsignedPtr_t i, j;
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

void* memSearchValue(const void* buf, UnsignedPtr_t n, const void* d, UnsignedPtr_t dn) {
	const unsigned char* pbuf = (const unsigned char*)buf, *ps = (const unsigned char*)d;
	UnsignedPtr_t i = 0, j;
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

char* strChr(const char* s, UnsignedPtr_t n, char c) {
	UnsignedPtr_t i;
	for (i = 0; i < n; ++i, ++s) {
		if (0 == *s) {
			break;
		}
		if (c == *s) {
			return (char*)s;
		}
	}
	return (char*)0;
}

char* strStr(const char* s1, UnsignedPtr_t s1len, const char* s2, UnsignedPtr_t s2len) {
	const char *p1, *p2;
	UnsignedPtr_t i;
	if (!s1 || !s2)
		return (char*)0;
	if (-1 == s2len)
		for (p2 = s2, s2len = 0; *p2; ++p2, ++s2len);
	if (-1 == s1len)
		for (p1 = s1, s1len = 0; *p1; ++p1, ++s1len);
	for (i = 0; s2len <= s1len - i && s1[i]; ++i) {
		UnsignedPtr_t len;
		for (p1 = s1 + i, p2 = s2, len = s2len; len && *p2 && *p1 && *p1 == *p2; ++p1, ++p2, --len);
		if ('\0' == *p2 || 0 == len)
			return (char*)(s1 + i);
	}
	return (char*)0;
}

/**
 * code example:
 *
 * const char* data = argv[1];
 * size_t datalen = strlen(data);
 * const char* sl = NULL, *s;
 * while (s = strSplit(data, datalen, &sl, ":")) {
 *     printf("%.*s\n", sl - s, s);
 * }
 */
char* strSplit(const char* str, UnsignedPtr_t len, const char** p_sc, const char* delim) {
	char* beg;
	const char* sc;
	if (len <= 0) {
		return (char*)0;
	}
	sc = *p_sc;
	if (sc) {
		++sc;
		if (sc <= str || sc - str > len) {
			return (char*)0;
		}
	}
	else {
		sc = str;
	}
	if (sc - str == len || 0 == *sc) {
		char last_c = sc[-1];
		const char* pd = delim;
		while (*pd) {
			if (*pd != last_c) {
				++pd;
				continue;
			}
			*p_sc = sc;
			return (char*)sc;
		}
		return (char*)0;
	}
	beg = (char*)sc;
	while (sc - str < len && *sc) {
		const char* pd = delim;
		while (*pd) {
			if (*pd != *sc) {
				++pd;
				continue;
			}
			*p_sc = sc;
			return beg;
		}
		++sc;
	}
	*p_sc = sc;
	return beg;
}

UnsignedPtr_t strLenUtf8(const char* s, UnsignedPtr_t s_bytelen) {
	UnsignedPtr_t u8_len = 0;
	UnsignedPtr_t i;
	for (i = 0; i < s_bytelen && s[i]; ++u8_len) {
		int bn = strUtf8CharacterByteNum(s + i);
		if (bn < 0) {
			return -1;
		}
		i += bn;
		if (i > s_bytelen) {
			break;
		}
	}
	return u8_len;
}

int strUtf8CharacterByteNum(const char* s) {
	unsigned char c = *s;
	if (c < 0x80) {
		return 1;
	}
	else if (c < 0xc0) {
		return -1;
	}
	else if (c < 0xe0) {
		return 2;
	}
	else if (c < 0xf0) {
		return 3;
	}
	else if (c < 0xf8) {
		return 4;
	}
	else if (c < 0xfc) {
		return 5;
	}
	else if (c < 0xfe) {
		return 6;
	}
	return -1;
}

static int alphabet_no_case_delta(const char c) {
	if (c >= 'a' && c <= 'z')
		return c - 'a';
	else if (c >= 'A' && c <= 'Z')
		return c - 'A';
	else
		return c - 'a';
}

int strCmpNoCase(const char* s1, const char* s2, UnsignedPtr_t n) {
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

int strIsInteger(const char* s, UnsignedPtr_t n) {
	UnsignedPtr_t i;
	for (i = 0; i < n && s[i]; ++i) {
		if ('+' == s[i] || '-' == s[i]) {
			if (i != 0) {
				return 0;
			}
			continue;
		}
		if (s[i] < '0' || s[i] > '9') {
			return 0;
		}
	}
	return 1;
}

#ifdef	__cplusplus
}
#endif
