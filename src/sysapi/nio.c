//
// Created by hujianzhe
//

#include "../../inc/sysapi/nio.h"
#include "../../inc/sysapi/assert.h"
#if defined(_WIN32) || defined(_WIN64)
	#include "../../inc/sysapi/io_overlapped.h"
	#include <mswsock.h>
	#include <ws2ipdef.h>
	#include <stdlib.h>
#else
	#include <arpa/inet.h>
	#include <sys/ioctl.h>
#endif
#include <errno.h>
#include <string.h>

#ifdef	__cplusplus
extern "C" {
#endif

static void nio_reg_alive_niofd(Nio_t* nio, NioFD_t* niofd) {
	niofd->__lprev = NULL;
	niofd->__lnext = nio->__alive_list_head;
	if (nio->__alive_list_head) {
		nio->__alive_list_head->__lprev = niofd;
	}
	nio->__alive_list_head = niofd;
}

static void nio_handle_free_list(Nio_t* nio) {
	NioFD_t* cur_free, * next_free;
	for (cur_free = nio->__free_list_head; cur_free; cur_free = next_free) {
		next_free = cur_free->__lnext;
		nio->__fn_free_niofd(cur_free);
	}
	nio->__free_list_head = NULL;
}

static void nio_exit_clean_soft(Nio_t* nio) {
	NioFD_t* niofd, *niofd_next;
	for (niofd = nio->__alive_list_head; niofd; niofd = niofd_next) {
		niofd_next = niofd->__lnext;
		niofdDelete(nio, niofd);
	}
	nio->__alive_list_head = NULL;
	nio_handle_free_list(nio);
}

#if _WIN32
extern BOOL win32_Iocp_PrepareRegUdp(SOCKET fd, int domain);
extern int win32_OverlappedConnectUpdate(SOCKET fd);

static void iocp_nio_link_ol(Nio_t* nio, IoOverlapped_t* ol) {
	ol->__prev = NULL;
	ol->__next = nio->__ol_list_head;
	if (nio->__ol_list_head) {
		nio->__ol_list_head->__prev = ol;
	}
	nio->__ol_list_head = ol;
}

static void iocp_nio_unlink_ol(Nio_t* nio, IoOverlapped_t* ol) {
	if (nio->__ol_list_head == ol) {
		nio->__ol_list_head = ol->__next;
	}
	if (ol->__prev) {
		ol->__prev->__next = ol->__next;
	}
	if (ol->__next) {
		ol->__next->__prev = ol->__prev;
	}
}

static void iocp_nio_exit_clean(Nio_t* nio) {
	while (nio->__ol_list_head) {
		ULONG i, n;
		OVERLAPPED_ENTRY e[128];
		if (!GetQueuedCompletionStatusEx((HANDLE)nio->__hNio, e, sizeof(e) / sizeof(e[0]), &n, 0, FALSE)) {
			if (GetLastError() == WAIT_TIMEOUT) {
				continue;
			}
			break;
		}
		for (i = 0; i < n; ++i) {
			IoOverlapped_t* ol = (IoOverlapped_t*)e[i].lpOverlapped;
			if (!ol) {
				continue;
			}
			iocp_nio_unlink_ol(nio, ol);
			ol->commit = 0;
			IoOverlapped_free(ol);
		}
		Sleep(5); /* avoid cpu busy */
	}
}

#endif

/* NIO */
BOOL nioCreate(Nio_t* nio, void(*fn_free_niofd)(NioFD_t*)) {
#if defined(_WIN32) || defined(_WIN64)
	SYSTEM_INFO si;
	GetSystemInfo(&si);
	nio->__wakeup = 0;
	nio->__ol_list_head = NULL;
	nio->__alive_list_head = NULL;
	nio->__free_list_head = NULL;
	nio->__fn_free_niofd = fn_free_niofd;
	nio->__hNio = (FD_t)CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, si.dwNumberOfProcessors << 1);
	return nio->__hNio != 0;
#elif __linux__
	int nio_ok = 0, pair_ok = 0;
	do {
		int nb = 1;
		struct epoll_event e = { 0 };
		nio->__hNio = epoll_create1(EPOLL_CLOEXEC);
		if (nio->__hNio < 0) {
			break;
		}
		nio_ok = 1;
		if (socketpair(AF_UNIX, SOCK_STREAM, 0, nio->__socketpair)) {
			break;
		}
		pair_ok = 1;
		if (ioctl(nio->__socketpair[0], FIONBIO, &nb)) {
			break;
		}
		if (ioctl(nio->__socketpair[1], FIONBIO, &nb)) {
			break;
		}
		e.data.ptr = &nio->__socketpair[0];
		e.events = EPOLLIN;
		if (epoll_ctl(nio->__hNio, EPOLL_CTL_ADD, nio->__socketpair[0], &e)) {
			break;
		}
		nio->__wakeup = 0;
		nio->__alive_list_head = NULL;
		nio->__free_list_head = NULL;
		nio->__fn_free_niofd = fn_free_niofd;
		return TRUE;
	} while (0);
	if (nio_ok) {
		assertTRUE(0 == close(nio->__hNio));
	}
	if (pair_ok) {
		assertTRUE(0 == close(nio->__socketpair[0]));
		assertTRUE(0 == close(nio->__socketpair[1]));
	}
	return FALSE;
#elif defined(__FreeBSD__) || defined(__APPLE__)
	int nio_ok = 0, pair_ok = 0;
	do {
		int nb = 1;
		struct kevent e;
		nio->__hNio = kqueue();
		if (nio->__hNio < 0) {
			break;
		}
		nio_ok = 1;
		if (socketpair(AF_UNIX, SOCK_STREAM, 0, nio->__socketpair)) {
			break;
		}
		pair_ok = 1;
		if (ioctl(nio->__socketpair[0], FIONBIO, &nb)) {
			break;
		}
		if (ioctl(nio->__socketpair[1], FIONBIO, &nb)) {
			break;
		}
		EV_SET(&e, (uintptr_t)(nio->__socketpair[0]), EVFILT_READ, EV_ADD, 0, 0, &nio->__socketpair[0]);
		if (kevent(nio->__hNio, &e, 1, NULL, 0, NULL)) {
			break;
		}
		nio->__wakeup = 0;
		nio->__alive_list_head = NULL;
		nio->__free_list_head = NULL;
		nio->__fn_free_niofd = fn_free_niofd;
		return TRUE;
	} while (0);
	if (nio_ok) {
		assertTRUE(0 == close(nio->__hNio));
	}
	if (pair_ok) {
		assertTRUE(0 == close(nio->__socketpair[0]));
		assertTRUE(0 == close(nio->__socketpair[1]));
	}
	return FALSE;
#endif
}

