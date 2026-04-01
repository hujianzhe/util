//
// Created by hujianzhe
//

#ifndef	UTIL_C_SYSLIB_MMAP_H
#define UTIL_C_SYSLIB_MMAP_H

#include "../platform_define.h"

typedef struct ShareMemMap_t {
#if defined(_WIN32) || defined(_WIN64)
	HANDLE handle;
#else
	int fd;
	size_t nbytes;
#endif
	void* addr;
} ShareMemMap_t;

#ifdef	__cplusplus
extern "C" {
#endif

__declspec_dll BOOL memoryCreateMapping(ShareMemMap_t* mm, const char* name, size_t nbytes);
__declspec_dll BOOL memoryOpenMapping(ShareMemMap_t* mm, const char* name);
__declspec_dll BOOL memoryCloseMapping(ShareMemMap_t* mm);
__declspec_dll BOOL memoryDoMapping(ShareMemMap_t* mm, void* va_base, void** ret_mptr);
__declspec_dll BOOL memoryUndoMapping(ShareMemMap_t* mm);

#ifdef	__cplusplus
}
#endif

#endif
