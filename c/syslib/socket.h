//
// Created by hujianzhe
//

#ifndef	UTIL_C_SYSLIB_SOCKET_H
#define	UTIL_C_SYSLIB_SOCKET_H

#include "../platform_define.h"

#if defined(_WIN32) || defined(_WIN64)
	#include <ws2tcpip.h>
	#include <iphlpapi.h>
	#include <winnetwk.h>
	#include <mswsock.h>
	#include <mstcpip.h>
	typedef struct if_nameindex {
		unsigned int if_index;
		char if_name[IF_NAMESIZE];
	} if_nameindex_t;
	#define	SHUT_RD					SD_RECEIVE
	#define	SHUT_WR					SD_SEND
	#define	SHUT_RDWR				SD_BOTH
	#pragma comment(lib, "wsock32.lib")
	#pragma comment(lib, "ws2_32.lib")
	#pragma comment(lib, "iphlpapi.lib")
#else
	#include <sys/socket.h>
	#if defined(__FreeBSD__) || defined(__APPLE__)
		#include <net/if_dl.h>
		#include <net/if_types.h>
	#elif __linux__
		#include <netpacket/packet.h>
		#include <linux/rtnetlink.h>
	#endif
	#include <sys/select.h>
	#include <poll.h>
	#include <netdb.h>
	#include <arpa/inet.h>
	#include <net/if.h>
	#include <net/if_arp.h>
	#include <netinet/tcp.h>
	#include <ifaddrs.h>
	#define	SD_RECEIVE				SHUT_RD
	#define	SD_SEND					SHUT_WR
	#define	SD_BOTH					SHUT_RDWR
	typedef struct if_nameindex		if_nameindex_t;
#endif

typedef char IPString_t[INET6_ADDRSTRLEN];
enum {
	IP_TYPE_UNKNOW,
	IPv4_TYPE_A,
	IPv4_TYPE_B,
	IPv4_TYPE_C,
	IPv4_TYPE_D,
	IPv4_TYPE_E,
	IPv6_TYPE_GLOBAL,
	IPv6_TYPE_LINK,
	IPv6_TYPE_SITE,
	IPv6_TYPE_v4MAP
};
enum {
	NETWORK_INTERFACE_UNKNOWN,
	NETWORK_INTERFACE_LOOPBACK,
	NETWORK_INTERFACE_PPP,
	NETWORK_INTERFACE_TOKENRING,
	NETWORK_INTERFACE_ETHERNET,
	NETWORK_INTERFACE_WIRELESS,
	NETWORK_INTERFACE_FIREWIRE
};
struct address_list {
	struct address_list* next;
	struct sockaddr_storage ip;
	struct sockaddr_storage mask;
};
typedef struct NetworkInterfaceInfo_t {
	struct NetworkInterfaceInfo_t* next;
	unsigned int if_index;
	char if_name[IF_NAMESIZE];
	int if_type;
	unsigned int mtu;
	unsigned int phyaddrlen;
#if defined(_WIN32) || defined(_WIN64)
	unsigned char phyaddr[MAX_ADAPTER_ADDRESS_LENGTH];
#else
	unsigned char phyaddr[8];
#endif
	struct address_list* address;
} NetworkInterfaceInfo_t;

