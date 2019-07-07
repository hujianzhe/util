//
// Created by hujianzhe
//

#ifndef UTIL_C_DATASTRUCT_BASE64_H
#define	UTIL_C_DATASTRUCT_BASE64_H

#include "../compiler_define.h"

#ifdef	__cplusplus
extern "C" {
#endif

#define	base64EncodeLength(len)	(((len) + 2) / 3 * 4)
__declspec_dll ptrlen_t base64Encode(const unsigned char* src, ptrlen_t srclen, char* dst);
#define	base64DecodeLength(len)	(((len) + 3) / 4 * 3)
__declspec_dll ptrlen_t base64Decode(const char* src, ptrlen_t srclen, unsigned char* dst);

#ifdef	__cplusplus
}
#endif

#endif // !UTIL_C_DATASTRUCT_BASE64_H
