//
// Created by hujianzhe on 2022-10-30
//

#ifndef	UTIL_C_COMPONENT_SWITCH_CO_SCHE_H
#define	UTIL_C_COMPONENT_SWITCH_CO_SCHE_H

#include "../compiler_define.h"

enum {
	SWITCH_STATUS_CANCEL = -3,
	SWITCH_STATUS_ERROR = -2,
	SWITCH_STATUS_FINISH = -1,
	SWITCH_STATUS_START = 0
};

typedef struct SwitchCo_t {
	int id; /* unique id, user read only */
	int status; /* switch status, user read only */
	void* ctx; /* hold routine execute context */
	void* arg; /* routine arguments */
	void* ret; /* routine result */
} SwitchCo_t;

struct SwitchCoSche_t;

#define SwitchCo_yield(co)	do { (co)->status = __COUNTER__ + 1; return; case __COUNTER__: ; } while(0)

#define	SwitchCo_await(sche, co, child_co)	\
do {	\
	if (SwitchCoSche_call_co(sche, child_co)->status < 0)	\
		break;	\
	SwitchCo_yield(co);	\
} while (1)

#ifdef __cplusplus
extern "C" {
#endif

__declspec_dll struct SwitchCoSche_t* SwitchCoSche_new(void);
__declspec_dll void SwitchCoSche_destroy(struct SwitchCoSche_t* sche);
__declspec_dll int SwitchCoSche_sche(struct SwitchCoSche_t* sche, int idle_msec);
__declspec_dll void SwitchCoSche_wake_up(struct SwitchCoSche_t* sche);
__declspec_dll void SwitchCoSche_exit(struct SwitchCoSche_t* sche);

__declspec_dll SwitchCo_t* SwitchCoSche_timeout_msec(struct SwitchCoSche_t* sche, long long msec, void(*proc)(struct SwitchCoSche_t*, SwitchCo_t*), void* arg);
__declspec_dll SwitchCo_t* SwitchCoSche_root_function(struct SwitchCoSche_t* sche, void(*proc)(struct SwitchCoSche_t*, SwitchCo_t*), void* arg);
__declspec_dll SwitchCo_t* SwitchCoSche_new_child_co(SwitchCo_t* parent_co, void(*proc)(struct SwitchCoSche_t*, SwitchCo_t*));
__declspec_dll SwitchCo_t* SwitchCoSche_sleep_msec(struct SwitchCoSche_t* sche, SwitchCo_t* parent_co, long long msec);
__declspec_dll SwitchCo_t* SwitchCoSche_block_point(struct SwitchCoSche_t* sche, SwitchCo_t* parent_co, long long block_msec);

__declspec_dll void SwitchCoSche_reuse_co(SwitchCo_t* co);
__declspec_dll SwitchCo_t* SwitchCoSche_call_co(struct SwitchCoSche_t* sche, SwitchCo_t* co);
__declspec_dll void SwitchCoSche_resume_co(struct SwitchCoSche_t* sche, int co_id, void* ret);
__declspec_dll void SwitchCoSche_cancel_co(struct SwitchCoSche_t* sche, SwitchCo_t* co);
__declspec_dll void SwitchCoSche_cancel_child_co(struct SwitchCoSche_t* sche, SwitchCo_t* co);

#ifdef __cplusplus
}
#endif

#endif
