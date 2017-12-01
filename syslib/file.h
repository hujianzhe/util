//
// Created by hujianzhe
//

#ifndef	UTIL_SYSLIB_FILE_H_
#define UTIL_SYSLIB_FILE_H_

#include "basicdef.h"

#if defined(_WIN32) || defined(_WIN64)
	#include <conio.h>
	typedef HANDLE					DIRECTORY;
	typedef	WIN32_FIND_DATAA		DIR_ITEM;
	#pragma comment(lib, "ws2_32.lib")
#else
	#include <fcntl.h>
	#include <sys/stat.h>
	#include <dirent.h>
	#include <termios.h>
	typedef	DIR*					DIRECTORY;
	typedef	struct dirent*			DIR_ITEM;
#endif

#define	FILE_READ_BIT			0x01
#define	FILE_WRITE_BIT			0x02
#define	FILE_APPEND_BIT			0x04
#define	FILE_CREAT_BIT			0x08
#define	FILE_EXCL_BIT			0x10
#define	FILE_TRUNC_BIT			0x20
#define	FILE_DSYNC_BIT			0x40
#define	FILE_ASYNC_BIT			0x80

typedef enum {
	FD_HADNLE_UNKNOWN,
	FD_HANDLE_DIRECTORY,
	FD_HANDLE_REGULAR,
	FD_HANDLE_SYMLINK,
	FD_HANDLE_SOCKET,
	FD_HANDLE_CHAR,
	FD_HANDLE_PIPE
} FD_HANDLE_TYPE;

#ifdef	__cplusplus
extern "C" {
#endif

/* terminate */
int t_Kbhit(void);
int t_Getch(void);
/* FD_HANDLE generate operator */
EXEC_RETURN fd_Type(FD_HANDLE fd, FD_HANDLE_TYPE* type);
EXEC_RETURN fd_GetInheritFlag(FD_HANDLE fd, BOOL* bool_val);
EXEC_RETURN fd_SetInheritFlag(FD_HANDLE fd, BOOL bool_val);
FD_HANDLE fd_Dup(FD_HANDLE fd);
FD_HANDLE fd_Dup2(FD_HANDLE oldfd, FD_HANDLE newfd);
/* file operator */
FD_HANDLE file_Open(const char* path, int obit);
int file_Read(FD_HANDLE fd, void* buf, unsigned int nbytes);
int file_Write(FD_HANDLE fd, const void* buf, unsigned int nbytes);
long long file_Seek(FD_HANDLE fd, long long offset, int whence);
long long file_Tell(FD_HANDLE fd);
EXEC_RETURN file_Flush(FD_HANDLE fd);
EXEC_RETURN file_Close(FD_HANDLE fd);
long long file_Size(FD_HANDLE fd);
EXEC_RETURN file_ChangeLength(FD_HANDLE fd, long long length);
/* file lock */
EXEC_RETURN file_LockExclusive(FD_HANDLE fd, long long offset, long long nbytes, BOOL block_bool);
EXEC_RETURN file_LockShared(FD_HANDLE fd, long long offset, long long nbytes, BOOL block_bool);
EXEC_RETURN file_Unlock(FD_HANDLE fd, long long offset, long long nbytes);
/* file name */
const char* file_ExtName(const char* path);
const char* file_FileName(const char* path);
/* file link */
EXEC_RETURN file_CreateSymlink(const char* actualpath, const char* sympath);
EXEC_RETURN file_CreateHardLink(const char* existpath, const char* newpath);
EXEC_RETURN file_HardLinkCount(FD_HANDLE fd, unsigned int* count);
EXEC_RETURN file_DeleteHardLink(const char* existpath);
/* directory  operator */
EXEC_RETURN dir_Create(const char* path);
EXEC_RETURN dir_CurrentPath(char* buf, size_t n);
EXEC_RETURN dir_Sheft(const char* path);
DIRECTORY dir_Open(const char* path);
EXEC_RETURN dir_Close(DIRECTORY dir);
EXEC_RETURN dir_Read(DIRECTORY dir, DIR_ITEM* item);
char* dir_FileName(DIR_ITEM* item);

#ifdef	__cplusplus
}
#endif

#endif
