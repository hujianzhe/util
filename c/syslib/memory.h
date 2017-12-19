//
// Created by hujianzhe
//

#ifndef	UTIL_C_SYSLIB_MEMORY_H
#define UTIL_C_SYSLIB_MEMORY_H

#include "platform_define.h"

#if defined(_WIN32) || defined(_WIN64)
	#define	MAP_FAILED		NULL
#else
	#include <sys/mman.h>
#endif

typedef struct {
	long granularity;
#if defined(_WIN32) || defined(_WIN64)
	HANDLE __handle;
#else
	int __fd;
#endif
} MEMORY_MAPPING;

#ifdef	__cplusplus
extern "C" {
#endif

/* memory align alloc */
#define	ctr_align_alloca(nbytes, alignment)	((void*)(((size_t)alloca(nbytes + alignment)) + (alignment - 1) & ~(((int)alignment) - 1)))
void* crt_align_malloc(size_t nbytes, int alignment);
void crt_align_free(void* p);
/* mmap */
EXEC_RETURN mmap_Create(MEMORY_MAPPING* mm, FD_HANDLE fd, const char* name, size_t nbytes);
EXEC_RETURN mmap_Open(MEMORY_MAPPING* mm, const char* name);
EXEC_RETURN mmap_Close(MEMORY_MAPPING* mm);
void* mmap_Map(MEMORY_MAPPING* mm, void* va_base, long long offset, size_t nbytes);
EXEC_RETURN mmap_Sync(void* addr, size_t nbytes);
EXEC_RETURN mmap_Unmap(void* addr, size_t nbytes);

#ifdef	__cplusplus
}
#endif

#endif