void niofdInit(NioFD_t* niofd, FD_t fd, int domain) {
	niofd->fd = fd;
	niofd->__lprev = NULL;
	niofd->__lnext = NULL;
	niofd->__delete_flag = 0;
	niofd->__reg = 0;
#if defined(_WIN32) || defined(_WIN64)
	niofd->__domain = domain;
	niofd->__read_ol = NULL;
	niofd->__write_ol = NULL;
#else
	niofd->__event_mask = 0;
#endif
}

void niofdDelete(Nio_t* nio, NioFD_t* niofd) {
	if (niofd->__delete_flag) {
		return;
	}
	niofd->__delete_flag = 1;
#if defined(_WIN32) || defined(_WIN64)
	if (niofd->__domain != AF_UNSPEC) {
		closesocket(niofd->fd);
		niofd->fd = INVALID_SOCKET;
	}
	else {
		CloseHandle((HANDLE)niofd->fd);
		niofd->fd = (FD_t)INVALID_HANDLE_VALUE;
	}
	if (niofd->__read_ol) {
		IoOverlapped_free(niofd->__read_ol);
		niofd->__read_ol = NULL;
	}
	if (niofd->__write_ol) {
		IoOverlapped_free(niofd->__write_ol);
		niofd->__write_ol = NULL;
	}
#else
	close(niofd->fd);
	niofd->fd = -1;
#endif
	if (!niofd->__reg) {
		nio->__fn_free_niofd(niofd);
		return;
	}
	/* remove from alive list */
	if (niofd->__lprev) {
		niofd->__lprev->__lnext = niofd->__lnext;
	}
	if (niofd->__lnext) {
		niofd->__lnext->__lprev = niofd->__lprev;
	}
	if (niofd == nio->__alive_list_head) {
		nio->__alive_list_head = niofd->__lnext;
	}
	/* insert into free list */
	niofd->__lprev = NULL;
	niofd->__lnext = nio->__free_list_head;
	nio->__free_list_head = niofd;
}

