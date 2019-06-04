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
/*
BOOL reactorCancel(Reactor_t* reactor, FD_t fd) {
#if defined(_WIN32) || defined(_WIN64)
	if (CancelIoEx((HANDLE)fd, NULL)) {
		// use GetOverlappedResult wait until all overlapped is completed ???
		return TRUE;
	}
	return GetLastError() == ERROR_NOT_FOUND;
	// iocp will catch this return and set overlapped.internal a magic number, but header not include that macro
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
*/
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
	DWORD dwNumberOfBytesTransferred;
	WSABUF wsabuf;
} IocpReadOverlapped;
typedef struct IocpAcceptExOverlapped {
	IocpOverlapped base;
	SOCKET accpetsocket;
	unsigned char saddrs[sizeof(struct sockaddr_storage) + 16 + sizeof(struct sockaddr_storage) + 16];
} IocpAcceptExOverlapped;
#endif

void* reactorMallocOverlapped(int opcode, const void* refbuf, unsigned int refsize, unsigned int appendsize) {
#if defined(_WIN32) || defined(_WIN64)
	switch (opcode) {
		case REACTOR_READ:
		{
			IocpReadOverlapped* ol = (IocpReadOverlapped*)malloc(sizeof(IocpReadOverlapped) + appendsize);
			if (ol) {
				memset(ol, 0, sizeof(IocpReadOverlapped));
				ol->base.opcode = REACTOR_READ;
				ol->saddr.ss_family = AF_UNSPEC;
				if (refbuf && refsize) {
					ol->wsabuf.buf = (char*)refbuf;
					ol->wsabuf.len = refsize;
				}
				else if (appendsize) {
					ol->wsabuf.buf = (char*)(ol + 1);
					ol->wsabuf.len = appendsize;
				}
			}
			return ol;
		}
		case REACTOR_WRITE:
		case REACTOR_CONNECT:
		{
			IocpOverlapped* ol = (IocpOverlapped*)calloc(1, sizeof(IocpOverlapped));
			if (ol) {
				ol->opcode = opcode;
			}
			return ol;
		}
		case REACTOR_ACCEPT:
		{
			/*
			char* ol = (char*)calloc(1, sizeof(OVERLAPPED) + 2 + sizeof(SOCKET) + (sizeof(struct sockaddr_storage) + 16) * 2);
			if (ol) {
				ol[sizeof(OVERLAPPED) + 1] = 1;
				*(SOCKET*)(ol + sizeof(OVERLAPPED) + 2) = INVALID_SOCKET;
			}
			return ol;
			*/
			IocpAcceptExOverlapped* ol = (IocpAcceptExOverlapped*)calloc(1, sizeof(IocpAcceptExOverlapped));
			if (ol) {
				ol->base.opcode = REACTOR_ACCEPT;
				ol->accpetsocket = INVALID_SOCKET;
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
	IocpOverlapped* iocp_ol = (IocpOverlapped*)ol;
	if (iocp_ol) {
		if (REACTOR_ACCEPT == iocp_ol->opcode) {
			IocpAcceptExOverlapped* iocp_acceptex = (IocpAcceptExOverlapped*)iocp_ol;
			if (INVALID_SOCKET != iocp_acceptex->accpetsocket) {
				closesocket(iocp_acceptex->accpetsocket);
				iocp_acceptex->accpetsocket = INVALID_SOCKET;
			}
		}
		if (iocp_ol->commit)
			iocp_ol->free = 1;
		else
			free(iocp_ol);
	}
#endif
}

BOOL reactorCommit(Reactor_t* reactor, FD_t fd, int opcode, void* ol, struct sockaddr_storage* saddr) {
#if defined(_WIN32) || defined(_WIN64)
	if (REACTOR_READ == opcode) {
		IocpReadOverlapped* iocp_ol = (IocpReadOverlapped*)ol;
		if (saddr) {
			int slen = sizeof(iocp_ol->saddr);
			DWORD Flags = 0;
			if (!WSARecvFrom((SOCKET)fd, &iocp_ol->wsabuf, 1, NULL, &Flags, (struct sockaddr*)&iocp_ol->saddr, &slen, (LPWSAOVERLAPPED)ol, NULL) ||
				WSAGetLastError() == WSA_IO_PENDING)
			{
				iocp_ol->base.commit = 1;
				return TRUE;
			}
		}
		else {
			if (ReadFileEx((HANDLE)fd, iocp_ol->wsabuf.buf, iocp_ol->wsabuf.len, (LPOVERLAPPED)ol, NULL) || GetLastError() == ERROR_IO_PENDING) {
				iocp_ol->base.commit = 1;
				return TRUE;
			}
		}
		return FALSE;
	}
	else if (REACTOR_WRITE == opcode) {
		IocpOverlapped* iocp_ol = (IocpOverlapped*)ol;
		if (saddr) {
			WSABUF wsabuf = { 0 };
			if (!WSASendTo((SOCKET)fd, &wsabuf, 1, NULL, 0, (struct sockaddr*)saddr, sizeof(*saddr), (LPWSAOVERLAPPED)ol, NULL) ||
				WSAGetLastError() == WSA_IO_PENDING)
			{
				iocp_ol->commit = 1;
				return TRUE;
			}
		}
		else {
			if (WriteFileEx((HANDLE)fd, NULL, 0, (LPOVERLAPPED)ol, NULL) || GetLastError() == ERROR_IO_PENDING) {
				iocp_ol->commit = 1;
				return TRUE;
			}
		}
		return FALSE;
	}
	else if (REACTOR_ACCEPT == opcode) {
		static LPFN_ACCEPTEX lpfnAcceptEx = NULL;
		IocpAcceptExOverlapped* iocp_ol = (IocpAcceptExOverlapped*)ol;
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
		if (INVALID_SOCKET == iocp_ol->accpetsocket) {
			iocp_ol->accpetsocket = socket(saddr->ss_family, SOCK_STREAM, 0);
			if (INVALID_SOCKET == iocp_ol->accpetsocket) {
				return FALSE;
			}
		}
		if (lpfnAcceptEx((SOCKET)fd, iocp_ol->accpetsocket, iocp_ol->saddrs, 0,
			sizeof(struct sockaddr_storage) + 16, sizeof(struct sockaddr_storage) + 16,
			NULL, (LPOVERLAPPED)ol) ||
			WSAGetLastError() == ERROR_IO_PENDING)
		{
			iocp_ol->base.commit = 1;
			return TRUE;
		}
		else {
			closesocket(iocp_ol->accpetsocket);
			iocp_ol->accpetsocket = INVALID_SOCKET;
			return FALSE;
		}
	}
	else if (REACTOR_CONNECT == opcode) {
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
				&dwBytes, NULL, NULL) == SOCKET_ERROR || !lpfnConnectEx)
			{
				return FALSE;
			}
		}
		_sa.ss_family = saddr->ss_family;
		if (bind((SOCKET)fd, (struct sockaddr*)&_sa, sizeof(_sa))) {
			return FALSE;
		}
		if (lpfnConnectEx((SOCKET)fd, (struct sockaddr*)saddr, slen, NULL, 0, NULL, (LPWSAOVERLAPPED)ol) ||
			WSAGetLastError() == ERROR_IO_PENDING)
		{
			((IocpOverlapped*)ol)->commit = 1;
			return TRUE;
		}
		return FALSE;
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

FD_t reactorEventFD(const NioEv_t* e) {
#if defined(_WIN32) || defined(_WIN64)
	return e->lpCompletionKey;
#elif __linux__
	return e->data.fd & 0x7fffffff;
#elif defined(__FreeBSD__) || defined(__APPLE__)
	return (size_t)(e->udata);
#endif
}

void* reactorEventOverlapped(const NioEv_t* e) {
#if defined(_WIN32) || defined(_WIN64)
	IocpOverlapped* iocp_ol = (IocpOverlapped*)(e->lpOverlapped);
	if (iocp_ol->free) {
		free(iocp_ol);
		return NULL;
	}
	else {
		iocp_ol->commit = 0;
		return iocp_ol;
	}
#else
	return (void*)(size_t)-1;
#endif
}

int reactorEventOpcode(const NioEv_t* e) {
#if defined(_WIN32) || defined(_WIN64)
	IocpOverlapped* iocp_ol = (IocpOverlapped*)(e->lpOverlapped);
	if (REACTOR_ACCEPT == iocp_ol->opcode)
		return REACTOR_READ;
	else if (REACTOR_CONNECT == iocp_ol->opcode)
		return REACTOR_WRITE;
	else if (REACTOR_READ == iocp_ol->opcode) {
		IocpReadOverlapped* iocp_read_ol = (IocpReadOverlapped*)iocp_ol;
		iocp_read_ol->dwNumberOfBytesTransferred = e->dwNumberOfBytesTransferred;
		return REACTOR_READ;
	}
	else if (REACTOR_WRITE == iocp_ol->opcode)
		return REACTOR_WRITE;
	else
		return REACTOR_NOP;
#elif __linux__
	/* epoll must catch this event */
	if ((e->events & EPOLLRDHUP) || (e->events & EPOLLHUP))
		return (e->data.fd & 0x80000000) ? REACTOR_WRITE : REACTOR_READ;
	/* we use __epfd to filter EPOLLOUT event */
	else if (e->events & EPOLLIN)
		return REACTOR_READ;
	else if (e->events & EPOLLOUT)
		return REACTOR_WRITE;
	else
		return REACTOR_NOP;
#elif defined(__FreeBSD__) || defined(__APPLE__)
	if (EVFILT_READ == e->filter)
		return REACTOR_READ;
	else if (EVFILT_WRITE == e->filter)
		return REACTOR_WRITE;
	else /* program don't run here... */
		return REACTOR_NOP;
#endif
}

int reactorEventOverlappedData(void* ol, Iobuf_t* iov, struct sockaddr_storage* saddr) {
#if defined(_WIN32) || defined(_WIN64)
	IocpOverlapped* iocp_ol = (IocpOverlapped*)ol;
	if (REACTOR_READ == iocp_ol->opcode) {
		IocpReadOverlapped* iocp_read_ol = (IocpReadOverlapped*)iocp_ol;
		iov->buf = iocp_read_ol->wsabuf.buf;
		iov->len = iocp_read_ol->dwNumberOfBytesTransferred;
		if (saddr)
			*saddr = iocp_read_ol->saddr;
		return 1;
	}
#endif
	iobufPtr(iov) = NULL;
	iobufLen(iov) = 0;
	if (saddr)
		saddr->ss_family = AF_UNSPEC;
	return 0;
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

FD_t reactorAcceptFirst(FD_t listenfd, void* ol, struct sockaddr_storage* peer_saddr) {
#if defined(_WIN32) || defined(_WIN64)
	IocpAcceptExOverlapped* iocp_ol = (IocpAcceptExOverlapped*)ol;
	SOCKET acceptfd = iocp_ol->accpetsocket;
	iocp_ol->accpetsocket = INVALID_SOCKET;
	do {
		int slen;
		if (setsockopt(acceptfd, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, (char*)&listenfd, sizeof(listenfd))) {
			closesocket(acceptfd);
			break;
		}
		slen = sizeof(*peer_saddr);
		if (getpeername(acceptfd, (struct sockaddr*)peer_saddr, &slen)) {
			closesocket(acceptfd);
			break;
		}
		return acceptfd;
	} while (0);
	return INVALID_FD_HANDLE;
#else
	socklen_t slen = sizeof(*peer_saddr);
	return accept(listenfd, (struct sockaddr*)peer_saddr, &slen);
#endif
}

FD_t reactorAcceptNext(FD_t listenfd, struct sockaddr_storage* peer_saddr) {
#if defined(_WIN32) || defined(_WIN64)
	int slen = sizeof(*peer_saddr);
#else
	socklen_t slen = sizeof(*peer_saddr);
#endif
	return accept(listenfd, (struct sockaddr*)peer_saddr, &slen);
/*
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
		case EPERM:
		case EINTR:
			return TRUE;
	}
#endif
	return FALSE;
*/
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
