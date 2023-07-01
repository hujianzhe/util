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
			return &ol->base;
		}
		case IO_OVERLAPEED_OP_CONNECT:
		case IO_OVERLAPPED_OP_WRITE:
		{
			IocpWriteOverlapped_t* ol = (IocpWriteOverlapped_t*)malloc(sizeof(IocpWriteOverlapped_t) + appendsize);
			if (!ol) {
				return NULL;
			}
			memset(ol, 0, sizeof(IocpWriteOverlapped_t));
			ol->base.opcode = opcode;
			ol->saddr.ss_family = AF_UNSPEC;
			ol->saddrlen = 0;
			if (refbuf && refsize) {
				ol->wsabuf.buf = (char*)refbuf;
				ol->wsabuf.len = refsize;
			}
			else if (appendsize) {
				ol->wsabuf.buf = (char*)(ol->append_data);
				ol->wsabuf.len = appendsize;
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
				ol->iov.iov_base = (char*)refbuf;
				ol->iov.iov_len = refsize;
			}
			else if (appendsize) {
				ol->iov.iov_base = (char*)(ol->append_data);
				ol->iov.iov_len = appendsize;
				ol->append_data[appendsize] = 0;
			}
			ol->msghdr.msg_name = &ol->saddr;
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
			if (refbuf && refsize) {
				ol->iov.iov_base = (char*)refbuf;
				ol->iov.iov_len = refsize;
			}
			else if (appendsize) {
				ol->iov.iov_base = (char*)(ol->append_data);
				ol->iov.iov_len = appendsize;
				ol->append_data[appendsize] = 0;
			}
			ol->msghdr.msg_name = &ol->saddr;
			ol->msghdr.msg_iov = &ol->iov;
			ol->msghdr.msg_iovlen = 1;
			return &ol->base;
		}
		case IO_OVERLAPPED_OP_ACCEPT:
		case IO_OVERLAPEED_OP_CONNECT:
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

void IoOverlapped_free(IoOverlapped_t* ol) {
	if (!ol) {
		return;
	}
#if defined(_WIN32) || defined(_WIN64)
	if (IO_OVERLAPPED_OP_ACCEPT == ol->opcode) {
		IocpAcceptExOverlapped* iocp_acceptex = (IocpAcceptExOverlapped*)ol;
		if (INVALID_SOCKET != iocp_acceptex->acceptsocket) {
			closesocket(iocp_acceptex->acceptsocket);
			iocp_acceptex->acceptsocket = INVALID_SOCKET;
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
