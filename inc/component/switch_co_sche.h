//
// Created by hujianzhe on 2022-10-30
//

#ifndef	UTIL_C_COMPONENT_SWITCH_CO_SCHE_H
#define	UTIL_C_COMPONENT_SWITCH_CO_SCHE_H

#include "../datastruct/switch_yield.h"

struct SwitchCoSche_t;

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

__declspec_dll void SwitchCoSche_resume_co(struct SwitchCoSche_t* sche, int co_id, void* ret);
__declspec_dll void SwitchCoSche_cancel_co(struct SwitchCoSche_t* sche, SwitchCo_t* co);
__declspec_dll void SwitchCoSche_cancel_child_co(struct SwitchCoSche_t* sche, SwitchCo_t* co);

#ifdef __cplusplus
}
#endif

#endif