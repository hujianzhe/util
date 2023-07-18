//
// Created by hujianzhe
//

#if	defined(_WIN32) || defined(USE_UNIX_AIO_API)

#include "../../inc/sysapi/aio.h"
#ifdef _WIN32
#include <mswsock.h>
#else
#include <sys/time.h>
#endif
#include <string.h>
#include <stdlib.h>
#include <errno.h>

static void aio_handle_free_list(Aio_t* aio) {
	AioFD_t* cur_free, * next_free;
	for (cur_free = aio->__free_list_head; cur_free; cur_free = next_free) {
		next_free = cur_free->__lnext;
		aio->__fn_free_aiofd(cur_free);
	}
	aio->__free_list_head = NULL;
}

static void free_all_ol_when_delete(IoOverlapped_t* tail) {
	IoOverlapped_t* ol, *prev_ol;
	for (ol = tail; ol; ol = prev_ol) {
		prev_ol = ol->prev;
		IoOverlapped_free(ol);
	}
}

static void aiofd_link_ol(AioFD_t* aiofd, IoOverlapped_t* ol) {
	if (aiofd->ol_list_tail) {
		aiofd->ol_list_tail->next = ol;
	}
	ol->prev = aiofd->ol_list_tail;
	ol->next = NULL;
	aiofd->ol_list_tail = ol;
}

static void aiofd_unlink_ol(AioFD_t* aiofd, IoOverlapped_t* ol) {
	if (aiofd->ol_list_tail == ol) {
		aiofd->ol_list_tail = ol->prev;
	}
	if (ol->prev) {
		ol->prev->next = ol->next;
	}
	if (ol->next) {
		ol->next->prev = ol->prev;
	}
	ol->prev = NULL;
	ol->next = NULL;
}

#ifdef	__linux__
static IoOverlapped_t s_wakeup_ol;

static int uring_filter_internal_ol__(IoOverlapped_t* ol, __u32 flags) {
	if (!ol) {
		return 1;
	}
	if (IO_OVERLAPPED_OP_INTERNAL_FD_CLOSE == ol->opcode ||
		IO_OVERLAPPED_OP_INTERNAL_SHUTDOWN == ol->opcode)
	{
		close(ol->fd);
		free(ol);
		return 1;
	}
	if (flags & IORING_CQE_F_NOTIF) {
		ol->__wait_cqe_notify = 0;
		if (ol->free_flag) {
			IoOverlapped_free(ol);
		}
		return 1;
	}
	return 0;
}

static void uring_cqe_save__(IoOverlapped_t* ol, struct io_uring_cqe* cqe) {
	if (cqe->res < 0) {
		ol->error = -cqe->res;
		ol->retval = 0;
	}
	else {
		ol->error = 0;
		ol->retval = cqe->res;
	}
	if (cqe->flags & IORING_CQE_F_MORE) {
		ol->__wait_cqe_notify = 1;
	}
}
#endif

