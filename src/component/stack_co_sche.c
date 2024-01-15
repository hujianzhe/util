//
// Created by hujianzhe on 2022-10-30
//

#include "../../inc/sysapi/atomic.h"
#include "../../inc/sysapi/process.h"
#include "../../inc/sysapi/time.h"
#include "../../inc/component/dataqueue.h"
#include "../../inc/component/rbtimer.h"
#include "../../inc/component/stack_co_sche.h"
#include "../../inc/datastruct/list.h"
#include "../../inc/datastruct/hashtable.h"
#include <limits.h>
#include <stdlib.h>
#include <string.h>

enum {
	STACK_CO_HDR_PROC = 1,
	STACK_CO_HDR_RESUME = 2,
};

typedef struct StackCoHdr_t {
	ListNode_t listnode;
	int type;
} StackCoHdr_t;

typedef struct StackCoNode_t {
	StackCoHdr_t hdr;
	int status;
	struct Fiber_t* fiber;
	void(*proc)(struct StackCoSche_t*, StackCoAsyncParam_t*);
	StackCoAsyncParam_t proc_param;
	List_t block_list;
	List_t reuse_block_list;
	RBTimerEvent_t* timeout_event;
	StackCoBlockGroup_t* wait_group;
} StackCoNode_t;

typedef struct StackCoLockOwner_t {
	int unique;
	size_t slen;
	char str[1];
} StackCoLockOwner_t;

typedef struct StackCoLock_t {
	HashtableNode_t htnode;
	List_t wait_block_list;
	StackCoLockOwner_t* owner;
	size_t enter_times;
} StackCoLock_t;

typedef struct StackCoBlockNode_t {
	StackCoBlock_t block;
	StackCoNode_t* exec_co_node;
	ListNode_t listnode;
	HashtableNode_t htnode;
	RBTimerEvent_t* timeout_event;
	ListNode_t ready_resume_listnode;
	int ready_resume_flag;
	StackCoLockOwner_t* lock_owner;
	StackCoLock_t* wait_lock;
	StackCoBlockGroup_t* wait_group;
} StackCoBlockNode_t;

typedef struct StackCoResume_t {
	StackCoHdr_t hdr;
	int block_id;
	int status;
	StackCoAsyncParam_t param;
} StackCoResume_t;

typedef struct StackCoSche_t {
	volatile int exit_flag;
	int handle_cnt;
	void* userdata;
	void(*fn_at_exit)(struct StackCoSche_t*, void*);
	void* fn_at_exit_arg;
	void(*fn_at_exit_arg_free)(void*);

	DataQueue_t dq;
	RBTimer_t timer;
	struct Fiber_t* proc_fiber;
	struct Fiber_t* cur_fiber;
	struct Fiber_t* sche_fiber;
	int stack_size;
	StackCoNode_t* exec_co_node;
	StackCoBlockNode_t* resume_block_node;
	List_t exec_co_list;
	List_t ready_resume_block_list;
	Hashtable_t block_co_htbl;
	HashtableNode_t* block_co_htbl_bulks[2048];
	Hashtable_t lock_htbl;
	HashtableNode_t* lock_htbl_bulks[2048];
} StackCoSche_t;

