//
// Created by hujianzhe
//

#ifndef	UTIL_C_SYSLIB_IO_OVERLAPPED_H
#define	UTIL_C_SYSLIB_IO_OVERLAPPED_H

#include "../platform_define.h"

#if defined(_WIN32) || defined(_WIN64)
typedef struct IocpOverlapped_t {
	OVERLAPPED ol;
	unsigned char commit;
	unsigned char free_flag;
	unsigned short opcode;
	void* completion_key;
} IoOverlapped_t;
#else
typedef struct UnixOverlapped_t {
	unsigned char commit;
	unsigned char free_flag;
	unsigned short opcode;
	int result;
	void* completion_key;
} IoOverlapped_t;
#endif

enum {
	IO_OVERLAPPED_OP_READ = 1,
	IO_OVERLAPPED_OP_WRITE = 2,
	IO_OVERLAPPED_OP_ACCEPT = 3,
	IO_OVERLAPPED_OP_CONNECT = 4,
};

/* note: internal define, not direct use */

#if defined(_WIN32) || defined(_WIN64)
typedef struct IocpReadOverlapped_t {
	IoOverlapped_t base;
	struct sockaddr_storage saddr;
	int saddrlen;
	DWORD dwNumberOfBytesTransferred;
	WSABUF wsabuf;
	DWORD dwFlags;
	unsigned char append_data[1]; /* convienent for text data */
} IocpReadOverlapped_t;

typedef struct IocpWriteOverlapped_t {
	IoOverlapped_t base;
	struct sockaddr_storage saddr;
	int saddrlen;
	DWORD dwNumberOfBytesTransferred;
	WSABUF wsabuf;
	unsigned char append_data[1]; /* convienent for text data */
} IocpWriteOverlapped_t, IocpConnectExOverlapped_t;

typedef struct IocpAcceptExOverlapped_t {
	IoOverlapped_t base;
	SOCKET acceptsocket;
	unsigned char saddrs[sizeof(struct sockaddr_storage) + 16 + sizeof(struct sockaddr_storage) + 16];
} IocpAcceptExOverlapped_t;
#else
#include <sys/socket.h>
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

__declspec_dll IoOverlapped_t* IoOverlapped_alloc(int opcode, const void* refbuf, unsigned int refsize, unsigned int appendsize);
__declspec_dll long long IoOverlapped_get_file_offset(IoOverlapped_t* ol);
__declspec_dll IoOverlapped_t* IoOverlapped_set_file_offest(IoOverlapped_t* ol, long long offset);
__declspec_dll FD_t IoOverlapped_pop_acceptfd(IoOverlapped_t* ol);
__declspec_dll void IoOverlapped_free(IoOverlapped_t* ol);

#ifdef	__cplusplus
}
#endif

#endif
