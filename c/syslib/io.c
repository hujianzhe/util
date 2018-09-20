//
// Created by hujianzhe
//

#include "io.h"
#include "assert.h"
#if defined(_WIN32) || defined(_WIN64)
	#include <mswsock.h>
	#include <ws2ipdef.h>
	#include <stdlib.h>
#else
	#include <arpa/inet.h>
#endif
#include <errno.h>
#include <string.h>

#ifdef	__cplusplus
extern "C" {
#endif

/* aiocb */
void aioInit(struct aiocb* cb, size_t udata) {
	memset(cb, 0, sizeof(*cb));
	cb->aio_lio_opcode = LIO_NOP;
	cb->aio_fildes = INVALID_FD_HANDLE;
	cb->aio_sigevent.sigev_value.sival_ptr = (void*)udata;
#if !defined(_WIN32) && !defined(_WIN64)
	cb->aio_sigevent.sigev_notify = SIGEV_NONE;
#endif
}

void aioSetOffset(struct aiocb* cb, long long offset) {
	cb->aio_offset = offset;
#if defined(_WIN32) || defined(_WIN64)
	if (LIO_READ == cb->aio_lio_opcode) {
		cb->in.__ol.Offset = cb->aio_offset;
		cb->in.__ol.OffsetHigh = cb->aio_offset >> 32;
	}
	else if (LIO_WRITE == cb->aio_lio_opcode) {
		cb->out.__ol.Offset = cb->aio_offset;
		cb->out.__ol.OffsetHigh = cb->aio_offset >> 32;
	}
#endif
}

BOOL aioCommit(struct aiocb* cb) {
	if (LIO_READ == cb->aio_lio_opcode) {
#if defined(_WIN32) || defined(_WIN64)
		return ReadFileEx((HANDLE)(cb->aio_fildes),
								cb->aio_buf, cb->aio_nbytes,
								&cb->in.__ol, NULL) ||
								GetLastError() == ERROR_IO_PENDING;
#else
		return aio_read(cb) == 0;
#endif
	}
	else if (LIO_WRITE == cb->aio_lio_opcode) {
#if defined(_WIN32) || defined(_WIN64)
		return WriteFileEx((HANDLE)(cb->aio_fildes),
								cb->aio_buf, cb->aio_nbytes,
								&cb->out.__ol, NULL) ||
								GetLastError() == ERROR_IO_PENDING;
#else
		return aio_write(cb) == 0;
#endif
	}
	return TRUE;
}

BOOL aioHasCompleted(const struct aiocb* cb) {
#if defined(_WIN32) || defined(_WIN64)
	if (LIO_READ == cb->aio_lio_opcode) {
		return HasOverlappedIoCompleted(&cb->in.__ol);
	}
	else if (LIO_WRITE == cb->aio_lio_opcode) {
		return HasOverlappedIoCompleted(&cb->out.__ol);
	}
	return TRUE;
#else
	return aio_error(cb) != EINPROGRESS;
#endif
}

BOOL aioSuspend(const struct aiocb* const cb_list[], int nent, int msec) {
#if defined(_WIN32) || defined(_WIN64)
	DWORD i;
	DWORD dwCount = nent > MAXIMUM_WAIT_OBJECTS ? MAXIMUM_WAIT_OBJECTS : nent;
	HANDLE hObjects[MAXIMUM_WAIT_OBJECTS];
	for (i = 0; i < dwCount; ++i) {
		hObjects[i] = (HANDLE)(cb_list[i]->aio_fildes);
	}
	i = WaitForMultipleObjects(dwCount, hObjects, FALSE, msec);
	return i >= WAIT_OBJECT_0 && i < WAIT_OBJECT_0 + MAXIMUM_WAIT_OBJECTS;
#else
	struct timespec tval, *t = NULL;
	if (msec >= 0) {
		tval.tv_sec = msec / 1000;
		tval.tv_nsec = msec % 1000;
		t = &tval;
	}
	return 0 == aio_suspend(cb_list, nent, t);
#endif
}

BOOL aioCancel(FD_t fd, struct aiocb* cb) {
#if defined(_WIN32) || defined(_WIN64)
	OVERLAPPED* p_ol = NULL;
	if (LIO_READ == cb->aio_lio_opcode) {
		p_ol = &cb->in.__ol;
	}
	else if (LIO_WRITE == cb->aio_lio_opcode) {
		p_ol = &cb->out.__ol;
	}
	else {
		return FALSE;
	}
	return CancelIoEx((HANDLE)fd, p_ol);
#else
	int res = aio_cancel(fd, cb);
	if (AIO_CANCELED == res || AIO_ALLDONE == res) {
		return TRUE;
	}
	else if (AIO_NOTCANCELED == res) {
		errno = EINPROGRESS;
	}
	return FALSE;
#endif
}

int aioResult(struct aiocb* cb, unsigned int* transfer_bytes) {
#if defined(_WIN32) || defined(_WIN64)
	OVERLAPPED* p_ol = NULL;
	if (LIO_READ == cb->aio_lio_opcode) {
		p_ol = &cb->in.__ol;
	}
	else if (LIO_WRITE == cb->aio_lio_opcode) {
		p_ol = &cb->out.__ol;
	}
	else {
		return EINVAL;/*return ERROR_INVALID_PARAMETER;*/
	}
	*transfer_bytes = p_ol->InternalHigh;
	return p_ol->Internal;
#else
	ssize_t num = aio_return(cb);
	if (num >= 0) {
		*transfer_bytes = num;
	}
	return aio_error(cb);
#endif
}

/* NIO */
BOOL reactorCreate(Reactor_t* reactor) {
#if defined(_WIN32) || defined(_WIN64)
	SYSTEM_INFO si;
	GetSystemInfo(&si);
	reactor->__hNio = (FD_t)CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, si.dwNumberOfProcessors << 1);
	return reactor->__hNio != 0;
#elif __linux__
	int nio_ok = 0, epfd_ok = 0;
	do {
		struct epoll_event e = { 0 };
		reactor->__hNio = epoll_create1(EPOLL_CLOEXEC);
		if (reactor->__hNio < 0) {
			break;
		}
		nio_ok = 1;
		reactor->__epfd = epoll_create1(EPOLL_CLOEXEC);
		if (reactor->__epfd < 0) {
			break;
		}
		epfd_ok = 1;
		e.data.fd = reactor->__epfd;
		e.events = EPOLLIN;
		if (epoll_ctl(reactor->__hNio, EPOLL_CTL_ADD, reactor->__epfd, &e)) {
			break;
		}
		return TRUE;
	} while (0);
	if (nio_ok) {
		assertTRUE(0 == close(reactor->__hNio));
	}
	if (epfd_ok) {
		assertTRUE(0 == close(reactor->__epfd));
	}
	return FALSE;
#elif defined(__FreeBSD__) || defined(__APPLE__)
	return !((reactor->__hNio = kqueue()) < 0);
#endif
}

