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

/* aiocb */
void aioInitCtx(AioCtx_t* ctx) {
	memset(ctx, 0, sizeof(*ctx));
	ctx->cb.aio_lio_opcode = LIO_NOP;
	ctx->cb.aio_fildes = INVALID_FD_HANDLE;
}
/*
#if defined(_WIN32) || defined(_WIN64)
static VOID WINAPI win32_apc_aio_callback(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped) {
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
*/
BOOL aioCommit(AioCtx_t* ctx) {
#if defined(_WIN32) || defined(_WIN64)
	DWORD realbytes;
	ctx->ol.Offset = (DWORD)ctx->cb.aio_offset;
	ctx->ol.OffsetHigh = ctx->cb.aio_offset >> 32;
	if (LIO_READ == ctx->cb.aio_lio_opcode) {
		return ReadFile((HANDLE)(ctx->cb.aio_fildes),
			ctx->cb.aio_buf, ctx->cb.aio_nbytes,
			&realbytes, &ctx->ol) ||
			GetLastError() == ERROR_IO_PENDING ||
			GetLastError() == ERROR_HANDLE_EOF;
	}
	else if (LIO_WRITE == ctx->cb.aio_lio_opcode) {
		return WriteFile((HANDLE)(ctx->cb.aio_fildes),
			ctx->cb.aio_buf, ctx->cb.aio_nbytes,
			&realbytes, &ctx->ol) ||
			GetLastError() == ERROR_IO_PENDING;
	}
#else
	/*
	if (ctx->callback) {
		ctx->cb.aio_sigevent.sigev_value.sival_ptr = ctx;
		ctx->cb.aio_sigevent.sigev_notify = SIGEV_THREAD;
		ctx->cb.aio_sigevent.sigev_notify_function = posix_aio_callback;
	}
	else {
		ctx->cb.aio_sigevent.sigev_notify = SIGEV_NONE;
	}
	*/
	ctx->cb.aio_sigevent.sigev_notify = SIGEV_NONE;
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
	DWORD realbytes, err;
	if (GetOverlappedResult((HANDLE)ctx->cb.aio_fildes, &ctx->ol, &realbytes, FALSE))
		return 0;
	err = GetLastError();
	return ERROR_HANDLE_EOF == err ? 0 : err;
#else
	return aio_error(&ctx->cb);
#endif
}

int aioNumberOfBytesTransfered(AioCtx_t* ctx) {
#if defined(_WIN32) || defined(_WIN64)
	DWORD realbytes;
	return GetOverlappedResult((HANDLE)ctx->cb.aio_fildes, &ctx->ol, &realbytes, FALSE) ? realbytes : 0;
#else
	return aio_return(&ctx->cb);
#endif
}

