//
// Created by hujianzhe
//

#include "../../inc/sysapi/io_overlapped.h"
#ifdef _WIN32
#include <mswsock.h>
#endif
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#ifdef	__cplusplus
extern "C" {
#endif

void iobufSkip(const Iobuf_t* iov, size_t iovcnt, size_t* iov_i, size_t* iov_off, size_t n) {
	while (*iov_i < iovcnt && n) {
		size_t iovleftsize = iobufLen(iov + *iov_i) - *iov_off;
		if (iovleftsize > n) {
			*iov_off += n;
			break;
		}
		*iov_off = 0;
		(*iov_i)++;
		n -= iovleftsize;
	}
}

size_t iobufShardCopy(const Iobuf_t* iov, size_t iovcnt, size_t* iov_i, size_t* iov_off, void* buf, size_t n) {
	size_t off = 0;
	unsigned char* ptr_buf = (unsigned char*)buf;
	while (*iov_i < iovcnt) {
		size_t leftsize = n - off;
		char* iovptr = ((char*)iobufPtr(iov + *iov_i)) + *iov_off;
		size_t iovleftsize = iobufLen(iov + *iov_i) - *iov_off;
		if (iovleftsize > leftsize) {
			memmove(ptr_buf + off, iovptr, leftsize);
			*iov_off += leftsize;
			off += leftsize;
			break;
		}
		memmove(ptr_buf + off, iovptr, iovleftsize);
		*iov_off = 0;
		(*iov_i)++;
		off += iovleftsize;
	}
	return off;
}

IoOverlapped_t* IoOverlapped_alloc(int opcode, unsigned int appendsize) {
#if defined(_WIN32) || defined(_WIN64)
	switch (opcode) {
		case IO_OVERLAPPED_OP_READ:
		{
			IocpReadOverlapped_t* ol = (IocpReadOverlapped_t*)malloc(sizeof(IocpReadOverlapped_t) + appendsize);
			if (!ol) {
				return NULL;
			}
			memset(ol, 0, sizeof(IocpReadOverlapped_t));
			ol->base.opcode = IO_OVERLAPPED_OP_READ;
			ol->saddr.ss_family = AF_UNSPEC;
			ol->saddrlen = sizeof(ol->saddr);
			ol->appendsize = appendsize;
			if (appendsize) {
				ol->base.iobuf.buf = (char*)(ol->append_data);
				ol->base.iobuf.len = appendsize;
				ol->append_data[appendsize] = 0;
			}
			return &ol->base;
		}
		case IO_OVERLAPPED_OP_CONNECT:
		case IO_OVERLAPPED_OP_WRITE:
		{
			IocpWriteOverlapped_t* ol = (IocpWriteOverlapped_t*)malloc(sizeof(IocpWriteOverlapped_t) + appendsize);
			if (!ol) {
				return NULL;
			}
			memset(ol, 0, sizeof(IocpWriteOverlapped_t));
			ol->base.opcode = opcode;
			ol->saddr.ss_family = AF_UNSPEC;
			ol->appendsize = appendsize;
			if (appendsize) {
				ol->base.iobuf.buf = (char*)(ol->append_data);
				ol->base.iobuf.len = appendsize;
				ol->append_data[appendsize] = 0;
			}
			return &ol->base;
		}
		case IO_OVERLAPPED_OP_ACCEPT:
		{
			IocpAcceptExOverlapped_t* ol = (IocpAcceptExOverlapped_t*)calloc(1, sizeof(IocpAcceptExOverlapped_t));
			if (!ol) {
				return NULL;
			}
			ol->base.opcode = IO_OVERLAPPED_OP_ACCEPT;
			ol->acceptsocket = INVALID_SOCKET;
			ol->listensocket = INVALID_SOCKET;
			return &ol->base;
		}
		default:
		{
			SetLastError(ERROR_INVALID_PARAMETER);
			return NULL;
		}
	}
#else
	switch (opcode) {
		case IO_OVERLAPPED_OP_READ:
		{
			UnixReadOverlapped_t* ol = (UnixReadOverlapped_t*)malloc(sizeof(UnixReadOverlapped_t) + appendsize);
			if (!ol) {
				return NULL;
			}
			memset(ol, 0, sizeof(UnixReadOverlapped_t));
			ol->base.opcode = IO_OVERLAPPED_OP_READ;
			ol->saddr.ss_family = AF_UNSPEC;
			ol->appendsize = appendsize;
			if (appendsize) {
				ol->base.iobuf.iov_base = (char*)(ol->append_data);
				ol->base.iobuf.iov_len = appendsize;
				ol->append_data[appendsize] = 0;
			}
			ol->msghdr.msg_iov = &ol->iov;
			ol->msghdr.msg_iovlen = 1;
			return &ol->base;
		}
		case IO_OVERLAPPED_OP_WRITE:
		{
			UnixWriteOverlapped_t* ol = (UnixWriteOverlapped_t*)malloc(sizeof(UnixWriteOverlapped_t) + appendsize);
			if (!ol) {
				return NULL;
			}
			memset(ol, 0, sizeof(UnixWriteOverlapped_t));
			ol->base.opcode = IO_OVERLAPPED_OP_WRITE;
			ol->saddr.ss_family = AF_UNSPEC;
			ol->appendsize = appendsize;
			if (appendsize) {
				ol->base.iobuf.iov_base = (char*)(ol->append_data);
				ol->base.iobuf.iov_len = appendsize;
				ol->append_data[appendsize] = 0;
			}
			ol->msghdr.msg_iov = &ol->iov;
			ol->msghdr.msg_iovlen = 1;
			return &ol->base;
		}
		case IO_OVERLAPPED_OP_ACCEPT:
		{
			UnixAcceptOverlapped_t* ol = (UnixAcceptOverlapped_t*)calloc(1, sizeof(UnixAcceptOverlapped_t));
			if (!ol) {
				return NULL;
			}
			ol->base.__fd = -1;
			ol->base.opcode = IO_OVERLAPPED_OP_ACCEPT;
			ol->saddr.ss_family = AF_UNSPEC;
			return &ol->base;
		}
		case IO_OVERLAPPED_OP_CONNECT:
		{
			UnixConnectOverlapped_t* ol = (UnixConnectOverlapped_t*)calloc(1, sizeof(UnixConnectOverlapped_t));
			if (!ol) {
				return NULL;
			}
			ol->base.opcode = IO_OVERLAPPED_OP_CONNECT;
			ol->saddr.ss_family = AF_UNSPEC;
			return &ol->base;
		}
		default:
		{
			errno = EINVAL;
			return NULL;
		}
	}
#endif
}

Iobuf_t* IoOverlapped_get_append_iobuf(const IoOverlapped_t* ol, Iobuf_t* iobuf) {
#if defined(_WIN32) || defined(_WIN64)
	if (IO_OVERLAPPED_OP_WRITE == ol->opcode) {
		IocpWriteOverlapped_t* write_ol = (IocpWriteOverlapped_t*)ol;
		iobuf->len = write_ol->appendsize;
		iobuf->buf = iobuf->len ? (char*)write_ol->append_data : NULL;
	}
	else if (IO_OVERLAPPED_OP_READ == ol->opcode) {
		IocpReadOverlapped_t* read_ol = (IocpReadOverlapped_t*)ol;
		iobuf->len = read_ol->appendsize;
		iobuf->buf = iobuf->len ? (char*)read_ol->append_data : NULL;
	}
	else if (IO_OVERLAPPED_OP_CONNECT == ol->opcode) {
		IocpConnectExOverlapped_t* conn_ol = (IocpConnectExOverlapped_t*)ol;
		iobuf->len = conn_ol->appendsize;
		iobuf->buf = iobuf->len ? (char*)conn_ol->append_data : NULL;
	}
	else {
		iobuf->len = 0;
		iobuf->buf = NULL;
	}
#else
	if (IO_OVERLAPPED_OP_WRITE == ol->opcode) {
		UnixWriteOverlapped_t* write_ol = (UnixWriteOverlapped_t*)ol;
		iobuf->iov_len = write_ol->appendsize;
		iobuf->iov_base = iobuf->iov_len ? write_ol->append_data : NULL;
	}
	else if (IO_OVERLAPPED_OP_READ == ol->opcode) {
		UnixReadOverlapped_t* read_ol = (UnixReadOverlapped_t*)ol;
		iobuf->iov_len = read_ol->appendsize;
		iobuf->iov_base = iobuf->iov_len ? read_ol->append_data : NULL;
	}
	else {
		iobuf->iov_len = 0;
		iobuf->iov_base = NULL;
	}
#endif
	return iobuf;
}

long long IoOverlapped_get_file_offset(const IoOverlapped_t* ol) {
#if defined(_WIN32) || defined(_WIN64)
	long long offset = ol->ol.OffsetHigh;
	offset <<= 32;
	offset |= ol->ol.Offset;
	return offset;
#else
	if (IO_OVERLAPPED_OP_READ == ol->opcode) {
		UnixReadOverlapped_t* read_ol = (UnixReadOverlapped_t*)ol;
		return read_ol->fd_offset;
	}
	if (IO_OVERLAPPED_OP_WRITE == ol->opcode) {
		UnixWriteOverlapped_t* write_ol = (UnixWriteOverlapped_t*)ol;
		return write_ol->fd_offset;
	}
	return 0;
#endif
}

IoOverlapped_t* IoOverlapped_set_file_offest(IoOverlapped_t* ol, long long offset) {
#if defined(_WIN32) || defined(_WIN64)
	if (IO_OVERLAPPED_OP_READ == ol->opcode || IO_OVERLAPPED_OP_WRITE == ol->opcode) {
		ol->ol.Offset = offset;
		ol->ol.OffsetHigh = offset >> 32;
	}
#else
	if (IO_OVERLAPPED_OP_READ == ol->opcode) {
		UnixReadOverlapped_t* read_ol = (UnixReadOverlapped_t*)ol;
		read_ol->fd_offset = offset;
	}
	else if (IO_OVERLAPPED_OP_WRITE == ol->opcode) {
		UnixWriteOverlapped_t* write_ol = (UnixWriteOverlapped_t*)ol;
		write_ol->fd_offset = offset;
	}
#endif
	return ol;
}

FD_t IoOverlapped_pop_acceptfd(IoOverlapped_t* ol, struct sockaddr* p_peer_saddr, socklen_t* plen) {
#if defined(_WIN32) || defined(_WIN64)
	if (IO_OVERLAPPED_OP_ACCEPT == ol->opcode) {
		IocpAcceptExOverlapped_t* iocp_acceptex = (IocpAcceptExOverlapped_t*)ol;
		SOCKET acceptfd = iocp_acceptex->acceptsocket;
		iocp_acceptex->acceptsocket = INVALID_SOCKET;
		if (setsockopt(acceptfd, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, (char*)&iocp_acceptex->listensocket, sizeof(iocp_acceptex->listensocket))) {
			closesocket(acceptfd);
			return INVALID_FD_HANDLE;
		}
		if (p_peer_saddr && plen) {
			if (getpeername(acceptfd, p_peer_saddr, plen)) {
				closesocket(acceptfd);
				return INVALID_FD_HANDLE;
			}
		}
		return acceptfd;
	}
#else
	if (IO_OVERLAPPED_OP_ACCEPT == ol->opcode) {
		int acceptfd = ol->__fd;
		ol->__fd = -1;
		if (p_peer_saddr && plen) {
			getpeername(acceptfd, p_peer_saddr, plen);
		}
		return acceptfd;
	}
#endif
	return INVALID_FD_HANDLE;
}

struct sockaddr* IoOverlapped_get_peer_sockaddr(const IoOverlapped_t* ol, struct sockaddr* saddr, socklen_t* plen) {
#if defined(_WIN32) || defined(_WIN64)
	switch (ol->opcode) {
		case IO_OVERLAPPED_OP_READ:
		{
			IocpReadOverlapped_t* read_ol = (IocpReadOverlapped_t*)ol;
			memmove(saddr, &read_ol->saddr, read_ol->saddrlen);
			*plen = read_ol->saddrlen;
			return saddr;
		}
		case IO_OVERLAPPED_OP_WRITE:
		{
			IocpWriteOverlapped_t* write_ol = (IocpWriteOverlapped_t*)ol;
			memmove(saddr, &write_ol->saddr, write_ol->saddrlen);
			*plen = write_ol->saddrlen;
			return saddr;
		}
		case IO_OVERLAPPED_OP_CONNECT:
		{
			IocpConnectExOverlapped_t* conn_ol = (IocpConnectExOverlapped_t*)ol;
			memmove(saddr, &conn_ol->saddr, conn_ol->saddrlen);
			*plen = conn_ol->saddrlen;
			return saddr;
		}
	}
#else
	switch (ol->opcode) {
		case IO_OVERLAPPED_OP_READ:
		{
			UnixReadOverlapped_t* read_ol = (UnixReadOverlapped_t*)ol;
			memmove(saddr, read_ol->msghdr.msg_name, read_ol->msghdr.msg_namelen);
			*plen = read_ol->msghdr.msg_namelen;
			return saddr;
		}
		case IO_OVERLAPPED_OP_WRITE:
		{
			UnixWriteOverlapped_t* write_ol = (UnixWriteOverlapped_t*)ol;
			memmove(saddr, write_ol->msghdr.msg_name, write_ol->msghdr.msg_namelen);
			*plen = write_ol->msghdr.msg_namelen;
			return saddr;
		}
		case IO_OVERLAPPED_OP_CONNECT:
		{
			UnixConnectOverlapped_t* conn_ol = (UnixConnectOverlapped_t*)ol;
			memmove(saddr, &conn_ol->saddr, conn_ol->saddrlen);
			*plen = conn_ol->saddrlen;
			return saddr;
		}
	}
#endif
	saddr->sa_family = AF_UNSPEC;
	*plen = 0;
	return NULL;
}

void IoOverlapped_set_peer_sockaddr(IoOverlapped_t* ol, const struct sockaddr* saddr, socklen_t saddrlen) {
#if defined(_WIN32) || defined(_WIN64)
	switch (ol->opcode) {
		case IO_OVERLAPPED_OP_WRITE:
		{
			IocpWriteOverlapped_t* w_ol = (IocpWriteOverlapped_t*)ol;
			if (saddr && saddrlen > 0) {
				memmove(&w_ol->saddr, saddr, saddrlen);
				w_ol->saddrlen = saddrlen;
			}
			else {
				w_ol->saddr.ss_family = AF_UNSPEC;
				w_ol->saddrlen = 0;
			}
			break;
		}
		case IO_OVERLAPPED_OP_CONNECT:
		{
			IocpConnectExOverlapped_t* conn_ol = (IocpConnectExOverlapped_t*)ol;
			memmove(&conn_ol->saddr, saddr, saddrlen);
			conn_ol->saddrlen = saddrlen;
			break;
		}
	}
#else
	switch (ol->opcode) {
		case IO_OVERLAPPED_OP_WRITE:
		{
			UnixWriteOverlapped_t* w_ol = (UnixWriteOverlapped_t*)ol;
			if (saddr && saddrlen > 0) {
				memmove(&w_ol->saddr, saddr, saddrlen);
				w_ol->msghdr.msg_name = &w_ol->saddr;
				w_ol->msghdr.msg_namelen = saddrlen;
			}
			else {
				w_ol->msghdr.msg_name = NULL;
				w_ol->msghdr.msg_namelen = 0;
			}
			break;
		}
		case IO_OVERLAPPED_OP_CONNECT:
		{
			UnixConnectOverlapped_t* conn_ol = (UnixConnectOverlapped_t*)ol;
			memmove(&conn_ol->saddr, saddr, saddrlen);
			conn_ol->saddrlen = saddrlen;
			break;
		}
	}
#endif
}

void IoOverlapped_free(IoOverlapped_t* ol) {
	if (!ol) {
		return;
	}
#if defined(_WIN32) || defined(_WIN64)
	if (IO_OVERLAPPED_OP_ACCEPT == ol->opcode) {
		IocpAcceptExOverlapped_t* iocp_acceptex = (IocpAcceptExOverlapped_t*)ol;
		if (INVALID_SOCKET != iocp_acceptex->acceptsocket) {
			closesocket(iocp_acceptex->acceptsocket);
			iocp_acceptex->acceptsocket = INVALID_SOCKET;
		}
	}
#else
	if (IO_OVERLAPPED_OP_ACCEPT == ol->opcode) {
		if (ol->__fd >= 0) {
			close(ol->__fd);
			ol->__fd = -1;
		}
	}
	#ifdef	__linux__
	if (ol->__wait_cqe_notify) {
		ol->free_flag = 1;
		return;
	}
	#endif
#endif
	if (ol->commit) {
		ol->free_flag = 1;
		return;
	}
	if (ol->on_destroy) {
		ol->on_destroy(ol);
	}
	free(ol);
}

/*
int IoOverlapped_check_free_able(const IoOverlapped_t* ol) {
#ifdef	__linux__
	if (ol->__wait_cqe_notify) {
		return 0;
	}
#endif
	return !ol->commit;
}
*/

int IoOverlapped_check_reuse_able(const IoOverlapped_t* ol) {
#ifdef	__linux__
	if (ol->__wait_cqe_notify) {
		return 0;
	}
#endif
	if (ol->commit || ol->free_flag) {
		return 0;
	}
	return 1;
}

#ifdef	__cplusplus
}
#endif
