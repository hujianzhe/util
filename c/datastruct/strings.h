//
// Created by hujianzhe
//

#ifndef UTIL_C_DATASTRUCT_STRINGS_H
#define	UTIL_C_DATASTRUCT_STRINGS_H

#include "../compiler_define.h"

#ifdef	__cplusplus
extern "C" {
#endif

__declspec_dll ptrlen_t strLen(const char* s);
__declspec_dll char* strChr(const char* s, ptrlen_t len, const char c);
__declspec_dll char* strStr(const char* s1, ptrlen_t s1len, const char* s2, ptrlen_t s2len);
__declspec_dll char* strSplit(char** s, const char* delim);
__declspec_dll void strTrim(const char* str, ptrlen_t len, char** newstr, ptrlen_t* newlen);
__declspec_dll ptrlen_t strLenUtf8(const char* s);
__declspec_dll char* strCopy(char* dst, ptrlen_t dst_len, const char* src, ptrlen_t src_len);
__declspec_dll int strCmp(const char* s1, const char* s2, ptrlen_t n);

#ifdef	__cplusplus
}
#endif

#endif // !UTIL_C_DATASTRUCT_STRINGS_H
