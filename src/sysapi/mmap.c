//
// Created by hujianzhe
//

#include "../../inc/sysapi/mmap.h"
#if !defined(_WIN32) && !defined(_WIN64)
#include <errno.h>
#include <fcntl.h>
#include <sys/ipc.h>
/*#include <sys/shm.h>*/
#include <sys/mman.h>
#include <sys/stat.h>
/*#include <sys/sysctl.h>*/
#endif

#ifdef	__cplusplus
extern "C" {
#endif

BOOL memoryCreateMapping(ShareMemMap_t* mm, const char* name, size_t nbytes) {
/* note: if already exist, size is lesser or equal than the size of that segment */
#if defined(_WIN32) || defined(_WIN64)
	HANDLE handle = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, ((UINT64)nbytes) >> 32, nbytes, name);
	if (!handle) {
		return FALSE;
	}
	/*
	if (GetLastError() == ERROR_ALREADY_EXISTS) {
		CloseHandle(handle);
		return FALSE;
	}
	*/
	mm->handle = handle;
	mm->addr = NULL;
	return TRUE;
#else
	int fd = shm_open(name, O_CREAT | O_RDWR, 0666);
	if (-1 == fd) {
		return FALSE;
	}
	if (ftruncate(fd, nbytes)) {
		close(fd);
		return FALSE;
	}
	mm->fd = fd;
	mm->addr = NULL;
	mm->nbytes = nbytes;
	return TRUE;
	/*
	key_t k = ftok(name, 0);
	if ((key_t)-1 == k) {
		return FALSE;
	}
	*mm = shmget(k, nbytes, 0666 | IPC_CREAT);
	return (*mm) != -1;
	*/
#endif
}

BOOL memoryOpenMapping(ShareMemMap_t* mm, const char* name) {
#if defined(_WIN32) || defined(_WIN64)
	HANDLE handle = OpenFileMappingA(FILE_MAP_READ | FILE_MAP_WRITE, FALSE, name);
	if (!handle) {
		return FALSE;
	}
	mm->handle = handle;
	mm->addr = NULL;
	return TRUE;
#else
	struct stat f_stat;
	int fd = shm_open(name, O_RDWR, 0666);
	if (-1 == fd) {
		return FALSE;
	}
	if (fstat(fd, &f_stat)) {
		close(fd);
		return FALSE;
	}
	mm->fd = fd;
	mm->addr = NULL;
	mm->nbytes = f_stat.st_size;
	return TRUE;
	/*
	key_t k = ftok(name, 0);
	if ((key_t)-1 == k) {
		return FALSE;
	}
	*mm = shmget(k, 0, 0666);
	return (*mm) != -1;
	*/
#endif
}

BOOL memoryCloseMapping(ShareMemMap_t* mm) {
#if defined(_WIN32) || defined(_WIN64)
	return CloseHandle(mm->handle);
#else
	return close(mm->fd) == 0;
	/*
	return shmctl(mm, IPC_RMID, NULL) == 0;
	*/
#endif
}

BOOL memoryDoMapping(ShareMemMap_t* mm, void* va_base, void** ret_mptr) {
#if defined(_WIN32) || defined(_WIN64)
	void* addr = MapViewOfFileEx(mm->handle, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, 0, va_base);
	if (!addr) {
		return FALSE;
	}
	mm->addr = addr;
	*ret_mptr = addr;
	return TRUE;
#else
	void* addr = mmap(NULL, mm->nbytes, PROT_READ | PROT_WRITE, MAP_SHARED, mm->fd, 0);
	if (MAP_FAILED == addr) {
		return FALSE;
	}
	mm->addr = addr;
	*ret_mptr = addr;
	return TRUE;
	/*
	void* addr = shmat(mm, va_base, 0);
	if ((void*)-1 == addr) {
		return FALSE;
	}
	*ret_mptr = addr;
	return TRUE;
	*/
#endif
}

BOOL memoryUndoMapping(ShareMemMap_t* mm) {
#if defined(_WIN32) || defined(_WIN64)
	return UnmapViewOfFile(mm->addr);
#else
	return munmap(mm->addr, mm->nbytes) == 0;
	/*
	return shmdt(mptr) == 0;
	*/
#endif
}

#ifdef	__cplusplus
}
#endif