BOOL nioCommit(Nio_t* nio, NioFD_t* niofd, int opcode, const struct sockaddr* saddr, socklen_t addrlen) {
#if defined(_WIN32) || defined(_WIN64)
	int fd_domain = niofd->__domain;
	FD_t fd = niofd->fd;
	IoOverlapped_t* ol = NULL;
	if (niofd->__delete_flag) {
		return FALSE;
	}
	if (!niofd->__reg) {
		if (AF_UNSPEC != fd_domain) {
			int socktype;
			int optlen = sizeof(socktype);
			if (getsockopt((SOCKET)fd, SOL_SOCKET, SO_TYPE, (char*)&socktype, &optlen)) {
				return FALSE;
			}
			if (SOCK_DGRAM == socktype) {
				if (!win32_Iocp_PrepareRegUdp(fd, fd_domain)) {
					return FALSE;
				}
			}
		}
		if (CreateIoCompletionPort((HANDLE)fd, (HANDLE)(nio->__hNio), (ULONG_PTR)niofd, 0) != (HANDLE)(nio->__hNio)) {
			return FALSE;
		}
		niofd->__reg = 1;
		/* insert into alive list */
		nio_reg_alive_niofd(nio, niofd);
	}

	if (NIO_OP_READ == opcode) {
		WSABUF* ptr_wsabuf;
		IocpReadOverlapped_t* read_ol;
		if (!niofd->__read_ol) {
			niofd->__read_ol = IoOverlapped_alloc(IO_OVERLAPPED_OP_READ, 0);
			if (!niofd->__read_ol) {
				return FALSE;
			}
		}
		else if (niofd->__read_ol->commit) {
			return TRUE;
		}

		read_ol = (IocpReadOverlapped_t*)niofd->__read_ol;
		ptr_wsabuf = &read_ol->base.iobuf;

		if (fd_domain != AF_UNSPEC) {
			read_ol->saddrlen = sizeof(read_ol->saddr);
			if (ptr_wsabuf->buf && ptr_wsabuf->len) {
				read_ol->dwFlags = 0;
			}
			else {
				read_ol->dwFlags = MSG_PEEK;
			}
			if (WSARecvFrom((SOCKET)fd, ptr_wsabuf, 1, NULL, &read_ol->dwFlags, (struct sockaddr*)&read_ol->saddr, &read_ol->saddrlen, (LPWSAOVERLAPPED)&read_ol->base.ol, NULL)) {
				if (WSAGetLastError() != WSA_IO_PENDING) {
					return FALSE;
				}
			}
		}
		else {
			if (!ReadFile((HANDLE)fd, ptr_wsabuf->buf, ptr_wsabuf->len, NULL, (LPOVERLAPPED)&read_ol->base.ol)) {
				if (GetLastError() != ERROR_IO_PENDING) {
					return FALSE;
				}
			}
		}
		ol = niofd->__read_ol;
	}
	else if (NIO_OP_WRITE == opcode) {
		WSABUF* ptr_wsabuf;
		IocpWriteOverlapped_t* write_ol;
		if (!niofd->__write_ol) {
			niofd->__write_ol = IoOverlapped_alloc(IO_OVERLAPPED_OP_WRITE, 0);
			if (!niofd->__write_ol) {
				return FALSE;
			}
		}
		else if (niofd->__write_ol->commit) {
			return TRUE;
		}

		write_ol = (IocpWriteOverlapped_t*)niofd->__write_ol;
		ptr_wsabuf = &write_ol->base.iobuf;

		if (fd_domain != AF_UNSPEC) {
			const struct sockaddr* toaddr;
			if (addrlen > 0 && saddr) {
				toaddr = (const struct sockaddr*)&write_ol->saddr;
				memmove(&write_ol->saddr, saddr, addrlen);
				write_ol->saddrlen = addrlen;
			}
			else {
				toaddr = NULL;
				addrlen = 0;
				write_ol->saddr.ss_family = AF_UNSPEC;
				write_ol->saddrlen = 0;
			}
			if (WSASendTo((SOCKET)fd, ptr_wsabuf, 1, NULL, write_ol->dwFlags, toaddr, addrlen, (LPWSAOVERLAPPED)&write_ol->base.ol, NULL)) {
				if (WSAGetLastError() != WSA_IO_PENDING) {
					return FALSE;
				}
			}
		}
		else {
			if (!WriteFile((HANDLE)fd, ptr_wsabuf->buf, ptr_wsabuf->len, NULL, (LPOVERLAPPED)&write_ol->base.ol)) {
				if (GetLastError() != ERROR_IO_PENDING) {
					return FALSE;
				}
			}
		}
		ol = niofd->__write_ol;
	}
	else if (NIO_OP_ACCEPT == opcode) {
		static LPFN_ACCEPTEX lpfnAcceptEx = NULL;
		IocpAcceptExOverlapped_t* accept_ol;
		if (!niofd->__read_ol) {
			niofd->__read_ol = IoOverlapped_alloc(IO_OVERLAPPED_OP_ACCEPT, 0);
			if (!niofd->__read_ol) {
				return FALSE;
			}
		}
		else if (niofd->__read_ol->commit) {
			return TRUE;
		}

		accept_ol = (IocpAcceptExOverlapped_t*)niofd->__read_ol;

		if (!lpfnAcceptEx) {
			DWORD dwBytes;
			GUID GuidAcceptEx = WSAID_ACCEPTEX;
			if (WSAIoctl((SOCKET)fd, SIO_GET_EXTENSION_FUNCTION_POINTER,
				&GuidAcceptEx, sizeof(GuidAcceptEx),
				&lpfnAcceptEx, sizeof(lpfnAcceptEx),
				&dwBytes, NULL, NULL) == SOCKET_ERROR || !lpfnAcceptEx)
			{
				return FALSE;
			}
		}
		if (INVALID_SOCKET == accept_ol->acceptsocket) {
			accept_ol->acceptsocket = socket(fd_domain, SOCK_STREAM, 0);
			if (INVALID_SOCKET == accept_ol->acceptsocket) {
				return FALSE;
			}
		}
		if (!lpfnAcceptEx((SOCKET)fd, accept_ol->acceptsocket, accept_ol->saddr_bytes, 0,
			sizeof(struct sockaddr_storage) + 16, sizeof(struct sockaddr_storage) + 16,
			NULL, (LPOVERLAPPED)&accept_ol->base.ol))
		{
			if (WSAGetLastError() != ERROR_IO_PENDING) {
				closesocket(accept_ol->acceptsocket);
				accept_ol->acceptsocket = INVALID_SOCKET;
				return FALSE;
			}
		}
		accept_ol->listensocket = (SOCKET)fd;
		ol = niofd->__read_ol;
	}
	else if (NIO_OP_CONNECT == opcode) {
		static LPFN_CONNECTEX lpfnConnectEx = NULL;
		struct sockaddr_storage st;
		WSABUF* ptr_wsabuf;
		IocpConnectExOverlapped_t* conn_ol;
		if (!niofd->__write_ol) {
			niofd->__write_ol = IoOverlapped_alloc(IO_OVERLAPPED_OP_CONNECT, 0);
			if (!niofd->__write_ol) {
				return FALSE;
			}
		}
		else if (niofd->__write_ol->commit) {
			return TRUE;
		}

		conn_ol = (IocpConnectExOverlapped_t*)niofd->__write_ol;
		/* ConnectEx must use really namelen, otherwise report WSAEADDRNOTAVAIL(10049) */
		if (!lpfnConnectEx){
			DWORD dwBytes;
			GUID GuidConnectEx = WSAID_CONNECTEX;
			if (WSAIoctl((SOCKET)fd, SIO_GET_EXTENSION_FUNCTION_POINTER,
				&GuidConnectEx, sizeof(GuidConnectEx),
				&lpfnConnectEx, sizeof(lpfnConnectEx),
				&dwBytes, NULL, NULL) == SOCKET_ERROR || !lpfnConnectEx)
			{
				return FALSE;
			}
		}
		memset(&st, 0, sizeof(st));
		st.ss_family = fd_domain;
		if (bind((SOCKET)fd, (struct sockaddr*)&st, addrlen)) {
			return FALSE;
		}
		memmove(&conn_ol->saddr, saddr, addrlen);
		ptr_wsabuf = &conn_ol->base.iobuf;
		if (!lpfnConnectEx((SOCKET)fd, (const struct sockaddr*)&conn_ol->saddr, addrlen, ptr_wsabuf->buf, ptr_wsabuf->len, NULL, (LPWSAOVERLAPPED)&conn_ol->base.ol)) {
			if (WSAGetLastError() != ERROR_IO_PENDING) {
				return FALSE;
			}
		}
		ol = niofd->__write_ol;
	}
	else {
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}
	ol->commit = 1;
	iocp_nio_link_ol(nio, ol);
	return TRUE;
#elif defined(__linux__)
	struct epoll_event e;
	unsigned int event_mask = niofd->__event_mask;
	unsigned int sys_event_mask = 0;
	if (niofd->__delete_flag) {
		return FALSE;
	}
	if (event_mask & NIO_OP_READ) {
		sys_event_mask |= EPOLLIN;
	}
	if (event_mask & NIO_OP_WRITE) {
		sys_event_mask |= EPOLLOUT;
	}

	if (NIO_OP_READ == opcode) {
		if (event_mask & NIO_OP_READ) {
			return TRUE;
		}
		event_mask |= NIO_OP_READ;
		sys_event_mask |= EPOLLIN;
	}
	else if (NIO_OP_WRITE == opcode) {
		if (event_mask & NIO_OP_WRITE) {
			return TRUE;
		}
		event_mask |= NIO_OP_WRITE;
		sys_event_mask |= EPOLLOUT;
	}
	else if (NIO_OP_ACCEPT == opcode) {
		if (event_mask & NIO_OP_READ) {
			return TRUE;
		}
		event_mask |= (NIO_OP_READ | NIO_OP_ACCEPT);
		sys_event_mask |= EPOLLIN;
	}
	else if (NIO_OP_CONNECT == opcode) {
		if (event_mask & NIO_OP_WRITE) {
			return TRUE;
		}
		if (connect(niofd->fd, saddr, addrlen) && EINPROGRESS != errno) {
			return FALSE;
		}
		event_mask |= (NIO_OP_WRITE | NIO_OP_CONNECT);
		sys_event_mask |= EPOLLOUT;
		/* 
		 * fd always add epoll when connect immediately finish or not...
		 * because I need Unified handle event
		 */
	}
	else {
		errno = EINVAL;
		return FALSE;
	}

	e.data.ptr = niofd;
	e.events = EPOLLET | sys_event_mask;
	if (epoll_ctl(nio->__hNio, EPOLL_CTL_MOD, niofd->fd, &e)) {
		if (ENOENT != errno) {
			return FALSE;
		}
		if (epoll_ctl(nio->__hNio, EPOLL_CTL_ADD, niofd->fd, &e) && EEXIST != errno) {
			return FALSE;
		}
	}
	niofd->__event_mask = event_mask;

	if (!niofd->__reg) {
		niofd->__reg = 1;
		/* insert into alive list */
		nio_reg_alive_niofd(nio, niofd);
	}
	return TRUE;
#elif defined(__FreeBSD__) || defined(__APPLE__)
	struct kevent e;
	if (niofd->__delete_flag) {
		return FALSE;
	}
	if (NIO_OP_READ == opcode) {
		if (niofd->__event_mask & NIO_OP_READ) {
			return TRUE;
		}
		EV_SET(&e, (uintptr_t)niofd->fd, EVFILT_READ, EV_ADD | EV_ONESHOT, 0, 0, niofd);
		if (kevent(nio->__hNio, &e, 1, NULL, 0, NULL)) {
			return FALSE;
		}
		niofd->__event_mask |= NIO_OP_READ;
	}
	else if (NIO_OP_WRITE == opcode) {
		if (niofd->__event_mask & NIO_OP_WRITE) {
			return TRUE;
		}
		EV_SET(&e, (uintptr_t)niofd->fd, EVFILT_WRITE, EV_ADD | EV_ONESHOT, 0, 0, niofd);
		if (kevent(nio->__hNio, &e, 1, NULL, 0, NULL)) {
			return FALSE;
		}
		niofd->__event_mask |= NIO_OP_WRITE;
	}
	else if (NIO_OP_ACCEPT == opcode) {
		if (niofd->__event_mask & NIO_OP_READ) {
			return TRUE;
		}
		EV_SET(&e, (uintptr_t)niofd->fd, EVFILT_READ, EV_ADD | EV_ONESHOT, 0, 0, niofd);
		if (kevent(nio->__hNio, &e, 1, NULL, 0, NULL)) {
			return FALSE;
		}
		niofd->__event_mask |= (NIO_OP_READ | NIO_OP_ACCEPT);
	}
	else if (NIO_OP_CONNECT == opcode) {
		if (niofd->__event_mask & NIO_OP_WRITE) {
			return TRUE;
		}
		if (connect(niofd->fd, saddr, addrlen) && EINPROGRESS != errno) {
			return FALSE;
		}
		/* 
		 * fd always add kevent when connect immediately finish or not...
		 * because I need Unified handle event
		 */
		EV_SET(&e, (uintptr_t)niofd->fd, EVFILT_WRITE, EV_ADD | EV_ONESHOT, 0, 0, niofd);
		if (kevent(nio->__hNio, &e, 1, NULL, 0, NULL)) {
			return FALSE;
		}
		niofd->__event_mask |= (NIO_OP_WRITE | NIO_OP_CONNECT);
	}
	else {
		errno = EINVAL;
		return FALSE;
	}

	if (!niofd->__reg) {
		niofd->__reg = 1;
		/* insert into alive list */
		nio_reg_alive_niofd(nio, niofd);
	}
	return TRUE;
#else
	return FALSE;
#endif
}

