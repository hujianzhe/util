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

static void aio_handle_free_dead(Aio_t* aio) {
	AioFD_t* cur, * next;
	for (cur = aio->__dead_list_head; cur; cur = next) {
		next = cur->__lnext;
		aio->__fn_free_aiofd(cur);
	}
	aio->__dead_list_head = NULL;
}

static void aiofd_link_pending_ol(AioFD_t* aiofd, IoOverlapped_t* ol) {
	if (aiofd->__ol_pending_list_tail) {
		aiofd->__ol_pending_list_tail->__next = ol;
	}
	ol->__prev = aiofd->__ol_pending_list_tail;
	ol->__next = NULL;
	aiofd->__ol_pending_list_tail = ol;
}

static void aiofd_unlink_pending_ol(AioFD_t* aiofd, IoOverlapped_t* ol) {
	if (aiofd->__ol_pending_list_tail == ol) {
		aiofd->__ol_pending_list_tail = ol->__prev;
	}
	if (ol->__prev) {
		ol->__prev->__next = ol->__next;
	}
	if (ol->__next) {
		ol->__next->__prev = ol->__prev;
	}
}

static void aiofd_free_all_pending_ol(AioFD_t* aiofd) {
	IoOverlapped_t* ol, *prev_ol;
	for (ol = aiofd->__ol_pending_list_tail; ol; ol = prev_ol) {
		prev_ol = ol->__prev;
		IoOverlapped_free(ol);
	}
}

