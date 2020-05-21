//
// Created by hujianzhe
//

#include "../../inc/sysapi/file.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef	__cplusplus
extern "C" {
#endif

#if	defined(_WIN32) || defined(_WIN64)
static char* __win32_path(const char* path) {
	char* p = strdup(path);
	if (p) {
		char* pa;
		for (pa = p; *pa; ++pa) {
			if ('/' == *pa)
				*pa = '\\';
		}
	}
	return p;
}
#endif

/* FD_t generate operator */
enum FDtype_t fdType(FD_t fd) {
#if defined(_WIN32) || defined(_WIN64)
	DWORD mode = GetFileType((HANDLE)fd);
	switch (mode) {
		case FILE_TYPE_REMOTE:
		case FILE_TYPE_UNKNOWN:
			return GetLastError() != NO_ERROR ? FD_TYPE_ERROR : FD_TYPE_UNKNOWN;

		case FILE_TYPE_PIPE:
			if (GetNamedPipeInfo((HANDLE)fd, NULL, NULL, NULL, NULL))
				return FD_TYPE_PIPE;
			else {
				int val;
				int len = sizeof(val);
				if (getsockopt(fd, SOL_SOCKET, SO_TYPE, (char*)&val, &len)) {
					return FD_TYPE_ERROR;
				}
				return FD_TYPE_SOCKET;
			}

		case FILE_TYPE_CHAR:
			return FD_TYPE_CHAR;

		case FILE_TYPE_DISK: {
			BY_HANDLE_FILE_INFORMATION info;
			if (!GetFileInformationByHandle((HANDLE)fd, &info))
				return FD_TYPE_ERROR;
			else if (info.dwFileAttributes & FILE_ATTRIBUTE_ARCHIVE)
				return FD_TYPE_REGULAR;
			else if (info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				return FD_TYPE_DIRECTORY;
			else if (info.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)
				return FD_TYPE_SYMLINK;
			else
				return FD_TYPE_UNKNOWN;
		}

		default:
			return FD_TYPE_UNKNOWN;
	}
#else
	struct stat f_stat;
	if (fstat(fd, &f_stat) == 0) {
		mode_t mode = f_stat.st_mode;
		if (S_ISDIR(mode))
			return FD_TYPE_DIRECTORY;
		else if (S_ISREG(mode))
			return FD_TYPE_REGULAR;
		else if (S_ISLNK(mode))
			return FD_TYPE_SYMLINK;
		else if (S_ISCHR(mode))
			return FD_TYPE_CHAR;
		else if (S_ISSOCK(mode))
			return FD_TYPE_SOCKET;
		else if (S_ISFIFO(mode))
			return FD_TYPE_PIPE;
		else
			return FD_TYPE_UNKNOWN;
	}
	return FD_TYPE_ERROR;
#endif
}

BOOL fdGetInheritFlag(FD_t fd, BOOL* bool_val) {
#if defined(_WIN32) || defined(_WIN64)
	DWORD flag;
	if (!GetHandleInformation((HANDLE)fd, &flag)) {
		return FALSE;
	}
	*bool_val = ((flag & HANDLE_FLAG_INHERIT) != 0);
	return TRUE;
#else
	int res = fcntl(fd,F_GETFD);
	if (res < 0) {
		return FALSE;
	}
	*bool_val = (0 == res);
	return TRUE;
#endif
}

BOOL fdSetInheritFlag(FD_t fd, BOOL bool_val) {
#if defined(_WIN32) || defined(_WIN64)
	return SetHandleInformation((HANDLE)fd, HANDLE_FLAG_INHERIT, bool_val ? HANDLE_FLAG_INHERIT : 0);
#else
	return fcntl(fd, F_SETFD, bool_val == 0) == 0;
#endif
}

FD_t fdDup(FD_t fd) {
#if defined(_WIN32) || defined(_WIN64)
	HANDLE new_fd = INVALID_HANDLE_VALUE;
	DuplicateHandle(GetCurrentProcess(), (HANDLE)fd, GetCurrentProcess(), &new_fd, 0, FALSE, DUPLICATE_SAME_ACCESS);
	return (FD_t)new_fd;
#else
	return dup(fd);
#endif
}

FD_t fdDup2(FD_t oldfd, FD_t newfd) {
#if defined(_WIN32) || defined(_WIN64)
	if (oldfd != newfd) {
		HANDLE new_fd = (HANDLE)newfd;
		if (!DuplicateHandle(GetCurrentProcess(), (HANDLE)oldfd, GetCurrentProcess(), (LPHANDLE)&newfd, 0, FALSE, DUPLICATE_SAME_ACCESS)) {
			return (FD_t)INVALID_HANDLE_VALUE;
		}
		CloseHandle(new_fd);
	}
	return newfd;
#else
	return dup2(oldfd, newfd);
#endif
}

/* file operator */
FD_t fdOpen(const char* path, int obit) {
#if defined(_WIN32) || defined(_WIN64)
	char* szFullPath;
	HANDLE fd;
	DWORD dwDesiredAccess = 0, dwCreationDisposition = 0, dwFlagsAndAttributes = 0;
	SECURITY_ATTRIBUTES sa = { 0 };
	//
	sa.nLength = sizeof(sa);
	sa.bInheritHandle = TRUE;
	//
	if (obit & FILE_READ_BIT)
		dwDesiredAccess |= FILE_GENERIC_READ;
	if (obit & FILE_WRITE_BIT) {
		if (obit & FILE_APPEND_BIT)
			dwDesiredAccess |=	STANDARD_RIGHTS_WRITE |
								FILE_WRITE_ATTRIBUTES |
								FILE_WRITE_EA |
								FILE_APPEND_DATA |
								SYNCHRONIZE;
		else
			dwDesiredAccess |= FILE_GENERIC_WRITE;
	}
	//
	if (obit & FILE_CREAT_BIT) {
		if (obit & FILE_EXCL_BIT)
			dwCreationDisposition = CREATE_NEW;
		else if (obit & FILE_TRUNC_BIT)
			dwCreationDisposition = CREATE_ALWAYS;
		else
			dwCreationDisposition = OPEN_ALWAYS;
	}
	else if (obit & FILE_TRUNC_BIT)
		dwCreationDisposition = TRUNCATE_EXISTING;
	else
		dwCreationDisposition = OPEN_EXISTING;
	//
	if (obit & FILE_TEMP_BIT)
		dwFlagsAndAttributes |= FILE_FLAG_DELETE_ON_CLOSE | FILE_ATTRIBUTE_TEMPORARY;
	else {
		dwFlagsAndAttributes |= FILE_ATTRIBUTE_NORMAL;
		if (obit & FILE_DSYNC_BIT)
			dwFlagsAndAttributes |= FILE_FLAG_WRITE_THROUGH;
		if (obit & FILE_ASYNC_BIT)
			dwFlagsAndAttributes |= FILE_FLAG_OVERLAPPED;
	}
	//
	szFullPath = __win32_path(path);
	if (!szFullPath)
		return (FD_t)INVALID_HANDLE_VALUE;
	fd = CreateFileA(szFullPath, dwDesiredAccess,
		FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
		&sa, dwCreationDisposition, dwFlagsAndAttributes, NULL);
	free(szFullPath);
	return (FD_t)fd;
#else
	mode_t mode = 0;
	int oflag = 0;
	if ((obit & FILE_READ_BIT) && (obit & FILE_WRITE_BIT)) {
		oflag |= O_RDWR;
		if (obit & FILE_APPEND_BIT)
			oflag |= O_APPEND;
	}
	else if (obit & FILE_READ_BIT)
		oflag |= O_RDONLY;
	else if (obit & FILE_WRITE_BIT) {
		oflag |= O_WRONLY;
		if (obit & FILE_APPEND_BIT)
			oflag |= O_APPEND;
	}
	if (obit & FILE_CREAT_BIT) {
		oflag |= O_CREAT;
		mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH;
	}
	if (obit & FILE_EXCL_BIT)
		oflag |= O_EXCL;
	if (obit & FILE_TRUNC_BIT)
		oflag |= O_TRUNC;
	if (obit & FILE_DSYNC_BIT)
		oflag |= O_DSYNC;
#if defined(__FreeBSD__) || defined(__APPLE__)
	if (obit & FILE_ASYNC_BIT)
		oflag |= O_ASYNC;
#endif
	if (obit & FILE_TEMP_BIT) {
		int fd;
		size_t pathlen = strlen(path);
		char* new_path = (char*)malloc(pathlen + 7);
		if (!new_path)
			return -1;
		memcpy(new_path, path, pathlen);
		memcpy(new_path + pathlen, "XXXXXX", 6);
		new_path[pathlen + 6] = 0;
		fd = mkostemp(new_path, oflag);
		if (fd >= 0)
			unlink(new_path);
		free(new_path);
		return fd;
	}
	else
		return mode ? open(path, oflag, mode) : open(path, oflag);
#endif
}

int fdRead(FD_t fd, void* buf, unsigned int nbytes) {
#if defined(_WIN32) || defined(_WIN64)
	DWORD _readbytes;
	return ReadFile((HANDLE)fd, buf, nbytes, &_readbytes, NULL) ? _readbytes : -1;
#else
	return read(fd, buf, nbytes);
#endif
}

int fdWrite(FD_t fd, const void* buf, unsigned int nbytes) {
#if defined(_WIN32) || defined(_WIN64)
	DWORD _writebytes = 0;
	return WriteFile((HANDLE)fd, buf, nbytes, &_writebytes, NULL) ? _writebytes : -1;
#else
	return write(fd, buf, nbytes);
#endif
}

long long fdSeek(FD_t fd, long long offset, int whence) {
#if defined(_WIN32) || defined(_WIN64)
	LARGE_INTEGER pos = {0};
	LARGE_INTEGER off = {0};
	off.QuadPart = offset;
	if (!SetFilePointerEx((HANDLE)fd, off, &pos, whence)) {
		pos.QuadPart = -1;
	}
	return pos.QuadPart;
#else
	return lseek(fd, offset, whence);
#endif
}

long long fdTell(FD_t fd) {
#if defined(_WIN32) || defined(_WIN64)
	LARGE_INTEGER pos = {0};
	LARGE_INTEGER off = {0};
	if (!SetFilePointerEx((HANDLE)fd, off, &pos, FILE_CURRENT)) {
		pos.QuadPart = -1;
	}
	return pos.QuadPart;
#else
	return lseek(fd, 0, SEEK_CUR);
#endif
}

BOOL fdFlush(FD_t fd) {
#if defined(_WIN32) || defined(_WIN64)
	return FlushFileBuffers((HANDLE)fd);
#else
	return fsync(fd) == 0;
#endif
}

BOOL fdClose(FD_t fd) {
	if (INVALID_FD_HANDLE == fd) {
		return TRUE;
	}
#if defined(_WIN32) || defined(_WIN64)
	return CloseHandle((HANDLE)fd);
#else
	return close(fd) == 0;
#endif
}

long long fdGetSize(FD_t fd) {
#if defined(_WIN32) || defined(_WIN64)
	LARGE_INTEGER n = {0};
	return GetFileSizeEx((HANDLE)fd, &n) ? n.QuadPart : -1;
#else
	struct stat f_stat;
	return fstat(fd, &f_stat) == 0 ? (f_stat.st_size) : -1;
#endif
}

BOOL fdSetLength(FD_t fd, long long length) {
#if defined(_WIN32) || defined(_WIN64)
	BOOL res;
	LARGE_INTEGER pos = {0};
	LARGE_INTEGER off = {0};
	if (!SetFilePointerEx((HANDLE)fd, off, &pos, FILE_CURRENT))
		return FALSE;
	off.QuadPart = length;
	if (!SetFilePointerEx((HANDLE)fd, off, NULL, FILE_BEGIN))
		return FALSE;
	res = SetEndOfFile((HANDLE)fd);
	SetFilePointerEx((HANDLE)fd, pos, NULL, FILE_BEGIN);
	return res;
#else
	return ftruncate(fd, length) == 0;
#endif
}

/* file lock */
BOOL fileLockExclusive(FD_t fd, long long offset, long long nbytes, BOOL block_bool) {
#if defined(_WIN32) || defined(_WIN64)
	DWORD dwFlags;
	OVERLAPPED overlapvar = {0};
	overlapvar.Offset = (DWORD)offset;
	overlapvar.OffsetHigh = offset >> 32;
	dwFlags = LOCKFILE_EXCLUSIVE_LOCK;
	if (!block_bool) {
		dwFlags |= LOCKFILE_FAIL_IMMEDIATELY;
	}
	return LockFileEx((HANDLE)fd, dwFlags, 0, (DWORD)nbytes, nbytes >> 32, &overlapvar);
#else
	struct flock fl = {0};
	fl.l_type = F_WRLCK;
	fl.l_start = offset;
	fl.l_whence = SEEK_SET;
	fl.l_len = nbytes;
	return fcntl(fd, block_bool ? F_SETLKW : F_SETLK, &fl) == 0;
#endif
}

BOOL fileLockShared(FD_t fd, long long offset, long long nbytes, BOOL block_bool) {
#if defined(_WIN32) || defined(_WIN64)
	DWORD dwFlags = 0;
	OVERLAPPED overlapvar = {0};
	overlapvar.Offset = (DWORD)offset;
	overlapvar.OffsetHigh = offset >> 32;
	if (!block_bool) {
		dwFlags |= LOCKFILE_FAIL_IMMEDIATELY;
	}
	return LockFileEx((HANDLE)fd, dwFlags, 0, (DWORD)nbytes, nbytes >> 32, &overlapvar);
#else
	struct flock fl = {0};
	fl.l_type = F_RDLCK;
	fl.l_start = offset;
	fl.l_whence = SEEK_SET;
	fl.l_len = nbytes;
	return fcntl(fd, block_bool ? F_SETLKW : F_SETLK, &fl) == 0;
#endif
}

BOOL fileUnlock(FD_t fd, long long offset, long long nbytes) {
#if defined(_WIN32) || defined(_WIN64)
	OVERLAPPED overlapvar = {0};
	overlapvar.Offset = (DWORD)offset;
	overlapvar.OffsetHigh = offset >> 32;
	return UnlockFileEx((HANDLE)fd, 0, (DWORD)nbytes, nbytes >> 32, &overlapvar);
#else
	struct flock fl = {0};
	fl.l_type = F_UNLCK;
	fl.l_start = offset;
	fl.l_whence = SEEK_SET;
	fl.l_len = nbytes;
	return fcntl(fd, F_SETLK, &fl) == 0;
#endif
}

/* file name */
BOOL fileIsExist(const char* path) {
#if defined(WIN32) || defined(_WIN64)
	char* szFullPath;
	WIN32_FIND_DATAA find_data;
	HANDLE hFind;
	BOOL exist;

	szFullPath = __win32_path(path);
	if (!szFullPath)
		return FALSE;
	hFind = FindFirstFileA(szFullPath, &find_data);
	if (hFind != INVALID_HANDLE_VALUE) {
		FindClose(hFind);
		exist = TRUE;
	}
	else {
		exist = FALSE;
	}
	free(szFullPath);
	return exist;
#else
	return access(path, F_OK) == 0;
#endif
}

const char* fileExtName(const char* path) {
	const char* ext = NULL;
	int i;
	for (i = strlen(path); i >= 0; --i) {
		if (path[i] == '.') {
			ext = path + i + 1;
			break;
		}
	}
	return ext;
}

const char* fileFileName(const char* path) {
	const char* fname = path;
	int i;
	for (i = strlen(path); i >= 0; --i) {
#if defined(WIN32) || defined(_WIN64)
		if (path[i] == '\\') {
#else
		if (path[i] == '/') {
#endif
			fname = path + i + 1;
			break;
		}
	}
	return fname;
}

/* file link */
BOOL fileCreateSymlink(const char* actualpath, const char* sympath) {
#if defined(_WIN32) || defined(_WIN64)
	char* szActualPath, *szSymPath;
	DWORD dwAttr, dwFlag;
	BOOLEAN ret;

	szActualPath = __win32_path(actualpath);
	if (!szActualPath)
		return FALSE;
	dwAttr = GetFileAttributesA(szActualPath);
	if (dwAttr == INVALID_FILE_ATTRIBUTES) {
		free(szActualPath);
		return FALSE;
	}
	dwFlag = (dwAttr & FILE_ATTRIBUTE_DIRECTORY) ? SYMBOLIC_LINK_FLAG_DIRECTORY : 0;
	szSymPath = __win32_path(sympath);
	if (!szSymPath)
		return FALSE;
	ret = CreateSymbolicLinkA(szSymPath, szActualPath, dwFlag);
	free(szSymPath);
	free(szActualPath);
	return ret;
#else
	return symlink(actualpath, sympath) == 0;
#endif
}

BOOL fileCreateHardLink(const char* existpath, const char* newpath) {
#if defined(_WIN32) || defined(_WIN64)
	BOOL ret;
	char *szExistPath, *szNewPath;
	szNewPath = __win32_path(newpath);
	if (!szNewPath)
		return FALSE;
	szExistPath = __win32_path(existpath);
	if (!szExistPath) {
		free(szNewPath);
		return FALSE;
	}
	ret = CreateHardLinkA(szNewPath, szExistPath, NULL);
	free(szExistPath);
	free(szNewPath);
	return ret;
#else
	return link(existpath, newpath) == 0;
#endif
}

BOOL fileDeleteHardLink(const char* existpath) {
#if defined(_WIN32) || defined(_WIN64)
	BOOL ret;
	char *szExistPath;
	szExistPath = __win32_path(existpath);
	if (!szExistPath)
		return FALSE;
	ret = DeleteFileA(szExistPath);
	free(szExistPath);
	return ret;
#else
	return unlink(existpath) == 0;
#endif
}

BOOL fileHardLinkCount(FD_t fd, unsigned int* count) {
#if defined(_WIN32) || defined(_WIN64)
	BY_HANDLE_FILE_INFORMATION info;
	if (!GetFileInformationByHandle((HANDLE)fd, &info))
		return FALSE;
	*count = info.nNumberOfLinks;
	return TRUE;
#else
	struct stat f_stat;
	if (fstat(fd, &f_stat) == 0) {
		*count = f_stat.st_nlink;
		return TRUE;
	}
	return FALSE;
#endif
}

/* directory operator */
BOOL dirCreate(const char* path) {
#if defined(_WIN32) || defined(_WIN64)
	BOOL ret;
	char *szFullPath;
	szFullPath = __win32_path(path);
	if (!szFullPath)
		return FALSE;
	ret = CreateDirectoryA(szFullPath, NULL);
	free(szFullPath);
	return ret;
#else
	return mkdir(path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == 0;
#endif
}

BOOL dirCurrentPath(char* buf, size_t n) {
#if defined(_WIN32) || defined(_WIN64)
	return GetCurrentDirectoryA(n, buf) != 0;
#else
	return getcwd(buf, n) != NULL;
#endif
}

BOOL dirSheftPath(const char* path) {
#if defined(_WIN32) || defined(_WIN64)
	BOOL ret;
	char *szFullPath;
	szFullPath = __win32_path(path);
	if (!szFullPath)
		return FALSE;
	ret = SetCurrentDirectoryA(szFullPath);
	free(szFullPath);
	return ret;
#else
	return chdir(path) == 0;
#endif
}

Dir_t dirOpen(const char* path) {
#if defined(_WIN32) || defined(_WIN64)
	HANDLE hFind;
	size_t i, len = strlen(path);
	WIN32_FIND_DATAA find_data;
	char *szFullPath;
	szFullPath = (char*)malloc(len + 3);
	if (!szFullPath)
		return INVALID_HANDLE_VALUE;
	memcpy(szFullPath, path, len);
	for (i = 0; i < len; ++i) {
		if (szFullPath[i] == '/')
			szFullPath[i] = '\\';
	}
	szFullPath[len] = 0;
	if (szFullPath[len - 1] == '\\')
		strcat(szFullPath, "*");
	else if (szFullPath[len - 1] != '*')
		strcat(szFullPath, "\\*");
	hFind = FindFirstFileA(szFullPath, &find_data);
	free(szFullPath);
	return hFind;
#else
	return opendir(path);
#endif
}

BOOL dirClose(Dir_t dir) {
#if defined(_WIN32) || defined(_WIN64)
	return FindClose(dir);
#else
	return closedir(dir) == 0;
#endif
}

BOOL dirRead(Dir_t dir, DirItem_t* item) {
#if defined(_WIN32) || defined(_WIN64)
	while (FindNextFileA(dir, item)) {
		if (!strcmp(".", item->cFileName) || !strcmp("..", item->cFileName))
			continue;
		return TRUE;
	}
	return FALSE;
#else
	struct dirent* i;
	while ((i = readdir(dir))) {
		if (!strcmp(".", i->d_name) || !strcmp("..", i->d_name))
			continue;
		*item = i;
		return TRUE;
	}
	return FALSE;
#endif
}

char* dirFileName(DirItem_t* item) {
#if defined(_WIN32) || defined(_WIN64)
	return item->cFileName;
#else
	return (*item)->d_name;
#endif
}

#ifdef  __cplusplus
}
#endif
