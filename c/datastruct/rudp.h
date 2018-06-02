//
// Created by hujianzhe
//

#ifndef	UTIL_C_RUDP_H
#define	UTIL_C_RUDP_H

#pragma pack(1)
struct rudp_hdr {
	unsigned char type;
	unsigned long long seq;
};
#pragma pack()

// inner struct, don't use
struct rudp_recv_cache {
	unsigned short len;
	const struct rudp_hdr* hdr;
};
struct rudp_send_cache {
	struct rudp_send_cache *prev, *next;
	long long last_resend_msec;
	int resend_times;
	unsigned short len;
	const struct rudp_hdr* hdr;
};
// outer struct
#define	RUDP_WND_SIZE	256
struct rudp_ctx {
// public
	int max_resend_times;
	int first_resend_interval_msec;

	void* userdata;
	void(*send_callback)(struct rudp_ctx*, const struct rudp_hdr*, unsigned short);
	void(*recv_callback)(struct rudp_ctx*, const struct rudp_hdr*, unsigned short);
	void(*free_callback)(struct rudp_ctx*, const struct rudp_hdr*);

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

void rudp_ctx_clean(struct rudp_ctx* ctx);
void rudp_recv_sort_and_ack(struct rudp_ctx* ctx, long long now_timestamp_msec, const struct rudp_hdr* hdr, unsigned short len);
int rudp_send(struct rudp_ctx* ctx, long long now_timestamp_msec, struct rudp_hdr* hdr, unsigned short len);
int rudp_check_resend(struct rudp_ctx* ctx, long long now_timestamp_msec, int* next_wait_msec);

#ifdef	__cplusplus
}
#endif

#endif
