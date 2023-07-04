//
// Created by hujianzhe
//

#if	defined(_WIN32) || defined(USE_UNIX_AIO_API)

#include "../../inc/sysapi/aio.h"
#ifdef _WIN32
#include <mswsock.h>
#endif
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#if defined(_WIN32) || defined(_WIN64)
extern BOOL Iocp_PrepareRegUdp(SOCKET fd, int domain);
#endif

static void aio_handle_free_list(Aio_t* aio) {
	AioFD_t* cur_free, * next_free;
	for (cur_free = aio->__free_list_head; cur_free; cur_free = next_free) {
		next_free = cur_free->__lnext;
		aio->__fn_free_aiofd(cur_free);
	}
	aio->__free_list_head = NULL;
}

#ifdef	__cplusplus
extern "C" {
#endif

Aio_t* aioCreate(Aio_t* aio, void(*fn_free_aiofd)(AioFD_t*)) {
#if defined(_WIN32) || defined(_WIN64)
	SYSTEM_INFO si;
	GetSystemInfo(&si);
	aio->__handle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, si.dwNumberOfProcessors << 1);
	if (!aio->__handle) {
		return NULL;
	}
#elif	__linux__
	int ret = io_uring_queue_init(16, &aio->__r, 0);
	if (ret < 0) {
		errno = -ret;
		return NULL;
	}
	aio->__wait_cqes = NULL;
	aio->__wait_cqes_cnt = 0;
#else
	return NULL;
#endif
	aio->__wakeup = 0;
	aio->__free_list_head = NULL;
	aio->__fn_free_aiofd = fn_free_aiofd;
	return aio;
}

BOOL aioClose(Aio_t* aio) {
	aio_handle_free_list(aio);
#if defined(_WIN32) || defined(_WIN64)
	return CloseHandle(aio->__handle);
#elif	__linux__
	io_uring_queue_exit(&aio->__r);
	free(aio->__wait_cqes);
	aio->__wait_cqes_cnt = 0;
#endif
	return 1;
}

void aiofdInit(AioFD_t* aiofd, FD_t fd, int domain) {
	aiofd->fd = fd;
	aiofd->__lnext = NULL;
	aiofd->__delete_flag = 0;
	aiofd->__domain = domain;
#if defined(_WIN32) || defined(_WIN64)
	aiofd->__reg = FALSE;
#endif
}

void aiofdDelete(Aio_t* aio, AioFD_t* aiofd) {
	if (aiofd->__delete_flag) {
		return;
	}
#if defined(_WIN32) || defined(_WIN64)
	if (aiofd->__domain != AF_UNSPEC) {
		closesocket(aiofd->fd);
		aiofd->fd = INVALID_SOCKET;
	}
	else {
		CloseHandle((HANDLE)aiofd->fd);
		aiofd->fd = (FD_t)INVALID_HANDLE_VALUE;
	}
#else
	close(aiofd->fd);
	aiofd->fd = -1;
#endif
	aiofd->__lnext = aio->__free_list_head;
	aio->__free_list_head = aiofd;
	aiofd->__delete_flag = 1;
}

