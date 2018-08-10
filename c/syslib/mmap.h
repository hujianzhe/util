//
// Created by hujianzhe
//

#ifndef	UTIL_C_SYSLIB_MMAP_H
#define UTIL_C_SYSLIB_MMAP_H

#include "../platform_define.h"

#if defined(_WIN32) || defined(_WIN64)
	#define	MAP_FAILED		NULL
#else
	#include <sys/mman.h>
#endif

typedef struct {
#if defined(_WIN32) || defined(_WIN64)
	HANDLE __handle;
#else
	int __fd;
	int __isref;
#endif
} MemoryMapping_t;

#ifdef	__cplusplus
extern "C" {
#endif

long memoryPageSize(void);
unsigned long long memorySize(void);

BOOL memoryCreateMapping(MemoryMapping_t* mm, FD_t fd, const char* name, size_t nbytes);
BOOL memoryOpenMapping(MemoryMapping_t* mm, const char* name);
BOOL memoryCloseMapping(MemoryMapping_t* mm);
void* memoryDoMapping(MemoryMapping_t* mm, void* va_base, long long offset, size_t nbytes);
BOOL memorySyncMapping(void* addr, size_t nbytes);
BOOL memoryUndoMapping(void* addr, size_t nbytes);

#ifdef	__cplusplus
}
#endif

#endif
