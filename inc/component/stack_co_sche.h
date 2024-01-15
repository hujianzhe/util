//
// Created by hujianzhe on 2022-10-30
//

#ifndef	UTIL_C_COMPONENT_STACK_CO_SCHE_H
#define	UTIL_C_COMPONENT_STACK_CO_SCHE_H

#include "../compiler_define.h"
#include "../datastruct/list.h"

enum {
	STACK_CO_STATUS_CANCEL = -3,
	STACK_CO_STATUS_ERROR = -2,
	STACK_CO_STATUS_FINISH = -1,
	STACK_CO_STATUS_START = 0
};

typedef struct StackCoAsyncParam_t {
	void* value;
	void(*fn_value_free)(void*);
} StackCoAsyncParam_t;

typedef struct StackCoBlock_t {
	int id; /* unique id, user read only */
	int status; /* switch status, user read only */
	StackCoAsyncParam_t resume_param; /* resume param */
} StackCoBlock_t;

typedef struct StackCoBlockGroup_t {
/* user not visit these fields */
	List_t wait_block_list;
	List_t ready_block_list;
} StackCoBlockGroup_t;

struct StackCoSche_t;
struct StackCoLock_t;
struct StackCoLockOwner_t;

#ifdef __cplusplus
extern "C" {
#endif

__declspec_dll struct StackCoSche_t* StackCoSche_new(size_t stack_size, void* userdata);
__declspec_dll void StackCoSche_destroy(struct StackCoSche_t* sche);
__declspec_dll int StackCoSche_sche(struct StackCoSche_t* sche, int idle_msec);
__declspec_dll void StackCoSche_wake_up(struct StackCoSche_t* sche);
__declspec_dll void StackCoSche_exit(struct StackCoSche_t* sche);
__declspec_dll void* StackCoSche_userdata(struct StackCoSche_t* sche);
__declspec_dll void StackCoSche_set_handle_cnt(struct StackCoSche_t* sche, int handle_cnt);
__declspec_dll int StackCoSche_has_exit(struct StackCoSche_t* sche);
__declspec_dll void StackCoSche_at_exit(struct StackCoSche_t* sche, void(*fn_at_exit)(struct StackCoSche_t*, void*), void* arg, void(*fn_arg_free)(void*));

__declspec_dll int StackCoSche_function(struct StackCoSche_t* sche, void(*proc)(struct StackCoSche_t*, StackCoAsyncParam_t*), const StackCoAsyncParam_t* param);
__declspec_dll int StackCoSche_timeout_util(struct StackCoSche_t* sche, long long tm_msec, void(*proc)(struct StackCoSche_t*, StackCoAsyncParam_t*), const StackCoAsyncParam_t* param);

__declspec_dll StackCoBlock_t* StackCoSche_block_point_util(struct StackCoSche_t* sche, long long tm_msec, StackCoBlockGroup_t* group);
__declspec_dll StackCoBlock_t* StackCoSche_sleep_util(struct StackCoSche_t* sche, long long tm_msec, StackCoBlockGroup_t* group);
__declspec_dll void StackCoSche_resume_block_by_id(struct StackCoSche_t* sche, int block_id, int status, const StackCoAsyncParam_t* param);

__declspec_dll struct StackCoLockOwner_t* StackCoSche_new_lock_owner(const char* s, size_t slen);
__declspec_dll struct StackCoLockOwner_t* StackCoSche_clone_lock_owner(const struct StackCoLockOwner_t* owner);
__declspec_dll void StackCoSche_free_lock_owner(struct StackCoLockOwner_t* owner);
__declspec_dll struct StackCoLock_t* StackCoSche_lock(struct StackCoSche_t* sche, const struct StackCoLockOwner_t* owner, const char* name);
__declspec_dll struct StackCoLock_t* StackCoSche_try_lock(struct StackCoSche_t* sche, const struct StackCoLockOwner_t* owner, const char* name);
__declspec_dll void StackCoSche_unlock(struct StackCoSche_t* sche, struct StackCoLock_t* lock);

__declspec_dll StackCoBlock_t* StackCoSche_yield(struct StackCoSche_t* sche);
__declspec_dll int StackCoSche_group_is_empty(StackCoBlockGroup_t* group);
__declspec_dll StackCoBlock_t* StackCoSche_yield_group(struct StackCoSche_t* sche, StackCoBlockGroup_t* group);

__declspec_dll void StackCoSche_cleanup_async_param(StackCoAsyncParam_t* ap);

__declspec_dll void StackCoSche_reuse_block(struct StackCoSche_t* sche, StackCoBlock_t* block);
__declspec_dll void StackCoSche_reuse_block_group(struct StackCoSche_t* sche, StackCoBlockGroup_t* group);

#ifdef __cplusplus
}
#endif

#endif
