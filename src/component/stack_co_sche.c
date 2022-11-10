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
	struct StackCoNode_t* exec_co_node;
	union {
		List_t co_list;
		struct {
			void(*proc)(struct StackCoSche_t*, void*);
			void* proc_arg;
		};
	};
	List_t reuse_list;
	RBTimerEvent_t* timeout_event;
	HashtableNode_t htnode;
} StackCoNode_t;

typedef struct StackCoResume_t {
	StackCoHdr_t hdr;
	int co_id;
	void* ret;
} StackCoResume_t;

typedef struct StackCoSche_t {
	volatile int exit_flag;
	DataQueue_t dq;
	RBTimer_t timer;
	Fiber_t* proc_fiber;
	Fiber_t* cur_fiber;
	Fiber_t* sche_fiber;
	int stack_size;
	StackCoNode_t* exec_co_node;
	StackCoNode_t* resume_co_node;
	void* proc_arg;
	void(*proc)(struct StackCoSche_t*, void*);
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
	return (StackCoNode_t*)calloc(1, sizeof(StackCoNode_t));
}

static void free_stack_co_node(StackCoSche_t* sche, StackCoNode_t* co_node) {
	if (co_node->timeout_event) {
		rbtimerDetachEvent(co_node->timeout_event);
		free(co_node->timeout_event);
	}
	if (co_node->htnode.key.i32) {
		hashtableRemoveNode(&sche->block_co_htbl, &co_node->htnode);
	}
	free(co_node);
}

static void free_child_stack_co_nodes(StackCoSche_t* sche, StackCoNode_t* co_node) {
	ListNode_t* lcur, *lnext;
	for (lcur = co_node->co_list.head; lcur; lcur = lnext) {
		StackCoNode_t* co_node = pod_container_of(lcur, StackCoNode_t, hdr.listnode);
		lnext = lcur->next;
		free_stack_co_node(sche, co_node);
	}
	for (lcur = co_node->reuse_list.head; lcur; lcur = lnext) {
		StackCoNode_t* co_node = pod_container_of(lcur, StackCoNode_t, hdr.listnode);
		lnext = lcur->next;
		free_stack_co_node(sche, co_node);
	}
}

static void reset_stack_co_data(StackCo_t* co) {
	co->id = 0;
	co->status = STACK_CO_STATUS_START;
	co->ret = NULL;
	co->udata = 0;
}

static void FiberProcEntry(Fiber_t* fiber) {
	StackCoSche_t* sche = (StackCoSche_t*)fiber->arg;
	while (1) {
		StackCoNode_t* exec_co_node = sche->exec_co_node;
		void(*proc)(StackCoSche_t*, void*) = sche->proc;
		void* proc_arg = sche->proc_arg;
		sche->proc = NULL;
		sche->proc_arg = NULL;
		proc(sche, proc_arg);
		exec_co_node->co.status = STACK_CO_STATUS_FINISH;
		sche->cur_fiber = sche->sche_fiber;
		fiberSwitch(fiber, sche->sche_fiber);
	}
}

static void stack_co_switch(StackCoSche_t* sche, StackCoNode_t* dst_co_node) {
	ListNode_t* lcur, *lnext;
	Fiber_t* cur_fiber = sche->cur_fiber;
	Fiber_t* dst_fiber = dst_co_node->fiber;
	StackCoNode_t* exec_co_node = dst_co_node->exec_co_node;

	sche->exec_co_node = exec_co_node;
	sche->resume_co_node = dst_co_node;
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
	free_child_stack_co_nodes(sche, exec_co_node);
	free_stack_co_node(sche, exec_co_node);
	sche->exec_co_node = NULL;
}

static void timer_sleep_callback(RBTimer_t* timer, struct RBTimerEvent_t* e) {
	StackCoSche_t* sche = pod_container_of(timer, StackCoSche_t, timer);
	StackCoNode_t* co_node = (StackCoNode_t*)e->arg;
	co_node->co.status = STACK_CO_STATUS_FINISH;
	stack_co_switch(sche, co_node);
}

static void timer_block_callback(RBTimer_t* timer, struct RBTimerEvent_t* e) {
	StackCoSche_t* sche = pod_container_of(timer, StackCoSche_t, timer);
	StackCoNode_t* co_node = (StackCoNode_t*)e->arg;
	hashtableRemoveNode(&sche->block_co_htbl, &co_node->htnode);
	co_node->htnode.key.i32 = 0;
	co_node->co.status = STACK_CO_STATUS_CANCEL;
	stack_co_switch(sche, co_node);
}