BOOL aioCommit(Aio_t* aio, AioFD_t* aiofd, IoOverlapped_t* ol, struct sockaddr* saddr, socklen_t addrlen) {
#if defined(_WIN32) || defined(_WIN64)
	int fd_domain = aiofd->__domain;
	FD_t fd = aiofd->fd;
	if (ol->commit) {
		return TRUE;
	}
	if (!aiofd->__reg) {
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
		if (CreateIoCompletionPort((HANDLE)fd, aio->__handle, (ULONG_PTR)aiofd, 0) != aio->__handle) {
			return FALSE;
		}
		aiofd->__reg = TRUE;
	}
	if (IO_OVERLAPPED_OP_READ == ol->opcode) {
		IocpReadOverlapped_t* read_ol = (IocpReadOverlapped_t*)ol;
		if (fd_domain != AF_UNSPEC) {
			read_ol->saddrlen = sizeof(read_ol->saddr);
			read_ol->dwFlags = 0;
			if (WSARecvFrom((SOCKET)fd, &read_ol->wsabuf, 1, NULL, &read_ol->dwFlags, (struct sockaddr*)&read_ol->saddr, &read_ol->saddrlen, (LPWSAOVERLAPPED)&read_ol->base.ol, NULL)) {
				if (WSAGetLastError() != WSA_IO_PENDING) {
					return FALSE;
				}
			}
		}
		else {
			if (!ReadFile((HANDLE)fd, read_ol->wsabuf.buf, read_ol->wsabuf.len, NULL, (LPOVERLAPPED)&read_ol->base.ol)) {
				if (GetLastError() != ERROR_IO_PENDING) {
					return FALSE;
				}
			}
		}
		read_ol->base.commit = 1;
		read_ol->base.completion_key = aiofd;
		return TRUE;
	}
	else if (IO_OVERLAPPED_OP_WRITE == ol->opcode) {
		IocpWriteOverlapped_t* write_ol = (IocpWriteOverlapped_t*)ol;
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
			if (WSASendTo((SOCKET)fd, &write_ol->wsabuf, 1, NULL, 0, toaddr, addrlen, (LPWSAOVERLAPPED)&write_ol->base.ol, NULL)) {
				if (WSAGetLastError() != WSA_IO_PENDING) {
					return FALSE;
				}
			}
		}
		else {
			if (!WriteFile((HANDLE)fd, write_ol->wsabuf.buf, write_ol->wsabuf.len, NULL, (LPOVERLAPPED)&write_ol->base.ol)) {
				if (GetLastError() != ERROR_IO_PENDING) {
					return FALSE;
				}
			}
		}
		write_ol->base.commit = 1;
		write_ol->base.completion_key = aiofd;
		return TRUE;
	}
	else if (IO_OVERLAPPED_OP_ACCEPT == ol->opcode) {
		static LPFN_ACCEPTEX lpfnAcceptEx = NULL;
		IocpAcceptExOverlapped_t* accept_ol = (IocpAcceptExOverlapped_t*)ol;
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
		accept_ol->acceptsocket = socket(fd_domain, SOCK_STREAM, 0);
		if (INVALID_SOCKET == accept_ol->acceptsocket) {
			return FALSE;
		}
		if (!lpfnAcceptEx((SOCKET)fd, accept_ol->acceptsocket, accept_ol->saddrs, 0,
			sizeof(struct sockaddr_storage) + 16, sizeof(struct sockaddr_storage) + 16,
			NULL, (LPOVERLAPPED)&accept_ol->base.ol))
		{
			if (WSAGetLastError() != ERROR_IO_PENDING) {
				closesocket(accept_ol->acceptsocket);
				accept_ol->acceptsocket = INVALID_SOCKET;
				return FALSE;
			}
		}
		accept_ol->base.commit = 1;
		accept_ol->base.completion_key = aiofd;
		return TRUE;
	}
	else if (IO_OVERLAPEED_OP_CONNECT == ol->opcode) {
		static LPFN_CONNECTEX lpfnConnectEx = NULL;
		struct sockaddr_storage st;
		IocpConnectExOverlapped_t* conn_ol = (IocpConnectExOverlapped_t*)ol;
		/* ConnectEx must use really namelen, otherwise report WSAEADDRNOTAVAIL(10049) */
		if (!lpfnConnectEx) {
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
		if (!lpfnConnectEx((SOCKET)fd, (const struct sockaddr*)&conn_ol->saddr, addrlen, conn_ol->wsabuf.buf, conn_ol->wsabuf.len, NULL, (LPWSAOVERLAPPED)&conn_ol->base.ol)) {
			if (WSAGetLastError() != ERROR_IO_PENDING) {
				return FALSE;
			}
		}
		conn_ol->base.commit = 1;
		conn_ol->base.completion_key = aiofd;
		return TRUE;
	}
	else {
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}
#elif	__linux__
	struct io_uring_sqe* sqe;
	if (ol->commit) {
		return 1;
	}
	if (IO_OVERLAPPED_OP_READ == ol->opcode) {
		UnixReadOverlapped_t* read_ol = (UnixReadOverlapped_t*)ol;
		sqe = io_uring_get_sqe(&aio->__r);
		if (!sqe) {
			return 0;
		}

		read_ol->msghdr.msg_name = &read_ol->saddr;
		read_ol->msghdr.msg_namelen = sizeof(read_ol->saddr);

		io_uring_prep_recvmsg(sqe, aiofd->fd, &read_ol->msghdr, 0);
	}
	else if (IO_OVERLAPPED_OP_WRITE == ol->opcode) {
		UnixWriteOverlapped_t* write_ol = (UnixWriteOverlapped_t*)ol;
		sqe = io_uring_get_sqe(&aio->__r);
		if (!sqe) {
			return 0;
		}

		if (addrlen > 0 && saddr) {
			memmove(&write_ol->saddr, saddr, addrlen);
			write_ol->msghdr.msg_name = &write_ol->saddr;
			write_ol->msghdr.msg_namelen = addrlen;
		}
		else {
			write_ol->msghdr.msg_name = NULL;
			write_ol->msghdr.msg_namelen = 0;
		}

		io_uring_prep_sendmsg(sqe, aiofd->fd, &write_ol->msghdr, 0);
	}
	else if (IO_OVERLAPEED_OP_CONNECT == ol->opcode) {
		UnixConnectOverlapped_t* conn_ol = (UnixConnectOverlapped_t*)ol;
		sqe = io_uring_get_sqe(&aio->__r);
		if (!sqe) {
			return 0;
		}

		memmove(&conn_ol->saddr, saddr, addrlen);
		conn_ol->saddrlen = addrlen;

		io_uring_prep_connect(sqe, aiofd->fd, (const struct sockaddr*)&conn_ol->saddr, conn_ol->saddrlen);
	}
	else if (IO_OVERLAPPED_OP_ACCEPT == ol->opcode) {
		UnixAcceptOverlapped_t* accept_ol = (UnixAcceptOverlapped_t*)ol;
		sqe = io_uring_get_sqe(&aio->__r);
		if (!sqe) {
			return 0;
		}

		accept_ol->saddrlen = sizeof(accept_ol->saddr);

		io_uring_prep_accept(sqe, aiofd->fd, (struct sockaddr*)&accept_ol->saddr, &accept_ol->saddrlen, 0);
	}
	else {
		errno = EOPNOTSUPP;
		return 0;
	}
	ol->commit = 1;
	ol->completion_key = aiofd;
	io_uring_sqe_set_data(sqe, ol);
	io_uring_submit(&aio->__r);
	return 1;
#else
	errno = ENOSYS;
	return 0;
#endif
}

