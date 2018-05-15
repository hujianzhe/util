//
// Created by hujianzhe
//

#ifndef	UTIL_C_SYSLIB_MMAP_H
#define UTIL_C_SYSLIB_MMAP_H

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
} MemoryMapping_t;

#ifdef	__cplusplus
extern "C" {
#endif

long mmap_Granularity(void);
BOOL mmap_Create(MemoryMapping_t* mm, FD_t fd, const char* name, size_t nbytes);
BOOL mmap_Open(MemoryMapping_t* mm, const char* name);
BOOL mmap_Close(MemoryMapping_t* mm);
void* mmap_Map(MemoryMapping_t* mm, void* va_base, long long offset, size_t nbytes);
BOOL mmap_Sync(void* addr, size_t nbytes);
BOOL mmap_Unmap(void* addr, size_t nbytes);

#ifdef	__cplusplus
}
#endif

#endif
