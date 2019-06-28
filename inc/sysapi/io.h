//
// Created by hujianzhe
//

#ifndef	UTIL_C_SYSLIB_IO_H
#define	UTIL_C_SYSLIB_IO_H

#include "../platform_define.h"

#if defined(_WIN32) || defined(_WIN64)
	#define	LIO_NOP				0
	#define	LIO_READ			1
	#define	LIO_WRITE			2
	typedef OVERLAPPED_ENTRY	NioEv_t;
	#pragma comment(lib, "wsock32.lib")
	#pragma comment(lib, "ws2_32.lib")
#elif defined(__FreeBSD__) || defined(__APPLE__)
	#include <sys/event.h>
	#include <aio.h>
	typedef struct kevent		NioEv_t;
#elif __linux__
	#include <sys/epoll.h>
	#include <aio.h>
	typedef struct epoll_event	NioEv_t;
#endif
struct sockaddr;
struct sockaddr_storage;

#ifdef	__cplusplus
extern "C" {
#endif

/* aiocb */
typedef struct AioCtx_t {
#if defined(_WIN32) || defined(_WIN64)
	OVERLAPPED ol;
	struct {
		FD_t aio_fildes;
		void* aio_buf;
		unsigned int aio_nbytes;
		int aio_lio_opcode;
		long long aio_offset;
	} cb;
#else
	struct aiocb cb;
#endif
	void(*callback)(int error, int transfer_bytes, struct AioCtx_t* self);
} AioCtx_t;
__declspec_dll void aioInit(AioCtx_t* ctx);
__declspec_dll BOOL aioCommit(AioCtx_t* ctx);
__declspec_dll BOOL aioHasCompleted(const AioCtx_t* ctx);
__declspec_dll int aioSuspend(const AioCtx_t* ctx, int msec);
__declspec_dll BOOL aioCancel(FD_t fd, AioCtx_t* ctx);
__declspec_dll int aioError(AioCtx_t* ctx);
__declspec_dll int aioNumberOfBytesTransfered(AioCtx_t* ctx);
/* reactor */
#define	REACTOR_NOP		0
#define	REACTOR_READ	1
#define	REACTOR_WRITE	2
#define	REACTOR_ACCEPT	3
#define REACTOR_CONNECT	4
typedef struct Reactor_t {
	FD_t __hNio;
#ifdef	__linux__
	FD_t __epfd;
#endif
} Reactor_t;
__declspec_dll BOOL reactorCreate(Reactor_t* reactor);
__declspec_dll BOOL reactorReg(Reactor_t* reactor, FD_t fd);
__declspec_dll void* reactorMallocOverlapped(int opcode, const void* refbuf, unsigned int refsize, unsigned int appendsize);
__declspec_dll void reactorFreeOverlapped(void* ol);
__declspec_dll BOOL reactorCommit(Reactor_t* reactor, FD_t fd, int opcode, void* ol, struct sockaddr* saddr, int addrlen);
__declspec_dll int reactorWait(Reactor_t* reactor, NioEv_t* e, unsigned int count, int msec);
__declspec_dll void* reactorEventOverlapped(const NioEv_t* e);
__declspec_dll FD_t reactorEventFD(const NioEv_t* e);
__declspec_dll int reactorEventOpcode(const NioEv_t* e);
__declspec_dll int reactorEventOverlappedData(void* ol, Iobuf_t* iov, struct sockaddr_storage* saddr);
__declspec_dll BOOL reactorConnectCheckSuccess(FD_t sockfd);
__declspec_dll FD_t reactorAcceptFirst(FD_t listenfd, void* ol, struct sockaddr_storage* peer_saddr);
__declspec_dll FD_t reactorAcceptNext(FD_t listenfd, struct sockaddr_storage* peer_saddr);
__declspec_dll BOOL reactorClose(Reactor_t* reactor);

#ifdef	__cplusplus
}
#endif

#endif
