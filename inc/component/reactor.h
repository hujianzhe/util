//
// Created by hujianzhe on 2019-07-14.
//

#ifndef	UTIL_C_COMPONENT_REACTOR_H
#define	UTIL_C_COMPONENT_REACTOR_H

#include "../sysapi/atomic.h"
#include "../sysapi/io.h"
#include "../sysapi/ipc.h"
#include "../sysapi/process.h"
#include "../sysapi/socket.h"
#include "../datastruct/list.h"
#include "../datastruct/hashtable.h"
#include "../datastruct/transport_ctx.h"

typedef NetPacketListNode_t	ReactorCmd_t;
enum {
	REACTOR_REG_CMD = 1,
	REACTOR_FREE_CMD,
	REACTOR_SEND_PACKET_CMD
};

typedef struct Reactor_t {
	/* public */
	void(*exec_cmd)(ReactorCmd_t* cmdnode, long long timestamp_msec);
	void(*free_cmd)(ReactorCmd_t* cmdnode);
	/* private */
	unsigned char m_runthreadhasbind;
	long long m_event_msec;
	Atom16_t m_wake;
	Thread_t m_runthread;
	Nio_t m_nio;
	FD_t m_socketpair[2];
	void* m_readol;
	CriticalSection_t m_cmdlistlock;
	List_t m_cmdlist;
	List_t m_invalidlist;
	Hashtable_t m_objht;
	HashtableNode_t* m_objht_bulks[2048];
} Reactor_t;

typedef struct ReactorObject_t {
	/* public */
	FD_t fd;
	int domain;
	int socktype;
	int protocol;
	Reactor_t* reactor;
	void* userdata;
	int invalid_timeout_msec;
	unsigned int write_fragment_size;
	volatile int valid;
	void(*exec)(struct ReactorObject_t* self, long long timestamp_msec);
	int(*preread)(struct ReactorObject_t* self, unsigned char* buf, unsigned int len, unsigned int off, long long timestamp_msec, const void* from_addr);
	void(*writeev)(struct ReactorObject_t* self, long long timestamp_msec);
	void(*inactive)(struct ReactorObject_t* self);
	void(*free)(struct ReactorObject_t* self);
	union {
		union {
			struct {
				Sockaddr_t listen_addr;
				void(*accept)(struct ReactorObject_t* self, FD_t newfd, const void* peeraddr, long long timestamp_msec);
			};
			struct {
				Sockaddr_t connect_addr;
				void(*connect)(struct ReactorObject_t* self, int err, long long timestamp_msec);
				StreamTransportCtx_t ctx;
				void(*free_sendfinished)(NetPacket_t* packet);
			};
		} stream;
		struct {
			unsigned short read_mtu;
		} dgram;
	};
	ReactorCmd_t regcmd;
	ReactorCmd_t freecmd;
	/* private */
	HashtableNode_t m_hashnode;
	ListNode_t m_invalidnode;
	void* m_readol;
	void* m_writeol;
	long long m_invalid_msec;
	char m_readol_has_commit;
	char m_writeol_has_commit;
	char m_stream_connected;
	char m_stream_listened;
	unsigned char* m_inbuf;
	int m_inbufoff;
	int m_inbuflen;
} ReactorObject_t;

#ifdef	__cplusplus
extern "C" {
#endif

__declspec_dll Reactor_t* reactorInit(Reactor_t* reactor);
__declspec_dll void reactorWake(Reactor_t* reactor);
__declspec_dll void reactorCommitCmd(Reactor_t* reactor, ReactorCmd_t* cmdnode);
__declspec_dll int reactorHandle(Reactor_t* reactor, NioEv_t e[], int n, long long timestamp_msec, int wait_msec);
__declspec_dll void reactorDestroy(Reactor_t* reactor);
__declspec_dll void reactorSetEventTimestamp(Reactor_t* reactor, long long timestamp_msec);

__declspec_dll ReactorObject_t* reactorobjectInit(ReactorObject_t* o, FD_t fd, int domain, int socktype, int protocol);
__declspec_dll int reactorobjectRequestRead(ReactorObject_t* o);
__declspec_dll int reactorobjectRequestWrite(ReactorObject_t* o);
__declspec_dll ReactorObject_t* reactorobjectInvalid(ReactorObject_t* o, long long timestamp_msec);

__declspec_dll void reactorobjectSendPacket(ReactorObject_t* o, NetPacket_t* packet);
__declspec_dll void reactorobjectSendPacketList(ReactorObject_t* o, List_t* packetlist);
__declspec_dll int reactorobjectSendStreamData(ReactorObject_t* o, const void* buf, unsigned int len, int pktype);

#ifdef	__cplusplus
}
#endif

#endif