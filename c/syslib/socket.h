//
// Created by hujianzhe
//

#ifndef	UTIL_C_SYSLIB_SOCKET_H
#define	UTIL_C_SYSLIB_SOCKET_H

#include "platform_define.h"

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

typedef char IP_STRING[INET6_ADDRSTRLEN];
typedef enum {
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
} IP_TYPE;

typedef enum NETWORK_INTERFACE_TYPE {
	NETWORK_INTERFACE_UNKNOWN,
	NETWORK_INTERFACE_LOOPBACK,
	NETWORK_INTERFACE_PPP,
	NETWORK_INTERFACE_TOKENRING,
	NETWORK_INTERFACE_ETHERNET,
	NETWORK_INTERFACE_WIRELESS,
	NETWORK_INTERFACE_FIREWIRE
} NETWORK_INTERFACE_TYPE;
struct address_list {
	struct address_list* next;
	struct sockaddr_storage ip;
	struct sockaddr_storage mask;
};
typedef struct NETWORK_INTERFACE_INFO {
	struct NETWORK_INTERFACE_INFO* next;
	unsigned int if_index;
	char if_name[IF_NAMESIZE];
	NETWORK_INTERFACE_TYPE if_type;
	unsigned int mtu;
	unsigned int phyaddrlen;
#if defined(_WIN32) || defined(_WIN64)
	unsigned char phyaddr[MAX_ADAPTER_ADDRESS_LENGTH];
#else
	unsigned char phyaddr[8];
#endif
	struct address_list* address;
} NETWORK_INTERFACE_INFO;

