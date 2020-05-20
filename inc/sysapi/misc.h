//
// Created by hujianzhe
//

#ifndef	UTIL_C_SYSLIB_MISC_H
#define UTIL_C_SYSLIB_MISC_H

#include "../platform_define.h"

#if defined(_WIN32) || defined(_WIN64)
	#include <malloc.h>
	#ifndef alloca
		#define alloca		_alloca
	#endif
#else
	#if	__linux__
		#include <alloca.h>
		#include <malloc.h>
	#endif
#endif
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#define	alignedAlloca(nbytes, alignment)\
((void*)(((size_t)alloca(nbytes + alignment)) + (alignment - 1) & ~(((size_t)alignment) - 1)))

#define	vaStringFormatLen(len, str_format)\
do {\
	char test_buf##len;\
	const char* format##len = (str_format);\
	va_list varg##len;\
	va_start(varg##len, format##len);\
	len = vsnprintf(&test_buf##len, 0, format##len, varg##len);\
	va_end(varg##len);\
} while (0)

#ifdef	__cplusplus
extern "C" {
#endif

__declspec_dll void* alignMalloc(size_t nbytes, size_t alignment);
__declspec_dll void alignFree(const void* p);

__declspec_dll int strFormatLen(const char* format, ...);
__declspec_dll char* strFormat(int* out_len, const char* format, ...);
__declspec_dll unsigned int iobufSharedCopy(const Iobuf_t* iov, unsigned int iovcnt, unsigned int* iov_i, unsigned int* iov_off, void* buf, unsigned int n);

#ifdef	__cplusplus
}
#endif

#endif