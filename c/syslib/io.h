//
// Created by hujianzhe
//

#ifndef	UTIL_C_SYSLIB_IO_H
#define	UTIL_C_SYSLIB_IO_H

#include "../platform_define.h"

#if defined(_WIN32) || defined(_WIN64)
	struct aiocb {
		union {
		#pragma pack(1)
			struct {
				OVERLAPPED __ol;
			} in;
			struct {
				unsigned char __reserved_align;
				OVERLAPPED __ol;
			} out;
		#pragma pack()
		};
		FD_t aio_fildes;
		void* aio_buf;
		unsigned int aio_nbytes;
		int aio_lio_opcode;
		long long aio_offset;
		struct {
			int sigev_notify;
			int sigev_signo;
			union {
				int sival_int;
				void* sival_ptr;
			} sigev_value;
			void(*sigev_notify_function)(struct aiocb*);
		} aio_sigevent;
	};
	#define	LIO_NOP				0
	#define	LIO_READ			1
	#define	LIO_WRITE			2
	typedef OVERLAPPED_ENTRY	NioEv_t;
	#pragma comment(lib, "wsock32.lib")
	#pragma comment(lib, "ws2_32.lib")
#elif defined(__FreeBSD__) || defined(__APPLE__)
	#include <sys/event.h>
	#include <aio.h>
	#define	LIO_NONE_NOTIFY		SIGEV_NONE
	typedef struct kevent		NioEv_t;
#elif __linux__
	#include <sys/epoll.h>
	#include <aio.h>
	typedef struct epoll_event	NioEv_t;
#endif
struct sockaddr_storage;

#ifdef	__cplusplus
extern "C" {
#endif

/* aiocb */
__declspec_dll void aioInit(struct aiocb* cb, size_t udata);
__declspec_dll void aioSetOffset(struct aiocb* cb, long long offset);
__declspec_dll BOOL aioCommit(struct aiocb* cb);
__declspec_dll BOOL aioHasCompleted(const struct aiocb* cb);
__declspec_dll BOOL aioSuspend(const struct aiocb* const cb_list[], int nent, int msec);
__declspec_dll BOOL aioCancel(FD_t fd, struct aiocb* cb);
__declspec_dll int aioResult(struct aiocb* cb, unsigned int* transfer_bytes);
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
__declspec_dll BOOL reactorCommit(Reactor_t* reactor, FD_t fd, int opcode, void* ol, struct sockaddr_storage* saddr);
__declspec_dll int reactorWait(Reactor_t* reactor, NioEv_t* e, unsigned int count, int msec);
__declspec_dll FD_t reactorEventFD(const NioEv_t* e);
__declspec_dll void reactorEventOpcodeAndOverlapped(const NioEv_t* e, int* p_opcode, void** p_ol);
__declspec_dll BOOL reactorConnectCheckSuccess(FD_t sockfd);
__declspec_dll FD_t reactorAcceptFirst(FD_t listenfd, void* ol, struct sockaddr_storage* peer_saddr);
__declspec_dll FD_t reactorAcceptNext(FD_t listenfd, struct sockaddr_storage* peer_saddr);
__declspec_dll BOOL reactorClose(Reactor_t* reactor);

#ifdef	__cplusplus
}
#endif

#endif