int nioWait(Nio_t* nio, NioEv_t* e, unsigned int count, int msec) {
#if defined(_WIN32) || defined(_WIN64)
	ULONG n;
	nio_handle_free_list(nio);
	if (GetQueuedCompletionStatusEx((HANDLE)(nio->__hNio), e, count, &n, msec, FALSE)) {
		return n;
	}
	if (GetLastError() == WAIT_TIMEOUT) {
		return 0;
	}
	return -1;
#elif __linux__
	int res;
	nio_handle_free_list(nio);
	do {
		res = epoll_wait(nio->__hNio, e, count, msec);
	} while (res < 0 && EINTR == errno);
	return res;
#elif defined(__FreeBSD__) || defined(__APPLE__)
	int res;
	struct timespec tval, *t = NULL;
	if (msec >= 0) {
		tval.tv_sec = msec / 1000;
		tval.tv_nsec = msec % 1000;
		tval.tv_nsec *= 1000000;
		t = &tval;
	}
	nio_handle_free_list(nio);
	do {
		res = kevent(nio->__hNio, NULL, 0, e, count, t);
	} while (res < 0 && EINTR == errno);
	return res;
#endif
}

void nioWakeup(Nio_t* nio) {
	if (0 == _xchg16(&nio->__wakeup, 1)) {
#if defined(_WIN32) || defined(_WIN64)
		PostQueuedCompletionStatus((HANDLE)nio->__hNio, 0, 0, NULL);
#else
		char c;
		write(nio->__socketpair[1], &c, sizeof(c));
#endif
	}
}

