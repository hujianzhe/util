//
// Created by hujianzhe
//

#ifndef	UTIL_C_SYSLIB_IO_OVERLAPPED_H
#define	UTIL_C_SYSLIB_IO_OVERLAPPED_H

#include "../platform_define.h"

#if defined(_WIN32) || defined(_WIN64)
	#include <ws2tcpip.h>
	typedef	WSABUF					Iobuf_t;
	#define	iobufStaticInit(p, n)	{ (ULONG)(n), (char*)(p) }
	#define	iobufPtr(iobuf)			((iobuf)->buf)
	#define	iobufLen(iobuf)			((iobuf)->len)
#else
	#include <sys/socket.h>
	typedef	struct iovec			Iobuf_t;
	#define	iobufStaticInit(p, n)	{ (void*)(p), n }
	#define	iobufPtr(iobuf)			((iobuf)->iov_base)
	#define iobufLen(iobuf)			((iobuf)->iov_len)
#endif

enum {
	IO_OVERLAPPED_OP_INTERNAL_FD_CLOSE = -1,

	IO_OVERLAPPED_OP_READ = 1,
	IO_OVERLAPPED_OP_WRITE = 2,
	IO_OVERLAPPED_OP_ACCEPT = 3,
	IO_OVERLAPPED_OP_CONNECT = 4,
};

typedef struct IoOverlapped_t {
#if defined(_WIN32) || defined(_WIN64)
	OVERLAPPED ol; /* private */
	DWORD transfer_bytes;
	WSABUF iobuf;
#else
	#ifdef	__linux__
	int __wait_cqe_notify; /* private */
	#endif
	union {
		int __fd; /* private */
		int retval;
		unsigned int transfer_bytes;
	};
	void* __completion_key; /* private */
	struct iovec iobuf;
#endif
	int error;
	unsigned char commit;
	unsigned char free_flag;
	short opcode;
	size_t bytes_off;
	struct IoOverlapped_t* __prev; /* private */
	struct IoOverlapped_t* __next; /* private */
} IoOverlapped_t;

/* note: internal define, not direct use */

#if defined(_WIN32) || defined(_WIN64)
typedef struct {
	IoOverlapped_t base;
	struct sockaddr_storage saddr;
	int saddrlen;
	DWORD dwFlags;
	WSABUF wsabuf;
	unsigned char append_data[1]; /* convienent for text data */
} IocpReadOverlapped_t, IocpWriteOverlapped_t, IocpConnectExOverlapped_t;

typedef struct IocpAcceptExOverlapped_t {
	IoOverlapped_t base;
	SOCKET acceptsocket;
	SOCKET listensocket;
	union {
		struct {
			struct sockaddr_storage peer_saddr;
			int peer_saddrlen;
		};
		unsigned char saddr_bytes[sizeof(struct sockaddr_storage) + 16 + sizeof(struct sockaddr_storage) + 16];
	};
} IocpAcceptExOverlapped_t;
#else
typedef struct {
	IoOverlapped_t base;
	struct msghdr msghdr;
	struct sockaddr_storage saddr;
	off_t offset;
	struct iovec iov;
	unsigned char append_data[1]; /* convienent for text data */
} UnixReadOverlapped_t, UnixWriteOverlapped_t;

typedef struct {
	IoOverlapped_t base;
	struct sockaddr_storage saddr;
	socklen_t saddrlen;
} UnixConnectOverlapped_t, UnixAcceptOverlapped_t;
#endif

#ifdef	__cplusplus
extern "C" {
#endif

__declspec_dll Iobuf_t* iobufPop(Iobuf_t* iov, size_t n);
__declspec_dll size_t iobufShardCopy(const Iobuf_t* iov, size_t iovcnt, size_t* iov_i, size_t* iov_off, void* buf, size_t n);

__declspec_dll IoOverlapped_t* IoOverlapped_alloc(int opcode, unsigned int appendsize);
__declspec_dll long long IoOverlapped_get_file_offset(IoOverlapped_t* ol);
__declspec_dll IoOverlapped_t* IoOverlapped_set_file_offest(IoOverlapped_t* ol, long long offset);
__declspec_dll int IoOverlapped_connect_update(FD_t sockfd);
__declspec_dll FD_t IoOverlapped_pop_acceptfd(IoOverlapped_t* ol, struct sockaddr* p_peer_saddr, socklen_t* plen);
__declspec_dll void IoOverlapped_peer_sockaddr(IoOverlapped_t* ol, struct sockaddr** pp_saddr, socklen_t* plen);
__declspec_dll void IoOverlapped_free(IoOverlapped_t* ol);
__declspec_dll int IoOverlapped_check_reuse_able(IoOverlapped_t* ol);

#ifdef	__cplusplus
}
#endif

#endif