#ifdef	__cplusplus
extern "C" {
#endif

/* Network */
#if defined(_WIN32) || defined(_WIN64)
UTIL_LIBAPI if_nameindex_t* if_nameindex(void);
UTIL_LIBAPI void if_freenameindex(if_nameindex_t* ptr);
#else
#if	__linux__
unsigned long long ntohll(unsigned long long val);
unsigned long long htonll(unsigned long long val);
#endif
unsigned int htonf(float val);
float ntohf(unsigned int val);
unsigned long long htond(double val);
double ntohd(unsigned long long val);
#endif
UTIL_LIBAPI BOOL networkSetupEnv(void);
UTIL_LIBAPI BOOL networkCleanEnv(void);
UTIL_LIBAPI NetworkInterfaceInfo_t* networkInterfaceInfo(void);
UTIL_LIBAPI void networkFreeInterfaceInfo(NetworkInterfaceInfo_t* info);
/* SOCKET ADDRESS */
UTIL_LIBAPI int sockaddrIPType(const struct sockaddr_storage* sa);
UTIL_LIBAPI const char* ipstrGetLoopback(int family);
UTIL_LIBAPI BOOL ipstrIsLoopback(const char* ip);
UTIL_LIBAPI BOOL ipstrIsInner(const char* ip);
UTIL_LIBAPI int ipstrFamily(const char* ip);
UTIL_LIBAPI BOOL sockaddrEncode(struct sockaddr_storage* saddr, int af, const char* strIP, unsigned short port);
UTIL_LIBAPI BOOL sockaddrDecode(const struct sockaddr_storage* saddr, char* strIP, unsigned short* port);
UTIL_LIBAPI BOOL sockaddrSetPort(struct sockaddr_storage* saddr, unsigned short port);
/* SOCKET */
UTIL_LIBAPI BOOL socketBindAddr(FD_t sockfd, const struct sockaddr_storage* saddr);
UTIL_LIBAPI BOOL socketGetLocalAddr(FD_t sockfd, struct sockaddr_storage* saddr);
UTIL_LIBAPI BOOL socketGetPeerAddr(FD_t sockfd, struct sockaddr_storage* saddr);
#if defined(_WIN32) || defined(_WIN64)
#define	socketClose(sockfd)	(closesocket((SOCKET)(sockfd)) == 0)
#else
#define	socketClose(sockfd)	(close(sockfd) == 0)
#endif
UTIL_LIBAPI int socketError(FD_t sockfd);
UTIL_LIBAPI BOOL socketUdpConnect(FD_t sockfd, const struct sockaddr_storage* saddr);
UTIL_LIBAPI BOOL socketUdpDisconnect(FD_t sockfd);
UTIL_LIBAPI FD_t socketTcpConnect(const struct sockaddr_storage* saddr, int msec);
#define socketTcpListen(sockfd)		(listen(sockfd, SOMAXCONN) == 0)
UTIL_LIBAPI BOOL socketIsListened(FD_t sockfd, BOOL* bool_value);
UTIL_LIBAPI FD_t socketTcpAccept(FD_t listenfd, int msec, struct sockaddr_storage* from);
#define	socketShutdown(sockfd, how)	(shutdown(sockfd, how) == 0)
UTIL_LIBAPI BOOL socketPair(int type, FD_t sockfd[2]);
UTIL_LIBAPI int socketRead(FD_t sockfd, void* buf, unsigned int nbytes, int flags, struct sockaddr_storage* from);
UTIL_LIBAPI int socketWrite(FD_t sockfd, const void* buf, unsigned int nbytes, int flags, const struct sockaddr_storage* to);
UTIL_LIBAPI int socketReadv(FD_t sockfd, Iobuf_t iov[], unsigned int iovcnt, int flags, struct sockaddr_storage* saddr);
UTIL_LIBAPI int socketWritev(FD_t sockfd, Iobuf_t iov[], unsigned int iovcnt, int flags, const struct sockaddr_storage* saddr);
UTIL_LIBAPI int socketTcpReadAll(FD_t sockfd, void* buf, unsigned int nbytes);
UTIL_LIBAPI int socketTcpWriteAll(FD_t sockfd, const void* buf, unsigned int nbytes);
#define socketTcpSendOOB(sockfd, oob) (send(sockfd, (char*)&(oob), 1, MSG_OOB) == 1)
UTIL_LIBAPI int socketTcpCanRecvOOB(FD_t sockfd);
UTIL_LIBAPI BOOL socketTcpReadOOB(FD_t sockfd, unsigned char* oob);
UTIL_LIBAPI int socketSelect(int nfds, fd_set* rset, fd_set* wset, fd_set* xset, int msec);
UTIL_LIBAPI int socketPoll(struct pollfd* fdarray, unsigned long nfds, int msec);
UTIL_LIBAPI BOOL socketNonBlock(FD_t sockfd, BOOL bool_val);
UTIL_LIBAPI int socketTcpReadableBytes(FD_t sockfd);
UTIL_LIBAPI BOOL socketSetSendTimeout(FD_t sockfd, int msec);
UTIL_LIBAPI BOOL socketSetRecvTimeout(FD_t sockfd, int msec);
UTIL_LIBAPI BOOL socketSetUnicastTTL(FD_t sockfd, int family, unsigned char ttl);
UTIL_LIBAPI BOOL socketSetMulticastTTL(FD_t sockfd, int family, int ttl);
UTIL_LIBAPI BOOL socketUdpMcastGroupJoin(FD_t sockfd, const struct sockaddr_storage* grp);
UTIL_LIBAPI BOOL socketUdpMcastGroupLeave(FD_t sockfd, const struct sockaddr_storage* grp);
UTIL_LIBAPI BOOL socketUdpMcastEnableLoop(FD_t sockfd, int family, BOOL bool_val);

#ifdef	__cplusplus
}
#endif

#endif
