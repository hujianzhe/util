//
// Created by hujianzhe
//

#if	defined(_WIN32) || defined(USE_UNIX_AIO_API)

#ifndef	UTIL_C_SYSLIB_AIO_H
#define	UTIL_C_SYSLIB_AIO_H

#include "atomic.h"
#include "io_overlapped.h"
#ifdef	__linux__
	#include <liburing.h>
#endif

#if defined(_WIN32) || defined(_WIN64)
	#include <ws2tcpip.h>
	typedef OVERLAPPED_ENTRY	AioEv_t;
	#pragma comment(lib, "wsock32.lib")
	#pragma comment(lib, "ws2_32.lib")
#else
	typedef struct AioEv_t {
		IoOverlapped_t* ol;
	} AioEv_t;
#endif

typedef struct AioOverlappedStream_t {
	IoOverlapped_t* head;
	IoOverlapped_t* tail;
	IoOverlapped_t* running;
} AioOverlappedStream_t;

typedef struct AioFD_t {
	FD_t fd;
	int domain;
	int socktype;
	int protocol;
	AioOverlappedStream_t* stream_rq;
	AioOverlappedStream_t* stream_wq;
	/* private */
	struct AioFD_t* __lprev;
	struct AioFD_t* __lnext;
	IoOverlapped_t* __ol_pending_list_tail;
	short __delete_flag;
	short __reg;
} AioFD_t;

typedef struct Aio_t {
#if defined(_WIN32) || defined(_WIN64)
	HANDLE __handle;
#elif	__linux__
	struct io_uring __r;
	int __wakeup_fds[2];
#endif
	Atom16_t __wakeup;
	AioFD_t* __alive_list_head;
	AioFD_t* __delete_list_head;
	AioFD_t* __dead_list_head;
	void(*__fn_free_aiofd)(AioFD_t*);
} Aio_t;

#ifdef	__cplusplus
extern "C" {
#endif

__declspec_dll Aio_t* aioCreate(Aio_t* aio, void(*fn_free_aiofd)(AioFD_t*), unsigned int entries);
__declspec_dll BOOL aioClose(Aio_t* aio);

__declspec_dll AioFD_t* aiofdInit(AioFD_t* aiofd, FD_t fd);
__declspec_dll void aiofdDelete(Aio_t* aio, AioFD_t* aiofd);

__declspec_dll BOOL aioCommit(Aio_t* aio, AioFD_t* aiofd, IoOverlapped_t* ol, int ol_flags);
__declspec_dll int aioWait(Aio_t* aio, AioEv_t* e, unsigned int n, int msec);
__declspec_dll void aioWakeup(Aio_t* aio);
__declspec_dll IoOverlapped_t* aioEventCheck(Aio_t* aio, const AioEv_t* e, AioFD_t** ol_aiofd);
__declspec_dll BOOL aioAckOverlappedStream(Aio_t* aio, AioFD_t* aiofd, IoOverlapped_t* ol);

#ifdef	__cplusplus
}
#endif

#endif

#endif