BOOL reactorReg(Reactor_t* reactor, FD_t fd) {
#if defined(_WIN32) || defined(_WIN64)
	return CreateIoCompletionPort((HANDLE)fd, (HANDLE)(reactor->__hNio), (ULONG_PTR)fd, 0) == (HANDLE)(reactor->__hNio);
#endif
	return TRUE;
}

BOOL reactorCancel(Reactor_t* reactor, FD_t fd) {
#if defined(_WIN32) || defined(_WIN64)
	return CancelIoEx((HANDLE)fd, NULL);
	/*
	 * iocp will catch this return and set overlapped.internal a magic number, but header not include that macro
	 */
#elif __linux__
	struct epoll_event e = { 0 };
	if (epoll_ctl(reactor->__hNio, EPOLL_CTL_DEL, fd, &e)) {
		return FALSE;
	}
	if (epoll_ctl(reactor->__epfd, EPOLL_CTL_DEL, fd, &e)) {
		return FALSE;
	}
	return TRUE;
#elif defined(__FreeBSD__) || defined(__APPLE__)
	struct kevent e[2];
	EV_SET(&e[0], (uintptr_t)fd, EVFILT_READ, EV_DISABLE, 0, 0, NULL);
	EV_SET(&e[1], (uintptr_t)fd, EVFILT_WRITE, EV_DISABLE, 0, 0, NULL);
	return kevent(reactor->__hNio, e, 2, NULL, 0, NULL) == 0;
#endif
}

void* reactorMallocOverlapped(int opcode) {
#if defined(_WIN32) || defined(_WIN64)
	switch (opcode) {
		case REACTOR_READ:
		case REACTOR_WRITE:
		case REACTOR_CONNECT:
		{
			return calloc(1, sizeof(OVERLAPPED) + 2);
		}
		case REACTOR_ACCEPT:
		{
			char* ol = (char*)calloc(1, sizeof(OVERLAPPED) + 2 + sizeof(SOCKET) + (sizeof(struct sockaddr_storage) + 16) * 2);
			if (ol) {
				ol[sizeof(OVERLAPPED) + 1] = 1;
				*(SOCKET*)(ol + sizeof(OVERLAPPED) + 2) = INVALID_SOCKET;
			}
			return ol;
		}
		default:
			SetLastError(ERROR_INVALID_PARAMETER);
			return NULL;
	}
#else
	return (void*)(size_t)-1;
#endif
}
void reactorFreeOverlapped(void* ol) {
#if defined(_WIN32) || defined(_WIN64)
	const char* p = (char*)ol;
	if (p && p[sizeof(OVERLAPPED) + 1]) {
		SOCKET s = *(SOCKET*)(p + sizeof(OVERLAPPED) + 2);
		if (INVALID_SOCKET != s)
			closesocket(s);
	}
	free(ol);
#endif
}

