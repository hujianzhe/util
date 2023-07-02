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
typedef OVERLAPPED_ENTRY	AioEv_t;
#else
typedef struct AioEv_t {
	IoOverlapped_t* ol;
} AioEv_t;
#endif

typedef struct AioFD_t {
	FD_t fd;
	struct AioFD_t* __lnext;
	short __delete_flag;
	int __domain;
#if defined(_WIN32) || defined(_WIN64)
	BOOL __reg;
#endif
} AioFD_t;

typedef struct Aio_t {
#if defined(_WIN32) || defined(_WIN64)
	HANDLE __handle;
#elif	__linux__
	struct io_uring __r;
	struct io_uring_cqe** __wait_cqes;
	unsigned int __wait_cqes_cnt;
#endif
	Atom16_t __wakeup;
	AioFD_t* __free_list_head;
	void(*__fn_free_aiofd)(AioFD_t*);
} Aio_t;


#ifdef	__cplusplus
extern "C" {
#endif

__declspec_dll Aio_t* aioCreate(Aio_t* aio, void(*fn_free_aiofd)(AioFD_t*));
__declspec_dll BOOL aioClose(Aio_t* aio);
__declspec_dll void aiofdInit(AioFD_t* aiofd, FD_t fd, int domain);
__declspec_dll void aiofdDelete(Aio_t* aio, AioFD_t* aiofd);
__declspec_dll BOOL aioCommit(Aio_t* aio, AioFD_t* aiofd, IoOverlapped_t* ol, struct sockaddr* saddr, socklen_t addrlen);
__declspec_dll int aioWait(Aio_t* aio, AioEv_t* e, unsigned int n, int msec);
__declspec_dll void aioWakeup(Aio_t* aio);
__declspec_dll IoOverlapped_t* aioEventCheck(Aio_t* aio, const AioEv_t* e);

#ifdef	__cplusplus
}
#endif

#endif

#endif
