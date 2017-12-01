//
// Created by hujianzhe
//

#ifndef	UTIL_SYSLIB_IO_H_
#define	UTIL_SYSLIB_IO_H_

#include "basicdef.h"

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
		FD_HANDLE aio_fildes;
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
	typedef OVERLAPPED_ENTRY	NIO_EVENT;
	#pragma comment(lib, "wsock32.lib")
	#pragma comment(lib, "ws2_32.lib")
#elif defined(__FreeBSD__) || defined(__APPLE__)
	#include <sys/event.h>
	#include <aio.h>
	#define	LIO_NONE_NOTIFY		SIGEV_NONE
	typedef struct kevent		NIO_EVENT;
#elif __linux__
	#include <sys/epoll.h>
	#include <aio.h>
	typedef struct epoll_event	NIO_EVENT;
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
EXEC_RETURN aio_Cancel(FD_HANDLE fd, struct aiocb* cb);
int aio_Result(struct aiocb* cb, unsigned int* transfer_bytes);
/* NIO */
#define	REACTOR_NOP		0
#define	REACTOR_READ	1
#define	REACTOR_WRITE	2
#define	REACTOR_ACCEPT	3
#define REACTOR_CONNECT	4
typedef struct {
	FD_HANDLE __hNio;
#ifdef	__linux__
	FD_HANDLE __epfd;
#endif
} REACTOR;
typedef void (*REACTOR_ACCEPT_CALLBACK) (FD_HANDLE, struct sockaddr_storage*, size_t);
EXEC_RETURN reactor_Create(REACTOR* reactor);
EXEC_RETURN reactor_Reg(REACTOR* reactor, FD_HANDLE fd);
EXEC_RETURN reactor_Cancel(REACTOR* reactor, FD_HANDLE fd);
EXEC_RETURN reactor_Commit(REACTOR* reactor, FD_HANDLE fd, int opcode, void** p_ol, struct sockaddr_storage* saddr);
int reactor_Wait(REACTOR* reactor, NIO_EVENT* e, unsigned int count, int msec);
void reactor_Result(const NIO_EVENT* e, FD_HANDLE* p_fd, int* p_event, void** p_ol);
BOOL reactor_ConnectCheckSuccess(FD_HANDLE sockfd);
BOOL reactor_AcceptPretreatment(FD_HANDLE listenfd, void* ol, REACTOR_ACCEPT_CALLBACK cbfunc, size_t arg);
EXEC_RETURN reactor_Close(REACTOR* reactor);

#ifdef	__cplusplus
}
#endif

#endif
