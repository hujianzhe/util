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

__declspec_dll long memoryPageSize(void);
__declspec_dll unsigned long long memorySize(void);

__declspec_dll BOOL memoryCreateFileMapping(MemoryMapping_t* mm, FD_t fd);
__declspec_dll BOOL memoryCreateMapping(MemoryMapping_t* mm, const char* name, size_t nbytes);
__declspec_dll BOOL memoryOpenMapping(MemoryMapping_t* mm, const char* name);
__declspec_dll BOOL memoryCloseMapping(MemoryMapping_t* mm);
__declspec_dll void* memoryDoMapping(MemoryMapping_t* mm, void* va_base, long long offset, size_t nbytes);
__declspec_dll BOOL memorySyncMapping(void* addr, size_t nbytes);
__declspec_dll BOOL memoryUndoMapping(void* addr);
//__declspec_dll BOOL memoryUnlinkMapping(const char* name);

#ifdef	__cplusplus
}
#endif

#endif
