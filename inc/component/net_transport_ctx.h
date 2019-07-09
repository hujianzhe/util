//
// Created by hujianzhe on 19-7-9.
//

#ifndef UTIL_C_COMPONENT_NET_TRANSPORT_CTX_H
#define	UTIL_C_COMPONENT_NET_TRANSPORT_CTX_H

#include "../datastruct/list.h"
#include "../sysapi/socket.h"

enum {
	NETPACKET_FRAGMENT = 1,
	NETPACKET_FRAGMENT_EOF,
	NETPACKET_ACK,
	NETPACKET_EOF
};

typedef struct NetPacket_t {
	ListNode_t* listnode;
	void* userdata;
	union {
		/* udp use */
		struct {
			unsigned char resendtimes;
			long long resend_timestamp_msec;
		};
		/* tcp use */
		struct {
			unsigned short need_ack;
			int offset;
		};
	};
	unsigned char type;
	unsigned int seq;
	int hdrlen;
	int len;
	unsigned char data[1];
} NetPacket_t;

#ifdef __cplusplus
extern "C" {
#endif

__declspec_dll NetPacket_t* netpacketSharding(const Iobuf_t iov[], unsigned int iovcnt, unsigned short mtu, int(*get_hdrlen)(int));
__declspec_dll void netpacketMerge(List_t* pklist, Iobuf_t* iov);

#ifdef __cplusplus
}
#endif

#endif // !UTIL_C_COMPONENT_NET_TRANSPORT_CTX_H
