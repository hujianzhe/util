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
__declspec_dll unsigned short memToBE16(unsigned short v);
__declspec_dll unsigned short memToLE16(unsigned short v);
__declspec_dll unsigned short memFromBE16(unsigned short v);
__declspec_dll unsigned short memFromLE16(unsigned short v);
__declspec_dll unsigned int memToBE32(unsigned int v);
__declspec_dll unsigned int memToLE32(unsigned int v);
__declspec_dll unsigned int memFromBE32(unsigned int v);
__declspec_dll unsigned int memFromLE32(unsigned int v);
__declspec_dll unsigned long long memToBE64(unsigned long long v);
__declspec_dll unsigned long long memToLE64(unsigned long long v);
__declspec_dll unsigned long long memFromBE64(unsigned long long v);
__declspec_dll unsigned long long memFromLE64(unsigned long long v);

__declspec_dll int memBitCheck(char* arr, UnsignedPtr_t bit_idx);
__declspec_dll void memBitSet(char* arr, UnsignedPtr_t bit_idx);
__declspec_dll void memBitUnset(char* arr, UnsignedPtr_t bit_idx);

__declspec_dll void memSwap(void* p1, void* p2, UnsignedPtr_t n);
__declspec_dll void* memCopy(void* dst, const void* src, UnsignedPtr_t n);
__declspec_dll unsigned char* memSkipByte(const void* p, UnsignedPtr_t n, const unsigned char* delim, UnsignedPtr_t dn);
__declspec_dll void* memZero(void* p, UnsignedPtr_t n);
__declspec_dll void* memReverse(void* p, UnsignedPtr_t len);
__declspec_dll unsigned short memCheckSum16(const void* buffer, int len);
#define	memCheckSumIsOk(cksum)	(0 == (cksum))
__declspec_dll void* memSearch(const void* buf, UnsignedPtr_t n, const void* s, UnsignedPtr_t sn);
__declspec_dll void* memSearchValue(const void* buf, UnsignedPtr_t n, const void* d, UnsignedPtr_t dn);

__declspec_dll char* strSkipByte(const char* s, const char* delim);
__declspec_dll char* strChr(const char* s, UnsignedPtr_t n, char c);
__declspec_dll char* strStr(const char* s1, UnsignedPtr_t s1len, const char* s2, UnsignedPtr_t s2len);
__declspec_dll char* strSplit(const char* str, UnsignedPtr_t len, const char** p_sc, const char* delim);
__declspec_dll UnsignedPtr_t strLenUtf8(const char* s, UnsignedPtr_t s_bytelen);
__declspec_dll int strUtf8CharacterByteNum(const char* s);
__declspec_dll int strCmpNoCase(const char* s1, const char* s2, UnsignedPtr_t n);

#ifdef	__cplusplus
}
#endif

#endif // !UTIL_C_DATASTRUCT_MEMFUNC_H
