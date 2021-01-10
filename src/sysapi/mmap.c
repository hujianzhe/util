//
// Created by hujianzhe
//

#include "../../inc/sysapi/mmap.h"
#include "../../inc/sysapi/assert.h"
#if !defined(_WIN32) && !defined(_WIN64)
#include <errno.h>
#include <fcntl.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/sysctl.h>
#endif

#ifdef	__cplusplus
extern "C" {
#endif

long memoryPageSize(void) {
#if defined(_WIN32) || defined(_WIN64)
	SYSTEM_INFO si;
	GetSystemInfo(&si);
	return si.dwPageSize;
#else
	return sysconf(_SC_PAGESIZE);
#endif
}

unsigned long long memorySize(void) {
#if defined(_WIN32) || defined(_WIN64)
	MEMORYSTATUSEX statex = { 0 };
	statex.dwLength = sizeof(statex);
	if (GlobalMemoryStatusEx(&statex)) {
		return statex.ullTotalPhys;
		//*avail = statex.ullAvailPhys;
		//return TRUE;
	}
	return 0;
	//return FALSE;
#elif __linux__
	unsigned long page_size, total_page;
	if ((page_size = sysconf(_SC_PAGESIZE)) == -1)
		return 0;
	if ((total_page = sysconf(_SC_PHYS_PAGES)) == -1)
		return 0;
	//if((free_page = sysconf(_SC_AVPHYS_PAGES)) == -1)
	//return FALSE;
	return (unsigned long long)total_page * (unsigned long long)page_size;
	//*avail = (unsigned long long)free_page * (unsigned long long)page_size;
	//return TRUE;
#elif __APPLE__
	int64_t value;
	size_t len = sizeof(value);
	return sysctlbyname("hw.memsize", &value, &len, NULL, 0) != -1 ? value : 0;
	//*avail = 0;// sorry...
	//return TRUE;
#endif
}

BOOL memoryCreateFileMapping(MemoryMapping_t* mm, FD_t fd) {
#if defined(_WIN32) || defined(_WIN64)
	mm->__handle = CreateFileMappingA((HANDLE)fd, NULL, PAGE_READWRITE, 0, 0, NULL);
	return mm->__handle != NULL;
#else
	mm->__isref = 1;
	mm->__fd = fd;
	return TRUE;
#endif
}

BOOL memoryCreateMapping(MemoryMapping_t* mm, const char* name, size_t nbytes) {
#if defined(_WIN32) || defined(_WIN64)
	HANDLE handle = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, ((long long)nbytes) >> 32, nbytes, name);
	if (GetLastError() == ERROR_ALREADY_EXISTS) {
		assertTRUE(CloseHandle(handle));
		return FALSE;
	}
	mm->__handle = handle;
	return TRUE;
#else
	/*
	int fd = open(name, O_CREAT|O_EXCL|O_TRUNC|O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
	if (-1 == fd) {
		return FALSE;
	}
	if (ftruncate(fd, nbytes)) {
		assertTRUE(close(fd) == 0);
		return FALSE;
	}
	mm->__isref = 0;
	mm->__fd = fd;
	*/
	key_t k = ftok(name, 0);
	if ((key_t)-1 == k) {
		return FALSE;
	}
	mm->__isref = 1;
	mm->__fd = shmget(k, nbytes, 0666 | IPC_CREAT);
	return -1 != mm->__fd;
#endif
}

BOOL memoryOpenMapping(MemoryMapping_t* mm, const char* name) {
#if defined(_WIN32) || defined(_WIN64)
	mm->__handle = OpenFileMappingA(FILE_MAP_READ | FILE_MAP_WRITE, FALSE, name);
	return mm->__handle != NULL;
#else
	/*
	mm->__isref = 0;
	mm->__fd = open(name, O_RDWR);
	return mm->__fd != -1;
	*/
	key_t k = ftok(name, 0);
	if ((key_t)-1 == k) {
		return FALSE;
	}
	mm->__isref = 1;
	mm->__fd = shmget(k, 0, 0666);
	return mm->__fd != -1;
#endif
}

BOOL memoryCloseMapping(MemoryMapping_t* mm) {
#if defined(_WIN32) || defined(_WIN64)
	return CloseHandle(mm->__handle);
#else
	return mm->__isref || close(mm->__fd) == 0;
#endif
}

void* memoryDoMapping(MemoryMapping_t* mm, void* va_base, long long offset, size_t nbytes) {
#if defined(_WIN32) || defined(_WIN64)
	return MapViewOfFileEx(mm->__handle, FILE_MAP_READ | FILE_MAP_WRITE, offset >> 32, (DWORD)offset, nbytes, va_base);
#else
	return shmat(mm->__fd, NULL, 0);
	//return mmap(va_base, nbytes, PROT_READ | PROT_WRITE, MAP_SHARED, mm->__fd, offset);
#endif
}

BOOL memorySyncMapping(void* addr, size_t nbytes) {
#if defined(_WIN32) || defined(_WIN64)
	return FlushViewOfFile(addr, nbytes);
#else
	return msync(addr, nbytes, MS_SYNC) == 0;
#endif
}

BOOL memoryUndoMapping(void* addr) {
#if defined(_WIN32) || defined(_WIN64)
	return UnmapViewOfFile(addr);
#else
	return shmdt(addr) == 0;
	//return munmap(addr, nbytes) == 0;
#endif
}

/*
BOOL memoryUnlinkMapping(const char* name) {
#if defined(_WIN32) || defined(_WIN64)
	return TRUE;
#else
	return unlink(name) == 0 || ENOENT == errno;
#endif
}
*/

#ifdef	__cplusplus
}
#endif