static void timer_timeout_callback(RBTimer_t* timer, struct RBTimerEvent_t* e) {
	StackCoSche_t* sche = pod_container_of(timer, StackCoSche_t, timer);
	StackCoNode_t* co_node = (StackCoNode_t*)e->arg;
	sche->proc = co_node->proc;
	sche->proc_arg = co_node->proc_arg;
	listInit(&co_node->co_list);
	co_node->fiber = sche->proc_fiber;
	co_node->timeout_event = NULL;
	free(e);
	listPushNodeBack(&sche->exec_co_list, &co_node->hdr.listnode);
	stack_co_switch(sche, co_node);
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

StackCoSche_t* StackCoSche_new(size_t stack_size) {
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
	sche->proc_fiber = NULL;
	sche->cur_fiber = NULL;
	sche->sche_fiber = NULL;
	sche->stack_size = stack_size;
	sche->exec_co_node = NULL;
	sche->resume_co_node = NULL;
	sche->proc_arg = NULL;
	sche->proc = NULL;
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

StackCo_t* StackCoSche_function(StackCoSche_t* sche, void(*proc)(StackCoSche_t*, void*), void* arg) {
	StackCoNode_t* co_node = alloc_stack_co_node();
	if (!co_node) {
		return NULL;
	}
	co_node->hdr.type = STACK_CO_HDR_PROC;
	co_node->proc = proc;
	co_node->proc_arg = arg;
	co_node->exec_co_node = co_node;
	dataqueuePush(&sche->dq, &co_node->hdr.listnode);
	return &co_node->co;
}

StackCo_t* StackCoSche_timeout_util(struct StackCoSche_t* sche, long long tm_msec, void(*proc)(struct StackCoSche_t*, void*), void* arg) {
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
	co_node->exec_co_node = co_node;
	co_node->timeout_event = e;

	e->timestamp = tm_msec;
	e->callback = timer_timeout_callback;
	e->arg = co_node;

	dataqueuePush(&sche->dq, &co_node->hdr.listnode);
	return &co_node->co;
}

StackCo_t* StackCoSche_block_point_util(StackCoSche_t* sche, long long tm_msec) {
	RBTimerEvent_t* e = NULL;
	StackCoNode_t* co_node = NULL, *exec_co_node;
	int co_node_alloc = 0, timeout_event_alloc = 0;

	if (sche->cur_fiber == sche->sche_fiber) {
		return NULL;
	}

	exec_co_node = sche->exec_co_node;
	if (listIsEmpty(&exec_co_node->reuse_list)) {
		co_node = alloc_stack_co_node();
		if (!co_node) {
			goto err;
		}
		co_node_alloc = 1;
	}
	else {
		ListNode_t* listnode = listPopNodeBack(&exec_co_node->reuse_list);
		co_node = pod_container_of(listnode, StackCoNode_t, hdr.listnode);
		reset_stack_co_data(&co_node->co);
	}
	if (tm_msec >= 0) {
		if (co_node->timeout_event) {
			e = co_node->timeout_event;
		}
		else {
			e = (RBTimerEvent_t*)calloc(1, sizeof(RBTimerEvent_t));
			if (!e) {
				goto err;
			}
			timeout_event_alloc = 1;
			co_node->timeout_event = e;
		}
	}
	co_node->co.id = gen_co_id();
	co_node->htnode.key.i32 = co_node->co.id;
	if (hashtableInsertNode(&sche->block_co_htbl, &co_node->htnode) != &co_node->htnode) {
		goto err;
	}
	if (!sche_update_proc_fiber(sche)) {
		hashtableRemoveNode(&sche->block_co_htbl, &co_node->htnode);
		goto err;
	}
	co_node->fiber = sche->cur_fiber;
	co_node->exec_co_node = exec_co_node;

	if (e) {
		e->timestamp = tm_msec;
		e->callback = timer_block_callback;
		e->arg = co_node;
		rbtimerAddEvent(&sche->timer, e);
	}

	listPushNodeBack(&exec_co_node->co_list, &co_node->hdr.listnode);
	return &co_node->co;
err:
	if (co_node_alloc) {
		free(co_node);
	}
	else if (co_node) {
		listPushNodeBack(&exec_co_node->reuse_list, &co_node->hdr.listnode);
	}
	if (timeout_event_alloc) {
		free(e);
	}
	return NULL;
}

StackCo_t* StackCoSche_sleep_util(StackCoSche_t* sche, long long tm_msec) {
	RBTimerEvent_t* e = NULL;
	StackCoNode_t* co_node = NULL, *exec_co_node;
	int co_node_alloc = 0, timeout_event_alloc = 0;

	if (sche->cur_fiber == sche->sche_fiber) {
		return NULL;
	}

	if (tm_msec < 0) {
		tm_msec = 0;
	}
	exec_co_node = sche->exec_co_node;
	if (listIsEmpty(&exec_co_node->reuse_list)) {
		co_node = alloc_stack_co_node();
		if (!co_node) {
			goto err;
		}
		co_node_alloc = 1;
	}
	else {
		ListNode_t* listnode = listPopNodeBack(&exec_co_node->reuse_list);
		co_node = pod_container_of(listnode, StackCoNode_t, hdr.listnode);
		reset_stack_co_data(&co_node->co);
	}
	if (co_node->timeout_event) {
		e = co_node->timeout_event;
	}
	else {
		e = (RBTimerEvent_t*)calloc(1, sizeof(RBTimerEvent_t));
		if (!e) {
			goto err;
		}
		timeout_event_alloc = 1;
		co_node->timeout_event = e;
	}
	if (!sche_update_proc_fiber(sche)) {
		goto err;
	}
	co_node->fiber = sche->cur_fiber;
	co_node->exec_co_node = exec_co_node;

	e->timestamp = tm_msec;
	e->callback = timer_sleep_callback;
	e->arg = co_node;
	rbtimerAddEvent(&sche->timer, co_node->timeout_event);

	listPushNodeBack(&exec_co_node->co_list, &co_node->hdr.listnode);
	return &co_node->co;
err:
	if (co_node_alloc) {
		free(co_node);
	}
	else if (co_node) {
		listPushNodeBack(&exec_co_node->reuse_list, &co_node->hdr.listnode);
	}
	if (timeout_event_alloc) {
		free(e);
	}
	return NULL;
}

StackCo_t* StackCoSche_yield(StackCoSche_t* sche) {
	Fiber_t* cur_fiber = sche->cur_fiber;
	sche->cur_fiber = sche->sche_fiber;
	fiberSwitch(cur_fiber, sche->sche_fiber);
	if (sche->resume_co_node) {
		return &sche->resume_co_node->co;
	}
	return NULL;
}

void StackCoSche_reuse_co(StackCo_t* co) {
	StackCoNode_t* co_node = pod_container_of(co, StackCoNode_t, co);
	StackCoNode_t* exec_co_node = co_node->exec_co_node;
	if (!exec_co_node || exec_co_node == co_node) {
		return;
	}
	listRemoveNode(&exec_co_node->co_list, &co_node->hdr.listnode);
	listPushNodeBack(&exec_co_node->reuse_list, &co_node->hdr.listnode);
}

void StackCoSche_resume_co(StackCoSche_t* sche, int co_id, void* ret) {
	StackCoResume_t* e = (StackCoResume_t*)malloc(sizeof(StackCoResume_t));
	if (!e) {
		return;
	}
	e->hdr.type = STACK_CO_HDR_RESUME;
	e->co_id = co_id;
	e->ret = ret;
	dataqueuePush(&sche->dq, &e->hdr.listnode);
}

void StackCoSche_cancel_co(StackCoSche_t* sche, StackCo_t* co) {
	StackCoNode_t* co_node = pod_container_of(co, StackCoNode_t, co);
	if (co->status < 0) {
		return;
	}
	co->status = STACK_CO_STATUS_CANCEL;
	if (co_node->timeout_event) {
		rbtimerDetachEvent(co_node->timeout_event);
		free(co_node->timeout_event);
		co_node->timeout_event = NULL;
	}
	if (co_node->htnode.key.i32) {
		hashtableRemoveNode(&sche->block_co_htbl, &co_node->htnode);
		co_node->htnode.key.i32 = 0;
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
			sche->resume_co_node = NULL;
			sche->cur_fiber = co_node->fiber;
			fiberSwitch(sche->sche_fiber, co_node->fiber);

			fiberFree(co_node->fiber);
			free_child_stack_co_nodes(sche, co_node);
			free_stack_co_node(sche, co_node);
		}
		fiberFree(sche->sche_fiber);
		fiberFree(sche->proc_fiber);
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
			sche->proc = co_node->proc;
			sche->proc_arg = co_node->proc_arg;
			listInit(&co_node->co_list);
			co_node->fiber = sche->proc_fiber;
			listPushNodeBack(&sche->exec_co_list, &co_node->hdr.listnode);
			stack_co_switch(sche, co_node);
		}
		else if (STACK_CO_HDR_RESUME == hdr->type) {
			StackCoResume_t* e = pod_container_of(hdr, StackCoResume_t, hdr);
			HashtableNode_t* htnode;
			HashtableNodeKey_t key;
			StackCoNode_t* co_node;
			int co_id = e->co_id;
			void* ret = e->ret;

			free(e);
			key.i32 = co_id;
			htnode = hashtableRemoveKey(&sche->block_co_htbl, key);
			if (!htnode) {
				continue;
			}
			htnode->key.i32 = 0;
			co_node = pod_container_of(htnode, StackCoNode_t, htnode);
			if (co_node->timeout_event) {
				rbtimerDetachEvent(co_node->timeout_event);
			}
			co_node->co.ret = ret;
			co_node->co.status = STACK_CO_STATUS_FINISH;
			stack_co_switch(sche, co_node);
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

#ifdef __cplusplus
}
#endif
