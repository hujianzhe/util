//
// Created by hujianzhe
//

#ifndef UTIL_C_DATASTRUCT_HASH_H
#define	UTIL_C_DATASTRUCT_HASH_H

#include "../compiler_define.h"

#ifdef	__cplusplus
extern "C" {
#endif

UTIL_LIBAPI unsigned int hashBKDR(const char* str);
UTIL_LIBAPI unsigned int hashDJB(const char *str);
UTIL_LIBAPI unsigned int hashJenkins(const char* key, unsigned int keylen);
UTIL_LIBAPI unsigned int hashMurmur2(const char *key, unsigned int keylen);

#ifdef	__cplusplus
}
#endif

#endif // !UTIL_C_DATASTRUCT_HASH_H
