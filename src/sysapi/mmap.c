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

BOOL memoryCreateMapping(ShareMemMap_t* mm, const char* name, size_t nbytes, int prot_bits) {
/* note: if already exist, size is lesser or equal than the size of that segment */
#if defined(_WIN32) || defined(_WIN64)
	HANDLE handle;
	DWORD flProtect = 0;
	mm->prot_bits = FILE_MAP_READ | FILE_MAP_WRITE;
	if (prot_bits & MMAP_PROT_EXECUTE_BIT) {
#ifdef PAGE_EXECUTE_READWRITE
		flProtect = PAGE_EXECUTE_READWRITE;
		mm->prot_bits |= FILE_MAP_EXECUTE;
#endif
	}
	else {
		flProtect = PAGE_READWRITE;
	}
	handle = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, flProtect, ((UINT64)nbytes) >> 32, nbytes, name);
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
	mm->prot_bits = PROT_READ | PROT_WRITE;
	if (prot_bits & MMAP_PROT_EXECUTE_BIT) {
		mm->prot_bits |= PROT_EXEC;
	}
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

BOOL memoryOpenMapping(ShareMemMap_t* mm, const char* name, int prot_bits) {
#if defined(_WIN32) || defined(_WIN64)
	HANDLE handle;
	DWORD dwDesiredAccess = 0;
	if (prot_bits & MMAP_PROT_READ_BIT) {
		dwDesiredAccess |= FILE_MAP_READ;
	}
	if (prot_bits & MMAP_PROT_WRITE_BIT) {
		dwDesiredAccess |= FILE_MAP_WRITE;
	}
	if (prot_bits & MMAP_PROT_EXECUTE_BIT) {
		dwDesiredAccess |= FILE_MAP_EXECUTE;
	}
	handle = OpenFileMappingA(dwDesiredAccess, FALSE, name);
	if (!handle) {
		return FALSE;
	}
	mm->handle = handle;
	mm->addr = NULL;
	mm->prot_bits = dwDesiredAccess;
	return TRUE;
#else
	struct stat f_stat;
	int fd, oflag = 0;
	mm->prot_bits = 0;
	if (prot_bits & MMAP_PROT_WRITE_BIT) {
		oflag = O_RDWR;
		mm->prot_bits |= (PROT_READ | PROT_WRITE);
	}
	else if (prot_bits & MMAP_PROT_READ_BIT) {
		oflag = O_RDONLY;
		mm->prot_bits |= PROT_READ;
	}
	if (prot_bits & MMAP_PROT_EXECUTE_BIT) {
		mm->prot_bits |= PROT_EXEC;
	}
	fd = shm_open(name, oflag, 0666);
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
	void* addr = MapViewOfFileEx(mm->handle, mm->prot_bits, 0, 0, 0, va_base);
	if (!addr) {
		return FALSE;
	}
	mm->addr = addr;
	*ret_mptr = addr;
	return TRUE;
#else
	void* addr = mmap(NULL, mm->nbytes, mm->prot_bits, MAP_SHARED, mm->fd, 0);
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
