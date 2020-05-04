/* ================ sha1.h ================ */
/*
SHA-1 in C
By Steve Reid <steve@edmweb.com>
100% Public Domain
*/

#ifndef UTIL_C_DATASTRUCT_SHA1_H
#define	UTIL_C_DATASTRUCT_SHA1_H

#include "../compiler_define.h"

typedef struct {
	unsigned int state[5];
	unsigned int count[2];
	unsigned char buffer[64];
} SHA1_CTX;

#ifdef __cplusplus
extern "C" {
#endif

__declspec_dll void SHA1Transform(unsigned int state[5], const unsigned char buffer[64]);
__declspec_dll void SHA1Init(SHA1_CTX* context);
__declspec_dll void SHA1Update(SHA1_CTX* context, const unsigned char* data, unsigned int len);
__declspec_dll void SHA1Final(unsigned char digest[20], SHA1_CTX* context);

#ifdef __cplusplus
}
#endif

#endif