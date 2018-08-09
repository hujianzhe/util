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

/* url */
size_t crypt_url_encode(const char* src, size_t srclen, char* dst);
size_t crypt_url_decode(const char* src, size_t srclen, char* dst);
/* base64 */
size_t crypt_base64_encode(const unsigned char* src, size_t srclen, char* dst);
size_t crypt_base64_decode(const char* src, size_t srclen, unsigned char* dst);
/* md5 */
BOOL crypt_md5_encode(const void* data, size_t len, unsigned char* md5);
/* sha1 */
BOOL crypt_sha1_encode(const void* data, size_t len, unsigned char* sha1);

#ifdef	__cplusplus
}
#endif

#endif
