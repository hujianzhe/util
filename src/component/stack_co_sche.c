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
#include <stdlib.h>
#include <limits.h>

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
	StackCo_t co;
	Fiber_t* fiber;
	void(*proc)(struct StackCoSche_t*, void*);
	void* proc_arg;
	void(*fn_proc_arg_free)(void*);
	List_t block_list;
	List_t reuse_block_list;
	RBTimerEvent_t* timeout_event;
} StackCoNode_t;

typedef struct StackCoResume_t {
	StackCoHdr_t hdr;
	int block_id;
	void* ret;
	void(*fn_ret_free)(void*);
} StackCoResume_t;

typedef struct StackCoBlockNode_t {
	StackCoBlock_t block;
	StackCoNode_t* exec_co_node;
	ListNode_t listnode;
	HashtableNode_t htnode;
	void(*fn_resume_ret_free)(void*);
	RBTimerEvent_t* timeout_event;
} StackCoBlockNode_t;

typedef struct StackCoSche_t {
	volatile int exit_flag;
	void* userdata;
	void(*fn_at_exit)(struct StackCoSche_t*, void*);
	void* fn_at_exit_arg;
	void(*fn_at_exit_arg_free)(void*);

	DataQueue_t dq;
	RBTimer_t timer;
	Fiber_t* proc_fiber;
	Fiber_t* cur_fiber;
	Fiber_t* sche_fiber;
	int stack_size;
	StackCoNode_t* exec_co_node;
	StackCoBlockNode_t* resume_block_node;
	List_t exec_co_list;
	Hashtable_t block_co_htbl;
	HashtableNode_t* block_co_htbl_bulks[2048];
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

static StackCoBlockNode_t* alloc_block_node(void) {
	StackCoBlockNode_t* block_node = (StackCoBlockNode_t*)calloc(1, sizeof(StackCoBlockNode_t));
	return block_node;
}

static void free_stack_co_node(StackCoSche_t* sche, StackCoNode_t* co_node) {
	if (co_node->timeout_event) {
		rbtimerDetachEvent(co_node->timeout_event);
		free(co_node->timeout_event);
	}
	if (co_node->fn_proc_arg_free) {
		co_node->fn_proc_arg_free(co_node->proc_arg);
	}
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
	if (block_node->fn_resume_ret_free) {
		block_node->fn_resume_ret_free(block_node->block.resume_ret);
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

static void reset_block_data(StackCoBlock_t* block) {
	block->id = 0;
	block->status = STACK_CO_STATUS_START;
	block->resume_ret = NULL;
}

static void FiberProcEntry(Fiber_t* fiber) {
	StackCoSche_t* sche = (StackCoSche_t*)fiber->arg;
	while (1) {
		StackCoNode_t* exec_co_node = sche->exec_co_node;
		exec_co_node->proc(sche, exec_co_node->proc_arg);
		exec_co_node->co.status = STACK_CO_STATUS_FINISH;
		sche->cur_fiber = sche->sche_fiber;
		fiberSwitch(fiber, sche->sche_fiber);
	}
}

static void stack_co_switch(StackCoSche_t* sche, StackCoNode_t* dst_co_node, StackCoBlockNode_t* dst_block_node) {
	Fiber_t* cur_fiber = sche->cur_fiber;
	Fiber_t* dst_fiber = dst_co_node->fiber;
	StackCoNode_t* exec_co_node = dst_co_node;

	sche->exec_co_node = exec_co_node;
	sche->resume_block_node = dst_block_node;
	sche->cur_fiber = dst_fiber;
	fiberSwitch(cur_fiber, dst_fiber);
	if (exec_co_node->co.status >= 0) {
		return;
	}
	if (dst_fiber != sche->proc_fiber) {
		fiberFree(dst_fiber);
	}
	exec_co_node->fiber = NULL;
	listRemoveNode(&sche->exec_co_list, &exec_co_node->hdr.listnode);
	free_co_block_nodes(sche, exec_co_node);
	free_stack_co_node(sche, exec_co_node);
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
		Fiber_t* proc_fiber = fiberCreate(sche->cur_fiber, sche->stack_size, FiberProcEntry);
		if (!proc_fiber) {
			return 0;
		}
		proc_fiber->arg = sche;
		sche->proc_fiber = proc_fiber;
	}
	return 1;
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
	hashtableInit(&sche->block_co_htbl,
			sche->block_co_htbl_bulks, sizeof(sche->block_co_htbl_bulks) / sizeof(sche->block_co_htbl_bulks[0]),
			hashtableDefaultKeyCmp32, hashtableDefaultKeyHash32);
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
			if (e->fn_ret_free) {
				e->fn_ret_free(e->ret);
			}
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

StackCo_t* StackCoSche_function(StackCoSche_t* sche, void(*proc)(StackCoSche_t*, void*), void* arg, void(*fn_arg_free)(void*)) {
	StackCoNode_t* co_node = alloc_stack_co_node();
	if (!co_node) {
		return NULL;
	}
	co_node->hdr.type = STACK_CO_HDR_PROC;
	co_node->proc = proc;
	co_node->proc_arg = arg;
	co_node->fn_proc_arg_free = fn_arg_free;
	dataqueuePush(&sche->dq, &co_node->hdr.listnode);
	return &co_node->co;
}

StackCo_t* StackCoSche_timeout_util(struct StackCoSche_t* sche, long long tm_msec, void(*proc)(struct StackCoSche_t*, void*), void* arg, void(*fn_arg_free)(void*)) {
	RBTimerEvent_t* e;
	StackCoNode_t* co_node;

	if (tm_msec < 0) {
		return NULL;
	}
	e = (RBTimerEvent_t*)calloc(1, sizeof(RBTimerEvent_t));
	if (!e) {
		return NULL;
	}
	co_node = alloc_stack_co_node();
	if (!co_node) {
		free(e);
		return NULL;
	}
	co_node->hdr.type = STACK_CO_HDR_PROC;
	co_node->proc = proc;
	co_node->proc_arg = arg;
	co_node->fn_proc_arg_free = fn_arg_free;
	co_node->timeout_event = e;

	e->timestamp = tm_msec;
	e->callback = timer_timeout_callback;
	e->arg = co_node;

	dataqueuePush(&sche->dq, &co_node->hdr.listnode);
	return &co_node->co;
}

StackCoBlock_t* StackCoSche_block_point_util(StackCoSche_t* sche, long long tm_msec) {
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

StackCoBlock_t* StackCoSche_sleep_util(StackCoSche_t* sche, long long tm_msec) {
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

StackCoBlock_t* StackCoSche_yield(StackCoSche_t* sche) {
	Fiber_t* cur_fiber = sche->cur_fiber;
	sche->cur_fiber = sche->sche_fiber;
	fiberSwitch(cur_fiber, sche->sche_fiber);
	if (sche->resume_block_node) {
		return &sche->resume_block_node->block;
	}
	return NULL;
}

void* StackCoSche_pop_resume_ret(StackCoBlock_t* block) {
	StackCoBlockNode_t* block_node = pod_container_of(block, StackCoBlockNode_t, block);
	void* ret = block->resume_ret;
	block->resume_ret = NULL;
	block_node->fn_resume_ret_free = NULL;
	return ret;
}

void StackCoSche_reuse_block(StackCoBlock_t* block) {
	StackCoBlockNode_t* block_node = pod_container_of(block, StackCoBlockNode_t, block);
	StackCoNode_t* exec_co_node = block_node->exec_co_node;
	if (!exec_co_node) {
		return;
	}
	if (block_node->fn_resume_ret_free) {
		block_node->fn_resume_ret_free(block->resume_ret);
		block_node->fn_resume_ret_free = NULL;
	}
	listRemoveNode(&exec_co_node->block_list, &block_node->listnode);
	listPushNodeBack(&exec_co_node->reuse_block_list, &block_node->listnode);
}

void StackCoSche_resume_block(StackCoSche_t* sche, int block_id, void* ret, void(*fn_ret_free)(void*)) {
	StackCoResume_t* e = (StackCoResume_t*)malloc(sizeof(StackCoResume_t));
	if (!e) {
		return;
	}
	e->hdr.type = STACK_CO_HDR_RESUME;
	e->block_id = block_id;
	e->ret = ret;
	e->fn_ret_free = fn_ret_free;
	dataqueuePush(&sche->dq, &e->hdr.listnode);
}

void StackCoSche_cancel_block(StackCoSche_t* sche, StackCoBlock_t* block) {
	StackCoBlockNode_t* block_node = pod_container_of(block, StackCoBlockNode_t, block);
	if (block->status < 0) {
		return;
	}
	block->status = STACK_CO_STATUS_CANCEL;
	if (block_node->timeout_event) {
		rbtimerDetachEvent(block_node->timeout_event);
		free(block_node->timeout_event);
		block_node->timeout_event = NULL;
	}
	if (block_node->htnode.key.i32) {
		hashtableRemoveNode(&sche->block_co_htbl, &block_node->htnode);
		block_node->htnode.key.i32 = 0;
	}
}

int StackCoSche_sche(StackCoSche_t* sche, int idle_msec) {
	int i;
	long long wait_msec;
	long long min_t, cur_msec;
	ListNode_t *lcur, *lnext;

	if (!sche->sche_fiber) {
		Fiber_t* sche_fiber = fiberFromThread();
		if (!sche_fiber) {
			return 1;
		}

		sche->proc_fiber = fiberCreate(sche_fiber, sche->stack_size, FiberProcEntry);
		if (!sche->proc_fiber) {
			fiberFree(sche_fiber);
			return 1;
		}
		sche->proc_fiber->arg = sche;

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
			fiberSwitch(sche->sche_fiber, co_node->fiber);

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

	min_t = rbtimerMiniumTimestamp(&sche->timer);
	if (min_t >= 0) {
		cur_msec = gmtimeMillisecond();
		if (min_t <= cur_msec) {
			wait_msec = 0;
		}
		else {
			wait_msec = min_t - cur_msec;
			if (idle_msec >= 0 && wait_msec > idle_msec) {
				wait_msec = idle_msec;
			}
		}
	}
	else {
		wait_msec = idle_msec;
	}

	for (lcur = dataqueuePopWait(&sche->dq, wait_msec, 100); lcur; lcur = lnext) {
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
			HashtableNode_t* htnode;
			HashtableNodeKey_t key;
			StackCoBlockNode_t* block_node;

			key.i32 = co_resume->block_id;
			htnode = hashtableRemoveKey(&sche->block_co_htbl, key);
			if (!htnode) {
				if (co_resume->fn_ret_free) {
					co_resume->fn_ret_free(co_resume->ret);
				}
				free(co_resume);
				continue;
			}
			htnode->key.i32 = 0;
			block_node = pod_container_of(htnode, StackCoBlockNode_t, htnode);
			if (block_node->timeout_event) {
				rbtimerDetachEvent(block_node->timeout_event);
			}
			block_node->fn_resume_ret_free = co_resume->fn_ret_free;
			block_node->block.resume_ret = co_resume->ret;
			block_node->block.status = STACK_CO_STATUS_FINISH;
			free(co_resume);
			stack_co_switch(sche, block_node->exec_co_node, block_node);
		}
	}

	cur_msec = gmtimeMillisecond();
	for (i = 0; i < 100; ++i) {
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

void StackCoSche_at_exit(StackCoSche_t* sche, void(*fn_at_exit)(StackCoSche_t*, void*), void* arg, void(*fn_arg_free)(void*)) {
	sche->fn_at_exit = fn_at_exit;
	sche->fn_at_exit_arg = arg;
	sche->fn_at_exit_arg_free = fn_arg_free;
}

#ifdef __cplusplus
}
#endif
