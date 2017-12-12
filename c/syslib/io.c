//
// Created by hujianzhe
//

#include "io.h"
#if defined(_WIN32) || defined(_WIN64)
	#include <mswsock.h>
	#include <ws2ipdef.h>
#else
	#include <sys/socket.h>
#endif
#include <errno.h>
#include <string.h>

#ifdef	__cplusplus
extern "C" {
#endif

/* aiocb */
void aio_Init(struct aiocb* cb, size_t udata) {
	memset(cb, 0, sizeof(*cb));
	cb->aio_lio_opcode = LIO_NOP;
	cb->aio_fildes = INVALID_FD_HANDLE;
	cb->aio_sigevent.sigev_value.sival_ptr = (void*)udata;
#if !defined(_WIN32) && !defined(_WIN64)
	cb->aio_sigevent.sigev_notify = SIGEV_NONE;
#endif
}

void aio_SetOffset(struct aiocb* cb, long long offset) {
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

EXEC_RETURN aio_Commit(struct aiocb* cb) {
	if (LIO_READ == cb->aio_lio_opcode) {
#if defined(_WIN32) || defined(_WIN64)
		BOOL res = ReadFileEx((HANDLE)(cb->aio_fildes),
								cb->aio_buf, cb->aio_nbytes,
								&cb->in.__ol, NULL) ||
								GetLastError() == ERROR_IO_PENDING;
		return res ? EXEC_SUCCESS : EXEC_ERROR;
#else
		return aio_read(cb) ? EXEC_ERROR : EXEC_SUCCESS;
#endif
	}
	else if (LIO_WRITE == cb->aio_lio_opcode) {
#if defined(_WIN32) || defined(_WIN64)
		BOOL res = WriteFileEx((HANDLE)(cb->aio_fildes),
								cb->aio_buf, cb->aio_nbytes,
								&cb->out.__ol, NULL) ||
								GetLastError() == ERROR_IO_PENDING;
		return res ? EXEC_SUCCESS : EXEC_ERROR;
#else
		return aio_write(cb) ? EXEC_ERROR : EXEC_SUCCESS;
#endif
	}
	return EXEC_SUCCESS;
}

BOOL aio_HasCompleted(const struct aiocb* cb) {
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

EXEC_RETURN aio_Suspend(const struct aiocb* const cb_list[], int nent, int msec) {
#if defined(_WIN32) || defined(_WIN64)
	DWORD i;
	DWORD dwCount = nent > MAXIMUM_WAIT_OBJECTS ? MAXIMUM_WAIT_OBJECTS : nent;
	HANDLE hObjects[MAXIMUM_WAIT_OBJECTS];
	for (i = 0; i < dwCount; ++i) {
		hObjects[i] = (HANDLE)(cb_list[i]->aio_fildes);
	}
	i = WaitForMultipleObjects(dwCount, hObjects, FALSE, msec);
	return i >= WAIT_OBJECT_0 && i < WAIT_OBJECT_0 + MAXIMUM_WAIT_OBJECTS ? EXEC_SUCCESS : EXEC_ERROR;
#else
	struct timespec tval, *t = NULL;
	if (msec >= 0) {
		tval.tv_sec = msec / 1000;
		tval.tv_nsec = msec % 1000;
		t = &tval;
	}
	return 0 == aio_suspend(cb_list, nent, t) ? EXEC_SUCCESS : EXEC_ERROR;
#endif
}

EXEC_RETURN aio_Cancel(FD_HANDLE fd, struct aiocb* cb) {
#if defined(_WIN32) || defined(_WIN64)
	OVERLAPPED* p_ol = NULL;
	if (LIO_READ == cb->aio_lio_opcode) {
		p_ol = &cb->in.__ol;
	}
	else if (LIO_WRITE == cb->aio_lio_opcode) {
		p_ol = &cb->out.__ol;
	}
	else {
		return EXEC_ERROR;
	}
	return CancelIoEx((HANDLE)fd, p_ol) ? EXEC_SUCCESS : EXEC_ERROR;
#else
	int res = aio_cancel(fd, cb);
	if (AIO_CANCELED == res || AIO_ALLDONE == res) {
		return EXEC_SUCCESS;
	}
	else if (AIO_NOTCANCELED == res) {
		errno = EINPROGRESS;
	}
	return EXEC_ERROR;
#endif
}

int aio_Result(struct aiocb* cb, unsigned int* transfer_bytes) {
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
EXEC_RETURN reactor_Create(REACTOR* reactor) {
#if defined(_WIN32) || defined(_WIN64)
	SYSTEM_INFO si;
	GetSystemInfo(&si);
	reactor->__hNio = (FD_HANDLE)CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, si.dwNumberOfProcessors << 1);
	return reactor->__hNio ? EXEC_SUCCESS : EXEC_ERROR;
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
		return EXEC_SUCCESS;
	} while (0);
	if (nio_ok) {
		assert_true(0 == close(reactor->__hNio));
	}
	if (epfd_ok) {
		assert_true(0 == close(reactor->__epfd));
	}
	return EXEC_ERROR;
#elif defined(__FreeBSD__) || defined(__APPLE__)
	return (reactor->__hNio = kqueue()) < 0 ? EXEC_ERROR : EXEC_SUCCESS;
#endif
}

EXEC_RETURN reactor_Reg(REACTOR* reactor, FD_HANDLE fd) {
#if defined(_WIN32) || defined(_WIN64)
	return CreateIoCompletionPort((HANDLE)fd, (HANDLE)(reactor->__hNio), (ULONG_PTR)fd, 0) == (HANDLE)(reactor->__hNio) ? EXEC_SUCCESS : EXEC_ERROR;
#elif defined(__FreeBSD__) || defined(__APPLE__)
	struct kevent e[2];
	EV_SET(&e[0], (uintptr_t)fd, EVFILT_READ, EV_ADD | EV_DISABLE, 0, 0, (void*)(size_t)fd);
	EV_SET(&e[1], (uintptr_t)fd, EVFILT_WRITE, EV_ADD | EV_DISABLE, 0, 0, (void*)(size_t)fd);
	return kevent(reactor->__hNio, e, 2, NULL, 0, NULL) == 0 ? EXEC_SUCCESS : EXEC_ERROR;
#endif
	return EXEC_SUCCESS;
}

EXEC_RETURN reactor_Cancel(REACTOR* reactor, FD_HANDLE fd) {
#if defined(_WIN32) || defined(_WIN64)
	return CancelIoEx((HANDLE)fd, NULL) ? EXEC_SUCCESS : EXEC_ERROR;
	/*
	 * iocp will catch this return and set overlapped.internal a magic number, but header not include that macro
	 */
#elif __linux__
	struct epoll_event e = { 0 };
	if (epoll_ctl(reactor->__hNio, EPOLL_CTL_DEL, fd, &e)) {
		return EXEC_ERROR;
	}
	if (epoll_ctl(reactor->__epfd, EPOLL_CTL_DEL, fd, &e)) {
		return EXEC_ERROR;
	}
	return EXEC_SUCCESS;
#elif defined(__FreeBSD__) || defined(__APPLE__)
	struct kevent e[2];
	EV_SET(&e[0], (uintptr_t)fd, EVFILT_READ, EV_DISABLE, 0, 0, NULL);
	EV_SET(&e[1], (uintptr_t)fd, EVFILT_WRITE, EV_DISABLE, 0, 0, NULL);
	return kevent(reactor->__hNio, e, 2, NULL, 0, NULL) == 0 ? EXEC_SUCCESS : EXEC_ERROR;
#endif
}

EXEC_RETURN reactor_Commit(REACTOR* reactor, FD_HANDLE fd, int opcode, void** p_ol, struct sockaddr_storage* saddr) {
#if defined(_WIN32) || defined(_WIN64)
	if (REACTOR_READ == opcode) {
		BOOL res;
		int slen = sizeof(*saddr);
		DWORD Flags = 0;
		WSABUF wsabuf = {0};
		*p_ol = *p_ol ? *p_ol : calloc(1, sizeof(OVERLAPPED) + 1);
		if (NULL == *p_ol) {
			return EXEC_ERROR;
		}
		res = !WSARecvFrom((SOCKET)fd,
							&wsabuf, 1, NULL, &Flags,
							(struct sockaddr*)saddr, &slen,/* connectionless socket must set this param */
							(LPWSAOVERLAPPED)*p_ol, NULL) ||
							WSAGetLastError() == WSA_IO_PENDING;
		if (!res) {
			free(*p_ol);
			*p_ol = NULL;
			return EXEC_ERROR;
		}
		return EXEC_SUCCESS;
	}
	else if (REACTOR_WRITE == opcode) {
		BOOL res;
		WSABUF wsabuf = {0};
		*p_ol = *p_ol ? *p_ol : calloc(1, sizeof(OVERLAPPED) + 1);
		if (NULL == *p_ol) {
			return EXEC_ERROR;
		}
		res = !WSASendTo((SOCKET)fd,
							&wsabuf, 1, NULL, 0,
							(struct sockaddr*)saddr, sizeof(*saddr),
							(LPWSAOVERLAPPED)(((char*)*p_ol) + 1), NULL) ||
							WSAGetLastError() == WSA_IO_PENDING;
		if (!res) {
			free(*p_ol);
			*p_ol = NULL;
			return EXEC_ERROR;
		}
		return EXEC_SUCCESS;
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
				&dwBytes, NULL, NULL) == SOCKET_ERROR || !lpfnAcceptEx) {
				return EXEC_ERROR;
			}
		}
		if (NULL == *p_ol) {
			*p_ol = calloc(1, sizeof(OVERLAPPED) + sizeof(SOCKET) + (sizeof(struct sockaddr_storage) + 16) * 2);
			if (NULL == *p_ol) {
				return EXEC_ERROR;
			}
			pConnfd = (SOCKET*)(((char*)*p_ol) + sizeof(OVERLAPPED));
			*pConnfd = socket(saddr->ss_family, SOCK_STREAM, 0);
			if (INVALID_SOCKET == *pConnfd) {
				free(*p_ol);
				*p_ol = NULL;
				return EXEC_ERROR;
			}
		}
		else {
			pConnfd = (SOCKET*)(((char*)*p_ol) + sizeof(OVERLAPPED));
		}
		res = lpfnAcceptEx((SOCKET)fd, *pConnfd,
							pConnfd + 1, 0,
							sizeof(struct sockaddr_storage) + 16,
							sizeof(struct sockaddr_storage) + 16,
							NULL, (LPOVERLAPPED)*p_ol) ||
							WSAGetLastError() == ERROR_IO_PENDING;
		if (!res) {
			closesocket(*pConnfd);
			free(*p_ol);
			*p_ol = NULL;
			return EXEC_ERROR;
		}
		return EXEC_SUCCESS;
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
				return EXEC_ERROR;
		}
		if (!lpfnConnectEx){
			DWORD dwBytes;
			GUID GuidConnectEx = WSAID_CONNECTEX;
			if (WSAIoctl((SOCKET)fd, SIO_GET_EXTENSION_FUNCTION_POINTER,
				&GuidConnectEx, sizeof(GuidConnectEx),
				&lpfnConnectEx, sizeof(lpfnConnectEx),
				&dwBytes, NULL, NULL) == SOCKET_ERROR || !lpfnConnectEx) {
				return EXEC_ERROR;
			}
		}
		_sa.ss_family = saddr->ss_family;
		if (bind((SOCKET)fd, (struct sockaddr*)&_sa, sizeof(_sa))) {
			return EXEC_ERROR;
		}
		*p_ol = *p_ol ? *p_ol : calloc(1, sizeof(OVERLAPPED) + 1);
		if (NULL == *p_ol) {
			return EXEC_ERROR;
		}
		res = lpfnConnectEx((SOCKET)fd,
							(struct sockaddr*)saddr, slen,
							NULL, 0, NULL,
							(LPWSAOVERLAPPED)(((char*)*p_ol) + 1)) ||
							WSAGetLastError() == ERROR_IO_PENDING;
		if (!res) {
			free(*p_ol);
			*p_ol = NULL;
			return EXEC_ERROR;
		}
		return EXEC_SUCCESS;
	}
	else {
		SetLastError(ERROR_INVALID_PARAMETER);
		return EXEC_ERROR;
	}