/* NIO */
BOOL nioCreate(Nio_t* nio) {
#if defined(_WIN32) || defined(_WIN64)
	SYSTEM_INFO si;
	GetSystemInfo(&si);
	nio->__wakeup = 0;
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

/*
BOOL nioUnReg(Nio_t* nio, FD_t fd) {
#if defined(_WIN32) || defined(_WIN64)
	// note: IOCP isn't support fd unreg
	return FALSE;
#elif __linux__
	struct epoll_event e = { 0 };
	if (epoll_ctl(nio->__hNio, EPOLL_CTL_DEL, fd, &e) && ENOENT != errno) {
		return FALSE;
	}
	return TRUE;
#elif defined(__FreeBSD__) || defined(__APPLE__)
	struct kevent e[2];
	EV_SET(&e[0], (uintptr_t)fd, EVFILT_READ, EV_DELETE, 0, 0, NULL);
	EV_SET(&e[1], (uintptr_t)fd, EVFILT_WRITE, EV_DELETE, 0, 0, NULL);
	return kevent(nio->__hNio, e, 2, NULL, 0, NULL) == 0;
#endif
}
*/

/*
BOOL nioCancel(Nio_t* nio, FD_t fd) {
#if defined(_WIN32) || defined(_WIN64)
	if (CancelIoEx((HANDLE)fd, NULL)) {
		// use GetOverlappedResult wait until all overlapped is completed ???
		return TRUE;
	}
	return GetLastError() == ERROR_NOT_FOUND;
	// iocp will catch this return and set overlapped.internal a magic number, but header not include that macro
#elif __linux__
	struct epoll_event e = { 0 };
	if (epoll_ctl(nio->__hNio, EPOLL_CTL_DEL, fd, &e)) {
		return FALSE;
	}
	return TRUE;
#elif defined(__FreeBSD__) || defined(__APPLE__)
	struct kevent e[2];
	EV_SET(&e[0], (uintptr_t)fd, EVFILT_READ, EV_DISABLE, 0, 0, NULL);
	EV_SET(&e[1], (uintptr_t)fd, EVFILT_WRITE, EV_DISABLE, 0, 0, NULL);
	return kevent(nio->__hNio, e, 2, NULL, 0, NULL) == 0;
#endif
}
*/
#if defined(_WIN32) || defined(_WIN64)
typedef struct IocpOverlapped {
	OVERLAPPED ol;
	unsigned char commit;
	unsigned char free;
	unsigned short opcode;
	int domain;
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
#endif

void* nioAllocOverlapped(int domain, int opcode, const void* refbuf, unsigned int refsize, unsigned int appendsize) {
#if defined(_WIN32) || defined(_WIN64)
	switch (opcode) {
		case NIO_OP_READ:
		{
			IocpReadOverlapped* ol = (IocpReadOverlapped*)malloc(sizeof(IocpReadOverlapped) + appendsize);
			if (ol) {
				memset(ol, 0, sizeof(IocpReadOverlapped));
				ol->base.opcode = NIO_OP_READ;
				ol->base.domain = domain;
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
			return ol;
		}
		case NIO_OP_CONNECT:
		case NIO_OP_WRITE:
		{
			IocpWriteOverlapped* ol = (IocpWriteOverlapped*)malloc(sizeof(IocpWriteOverlapped) + appendsize);
			if (ol) {
				memset(ol, 0, sizeof(IocpWriteOverlapped));
				ol->base.opcode = opcode;
				ol->base.domain = domain;
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
			return ol;
		}
		case NIO_OP_ACCEPT:
		{
			IocpAcceptExOverlapped* ol = (IocpAcceptExOverlapped*)calloc(1, sizeof(IocpAcceptExOverlapped));
			if (ol) {
				ol->base.opcode = NIO_OP_ACCEPT;
				ol->base.domain = domain;
				ol->acceptsocket = INVALID_SOCKET;
			}
			return ol;
		}
		default:
		{
			SetLastError(ERROR_INVALID_PARAMETER);
			return NULL;
		}
	}
#else
	return (void*)(size_t)opcode;
#endif
}

void nioFreeOverlapped(void* ol) {
#if defined(_WIN32) || defined(_WIN64)
	if (ol) {
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
#endif
}

BOOL nioCommit(Nio_t* nio, NioFD_t* niofd, void* ol, const struct sockaddr* saddr, socklen_t addrlen) {
#if defined(_WIN32) || defined(_WIN64)
	FD_t fd;
	IocpOverlapped* iocp_ol = (IocpOverlapped*)ol;
	if (iocp_ol->commit) {
		return TRUE;
	}
	if (!niofd->__reg) {
		if (CreateIoCompletionPort((HANDLE)(niofd->fd), (HANDLE)(nio->__hNio), (ULONG_PTR)niofd, 0) != (HANDLE)(nio->__hNio)) {
			return FALSE;
		}
		niofd->__reg = TRUE;
	}
	fd = niofd->fd;
	if (NIO_OP_READ == iocp_ol->opcode) {
		IocpReadOverlapped* read_ol = (IocpReadOverlapped*)ol;
		if (iocp_ol->domain != AF_UNSPEC) {
			read_ol->saddrlen = sizeof(read_ol->saddr);
			read_ol->dwFlags = 0;
			if (!WSARecvFrom((SOCKET)fd, &read_ol->wsabuf, 1, NULL, &read_ol->dwFlags, (struct sockaddr*)&read_ol->saddr, &read_ol->saddrlen, (LPWSAOVERLAPPED)ol, NULL)) {
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
			if (ReadFile((HANDLE)fd, read_ol->wsabuf.buf, read_ol->wsabuf.len, NULL, (LPOVERLAPPED)ol)) {
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
	else if (NIO_OP_WRITE == iocp_ol->opcode) {
		IocpWriteOverlapped* write_ol = (IocpWriteOverlapped*)ol;
		if (iocp_ol->domain != AF_UNSPEC) {
			const struct sockaddr* toaddr;
			if (addrlen > 0 && saddr) {
				toaddr = (const struct sockaddr*)&write_ol->saddr;
				memcpy(&write_ol->saddr, saddr, addrlen);
			}
			else {
				toaddr = NULL;
				addrlen = 0;
			}
			if (!WSASendTo((SOCKET)fd, &write_ol->wsabuf, 1, NULL, 0, toaddr, addrlen, (LPWSAOVERLAPPED)ol, NULL)) {
				write_ol->base.commit = 1;
				return TRUE;
			}
			else if (WSAGetLastError() == WSA_IO_PENDING) {
				write_ol->base.commit = 1;
				return TRUE;
			}
		}
		else {
			if (WriteFile((HANDLE)fd, NULL, 0, NULL, (LPOVERLAPPED)ol)) {
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
	else if (NIO_OP_ACCEPT == iocp_ol->opcode) {
		static LPFN_ACCEPTEX lpfnAcceptEx = NULL;
		IocpAcceptExOverlapped* accept_ol = (IocpAcceptExOverlapped*)ol;
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
			accept_ol->acceptsocket = socket(iocp_ol->domain, SOCK_STREAM, 0);
			if (INVALID_SOCKET == accept_ol->acceptsocket) {
				return FALSE;
			}
		}
		if (lpfnAcceptEx((SOCKET)fd, accept_ol->acceptsocket, accept_ol->saddrs, 0,
			sizeof(struct sockaddr_storage) + 16, sizeof(struct sockaddr_storage) + 16,
			NULL, (LPOVERLAPPED)ol))
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
	else if (NIO_OP_CONNECT == iocp_ol->opcode) {
		struct sockaddr_storage st;
		static LPFN_CONNECTEX lpfnConnectEx = NULL;
		IocpWriteOverlapped* conn_ol = (IocpWriteOverlapped*)ol;
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
		st.ss_family = iocp_ol->domain;
		if (bind((SOCKET)fd, (struct sockaddr*)&st, addrlen)) {
			return FALSE;
		}
		memcpy(&conn_ol->saddr, saddr, addrlen);
		if (lpfnConnectEx((SOCKET)fd, (const struct sockaddr*)&conn_ol->saddr, addrlen, conn_ol->wsabuf.buf, conn_ol->wsabuf.len, NULL, (LPWSAOVERLAPPED)ol)) {
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

	if (NIO_OP_READ == (size_t)ol || NIO_OP_ACCEPT == (size_t)ol) {
		event_mask |= NIO_OP_READ;
		sys_event_mask |= EPOLLIN;
	}
	else if (NIO_OP_WRITE == (size_t)ol) {
		event_mask |= NIO_OP_WRITE;
		sys_event_mask |= EPOLLOUT;
	}
	else if (NIO_OP_CONNECT == (size_t)ol) {
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
	e.events = EPOLLET | EPOLLONESHOT | sys_event_mask;
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
	if (NIO_OP_READ == (size_t)ol || NIO_OP_ACCEPT == (size_t)ol) {
		struct kevent e;
		EV_SET(&e, (uintptr_t)niofd->fd, EVFILT_READ, EV_ADD | EV_ONESHOT, 0, 0, niofd);
		return kevent(nio->__hNio, &e, 1, NULL, 0, NULL) == 0;
	}
	else if (NIO_OP_WRITE == (size_t)ol) {
		struct kevent e;
		EV_SET(&e, (uintptr_t)niofd->fd, EVFILT_WRITE, EV_ADD | EV_ONESHOT, 0, 0, niofd);
		return kevent(nio->__hNio, &e, 1, NULL, 0, NULL) == 0;
	}
	else if (NIO_OP_CONNECT == (size_t)ol) {
		struct kevent e;
		if (connect(niofd->fd, saddr, addrlen) && EINPROGRESS != errno) {
			return FALSE;
		}
		/* 
		 * fd always add kevent when connect immediately finish or not...
		 * because I need Unified handle event
		 */
		EV_SET(&e, (uintptr_t)niofd->fd, EVFILT_WRITE, EV_ADD | EV_ONESHOT, 0, 0, niofd);
		return kevent(nio->__hNio, &e, 1, NULL, 0, NULL) == 0;
	}
	else {
		errno = EINVAL;
		return FALSE;
	}
#endif
	return FALSE;
}

int nioWait(Nio_t* nio, NioEv_t* e, unsigned int count, int msec) {
#if defined(_WIN32) || defined(_WIN64)
	ULONG n;
	return GetQueuedCompletionStatusEx((HANDLE)(nio->__hNio), e, count, &n, msec, FALSE) ? n : (GetLastError() == WAIT_TIMEOUT ? 0 : -1);
#elif __linux__
	int res;
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

	if (NIO_OP_ACCEPT == iocp_ol->opcode) {
		*ev_mask = NIO_OP_READ;
	}
	else if (NIO_OP_CONNECT == iocp_ol->opcode) {
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

	return (NioFD_t*)e->lpCompletionKey;
#elif __linux__
	NioFD_t* niofd;
	if (e->data.ptr == (void*)&nio->__socketpair[0]) {
		char c[256];
		read(nio->__socketpair[0], c, sizeof(c));
		_xchg16(&nio->__wakeup, 0);
		return NULL;
	}
	niofd = (NioFD_t*)e->data.ptr;
	/* epoll must catch this event */
	if ((e->events & EPOLLRDHUP) || (e->events & EPOLLHUP)) {
		niofd->__event_mask = 0;
		*ev_mask = NIO_OP_READ;
	}
	else {
		*ev_mask = 0;
		if (e->events & EPOLLIN) {
			niofd->__event_mask &= ~NIO_OP_READ;
			*ev_mask |= NIO_OP_READ;
		}
		if (e->events & EPOLLOUT) {
			niofd->__event_mask &= ~NIO_OP_WRITE;
			*ev_mask |= NIO_OP_WRITE;
		}
	}
	return niofd;
#elif defined(__FreeBSD__) || defined(__APPLE__)
	if (e->udata == (void*)&nio->__socketpair[0]) {
		char c[256];
		read(nio->__socketpair[0], c, sizeof(c));
		_xchg16(&nio->__wakeup, 0);
		return NULL;
	}
	if (EVFILT_READ == e->filter) {
		*ev_mask = NIO_OP_READ;
	}
	else if (EVFILT_WRITE == e->filter) {
		*ev_mask = NIO_OP_WRITE;
	}
	else { /* program don't run here... */
		*ev_mask = 0;
	}
	return (NioFD_t*)e->udata;
#endif
	return NULL;
}

int nioOverlappedReadResult(void* ol, Iobuf_t* iov, struct sockaddr_storage* saddr, socklen_t* p_slen) {
#if defined(_WIN32) || defined(_WIN64)
	IocpOverlapped* iocp_ol = (IocpOverlapped*)ol;
	if (NIO_OP_READ == iocp_ol->opcode) {
		IocpReadOverlapped* iocp_read_ol = (IocpReadOverlapped*)iocp_ol;
		iov->buf = iocp_read_ol->wsabuf.buf;
		iov->len = iocp_read_ol->dwNumberOfBytesTransferred;
		if (saddr) {
			memcpy(saddr, &iocp_read_ol->saddr, iocp_read_ol->saddrlen);
			*p_slen = iocp_read_ol->saddrlen;
		}
		return 1;
	}
#endif
	iobufPtr(iov) = NULL;
	iobufLen(iov) = 0;
	if (saddr) {
		saddr->ss_family = AF_UNSPEC;
		*p_slen = 0;
	}
	return 0;
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

FD_t nioAcceptFirst(FD_t listenfd, void* ol, struct sockaddr* peer_saddr, socklen_t* p_slen) {
#if defined(_WIN32) || defined(_WIN64)
	IocpAcceptExOverlapped* iocp_ol = (IocpAcceptExOverlapped*)ol;
	SOCKET acceptfd = iocp_ol->acceptsocket;
	iocp_ol->acceptsocket = INVALID_SOCKET;
	do {
		if (setsockopt(acceptfd, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, (char*)&listenfd, sizeof(listenfd))) {
			closesocket(acceptfd);
			break;
		}
		if (getpeername(acceptfd, peer_saddr, p_slen)) {
			closesocket(acceptfd);
			break;
		}
		return acceptfd;
	} while (0);
	return INVALID_FD_HANDLE;
#else
	return accept(listenfd, peer_saddr, p_slen);
#endif
}

BOOL nioClose(Nio_t* nio) {
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
