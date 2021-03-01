//
// Created by hujianzhe
//

#ifndef UTIL_C_DATASTRUCT_MEMFUNC_H
#define	UTIL_C_DATASTRUCT_MEMFUNC_H

#include "../compiler_define.h"

#ifdef	__cplusplus
extern "C" {
#endif

__declspec_dll int byteorderIsLE(void);
__declspec_dll unsigned short memBE16(unsigned short v);
__declspec_dll unsigned short memLE16(unsigned short v);
__declspec_dll unsigned int memBE32(unsigned int v);
__declspec_dll unsigned int memLE32(unsigned int v);
__declspec_dll unsigned long long memBE64(unsigned long long v);
__declspec_dll unsigned long long memLE64(unsigned long long v);

__declspec_dll void memSwap(void* p1, void* p2, ptrlen_t n);
__declspec_dll void* memCopy(void* dst, const void* src, ptrlen_t n);
__declspec_dll unsigned char* memSkipByte(const void* p, ptrlen_t n, const unsigned char* delim, ptrlen_t dn);
__declspec_dll void* memZero(void* p, ptrlen_t n);
__declspec_dll void* memReverse(void* p, ptrlen_t len);
__declspec_dll unsigned short memCheckSum16(const void* buffer, int len);
#define	memCheckSumIsOk(cksum)	(0 == (cksum))
__declspec_dll void* memSearch(const void* buf, ptrlen_t n, const void* s, ptrlen_t sn);
__declspec_dll void* memSearchValue(const void* buf, ptrlen_t n, const void* d, ptrlen_t dn);

__declspec_dll char* strSkipByte(const char* s, const char* delim);
__declspec_dll char* strStr(const char* s1, ptrlen_t s1len, const char* s2, ptrlen_t s2len);
__declspec_dll char* strSplit(char** s, const char* delim);
__declspec_dll ptrlen_t strLenUtf8(const char* s, ptrlen_t s_bytelen);
__declspec_dll int strCmpNoCase(const char* s1, const char* s2, ptrlen_t n);

#ifdef	__cplusplus
}
#endif

#endif // !UTIL_C_DATASTRUCT_MEMFUNC_H