#else
	if (REACTOR_READ == opcode || REACTOR_ACCEPT == opcode) {
	#ifdef __linux__
		struct epoll_event e;
		e.data.fd = fd;
		e.events = EPOLLET | EPOLLONESHOT | EPOLLIN;
		if (epoll_ctl(reactor->__hNio, EPOLL_CTL_MOD, fd, &e)) {
			if (ENOENT != errno) {
				return EXEC_ERROR;
			}
			return epoll_ctl(reactor->__hNio, EPOLL_CTL_ADD, fd, &e) == 0 || EEXIST == errno ? EXEC_SUCCESS : EXEC_ERROR;
		}
		return EXEC_SUCCESS;
	#elif defined(__FreeBSD__) || defined(__APPLE__)
		struct kevent e;
		EV_SET(&e, (uintptr_t)fd, EVFILT_READ, EV_ENABLE | EV_ONESHOT, 0, 0, (void*)(size_t)fd);
		return kevent(reactor->__hNio, &e, 1, NULL, 0, NULL) ? EXEC_ERROR : EXEC_SUCCESS;
	#endif
	}
	else if (REACTOR_WRITE == opcode || REACTOR_CONNECT == opcode) {
		if (REACTOR_CONNECT == opcode) {/* try connect... */
			if (connect(fd, (struct sockaddr*)saddr, sizeof(*saddr)) && EINPROGRESS != errno) {
				return EXEC_ERROR;
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
				return EXEC_ERROR;
			}
			return epoll_ctl(reactor->__epfd, EPOLL_CTL_ADD, fd, &e) == 0 || EEXIST == errno ? EXEC_SUCCESS : EXEC_ERROR;
		}
		return EXEC_SUCCESS;
	#elif defined(__FreeBSD__) || defined(__APPLE__)
		struct kevent e;
		EV_SET(&e, (uintptr_t)fd, EVFILT_WRITE, EV_ENABLE | EV_ONESHOT, 0, 0, (void*)(size_t)fd);
		return kevent(reactor->__hNio, &e, 1, NULL, 0, NULL) ? EXEC_ERROR : EXEC_SUCCESS;
	#endif
	}
	else {
		errno = EINVAL;
		return EXEC_ERROR;
	}
