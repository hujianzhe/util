//
// Created by hujianzhe
//

#ifndef	UTIL_C_SYSLIB_ALLOCA_H
#define UTIL_C_SYSLIB_ALLOCA_H

#include "platform_define.h"

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
#include <stdlib.h>

#define	alloca_align(nbytes, alignment)\
((void*)(((size_t)alloca(nbytes + alignment)) + (alignment - 1) & ~(((size_t)alignment) - 1)))

#ifdef	__cplusplus
extern "C" {
#endif

void* malloc_align(size_t nbytes, size_t alignment);
void free_align(const void* p);

#ifdef	__cplusplus
}
#endif

#endif