BOOL reactorCommit(Reactor_t* reactor, FD_t fd, int opcode, void* ol, struct sockaddr_storage* saddr) {
#if defined(_WIN32) || defined(_WIN64)
	if (REACTOR_READ == opcode) {
		if (saddr) {
			int slen = sizeof(*saddr);/* connectionless socket must set this param */
			DWORD Flags = 0;
			WSABUF wsabuf = { 0 };
			return !WSARecvFrom((SOCKET)fd, &wsabuf, 1, NULL, &Flags, (struct sockaddr*)saddr, &slen, (LPWSAOVERLAPPED)ol, NULL) ||
					WSAGetLastError() == WSA_IO_PENDING;
		}
		else {
			return ReadFileEx((HANDLE)fd, NULL, 0, (LPOVERLAPPED)ol, NULL) || GetLastError() == ERROR_IO_PENDING;
		}
	}
	else if (REACTOR_WRITE == opcode) {
		if (saddr) {
			WSABUF wsabuf = { 0 };
			return	!WSASendTo((SOCKET)fd, &wsabuf, 1, NULL, 0, (struct sockaddr*)saddr, sizeof(*saddr), (LPWSAOVERLAPPED)(((char*)ol) + 1), NULL) ||
					WSAGetLastError() == WSA_IO_PENDING;
		}
		else {
			return WriteFileEx((HANDLE)fd, NULL, 0, (LPOVERLAPPED)(((char*)ol) + 1), NULL) || GetLastError() == ERROR_IO_PENDING;
		}
	}
	else if (REACTOR_ACCEPT == opcode) {
		BOOL res;
		SOCKET* pConnfd;
		static LPFN_ACCEPTEX lpfnAcceptEx = NULL;
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
		pConnfd = (SOCKET*)(((char*)ol) + sizeof(OVERLAPPED) + 2);
		if (INVALID_SOCKET == *pConnfd) {
			*pConnfd = socket(saddr->ss_family, SOCK_STREAM, 0);
			if (INVALID_SOCKET == *pConnfd) {
				return FALSE;
			}
		}
		res = lpfnAcceptEx((SOCKET)fd, *pConnfd,
							pConnfd + 1, 0,
							sizeof(struct sockaddr_storage) + 16,
							sizeof(struct sockaddr_storage) + 16,
							NULL, (LPOVERLAPPED)ol) ||
							WSAGetLastError() == ERROR_IO_PENDING;
		if (!res) {
			closesocket(*pConnfd);
			*pConnfd = INVALID_SOCKET;
		}

		return res;
	}
	else if (REACTOR_CONNECT == opcode) {
		BOOL res;
		struct sockaddr_storage _sa = {0};
		static LPFN_CONNECTEX lpfnConnectEx = NULL;
		int slen;
		/* ConnectEx must get really namelen, otherwise report WSAEADDRNOTAVAIL(10049) */
		switch (saddr->ss_family) {
			case AF_INET:
				slen = sizeof(struct sockaddr_in);
				break;
			case AF_INET6:
				slen = sizeof(struct sockaddr_in6);
				break;
			default:
				SetLastError(WSAEAFNOSUPPORT);
				return FALSE;
		}
		if (!lpfnConnectEx){
			DWORD dwBytes;
			GUID GuidConnectEx = WSAID_CONNECTEX;
			if (WSAIoctl((SOCKET)fd, SIO_GET_EXTENSION_FUNCTION_POINTER,
				&GuidConnectEx, sizeof(GuidConnectEx),
				&lpfnConnectEx, sizeof(lpfnConnectEx),
				&dwBytes, NULL, NULL) == SOCKET_ERROR || !lpfnConnectEx) {
				return FALSE;
			}
		}
		_sa.ss_family = saddr->ss_family;
		if (bind((SOCKET)fd, (struct sockaddr*)&_sa, sizeof(_sa))) {
			return FALSE;
		}
		res = lpfnConnectEx((SOCKET)fd,
							(struct sockaddr*)saddr, slen,
							NULL, 0, NULL,
							(LPWSAOVERLAPPED)(((char*)ol) + 1)) ||
							WSAGetLastError() == ERROR_IO_PENDING;
		return res;
	}
	else {
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}
#else
	if (REACTOR_READ == opcode || REACTOR_ACCEPT == opcode) {
	#ifdef __linux__
		struct epoll_event e;
		e.data.fd = fd;
		e.events = EPOLLET | EPOLLONESHOT | EPOLLIN;
		if (epoll_ctl(reactor->__hNio, EPOLL_CTL_MOD, fd, &e)) {
			if (ENOENT != errno) {
				return FALSE;
			}
			return epoll_ctl(reactor->__hNio, EPOLL_CTL_ADD, fd, &e) == 0 || EEXIST == errno;
		}
		return TRUE;
	#elif defined(__FreeBSD__) || defined(__APPLE__)
		struct kevent e;
		EV_SET(&e, (uintptr_t)fd, EVFILT_READ, EV_ADD | EV_ONESHOT, 0, 0, (void*)(size_t)fd);
		return kevent(reactor->__hNio, &e, 1, NULL, 0, NULL) == 0;
	#endif
	}
	else if (REACTOR_WRITE == opcode || REACTOR_CONNECT == opcode) {
		if (REACTOR_CONNECT == opcode) {/* try connect... */
			int socklen;
			if (AF_INET == saddr->ss_family)
				socklen = sizeof(struct sockaddr_in);
			else if (AF_INET6 == saddr->ss_family)
				socklen = sizeof(struct sockaddr_in6);
			else {
				errno = EAFNOSUPPORT;
				return FALSE;
			}
			if (connect(fd, (struct sockaddr*)saddr, socklen) && EINPROGRESS != errno) {
				return FALSE;
			}
			/* 
			 * fd always add __epfd when connect immediately finish or not...
			 * because I need Unified handle event
			 */
		}
	#ifdef __linux__
		struct epoll_event e;
		e.data.fd = fd | 0x80000000;
		e.events = EPOLLET | EPOLLONESHOT | EPOLLOUT;
		if (epoll_ctl(reactor->__epfd, EPOLL_CTL_MOD, fd, &e)) {
			if (ENOENT != errno) {
				return FALSE;
			}
			return epoll_ctl(reactor->__epfd, EPOLL_CTL_ADD, fd, &e) == 0 || EEXIST == errno;
		}
		return TRUE;
	#elif defined(__FreeBSD__) || defined(__APPLE__)
		struct kevent e;
		EV_SET(&e, (uintptr_t)fd, EVFILT_WRITE, EV_ADD | EV_ONESHOT, 0, 0, (void*)(size_t)fd);
		return kevent(reactor->__hNio, &e, 1, NULL, 0, NULL) == 0;
	#endif
	}
	else {
		errno = EINVAL;
		return FALSE;
	}
#endif
	return FALSE;
}