#endif
	return EXEC_ERROR;
}

int reactor_Wait(REACTOR* reactor, NIO_EVENT* e, unsigned int count, int msec) {
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

void reactor_Result(const NIO_EVENT* e, FD_HANDLE* p_fd, int* p_event, void** p_ol) {
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

BOOL reactor_ConnectCheckSuccess(FD_HANDLE sockfd) {
#if defined(_WIN32) || defined(_WIN64)
	int sec;
	int len = sizeof(sec);
	if (getsockopt(sockfd, SOL_SOCKET, SO_CONNECT_TIME, (char*)&sec, &len)) {
		return FALSE;
	}
	if (~0 == sec) {
		return FALSE;
	}
	return !setsockopt(sockfd, SOL_SOCKET, SO_UPDATE_CONNECT_CONTEXT, NULL, 0);
#else
	int error;
	socklen_t len = sizeof(int);
	if (getsockopt(sockfd, SOL_SOCKET, SO_ERROR, (char*)&error, &len)) {
		return FALSE;
	}
	return !error;
#endif
}

BOOL reactor_AcceptPretreatment(FD_HANDLE listenfd, void* ol, REACTOR_ACCEPT_CALLBACK cbfunc, size_t arg) {
#if defined(_WIN32) || defined(_WIN64)
	SOCKET* pConnfd = ol ? (SOCKET*)(((char*)ol) + sizeof(OVERLAPPED)) : NULL;
	if (!pConnfd) {
		return TRUE;
	}
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
		if (cbfunc) {
			slen = sizeof(saddr);
			if (getpeername(connfd, (struct sockaddr*)&saddr, &slen)) {
				closesocket(connfd);
				break;
			}
			cbfunc(connfd, &saddr, arg);
		}
	} while (0);
	return *pConnfd != INVALID_SOCKET;
#else
	return TRUE;
#endif
}

EXEC_RETURN reactor_Close(REACTOR* reactor) {
#if defined(_WIN32) || defined(_WIN64)
	return CloseHandle((HANDLE)(reactor->__hNio)) ? EXEC_SUCCESS : EXEC_ERROR;
#else
	#ifdef	__linux__
	if (reactor->__epfd >= 0 && close(reactor->__epfd)) {
		return EXEC_ERROR;
	}
	reactor->__epfd = -1;
	#endif
	return close(reactor->__hNio) == 0 ? EXEC_SUCCESS : EXEC_ERROR;
#endif
}

#ifdef	__cplusplus
}
#endif
