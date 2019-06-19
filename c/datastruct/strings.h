//
// Created by hujianzhe
//

#ifndef UTIL_C_DATASTRUCT_STRINGS_H
#define	UTIL_C_DATASTRUCT_STRINGS_H

#include "../compiler_define.h"

#ifdef	__cplusplus
extern "C" {
#endif

__declspec_dll void memSwap(void* p1, void* p2, ptrlen_t n);
__declspec_dll unsigned char* memByteFind(const void* p, ptrlen_t n, unsigned char b);
__declspec_dll void* memZero(void* p, ptrlen_t n);
__declspec_dll void* memReverse(void* s, ptrlen_t len);
__declspec_dll char* strSkipByte(const char* s, const char* delim);
__declspec_dll char* strStr(const char* s1, ptrlen_t s1len, const char* s2, ptrlen_t s2len);
__declspec_dll char* strSplit(char** s, const char* delim);
__declspec_dll ptrlen_t strLenUtf8(const char* s, ptrlen_t s_bytelen);
__declspec_dll int strCmpIgnoreCase(const char* s1, const char* s2, ptrlen_t n);

#ifdef	__cplusplus
}
#endif

#endif // !UTIL_C_DATASTRUCT_STRINGS_H
