//
// Created by hujianzhe
//

#include "../../inc/sysapi/io.h"
#include "../../inc/sysapi/assert.h"
#if defined(_WIN32) || defined(_WIN64)
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

/* NIO */
BOOL nioCreate(Nio_t* nio, void(*fn_free_niofd)(NioFD_t*)) {
#if defined(_WIN32) || defined(_WIN64)
	SYSTEM_INFO si;
	GetSystemInfo(&si);
	nio->__wakeup = 0;
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

#if defined(_WIN32) || defined(_WIN64)
typedef struct IocpOverlapped {
	OVERLAPPED ol;
	unsigned char commit;
	unsigned char free;
	unsigned short opcode;
} IocpOverlapped;
typedef struct IocpReadOverlapped {
	IocpOverlapped base;
	struct sockaddr_storage saddr;
	int saddrlen;
	DWORD dwFlags;
	DWORD dwNumberOfBytesTransferred;
	WSABUF wsabuf;
	unsigned char append_data[1]; /* convienent for text data */
} IocpReadOverlapped;
typedef struct IocpWriteOverlapped {
	IocpOverlapped base;
	struct sockaddr_storage saddr;
	DWORD dwNumberOfBytesTransferred;
	WSABUF wsabuf;
	unsigned char append_data[1]; /* convienent for text data */
} IocpWriteOverlapped;
typedef struct IocpAcceptExOverlapped {
	IocpOverlapped base;
	SOCKET acceptsocket;
	unsigned char saddrs[sizeof(struct sockaddr_storage) + 16 + sizeof(struct sockaddr_storage) + 16];
} IocpAcceptExOverlapped;

static OVERLAPPED* Iocp_AllocOverlapped(int opcode, const void* refbuf, unsigned int refsize, unsigned int appendsize) {
	switch (opcode) {
		case NIO_OP_READ:
		{
			IocpReadOverlapped* ol = (IocpReadOverlapped*)malloc(sizeof(IocpReadOverlapped) + appendsize);
			if (ol) {
				memset(ol, 0, sizeof(IocpReadOverlapped));
				ol->base.opcode = NIO_OP_READ;
				ol->saddr.ss_family = AF_UNSPEC;
				ol->saddrlen = sizeof(ol->saddr);
				ol->dwFlags = 0;
				if (refbuf && refsize) {
					ol->wsabuf.buf = (char*)refbuf;
					ol->wsabuf.len = refsize;
				}
				else if (appendsize) {
					ol->wsabuf.buf = (char*)(ol->append_data);
					ol->wsabuf.len = appendsize;
					ol->append_data[appendsize] = 0;
				}
			}
			return &ol->base.ol;
		}
		case NIO_OP_CONNECT:
		case NIO_OP_WRITE:
		{
			IocpWriteOverlapped* ol = (IocpWriteOverlapped*)malloc(sizeof(IocpWriteOverlapped) + appendsize);
			if (ol) {
				memset(ol, 0, sizeof(IocpWriteOverlapped));
				ol->base.opcode = opcode;
				ol->saddr.ss_family = AF_UNSPEC;
				if (refbuf && refsize) {
					ol->wsabuf.buf = (char*)refbuf;
					ol->wsabuf.len = refsize;
				}
				else if (appendsize) {
					ol->wsabuf.buf = (char*)(ol->append_data);
					ol->wsabuf.len = appendsize;
					ol->append_data[appendsize] = 0;
				}
			}
			return &ol->base.ol;
		}
		case NIO_OP_ACCEPT:
		{
			IocpAcceptExOverlapped* ol = (IocpAcceptExOverlapped*)calloc(1, sizeof(IocpAcceptExOverlapped));
			if (ol) {
				ol->base.opcode = NIO_OP_ACCEPT;
				ol->acceptsocket = INVALID_SOCKET;
			}
			return &ol->base.ol;
		}
		default:
		{
			SetLastError(ERROR_INVALID_PARAMETER);
			return NULL;
		}
	}
}

static void Iocp_FreeOverlapped(OVERLAPPED* ol) {
	IocpOverlapped* iocp_ol = (IocpOverlapped*)ol;
	if (NIO_OP_ACCEPT == iocp_ol->opcode) {
		IocpAcceptExOverlapped* iocp_acceptex = (IocpAcceptExOverlapped*)iocp_ol;
		if (INVALID_SOCKET != iocp_acceptex->acceptsocket) {
			closesocket(iocp_acceptex->acceptsocket);
			iocp_acceptex->acceptsocket = INVALID_SOCKET;
		}
	}
	if (iocp_ol->commit) {
		iocp_ol->free = 1;
	}
	else {
		free(iocp_ol);
	}
}

static BOOL Iocp_PrepareRegUdp(SOCKET fd, int domain) {
	struct sockaddr_storage local_saddr;
	socklen_t slen;
	DWORD dwBytesReturned = 0;
	BOOL bNewBehavior = FALSE;
	/* winsock2 BUG, udp recvfrom WSAECONNRESET(10054) error and post Overlapped IO error */
	if (WSAIoctl(fd, SIO_UDP_CONNRESET, &bNewBehavior, sizeof(bNewBehavior), NULL, 0, &dwBytesReturned, NULL, NULL)) {
		return FALSE;
	}
	/* note: UDP socket need bind a address before call WSA function, otherwise WSAGetLastError return WSAINVALID */
	slen = sizeof(local_saddr);
	if (!getsockname(fd, (struct sockaddr*)&local_saddr, &slen)) {
		return TRUE;
	}
	if (WSAEINVAL != WSAGetLastError()) {
		return FALSE;
	}
	if (AF_INET == domain) {
		struct sockaddr_in* addr_in = (struct sockaddr_in*)&local_saddr;
		slen = sizeof(*addr_in);
		memset(addr_in, 0, sizeof(*addr_in));
		addr_in->sin_family = AF_INET;
		addr_in->sin_port = 0;
		addr_in->sin_addr.s_addr = htonl(INADDR_ANY);
	}
	else if (AF_INET6 == domain) {
		struct sockaddr_in6* addr_in6 = (struct sockaddr_in6*)&local_saddr;
		slen = sizeof(*addr_in6);
		memset(addr_in6, 0, sizeof(*addr_in6));
		addr_in6->sin6_family = AF_INET6;
		addr_in6->sin6_port = 0;
		addr_in6->sin6_addr = in6addr_any;
	}
	else {
		WSASetLastError(WSAEAFNOSUPPORT);
		return FALSE;
	}
	if (bind(fd, (struct sockaddr*)&local_saddr, slen)) {
		return FALSE;
	}
	return TRUE;
}
#endif

void niofdInit(NioFD_t* niofd, FD_t fd, int domain) {
	niofd->fd = fd;
	niofd->__lnext = NULL;
	niofd->__delete_flag = 0;
#if defined(_WIN32) || defined(_WIN64)
	niofd->__reg = FALSE;
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
		Iocp_FreeOverlapped(niofd->__read_ol);
		niofd->__read_ol = NULL;
	}
	if (niofd->__write_ol) {
		Iocp_FreeOverlapped(niofd->__write_ol);
		niofd->__write_ol = NULL;
	}
#else
	close(niofd->fd);
	niofd->fd = -1;
#endif
	niofd->__lnext = nio->__free_list_head;
	nio->__free_list_head = niofd;
	niofd->__delete_flag = 1;
}