int aioWait(Aio_t* aio, AioEv_t* e, unsigned int n, int msec) {
#if defined(_WIN32) || defined(_WIN64)
	ULONG cnt;
	aio_handle_free_list(aio);
	if (GetQueuedCompletionStatusEx(aio->__handle, e, n, &cnt, msec, FALSE)) {
		return cnt;
	}
	if (GetLastError() == WAIT_TIMEOUT) {
		return 0;
	}
	return -1;
#elif	__linux__
	int ret;
	unsigned head;
	struct io_uring_cqe* cqe, **cqes;
	struct __kernel_timespec tval, *t;
	if (msec >= 0) {
		tval.tv_sec = msec / 1000;
		tval.tv_nsec = msec % 1000;
		tval.tv_nsec *= 1000000LL;
		t = &tval;
	}
	else {
		t = NULL;
	}
	aio_handle_free_list(aio);

	ret = io_uring_wait_cqe_timeout(&aio->__r, &cqe, t);
	if (ret != 0) {
		if (ETIME == -ret) {
			return 0;
		}
		errno = -ret;
		return -1;
	}
	e[0].ol = (IoOverlapped_t*)io_uring_cqe_get_data(cqe);
	if (e[0].ol) {
		e[0].ol->result = cqe->res;
	}
	io_uring_cqe_seen(&aio->__r, cqe);
	if (n <= 1) {
		return 1;
	}
	n -= 1;
	if (n > aio->__wait_cqes_cnt) {
		cqes = (struct io_uring_cqe**)realloc(aio->__wait_cqes, sizeof(aio->__wait_cqes[0]) * n);
		if (cqes) {
			aio->__wait_cqes = cqes;
			aio->__wait_cqes_cnt = n;
		}
		else if (!aio->__wait_cqes) {
			return 1;
		}
		else {
			cqes = aio->__wait_cqes;
			n = aio->__wait_cqes_cnt;
		}
	}
	else {
		cqes = aio->__wait_cqes;
	}
	tval.tv_sec = 0;
	tval.tv_nsec = 0;
	ret = io_uring_wait_cqes(&aio->__r, cqes, n, &tval, NULL);
	if (ret != 0) {
		if (ETIME == -ret) {
			return 1;
		}
		errno = -ret;
		return -1;
	}
	n = 0;
	io_uring_for_each_cqe(&aio->__r, head, cqe) {
		e[n].ol = (IoOverlapped_t*)io_uring_cqe_get_data(cqe);
		if (e[n].ol) {
			e[n].ol->result = cqe->res;
		}
		++n;
	}
	io_uring_cq_advance(&aio->__r, n);
	return n + 1;
#else
	errno = ENOSYS;
	return -1;
#endif
}

