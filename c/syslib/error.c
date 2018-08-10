//
// Created by hujianzhe
//

#include "error.h"
#include <string.h>

#ifdef  __cplusplus
extern "C" {
#endif

int errnoGet(void) {
#if defined(_WIN32) || defined(_WIN64)
	switch (GetLastError()) {
		case ERROR_TOO_MANY_OPEN_FILES:
			errno = ENFILE;
			break;
		case ERROR_PATH_NOT_FOUND:
		case ERROR_FILE_NOT_FOUND:
			errno = ENOENT;
			break;
		case ERROR_DEV_NOT_EXIST:
			errno = ENODEV;
			break;
		case ERROR_FILE_EXISTS:
		case ERROR_ALREADY_EXISTS:
			errno = EEXIST;
			break;
		case WSAEBADF:
		case ERROR_INVALID_ACCEL_HANDLE:
		case ERROR_INVALID_HANDLE:
		case ERROR_BAD_PIPE:
		case ERROR_FILE_INVALID:
			errno = EBADF;
			break;
		case WSAEACCES:
		case ERROR_ACCESS_DENIED:
		case ERROR_INVALID_ACCESS:
		case ERROR_FILE_READ_ONLY:
			errno = EACCES;
			break;
		case WSAENOBUFS:
		case ERROR_NOT_ENOUGH_MEMORY:
			errno = ENOMEM;
			break;
		case ERROR_BUSY:
		case ERROR_DS_BUSY:
		case ERROR_PIPE_BUSY:
		case ERROR_BUSY_DRIVE:
			errno = EBUSY;
			break;
		case WSAETIMEDOUT:
		case ERROR_TIMEOUT:
			errno = ETIMEDOUT;
			break;
		case WSAEINVAL:
		case ERROR_INVALID_FLAGS:
		case ERROR_INVALID_PARAMETER:
			errno = EINVAL;
			break;
		case ERROR_BROKEN_PIPE:
			errno = EPIPE;
			break;
		/**/
		case WSAEINTR:
			errno = EINTR;
			break;
		case WSAESOCKTNOSUPPORT:
		case WSAEPFNOSUPPORT:
			errno = ENOTSUP;
			break;
		case WSAEPROTOTYPE:
			errno = EPROTOTYPE;
			break;
		case WSAENOPROTOOPT:
			errno = ENOPROTOOPT;
			break;
		case WSAEPROTONOSUPPORT:
			errno = EPROTONOSUPPORT;
			break;
		case WSAEMSGSIZE:
			errno = EMSGSIZE;
			break;
		case WSAEDESTADDRREQ:
			errno = EDESTADDRREQ;
			break;
		case WSAEFAULT:
		case ERROR_INVALID_ADDRESS:
			errno = EADDRNOTAVAIL;
			break;
		case WSAENOTSOCK:
			errno = ENOTSOCK;
			break;
		case WSAEMFILE:
			errno = EMFILE;
			break;
		case WSAEAFNOSUPPORT:
			errno = EAFNOSUPPORT;
			break;
		case WSAEINPROGRESS:
			errno = EINPROGRESS;
			break;
		case WSAEALREADY:
			errno = EALREADY;
			break;
		case WSAEADDRINUSE:
			errno = EADDRINUSE;
			break;
		case WSAENETUNREACH:
			errno = ENETUNREACH;
			break;
		case WSAEISCONN:
			errno = EISCONN;
			break;
		case WSAENOTCONN:
			errno = ENOTCONN;
			break;
		case WSAECONNABORTED:
			errno = ECONNABORTED;
			break;
		case WSAECONNREFUSED:
			errno = ECONNREFUSED;
			break;
		case WSAEHOSTDOWN:
		case WSAECONNRESET:
			errno = ECONNRESET;
			break;
		case WSATRY_AGAIN:
		case WSAEWOULDBLOCK:
			errno = EWOULDBLOCK;
			break;
		default:
			errno = GetLastError();
	}
#else
#if EAGAIN != EWOULDBLOCK
	if (EAGAIN == errno)
		errno = EWOULDBLOCK;
#endif
#endif
	return errno;
}

void errnoSet(int errnum) {
#if defined(_WIN32) || defined(_WIN64)
	SetLastError(errnum);
#endif
	errno = errnum;
}

char* errnoText(int errnum, char* buf, size_t bufsize) {
#if defined(_WIN32) || defined(_WIN64)
	return strerror_s(buf, bufsize, errnum) ? NULL : buf;
#else
	#if defined(_GNU_SOURCE)
	return strerror_r(errnum, buf, bufsize);
	#elif defined(_XOPEN_SOURCE)
	return strerror_r(errnum, buf, bufsize) ? NULL : buf;
	#else
	strerror_r(errnum, buf, bufsize);
	return buf;
	#endif
#endif
}

#ifdef  __cplusplus
}
#endif
