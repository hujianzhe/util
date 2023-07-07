//
// Created by hujianzhe
//

#include "../../inc/sysapi/io_overlapped.h"
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#ifdef	__cplusplus
extern "C" {
#endif

IoOverlapped_t* IoOverlapped_alloc(int opcode, const void* refbuf, unsigned int refsize, unsigned int appendsize) {
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
			if (refbuf && refsize) {
				ol->base.iobuf.buf = (char*)refbuf;
				ol->base.iobuf.len = refsize;
			}
			else if (appendsize) {
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
			if (refbuf && refsize) {
				ol->base.iobuf.buf = (char*)refbuf;
				ol->base.iobuf.len = refsize;
			}
			else if (appendsize) {
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
			if (refbuf && refsize) {
				ol->base.iobuf.iov_base = (char*)refbuf;
				ol->base.iobuf.iov_len = refsize;
			}
			else if (appendsize) {
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
			if (refbuf && refsize) {
				ol->base.iobuf.iov_base = (char*)refbuf;
				ol->base.iobuf.iov_len = refsize;
			}
			else if (appendsize) {
				ol->base.iobuf.iov_base = (char*)(ol->append_data);
				ol->base.iobuf.iov_len = appendsize;
				ol->append_data[appendsize] = 0;
			}
			ol->msghdr.msg_iov = &ol->base.iobuf;
			ol->msghdr.msg_iovlen = 1;
			return &ol->base;
		}
		case IO_OVERLAPPED_OP_ACCEPT:
		case IO_OVERLAPPED_OP_CONNECT:
		{
			UnixConnectOverlapped_t* ol = (UnixConnectOverlapped_t*)calloc(1, sizeof(UnixConnectOverlapped_t));
			if (!ol) {
				return NULL;
			}
			ol->base.opcode = opcode;
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

FD_t IoOverlapped_pop_acceptfd(IoOverlapped_t* ol) {
#if defined(_WIN32) || defined(_WIN64)
	if (IO_OVERLAPPED_OP_ACCEPT == ol->opcode) {
		IocpAcceptExOverlapped_t* iocp_acceptex = (IocpAcceptExOverlapped_t*)ol;
		SOCKET acceptfd = iocp_acceptex->acceptsocket;
		iocp_acceptex->acceptsocket = INVALID_SOCKET;
		return acceptfd;
	}
#elif	__linux__
	if (IO_OVERLAPPED_OP_ACCEPT == ol->opcode) {
		int acceptfd = ol->acceptfd;
		ol->acceptfd = -1;
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
		case IO_OVERLAPPED_OP_ACCEPT:
		default:
			pp_saddr = NULL;
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
			*pp_saddr = (struct sockaddr*)((UnixConnectOverlapped_t*)ol)->saddr;
			*plen = ((UnixConnectOverlapped_t*)ol)->saddrlen;
			break;
		case IO_OVERLAPPED_OP_ACCEPT:
			*pp_saddr = (struct sockaddr*)((UnixAcceptOverlapped_t*)ol)->saddr;
			*plen = ((UnixAcceptOverlapped_t*)ol)->saddrlen;
			break;
		default:
			pp_saddr = NULL;
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
		if (ol->acceptfd >= 0) {
			close(ol->acceptfd);
			ol->acceptfd = -1;
		}
	}
#endif
	if (ol->commit) {
		ol->free_flag = 1;
		return;
	}
	free(ol);
}

#ifdef	__cplusplus
}
#endif
