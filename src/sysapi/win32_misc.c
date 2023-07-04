//
// Created by hujianzhe
//

#ifdef _WIN32
#include "../../inc/platform_define.h"
#include <ws2tcpip.h>
#include <mswsock.h>

#ifdef	__cplusplus
extern "C" {
#endif

BOOL Iocp_PrepareRegUdp(SOCKET fd, int domain) {
	struct sockaddr_storage local_saddr;
	socklen_t slen;
	DWORD dwBytesReturned = 0;
	BOOL bNewBehavior = FALSE;
	/* winsock2 BUG, udp recvfrom WSAECONNRESET(10054) error and post Overlapped IO error */
	if (WSAIoctl(fd, SIO_UDP_CONNRESET, &bNewBehavior, sizeof(bNewBehavior), NULL, 0, &dwBytesReturned, NULL, NULL)) {
		return FALSE;
	}
	/* note: UDP socket need bind a address before call WSA function, otherwise WSAGetLastError return WSAINVALID */
	slen = sizeof(local_saddr);
	if (!getsockname(fd, (struct sockaddr*)&local_saddr, &slen)) {
		return TRUE;
	}
	if (WSAEINVAL != WSAGetLastError()) {
		return FALSE;
	}
	if (AF_INET == domain) {
		struct sockaddr_in* addr_in = (struct sockaddr_in*)&local_saddr;
		slen = sizeof(*addr_in);
		memset(addr_in, 0, sizeof(*addr_in));
		addr_in->sin_family = AF_INET;
		addr_in->sin_port = 0;
		addr_in->sin_addr.s_addr = htonl(INADDR_ANY);
	}
	else if (AF_INET6 == domain) {
		struct sockaddr_in6* addr_in6 = (struct sockaddr_in6*)&local_saddr;
		slen = sizeof(*addr_in6);
		memset(addr_in6, 0, sizeof(*addr_in6));
		addr_in6->sin6_family = AF_INET6;
		addr_in6->sin6_port = 0;
		addr_in6->sin6_addr = in6addr_any;
	}
	else {
		WSASetLastError(WSAEAFNOSUPPORT);
		return FALSE;
	}
	if (bind(fd, (struct sockaddr*)&local_saddr, slen)) {
		return FALSE;
	}
	return TRUE;
}

#ifdef	__cplusplus
}
#endif

#endif // _WIN32