//
// Created by hujianzhe on 2022-10-30
//

#ifndef	UTIL_C_COMPONENT_STACK_CO_SCHE_H
#define	UTIL_C_COMPONENT_STACK_CO_SCHE_H

enum {
	STACK_CO_STATUS_CANCEL = -3,
	STACK_CO_STATUS_ERROR = -2,
	STACK_CO_STATUS_FINISH = -1,
	STACK_CO_STATUS_START = 0
};

typedef struct StackCo_t {
	int id; /* unique id, user read only */
	int status; /* switch status, user read only */
	void* resume_ret; /* resume result */
} StackCo_t;

struct StackCoSche_t;

#ifdef __cplusplus
extern "C" {
#endif

__declspec_dll struct StackCoSche_t* StackCoSche_new(size_t stack_size, void* userdata);
__declspec_dll void StackCoSche_destroy(struct StackCoSche_t* sche);
__declspec_dll int StackCoSche_sche(struct StackCoSche_t* sche, int idle_msec);
__declspec_dll void StackCoSche_wake_up(struct StackCoSche_t* sche);
__declspec_dll void StackCoSche_exit(struct StackCoSche_t* sche);
__declspec_dll void* StackCoSche_userdata(struct StackCoSche_t* sche);
__declspec_dll void StackCoSche_at_exit(struct StackCoSche_t* sche, void(*fn_at_exit)(struct StackCoSche_t*, void*), void* arg, void(*fn_arg_free)(void*));

__declspec_dll StackCo_t* StackCoSche_function(struct StackCoSche_t* sche, void(*proc)(struct StackCoSche_t*, void*), void* arg, void(*fn_arg_free)(void*));
__declspec_dll StackCo_t* StackCoSche_timeout_util(struct StackCoSche_t* sche, long long tm_msec, void(*proc)(struct StackCoSche_t*, void*), void* arg, void(*fn_arg_free)(void*));
__declspec_dll StackCo_t* StackCoSche_block_point_util(struct StackCoSche_t* sche, long long tm_msec);
__declspec_dll StackCo_t* StackCoSche_sleep_util(struct StackCoSche_t* sche, long long msec);

__declspec_dll StackCo_t* StackCoSche_yield(struct StackCoSche_t* sche);
__declspec_dll void* StackCoSche_pop_resume_ret(StackCo_t* co);
__declspec_dll void StackCoSche_reuse_co(StackCo_t* co);
__declspec_dll void StackCoSche_resume_co(struct StackCoSche_t* sche, int co_id, void* ret, void(*fn_ret_free)(void*));
__declspec_dll void StackCoSche_cancel_co(struct StackCoSche_t* sche, StackCo_t* co);

#ifdef __cplusplus
}
#endif

#endif