NioFD_t* nioEventCheck(Nio_t* nio, const NioEv_t* e, int* ev_mask) {
#if defined(_WIN32) || defined(_WIN64)
	NioFD_t* niofd;
	IoOverlapped_t* ol = (IoOverlapped_t*)(e->lpOverlapped);
	if (!ol) {
		_xchg16(&nio->__wakeup, 0);
		return NULL;
	}
	iocp_nio_unlink_ol(nio, ol);
	ol->commit = 0;
	if (ol->free_flag) {
		IoOverlapped_free(ol);
		return NULL;
	}

	niofd = (NioFD_t*)e->lpCompletionKey;
	if (niofd->__delete_flag) {
		return NULL;
	}

	if (IO_OVERLAPPED_OP_ACCEPT == ol->opcode) {
		*ev_mask = (NIO_OP_READ | NIO_OP_ACCEPT);
	}
	else if (IO_OVERLAPPED_OP_CONNECT == ol->opcode) {
		ol->opcode = IO_OVERLAPPED_OP_WRITE;
		*ev_mask = (NIO_OP_WRITE | NIO_OP_CONNECT);
	}
	else if (IO_OVERLAPPED_OP_READ == ol->opcode) {
		*ev_mask = NIO_OP_READ;
	}
	else if (IO_OVERLAPPED_OP_WRITE == ol->opcode) {
		*ev_mask = NIO_OP_WRITE;
	}
	else {
		*ev_mask = 0;
	}
	return niofd;
#elif __linux__
	NioFD_t* niofd;
	if (e->data.ptr == (void*)&nio->__socketpair[0]) {
		char c[256];
		read(nio->__socketpair[0], c, sizeof(c));
		_xchg16(&nio->__wakeup, 0);
		return NULL;
	}
	niofd = (NioFD_t*)e->data.ptr;
	if (niofd->__delete_flag) {
		return NULL;
	}

	*ev_mask = 0;
	/* epoll must catch this event */
	if ((e->events & EPOLLRDHUP) || (e->events & EPOLLHUP)) {
		if (niofd->__event_mask & NIO_OP_ACCEPT) {
			*ev_mask |= NIO_OP_ACCEPT;
		}
		*ev_mask |= NIO_OP_READ;
		niofd->__event_mask = 0;
		return niofd;
	}
	if (e->events & EPOLLIN) {
		if (niofd->__event_mask & NIO_OP_ACCEPT) {
			niofd->__event_mask &= ~NIO_OP_ACCEPT;
			*ev_mask |= NIO_OP_ACCEPT;
		}
		niofd->__event_mask &= ~NIO_OP_READ;
		*ev_mask |= NIO_OP_READ;
	}
	if (e->events & EPOLLOUT) {
		if (niofd->__event_mask & NIO_OP_CONNECT) {
			niofd->__event_mask &= ~NIO_OP_CONNECT;
			*ev_mask |= NIO_OP_CONNECT;
		}
		niofd->__event_mask &= ~NIO_OP_WRITE;
		*ev_mask |= NIO_OP_WRITE;
	}
	return niofd;
#elif defined(__FreeBSD__) || defined(__APPLE__)
	NioFD_t* niofd;
	if (e->udata == (void*)&nio->__socketpair[0]) {
		char c[256];
		read(nio->__socketpair[0], c, sizeof(c));
		_xchg16(&nio->__wakeup, 0);
		return NULL;
	}
	niofd = (NioFD_t*)e->udata;
	if (niofd->__delete_flag) {
		return NULL;
	}

	*ev_mask = 0;
	if (EVFILT_READ == e->filter) {
		if (niofd->__event_mask & NIO_OP_ACCEPT) {
			niofd->__event_mask &= ~NIO_OP_ACCEPT;
			*ev_mask |= NIO_OP_ACCEPT;
		}
		niofd->__event_mask &= ~NIO_OP_READ;
		*ev_mask |= NIO_OP_READ;
	}
	else if (EVFILT_WRITE == e->filter) {
		if (niofd->__event_mask & NIO_OP_CONNECT) {
			niofd->__event_mask &= ~NIO_OP_CONNECT;
			*ev_mask |= NIO_OP_CONNECT;
		}
		niofd->__event_mask &= ~NIO_OP_WRITE;
		*ev_mask |= NIO_OP_WRITE;
	}
	else { /* program don't run here... */
		return NULL;
	}
	return niofd;
#endif
	return NULL;
}

