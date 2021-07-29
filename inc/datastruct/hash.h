//
// Created by hujianzhe
//

#ifndef	UTIL_C_DATASTRUCT_HASH_H
#define	UTIL_C_DATASTRUCT_HASH_H

#include "../compiler_define.h"

#ifdef	__cplusplus
extern "C" {
#endif

__declspec_dll unsigned int hashBKDR(const char *str);
__declspec_dll unsigned int hashDJB(const char *str);
__declspec_dll unsigned int hashJenkins(const char *key, UnsignedPtr_t keylen);
__declspec_dll unsigned int hashMurmur2(const char *key, UnsignedPtr_t keylen);

#ifdef	__cplusplus
}
#endif

#endif // !UTIL_C_DATASTRUCT_HASH_H