#ifdef __cplusplus
extern "C" {
#endif

static int gen_co_id() {
	static Atom32_t s_id = 0;
	int id;
	while (0 == (id = _xadd32(&s_id, 1) + 1));
	return id;
}

static StackCoNode_t* alloc_stack_co_node(void) {
	StackCoNode_t* co_node = (StackCoNode_t*)calloc(1, sizeof(StackCoNode_t));
	if (!co_node) {
		return NULL;
	}
	listInit(&co_node->block_list);
	listInit(&co_node->reuse_block_list);
	return co_node;
}

static void reset_block_data(StackCoBlock_t* block) {
	block->id = 0;
	block->status = STACK_CO_STATUS_START;
	memset(&block->resume_param, 0, sizeof(block->resume_param));
}

static StackCoBlockNode_t* alloc_block_node(void) {
	StackCoBlockNode_t* block_node = (StackCoBlockNode_t*)calloc(1, sizeof(StackCoBlockNode_t));
	if (block_node) {
		reset_block_data(&block_node->block);
	}
	return block_node;
}

static void free_stack_co_node(StackCoSche_t* sche, StackCoNode_t* co_node) {
	if (co_node->timeout_event) {
		rbtimerDetachEvent(co_node->timeout_event);
		free(co_node->timeout_event);
	}
	StackCoSche_cleanup_async_param(&co_node->proc_param);
	free(co_node);
}

static void free_block_node(StackCoSche_t* sche, StackCoBlockNode_t* block_node) {
	if (block_node->timeout_event) {
		rbtimerDetachEvent(block_node->timeout_event);
		free(block_node->timeout_event);
	}
	if (block_node->htnode.key.i32) {
		hashtableRemoveNode(&sche->block_co_htbl, &block_node->htnode);
	}
	StackCoSche_cleanup_async_param(&block_node->block.resume_param);

	if (block_node->ready_resume_flag) {
		listRemoveNode(&sche->ready_resume_block_list, &block_node->ready_resume_listnode);
	}
	else if (block_node->wait_lock) {
		listRemoveNode(&block_node->wait_lock->wait_block_list, &block_node->ready_resume_listnode);
		if (block_node->lock_owner) {
			StackCoSche_free_lock_owner(block_node->lock_owner);
		}
	}
	else if (block_node->wait_group) {
		if (block_node->block.status != STACK_CO_STATUS_START) {
			listRemoveNode(&block_node->wait_group->ready_block_list, &block_node->ready_resume_listnode);
		}
		else {
			listRemoveNode(&block_node->wait_group->wait_block_list, &block_node->ready_resume_listnode);
		}
	}

	free(block_node);
}

static void free_co_block_nodes(StackCoSche_t* sche, StackCoNode_t* co_node) {
	ListNode_t* lcur, *lnext;
	for (lcur = co_node->block_list.head; lcur; lcur = lnext) {
		StackCoBlockNode_t* block_node = pod_container_of(lcur, StackCoBlockNode_t, listnode);
		lnext = lcur->next;
		free_block_node(sche, block_node);
	}
	for (lcur = co_node->reuse_block_list.head; lcur; lcur = lnext) {
		StackCoBlockNode_t* co_node = pod_container_of(lcur, StackCoBlockNode_t, listnode);
		lnext = lcur->next;
		free_block_node(sche, co_node);
	}
}

static void FiberProcEntry(struct Fiber_t* fiber, void* arg) {
	StackCoSche_t* sche = (StackCoSche_t*)arg;
	while (1) {
		StackCoNode_t* exec_co_node = sche->exec_co_node;
		exec_co_node->proc(sche, &exec_co_node->proc_param);
		exec_co_node->status = STACK_CO_STATUS_FINISH;
		sche->cur_fiber = sche->sche_fiber;
		fiberSwitch(fiber, sche->sche_fiber, 1);
	}
}

static void stack_co_switch(StackCoSche_t* sche, StackCoNode_t* dst_co_node, StackCoBlockNode_t* dst_block_node) {
	struct Fiber_t* cur_fiber, *dst_fiber;

	if (dst_block_node) {
		StackCoBlockGroup_t* dst_group = dst_block_node->wait_group;
		if (dst_group) {
			listRemoveNode(&dst_group->wait_block_list, &dst_block_node->ready_resume_listnode);
		}
		if (dst_co_node->wait_group != dst_group) {
			if (dst_group) {
				listPushNodeBack(&dst_group->ready_block_list, &dst_block_node->ready_resume_listnode);
			}
			return;
		}
		dst_block_node->wait_group = NULL;
		dst_co_node->wait_group = NULL;
	}
	cur_fiber = sche->cur_fiber;
	dst_fiber = dst_co_node->fiber;

	sche->exec_co_node = dst_co_node;
	sche->resume_block_node = dst_block_node;
	sche->cur_fiber = dst_fiber;
	fiberSwitch(cur_fiber, dst_fiber, 0);
	if (dst_co_node->status >= 0) {
		return;
	}
	if (dst_fiber != sche->proc_fiber) {
		fiberFree(dst_fiber);
	}
	dst_co_node->fiber = NULL;
	listRemoveNode(&sche->exec_co_list, &dst_co_node->hdr.listnode);
	free_co_block_nodes(sche, dst_co_node);
	free_stack_co_node(sche, dst_co_node);
	sche->exec_co_node = NULL;
}

static void timer_sleep_callback(RBTimer_t* timer, struct RBTimerEvent_t* e) {
	StackCoSche_t* sche = pod_container_of(timer, StackCoSche_t, timer);
	StackCoBlockNode_t* block_node = (StackCoBlockNode_t*)e->arg;
	block_node->block.status = STACK_CO_STATUS_FINISH;
	stack_co_switch(sche, block_node->exec_co_node, block_node);
}

static void timer_block_callback(RBTimer_t* timer, struct RBTimerEvent_t* e) {
	StackCoSche_t* sche = pod_container_of(timer, StackCoSche_t, timer);
	StackCoBlockNode_t* block_node = (StackCoBlockNode_t*)e->arg;
	hashtableRemoveNode(&sche->block_co_htbl, &block_node->htnode);
	block_node->htnode.key.i32 = 0;
	block_node->block.status = STACK_CO_STATUS_CANCEL;
	stack_co_switch(sche, block_node->exec_co_node, block_node);
}

static void timer_timeout_callback(RBTimer_t* timer, struct RBTimerEvent_t* e) {
	StackCoSche_t* sche = pod_container_of(timer, StackCoSche_t, timer);
	StackCoNode_t* co_node = (StackCoNode_t*)e->arg;

	co_node->fiber = sche->proc_fiber;
	co_node->timeout_event = NULL;
	free(e);
	listPushNodeBack(&sche->exec_co_list, &co_node->hdr.listnode);
	stack_co_switch(sche, co_node, NULL);
}

static int sche_update_proc_fiber(StackCoSche_t* sche) {
	if (sche->cur_fiber == sche->proc_fiber) {
		struct Fiber_t* proc_fiber = fiberCreate(sche->cur_fiber, sche->stack_size, FiberProcEntry, sche);
		if (!proc_fiber) {
			return 0;
		}
		sche->proc_fiber = proc_fiber;
	}
	return 1;
}

static long long sche_calculate_min_wait_timelen(StackCoSche_t* sche, int idle_msec) {
	long long t, cur_msec;

	if (!listIsEmpty(&sche->ready_resume_block_list)) {
		return 0;
	}
	t = rbtimerMiniumTimestamp(&sche->timer);
	if (t < 0) {
		return idle_msec;
	}
	cur_msec = gmtimeMillisecond();
	if (t <= cur_msec) {
		return 0;
	}
	t -= cur_msec;
	if (idle_msec < 0) {
		return t;
	}
	if (t > idle_msec) {
		return idle_msec;
	}
	return t;
}

static StackCoLock_t* get_stack_co_lock(StackCoSche_t* sche, const char* name) {
	HashtableNode_t* htnode;
	HashtableNodeKey_t key;

	key.ptr = name;
	htnode = hashtableSearchKey(&sche->lock_htbl, key);
	return htnode ? pod_container_of(htnode, StackCoLock_t, htnode) : NULL;
}

static StackCoLock_t* new_stack_co_lock(StackCoSche_t* sche, const char* name) {
	StackCoLock_t* co_lock = (StackCoLock_t*)calloc(1, sizeof(StackCoLock_t));
	if (!co_lock) {
		return NULL;
	}
	co_lock->htnode.key.ptr = strdup(name);
	hashtableInsertNode(&sche->lock_htbl, &co_lock->htnode);
	listInit(&co_lock->wait_block_list);
	return co_lock;
}

static int lock_owner_check_equal(const StackCoLockOwner_t* a, const StackCoLockOwner_t* b) {
	if (a == b) {
		return 1;
	}
	if (a->unique || b->unique) {
		return 0;
	}
	if (a->slen != b->slen) {
		return 0;
	}
	return 0 == memcmp(a->str, b->str, b->slen);
}

static StackCoLock_t* sche_lock(StackCoSche_t* sche, const StackCoLockOwner_t* owner, StackCoLock_t* lock) {
	StackCoNode_t* exec_co_node;
	StackCoBlockNode_t* block_node;
	StackCoLockOwner_t* clone_owner;
	int block_node_alloc = 0;

	if (!lock->owner) {
		lock->owner = StackCoSche_clone_lock_owner(owner);
		if (!lock->owner) {
			return NULL;
		}
		lock->enter_times = 1;
		return lock;
	}
	if (lock_owner_check_equal(lock->owner, owner)) {
		lock->enter_times++;
		return lock;
	}

	clone_owner = StackCoSche_clone_lock_owner(owner);
	if (!clone_owner) {
		return NULL;
	}
	exec_co_node = sche->exec_co_node;
	if (listIsEmpty(&exec_co_node->reuse_block_list)) {
		block_node = alloc_block_node();
		if (!block_node) {
			goto err;
		}
		block_node_alloc = 1;
	}
	else {
		ListNode_t* listnode = listPopNodeBack(&exec_co_node->reuse_block_list);
		block_node = pod_container_of(listnode, StackCoBlockNode_t, listnode);
		reset_block_data(&block_node->block);
	}
	if (!sche_update_proc_fiber(sche)) {
		goto err;
	}

	block_node->exec_co_node = exec_co_node;
	block_node->lock_owner = clone_owner;
	block_node->wait_lock = lock;
	listPushNodeBack(&exec_co_node->block_list, &block_node->listnode);
	listPushNodeBack(&lock->wait_block_list, &block_node->ready_resume_listnode);

	StackCoSche_yield(sche);
	return lock;
err:
	if (clone_owner) {
		StackCoSche_free_lock_owner(clone_owner);
	}
	if (block_node_alloc) {
		free(block_node);
	}
	else if (block_node) {
		listPushNodeBack(&exec_co_node->reuse_block_list, &block_node->listnode);
	}
	return NULL;
}

static void free_stack_co_lock(StackCoLock_t* co_lock) {
	StackCoSche_free_lock_owner(co_lock->owner);
	free((void*)co_lock->htnode.key.ptr);
	free(co_lock);
}

StackCoSche_t* StackCoSche_new(size_t stack_size, void* userdata) {
	int dq_ok = 0, timer_ok = 0;
	StackCoSche_t* sche = (StackCoSche_t*)malloc(sizeof(StackCoSche_t));
	if (!sche) {
		return NULL;
	}
	if (!dataqueueInit(&sche->dq)) {
		goto err;
	}
	dq_ok = 1;
	if (!rbtimerInit(&sche->timer)) {
		goto err;
	}
	timer_ok = 1;
	sche->exit_flag = 0;
	sche->handle_cnt = 100;
	sche->userdata = userdata;
	sche->fn_at_exit = NULL;
	sche->fn_at_exit_arg = NULL;
	sche->fn_at_exit_arg_free = NULL;
	sche->proc_fiber = NULL;
	sche->cur_fiber = NULL;
	sche->sche_fiber = NULL;
	sche->stack_size = stack_size;
	sche->exec_co_node = NULL;
	sche->resume_block_node = NULL;
	listInit(&sche->exec_co_list);
	listInit(&sche->ready_resume_block_list);
	hashtableInit(&sche->block_co_htbl,
			sche->block_co_htbl_bulks, sizeof(sche->block_co_htbl_bulks) / sizeof(sche->block_co_htbl_bulks[0]),
			hashtableDefaultKeyCmp32, hashtableDefaultKeyHash32);
	hashtableInit(&sche->lock_htbl,
			sche->lock_htbl_bulks, sizeof(sche->lock_htbl_bulks) / sizeof(sche->lock_htbl_bulks[0]),
			hashtableDefaultKeyCmpStr, hashtableDefaultKeyHashStr);
	return sche;
err:
	if (dq_ok) {
		dataqueueDestroy(&sche->dq);
	}
	if (timer_ok) {
		rbtimerDestroy(&sche->timer);
	}
	free(sche);
	return NULL;
}

void StackCoSche_destroy(StackCoSche_t* sche) {
	ListNode_t *lcur, *lnext;

	for (lcur = dataqueuePopWait(&sche->dq, 0, -1); lcur; lcur = lnext) {
		StackCoHdr_t* hdr = pod_container_of(lcur, StackCoHdr_t, listnode);
		lnext = lcur->next;

		if (STACK_CO_HDR_PROC == hdr->type) {
			StackCoNode_t* co_node = pod_container_of(hdr, StackCoNode_t, hdr);
			free_stack_co_node(sche, co_node);
		}
		else if (STACK_CO_HDR_RESUME == hdr->type) {
			StackCoResume_t* e = pod_container_of(hdr, StackCoResume_t, hdr);
			StackCoSche_cleanup_async_param(&e->param);
			free(e);
		}
		else {
			free(hdr);
		}
	}
	while (1) {
		StackCoNode_t* co_node;
		RBTimerEvent_t* e = rbtimerTimeoutPopup(&sche->timer, LLONG_MAX);
		if (!e) {
			break;
		}
		co_node = (StackCoNode_t*)e->arg;
		free_stack_co_node(sche, co_node);
	}
	rbtimerDestroy(&sche->timer);
	dataqueueDestroy(&sche->dq);
	free(sche);
}

int StackCoSche_function(StackCoSche_t* sche, void(*proc)(StackCoSche_t*, StackCoAsyncParam_t*), const StackCoAsyncParam_t* param) {
	StackCoNode_t* co_node = alloc_stack_co_node();
	if (!co_node) {
		return 0;
	}
	co_node->hdr.type = STACK_CO_HDR_PROC;
	co_node->proc = proc;
	if (param) {
		co_node->proc_param = *param;
	}
	dataqueuePush(&sche->dq, &co_node->hdr.listnode);
	return 1;
}

int StackCoSche_timeout_util(StackCoSche_t* sche, long long tm_msec, void(*proc)(StackCoSche_t*, StackCoAsyncParam_t*), const StackCoAsyncParam_t* param) {
	RBTimerEvent_t* e;
	StackCoNode_t* co_node;

	if (tm_msec < 0) {
		return 0;
	}
	e = (RBTimerEvent_t*)calloc(1, sizeof(RBTimerEvent_t));
	if (!e) {
		return 0;
	}
	co_node = alloc_stack_co_node();
	if (!co_node) {
		free(e);
		return 0;
	}
	co_node->hdr.type = STACK_CO_HDR_PROC;
	co_node->proc = proc;
	if (param) {
		co_node->proc_param = *param;
	}
	co_node->timeout_event = e;

	e->timestamp = tm_msec;
	e->callback = timer_timeout_callback;
	e->arg = co_node;

	dataqueuePush(&sche->dq, &co_node->hdr.listnode);
	return 1;
}

StackCoBlock_t* StackCoSche_block_point_util(StackCoSche_t* sche, long long tm_msec, StackCoBlockGroup_t* group) {
	RBTimerEvent_t* e = NULL;
	StackCoNode_t* exec_co_node;
	StackCoBlockNode_t* block_node = NULL;
	int block_node_alloc = 0, timeout_event_alloc = 0;

	if (sche->cur_fiber == sche->sche_fiber) {
		return NULL;
	}

	exec_co_node = sche->exec_co_node;
	if (listIsEmpty(&exec_co_node->reuse_block_list)) {
		block_node = alloc_block_node();
		if (!block_node) {
			goto err;
		}
		block_node_alloc = 1;
	}
	else {
		ListNode_t* listnode = listPopNodeBack(&exec_co_node->reuse_block_list);
		block_node = pod_container_of(listnode, StackCoBlockNode_t, listnode);
		reset_block_data(&block_node->block);
	}
	if (tm_msec >= 0) {
		if (block_node->timeout_event) {
			e = block_node->timeout_event;
		}
		else {
			e = (RBTimerEvent_t*)calloc(1, sizeof(RBTimerEvent_t));
			if (!e) {
				goto err;
			}
			timeout_event_alloc = 1;
			block_node->timeout_event = e;
		}
	}
	block_node->block.id = gen_co_id();
	block_node->htnode.key.i32 = block_node->block.id;
	if (hashtableInsertNode(&sche->block_co_htbl, &block_node->htnode) != &block_node->htnode) {
		goto err;
	}
	if (!sche_update_proc_fiber(sche)) {
		hashtableRemoveNode(&sche->block_co_htbl, &block_node->htnode);
		goto err;
	}
	block_node->exec_co_node = exec_co_node;

	if (e) {
		e->timestamp = tm_msec;
		e->callback = timer_block_callback;
		e->arg = block_node;
		rbtimerAddEvent(&sche->timer, e);
	}

	if (group) {
		listPushNodeBack(&group->wait_block_list, &block_node->ready_resume_listnode);
	}
	block_node->wait_group = group;

	listPushNodeBack(&exec_co_node->block_list, &block_node->listnode);
	return &block_node->block;
err:
	if (block_node_alloc) {
		free(block_node);
	}
	else if (block_node) {
		listPushNodeBack(&exec_co_node->reuse_block_list, &block_node->listnode);
	}
	if (timeout_event_alloc) {
		free(e);
	}
	return NULL;
}

StackCoBlock_t* StackCoSche_sleep_util(StackCoSche_t* sche, long long tm_msec, StackCoBlockGroup_t* group) {
	RBTimerEvent_t* e = NULL;
	StackCoNode_t* exec_co_node;
	StackCoBlockNode_t* block_node = NULL;
	int block_node_alloc = 0, timeout_event_alloc = 0;

	if (sche->cur_fiber == sche->sche_fiber) {
		return NULL;
	}

	exec_co_node = sche->exec_co_node;
	if (listIsEmpty(&exec_co_node->reuse_block_list)) {
		block_node = alloc_block_node();
		if (!block_node) {
			goto err;
		}
		block_node_alloc = 1;
	}
	else {
		ListNode_t* listnode = listPopNodeBack(&exec_co_node->reuse_block_list);
		block_node = pod_container_of(listnode, StackCoBlockNode_t, listnode);
		reset_block_data(&block_node->block);
	}
	if (tm_msec >= 0) {
		if (block_node->timeout_event) {
			e = block_node->timeout_event;
		}
		else {
			e = (RBTimerEvent_t*)calloc(1, sizeof(RBTimerEvent_t));
			if (!e) {
				goto err;
			}
			timeout_event_alloc = 1;
			block_node->timeout_event = e;
		}
	}
	if (!sche_update_proc_fiber(sche)) {
		goto err;
	}
	block_node->exec_co_node = exec_co_node;

	if (e) {
		e->timestamp = tm_msec;
		e->callback = timer_sleep_callback;
		e->arg = block_node;
		rbtimerAddEvent(&sche->timer, e);
	}

	if (group) {
		listPushNodeBack(&group->wait_block_list, &block_node->ready_resume_listnode);
	}
	block_node->wait_group = group;

	listPushNodeBack(&exec_co_node->block_list, &block_node->listnode);
	return &block_node->block;
err:
	if (block_node_alloc) {
		free(block_node);
	}
	else if (block_node) {
		listPushNodeBack(&exec_co_node->reuse_block_list, &block_node->listnode);
	}
	if (timeout_event_alloc) {
		free(e);
	}
	return NULL;
}

StackCoLockOwner_t* StackCoSche_new_lock_owner(const char* s, size_t slen) {
	StackCoLockOwner_t* owner;
	if (!s) {
		slen = 0;
	}
	owner = (StackCoLockOwner_t*)malloc(sizeof(StackCoLockOwner_t) + slen);
	if (!owner) {
		return NULL;
	}
	if (s && slen) {
		memcpy(owner->str, s, slen);
		owner->unique = 0;
	}
	else {
		owner->unique = 1;
	}
	owner->slen = slen;
	owner->str[slen] = 0;
	return owner;
}

StackCoLockOwner_t* StackCoSche_clone_lock_owner(const StackCoLockOwner_t* owner) {
	return StackCoSche_new_lock_owner(owner->str, owner->slen);
}

void StackCoSche_free_lock_owner(StackCoLockOwner_t* owner) { free(owner); }

StackCoLock_t* StackCoSche_lock(StackCoSche_t* sche, const StackCoLockOwner_t* owner, const char* name) {
	StackCoLock_t* lock = NULL;
	int co_lock_alloc = 0;

	lock = get_stack_co_lock(sche, name);
	if (!lock) {
		lock = new_stack_co_lock(sche, name);
		if (!lock) {
			goto err;
		}
		co_lock_alloc = 1;
	}
	if (!sche_lock(sche, owner, lock)) {
		goto err;
	}
	return lock;
err:
	if (co_lock_alloc) {
		hashtableRemoveNode(&sche->lock_htbl, &lock->htnode);
		free_stack_co_lock(lock);
	}
	return NULL;
}

StackCoLock_t* StackCoSche_try_lock(StackCoSche_t* sche, const StackCoLockOwner_t* owner, const char* name) {
	int co_lock_alloc = 0;
	StackCoLock_t* lock;

	lock = get_stack_co_lock(sche, name);
	if (!lock) {
		lock = new_stack_co_lock(sche, name);
		if (!lock) {
			goto err;
		}
		co_lock_alloc = 1;
	}
	else if (lock->owner && !lock_owner_check_equal(lock->owner, owner)) {
		return NULL;
	}
	if (!sche_lock(sche, owner, lock)) {
		goto err;
	}
	return lock;
err:
	if (co_lock_alloc) {
		hashtableRemoveNode(&sche->lock_htbl, &lock->htnode);
		free_stack_co_lock(lock);
	}
	return NULL;
}

void StackCoSche_unlock(StackCoSche_t* sche, StackCoLock_t* lock) {
	ListNode_t* lnode;
	StackCoBlockNode_t* block_node;

	if (lock->enter_times > 1) {
		lock->enter_times--;
		return;
	}
	if (listIsEmpty(&lock->wait_block_list)) {
		hashtableRemoveNode(&sche->lock_htbl, &lock->htnode);
		free_stack_co_lock(lock);
		return;
	}
	lnode = listPopNodeFront(&lock->wait_block_list);
	block_node = pod_container_of(lnode, StackCoBlockNode_t, ready_resume_listnode);
	StackCoSche_free_lock_owner(lock->owner);
	lock->owner = block_node->lock_owner;
	block_node->lock_owner = NULL;
	block_node->wait_lock = NULL;
	block_node->ready_resume_flag = 1;
	block_node->block.status = STACK_CO_STATUS_FINISH;

	listPushNodeBack(&sche->ready_resume_block_list, lnode);
	for (lnode = lock->wait_block_list.head; lnode; ) {
		ListNode_t* lnext;
		block_node = pod_container_of(lnode, StackCoBlockNode_t, ready_resume_listnode);
		if (!lock_owner_check_equal(lock->owner, block_node->lock_owner)) {
			return;
		}
		lock->enter_times++;
		StackCoSche_free_lock_owner(block_node->lock_owner);
		block_node->lock_owner = NULL;
		block_node->wait_lock = NULL;
		block_node->ready_resume_flag = 1;
		block_node->block.status = STACK_CO_STATUS_FINISH;

		lnext = lnode->next;
		listRemoveNode(&lock->wait_block_list, lnode);
		listPushNodeBack(&sche->ready_resume_block_list, lnode);
		lnode = lnext;
	}
}

StackCoBlock_t* StackCoSche_yield(StackCoSche_t* sche) {
	struct Fiber_t* cur_fiber = sche->cur_fiber;
	sche->cur_fiber = sche->sche_fiber;
	fiberSwitch(cur_fiber, sche->sche_fiber, 0);
	if (sche->resume_block_node) {
		return &sche->resume_block_node->block;
	}
	return NULL;
}

int StackCoSche_group_is_empty(StackCoBlockGroup_t* group) {
	return listIsEmpty(&group->wait_block_list) && listIsEmpty(&group->ready_block_list);
}

StackCoBlock_t* StackCoSche_yield_group(struct StackCoSche_t* sche, StackCoBlockGroup_t* group) {
	ListNode_t* lcur = listPopNodeFront(&group->ready_block_list);
	if (lcur) {
		StackCoBlockNode_t* block_node = pod_container_of(lcur, StackCoBlockNode_t, ready_resume_listnode);
		block_node->wait_group = NULL;
		return &block_node->block;
	}
	if (listIsEmpty(&group->wait_block_list)) {
		return NULL;
	}
	sche->exec_co_node->wait_group = group;
	return StackCoSche_yield(sche);
}

void StackCoSche_cleanup_async_param(StackCoAsyncParam_t* ap) {
	if (!ap) {
		return;
	}
	if (ap->fn_value_free) {
		ap->fn_value_free(ap->value);
		ap->fn_value_free = NULL;
	}
}

void StackCoSche_reuse_block(StackCoSche_t* sche, StackCoBlock_t* block) {
	StackCoBlockNode_t* block_node = pod_container_of(block, StackCoBlockNode_t, block);
	StackCoNode_t* exec_co_node = block_node->exec_co_node;
	if (!exec_co_node) {
		return;
	}
	block_node->exec_co_node = NULL;
	if (block_node->timeout_event) {
		rbtimerDetachEvent(block_node->timeout_event);
	}
	if (block_node->htnode.key.i32) {
		hashtableRemoveNode(&sche->block_co_htbl, &block_node->htnode);
		block_node->htnode.key.i32 = 0;
	}
	StackCoSche_cleanup_async_param(&block->resume_param);

	if (block_node->ready_resume_flag) {
		listRemoveNode(&sche->ready_resume_block_list, &block_node->ready_resume_listnode);
		block_node->ready_resume_flag = 0;
	}
	else if (block_node->wait_lock) {
		listRemoveNode(&block_node->wait_lock->wait_block_list, &block_node->ready_resume_listnode);
		block_node->wait_lock = NULL;
		if (block_node->lock_owner) {
			StackCoSche_free_lock_owner(block_node->lock_owner);
			block_node->lock_owner = NULL;
		}
	}
	else if (block_node->wait_group) {
		if (block_node->block.status != STACK_CO_STATUS_START) {
			listRemoveNode(&block_node->wait_group->ready_block_list, &block_node->ready_resume_listnode);
		}
		else {
			listRemoveNode(&block_node->wait_group->wait_block_list, &block_node->ready_resume_listnode);
		}
		block_node->wait_group = NULL;
	}

	listRemoveNode(&exec_co_node->block_list, &block_node->listnode);
	listPushNodeBack(&exec_co_node->reuse_block_list, &block_node->listnode);
}

void StackCoSche_reuse_block_group(struct StackCoSche_t* sche, StackCoBlockGroup_t* group) {
	ListNode_t* lcur;
	for (lcur = group->wait_block_list.head; lcur; lcur = lcur->next) {
		StackCoBlockNode_t* block_node = pod_container_of(lcur, StackCoBlockNode_t, ready_resume_listnode);
		block_node->wait_group = NULL;
		StackCoSche_reuse_block(sche, &block_node->block);
	}
	for (lcur = group->ready_block_list.head; lcur; lcur = lcur->next) {
		StackCoBlockNode_t* block_node = pod_container_of(lcur, StackCoBlockNode_t, ready_resume_listnode);
		block_node->wait_group = NULL;
		StackCoSche_reuse_block(sche, &block_node->block);
	}
	listInit(&group->wait_block_list);
	listInit(&group->ready_block_list);
}

void StackCoSche_resume_block_by_id(StackCoSche_t* sche, int block_id, int status, const StackCoAsyncParam_t* param) {
	StackCoResume_t* e;
	if (status >= 0) {
		return;
	}
	e = (StackCoResume_t*)malloc(sizeof(StackCoResume_t));
	if (!e) {
		return;
	}
	e->hdr.type = STACK_CO_HDR_RESUME;
	e->block_id = block_id;
	e->status = status;
	if (param) {
		e->param = *param;
	}
	else {
		memset(&e->param, 0, sizeof(e->param));
	}
	dataqueuePush(&sche->dq, &e->hdr.listnode);
}

int StackCoSche_sche(StackCoSche_t* sche, int idle_msec) {
	int i, handle_cnt;
	long long wait_msec;
	long long cur_msec;
	ListNode_t *lcur, *lnext;

	if (!sche->sche_fiber) {
		struct Fiber_t* sche_fiber = fiberFromThread();
		if (!sche_fiber) {
			return 1;
		}

		sche->proc_fiber = fiberCreate(sche_fiber, sche->stack_size, FiberProcEntry, sche);
		if (!sche->proc_fiber) {
			fiberFree(sche_fiber);
			return 1;
		}

		sche->cur_fiber = sche_fiber;
		sche->sche_fiber = sche_fiber;
	}

	if (sche->exit_flag) {
		for (lcur = sche->exec_co_list.head; lcur; lcur = lnext) {
			StackCoNode_t* co_node = pod_container_of(lcur, StackCoNode_t, hdr.listnode);
			lnext = lcur->next;

			sche->exec_co_node = co_node;
			sche->resume_block_node = NULL;
			sche->cur_fiber = co_node->fiber;
			fiberSwitch(sche->sche_fiber, co_node->fiber, 0);

			fiberFree(co_node->fiber);
			free_co_block_nodes(sche, co_node);
			free_stack_co_node(sche, co_node);
		}
		fiberFree(sche->sche_fiber);
		fiberFree(sche->proc_fiber);

		if (sche->fn_at_exit) {
			sche->fn_at_exit(sche, sche->fn_at_exit_arg);
			if (sche->fn_at_exit_arg_free) {
				sche->fn_at_exit_arg_free(sche->fn_at_exit_arg);
			}
		}
		return 1;
	}

	wait_msec = sche_calculate_min_wait_timelen(sche, idle_msec);
	handle_cnt = sche->handle_cnt;
	for (lcur = dataqueuePopWait(&sche->dq, wait_msec, handle_cnt); lcur; lcur = lnext) {
		StackCoHdr_t* hdr = pod_container_of(lcur, StackCoHdr_t, listnode);
		lnext = lcur->next;

		if (STACK_CO_HDR_PROC == hdr->type) {
			StackCoNode_t* co_node = pod_container_of(hdr, StackCoNode_t, hdr);
			if (co_node->timeout_event) {
				rbtimerAddEvent(&sche->timer, co_node->timeout_event);
				continue;
			}
			co_node->fiber = sche->proc_fiber;
			listPushNodeBack(&sche->exec_co_list, &co_node->hdr.listnode);
			stack_co_switch(sche, co_node, NULL);
		}
		else if (STACK_CO_HDR_RESUME == hdr->type) {
			StackCoResume_t* co_resume = pod_container_of(hdr, StackCoResume_t, hdr);
			StackCoBlockNode_t* block_node;
			HashtableNode_t* htnode;
			HashtableNodeKey_t key;

			key.i32 = co_resume->block_id;
			htnode = hashtableRemoveKey(&sche->block_co_htbl, key);
			if (!htnode) {
				StackCoSche_cleanup_async_param(&co_resume->param);
				free(co_resume);
				continue;
			}
			htnode->key.i32 = 0;
			block_node = pod_container_of(htnode, StackCoBlockNode_t, htnode);
			if (block_node->timeout_event) {
				rbtimerDetachEvent(block_node->timeout_event);
			}
			block_node->block.status = co_resume->status;
			block_node->block.resume_param = co_resume->param;
			free(co_resume);
			stack_co_switch(sche, block_node->exec_co_node, block_node);
		}
	}
	i = 0;
	for (lcur = sche->ready_resume_block_list.head; lcur && i < handle_cnt; lcur = lnext, ++i) {
		StackCoBlockNode_t* block_node = pod_container_of(lcur, StackCoBlockNode_t, ready_resume_listnode);
		lnext = lcur->next;
		listRemoveNode(&sche->ready_resume_block_list, lcur);
		block_node->ready_resume_flag = 0;

		if (block_node->timeout_event) {
			rbtimerDetachEvent(block_node->timeout_event);
		}
		stack_co_switch(sche, block_node->exec_co_node, block_node);
	}

	cur_msec = gmtimeMillisecond();
	for (i = 0; i < handle_cnt; ++i) {
		RBTimerEvent_t* e = rbtimerTimeoutPopup(&sche->timer, cur_msec);
		if (!e) {
			break;
		}
		e->callback(&sche->timer, e);
	}

	return 0;
}

void StackCoSche_wake_up(StackCoSche_t* sche) {
	dataqueueWake(&sche->dq);
}

void StackCoSche_exit(StackCoSche_t* sche) {
	sche->exit_flag = 1;
	dataqueueWake(&sche->dq);
}

void* StackCoSche_userdata(StackCoSche_t* sche) {
	return sche->userdata;
}

void StackCoSche_set_handle_cnt(struct StackCoSche_t* sche, int handle_cnt) {
	if (handle_cnt <= 0) {
		return;
	}
	sche->handle_cnt = handle_cnt;
}

int StackCoSche_has_exit(struct StackCoSche_t* sche) {
	return sche->exit_flag;
}

void StackCoSche_at_exit(StackCoSche_t* sche, void(*fn_at_exit)(StackCoSche_t*, void*), void* arg, void(*fn_arg_free)(void*)) {
	sche->fn_at_exit = fn_at_exit;
	sche->fn_at_exit_arg = arg;
	sche->fn_at_exit_arg_free = fn_arg_free;
}

#ifdef __cplusplus
}
#endif
