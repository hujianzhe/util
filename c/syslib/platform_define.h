//
// Created by hujianzhe
//

#ifndef	UTIL_C_SYSLIB_PLATFORM_DEFINE_H
#define	UTIL_C_SYSLIB_PLATFORM_DEFINE_H

#include "../compiler_define.h"

#if defined(_WIN32) || defined(_WIN64)
	#ifndef	_WIN32_WINNT
		/* windows 7 */
		#define	_WIN32_WINNT		0x0701
	#endif
	#ifndef WINVER
		#define WINVER	_WIN32_WINNT
	#endif
	#ifndef	_WIN32_IE
		#define	_WIN32_IE			0x0700
	#endif
	#define	_CRT_RAND_S
	#include <winsock2.h>
	#include <windows.h>
	typedef	WSABUF					IO_BUFFER;
	#define	iobuffer_buf(iobuf)		((iobuf)->buf)
	#define	iobuffer_len(iobuf)		((iobuf)->len)
	typedef SOCKET					FD_HANDLE;
	#define INVALID_FD_HANDLE       ((SOCKET)INVALID_HANDLE_VALUE)
	#define	INFTIM					-1
	#ifdef	_WIN64
		/* long long*/
		typedef	__int64				ssize_t;
	#else
		/* long */
		typedef	int					ssize_t;
	#endif
#else
	#ifndef _REENTRANT
		#define	_REENTRANT
	#endif
	#undef _FILE_OFFSET_BITS
	#define	_FILE_OFFSET_BITS		64
	#include <unistd.h>
	#include <sys/types.h>
	#include <sys/param.h>
	#include <stdlib.h>
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
#endif

#define	assert_true(exp)			if (!(exp)) abort()

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