void aioWakeup(Aio_t* aio) {
	if (0 == _xchg16(&aio->__wakeup, 1)) {
#if defined(_WIN32) || defined(_WIN64)
		PostQueuedCompletionStatus(aio->__handle, 0, 0, NULL);
#elif	__linux__
		struct io_uring_sqe* sqe = io_uring_get_sqe(&aio->__r);
		if (!sqe) {
			_xchg16(&aio->__wakeup, 0);
			return;
		}
		io_uring_prep_nop(sqe);
		io_uring_submit(&aio->__r);
#endif
	}
}

IoOverlapped_t* aioEventCheck(Aio_t* aio, const AioEv_t* e) {
#if defined(_WIN32) || defined(_WIN64)
	IoOverlapped_t* ol = (IoOverlapped_t*)e->lpOverlapped;
#else
	IoOverlapped_t* ol = (IoOverlapped_t*)e->ol;
#endif
	if (!ol) {
		_xchg16(&aio->__wakeup, 0);
		return NULL;
	}
	if (ol->free_flag) {
		free(ol);
		return NULL;
	}
	ol->commit = 0;
#if defined(_WIN32) || defined(_WIN64)
	if (IO_OVERLAPPED_OP_ACCEPT == ol->opcode) {
		AioFD_t* aiofd = (AioFD_t*)e->lpCompletionKey;
		IocpAcceptExOverlapped_t* accept_ol = (IocpAcceptExOverlapped_t*)ol;
		if (setsockopt(accept_ol->acceptsocket, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, (char*)&aiofd->fd, sizeof(aiofd->fd))) {
			closesocket(accept_ol->acceptsocket);
			accept_ol->acceptsocket = INVALID_SOCKET;
		}
	}
	else if (IO_OVERLAPEED_OP_CONNECT == ol->opcode) {
		AioFD_t* aiofd = (AioFD_t*)e->lpCompletionKey;
		IocpConnectExOverlapped_t* conn_ol = (IocpConnectExOverlapped_t*)ol;
		do {
			int sec;
			int len = sizeof(sec);
			if (getsockopt(aiofd->fd, SOL_SOCKET, SO_CONNECT_TIME, (char*)&sec, &len)) {
				break;
			}
			if (~0 == sec) {
				//SetLastError(ERROR_TIMEOUT);
				break;
			}
			if (setsockopt(aiofd->fd, SOL_SOCKET, SO_UPDATE_CONNECT_CONTEXT, NULL, 0)) {
				break;
			}
		} while (0);
		conn_ol->dwNumberOfBytesTransferred = e->dwNumberOfBytesTransferred;
	}
	else if (IO_OVERLAPPED_OP_READ == ol->opcode) {
		IocpReadOverlapped_t* read_ol = (IocpReadOverlapped_t*)ol;
		read_ol->dwNumberOfBytesTransferred = e->dwNumberOfBytesTransferred;
	}
	else if (IO_OVERLAPPED_OP_WRITE == ol->opcode) {
		IocpWriteOverlapped_t* write_ol = (IocpWriteOverlapped_t*)ol;
		write_ol->dwNumberOfBytesTransferred = e->dwNumberOfBytesTransferred;
	}
#endif
	return ol;
}

#ifdef	__cplusplus
}
#endif

#endif