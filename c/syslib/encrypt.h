//
// Created by hujianzhe
//

#ifndef	UTIL_C_SYSLIB_ENCRYPT_H
#define	UTIL_C_SYSLIB_ENCRYPT_H

#include "platform_define.h"
#include <ctype.h>
#if defined(_WIN32) || defined(_WIN64)
	#pragma comment(lib, "Advapi32.lib")
	#pragma comment(lib, "Crypt32.lib")
	typedef struct __WIN32_CRYPT_CTX {
		HCRYPTPROV hProv;
		HCRYPTHASH hHash;
		DWORD dwLen;
	} CC_MD5_CTX,CC_SHA1_CTX;
	#define	CC_SHA1_DIGEST_LENGTH	20
	#define	CC_MD5_DIGEST_LENGTH	16
#else
	#ifdef	__APPLE__
		#include <CommonCrypto/CommonCrypto.h>
	#else
		#include <openssl/md5.h>
		#include <openssl/sha.h>
		typedef	MD5_CTX					CC_MD5_CTX;
		typedef SHA_CTX					CC_SHA1_CTX;
		#define	CC_SHA1_DIGEST_LENGTH	SHA_DIGEST_LENGTH
		#define	CC_MD5_DIGEST_LENGTH	MD5_DIGEST_LENGTH
	#endif
#endif

#ifdef	__cplusplus
extern "C" {
#endif

/* url */
size_t url_Encode(const char* src, size_t srclen, char* dst);
size_t url_Decode(const char* src, size_t srclen, char* dst);
/* base64 */
size_t base64_Encode(const unsigned char* src, size_t srclen, char* dst);
size_t base64_Decode(const char* src, size_t srclen, unsigned char* dst);
/* md5 */
EXEC_RETURN md5_Init(CC_MD5_CTX* ctx);
EXEC_RETURN md5_Update(CC_MD5_CTX* ctx, const void* data, size_t len);
EXEC_RETURN md5_Final(unsigned char* md5, CC_MD5_CTX* ctx);
EXEC_RETURN md5_Clean(CC_MD5_CTX* ctx);
/* sha1 */
EXEC_RETURN sha1_Init(CC_SHA1_CTX* ctx);
EXEC_RETURN sha1_Update(CC_SHA1_CTX* ctx, const void* data, size_t len);
EXEC_RETURN sha1_Final(unsigned char* sha1, CC_SHA1_CTX* ctx);
EXEC_RETURN sha1_Clean(CC_SHA1_CTX* ctx);

#ifdef	__cplusplus
}
#endif

#endif
