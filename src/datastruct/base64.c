//
// Created by hujianzhe
//

#include "../../inc/datastruct/base64.h"

#ifdef	__cplusplus
extern "C" {
#endif

static const unsigned char base64map[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";

ptrlen_t base64Encode(const unsigned char* src, ptrlen_t srclen, char* dst) {
	ptrlen_t i, dstLen;
	ptrlen_t leven = 3 * (srclen / 3);
	for (dstLen = 0, i = 0; i < leven; i += 3) {
		if (dst) {
			dst[dstLen] = base64map[src[0] >> 2];
			dst[dstLen + 1] = base64map[((src[0] & 3) << 4) + (src[1] >> 4)];
			dst[dstLen + 2] = base64map[((src[1] & 0xf) << 2) + (src[2] >> 6)];
			dst[dstLen + 3] = base64map[src[2] & 0x3f];
		}
		dstLen += 4;
		src += 3;
	}
	if (i < srclen) {
		unsigned char a = src[0];
		unsigned char b = (unsigned char)((i + 1 < srclen) ? src[1] : 0);
		unsigned c = 0;

		if (dst) {
			dst[dstLen] = base64map[a >> 2];
			dst[dstLen + 1] = base64map[((a & 3) << 4) + (b >> 4)];
			dst[dstLen + 2] = (char)((i + 1 < srclen) ? base64map[((b & 0xf) << 2) + (c >> 6)] : '=');
			dst[dstLen + 3] = '=';
		}
		dstLen += 4;
	}
	if (dst) {
		dst[dstLen] = 0;
	}
	return dstLen;
}

static unsigned char base64byte(char c) {
	if (c >= '0' && c <= '9')
		return (unsigned char)(c - '0' + 52);
	if (c >= 'A' && c <= 'Z')
		return (unsigned char)(c - 'A');
	if (c >= 'a' && c <= 'z')
		return (unsigned char)(c - 'a' + 26);
	if ('+' == c)
		return 62;
	if ('/' == c)
		return 63;
	else
		return 64;
}

ptrlen_t base64Decode(const char* src, ptrlen_t srclen, unsigned char* dst) {
	unsigned char input[4];
	ptrlen_t i, dstLen;
	for (dstLen = 0, i = 0; i < srclen; i += 4) {
		if (dst) {
			input[0] = base64byte(src[i]);
			input[1] = base64byte(src[i + 1]);
			dst[dstLen] = (input[0] << 2) + (input[1] >> 4);
		}
		++dstLen;
		if (src[i + 2] != '=') {
			if (dst) {
				input[2] = base64byte(src[i + 2]);
				dst[dstLen] = (input[1] << 4) + (input[2] >> 2);
			}
			++dstLen;
		}
		if (src[i + 3] != '=') {
			if (dst) {
				input[3] = base64byte(src[i + 3]);
				dst[dstLen] = (input[2] << 6) + input[3];
			}
			++dstLen;
		}
	}
	return dstLen;
}

#ifdef	__cplusplus
}
#endif