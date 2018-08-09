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

enum {
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
BOOL fd_Type(FD_t fd, int* type);
BOOL fd_GetInheritFlag(FD_t fd, BOOL* bool_val);
BOOL fd_SetInheritFlag(FD_t fd, BOOL bool_val);
FD_t fd_Dup(FD_t fd);
FD_t fd_Dup2(FD_t oldfd, FD_t newfd);
/* file operator */
FD_t fd_Open(const char* path, int obit);
int fd_Read(FD_t fd, void* buf, unsigned int nbytes);
int fd_Write(FD_t fd, const void* buf, unsigned int nbytes);
long long fd_Seek(FD_t fd, long long offset, int whence);
long long fd_Tell(FD_t fd);
BOOL fd_Flush(FD_t fd);
BOOL fd_Close(FD_t fd);
long long fd_GetSize(FD_t fd);
BOOL fd_SetLength(FD_t fd, long long length);
/* file lock */
BOOL file_LockExclusive(FD_t fd, long long offset, long long nbytes, BOOL block_bool);
BOOL file_LockShared(FD_t fd, long long offset, long long nbytes, BOOL block_bool);
BOOL file_Unlock(FD_t fd, long long offset, long long nbytes);
/* file name */
const char* file_ExtName(const char* path);
const char* file_FileName(const char* path);
/* file link */
BOOL file_CreateSymlink(const char* actualpath, const char* sympath);
BOOL file_CreateHardLink(const char* existpath, const char* newpath);
BOOL file_HardLinkCount(FD_t fd, unsigned int* count);
BOOL file_DeleteHardLink(const char* existpath);
/* directory  operator */
BOOL dir_Create(const char* path);
BOOL dir_CurrentPath(char* buf, size_t n);
BOOL dir_Sheft(const char* path);
Dir_t dir_Open(const char* path);
BOOL dir_Close(Dir_t dir);
BOOL dir_Read(Dir_t dir, DirItem_t* item);
char* dir_FileName(DirItem_t* item);

#ifdef	__cplusplus
}
#endif

#endif
