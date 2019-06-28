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
#endif
#include <errno.h>
#include <string.h>

#ifdef	__cplusplus
extern "C" {
#endif

/* aiocb */
void aioInit(AioCtx_t* ctx) {
	memset(ctx, 0, sizeof(*ctx));
	ctx->cb.aio_lio_opcode = LIO_NOP;
	ctx->cb.aio_fildes = INVALID_FD_HANDLE;
}

#if defined(_WIN32) || defined(_WIN64)
static VOID WINAPI win32_aio_callback(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped) {
	AioCtx_t* ctx = (AioCtx_t*)lpOverlapped;
	ctx->callback(dwErrorCode, dwNumberOfBytesTransfered, ctx);
}
#else
static void posix_aio_callback(union sigval v) {
	AioCtx_t* ctx = (AioCtx_t*)(v.sival_ptr);
	int error = aio_error(&ctx->cb);
	ssize_t nbytes = aio_return(&ctx->cb);
	ctx->callback(error, nbytes, ctx);
}
#endif

BOOL aioCommit(AioCtx_t* ctx) {
#if defined(_WIN32) || defined(_WIN64)
	ctx->ol.Offset = ctx->cb.aio_offset;
	ctx->ol.OffsetHigh = ctx->cb.aio_offset >> 32;
	if (LIO_READ == ctx->cb.aio_lio_opcode) {
		return ReadFileEx((HANDLE)(ctx->cb.aio_fildes),
			ctx->cb.aio_buf, ctx->cb.aio_nbytes,
			&ctx->ol, ctx->callback ? win32_aio_callback : NULL) ||
			GetLastError() == ERROR_IO_PENDING;
	}
	else if (LIO_WRITE == ctx->cb.aio_lio_opcode) {
		return WriteFileEx((HANDLE)(ctx->cb.aio_fildes),
			ctx->cb.aio_buf, ctx->cb.aio_nbytes,
			&ctx->ol, ctx->callback ? win32_aio_callback : NULL) ||
			GetLastError() == ERROR_IO_PENDING;
	}
#else
	if (ctx->callback) {
		ctx->cb.aio_sigevent.sigev_value.sival_ptr = ctx;
		ctx->cb.aio_sigevent.sigev_notify = SIGEV_THREAD;
		ctx->cb.aio_sigevent.sigev_notify_function = posix_aio_callback;
	}
	else {
		ctx->cb.aio_sigevent.sigev_notify = SIGEV_NONE;
	}
	if (LIO_READ == ctx->cb.aio_lio_opcode) {
		return aio_read(&ctx->cb) == 0;
	}
	else if (LIO_WRITE == ctx->cb.aio_lio_opcode) {
		return aio_write(&ctx->cb) == 0;
	}
#endif
	return FALSE;
}

BOOL aioHasCompleted(const AioCtx_t* ctx) {
#if defined(_WIN32) || defined(_WIN64)
	return HasOverlappedIoCompleted(&ctx->ol);
#else
	return aio_error(&ctx->cb) != EINPROGRESS;
#endif
}

int aioSuspend(const AioCtx_t* ctx, int msec) {
#if defined(_WIN32) || defined(_WIN64)
	HANDLE handle = (HANDLE)ctx->cb.aio_fildes;
	DWORD result = WaitForSingleObject(handle, msec);
	if (WAIT_OBJECT_0 == result)
		return 1;
	else if (WAIT_TIMEOUT == result || WSA_WAIT_TIMEOUT == result)
		return 0;
	else
		return -1;
#else
	const struct aiocb * const cblist[] = { &ctx->cb };
	struct timespec tval, *t = NULL;
	if (msec >= 0) {
		tval.tv_sec = msec / 1000;
		tval.tv_nsec = msec % 1000;
		t = &tval;
	}
	if (0 == aio_suspend(cblist, 1, t))
		return 1;
	else if (EAGAIN == errno)
		return 0;
	else
		return -1;
#endif
}

BOOL aioCancel(FD_t fd, AioCtx_t* ctx) {
#if defined(_WIN32) || defined(_WIN64)
	return CancelIoEx((HANDLE)fd, ctx ? &ctx->ol : NULL) || ERROR_OPERATION_ABORTED == GetLastError();
#else
	int res = aio_cancel(fd, ctx ? &ctx->cb : NULL);
	if (AIO_CANCELED == res || AIO_ALLDONE == res)
		return TRUE;
	else if (AIO_NOTCANCELED == res)
		errno = EINPROGRESS;
	return FALSE;
#endif
}

int aioError(AioCtx_t* ctx) {
#if defined(_WIN32) || defined(_WIN64)
	return ctx->ol.Internal;
#else
	return aio_error(&ctx->cb);
#endif
}

int aioNumberOfBytesTransfered(AioCtx_t* ctx) {
#if defined(_WIN32) || defined(_WIN64)
	return ctx->ol.InternalHigh;
#else
	return aio_return(&ctx->cb);
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
	SOCKET acceptsocket;
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
				ol->acceptsocket = INVALID_SOCKET;
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
			if (INVALID_SOCKET != iocp_acceptex->acceptsocket) {
				closesocket(iocp_acceptex->acceptsocket);
				iocp_acceptex->acceptsocket = INVALID_SOCKET;
			}
		}
		if (iocp_ol->commit)
			iocp_ol->free = 1;
		else
			free(iocp_ol);
	}