BOOL nioCommit(Nio_t* nio, NioFD_t* niofd, int opcode, const struct sockaddr* saddr, socklen_t addrlen) {
#if defined(_WIN32) || defined(_WIN64)
	int fd_domain = niofd->__domain;
	FD_t fd = niofd->fd;
	if (!niofd->__reg) {
		if (AF_UNSPEC != fd_domain) {
			int socktype;
			int optlen = sizeof(socktype);
			if (getsockopt((SOCKET)fd, SOL_SOCKET, SO_TYPE, (char*)&socktype, &optlen)) {
				return FALSE;
			}
			if (SOCK_DGRAM == socktype) {
				if (!Iocp_PrepareRegUdp(fd, fd_domain)) {
					return FALSE;
				}
			}
		}
		if (CreateIoCompletionPort((HANDLE)fd, (HANDLE)(nio->__hNio), (ULONG_PTR)niofd, 0) != (HANDLE)(nio->__hNio)) {
			return FALSE;
		}
		niofd->__reg = TRUE;
	}

	if (NIO_OP_READ == opcode) {
		IocpReadOverlapped* read_ol;
		if (!niofd->__read_ol) {
			niofd->__read_ol = Iocp_AllocOverlapped(opcode, NULL, 0, 0);
			if (!niofd->__read_ol) {
				return FALSE;
			}
		}
		read_ol = (IocpReadOverlapped*)niofd->__read_ol;
		if (read_ol->base.commit) {
			return TRUE;
		}

		if (fd_domain != AF_UNSPEC) {
			read_ol->saddrlen = sizeof(read_ol->saddr);
			if (read_ol->wsabuf.buf && read_ol->wsabuf.len) {
				read_ol->dwFlags = 0;
			}
			else {
				read_ol->dwFlags = MSG_PEEK;
			}
			if (!WSARecvFrom((SOCKET)fd, &read_ol->wsabuf, 1, NULL, &read_ol->dwFlags, (struct sockaddr*)&read_ol->saddr, &read_ol->saddrlen, (LPWSAOVERLAPPED)&read_ol->base.ol, NULL)) {
				read_ol->base.commit = 1;
				return TRUE;
			}
			else if (WSAGetLastError() == WSA_IO_PENDING) {
				read_ol->base.commit = 1;
				return TRUE;
			}
			/* note: UDP socket need bind a address before call this function, otherwise WSAGetLastError return WSAINVALID */
		}
		else {
			if (ReadFile((HANDLE)fd, read_ol->wsabuf.buf, read_ol->wsabuf.len, NULL, (LPOVERLAPPED)&read_ol->base.ol)) {
				read_ol->base.commit = 1;
				return TRUE;
			}
			else if (GetLastError() == ERROR_IO_PENDING) {
				read_ol->base.commit = 1;
				return TRUE;
			}
		}
		return FALSE;
	}
	else if (NIO_OP_WRITE == opcode) {
		IocpWriteOverlapped* write_ol;
		if (!niofd->__write_ol) {
			niofd->__write_ol = Iocp_AllocOverlapped(opcode, NULL, 0, 0);
			if (!niofd->__write_ol) {
				return FALSE;
			}
		}
		write_ol = (IocpWriteOverlapped*)niofd->__write_ol;
		if (write_ol->base.commit) {
			return TRUE;
		}

		if (fd_domain != AF_UNSPEC) {
			const struct sockaddr* toaddr;
			if (addrlen > 0 && saddr) {
				toaddr = (const struct sockaddr*)&write_ol->saddr;
				memmove(&write_ol->saddr, saddr, addrlen);
			}
			else {
				toaddr = NULL;
				addrlen = 0;
			}
			if (!WSASendTo((SOCKET)fd, &write_ol->wsabuf, 1, NULL, 0, toaddr, addrlen, (LPWSAOVERLAPPED)&write_ol->base.ol, NULL)) {
				write_ol->base.commit = 1;
				return TRUE;
			}
			else if (WSAGetLastError() == WSA_IO_PENDING) {
				write_ol->base.commit = 1;
				return TRUE;
			}
		}
		else {
			if (WriteFile((HANDLE)fd, write_ol->wsabuf.buf, write_ol->wsabuf.len, NULL, (LPOVERLAPPED)&write_ol->base.ol)) {
				write_ol->base.commit = 1;
				return TRUE;
			}
			else if (GetLastError() == ERROR_IO_PENDING) {
				write_ol->base.commit = 1;
				return TRUE;
			}
		}
		return FALSE;
	}
	else if (NIO_OP_ACCEPT == opcode) {
		static LPFN_ACCEPTEX lpfnAcceptEx = NULL;
		IocpAcceptExOverlapped* accept_ol;
		if (!niofd->__read_ol) {
			niofd->__read_ol = Iocp_AllocOverlapped(opcode, NULL, 0, 0);
			if (!niofd->__read_ol) {
				return FALSE;
			}
		}
		accept_ol = (IocpAcceptExOverlapped*)niofd->__read_ol;
		if (accept_ol->base.commit) {
			return TRUE;
		}

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
		if (lpfnAcceptEx((SOCKET)fd, accept_ol->acceptsocket, accept_ol->saddrs, 0,
			sizeof(struct sockaddr_storage) + 16, sizeof(struct sockaddr_storage) + 16,
			NULL, (LPOVERLAPPED)&accept_ol->base.ol))
		{
			accept_ol->base.commit = 1;
			return TRUE;
		}
		else if (WSAGetLastError() == ERROR_IO_PENDING) {
			accept_ol->base.commit = 1;
			return TRUE;
		}
		else {
			closesocket(accept_ol->acceptsocket);
			accept_ol->acceptsocket = INVALID_SOCKET;
			return FALSE;
		}
	}
	else if (NIO_OP_CONNECT == opcode) {
		static LPFN_CONNECTEX lpfnConnectEx = NULL;
		struct sockaddr_storage st;
		IocpWriteOverlapped* conn_ol;
		if (!niofd->__write_ol) {
			niofd->__write_ol = Iocp_AllocOverlapped(opcode, NULL, 0, 0);
			if (!niofd->__write_ol) {
				return FALSE;
			}
		}
		conn_ol = (IocpWriteOverlapped*)niofd->__write_ol;
		if (conn_ol->base.commit) {
			return TRUE;
		}
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
		if (lpfnConnectEx((SOCKET)fd, (const struct sockaddr*)&conn_ol->saddr, addrlen, conn_ol->wsabuf.buf, conn_ol->wsabuf.len, NULL, (LPWSAOVERLAPPED)&conn_ol->base.ol)) {
			conn_ol->base.commit = 1;
			return TRUE;
		}
		else if (WSAGetLastError() == ERROR_IO_PENDING) {
			conn_ol->base.commit = 1;
			return TRUE;
		}
		return FALSE;
	}
	else {
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}
#elif defined(__linux__)
	struct epoll_event e;
	unsigned int event_mask = niofd->__event_mask;
	unsigned int sys_event_mask = 0;
	if (event_mask & NIO_OP_READ) {
		sys_event_mask |= EPOLLIN;
	}
	if (event_mask & NIO_OP_WRITE) {
		sys_event_mask |= EPOLLOUT;
	}

	if (NIO_OP_READ == opcode || NIO_OP_ACCEPT == opcode) {
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
	else if (NIO_OP_CONNECT == opcode) {
		if (event_mask & NIO_OP_WRITE) {
			return TRUE;
		}
		if (connect(niofd->fd, saddr, addrlen) && EINPROGRESS != errno) {
			return FALSE;
		}
		event_mask |= NIO_OP_WRITE;
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
	return TRUE;
#elif defined(__FreeBSD__) || defined(__APPLE__)
	struct kevent e;
	if (NIO_OP_READ == opcode || NIO_OP_ACCEPT == opcode) {
		if (niofd->__event_mask & NIO_OP_READ) {
			return TRUE;
		}
		EV_SET(&e, (uintptr_t)niofd->fd, EVFILT_READ, EV_ADD | EV_ONESHOT, 0, 0, niofd);
		if (kevent(nio->__hNio, &e, 1, NULL, 0, NULL)) {
			return FALSE;
		}
		niofd->__event_mask |= NIO_OP_READ;
		return TRUE;
	}
	if (NIO_OP_WRITE == opcode) {
		if (niofd->__event_mask & NIO_OP_WRITE) {
			return TRUE;
		}
		EV_SET(&e, (uintptr_t)niofd->fd, EVFILT_WRITE, EV_ADD | EV_ONESHOT, 0, 0, niofd);
		if (kevent(nio->__hNio, &e, 1, NULL, 0, NULL)) {
			return FALSE;
		}
		niofd->__event_mask |= NIO_OP_WRITE;
		return TRUE;
	}
	if (NIO_OP_CONNECT == opcode) {
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
		niofd->__event_mask |= NIO_OP_WRITE;
		return TRUE;
	}
	errno = EINVAL;
	return FALSE;
#endif
	return FALSE;
}

static void nio_handle_free_list(Nio_t* nio) {
	NioFD_t* cur_free, * next_free;
	for (cur_free = nio->__free_list_head; cur_free; cur_free = next_free) {
		next_free = cur_free->__lnext;
		nio->__fn_free_niofd(cur_free);
	}
	nio->__free_list_head = NULL;
}

int nioWait(Nio_t* nio, NioEv_t* e, unsigned int count, int msec) {
#if defined(_WIN32) || defined(_WIN64)
	ULONG n;
	nio_handle_free_list(nio);
	return GetQueuedCompletionStatusEx((HANDLE)(nio->__hNio), e, count, &n, msec, FALSE) ? n : (GetLastError() == WAIT_TIMEOUT ? 0 : -1);
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
	if (0 == _cmpxchg16(&nio->__wakeup, 1, 0)) {
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
	IocpOverlapped* iocp_ol = (IocpOverlapped*)(e->lpOverlapped);
	if (!iocp_ol) {
		_xchg16(&nio->__wakeup, 0);
		return NULL;
	}
	if (iocp_ol->free) {
		free(iocp_ol);
		return NULL;
	}
	iocp_ol->commit = 0;

	niofd = (NioFD_t*)e->lpCompletionKey;
	if (niofd->__delete_flag) {
		return NULL;
	}

	if (NIO_OP_ACCEPT == iocp_ol->opcode) {
		*ev_mask = NIO_OP_READ;
	}
	else if (NIO_OP_CONNECT == iocp_ol->opcode) {
		iocp_ol->opcode = NIO_OP_WRITE;
		*ev_mask = NIO_OP_WRITE;
	}
	else if (NIO_OP_READ == iocp_ol->opcode) {
		IocpReadOverlapped* iocp_read_ol = (IocpReadOverlapped*)iocp_ol;
		iocp_read_ol->dwNumberOfBytesTransferred = e->dwNumberOfBytesTransferred;
		*ev_mask = NIO_OP_READ;
	}
	else if (NIO_OP_WRITE == iocp_ol->opcode) {
		IocpWriteOverlapped* iocp_write_ol = (IocpWriteOverlapped*)iocp_ol;
		iocp_write_ol->dwNumberOfBytesTransferred = e->dwNumberOfBytesTransferred;
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
	/* epoll must catch this event */
	if ((e->events & EPOLLRDHUP) || (e->events & EPOLLHUP)) {
		niofd->__event_mask = 0;
		*ev_mask = NIO_OP_READ;
	}
	else {
		*ev_mask = 0;
		if (e->events & EPOLLIN) {
			if (niofd->__event_mask & NIO_OP_READ) {
				niofd->__event_mask &= ~NIO_OP_READ;
				*ev_mask |= NIO_OP_READ;
			}
		}
		if (e->events & EPOLLOUT) {
			if (niofd->__event_mask & NIO_OP_WRITE) {
				niofd->__event_mask &= ~NIO_OP_WRITE;
				*ev_mask |= NIO_OP_WRITE;
			}
		}
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
	if (EVFILT_READ == e->filter) {
		niofd->__event_mask &= ~NIO_OP_READ;
		*ev_mask = NIO_OP_READ;
	}
	else if (EVFILT_WRITE == e->filter) {
		niofd->__event_mask &= ~NIO_OP_WRITE;
		*ev_mask = NIO_OP_WRITE;
	}
	else { /* program don't run here... */
		return NULL;
	}
	return niofd;
#endif
	return NULL;
}

BOOL nioConnectCheckSuccess(FD_t sockfd) {
#if defined(_WIN32) || defined(_WIN64)
	int sec;
	int len = sizeof(sec);
	if (getsockopt(sockfd, SOL_SOCKET, SO_CONNECT_TIME, (char*)&sec, &len)) {
		return FALSE;
	}
	if (~0 == sec) {
		SetLastError(ERROR_TIMEOUT);
		return FALSE;
	}
	return !setsockopt(sockfd, SOL_SOCKET, SO_UPDATE_CONNECT_CONTEXT, NULL, 0);
#else
	int error;
	socklen_t len = sizeof(int);
	if (getsockopt(sockfd, SOL_SOCKET, SO_ERROR, (char*)&error, &len)) {
		return FALSE;
	}
	if (error) {
		errno = error;
	}
	return !error;
#endif
}

FD_t nioAccept(NioFD_t* niofd, struct sockaddr* peer_saddr, socklen_t* p_slen) {
#if defined(_WIN32) || defined(_WIN64)
	SOCKET acceptfd;
	IocpAcceptExOverlapped* iocp_ol = (IocpAcceptExOverlapped*)niofd->__read_ol;
	if (!iocp_ol || INVALID_SOCKET == iocp_ol->acceptsocket) {
		return accept(niofd->fd, peer_saddr, p_slen);
	}
	acceptfd = iocp_ol->acceptsocket;
	iocp_ol->acceptsocket = INVALID_SOCKET;
	if (setsockopt(acceptfd, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, (char*)&niofd->fd, sizeof(niofd->fd))) {
		closesocket(acceptfd);
		return INVALID_FD_HANDLE;
	}
	if (!peer_saddr || !p_slen) {
		return acceptfd;
	}
	if (getpeername(acceptfd, peer_saddr, p_slen)) {
		closesocket(acceptfd);
		return INVALID_FD_HANDLE;
	}
	return acceptfd;
#else
	return accept(niofd->fd, peer_saddr, p_slen);
#endif
}

BOOL nioClose(Nio_t* nio) {
	nio_handle_free_list(nio);
#if defined(_WIN32) || defined(_WIN64)
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
