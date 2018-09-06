//
// Created by hujianzhe
//

#include "rudp.h"

#ifdef	__cplusplus
extern "C" {
#endif

static unsigned long long htonll(unsigned long long v) {
	short is_little_endian = 1;
	if (*(unsigned char*)(&is_little_endian)) {
		int i;
		unsigned char* p = (unsigned char*)&v;
		for (i = 0; i < sizeof(v) / 2; ++i) {
			unsigned char temp = p[i];
			p[i] = p[sizeof(v) - i - 1];
			p[sizeof(v) - i - 1] = temp;
		}
	}
	return v;
}
static unsigned long long ntohll(unsigned long long v) {
	short is_little_endian = 1;
	if (*(unsigned char*)(&is_little_endian)) {
		int i;
		unsigned char* p = (unsigned char*)&v;
		for (i = 0; i < sizeof(v) / 2; ++i) {
			unsigned char temp = p[i];
			p[i] = p[sizeof(v) - i - 1];
			p[sizeof(v) - i - 1] = temp;
		}
	}
	return v;
}

enum hdrtype {
	RUDP_HDR_TYPE_DATA,
	RUDP_HDR_TYPE_ACK
};

void rudpCleanCtx(struct RudpCtx_t* ctx) {
	int i;
	if (ctx->free_callback) {
		for (i = 0; i < RUDP_WND_SIZE; ++i) {
			if (ctx->recv_wnd[i].hdr) {
				ctx->free_callback(ctx, ctx->recv_wnd[i].hdr);
			}
			if (ctx->send_wnd[i].hdr) {
				ctx->free_callback(ctx, ctx->send_wnd[i].hdr);
			}
		}
	}
	for (i = 0; i < sizeof(*ctx); ++i) {
		((unsigned char*)ctx)[i] = 0;
	}
}

void rudpRecvSortAndAck(struct RudpCtx_t* ctx, long long now_timestamp_msec, const struct RudpHdr_t* hdr) {
	unsigned long long hdr_seq = ntohll(hdr->seq);
	switch (hdr->type) {
		case RUDP_HDR_TYPE_DATA:
		{
			int i;
			struct rudp_recv_cache* cache;

			// TODO: try ack
			do {
				struct RudpHdr_t ack_hdr = { RUDP_HDR_TYPE_ACK, hdr->seq };
				ctx->send_callback(ctx, &ack_hdr, sizeof(ack_hdr));
			} while (0);

			// check seq is valid
			if (hdr_seq < RUDP_WND_SIZE) {}
			else if (hdr_seq < ctx->recv_seq) {
				break;
			}
			if (hdr_seq - ctx->recv_seq >= RUDP_WND_SIZE) {
				break;
			}

			cache = &ctx->recv_wnd[hdr_seq % RUDP_WND_SIZE];
			if (cache->hdr) {
				// already exist ... 
				break;
			}
			cache->hdr = hdr;

			if (ctx->recv_seq != hdr_seq) {
				// packet disOrder !!!
				break;
			}

			for (i = 0; i < RUDP_WND_SIZE; ++i) {
				cache = &ctx->recv_wnd[ctx->recv_seq % RUDP_WND_SIZE];
				if (!cache->hdr) {
					// wait recv
					break;
				}

				// packet has order !
				ctx->recv_callback(ctx, cache->hdr, cache->len);

				// free packet
				ctx->free_callback(ctx, cache->hdr);

				cache->hdr = (struct RudpHdr_t*)0;
				ctx->recv_seq++;
			}

			break;
		}

		case RUDP_HDR_TYPE_ACK:
		{
			int ack_seq = hdr_seq % RUDP_WND_SIZE;
			// ack success and should be free
			struct rudp_send_cache* cache = &ctx->send_wnd[ack_seq];
			if (cache->hdr) {
				if (ntohll(cache->hdr->seq) != hdr_seq) {
					break;
				}
				ctx->free_callback(ctx, cache->hdr);
				cache->hdr = (struct RudpHdr_t*)0;

				if (cache->prev)
					cache->prev->next = cache->next;
				if (cache->next)
					cache->next->prev = cache->prev;
				if (cache == ctx->send_head)
					ctx->send_head = cache->next;
				if (cache == ctx->send_tail)
					ctx->send_tail = cache->prev;
			}
			else {
				break;
			}

			// update send ack_seq
			if (ack_seq == ctx->ack_seq) {
				do {
					++ctx->ack_seq;
					if (ctx->send_wnd[ctx->ack_seq].hdr) {
						break;
					}
				} while (ctx->ack_seq != ctx->send_seq % RUDP_WND_SIZE);
			}
			else if (ctx->ack_seq + 2 <= ack_seq) {
				// fast resend
				cache = &ctx->send_wnd[ctx->ack_seq];
				if (cache->hdr) {
					cache->last_resend_msec = now_timestamp_msec;
					ctx->send_callback(ctx, cache->hdr, cache->len);
				}
			}

			ctx->free_callback(ctx, hdr);

			break;
		}

		default:
		{
			ctx->free_callback(ctx, hdr);
		}
	}
}

int rudpSend(struct RudpCtx_t* ctx, long long now_timestamp_msec, struct RudpHdr_t* hdr, unsigned short len) {
	struct rudp_send_cache* cache;
	int ack_seq = ctx->send_seq % RUDP_WND_SIZE;
	if (ctx->send_wnd[ack_seq].hdr) {
		// packet hasn't be ack
		return -1;
	}
	hdr->type = RUDP_HDR_TYPE_DATA;
	hdr->seq = htonll(ctx->send_seq++);

	// wait ack
	cache = ctx->send_wnd + ack_seq;
	cache->hdr = hdr;
	cache->len = len;
	cache->resend_times = 0;
	cache->last_resend_msec = now_timestamp_msec;

	if (ctx->send_tail) {
		ctx->send_tail->next = cache;
		cache->prev = ctx->send_tail;
	}
	else {
		ctx->send_head = cache;
		cache->prev = (struct rudp_send_cache*)0;
	}
	ctx->send_tail = cache;
	cache->next = (struct rudp_send_cache*)0;

	// try send
	ctx->send_callback(ctx, hdr, len);

	return 0;
}

int rudpCheckResend(struct RudpCtx_t* ctx, long long now_timestamp_msec, int* next_wait_msec) {
	struct rudp_send_cache* cache;
	*next_wait_msec = -1;
	for (cache = ctx->send_head; cache; cache = cache->next) {
		int delta_timelen, rto;
		if (!cache->hdr) {
			// this wnd hasn't packet
			continue;
		}

		// check resend timeout
		delta_timelen = (int)(now_timestamp_msec - cache->last_resend_msec);
		if (delta_timelen < 0) {
			delta_timelen = -delta_timelen;
		}
		// calculator rto
		rto = ctx->first_resend_interval_msec;
		if (cache->resend_times) {
			rto += ctx->first_resend_interval_msec >> 1;
		}
		if (delta_timelen < rto) {
			if (-1 == *next_wait_msec || rto - delta_timelen < *next_wait_msec) {
				// update next wait millionsecond
				*next_wait_msec = rto - delta_timelen;
			}
			continue;
		}

		// check resend times overflow
		if (cache->resend_times + 1 > ctx->max_resend_times) {
			// network maybe lost !
			return -1;
		}

		// TODO: try resend
		++cache->resend_times;
		cache->last_resend_msec = now_timestamp_msec;
		ctx->send_callback(ctx, cache->hdr, cache->len);
	}
	return 0;
}

#ifdef	__cplusplus
}
#endif
