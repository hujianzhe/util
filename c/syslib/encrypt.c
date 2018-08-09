//
// Created by hujianzhe
//

#include "encrypt.h"
#include <ctype.h>

#ifdef  __cplusplus
extern "C" {
#endif

/* url */
size_t crypt_url_encode(const char* src, size_t srclen, char* dst) {
	static const char hex2char[] = "0123456789ABCDEF";
	size_t i, dstlen;
	for (dstlen = 0, i = 0; i < srclen; ++i) {
		char c = src[i];
		if (isdigit(c) || isalpha(c)) {
			if (dst) {
				dst[dstlen] = c;
			}
			++dstlen;
		}
		else if (c == ' ') {
			if (dst) {
				dst[dstlen] = '+';
			}
			++dstlen;
		}
		else {
			if (dst) {
				dst[dstlen] = '%';
				dst[dstlen + 1] = hex2char[((unsigned char)c) >> 4];
				dst[dstlen + 2] = hex2char[c & 0xf];
			}
			dstlen += 3;
		}
	}
	if (dst) {
		dst[dstlen] = 0;
	}
	return dstlen;
}

size_t crypt_url_decode(const char* src, size_t srclen, char* dst) {
	size_t i, dstlen;
	for (dstlen = 0,i = 0; i < srclen; ++i) {
		char c = src[i];
		if (c == '+') {
			if (dst) {
				dst[dstlen] = ' ';
			}
			++dstlen;
		}
		else if (c == '%') {
			if (dst) {
				char ch = src[i + 1];
				char cl = src[i + 2];
				dst[dstlen] = (char) ((isdigit(ch) ? ch - '0' : toupper(ch) - 'A' + 10) << 4);
				dst[dstlen] |= (char) (isdigit(cl) ? cl - '0' : toupper(cl) - 'A' + 10);
			}
			i += 2;
			++dstlen;
		}
		else {
			if (dst) {
				dst[dstlen] = c;
			}
			++dstlen;
		}
	}
	if (dst) {
		dst[dstlen] = 0;
	}
	return dstlen;
}

/* base64 */
#if !defined(_WIN32) && !defined(_WIN64)
static const unsigned char base64map[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";
#endif
size_t crypt_base64_encode(const unsigned char* src, size_t srclen, char* dst) {
#if defined(_WIN32) || defined(_WIN64)
	DWORD dstLen;
	return CryptBinaryToStringA(src, srclen, CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, dst, &dstLen) ? dstLen : 0;
#else
	size_t i, dstLen;
	size_t leven = 3 * (srclen / 3);
	for (dstLen = 0, i = 0; i < leven; i += 3) {
		if(dst) {
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
#endif
}
#if !defined(_WIN32) && !defined(_WIN64)
static unsigned char base64byte(char c) {
	if (isdigit(c))
		return (unsigned char)(c - '0' + 52);
	if (isupper(c))
		return (unsigned char)(c - 'A');
	if (islower(c))
		return (unsigned char)(c - 'a' + 26);
	switch (c) {
		case '+':
			return 62;
		case '/':
			return 63;
		default:
			return 64;
	}
}
#endif
size_t crypt_base64_decode(const char* src, size_t srclen, unsigned char* dst) {
#if defined(_WIN32) || defined(_WIN64)
	DWORD dstLen;
	return CryptStringToBinaryA(src, srclen, CRYPT_STRING_BASE64, dst, &dstLen, NULL, NULL) ? dstLen : 0;
#else
	unsigned char input[4];
	size_t i, dstLen;
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
#endif
}

#if defined(_WIN32) || defined(_WIN64)
typedef struct __WIN32_CRYPT_CTX {
	HCRYPTPROV hProv;
	HCRYPTHASH hHash;
	DWORD dwLen;
} __WIN32_CRYPT_CTX;

static BOOL __win32_crypt_init(struct __WIN32_CRYPT_CTX* ctx) {
	ctx->hProv = ctx->hHash = 0;
	return CryptAcquireContext(&ctx->hProv, NULL, MS_DEF_PROV, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT);
}
static BOOL __win32_crypt_hash_create(struct __WIN32_CRYPT_CTX* ctx, DWORD Algid) {
	DWORD dwDataLen = sizeof(ctx->dwLen);
	if (!CryptCreateHash(ctx->hProv, Algid, 0, 0, &ctx->hHash)) {
		return FALSE;
	}
	return CryptGetHashParam(ctx->hHash, HP_HASHSIZE, (BYTE*)(&ctx->dwLen), &dwDataLen, 0);
}
static BOOL __win32_crypt_update(struct __WIN32_CRYPT_CTX* ctx, const BYTE* data, DWORD len) {
	return CryptHashData(ctx->hHash, (const BYTE*)data, len, 0);
}
static BOOL __win32_crypt_final(unsigned char* data, struct __WIN32_CRYPT_CTX* ctx) {
	return CryptGetHashParam(ctx->hHash, HP_HASHVAL, data, &ctx->dwLen, 0);
}
static BOOL __win32_crypt_clean(struct __WIN32_CRYPT_CTX* ctx) {
	if (ctx->hHash && !CryptDestroyHash(ctx->hHash)) {
		return FALSE;
	}
	return CryptReleaseContext(ctx->hProv, 0);
}
#endif

/* md5 */
BOOL crypt_md5_encode(const void* data, size_t len, unsigned char* md5) {
#if defined(_WIN32) || defined(_WIN64)
	__WIN32_CRYPT_CTX ctx;
	BOOL ok = __win32_crypt_init(&ctx);
	if (!ok)
		return FALSE;
	do {
		ok = FALSE;
		if (ctx.hHash == 0 && !__win32_crypt_hash_create(&ctx, CALG_MD5))
			break;
		if (!__win32_crypt_update(&ctx, (const BYTE*)data, len))
			break;
		if (!__win32_crypt_final(md5, &ctx))
			break;
		ok = TRUE;
	} while (0);
	__win32_crypt_clean(&ctx);
	return ok;
#elif	__APPLE__
	CC_MD5_CTX ctx;
	return CC_MD5_Init(&ctx) && CC_MD5_Update(&ctx, data, (CC_LONG)len) && CC_MD5_Final(md5, &ctx);
#else
	MD5_CTX ctx;
	return MD5_Init(&ctx) && MD5_Update(&ctx, data, len) && MD5_Final(md5, &ctx);
#endif
}

/* sha1 */
BOOL crypt_sha1_encode(const void* data, size_t len, unsigned char* sha1) {
#if defined(_WIN32) || defined(_WIN64)
	__WIN32_CRYPT_CTX ctx;
	BOOL ok = __win32_crypt_init(&ctx);
	if (!ok)
		return FALSE;
	do {
		if (ctx.hHash == 0 && !__win32_crypt_hash_create(&ctx, CALG_SHA1))
			break;
		if (!__win32_crypt_update(&ctx, (const BYTE*)data, len))
			break;
		if (!__win32_crypt_final(sha1, &ctx))
			break;
		ok = TRUE;
	} while (0);
	__win32_crypt_clean(&ctx);
	return ok;
#elif	__APPLE__
	CC_SHA1_CTX ctx;
	return CC_SHA1_Init(&ctx) && CC_SHA1_Update(&ctx, data, len) && CC_SHA1_Final(sha1, &ctx);
#else
	SHA_CTX ctx;
	return SHA1_Init(&ctx) && SHA1_Update(&ctx, data, len) && SHA1_Final(sha1, &ctx);
#endif
}

#ifdef  __cplusplus
}
#endif
