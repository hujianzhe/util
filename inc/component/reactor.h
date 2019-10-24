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

enum {
	REACTOR_OBJECT_REG_CMD = 1,
	REACTOR_OBJECT_DETACH_CMD,
	REACTOR_OBJECT_FREE_CMD,
	REACTOR_STREAM_SENDFIN_CMD,

	REACTOR_CHANNEL_FREE_CMD,

	REACTOR_INNER_CMD,

	REACTOR_SEND_PACKET_CMD,
	REACTOR_USER_CMD
};
enum {
	REACTOR_REG_ERR = 1,
	REACTOR_IO_ERR,
	REACTOR_CONNECT_ERR,
	REACTOR_ZOMBIE_ERR,
	REACTOR_ONREAD_ERR
};
typedef ListNodeTemplateDeclare(int, type)	ReactorCmd_t;

typedef struct Reactor_t {
	/* public */
	void(*cmd_exec)(ReactorCmd_t* cmdnode);
	void(*cmd_free)(ReactorCmd_t* cmdnode);
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
	int detach_error;
	int detach_timeout_msec;
	unsigned int read_fragment_size;
	unsigned int write_fragment_size;
	struct {
		Sockaddr_t connect_addr;
		ReactorCmd_t sendfincmd;
		Atom8_t m_sendfincmdhaspost;
		char m_connected;
		char m_listened;
	} stream;
	ReactorCmd_t regcmd;
	ReactorCmd_t detachcmd;
	ReactorCmd_t freecmd;
	/* channel objcet list */
	List_t channel_list;
	/* interface */
	void(*reg)(struct ReactorObject_t* self, long long timestamp_msec);
	void(*writeev)(struct ReactorObject_t* self, long long timestamp_msec);
	void(*detach)(struct ReactorObject_t* self);
/* private */
	HashtableNode_t m_hashnode;
	ListNode_t m_invalidnode;
	Atom8_t m_reghaspost;
	Atom8_t m_detachhaspost;
	char m_valid;
	char m_hashnode_has_insert;
	char m_detach_has_commit;
	char m_readol_has_commit;
	char m_writeol_has_commit;
	void* m_readol;
	void* m_writeol;
	long long m_invalid_msec;
	unsigned char* m_inbuf;
	int m_inbufoff;
	int m_inbuflen;
} ReactorObject_t;

struct ReactorPacket_t;
typedef struct ChannelBase_t {
	ReactorCmd_t regcmd;
	ReactorObject_t* o;
	int detach_error;
	ReactorCmd_t freecmd;
	Sockaddr_t to_addr;
	union {
		Sockaddr_t listen_addr;
		Sockaddr_t connect_addr;
	};
	struct {
		StreamTransportCtx_t stream_ctx;
		void(*stream_recvfin)(struct ChannelBase_t* self, long long timestamp_msec);
		void(*stream_sendfin)(struct ChannelBase_t* self, long long timestamp_msec);
		char stream_sendfinwait;
	};
	char has_recvfin;
	char has_sendfin;
	char detached;

	void(*ack_halfconn)(struct ChannelBase_t* self, FD_t newfd, const void* peer_addr, long long ts_msec); /* listener use */
	void(*syn_ack)(struct ChannelBase_t* self, long long ts_msec); /* listener use */
	void(*reg)(struct ChannelBase_t* self, long long timestamp_msec);
	void(*exec)(struct ChannelBase_t* self, long long timestamp_msec, long long ev_msec);
	int(*on_read)(struct ChannelBase_t* self, unsigned char* buf, unsigned int len, unsigned int off, long long timestamp_msec, const void* from_addr);
	int(*pre_send)(struct ChannelBase_t* self, struct ReactorPacket_t* packet, long long timestamp_msec);
	void(*detach)(struct ChannelBase_t* self);
} ChannelBase_t;

typedef struct ReactorPacket_t {
	ReactorCmd_t cmd;
	ChannelBase_t* channel;
	NetPacket_t _;
} ReactorPacket_t;

#ifdef	__cplusplus
extern "C" {
#endif

__declspec_dll Reactor_t* reactorInit(Reactor_t* reactor);
__declspec_dll void reactorWake(Reactor_t* reactor);
__declspec_dll void reactorCommitCmd(Reactor_t* reactor, ReactorCmd_t* cmdnode);
__declspec_dll int reactorHandle(Reactor_t* reactor, NioEv_t e[], int n, long long timestamp_msec, int wait_msec);
__declspec_dll void reactorDestroy(Reactor_t* reactor);
__declspec_dll void reactorSetEventTimestamp(Reactor_t* reactor, long long timestamp_msec);

__declspec_dll ReactorObject_t* reactorobjectOpen(FD_t fd, int domain, int socktype, int protocol);
__declspec_dll ChannelBase_t* channelbaseOpen(size_t sz, ReactorObject_t* o, const void* addr);

__declspec_dll void channelbaseSendPacket(ChannelBase_t* channel, ReactorPacket_t* packet);
__declspec_dll void channelbaseSendPacketList(ChannelBase_t* channel, List_t* packetlist);
__declspec_dll int channelbaseSend(ChannelBase_t* channel, int pktype, const void* buf, unsigned int len, const void* addr);

#ifdef	__cplusplus
}
#endif

#endif