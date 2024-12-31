//
// Created by hujianzhe
//

#ifndef UTIL_C_CRT_STRING_H
#define	UTIL_C_CRT_STRING_H

#include "../datastruct/memfunc.h"
#include <string.h>

#ifdef	__cplusplus
extern "C" {
#endif

__declspec_dll int strFormatLen(const char* format, ...);
__declspec_dll char* strFormat(int* out_len, const char* format, ...);
__declspec_dll void strFreeMemory(char* s);

#ifdef	__cplusplus
}
#endif

#endif // !UTIL_C_CRT_STRING_H
