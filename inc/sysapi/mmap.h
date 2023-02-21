//
// Created by hujianzhe
//

#ifndef	UTIL_C_SYSLIB_MMAP_H
#define UTIL_C_SYSLIB_MMAP_H

#include "../platform_define.h"

#if defined(_WIN32) || defined(_WIN64)
	typedef HANDLE	ShareMemMap_t;
#else
	typedef int		ShareMemMap_t;
#endif

#ifdef	__cplusplus
extern "C" {
#endif

__declspec_dll BOOL memoryCreateMapping(ShareMemMap_t* mm, const char* name, size_t nbytes);
__declspec_dll BOOL memoryOpenMapping(ShareMemMap_t* mm, const char* name);
__declspec_dll BOOL memoryCloseMapping(ShareMemMap_t mm);
__declspec_dll BOOL memoryDoMapping(ShareMemMap_t mm, void* va_base, size_t nbytes, void** ret_mptr);
__declspec_dll BOOL memoryUndoMapping(void* mptr);

#ifdef	__cplusplus
}
#endif

#endif
