//
// Created by hujianzhe
//

#ifndef	UTIL_C_SYSLIB_FILE_H
#define UTIL_C_SYSLIB_FILE_H

#include "../platform_define.h"

#if defined(_WIN32) || defined(_WIN64)
	typedef HANDLE					Dir_t;
	typedef	WIN32_FIND_DATAA		DirItem_t;
	#pragma comment(lib, "ws2_32.lib")
#else
	#include <fcntl.h>
	#include <sys/stat.h>
	#include <dirent.h>
	typedef	DIR*					Dir_t;
	typedef	struct dirent*			DirItem_t;
#endif

enum {
	FILE_READ_BIT = 0x01,
	FILE_WRITE_BIT = 0x02,
	FILE_APPEND_BIT	= 0x04,
	FILE_CREAT_BIT = 0x08,
	FILE_EXCL_BIT = 0x10,
	FILE_TRUNC_BIT = 0x20,
	FILE_DSYNC_BIT = 0x40,
	FILE_ASYNC_BIT = 0x80
};

enum FDtype_t {
	FD_TYPE_ERROR,
	FD_TYPE_UNKNOWN,
	FD_TYPE_DIRECTORY,
	FD_TYPE_REGULAR,
	FD_TYPE_SYMLINK,
	FD_TYPE_SOCKET,
	FD_TYPE_CHAR,
	FD_TYPE_PIPE
};

#ifdef	__cplusplus
extern "C" {
#endif

/* FD_t generate operator */
UTIL_LIBAPI enum FDtype_t fdType(FD_t fd);
UTIL_LIBAPI BOOL fdGetInheritFlag(FD_t fd, BOOL* bool_val);
UTIL_LIBAPI BOOL fdSetInheritFlag(FD_t fd, BOOL bool_val);
UTIL_LIBAPI FD_t fdDup(FD_t fd);
UTIL_LIBAPI FD_t fdDup2(FD_t oldfd, FD_t newfd);
/* file operator */
UTIL_LIBAPI FD_t fdOpen(const char* path, int obit);
UTIL_LIBAPI int fdRead(FD_t fd, void* buf, unsigned int nbytes);
UTIL_LIBAPI int fdWrite(FD_t fd, const void* buf, unsigned int nbytes);
UTIL_LIBAPI long long fdSeek(FD_t fd, long long offset, int whence);
UTIL_LIBAPI long long fdTell(FD_t fd);
UTIL_LIBAPI BOOL fdFlush(FD_t fd);
UTIL_LIBAPI BOOL fdClose(FD_t fd);
UTIL_LIBAPI long long fdGetSize(FD_t fd);
UTIL_LIBAPI BOOL fdSetLength(FD_t fd, long long length);
/* file lock */
UTIL_LIBAPI BOOL fileLockExclusive(FD_t fd, long long offset, long long nbytes, BOOL block_bool);
UTIL_LIBAPI BOOL fileLockShared(FD_t fd, long long offset, long long nbytes, BOOL block_bool);
UTIL_LIBAPI BOOL fileUnlock(FD_t fd, long long offset, long long nbytes);
/* file name */
UTIL_LIBAPI const char* fileExtName(const char* path);
UTIL_LIBAPI const char* fileFileName(const char* path);
/* file link */
UTIL_LIBAPI BOOL fileCreateSymlink(const char* actualpath, const char* sympath);
UTIL_LIBAPI BOOL fileCreateHardLink(const char* existpath, const char* newpath);
UTIL_LIBAPI BOOL fileHardLinkCount(FD_t fd, unsigned int* count);
UTIL_LIBAPI BOOL fileDeleteHardLink(const char* existpath);
/* directory  operator */
UTIL_LIBAPI BOOL dirCreate(const char* path);
UTIL_LIBAPI BOOL dirCurrentPath(char* buf, size_t n);
UTIL_LIBAPI BOOL dirSheftPath(const char* path);
UTIL_LIBAPI Dir_t dirOpen(const char* path);
UTIL_LIBAPI BOOL dirClose(Dir_t dir);
UTIL_LIBAPI BOOL dirRead(Dir_t dir, DirItem_t* item);
UTIL_LIBAPI char* dirFileName(DirItem_t* item);

#ifdef	__cplusplus
}
#endif

#endif