static void aio_ol_acked(Aio_t* aio, AioFD_t* aiofd, IoOverlapped_t* ol, int enter_dead_list) {
	aiofd_unlink_pending_ol(aiofd, ol);
	if (aiofd->__delete_flag && !aiofd->__ol_pending_list_tail) {
	#if defined(_WIN32) || defined(_WIN64)
		if (aiofd->domain != AF_UNSPEC) {
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
		/* remove from delete list */
		if (aiofd == aio->__delete_list_head) {
			aio->__delete_list_head = aiofd->__lnext;
		}
		if (aiofd->__lprev) {
			aiofd->__lprev->__lnext = aiofd->__lnext;
		}
		if (aiofd->__lnext) {
			aiofd->__lnext->__lprev = aiofd->__lprev;
		}

		if (enter_dead_list) {
			aiofd->__lprev = NULL;
			aiofd->__lnext = aio->__dead_list_head;
			aio->__dead_list_head = aiofd;
		}
		else {
			aio->__fn_free_aiofd(aiofd);
		}
	}
}

#ifdef	__cplusplus
extern "C" {
#endif

#if _WIN32
extern BOOL win32_Iocp_PrepareRegUdp(SOCKET fd, int domain);
extern int win32_OverlappedConnectUpdate(SOCKET fd);
#endif

#ifdef	__cplusplus
}
#endif

static int aio_regfd(Aio_t* aio, AioFD_t* aiofd) {
	if (aiofd->__reg) {
		return 1;
	}
#if defined(_WIN32) || defined(_WIN64)
	if (SOCK_DGRAM == aiofd->socktype && AF_UNSPEC != aiofd->domain) {
		if (!win32_Iocp_PrepareRegUdp(aiofd->fd, aiofd->domain)) {
			return 0;
		}
	}
	if (CreateIoCompletionPort((HANDLE)aiofd->fd, aio->__handle, (ULONG_PTR)aiofd, 0) != aio->__handle) {
		return 0;
	}
#endif
	aiofd->__reg = 1;
	/* insert into alive list */
	aiofd->__lprev = NULL;
	aiofd->__lnext = aio->__alive_list_head;
	if (aio->__alive_list_head) {
		aio->__alive_list_head->__lprev = aiofd;
	}
	aio->__alive_list_head = aiofd;
	return 1;
}

#ifdef	__linux__
static IoOverlapped_t s_wakeup_ol;
static uint64_t s_wakeup_buf;

static int uring_submit_wakeup__(struct io_uring* r, int fd) {
	struct io_uring_sqe* sqe;
	sqe = io_uring_get_sqe(r);
	if (!sqe) {
		return 0;
	}
	io_uring_prep_read(sqe, fd, &s_wakeup_buf, sizeof(s_wakeup_buf), 0);
	io_uring_sqe_set_data(sqe, &s_wakeup_ol);
	io_uring_submit(r);
	return 1;
}

static int uring_filter_internal_ol__(Aio_t* aio, IoOverlapped_t* ol, __u32 flags) {
	if (!ol) {
		return 1;
	}
	if (IO_OVERLAPPED_OP_INTERNAL_FD_CLOSE == ol->opcode) {
		aio_ol_acked(aio, (AioFD_t*)ol->__completion_key, ol, 0);
		ol->commit = 0;
		IoOverlapped_free(ol);
		return 1;
	}
	if (flags & IORING_CQE_F_NOTIF) {
		ol->__wait_cqe_notify = 0;
		if (!ol->commit) {
			aio_ol_acked(aio, (AioFD_t*)ol->__completion_key, ol, 0);
			IoOverlapped_free(ol);
		}
		return 1;
	}
	return 0;
}

static void uring_cqe_save__(IoOverlapped_t* ol, struct io_uring_cqe* cqe) {
	if (cqe->res < 0) {
		ol->error = -cqe->res;
		if (IO_OVERLAPPED_OP_ACCEPT == ol->opcode) {
			ol->__fd = -1;
		}
	}
	else {
		ol->error = 0;
		if (IO_OVERLAPPED_OP_READ == ol->opcode || IO_OVERLAPPED_OP_WRITE == ol->opcode) {
			ol->transfer_bytes = cqe->res;
		}
		else if (IO_OVERLAPPED_OP_ACCEPT == ol->opcode) {
			ol->__fd = cqe->res;
		}
		else {
			ol->retval = cqe->res;
		}
	}
	if (cqe->flags & IORING_CQE_F_MORE) {
		ol->__wait_cqe_notify = 1;
	}
}

static int uring_submit_async_cancel__(struct io_uring* r, AioFD_t* aiofd) {
	struct io_uring_sqe* sqe;
	IoOverlapped_t* ol;

	ol = (IoOverlapped_t*)calloc(1, sizeof(IoOverlapped_t));
	if (!ol) {
		return 0;
	}
	sqe = io_uring_get_sqe(r);
	if (!sqe) {
		IoOverlapped_free(ol);
		return 0;
	}
	ol->opcode = IO_OVERLAPPED_OP_INTERNAL_FD_CLOSE;
	ol->__fd = aiofd->fd;
	ol->__completion_key = aiofd;
	ol->commit = 1;
	aiofd_link_pending_ol(aiofd, ol);

	io_uring_prep_cancel_fd(sqe, aiofd->fd, IORING_ASYNC_CANCEL_ALL);
	io_uring_sqe_set_data(sqe, ol);
	io_uring_submit(r);
	return 1;
}

static void uring_cqe_exit_clean__(Aio_t* aio, struct io_uring_cqe* cqe) {
	IoOverlapped_t* ol = (IoOverlapped_t*)io_uring_cqe_get_data(cqe);
	if (&s_wakeup_ol == ol) {
		return;
	}
	if (uring_filter_internal_ol__(aio, ol, cqe->flags)) {
		return;
	}
	ol->commit = 0;
	if (cqe->flags & IORING_CQE_F_MORE) {
		ol->__wait_cqe_notify = 1;
		return;
	}
	aio_ol_acked(aio, (AioFD_t*)ol->__completion_key, ol, 0);
	IoOverlapped_free(ol);
}

static void uring_aio_exit_clean__(Aio_t* aio) {
	unsigned sleep_peek_cnt = 128;
	AioFD_t* aiofd, *aiofd_next;
	for (aiofd = aio->__alive_list_head; aiofd; aiofd = aiofd_next) {
		aiofd_next = aiofd->__lnext;
		aiofdDelete(aio, aiofd);
	}
	aio_handle_free_dead(aio);
	while (aio->__delete_list_head) {
		int ret;
		unsigned head, advance_n;
		struct io_uring_cqe* cqe;
		/* avoid cpu busy */
		if (sleep_peek_cnt <= 0) {
			sleep_peek_cnt = 128;
			usleep(5000);
		}
		/* scan left */
		advance_n = 0;
		io_uring_for_each_cqe(&aio->__r, head, cqe) {
			advance_n++;
			uring_cqe_exit_clean__(aio, cqe);
			if (advance_n >= sleep_peek_cnt) {
				break;
			}
		}
		io_uring_cq_advance(&aio->__r, advance_n);
		sleep_peek_cnt -= advance_n;
		if (sleep_peek_cnt <= 0 || advance_n > 0) {
			continue;
		}
		/* wait more */
		ret = io_uring_wait_cqe(&aio->__r, &cqe);
		if (ret != 0) {
			/*
			if (EINTR == -ret) {
				continue;
			}
			*/
			errno = -ret;
			return;
		}
		if (!cqe) {
			continue;
		}
		uring_cqe_exit_clean__(aio, cqe);
		io_uring_cqe_seen(&aio->__r, cqe);
		sleep_peek_cnt -= 1;
	}
}
#endif

#if	_WIN32
static void iocp_aio_exit_clean__(Aio_t* aio) {
	AioFD_t* aiofd, *aiofd_next;
	for (aiofd = aio->__alive_list_head; aiofd; aiofd = aiofd_next) {
		aiofd_next = aiofd->__lnext;
		aiofdDelete(aio, aiofd);
	}
	aio_handle_free_dead(aio);
	while (aio->__delete_list_head) {
		ULONG i, cnt;
		OVERLAPPED_ENTRY e[128];
		if (!GetQueuedCompletionStatusEx(aio->__handle, e, sizeof(e) / sizeof(e[0]), &cnt, 0, FALSE)) {
			if (GetLastError() == WAIT_TIMEOUT) {
				continue;
			}
			break;
		}
		for (i = 0; i < cnt; ++i) {
			IoOverlapped_t* ol = (IoOverlapped_t*)e[i].lpOverlapped;
			if (!ol) {
				continue;
			}
			aio_ol_acked(aio, (AioFD_t*)e[i].lpCompletionKey, ol, 0);
			ol->commit = 0;
			IoOverlapped_free(ol);
		}
		Sleep(5); /* avoid cpu busy */
	}
}
#endif

static void ol_stream_push_back(AioOverlappedStream_t* s, IoOverlapped_t* ol) {
	if (!s->head) {
		s->head = ol;
	}
	if (s->tail) {
		s->tail->__next = ol;
	}
	ol->__prev = s->tail;
	ol->__next = NULL;
	s->tail = ol;
}

static void ol_stream_push_front(AioOverlappedStream_t* s, IoOverlapped_t* ol) {
	if (!s->tail) {
		s->tail = ol;
	}
	if (s->head) {
		s->head->__prev = ol;
	}
	ol->__next = s->head;
	ol->__prev = NULL;
	s->head = ol;
}

static IoOverlapped_t* ol_stream_pop_front(AioOverlappedStream_t* s) {
	IoOverlapped_t* ol = s->head;
	if (!ol) {
		return NULL;
	}
	if (s->tail == s->head) {
		s->head = s->tail = NULL;
		return ol;
	}
	s->head = s->head->__next;
	s->head->__prev = NULL;
	return ol;
}

static void ol_stream_destroy(AioOverlappedStream_t* s) {
	IoOverlapped_t* ol = s->head;
	while (ol) {
		IoOverlapped_t* next_ol = ol->__next;
		IoOverlapped_free(ol);
		ol = next_ol;
	}
	s->head = s->tail = s->running = NULL;
}

#ifdef	__cplusplus
extern "C" {
#endif

Aio_t* aioCreate(Aio_t* aio, void(*fn_free_aiofd)(AioFD_t*), unsigned int entries) {
#if defined(_WIN32) || defined(_WIN64)
	SYSTEM_INFO si;
	GetSystemInfo(&si);
	if (entries > si.dwNumberOfProcessors * 2) {
		entries = si.dwNumberOfProcessors * 2;
	}
	aio->__handle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, entries);
	if (!aio->__handle) {
		return NULL;
	}
#elif	__linux__
	int evfds[2], ret;
	struct io_uring_params p;

	if (pipe(evfds)) {
		return NULL;
	}
	memset(&p, 0, sizeof(p));
	p.flags = 	IORING_SETUP_CLAMP | IORING_SETUP_SUBMIT_ALL |
				IORING_SETUP_COOP_TASKRUN | IORING_SETUP_TASKRUN_FLAG;
	ret = io_uring_queue_init_params(entries, &aio->__r, &p);
	if (ret < 0) {
		close(evfds[0]);
		close(evfds[1]);
		errno = -ret;
		return NULL;
	}
	if (!(p.features & IORING_FEAT_NODROP) ||
		!(p.features & IORING_FEAT_SUBMIT_STABLE))
	{
		io_uring_queue_exit(&aio->__r);
		close(evfds[0]);
		close(evfds[1]);
		errno = ENOSYS;
		return NULL;
	}
	if (!uring_submit_wakeup__(&aio->__r, evfds[0])) {
		int e = errno;
		io_uring_queue_exit(&aio->__r);
		close(evfds[0]);
		close(evfds[1]);
		errno = e;
		return NULL;
	}
	aio->__wakeup_fds[0] = evfds[0];
	aio->__wakeup_fds[1] = evfds[1];
#else
	return NULL;
#endif
	aio->__wakeup = 0;
	aio->__alive_list_head = NULL;
	aio->__delete_list_head = NULL;
	aio->__dead_list_head = NULL;
	aio->__fn_free_aiofd = fn_free_aiofd;
	return aio;
}

BOOL aioClose(Aio_t* aio) {
#if defined(_WIN32) || defined(_WIN64)
	iocp_aio_exit_clean__(aio);
	return CloseHandle(aio->__handle);
#elif	__linux__
	uring_aio_exit_clean__(aio);
	io_uring_queue_exit(&aio->__r);
	close(aio->__wakeup_fds[0]);
	close(aio->__wakeup_fds[1]);
#endif
	return 1;
}

AioFD_t* aiofdInit(AioFD_t* aiofd, FD_t fd) {
	aiofd->__lprev = NULL;
	aiofd->__lnext = NULL;
	aiofd->__ol_pending_list_tail = NULL;
	aiofd->__delete_flag = 0;
	aiofd->__reg = 0;

	aiofd->fd = fd;
	aiofd->domain = AF_UNSPEC;
	aiofd->socktype = 0;
	aiofd->protocol = 0;
	aiofd->stream_rq = NULL;
	aiofd->stream_wq = NULL;
	return aiofd;
}

void aiofdDelete(Aio_t* aio, AioFD_t* aiofd) {
	if (aiofd->__delete_flag) {
		return;
	}
	aiofd->__delete_flag = 1;
	if (aiofd->__reg) {
		/* remove from alive list */
		if (aiofd->__lprev) {
			aiofd->__lprev->__lnext = aiofd->__lnext;
		}
		if (aiofd->__lnext) {
			aiofd->__lnext->__lprev = aiofd->__lprev;
		}
		if (aiofd == aio->__alive_list_head) {
			aio->__alive_list_head = aiofd->__lnext;
		}
	}
	if (aiofd->stream_rq) {
		ol_stream_destroy(aiofd->stream_rq);
	}
	if (aiofd->stream_wq) {
		ol_stream_destroy(aiofd->stream_wq);
	}
	aiofd_free_all_pending_ol(aiofd);
#if defined(_WIN32) || defined(_WIN64)
	if (!aiofd->__ol_pending_list_tail) {
		if (aiofd->domain != AF_UNSPEC) {
			closesocket(aiofd->fd);
			aiofd->fd = INVALID_SOCKET;
		}
		else {
			CloseHandle((HANDLE)aiofd->fd);
			aiofd->fd = (FD_t)INVALID_HANDLE_VALUE;
		}
		aio->__fn_free_aiofd(aiofd);
		return;
	}
	CancelIo((HANDLE)aiofd->fd);
#elif	__linux__
	if (!aiofd->__ol_pending_list_tail) {
		if (SOCK_STREAM == aiofd->socktype) {
			shutdown(aiofd->fd, SHUT_RDWR);
		}
		close(aiofd->fd);
		aiofd->fd = -1;
		aio->__fn_free_aiofd(aiofd);
		return;
	}
	if (!uring_submit_async_cancel__(&aio->__r, aiofd)) {
		struct io_uring_sync_cancel_reg param = { 0 };
		param.fd = aiofd->fd;
		param.flags = (IORING_ASYNC_CANCEL_FD | IORING_ASYNC_CANCEL_ALL);
		param.timeout.tv_sec = -1UL;
		param.timeout.tv_nsec = -1UL;
		io_uring_register_sync_cancel(&aio->__r, &param);
	}
#endif
	/* insert into delete list */
	aiofd->__lnext = aio->__delete_list_head;
	aiofd->__lprev = NULL;
	if (aio->__delete_list_head) {
		aio->__delete_list_head->__lprev = aiofd;
	}
	aio->__delete_list_head = aiofd;
}

BOOL aioCommit(Aio_t* aio, AioFD_t* aiofd, IoOverlapped_t* ol, int flag_bits) {
#if defined(_WIN32) || defined(_WIN64)
	AioOverlappedStream_t* ol_stream;
	int fd_domain = aiofd->domain;
	FD_t fd = aiofd->fd;
	if (aiofd->__delete_flag) {
		return FALSE;
	}
	if (ol->commit) {
		return TRUE;
	}
	if (!aio_regfd(aio, aiofd)) {
		return FALSE;
	}
	ol_stream = NULL;
	ol->flag_bits = flag_bits;
	if (IO_OVERLAPPED_OP_READ == ol->opcode) {
		IocpReadOverlapped_t* read_ol = (IocpReadOverlapped_t*)ol;
		ol_stream = aiofd->stream_rq;
		if (ol_stream) {
			if (ol_stream->running && ol_stream->running != ol) {
				ol_stream_push_back(ol_stream, ol);
				return 1;
			}
		}
		read_ol->wsabuf.buf = read_ol->base.iobuf.buf + read_ol->base.bytes_off;
		read_ol->wsabuf.len = read_ol->base.iobuf.len - read_ol->base.bytes_off;
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
	}
	else if (IO_OVERLAPPED_OP_WRITE == ol->opcode) {
		IocpWriteOverlapped_t* write_ol = (IocpWriteOverlapped_t*)ol;
		ol_stream = aiofd->stream_wq;
		if (ol_stream) {
			if (ol_stream->running && ol_stream->running != ol) {
				ol_stream_push_back(ol_stream, ol);
				return 1;
			}
		}
		write_ol->wsabuf.buf = write_ol->base.iobuf.buf + write_ol->base.bytes_off;
		write_ol->wsabuf.len = write_ol->base.iobuf.len - write_ol->base.bytes_off;
		if (fd_domain != AF_UNSPEC) {
			const struct sockaddr* toaddr;
			int toaddrlen = write_ol->saddrlen;
			if (toaddrlen > 0 && aiofd->socktype != SOCK_STREAM) {
				toaddr = (const struct sockaddr*)&write_ol->saddr;
			}
			else {
				toaddr = NULL;
				toaddrlen = 0;
			}
			if (WSASendTo((SOCKET)fd, &write_ol->wsabuf, 1, NULL, write_ol->dwFlags, toaddr, toaddrlen, (LPWSAOVERLAPPED)&write_ol->base.ol, NULL)) {
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
		ol_stream = aiofd->stream_wq;
		if (ol_stream) {
			if (ol_stream->running && ol_stream->running != ol) {
				ol_stream_push_back(ol_stream, ol);
				return 1;
			}
		}
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
		if (bind((SOCKET)fd, (struct sockaddr*)&st, conn_ol->saddrlen)) {
			return FALSE;
		}
		conn_ol->wsabuf.buf = conn_ol->base.iobuf.buf + conn_ol->base.bytes_off;
		conn_ol->wsabuf.len = conn_ol->base.iobuf.len - conn_ol->base.bytes_off;
		if (!lpfnConnectEx((SOCKET)fd, (const struct sockaddr*)&conn_ol->saddr, conn_ol->saddrlen, conn_ol->wsabuf.buf, conn_ol->wsabuf.len, NULL, (LPWSAOVERLAPPED)&conn_ol->base.ol)) {
			if (WSAGetLastError() != ERROR_IO_PENDING) {
				return FALSE;
			}
		}
	}
	else {
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}

	if (ol_stream) {
		ol_stream->running = ol;
	}
	ol->commit = 1;
	aiofd_link_pending_ol(aiofd, ol);
	return TRUE;
#elif	__linux__
	AioOverlappedStream_t* ol_stream;
	struct io_uring_sqe* sqe;
	if (aiofd->__delete_flag) {
		return 0;
	}
	if (ol->__wait_cqe_notify) {
		return 0;
	}
	if (ol->commit) {
		return 1;
	}
	if (!aio_regfd(aio, aiofd)) {
		return 0;
	}
	ol->flag_bits = flag_bits;
	ol_stream = NULL;
	if (IO_OVERLAPPED_OP_READ == ol->opcode) {
		UnixReadOverlapped_t* read_ol = (UnixReadOverlapped_t*)ol;
		ol_stream = aiofd->stream_rq;
		if (ol_stream) {
			if (ol_stream->running && ol_stream->running != ol) {
				ol_stream_push_back(ol_stream, ol);
				return 1;
			}
		}
		sqe = io_uring_get_sqe(&aio->__r);
		if (!sqe) {
			return 0;
		}
		read_ol->iov.iov_base = ((char*)read_ol->base.iobuf.iov_base) + read_ol->base.bytes_off;
		read_ol->iov.iov_len = read_ol->base.iobuf.iov_len - read_ol->base.bytes_off;
		if (aiofd->domain != AF_UNSPEC) {
			read_ol->msghdr.msg_name = &read_ol->saddr;
			read_ol->msghdr.msg_namelen = sizeof(read_ol->saddr);

			io_uring_prep_recvmsg(sqe, aiofd->fd, &read_ol->msghdr, 0);
		}
		else {
			io_uring_prep_readv(sqe, aiofd->fd, read_ol->msghdr.msg_iov, read_ol->msghdr.msg_iovlen, read_ol->fd_offset);
		}
	}
	else if (IO_OVERLAPPED_OP_WRITE == ol->opcode) {
		UnixWriteOverlapped_t* write_ol = (UnixWriteOverlapped_t*)ol;
		ol_stream = aiofd->stream_wq;
		if (ol_stream) {
			if (ol_stream->running && ol_stream->running != ol) {
				ol_stream_push_back(ol_stream, ol);
				return 1;
			}
		}
		sqe = io_uring_get_sqe(&aio->__r);
		if (!sqe) {
			return 0;
		}
		write_ol->iov.iov_base = ((char*)write_ol->base.iobuf.iov_base) + write_ol->base.bytes_off;
		write_ol->iov.iov_len = write_ol->base.iobuf.iov_len - write_ol->base.bytes_off;
		if (aiofd->domain != AF_UNSPEC) {
			if (flag_bits & IO_OVERLAPPED_FLAG_BIT_WRITE_ZC) {
				io_uring_prep_sendmsg_zc(sqe, aiofd->fd, &write_ol->msghdr, 0);
			}
			else {
				io_uring_prep_sendmsg(sqe, aiofd->fd, &write_ol->msghdr, 0);
			}
		}
		else {
			io_uring_prep_writev(sqe, aiofd->fd, write_ol->msghdr.msg_iov, write_ol->msghdr.msg_iovlen, write_ol->fd_offset);
		}
	}
	else if (IO_OVERLAPPED_OP_CONNECT == ol->opcode) {
		UnixConnectOverlapped_t* conn_ol = (UnixConnectOverlapped_t*)ol;
		ol_stream = aiofd->stream_wq;
		if (ol_stream) {
			if (ol_stream->running && ol_stream->running != ol) {
				ol_stream_push_back(ol_stream, ol);
				return 1;
			}
		}
		sqe = io_uring_get_sqe(&aio->__r);
		if (!sqe) {
			return 0;
		}
		io_uring_prep_connect(sqe, aiofd->fd, (const struct sockaddr*)&conn_ol->saddr, conn_ol->saddrlen);
	}
	else if (IO_OVERLAPPED_OP_ACCEPT == ol->opcode) {
		UnixAcceptOverlapped_t* accept_ol = (UnixAcceptOverlapped_t*)ol;
		sqe = io_uring_get_sqe(&aio->__r);
		if (!sqe) {
			return 0;
		}
		accept_ol->base.__fd = -1;
		accept_ol->saddrlen = sizeof(accept_ol->saddr);

		io_uring_prep_accept(sqe, aiofd->fd, (struct sockaddr*)&accept_ol->saddr, &accept_ol->saddrlen, 0);
	}
	else {
		errno = EOPNOTSUPP;
		return 0;
	}

	if (ol_stream) {
		ol_stream->running = ol;
	}
	ol->commit = 1;
	ol->__completion_key = aiofd;
	aiofd_link_pending_ol(aiofd, ol);
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
	aio_handle_free_dead(aio);
	if (GetQueuedCompletionStatusEx(aio->__handle, e, n, &cnt, msec, FALSE)) {
		return cnt;
	}
	if (GetLastError() == WAIT_TIMEOUT) {
		return 0;
	}
	return -1;
#elif	__linux__
	int ret;
	unsigned int arg_n = n;
	unsigned head, advance_n;
	IoOverlapped_t* ol;
	struct io_uring_cqe* cqe;

	aio_handle_free_dead(aio);

	n = advance_n = 0;
	io_uring_for_each_cqe(&aio->__r, head, cqe) {
		advance_n++;
		ol = (IoOverlapped_t*)io_uring_cqe_get_data(cqe);
		if (uring_filter_internal_ol__(aio, ol, cqe->flags)) {
			continue;
		}
		uring_cqe_save__(ol, cqe);
		e[n].ol = ol;
		n++;
		if (n >= arg_n) {
			break;
		}
	}
	io_uring_cq_advance(&aio->__r, advance_n);
	if (n > 0) {
		return n;
	}

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
			struct __kernel_timespec kt;
			kt.tv_sec = msec / 1000;
			kt.tv_nsec = msec % 1000;
			kt.tv_nsec *= 1000000LL;
			ret = io_uring_wait_cqe_timeout(&aio->__r, &cqe, &kt);
			if (ret != 0) {
				if (ETIME == -ret) {
					return 0;
				}
				if (EINTR != -ret) {
					errno = -ret;
					return -1;
				}
				cqe = NULL;
			}
			if (cqe) {
				ol = (IoOverlapped_t*)io_uring_cqe_get_data(cqe);
				if (!uring_filter_internal_ol__(aio, ol, cqe->flags)) {
					break;
				}
				io_uring_cqe_seen(&aio->__r, cqe);
			}
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
			start_tm = tm;
		}
	}
	else {
		while (1) {
			ret = io_uring_wait_cqe(&aio->__r, &cqe);
			if (ret != 0) {
				if (EINTR == -ret) {
					continue;
				}
				errno = -ret;
				return -1;
			}
			if (!cqe) {
				continue;
			}
			ol = (IoOverlapped_t*)io_uring_cqe_get_data(cqe);
			if (!uring_filter_internal_ol__(aio, ol, cqe->flags)) {
				break;
			}
			io_uring_cqe_seen(&aio->__r, cqe);
		}
	}
	uring_cqe_save__(ol, cqe);
	io_uring_cqe_seen(&aio->__r, cqe);
	e[0].ol = ol;
	return 1;
#else
	errno = ENOSYS;
	return -1;
#endif
}

void aioWakeup(Aio_t* aio) {
	if (_xchg16(&aio->__wakeup, 1)) {
		return;
	}
#if defined(_WIN32) || defined(_WIN64)
	PostQueuedCompletionStatus(aio->__handle, 0, 0, NULL);
#elif	__linux__
	write(aio->__wakeup_fds[1], &s_wakeup_buf, 1);
#endif
}

IoOverlapped_t* aioEventCheck(Aio_t* aio, const AioEv_t* e, AioFD_t** ol_aiofd) {
#if defined(_WIN32) || defined(_WIN64)
	AioFD_t* aiofd;
	IoOverlapped_t* ol = (IoOverlapped_t*)e->lpOverlapped;
	if (!ol) {
		_xchg16(&aio->__wakeup, 0);
		*ol_aiofd = NULL;
		return NULL;
	}
	aiofd = (AioFD_t*)e->lpCompletionKey;
	aio_ol_acked(aio, aiofd, ol, 1);

	ol->commit = 0;
	if (ol->free_flag) {
		*ol_aiofd = NULL;
		IoOverlapped_free(ol);
		return NULL;
	}

	ol->transfer_bytes = e->dwNumberOfBytesTransferred;
	if (NT_ERROR(e->Internal)) {
		ol->error = e->Internal;
	}
	else if (IO_OVERLAPPED_OP_CONNECT == ol->opcode) {
		ol->error = win32_OverlappedConnectUpdate(aiofd->fd);
		if (0 == ol->error) {
			ol->bytes_off += ol->transfer_bytes;
		}
	}
	else {
		if (IO_OVERLAPPED_OP_READ == ol->opcode || IO_OVERLAPPED_OP_WRITE == ol->opcode) {
			ol->bytes_off += ol->transfer_bytes;
		}
		ol->error = 0;
	}

	*ol_aiofd = aiofd;
	return ol;
#elif	__linux__
	AioFD_t* aiofd;
	IoOverlapped_t* ol = (IoOverlapped_t*)e->ol;
	if (&s_wakeup_ol == ol) {
		uring_submit_wakeup__(&aio->__r, aio->__wakeup_fds[0]);
		_xchg16(&aio->__wakeup, 0);
		*ol_aiofd = NULL;
		return NULL;
	}

	aiofd = (AioFD_t*)ol->__completion_key;
	if (!ol->__wait_cqe_notify) {
		aio_ol_acked(aio, aiofd, ol, 1);
	}
	ol->commit = 0;
	if (ol->free_flag) {
		*ol_aiofd = NULL;
		if (ol->__wait_cqe_notify) {
			return NULL;
		}
		IoOverlapped_free(ol);
		return NULL;
	}

	if (ol->error) {
		ol->transfer_bytes = 0;
	}
	else if (IO_OVERLAPPED_OP_READ == ol->opcode || IO_OVERLAPPED_OP_WRITE == ol->opcode) {
		ol->bytes_off += ol->transfer_bytes;
	}
	else if (IO_OVERLAPPED_OP_CONNECT == ol->opcode) {
		int err = 0;
		socklen_t len = sizeof(err);
		if (getsockopt(aiofd->fd, SOL_SOCKET, SO_ERROR, (char*)&err, &len)) {
			ol->error = errno;
		}
		else {
			ol->error = err;
		}
	}

	*ol_aiofd = aiofd;
	return ol;
#else
	*ol_aiofd = NULL;
	return NULL;
#endif
}

BOOL aioAckOverlappedStream(Aio_t* aio, AioFD_t* aiofd, IoOverlapped_t* ack_ol) {
	size_t i;
	AioOverlappedStream_t* s_arr[] = {
		aiofd->stream_wq,
		aiofd->stream_rq
	};
	for (i = 0; i < sizeof(s_arr) / sizeof(s_arr[0]); ++i) {
		IoOverlapped_t* ol;
		AioOverlappedStream_t* s = s_arr[i];
		if (!s || s->running != ack_ol) {
			continue;
		}
		s->running = NULL;
		if (ack_ol->bytes_off < iobufLen(&ack_ol->iobuf)) {
			break;
		}
		ol = ol_stream_pop_front(s);
		if (!ol) {
			break;
		}
		if (aioCommit(aio, aiofd, ol, ol->flag_bits)) {
			break;
		}
		ol_stream_push_front(s, ol);
		return FALSE;
	}
	return TRUE;
}

#ifdef	__cplusplus
}
#endif

#endif
