//
// Created by hujianzhe
//

#ifndef	UTIL_C_SYSLIB_IO_H
#define	UTIL_C_SYSLIB_IO_H

#include "platform_define.h"

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
void aio_Init(struct aiocb* cb, size_t udata);
void aio_SetOffset(struct aiocb* cb, long long offset);
EXEC_RETURN aio_Commit(struct aiocb* cb);
BOOL aio_HasCompleted(const struct aiocb* cb);
EXEC_RETURN aio_Suspend(const struct aiocb* const cb_list[], int nent, int msec);
EXEC_RETURN aio_Cancel(FD_t fd, struct aiocb* cb);
int aio_Result(struct aiocb* cb, unsigned int* transfer_bytes);
/* NIO */
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
typedef void (*REACTOR_ACCEPT_CALLBACK) (FD_t, struct sockaddr_storage*, size_t);
EXEC_RETURN reactor_Create(Reactor_t* reactor);
EXEC_RETURN reactor_Reg(Reactor_t* reactor, FD_t fd);
EXEC_RETURN reactor_Cancel(Reactor_t* reactor, FD_t fd);
EXEC_RETURN reactor_Commit(Reactor_t* reactor, FD_t fd, int opcode, void** p_ol, struct sockaddr_storage* saddr);
int reactor_Wait(Reactor_t* reactor, NioEv_t* e, unsigned int count, int msec);
void reactor_Result(const NioEv_t* e, FD_t* p_fd, int* p_event, void** p_ol);
BOOL reactor_ConnectCheckSuccess(FD_t sockfd);
EXEC_RETURN reactor_Accept(FD_t listenfd, void* ol, REACTOR_ACCEPT_CALLBACK cbfunc, size_t arg);
EXEC_RETURN reactor_Close(Reactor_t* reactor);

#ifdef	__cplusplus
}
#endif

#endif
