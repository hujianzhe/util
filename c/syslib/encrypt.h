//
// Created by hujianzhe
//

#ifndef	UTIL_C_SYSLIB_ENCRYPT_H
#define	UTIL_C_SYSLIB_ENCRYPT_H

#include "../platform_define.h"

#if defined(_WIN32) || defined(_WIN64)
	#pragma comment(lib, "Advapi32.lib")
	#pragma comment(lib, "Crypt32.lib")
	#define	CC_SHA1_DIGEST_LENGTH	20
	#define	CC_MD5_DIGEST_LENGTH	16
#else
	#ifdef	__APPLE__
		#include <CommonCrypto/CommonCrypto.h>
	#else
		#include <openssl/md5.h>
		#include <openssl/sha.h>
		#define	CC_SHA1_DIGEST_LENGTH	SHA_DIGEST_LENGTH
		#define	CC_MD5_DIGEST_LENGTH	MD5_DIGEST_LENGTH
	#endif
#endif

#ifdef	__cplusplus
extern "C" {
#endif

__declspec_dll size_t cryptUrlEncode(const char* src, size_t srclen, char* dst);
__declspec_dll size_t cryptUrlDecode(const char* src, size_t srclen, char* dst);
__declspec_dll size_t cryptBase64Encode(const unsigned char* src, size_t srclen, char* dst);
__declspec_dll size_t cryptBase64Decode(const char* src, size_t srclen, unsigned char* dst);
__declspec_dll BOOL cryptMD5Encode(const void* data, size_t len, unsigned char* md5);
__declspec_dll BOOL cryptSHA1Encode(const void* data, size_t len, unsigned char* sha1);

#ifdef	__cplusplus
}
#endif

#endif
