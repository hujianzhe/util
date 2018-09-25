//
// Created by hujianzhe
//

#ifndef	UTIL_C_DATASTRUCT_RUDP_H
#define	UTIL_C_DATASTRUCT_RUDP_H

#include "../compiler_define.h"

#pragma pack(1)
struct RudpHdr_t {
	unsigned char type;
	unsigned long long seq;
};
#pragma pack()

// inner struct, don't use
struct rudp_recv_cache {
	unsigned short len;
	const struct RudpHdr_t* hdr;
};
struct rudp_send_cache {
	struct rudp_send_cache *prev, *next;
	long long last_resend_msec;
	int resend_times;
	unsigned short len;
	const struct RudpHdr_t* hdr;
};
// outer struct
#define	RUDP_WND_SIZE	256
struct RudpCtx_t {
// public
	int max_resend_times;
	int first_resend_interval_msec;

	void* userdata;
	void(*send_callback)(struct RudpCtx_t*, const struct RudpHdr_t*, unsigned short);
	void(*recv_callback)(struct RudpCtx_t*, const struct RudpHdr_t*, unsigned short);
	void(*free_callback)(struct RudpCtx_t*, const struct RudpHdr_t*);

// private
	unsigned char ack_seq;
	unsigned long long send_seq;
	unsigned long long recv_seq;
	struct rudp_recv_cache recv_wnd[RUDP_WND_SIZE];
	struct rudp_send_cache send_wnd[RUDP_WND_SIZE];
	struct rudp_send_cache *send_head, *send_tail;
};

#ifdef	__cplusplus
extern "C" {
#endif

__declspec_dll void rudpCleanCtx(struct RudpCtx_t* ctx);
__declspec_dll void rudpRecvSortAndAck(struct RudpCtx_t* ctx, long long now_timestamp_msec, const struct RudpHdr_t* hdr);
__declspec_dll int rudpSend(struct RudpCtx_t* ctx, long long now_timestamp_msec, struct RudpHdr_t* hdr, unsigned short len);
__declspec_dll int rudpCheckResend(struct RudpCtx_t* ctx, long long now_timestamp_msec, int* next_wait_msec);

#ifdef	__cplusplus
}
#endif

#endif
