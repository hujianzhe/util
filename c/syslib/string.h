//
// Created by hujianzhe
//

#ifndef UTIL_C_SYSLIB_STRING_H
#define UTIL_C_SYSLIB_STRING_H

#include "../platform_define.h"
#include <string.h>

#ifdef	__cplusplus
extern "C" {
#endif

__declspec_dll size_t strLength(const char* s);
__declspec_dll char* strChr(const char* s, size_t len, const char c);
__declspec_dll char* strStr(const char* s1, size_t s1len, const char* s2, size_t s2len);
__declspec_dll char* strSplit(char** s, const char* delim);
__declspec_dll void strTrim(const char* str, size_t len, char** newstr, size_t* newlen);
__declspec_dll size_t strlenUTF8(const char* s);
__declspec_dll char* strCopy(char* dst, size_t dst_len, const char* src, size_t src_len);
__declspec_dll int strCmp(const char* s1, const char* s2, size_t n);

#ifdef	__cplusplus
}
#endif

#endif
