//
// Created by hujianzhe
//

#ifndef UTIL_C_DATASTRUCT_MD5_H
#define	UTIL_C_DATASTRUCT_MD5_H

#include "../compiler_define.h"

typedef struct {
	unsigned int count[2];
	unsigned int state[4];
	unsigned char buffer[64];
} MD5_CTX;

#ifdef	__cplusplus
extern "C" {
#endif

__declspec_dll void MD5Transform(unsigned int state[4], unsigned char block[64]);
__declspec_dll void MD5Init(MD5_CTX *context);
__declspec_dll void MD5Update(MD5_CTX *context, unsigned char *input, unsigned int inputlen);
__declspec_dll void MD5Final(MD5_CTX *context, unsigned char digest[16]);

#ifdef	__cplusplus
}
#endif

#endif // !UTIL_C_DATASTRUCT_MD5_H
