//
// Created by hujianzhe
//

#ifndef	UTIL_C_SYSLIB_MMAP_H
#define UTIL_C_SYSLIB_MMAP_H

#include "../platform_define.h"

typedef struct ShareMemMap_t {
#if defined(_WIN32) || defined(_WIN64)
	DWORD prot_bits;
	HANDLE handle;
#else
	int prot_bits;
	int fd;
	size_t nbytes;
#endif
	void* addr;
} ShareMemMap_t;

enum {
	MMAP_PROT_READ_BIT = 0x01,
	MMAP_PROT_WRITE_BIT = 0x02,
	MMAP_PROT_EXECUTE_BIT = 0x04
};

#ifdef	__cplusplus
extern "C" {
#endif

__declspec_dll BOOL memoryCreateMapping(ShareMemMap_t* mm, const char* name, size_t nbytes, int prot_bits);
__declspec_dll BOOL memoryOpenMapping(ShareMemMap_t* mm, const char* name, int prot_bits);
__declspec_dll BOOL memoryCloseMapping(ShareMemMap_t* mm);
__declspec_dll BOOL memoryDoMapping(ShareMemMap_t* mm, void* va_base, void** ret_mptr);
__declspec_dll BOOL memoryUndoMapping(ShareMemMap_t* mm);

#ifdef	__cplusplus
}
#endif

#endif