int reactorWait(Reactor_t* reactor, NioEv_t* e, unsigned int count, int msec) {
#if defined(_WIN32) || defined(_WIN64)
	ULONG n;
	return GetQueuedCompletionStatusEx((HANDLE)(reactor->__hNio), e, count, &n, msec, FALSE) ? n : (GetLastError() == WAIT_TIMEOUT ? 0 : -1);
#elif __linux__
	int res;
	do {
		res = epoll_wait(reactor->__hNio, e, count / 2 + 1, msec);
	} while (res < 0 && EINTR == errno);
	if (res > 0) {
		int i;
		for (i = 0; i < res && reactor->__epfd != e[i].data.fd; ++i);
		if (i < res) {/* has EPOLLOUT */
			int out_res;
			e[i] = e[--res];
			do {
				out_res = epoll_wait(reactor->__epfd, e + res, count - res, 0);
			} while (out_res < 0 && EINTR == errno);
			if (out_res > 0) {
				res += out_res;
			}
		}
	}
	return res;
#elif defined(__FreeBSD__) || defined(__APPLE__)
	int res;
	struct timespec tval, *t = NULL;
	if (msec >= 0) {
		tval.tv_sec = msec / 1000;
		tval.tv_nsec = msec % 1000;
		t = &tval;
	}
	do {
		res = kevent(reactor->__hNio, NULL, 0, e, count, t);
	} while (res < 0 && EINTR == errno);
	return res;
#endif
}

