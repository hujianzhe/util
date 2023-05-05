//
// Created by hujianzhe
//

#include "../../inc/sysapi/mmap.h"
#if !defined(_WIN32) && !defined(_WIN64)
#include <errno.h>
#include <fcntl.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
/*#include <sys/sysctl.h>*/
#endif

#ifdef	__cplusplus
extern "C" {
#endif

BOOL memoryCreateMapping(ShareMemMap_t* mm, const char* name, size_t nbytes) {
/* note: if already exist, size is lesser or equal than the size of that segment */
#if defined(_WIN32) || defined(_WIN64)
	HANDLE handle = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, ((long long)nbytes) >> 32, nbytes, name);
	if (!handle) {
		return FALSE;
	}
	/*
	if (GetLastError() == ERROR_ALREADY_EXISTS) {
		CloseHandle(handle);
		return FALSE;
	}
	*/
	*mm = handle;
	return TRUE;
#else
	key_t k = ftok(name, 0);
	if ((key_t)-1 == k) {
		return FALSE;
	}
	*mm = shmget(k, nbytes, 0666 | IPC_CREAT);
	return (*mm) != -1;
#endif
}

BOOL memoryOpenMapping(ShareMemMap_t* mm, const char* name) {
#if defined(_WIN32) || defined(_WIN64)
	*mm = OpenFileMappingA(FILE_MAP_READ | FILE_MAP_WRITE, FALSE, name);
	return (*mm) != NULL;
#else
	key_t k = ftok(name, 0);
	if ((key_t)-1 == k) {
		return FALSE;
	}
	*mm = shmget(k, 0, 0666);
	return (*mm) != -1;
#endif
}

BOOL memoryCloseMapping(ShareMemMap_t mm) {
#if defined(_WIN32) || defined(_WIN64)
	return CloseHandle(mm);
#else
	return shmctl(mm, IPC_RMID, NULL) == 0;
#endif
}

BOOL memoryDoMapping(ShareMemMap_t mm, void* va_base, size_t nbytes, void** ret_mptr) {
#if defined(_WIN32) || defined(_WIN64)
	void* addr = MapViewOfFileEx(mm, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, nbytes, va_base);
	if (!addr) {
		return FALSE;
	}
	*ret_mptr = addr;
	return TRUE;
#else
	void* addr = shmat(mm, va_base, 0);
	if ((void*)-1 == addr) {
		return FALSE;
	}
	*ret_mptr = addr;
	return TRUE;
#endif
}

BOOL memoryUndoMapping(void* mptr) {
#if defined(_WIN32) || defined(_WIN64)
	return UnmapViewOfFile(mptr);
#else
	return shmdt(mptr) == 0;
#endif
}

#ifdef	__cplusplus
}
#endif
