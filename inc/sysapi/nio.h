//
// Created by hujianzhe
//

#ifndef	UTIL_C_SYSLIB_NIO_H
#define	UTIL_C_SYSLIB_NIO_H

#include "atomic.h"

#if defined(_WIN32) || defined(_WIN64)
	#include "io_overlapped.h"
	#include <ws2tcpip.h>
	typedef OVERLAPPED_ENTRY	NioEv_t;
	#pragma comment(lib, "wsock32.lib")
	#pragma comment(lib, "ws2_32.lib")
#elif defined(__FreeBSD__) || defined(__APPLE__)
	#include <sys/event.h>
	#include <sys/socket.h>
	typedef struct kevent		NioEv_t;
#elif __linux__
	#include <sys/epoll.h>
	typedef struct epoll_event	NioEv_t;
#endif
struct sockaddr;
struct sockaddr_storage;

/* not support disk file io */

#ifdef	__cplusplus
extern "C" {
#endif

/* nio */
enum {
	NIO_OP_READ = 1,
	NIO_OP_WRITE = 2,
	NIO_OP_ACCEPT = 4,
	NIO_OP_CONNECT = 8
};

typedef struct NioFD_t {
	FD_t fd;
	struct NioFD_t* __lprev;
	struct NioFD_t* __lnext;
	short __delete_flag;
	short __reg;
#if defined(_WIN32) || defined(_WIN64)
	int __domain;
	IoOverlapped_t* __read_ol;
	IoOverlapped_t* __write_ol;
#else
	unsigned int __event_mask;
#endif
} NioFD_t;

typedef struct Nio_t {
	FD_t __hNio;
#if defined(_WIN32) || defined(_WIN64)
	IoOverlapped_t* __ol_list_head;
#else
	FD_t __socketpair[2];
#endif
	Atom16_t __wakeup;
	NioFD_t* __alive_list_head;
	NioFD_t* __free_list_head;
	void(*__fn_free_niofd)(NioFD_t*);
} Nio_t;

__declspec_dll BOOL nioCreate(Nio_t* nio, void(*fn_free_niofd)(NioFD_t*));
__declspec_dll void niofdInit(NioFD_t* niofd, FD_t fd, int domain);
__declspec_dll void niofdDelete(Nio_t* nio, NioFD_t* niofd);
__declspec_dll BOOL nioCommit(Nio_t* nio, NioFD_t* niofd, int opcode, const struct sockaddr* saddr, socklen_t addrlen);
__declspec_dll int nioWait(Nio_t* nio, NioEv_t* e, unsigned int count, int msec);
__declspec_dll void nioWakeup(Nio_t* nio);
__declspec_dll NioFD_t* nioEventCheck(Nio_t* nio, const NioEv_t* e, int* ev_mask);
__declspec_dll int nioConnectUpdate(NioFD_t* niofd);
__declspec_dll FD_t nioAccept(NioFD_t* niofd, struct sockaddr* peer_saddr, socklen_t* p_slen);
__declspec_dll BOOL nioClose(Nio_t* nio);

#ifdef	__cplusplus
}
#endif

#endif