#ifdef	__cplusplus
extern "C" {
#endif

/* Network */
#if defined(_WIN32) || defined(_WIN64)
if_nameindex_t* if_nameindex(void);
void if_freenameindex(if_nameindex_t* ptr);
#elif __linux__
unsigned long long ntohll(unsigned long long val);
unsigned long long htonll(unsigned long long val);
#endif
EXEC_RETURN network_SetupEnv(void);
EXEC_RETURN network_CleanEnv(void);
NETWORK_INTERFACE_INFO* network_InterfaceInfo(void);
void network_FreeInterfaceInfo(NETWORK_INTERFACE_INFO* info);
/* SOCKET ADDRESS */
IP_TYPE IPtype(const struct sockaddr_storage* sa);
const char* loopbackIPstring(int family);
BOOL IPstringIsLoopBack(const char* ip);
BOOL IPstringIsInner(const char* ip);
int IPstring2Family(const char* ip);
struct addrinfo* sock_AddrInfoList(const char* host, const char* serv, int ai_socktype);
EXEC_RETURN sock_Text2Addr(struct sockaddr_storage* saddr, int af, const char* strIP, unsigned short port);
EXEC_RETURN sock_Addr2Text(const struct sockaddr_storage* saddr, char* strIP, unsigned short* port);
EXEC_RETURN sock_AddrSetPort(struct sockaddr_storage* saddr, unsigned short port);
EXEC_RETURN sock_BindSockaddr(FD_t sockfd, const struct sockaddr_storage* saddr);
EXEC_RETURN sock_GetSockAddr(FD_t sockfd, struct sockaddr_storage* saddr);
EXEC_RETURN sock_GetPeerAddr(FD_t sockfd, struct sockaddr_storage* saddr);
/* SOCKET */
#if defined(_WIN32) || defined(_WIN64)
#define	sock_Close(sockfd)	(closesocket((SOCKET)(sockfd)) == 0 ? EXEC_SUCCESS : EXEC_ERROR)
#else
#define	sock_Close(sockfd)	(close(sockfd) == 0 ? EXEC_SUCCESS : EXEC_ERROR)
#endif
int sock_Type(FD_t sockfd);
int sock_Error(FD_t sockfd);
EXEC_RETURN sock_UdpConnect(FD_t sockfd, const struct sockaddr_storage* saddr);
EXEC_RETURN sock_UdpDisconnect(FD_t sockfd);
FD_t sock_TcpConnect(const struct sockaddr_storage* saddr, int msec);
#define sock_TcpListen(sockfd)		(listen(sockfd, SOMAXCONN) == 0 ? EXEC_SUCCESS : EXEC_ERROR)
EXEC_RETURN sock_IsListened(FD_t sockfd, BOOL* bool_value);
FD_t sock_TcpAccept(FD_t listenfd, int msec, struct sockaddr_storage* from);
#define	sock_Shutdown(sockfd, how)	(shutdown(sockfd, how) == 0 ? EXEC_SUCCESS : EXEC_ERROR)
EXEC_RETURN sock_Pair(int type, FD_t sockfd[2]);
/* SOCKET IO */
EXEC_RETURN sock_NonBlock(FD_t sockfd, BOOL bool_val);
int sock_TcpReadableBytes(FD_t sockfd);
int sock_Recv(FD_t sockfd, void* buf, unsigned int nbytes, int flags, struct sockaddr_storage* from);
int sock_Send(FD_t sockfd, const void* buf, unsigned int nbytes, int flags, const struct sockaddr_storage* to);
int sock_RecvVec(FD_t sockfd, IoBuf_t* iov, unsigned int iovcnt, int flags, struct sockaddr_storage* saddr);
int sock_SendVec(FD_t sockfd, IoBuf_t* iov, unsigned int iovcnt, int flags, const struct sockaddr_storage* saddr);
int sock_TcpRecvAll(FD_t sockfd, void* buf, unsigned int nbytes);
int sock_TcpSendAll(FD_t sockfd, const void* buf, unsigned int nbytes);
#define sock_TcpSendOOB(sockfd, oob) (send(sockfd, (char*)&(oob), 1, MSG_OOB) == 1 ? EXEC_SUCCESS : EXEC_ERROR)
int sock_TcpCanRecvOOB(FD_t sockfd);
EXEC_RETURN sock_TcpRecvOOB(FD_t sockfd, unsigned char* oob);
/* SOCKET Reactor_t */
int sock_Select(int nfds, fd_set* rset, fd_set* wset, fd_set* xset, int msec);
int sock_Poll(struct pollfd* fdarray, unsigned long nfds, int msec);
/* SOCKET OPTION */
EXEC_RETURN sock_SetSendTimeOut(FD_t sockfd, int msec);
EXEC_RETURN sock_SetRecvTimeOut(FD_t sockfd, int msec);
EXEC_RETURN sock_SetRecvMininumSize(FD_t sockfd, int size);
EXEC_RETURN sock_SetSendMininumSize(FD_t sockfd, int size);
int sock_GetSendBufSize(FD_t sockfd);
EXEC_RETURN sock_SetSendBufSize(FD_t sockfd, int size);
int sock_GetRecvBufSize(FD_t sockfd);
EXEC_RETURN sock_SetRecvBufSize(FD_t sockfd, int size);
EXEC_RETURN sock_SetUniCastTTL(FD_t sockfd, unsigned char ttl);
EXEC_RETURN sock_SetMultiCastTTL(FD_t sockfd, int ttl);
EXEC_RETURN sock_TcpGetMSS(FD_t sockfd, int* mss);
EXEC_RETURN sock_TcpSetMSS(FD_t sockfd, int mss);
EXEC_RETURN sock_TcpNoDelay(FD_t sockfd, BOOL bool_val);
EXEC_RETURN sock_TcpEnableProbePeerAlive(FD_t sockfd, BOOL bool_val);
EXEC_RETURN sock_TcpEnableOOBInLine(FD_t sockfd, BOOL bool_val);
EXEC_RETURN sock_TcpEnableLinger(FD_t sockfd, BOOL bool_val, unsigned int sec);
EXEC_RETURN sock_UdpEnableBroadcast(FD_t sockfd, BOOL bool_val);
EXEC_RETURN sock_UdpMcastGroupJoin(FD_t sockfd, const struct sockaddr_storage* grp);
EXEC_RETURN sock_UdpMcastGroupLeave(FD_t sockfd, const struct sockaddr_storage* grp);
EXEC_RETURN sock_UdpMcastEnableLoop(FD_t sockfd, BOOL bool_val);
EXEC_RETURN sock_UdpMcastGetInterface(FD_t sockfd, struct in_addr* iaddr, unsigned int* ifindex);
EXEC_RETURN sock_UdpMcastSetInterface(FD_t sockfd, struct in_addr iaddr, unsigned int ifindex);

#ifdef	__cplusplus
}
#endif

#endif