#ifdef	__cplusplus
extern "C" {
#endif

#if _WIN32
extern BOOL win32_Iocp_PrepareRegUdp(SOCKET fd, int domain);
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

AioFD_t* aiofdInit(AioFD_t* aiofd, FD_t fd) {
#ifdef	__linux__
	aiofd->__delete_ol = (IoOverlapped_t*)malloc(sizeof(IoOverlapped_t));
	if (!aiofd->__delete_ol) {
		return NULL;
	}
#endif
	aiofd->fd = fd;
	aiofd->__lnext = NULL;
	aiofd->__delete_flag = 0;
	aiofd->__domain = 0;
	aiofd->__socktype = 0;
	aiofd->__protocol = 0;
#if defined(_WIN32) || defined(_WIN64)
	aiofd->__reg = FALSE;
#endif
	aiofd->ol_list_tail = NULL;
	return aiofd;
}

void aiofdSetSocketInfo(AioFD_t* aiofd, int domain, int socktype, int protocol) {
	aiofd->__domain = domain;
	aiofd->__socktype = socktype;
	aiofd->__protocol = protocol;
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
#elif	__linux__
	if (aiofd->__domain != AF_UNSPEC && SOCK_STREAM == aiofd->__socktype) {
		struct io_uring_sqe* sqe = io_uring_get_sqe(&aio->__r);
		if (sqe) {
			IoOverlapped_t* ol = aiofd->__delete_ol;
			ol->opcode = IO_OVERLAPPED_OP_INTERNAL_SHUTDOWN;
			ol->fd = aiofd->fd;

			io_uring_prep_shutdown(sqe, aiofd->fd, 0);
			io_uring_sqe_set_data(sqe, ol);
			io_uring_submit(&aio->__r);
		}
		else {
			free(aiofd->__delete_ol);
			shutdown(aiofd->fd, SHUT_RDWR);
			close(aiofd->fd);
		}
	}
	else {
		struct io_uring_sqe* sqe = io_uring_get_sqe(&aio->__r);
		if (sqe) {
			IoOverlapped_t* ol = aiofd->__delete_ol;
			ol->opcode = IO_OVERLAPPED_OP_INTERNAL_FD_CLOSE;
			ol->fd = aiofd->fd;

			io_uring_prep_cancel_fd(sqe, aiofd->fd, 0);
			io_uring_sqe_set_data(sqe, ol);
			io_uring_submit(&aio->__r);
		}
		else {
			free(aiofd->__delete_ol);
			close(aiofd->fd);
		}
	}
	aiofd->__delete_ol = NULL;
	aiofd->fd = -1;
#else
	close(aiofd->fd);
	aiofd->fd = -1;
#endif
	aiofd->__delete_flag = 1;
	aiofd->__lnext = aio->__free_list_head;
	aio->__free_list_head = aiofd;
	free_all_ol_when_delete(aiofd->ol_list_tail);
	aiofd->ol_list_tail = NULL;
}

BOOL aioCommit(Aio_t* aio, AioFD_t* aiofd, IoOverlapped_t* ol, struct sockaddr* saddr, socklen_t addrlen) {
#if defined(_WIN32) || defined(_WIN64)
	int fd_domain = aiofd->__domain;
	FD_t fd = aiofd->fd;
	if (aiofd->__delete_flag) {
		return FALSE;
	}
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
				if (!win32_Iocp_PrepareRegUdp(fd, fd_domain)) {
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
		WSABUF* ptr_wsabuf = &read_ol->base.iobuf;
		if (fd_domain != AF_UNSPEC) {
			read_ol->saddrlen = sizeof(read_ol->saddr);
			read_ol->dwFlags = 0;
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
	}
	else if (IO_OVERLAPPED_OP_WRITE == ol->opcode) {
		IocpWriteOverlapped_t* write_ol = (IocpWriteOverlapped_t*)ol;
		WSABUF* ptr_wsabuf = &write_ol->base.iobuf;
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
	}
	else if (IO_OVERLAPPED_OP_CONNECT == ol->opcode) {
		static LPFN_CONNECTEX lpfnConnectEx = NULL;
		struct sockaddr_storage st;
		IocpConnectExOverlapped_t* conn_ol = (IocpConnectExOverlapped_t*)ol;
		WSABUF* ptr_wsabuf;
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
		ptr_wsabuf = &conn_ol->base.iobuf;
		if (!lpfnConnectEx((SOCKET)fd, (const struct sockaddr*)&conn_ol->saddr, addrlen, ptr_wsabuf->buf, ptr_wsabuf->len, NULL, (LPWSAOVERLAPPED)&conn_ol->base.ol)) {
			if (WSAGetLastError() != ERROR_IO_PENDING) {
				return FALSE;
			}
		}
	}
	else {
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}
	ol->commit = 1;
	ol->completion_key = aiofd;
	aiofd_link_ol(aiofd, ol);
	return TRUE;
#elif	__linux__
	struct io_uring_sqe* sqe;
	if (aiofd->__delete_flag) {
		return 0;
	}
	if (ol->commit) {
		return 1;
	}
	if (IO_OVERLAPPED_OP_READ == ol->opcode) {
		UnixReadOverlapped_t* read_ol = (UnixReadOverlapped_t*)ol;
		sqe = io_uring_get_sqe(&aio->__r);
		if (!sqe) {
			return 0;
		}
		if (aiofd->__domain != AF_UNSPEC) {
			read_ol->msghdr.msg_name = &read_ol->saddr;
			read_ol->msghdr.msg_namelen = sizeof(read_ol->saddr);

			io_uring_prep_recvmsg(sqe, aiofd->fd, &read_ol->msghdr, 0);
		}
		else {
			io_uring_prep_readv(sqe, aiofd->fd, read_ol->msghdr.msg_iov, read_ol->msghdr.msg_iovlen, read_ol->offset);
		}
	}
	else if (IO_OVERLAPPED_OP_WRITE == ol->opcode) {
		UnixWriteOverlapped_t* write_ol = (UnixWriteOverlapped_t*)ol;
		sqe = io_uring_get_sqe(&aio->__r);
		if (!sqe) {
			return 0;
		}
		if (aiofd->__domain != AF_UNSPEC) {
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
		else {
			io_uring_prep_writev(sqe, aiofd->fd, write_ol->msghdr.msg_iov, write_ol->msghdr.msg_iovlen, write_ol->offset);
		}
	}
	else if (IO_OVERLAPPED_OP_CONNECT == ol->opcode) {
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
	aiofd_link_ol(aiofd, ol);
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
	unsigned head, advance_n;
	IoOverlapped_t* ol;
	struct io_uring_cqe* cqe, **cqes;
	struct __kernel_timespec kt;

	aio_handle_free_list(aio);

	if (msec >= 0) {
		long long start_tm;
		struct timeval tval;
		if (gettimeofday(&tval, NULL)) {
			return -1;
		}
		start_tm = tval.tv_sec;
		start_tm *= 1000;
		start_tm += tval.tv_usec / 1000;
		while (1) {
			long long tm, delta_tlen;
			kt.tv_sec = msec / 1000;
			kt.tv_nsec = msec % 1000;
			kt.tv_nsec *= 1000000LL;
			ret = io_uring_wait_cqe_timeout(&aio->__r, &cqe, &kt);
			if (ret != 0) {
				if (ETIME == -ret) {
					return 0;
				}
				errno = -ret;
				return -1;
			}
			ol = (IoOverlapped_t*)io_uring_cqe_get_data(cqe);
			if (!uring_filter_internal_ol__(ol, cqe->flags)) {
				break;
			}
			io_uring_cqe_seen(&aio->__r, cqe);
			if (gettimeofday(&tval, NULL)) {
				return -1;
			}
			tm = tval.tv_sec;
			tm *= 1000;
			tm += tval.tv_usec / 1000;
			delta_tlen = tm - start_tm;
			if (delta_tlen >= msec || delta_tlen < 0) {
				return 0;
			}
			msec -= delta_tlen;
		}
	}
	else {
		while (1) {
			ret = io_uring_wait_cqe_timeout(&aio->__r, &cqe, NULL);
			if (ret != 0) {
				if (ETIME == -ret) {
					return 0;
				}
				errno = -ret;
				return -1;
			}
			ol = (IoOverlapped_t*)io_uring_cqe_get_data(cqe);
			if (!uring_filter_internal_ol__(ol, cqe->flags)) {
				break;
			}
			io_uring_cqe_seen(&aio->__r, cqe);
		}
	}
	uring_cqe_save__(ol, cqe);
	io_uring_cqe_seen(&aio->__r, cqe);
	e[0].ol = ol;
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
	kt.tv_sec = 0;
	kt.tv_nsec = 0;
	ret = io_uring_wait_cqes(&aio->__r, cqes, n, &kt, NULL);
	if (ret != 0) {
		if (ETIME == -ret) {
			return 1;
		}
		errno = -ret;
		return -1;
	}
	n = 1;
	advance_n = 0;
	io_uring_for_each_cqe(&aio->__r, head, cqe) {
		advance_n++;
		ol = (IoOverlapped_t*)io_uring_cqe_get_data(cqe);
		if (uring_filter_internal_ol__(ol, cqe->flags)) {
			continue;
		}
		uring_cqe_save__(ol, cqe);
		e[n].ol = ol;
		n++;
	}
	io_uring_cq_advance(&aio->__r, advance_n);
	return n;
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
		io_uring_sqe_set_data(sqe, &s_wakeup_ol);
		io_uring_submit(&aio->__r);
#endif
	}
}

IoOverlapped_t* aioEventCheck(Aio_t* aio, const AioEv_t* e) {
#if defined(_WIN32) || defined(_WIN64)
	AioFD_t* aiofd;
	IoOverlapped_t* ol = (IoOverlapped_t*)e->lpOverlapped;
	if (!ol) {
		_xchg16(&aio->__wakeup, 0);
		return NULL;
	}
	if (ol->free_flag) {
		free(ol);
		return NULL;
	}
	ol->commit = 0;

	aiofd = (AioFD_t*)e->lpCompletionKey;
	aiofd_unlink_ol(aiofd, ol);

	ol->transfer_bytes = e->dwNumberOfBytesTransferred;
	if (NT_ERROR(e->Internal)) {
		ol->error = e->Internal;
	}
	else {
		ol->error = 0;
	}

	return ol;
#elif	__linux__
	AioFD_t* aiofd;
	IoOverlapped_t* ol = (IoOverlapped_t*)e->ol;
	if (&s_wakeup_ol == ol) {
		_xchg16(&aio->__wakeup, 0);
		return NULL;
	}
	if (ol->free_flag) {
		free(ol);
		return NULL;
	}
	ol->commit = 0;

	aiofd = (AioFD_t*)ol->completion_key;
	aiofd_unlink_ol(aiofd, ol);

	return ol;
#else
	return NULL;
#endif
}

#ifdef	__cplusplus
}
#endif

#endif
