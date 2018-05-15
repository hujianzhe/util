//
// Created by hujianzhe
//

#include "mmap.h"
#if !defined(_WIN32) && !defined(_WIN64)
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#endif

#ifdef	__cplusplus
extern "C" {
#endif

long mmap_Granularity(void) {
#if defined(_WIN32) || defined(_WIN64)
	SYSTEM_INFO sinfo = {0};
	GetSystemInfo(&sinfo);
	return sinfo.dwAllocationGranularity;
#else
	return sysconf(_SC_PAGESIZE);
#endif
}

BOOL mmap_Create(MemoryMapping_t* mm, FD_t fd, const char* name, size_t nbytes) {
	mm->granularity = mmap_Granularity();
	if (fd != INVALID_FD_HANDLE) {
#if defined(_WIN32) || defined(_WIN64)
		mm->__handle = CreateFileMappingA((HANDLE)fd, NULL, PAGE_READWRITE, 0, 0, NULL);
		return mm->__handle != NULL;
#else
		mm->__fd = fd;
#endif
	}
	else {
#if defined(_WIN32) || defined(_WIN64)
		HANDLE handle = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, ((long long)nbytes) >> 32, nbytes, name);
		if (GetLastError() == ERROR_ALREADY_EXISTS) {
			assert_true(CloseHandle(handle));
			return FALSE;
		}
		mm->__handle = handle;
#else
		int fd = open(name, O_CREAT|O_EXCL|O_TRUNC|O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
		if (-1 == fd) {
			return FALSE;
		}
		if (ftruncate(fd, nbytes)) {
			assert_true(close(fd) == 0);
			return FALSE;
		}
		mm->__fd = fd;
#endif
	}
	return TRUE;
}

BOOL mmap_Open(MemoryMapping_t* mm, const char* name) {
#if defined(_WIN32) || defined(_WIN64)
	mm->__handle = OpenFileMappingA(FILE_MAP_READ | FILE_MAP_WRITE, FALSE, name);
	return mm->__handle != NULL;
#else
	mm->__fd = open(name, O_RDWR);
	return mm->__fd != -1;
#endif
}

BOOL mmap_Close(MemoryMapping_t* mm) {
#if defined(_WIN32) || defined(_WIN64)
	return CloseHandle((HANDLE)(mm->__handle));
#else
	return close(mm->__fd) == 0;
#endif
}

void* mmap_Map(MemoryMapping_t* mm, void* va_base, long long offset, size_t nbytes) {
#if defined(_WIN32) || defined(_WIN64)
	return MapViewOfFileEx((HANDLE)(mm->__handle), FILE_MAP_READ | FILE_MAP_WRITE, offset >> 32, (DWORD)offset, nbytes, va_base);
#else
	return mmap(va_base, nbytes, PROT_READ | PROT_WRITE, MAP_SHARED, mm->__fd, offset);
#endif
}

BOOL mmap_Sync(void* addr, size_t nbytes) {
#if defined(_WIN32) || defined(_WIN64)
	return FlushViewOfFile(addr, nbytes);
#else
	return msync(addr, nbytes, MS_SYNC) == 0;
#endif
}

BOOL mmap_Unmap(void* addr, size_t nbytes) {
#if defined(_WIN32) || defined(_WIN64)
	return UnmapViewOfFile(addr);
#else
	return munmap(addr, nbytes) == 0;
#endif
}

#ifdef	__cplusplus
}
#endif