int nioConnectUpdate(NioFD_t* niofd) {
#if defined(_WIN32) || defined(_WIN64)
	return win32_OverlappedConnectUpdate(niofd->fd);
#else
	int err = 0;
	socklen_t len = sizeof(int);
	if (getsockopt(niofd->fd, SOL_SOCKET, SO_ERROR, (char*)&err, &len)) {
		return errno;
	}
	return err;
#endif
}

FD_t nioAccept(NioFD_t* niofd, struct sockaddr* peer_saddr, socklen_t* p_slen) {
#if defined(_WIN32) || defined(_WIN64)
	IocpAcceptExOverlapped_t* iocp_ol = (IocpAcceptExOverlapped_t*)niofd->__read_ol;
	if (!iocp_ol || INVALID_SOCKET == iocp_ol->acceptsocket) {
		return accept(niofd->fd, peer_saddr, p_slen);
	}
	return IoOverlapped_pop_acceptfd(&iocp_ol->base, peer_saddr, p_slen);
#else
	return accept(niofd->fd, peer_saddr, p_slen);
#endif
}

BOOL nioClose(Nio_t* nio) {
	nio_exit_clean_soft(nio);
#if defined(_WIN32) || defined(_WIN64)
	iocp_nio_exit_clean(nio);
	return CloseHandle((HANDLE)(nio->__hNio));
#else
	close(nio->__socketpair[0]);
	close(nio->__socketpair[1]);
	return close(nio->__hNio) == 0;
#endif
}

#ifdef	__cplusplus
}
#endif
