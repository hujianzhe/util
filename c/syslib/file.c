//
// Created by hujianzhe
//

#include "file.h"
#include <string.h>
#include <time.h>

#ifdef	__cplusplus
extern "C" {
#endif

/* terminate */
#if	defined(_WIN32) || defined(_WIN64)
static const char* __win32_path(char* path) {
	char* p;
	for (p = path; *p; ++p) {
		if ('/' == *p)
			*p = '\\';
	}
	return path;
}
#else
static int SetTTY(int ttyfd, struct termios* old, int min, int time) {
	if (!isatty(ttyfd)) {
		return -1;
	}
	tcgetattr(ttyfd, old);
	struct termios newt = *old;
	newt.c_lflag &= ~ICANON;
	newt.c_lflag &= ~ECHO;
	newt.c_lflag &= ~ISIG;
	newt.c_cc[VMIN] = min;
	newt.c_cc[VTIME] = time;
	tcsetattr(ttyfd, TCSANOW, &newt);
	return 0;
}
#endif

int t_Kbhit(void) {
#if defined(_WIN32) || defined(_WIN64)
	return _kbhit();
#else
	struct termios old;
	SetTTY(STDIN_FILENO, &old, 0, 0);
	char c = -1;
	ssize_t n = read(STDIN_FILENO, &c, 1);
	tcsetattr(STDIN_FILENO, TCSANOW, &old);
	return !n ? 0 : c;
#endif
}

int t_Getch(void) {
#if defined(_WIN32) || defined(_WIN64)
	return _getch();
#else
	struct termios old;
	SetTTY(STDIN_FILENO, &old, 1, 0);
	char c = -1;
	int n = (int)read(STDIN_FILENO, &c, 1);
	tcsetattr(STDIN_FILENO, TCSANOW, &old);
	return c;
#endif
}

/* FD_HANDLE generate operator */
EXEC_RETURN fd_Type(FD_HANDLE fd, FD_HANDLE_TYPE* type) {
#if defined(_WIN32) || defined(_WIN64)
	DWORD mode = GetFileType((HANDLE)fd);
	switch (mode) {
		case FILE_TYPE_REMOTE:
		case FILE_TYPE_UNKNOWN:
			*type = FD_HADNLE_UNKNOWN;
			if (GetLastError() != NO_ERROR) {
				return EXEC_ERROR;
			}
			break;

		case FILE_TYPE_PIPE:
			if (GetNamedPipeInfo((HANDLE)fd, NULL, NULL, NULL, NULL))
				*type = FD_HANDLE_PIPE;
			else {
				int val;
				int len = sizeof(val);
				if (getsockopt(fd, SOL_SOCKET, SO_TYPE, (char*)&val, &len)) {
					return EXEC_ERROR;
				}
				*type = FD_HANDLE_SOCKET;
			}
			break;

		case FILE_TYPE_CHAR:
			*type = FD_HANDLE_CHAR;
			break;

		case FILE_TYPE_DISK: {
			BY_HANDLE_FILE_INFORMATION info;
			if (!GetFileInformationByHandle((HANDLE)fd, &info))
				return EXEC_ERROR;
			else if (info.dwFileAttributes & FILE_ATTRIBUTE_ARCHIVE)
				*type = FD_HANDLE_REGULAR;
			else if (info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				*type = FD_HANDLE_DIRECTORY;
			else if (info.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)
				*type = FD_HANDLE_SYMLINK;
			break;
		}
	}
	return EXEC_SUCCESS;
#else
	struct stat f_stat;
	if (fstat(fd, &f_stat) == 0) {
		mode_t mode = f_stat.st_mode;
		if (S_ISDIR(mode))
			*type = FD_HANDLE_DIRECTORY;
		else if (S_ISREG(mode))
			*type = FD_HANDLE_REGULAR;
		else if (S_ISLNK(mode))
			*type = FD_HANDLE_SYMLINK;
		else if (S_ISCHR(mode))
			*type = FD_HANDLE_CHAR;
		else if (S_ISSOCK(mode))
			*type = FD_HANDLE_SOCKET;
		else if (S_ISFIFO(mode))
			*type = FD_HANDLE_PIPE;
		else
			*type = FD_HADNLE_UNKNOWN;
		return EXEC_SUCCESS;
	}
	return EXEC_ERROR;
#endif
}

EXEC_RETURN fd_GetInheritFlag(FD_HANDLE fd, BOOL* bool_val) {
#if defined(_WIN32) || defined(_WIN64)
	DWORD flag;
	if (!GetHandleInformation((HANDLE)fd, &flag)) {
		return EXEC_ERROR;
	}
	*bool_val = ((flag & HANDLE_FLAG_INHERIT) != 0);
	return EXEC_SUCCESS;
#else
	int res = fcntl(fd,F_GETFD);
	if (res < 0) {
		return EXEC_ERROR;
	}
	*bool_val = (0 == res);
	return EXEC_SUCCESS;
#endif
}

EXEC_RETURN fd_SetInheritFlag(FD_HANDLE fd, BOOL bool_val) {
#if defined(_WIN32) || defined(_WIN64)
	return SetHandleInformation((HANDLE)fd, HANDLE_FLAG_INHERIT, bool_val ? HANDLE_FLAG_INHERIT : 0) ? EXEC_SUCCESS : EXEC_ERROR;
#else
	return fcntl(fd, F_SETFD, bool_val == 0) == 0 ? EXEC_SUCCESS : EXEC_ERROR;
#endif
}

FD_HANDLE fd_Dup(FD_HANDLE fd) {
#if defined(_WIN32) || defined(_WIN64)
	HANDLE new_fd = INVALID_HANDLE_VALUE;
	DuplicateHandle(GetCurrentProcess(), (HANDLE)fd, GetCurrentProcess(), &new_fd, 0, FALSE, DUPLICATE_SAME_ACCESS);
	return (FD_HANDLE)new_fd;
#else
	return dup(fd);
#endif
}

FD_HANDLE fd_Dup2(FD_HANDLE oldfd, FD_HANDLE newfd) {
#if defined(_WIN32) || defined(_WIN64)
	if (oldfd != newfd) {
		HANDLE new_fd = (HANDLE)newfd;
		if (!DuplicateHandle(GetCurrentProcess(), (HANDLE)oldfd, GetCurrentProcess(), (LPHANDLE)&newfd, 0, FALSE, DUPLICATE_SAME_ACCESS)) {
			return (FD_HANDLE)INVALID_HANDLE_VALUE;
		}
		CloseHandle(new_fd);
	}
	return newfd;
#else
	return dup2(oldfd, newfd);
#endif
}

/* file operator */
FD_HANDLE file_Open(const char* path, int obit) {
#if defined(_WIN32) || defined(_WIN64)
	char szFullPath[MAX_PATH];
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
			dwDesiredAccess |= FILE_APPEND_DATA | SYNCHRONIZE;
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
	if (obit & FILE_DSYNC_BIT)
		dwFlagsAndAttributes |= FILE_FLAG_WRITE_THROUGH;
	if (obit & FILE_ASYNC_BIT)
		dwFlagsAndAttributes |= FILE_FLAG_OVERLAPPED;
	dwFlagsAndAttributes |= FILE_ATTRIBUTE_NORMAL;
	//
	return (FD_HANDLE)CreateFileA(
		__win32_path(strcpy(szFullPath, path)), dwDesiredAccess,
		FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
		&sa, dwCreationDisposition, dwFlagsAndAttributes, NULL);
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
	//
	return mode ? open(path, oflag, mode) : open(path, oflag);
#endif
}

int file_Read(FD_HANDLE fd, void* buf, unsigned int nbytes) {
#if defined(_WIN32) || defined(_WIN64)
	DWORD _readbytes;
	return ReadFile((HANDLE)fd, buf, nbytes, &_readbytes, NULL) ? _readbytes : -1;
#else
	return read(fd, buf, nbytes);
#endif
}

int file_Write(FD_HANDLE fd, const void* buf, unsigned int nbytes) {
#if defined(_WIN32) || defined(_WIN64)
	DWORD _writebytes = 0;
	return WriteFile((HANDLE)fd, buf, nbytes, &_writebytes, NULL) ? _writebytes : -1;
#else
	return write(fd, buf, nbytes);
#endif
}

long long file_Seek(FD_HANDLE fd, long long offset, int whence) {
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

long long file_Tell(FD_HANDLE fd) {
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

EXEC_RETURN file_Flush(FD_HANDLE fd) {
#if defined(_WIN32) || defined(_WIN64)
	return FlushFileBuffers((HANDLE)fd) ? EXEC_SUCCESS : EXEC_ERROR;
#else
	return fsync(fd) == 0 ? EXEC_SUCCESS : EXEC_ERROR;
#endif
}

EXEC_RETURN file_Close(FD_HANDLE fd) {
	if (INVALID_FD_HANDLE == fd) {
		return EXEC_SUCCESS;
	}
#if defined(_WIN32) || defined(_WIN64)
	return CloseHandle((HANDLE)fd) ? EXEC_SUCCESS : EXEC_ERROR;
#else
	return close(fd) == 0 ? EXEC_SUCCESS : EXEC_ERROR;
#endif
}

long long file_Size(FD_HANDLE fd) {
#if defined(_WIN32) || defined(_WIN64)
	LARGE_INTEGER n = {0};
	return GetFileSizeEx((HANDLE)fd, &n) ? n.QuadPart : -1;
#else
	struct stat f_stat;
	return fstat(fd, &f_stat) == 0 ? (f_stat.st_size) : -1;
#endif
}

EXEC_RETURN file_ChangeLength(FD_HANDLE fd, long long length) {
#if defined(_WIN32) || defined(_WIN64)
	BOOL res;
	LARGE_INTEGER pos = {0};
	LARGE_INTEGER off = {0};
	if (!SetFilePointerEx((HANDLE)fd, off, &pos, FILE_CURRENT))
		return EXEC_ERROR;
	off.QuadPart = length;
	if (!SetFilePointerEx((HANDLE)fd, off, NULL, FILE_BEGIN))
		return EXEC_ERROR;
	res = SetEndOfFile((HANDLE)fd);
	SetFilePointerEx((HANDLE)fd, pos, NULL, FILE_BEGIN);
	return res ? EXEC_SUCCESS : EXEC_ERROR;
#else
	return ftruncate(fd, length) == 0 ? EXEC_SUCCESS : EXEC_ERROR;
#endif
}

/* file lock */
EXEC_RETURN file_LockExclusive(FD_HANDLE fd, long long offset, long long nbytes, BOOL block_bool) {
#if defined(_WIN32) || defined(_WIN64)
	DWORD dwFlags;
	OVERLAPPED overlapvar = {0};
	overlapvar.Offset = (DWORD)offset;
	overlapvar.OffsetHigh = offset >> 32;
	dwFlags = LOCKFILE_EXCLUSIVE_LOCK;
	if (!block_bool) {
		dwFlags |= LOCKFILE_FAIL_IMMEDIATELY;
	}
	return LockFileEx((HANDLE)fd, dwFlags, 0, (DWORD)nbytes, nbytes >> 32, &overlapvar) ? EXEC_SUCCESS : EXEC_ERROR;
#else
	struct flock fl = {0};
	fl.l_type = F_WRLCK;
	fl.l_start = offset;
	fl.l_whence = SEEK_SET;
	fl.l_len = nbytes;
	return fcntl(fd, block_bool ? F_SETLKW : F_SETLK, &fl) == 0 ? EXEC_SUCCESS : EXEC_ERROR;
#endif
}

EXEC_RETURN file_LockShared(FD_HANDLE fd, long long offset, long long nbytes, BOOL block_bool) {
#if defined(_WIN32) || defined(_WIN64)
	DWORD dwFlags = 0;
	OVERLAPPED overlapvar = {0};
	overlapvar.Offset = (DWORD)offset;
	overlapvar.OffsetHigh = offset >> 32;
	if (!block_bool) {
		dwFlags |= LOCKFILE_FAIL_IMMEDIATELY;
	}
	return LockFileEx((HANDLE)fd, dwFlags, 0, (DWORD)nbytes, nbytes >> 32, &overlapvar) ? EXEC_SUCCESS : EXEC_ERROR;
#else
	struct flock fl = {0};
	fl.l_type = F_RDLCK;
	fl.l_start = offset;
	fl.l_whence = SEEK_SET;
	fl.l_len = nbytes;
	return fcntl(fd, block_bool ? F_SETLKW : F_SETLK, &fl) == 0 ? EXEC_SUCCESS : EXEC_ERROR;
#endif
}

EXEC_RETURN file_Unlock(FD_HANDLE fd, long long offset, long long nbytes) {
#if defined(_WIN32) || defined(_WIN64)
	OVERLAPPED overlapvar = {0};
	overlapvar.Offset = (DWORD)offset;
	overlapvar.OffsetHigh = offset >> 32;
	return UnlockFileEx((HANDLE)fd, 0, (DWORD)nbytes, nbytes >> 32, &overlapvar) ? EXEC_SUCCESS : EXEC_ERROR;
#else
	struct flock fl = {0};
	fl.l_type = F_UNLCK;
	fl.l_start = offset;
	fl.l_whence = SEEK_SET;
	fl.l_len = nbytes;
	return fcntl(fd, F_SETLK, &fl) == 0 ? EXEC_SUCCESS : EXEC_ERROR;
#endif
}

/* file name */
const char* file_ExtName(const char* path) {
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

const char* file_FileName(const char* path) {
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
EXEC_RETURN file_CreateSymlink(const char* actualpath, const char* sympath) {
#if defined(_WIN32) || defined(_WIN64)
	char szActualPath[MAX_PATH], szSymPath[MAX_PATH];
	DWORD dwAttr, dwFlag;
	dwAttr = GetFileAttributesA(__win32_path(strcpy(szActualPath, actualpath)));
	if (dwAttr == INVALID_FILE_ATTRIBUTES)
		return EXEC_ERROR;
	dwFlag = (dwAttr & FILE_ATTRIBUTE_DIRECTORY) ? SYMBOLIC_LINK_FLAG_DIRECTORY : 0;
	return CreateSymbolicLinkA(__win32_path(strcpy(szSymPath, sympath)), szActualPath, dwFlag) ? EXEC_SUCCESS : EXEC_ERROR;
#else
	return symlink(actualpath, sympath) == 0 ? EXEC_SUCCESS : EXEC_ERROR;
#endif
}

EXEC_RETURN file_CreateHardLink(const char* existpath, const char* newpath) {
#if defined(_WIN32) || defined(_WIN64)
	char szExistPath[MAX_PATH], szNewPath[MAX_PATH];
	return CreateHardLinkA(__win32_path(strcpy(szNewPath, newpath)), __win32_path(strcpy(szExistPath, existpath)), NULL) ? EXEC_SUCCESS : EXEC_ERROR;
#else
	return link(existpath, newpath) == 0 ? EXEC_SUCCESS : EXEC_ERROR;
#endif
}

EXEC_RETURN file_DeleteHardLink(const char* existpath) {
#if defined(_WIN32) || defined(_WIN64)
	char szExistPath[MAX_PATH];
	return DeleteFileA(__win32_path(strcpy(szExistPath, existpath))) ? EXEC_SUCCESS : EXEC_ERROR;
#else
	return unlink(existpath) == 0 ? EXEC_SUCCESS : EXEC_ERROR;
#endif
}

EXEC_RETURN file_HardLinkCount(FD_HANDLE fd, unsigned int* count) {
#if defined(_WIN32) || defined(_WIN64)
	BY_HANDLE_FILE_INFORMATION info;
	if (!GetFileInformationByHandle((HANDLE)fd, &info))
		return EXEC_ERROR;
	*count = info.nNumberOfLinks;
	return EXEC_SUCCESS;
#else
	struct stat f_stat;
	if (fstat(fd, &f_stat) == 0) {
		*count = f_stat.st_nlink;
		return EXEC_SUCCESS;
	}
	return EXEC_ERROR;
#endif
}

/* directory operator */
EXEC_RETURN dir_Create(const char* path) {
#if defined(_WIN32) || defined(_WIN64)
	char szFullPath[MAX_PATH];
	return CreateDirectoryA(__win32_path(strcpy(szFullPath, path)), NULL) ? EXEC_SUCCESS : EXEC_ERROR;
#else
	return mkdir(path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == 0 ? EXEC_SUCCESS : EXEC_ERROR;
#endif
}

EXEC_RETURN dir_CurrentPath(char* buf, size_t n) {
#if defined(_WIN32) || defined(_WIN64)
	return GetCurrentDirectoryA(n, buf) ? EXEC_SUCCESS : EXEC_ERROR;
#else
	return getcwd(buf, n) ? EXEC_SUCCESS : EXEC_ERROR;
#endif
}

EXEC_RETURN dir_Sheft(const char* path) {
#if defined(_WIN32) || defined(_WIN64)
	char szFullPath[MAX_PATH];
	return SetCurrentDirectoryA(__win32_path(strcpy(szFullPath, path))) ? EXEC_SUCCESS : EXEC_ERROR;
#else
	return chdir(path) == 0 ? EXEC_SUCCESS : EXEC_ERROR;
#endif
}

DIRECTORY dir_Open(const char* path) {
#if defined(_WIN32) || defined(_WIN64)
	size_t len;
	WIN32_FIND_DATAA fd;
	char szFullPath[MAX_PATH];
	__win32_path(strcpy(szFullPath, path));
	len = strlen(szFullPath);
	if (szFullPath[len - 1] == '\\')
		strcat(szFullPath, "*");
	else if (szFullPath[len - 1] != '*')
		strcat(szFullPath, "\\*");
	return FindFirstFileA(szFullPath, &fd);
#else
	return opendir(path);
#endif
}

EXEC_RETURN dir_Close(DIRECTORY dir) {
#if defined(_WIN32) || defined(_WIN64)
	return FindClose(dir) ? EXEC_SUCCESS : EXEC_ERROR;
#else
	return closedir(dir) == 0 ? EXEC_SUCCESS : EXEC_ERROR;
#endif
}

EXEC_RETURN dir_Read(DIRECTORY dir, DIR_ITEM* item) {
#if defined(_WIN32) || defined(_WIN64)
	while (FindNextFileA(dir, item)) {
		if (!strcmp(".", item->cFileName) || !strcmp("..", item->cFileName))
			continue;
		return EXEC_SUCCESS;
	}
	return EXEC_ERROR;
#else
	struct dirent* i;
	while ((i = readdir(dir))) {
		if (!strcmp(".", i->d_name) || !strcmp("..", i->d_name))
			continue;
		*item = i;
		return EXEC_SUCCESS;
	}
	return EXEC_ERROR;
#endif
}

char* dir_FileName(DIR_ITEM* item) {
#if defined(_WIN32) || defined(_WIN64)
	return item->cFileName;
#else
	return (*item)->d_name;
#endif
}

#ifdef  __cplusplus
}
#endif