void reactorResult(const NioEv_t* e, FD_t* p_fd, int* p_event, void** p_ol) {
#if defined(_WIN32) || defined(_WIN64)
	*p_ol = (void*)(((size_t)(e->lpOverlapped)) & ~1);
	*p_fd = e->lpCompletionKey;
	*p_event = ((ULONG_PTR)(e->lpOverlapped) & 1) ? REACTOR_WRITE : REACTOR_READ;
#elif __linux__
	*p_ol = NULL;
	*p_fd = e->data.fd & 0x7fffffff;
	if ((e->events & EPOLLRDHUP) || (e->events & EPOLLHUP)) {/* epoll must catch this event */
		*p_event = (e->data.fd & 0x80000000) ? REACTOR_WRITE : REACTOR_READ;
	}
	else {
		*p_event = REACTOR_NOP;
		/* we use __epfd to filter EPOLLOUT event */
		if (e->events & EPOLLIN) {
			*p_event = REACTOR_READ;
		}
		else if (e->events & EPOLLOUT) {
			*p_event = REACTOR_WRITE;
		}
	}
#elif defined(__FreeBSD__) || defined(__APPLE__)
	*p_ol = NULL;
	*p_fd = (size_t)(e->udata);
	if (EVFILT_READ == e->filter)
		*p_event = REACTOR_READ;
	else if (EVFILT_WRITE == e->filter)
		*p_event = REACTOR_WRITE;
	else /* program don't run here... */
		*p_event = REACTOR_NOP;
#endif
}

BOOL reactorConnectCheckSuccess(FD_t sockfd) {
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

#if defined(_WIN32) || defined(_WIN64)
static BOOL __win32AcceptPretreatment(FD_t listenfd, void* ol, void(*callback)(FD_t, struct sockaddr_storage*, void*), void* arg) {
	SOCKET* pConnfd = (SOCKET*)(((char*)ol) + sizeof(OVERLAPPED) + 2);
	do {
		SOCKET connfd = *pConnfd;
		struct sockaddr_storage saddr;
		int slen = sizeof(saddr);
		/**/
		if (getsockname(listenfd, (struct sockaddr*)&saddr, &slen)) {
			break;
		}
		*pConnfd = socket(saddr.ss_family, SOCK_STREAM, 0);
		if (INVALID_SOCKET == *pConnfd) {
			closesocket(connfd);
			*pConnfd = socket(saddr.ss_family, SOCK_STREAM, 0);
			break;
		}
		/**/
		if (setsockopt(connfd, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, (char*)&listenfd, sizeof(listenfd))) {
			closesocket(connfd);
			break;
		}
		if (callback) {
			slen = sizeof(saddr);
			if (getpeername(connfd, (struct sockaddr*)&saddr, &slen)) {
				closesocket(connfd);
				break;
			}
			callback(connfd, &saddr, arg);
		}
	} while (0);
	return *pConnfd != INVALID_SOCKET;
}
#endif

BOOL reactorAccept(FD_t listenfd, void* ol, void(*callback)(FD_t, struct sockaddr_storage*, void*), void* arg) {
	FD_t connfd;
	struct sockaddr_storage saddr;
#if defined(_WIN32) || defined(_WIN64)
	int slen = sizeof(saddr);
	if (!__win32AcceptPretreatment(listenfd, ol, callback, arg)) {
		return FALSE;
	}
#else
	socklen_t slen = sizeof(saddr);
#endif
	while ((connfd = accept(listenfd, (struct sockaddr*)&saddr, &slen)) != INVALID_FD_HANDLE) {
		slen = sizeof(saddr);
		if (callback) {
			callback(connfd, &saddr, arg);
		}
	}
#if defined(_WIN32) || defined(_WIN64)
	switch (WSAGetLastError()) {
		case WSAEMFILE:
		case WSAENOBUFS:
		case WSAEWOULDBLOCK:
			return TRUE;
		case WSAECONNRESET:
		case WSAEINTR:
			return TRUE;
	}
#else
	switch (errno) {
		case EAGAIN:
	#if	EAGAIN != EWOULDBLOCK
		case EWOULDBLOCK:
	#endif
		case ENFILE:
		case EMFILE:
		case ENOBUFS:
		case ENOMEM:
			return TRUE;
		case ECONNABORTED:
		case EPERM:/* maybe linux firewall */
		case EINTR:
			return TRUE;
	}
#endif
	return FALSE;
}

BOOL reactorClose(Reactor_t* reactor) {
#if defined(_WIN32) || defined(_WIN64)
	return CloseHandle((HANDLE)(reactor->__hNio));
#else
	#ifdef	__linux__
	if (reactor->__epfd >= 0 && close(reactor->__epfd)) {
		return FALSE;
	}
	reactor->__epfd = -1;
	#endif
	return close(reactor->__hNio) == 0;
#endif
}

#ifdef	__cplusplus
}
#endif
