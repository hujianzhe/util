//
// Created by hujianzhe on 2022-10-30
//

#ifndef	UTIL_C_COMPONENT_SWITCH_CO_SCHE_H
#define	UTIL_C_COMPONENT_SWITCH_CO_SCHE_H

#include "../compiler_define.h"

enum {
	SWITCH_STATUS_CANCEL = -3,
	SWITCH_STATUS_DOING = -2,
	SWITCH_STATUS_FINISH = -1,
	SWITCH_STATUS_START = 0
};

typedef struct SwitchCo_t {
	int id; /* unique id, user read only */
	int status; /* switch status, user read only */
	void* ctx; /* hold routine execute context */
	void(*fn_ctx_free)(void*); /* free execute context */
	void* resume_ret; /* resume result, user read only */
	struct SwitchCo_t* await_co; /* cache current async wait sub co, user read only */
} SwitchCo_t;

struct SwitchCoSche_t;

#define	SwitchCo_code_begin(co) \
switch (co->status) { \
	case SWITCH_STATUS_START: \
		co->status = SWITCH_STATUS_DOING; \
		do

#define	SwitchCo_code_end(co) \
		while (0); \
	default: \
		if (SWITCH_STATUS_DOING == co->status) { \
			co->status = SWITCH_STATUS_FINISH; \
		} \
		if (co->fn_ctx_free) { \
			co->fn_ctx_free(co->ctx); \
			co->fn_ctx_free = NULL; \
		} \
		SwitchCoSche_cancel_child_co(sche, co); \
}

#define SwitchCo_yield(co)	do { (co)->status = __COUNTER__ + 1; return; case __COUNTER__: (co)->status = SWITCH_STATUS_DOING; } while(0)

#define	SwitchCo_await_co(sche, co, child_co)	\
do {	\
	if (SwitchCoSche_call_co(sche, child_co)->status < 0)	\
		break;	\
	SwitchCo_yield(co);	\
} while (1)

#define	SwitchCo_await(sche, co, func_expr)	\
do {	\
	co->await_co = SwitchCoSche_new_child_co(sche, co);	\
	do {	\
		func_expr;	\
		if (co->await_co->status < 0)	\
			break;	\
		SwitchCo_yield(co);	\
	} while (1);	\
	SwitchCoSche_reuse_co(co->await_co);	\
} while (0)

#ifdef __cplusplus
extern "C" {
#endif

__declspec_dll struct SwitchCoSche_t* SwitchCoSche_new(void* userdata);
__declspec_dll void SwitchCoSche_destroy(struct SwitchCoSche_t* sche);
__declspec_dll int SwitchCoSche_sche(struct SwitchCoSche_t* sche, int idle_msec);
__declspec_dll void SwitchCoSche_wake_up(struct SwitchCoSche_t* sche);
__declspec_dll void SwitchCoSche_exit(struct SwitchCoSche_t* sche);
__declspec_dll void* SwitchCoSche_userdata(struct SwitchCoSche_t* sche);
__declspec_dll void SwitchCoSche_at_exit(struct SwitchCoSche_t* sche, void(*fn_at_exit)(struct SwitchCoSche_t*, void*), void* arg, void(*fn_arg_free)(void*));

__declspec_dll SwitchCo_t* SwitchCoSche_root_function(struct SwitchCoSche_t* sche, void(*proc)(struct SwitchCoSche_t*, SwitchCo_t*, void*), void* arg, void(*fn_arg_free)(void*));
__declspec_dll SwitchCo_t* SwitchCoSche_timeout_util(struct SwitchCoSche_t* sche, long long tm_msec, void(*proc)(struct SwitchCoSche_t*, SwitchCo_t*, void*), void* arg, void(*fn_arg_free)(void*));
__declspec_dll SwitchCo_t* SwitchCoSche_new_child_co(struct SwitchCoSche_t* sche, SwitchCo_t* parent_co);
__declspec_dll SwitchCo_t* SwitchCoSche_sleep_util(struct SwitchCoSche_t* sche, SwitchCo_t* parent_co, long long tm_msec);
__declspec_dll SwitchCo_t* SwitchCoSche_block_point_util(struct SwitchCoSche_t* sche, SwitchCo_t* parent_co, long long tm_msec);

__declspec_dll void* SwitchCoSche_pop_resume_ret(SwitchCo_t* co);
__declspec_dll void SwitchCoSche_reuse_co(SwitchCo_t* co);
__declspec_dll SwitchCo_t* SwitchCoSche_call_co(struct SwitchCoSche_t* sche, SwitchCo_t* co);
__declspec_dll void SwitchCoSche_resume_co(struct SwitchCoSche_t* sche, int co_id, void* ret, void(*fn_ret_free)(void*));
__declspec_dll void SwitchCoSche_cancel_co(struct SwitchCoSche_t* sche, SwitchCo_t* co);
__declspec_dll void SwitchCoSche_cancel_child_co(struct SwitchCoSche_t* sche, SwitchCo_t* co);

#ifdef __cplusplus
}
#endif

#endif
