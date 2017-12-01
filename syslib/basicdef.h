//
// Created by hujianzhe
//

#ifndef	UTIL_SYSLIB_BASICDEF_H_
#define	UTIL_SYSLIB_BASICDEF_H_

#if defined(_WIN32) || defined(_WIN64)
	#ifndef	_WIN32_WINNT
		#define	_WIN32_WINNT	0x0701
	#endif
	#ifndef WINVER	
		#define WINVER	_WIN32_WINNT
	#endif
	#ifndef	_WIN32_IE
		#define	_WIN32_IE		0x0700
	#endif
	#define	_CRT_RAND_S
	#ifdef _MSC_VER
		#ifdef	_WIN64
			typedef long long	ssize_t;
		#else
			typedef long 		ssize_t;
		#endif
		#pragma warning(disable:4200)
		#pragma warning(disable:4018)
		#pragma warning(disable:4244)
		#pragma warning(disable:4267)
		#pragma warning(disable:4800)
		#pragma warning(disable:4819)
		#define	_CRT_SECURE_NO_WARNINGS
		#define	_WINSOCK_DEPRECATED_NO_WARNINGS
	#endif
	#include <winsock2.h>
	#include <windows.h>
	typedef	WSABUF					IO_BUFFER;
	#define	iobuffer_buf(iobuf)		((iobuf)->buf)
	#define	iobuffer_len(iobuf)		((iobuf)->len)
	typedef SOCKET					FD_HANDLE;
	#define INVALID_FD_HANDLE       ((SOCKET)INVALID_HANDLE_VALUE)
	#define	INFTIM					-1
#else
	#ifndef _REENTRANT
		#define	_REENTRANT
	#endif
	#ifdef	__APPLE__
		#ifndef	_XOPEN_SOURCE
			#define	_XOPEN_SOURCE
		#endif
	#elif	__linux__
		#ifndef	_GNU_SOURCE
			#define _GNU_SOURCE
		#endif
	#endif
	#undef _FILE_OFFSET_BITS
	#define	_FILE_OFFSET_BITS		64
	#include <unistd.h>
	#include <sys/types.h>
	#include <sys/param.h>
	typedef int						BOOL;
	#define	TRUE					1
	#define	FALSE					0
	typedef	struct iovec			IO_BUFFER;
	#define	iobuffer_buf(iobuf)		((iobuf)->iov_base)
	#define iobuffer_len(iobuf)		((iobuf)->iov_len)
	typedef	int						FD_HANDLE;
    #define INVALID_FD_HANDLE       -1
	#ifndef INFTIM
		#define	INFTIM				-1
	#endif
	#define CONTAINING_RECORD(address, type, field) ((type *)((char*)(address) - (char*)(&((type *)0)->field)))
#endif

typedef enum {
#if defined(_WIN32) || defined(_WIN64)
	EXEC_ERROR = 0,
	EXEC_SUCCESS = 1,
#else
	EXEC_ERROR = -1,
	EXEC_SUCCESS = 0,
#endif
} EXEC_RETURN;

#endif