#endif
}

BOOL reactorCommit(Reactor_t* reactor, FD_t fd, int opcode, void* ol, struct sockaddr* saddr, int addrlen) {
#if defined(_WIN32) || defined(_WIN64)
	if (REACTOR_READ == opcode) {
		IocpReadOverlapped* iocp_ol = (IocpReadOverlapped*)ol;
		if (saddr) {
			int slen = sizeof(iocp_ol->saddr);
			DWORD Flags = 0;
			if (!WSARecvFrom((SOCKET)fd, &iocp_ol->wsabuf, 1, NULL, &Flags, (struct sockaddr*)&iocp_ol->saddr, &slen, (LPWSAOVERLAPPED)ol, NULL)) {
				iocp_ol->base.commit = 1;
				return TRUE;
			}
			else if (WSAGetLastError() == WSA_IO_PENDING) {
				iocp_ol->base.commit = 1;
				return TRUE;
			}
		}
		else {
			if (ReadFileEx((HANDLE)fd, iocp_ol->wsabuf.buf, iocp_ol->wsabuf.len, (LPOVERLAPPED)ol, NULL)) {
				iocp_ol->base.commit = 1;
				return TRUE;
			}
			else if (GetLastError() == ERROR_IO_PENDING) {
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
			if (!WSASendTo((SOCKET)fd, &wsabuf, 1, NULL, 0, saddr, addrlen, (LPWSAOVERLAPPED)ol, NULL)) {
				iocp_ol->commit = 1;
				return TRUE;
			}
			else if (WSAGetLastError() == WSA_IO_PENDING) {
				iocp_ol->commit = 1;
				return TRUE;
			}
		}
		else {
			if (WriteFileEx((HANDLE)fd, NULL, 0, (LPOVERLAPPED)ol, NULL)) {
				iocp_ol->commit = 1;
				return TRUE;
			}
			else if (GetLastError() == ERROR_IO_PENDING) {
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
		if (INVALID_SOCKET == iocp_ol->acceptsocket) {
			iocp_ol->acceptsocket = socket(saddr->sa_family, SOCK_STREAM, 0);
			if (INVALID_SOCKET == iocp_ol->acceptsocket) {
				return FALSE;
			}
		}
		if (lpfnAcceptEx((SOCKET)fd, iocp_ol->acceptsocket, iocp_ol->saddrs, 0,
			sizeof(struct sockaddr_storage) + 16, sizeof(struct sockaddr_storage) + 16,
			NULL, (LPOVERLAPPED)ol))
		{
			iocp_ol->base.commit = 1;
			return TRUE;
		}
		else if (WSAGetLastError() == ERROR_IO_PENDING) {
			iocp_ol->base.commit = 1;
			return TRUE;
		}
		else {
			closesocket(iocp_ol->acceptsocket);
			iocp_ol->acceptsocket = INVALID_SOCKET;
			return FALSE;
		}
	}
	else if (REACTOR_CONNECT == opcode) {
		struct sockaddr_storage st;
		static LPFN_CONNECTEX lpfnConnectEx = NULL;
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
		st.ss_family = saddr->sa_family;
		if (bind((SOCKET)fd, (struct sockaddr*)&st, addrlen)) {
			return FALSE;
		}
		if (lpfnConnectEx((SOCKET)fd, saddr, addrlen, NULL, 0, NULL, (LPWSAOVERLAPPED)ol)) {
			((IocpOverlapped*)ol)->commit = 1;
			return TRUE;
		}
		else if (WSAGetLastError() == ERROR_IO_PENDING) {
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
			if (connect(fd, saddr, addrlen) && EINPROGRESS != errno) {
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
	SOCKET acceptfd = iocp_ol->acceptsocket;
	iocp_ol->acceptsocket = INVALID_SOCKET;
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
