//
// Created by hujianzhe
//

#ifndef	UTIL_C_PLATFORM_DEFINE_H
#define	UTIL_C_PLATFORM_DEFINE_H

#include "compiler_define.h"

#if defined(_WIN32) || defined(_WIN64)
	#ifndef	_WIN32_WINNT
		#define	_WIN32_WINNT		0xFFFFFFFF
	#endif
	#ifndef WINVER
		#define	WINVER				0xFFFFFFFF
	#endif
	#ifndef	_WIN32_IE
		#define	_WIN32_IE			0xFFFFFFFF
	#endif
	#ifndef _CRT_SECURE_NO_WARNINGS
		#define	_CRT_SECURE_NO_WARNINGS
	#endif
	#ifndef _WINSOCK_DEPRECATED_NO_WARNINGS
		#define	_WINSOCK_DEPRECATED_NO_WARNINGS
	#endif
	#define	_CRT_RAND_S
	#include <winsock2.h>
	#include <windows.h>
	STATIC_ASSERT(sizeof(HANDLE) == sizeof(SOCKET), "");
	typedef	WSABUF					Iobuf_t;
	#define	iobufStaticInit(p, n)	{ (ULONG)(n), (char*)(p) }
	#define	iobufPtr(iobuf)			((iobuf)->buf)
	#define	iobufLen(iobuf)			((iobuf)->len)
	typedef SOCKET					FD_t;
	#define INVALID_FD_HANDLE       ((SOCKET)INVALID_HANDLE_VALUE)
	#define	INFTIM					-1
	typedef	SSIZE_T					ssize_t;
#else
	#ifndef _REENTRANT
		#define	_REENTRANT
	#endif
	#ifndef	_XOPEN_SOURCE
		#define	_XOPEN_SOURCE
	#endif
	#ifdef	__linux__
		#ifndef	_GNU_SOURCE
			#define _GNU_SOURCE
		#endif
	#endif
	#undef _FILE_OFFSET_BITS
	#define	_FILE_OFFSET_BITS		64
	#include <unistd.h>
	#include <sys/types.h>
	#include <sys/param.h>
	#include <sys/uio.h>
	typedef int						BOOL;
	#define	TRUE					1
	#define	FALSE					0
	typedef	struct iovec			Iobuf_t;
	#define	iobufStaticInit(p, n)	{ (void*)(p), n }
	#define	iobufPtr(iobuf)			((iobuf)->iov_base)
	#define iobufLen(iobuf)			((iobuf)->iov_len)
	typedef	int						FD_t;
	#define INVALID_FD_HANDLE       -1
	#ifndef INFTIM
		#define	INFTIM				-1
	#endif
#endif

#define	undefined					-1

#endif
