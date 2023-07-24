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
			if (appendsize) {
				ol->base.iobuf.iov_base = (char*)(ol->append_data);
				ol->base.iobuf.iov_len = appendsize;
				ol->append_data[appendsize] = 0;
			}
			ol->msghdr.msg_iov = &ol->base.iobuf;
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
			if (appendsize) {
				ol->base.iobuf.iov_base = (char*)(ol->append_data);
				ol->base.iobuf.iov_len = appendsize;
				ol->append_data[appendsize] = 0;
			}
			ol->msghdr.msg_iov = &ol->base.iobuf;
			ol->msghdr.msg_iovlen = 1;
			return &ol->base;
		}
		case IO_OVERLAPPED_OP_ACCEPT:
		{
			UnixAcceptOverlapped_t* ol = (UnixAcceptOverlapped_t*)calloc(1, sizeof(UnixAcceptOverlapped_t));
			if (!ol) {
				return NULL;
			}
			ol->base.fd = -1;
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

long long IoOverlapped_get_file_offset(IoOverlapped_t* ol) {
#if defined(_WIN32) || defined(_WIN64)
	long long offset = ol->ol.OffsetHigh;
	offset <<= 32;
	offset |= ol->ol.Offset;
	return offset;
#else
	if (IO_OVERLAPPED_OP_READ == ol->opcode) {
		UnixReadOverlapped_t* read_ol = (UnixReadOverlapped_t*)ol;
		return read_ol->offset;
	}
	if (IO_OVERLAPPED_OP_WRITE == ol->opcode) {
		UnixWriteOverlapped_t* write_ol = (UnixWriteOverlapped_t*)ol;
		return write_ol->offset;
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
		read_ol->offset = offset;
	}
	else if (IO_OVERLAPPED_OP_WRITE == ol->opcode) {
		UnixWriteOverlapped_t* write_ol = (UnixWriteOverlapped_t*)ol;
		write_ol->offset = offset;
	}
#endif
	return ol;
}

int IoOverlapped_connect_update(FD_t sockfd) {
#if defined(_WIN32) || defined(_WIN64)
	int sec;
	int len = sizeof(sec);
	if (getsockopt(sockfd, SOL_SOCKET, SO_CONNECT_TIME, (char*)&sec, &len)) {
		return WSAGetLastError();
	}
	if (~0 == sec) {
		return ERROR_TIMEOUT;
	}
	if (setsockopt(sockfd, SOL_SOCKET, SO_UPDATE_CONNECT_CONTEXT, NULL, 0)) {
		return WSAGetLastError();
	}
	return 0;
#else
	int err = 0;
	socklen_t len = sizeof(int);
	if (getsockopt(sockfd, SOL_SOCKET, SO_ERROR, (char*)&err, &len)) {
		return errno;
	}
	return err;
#endif
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
#elif	__linux__
	if (IO_OVERLAPPED_OP_ACCEPT == ol->opcode) {
		int acceptfd = ol->fd;
		ol->fd = -1;
		if (p_peer_saddr && plen) {
			getpeername(acceptfd, p_peer_saddr, plen);
		}
		return acceptfd;
	}
#endif
	return INVALID_FD_HANDLE;
}

void IoOverlapped_peer_sockaddr(IoOverlapped_t* ol, struct sockaddr** pp_saddr, socklen_t* plen) {
#if defined(_WIN32) || defined(_WIN64)
	switch (ol->opcode) {
		case IO_OVERLAPPED_OP_READ:
			*pp_saddr = (struct sockaddr*)&((IocpReadOverlapped_t*)ol)->saddr;
			*plen = ((IocpReadOverlapped_t*)ol)->saddrlen;
			break;
		case IO_OVERLAPPED_OP_WRITE:
			*pp_saddr = (struct sockaddr*)&((IocpWriteOverlapped_t*)ol)->saddr;
			*plen = ((IocpWriteOverlapped_t*)ol)->saddrlen;
			break;
		case IO_OVERLAPPED_OP_CONNECT:
			*pp_saddr = (struct sockaddr*)&((IocpConnectExOverlapped_t*)ol)->saddr;
			*plen = ((IocpConnectExOverlapped_t*)ol)->saddrlen;
			break;
		default:
			*pp_saddr = NULL;
			*plen = 0;
	}
#else
	switch (ol->opcode) {
		case IO_OVERLAPPED_OP_READ:
			*pp_saddr = (struct sockaddr*)((UnixReadOverlapped_t*)ol)->msghdr.msg_name;
			*plen = ((UnixReadOverlapped_t*)ol)->msghdr.msg_namelen;
			break;
		case IO_OVERLAPPED_OP_WRITE:
			*pp_saddr = (struct sockaddr*)((UnixWriteOverlapped_t*)ol)->msghdr.msg_name;
			*plen = ((UnixWriteOverlapped_t*)ol)->msghdr.msg_namelen;
			break;
		case IO_OVERLAPPED_OP_CONNECT:
			*pp_saddr = (struct sockaddr*)&((UnixConnectOverlapped_t*)ol)->saddr;
			*plen = ((UnixConnectOverlapped_t*)ol)->saddrlen;
			break;
		default:
			*pp_saddr = NULL;
			*plen = 0;
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
#elif	__linux__
	if (IO_OVERLAPPED_OP_ACCEPT == ol->opcode) {
		if (ol->fd >= 0) {
			close(ol->fd);
			ol->fd = -1;
		}
	}
	if (ol->__wait_cqe_notify) {
		ol->free_flag = 1;
		return;
	}
#endif
	if (ol->commit) {
		ol->free_flag = 1;
		return;
	}
	free(ol);
}

int IoOverlapped_check_free_able(IoOverlapped_t* ol) {
#ifdef	__linux__
	if (ol->__wait_cqe_notify) {
		return 0;
	}
#endif
	return !ol->commit;
}

int IoOverlapped_check_reuse_able(IoOverlapped_t* ol) {
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
