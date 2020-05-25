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
	FILE_ASYNC_BIT = 0x80,
	FILE_TEMP_BIT = 0x100
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
__declspec_dll enum FDtype_t fdType(FD_t fd);
__declspec_dll BOOL fdGetInheritFlag(FD_t fd, BOOL* bool_val);
__declspec_dll BOOL fdSetInheritFlag(FD_t fd, BOOL bool_val);
__declspec_dll FD_t fdDup(FD_t fd);
__declspec_dll FD_t fdDup2(FD_t oldfd, FD_t newfd);
/* file operator */
__declspec_dll FD_t fdOpen(const char* path, int obit);
__declspec_dll int fdRead(FD_t fd, void* buf, unsigned int nbytes);
__declspec_dll int fdWrite(FD_t fd, const void* buf, unsigned int nbytes);
__declspec_dll long long fdSeek(FD_t fd, long long offset, int whence);
__declspec_dll long long fdTell(FD_t fd);
__declspec_dll BOOL fdFlush(FD_t fd);
__declspec_dll BOOL fdClose(FD_t fd);
__declspec_dll long long fdGetSize(FD_t fd);
__declspec_dll BOOL fdSetLength(FD_t fd, long long length);
/* file lock */
__declspec_dll BOOL fileLockExclusive(FD_t fd, long long offset, long long nbytes, BOOL block_bool);
__declspec_dll BOOL fileLockShared(FD_t fd, long long offset, long long nbytes, BOOL block_bool);
__declspec_dll BOOL fileUnlock(FD_t fd, long long offset, long long nbytes);
/* file name */
__declspec_dll BOOL fileIsExist(const char* path);
__declspec_dll const char* fileExtName(const char* path);
__declspec_dll const char* fileFileName(const char* path);
__declspec_dll char* fileReadAllData(const char* path, long long* ptr_file_sz);
__declspec_dll int fileWriteCoverData(const char* path, const void* data, unsigned int len);
/* file link */
__declspec_dll BOOL fileCreateSymlink(const char* actualpath, const char* sympath);
__declspec_dll BOOL fileCreateHardLink(const char* existpath, const char* newpath);
__declspec_dll BOOL fileHardLinkCount(FD_t fd, unsigned int* count);
__declspec_dll BOOL fileDeleteHardLink(const char* existpath);
/* directory  operator */
__declspec_dll BOOL dirCreate(const char* path);
__declspec_dll BOOL dirCurrentPath(char* buf, size_t n);
__declspec_dll BOOL dirSheftPath(const char* path);
__declspec_dll Dir_t dirOpen(const char* path);
__declspec_dll BOOL dirClose(Dir_t dir);
__declspec_dll BOOL dirRead(Dir_t dir, DirItem_t* item);
__declspec_dll char* dirFileName(DirItem_t* item);

#ifdef	__cplusplus
}
#endif

#endif
