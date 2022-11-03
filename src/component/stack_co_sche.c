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
	int return_flag;
	int stack_size;
	StackCoNode_t* exec_co_node;
	StackCoNode_t* resume_co_node;
	void* proc_arg;
	void(*proc)(struct StackCoSche_t*, void*);
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

static void stack_co_switch(StackCoSche_t* sche, StackCoNode_t* dst_co_node) {
	Fiber_t* cur_fiber = sche->cur_fiber;
	sche->exec_co_node = dst_co_node->exec_co_node;
	sche->resume_co_node = dst_co_node;
	sche->cur_fiber = dst_co_node->fiber;
	fiberSwitch(cur_fiber, dst_co_node->fiber);
}

static void timer_sleep_callback(RBTimer_t* timer, struct RBTimerEvent_t* e) {
	StackCoSche_t* sche = pod_container_of(timer, StackCoSche_t, timer);
	StackCoNode_t* co_node = (StackCoNode_t*)e->arg;
	co_node->timeout_event = NULL;
	co_node->co.cancel = 1;
	free(e);
	stack_co_switch(sche, co_node);
}

static void timer_block_callback(RBTimer_t* timer, struct RBTimerEvent_t* e) {
	StackCoSche_t* sche = pod_container_of(timer, StackCoSche_t, timer);
	StackCoNode_t* co_node = (StackCoNode_t*)e->arg;
	hashtableRemoveNode(&sche->block_co_htbl, &co_node->htnode);
	co_node->htnode.key.i32 = 0;
	co_node->timeout_event = NULL;
	co_node->co.cancel = 1;
	free(e);
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
	stack_co_switch(sche, co_node);
}

static void FiberProcEntry(Fiber_t* fiber) {
	StackCoSche_t* sche = (StackCoSche_t*)fiber->arg;
	while (1) {
		void(*proc)(StackCoSche_t*, void*) = sche->proc;
		void* proc_arg = sche->proc_arg;
		sche->proc = NULL;
		sche->proc_arg = NULL;
		proc(sche, proc_arg);
		sche->exec_co_node = NULL;
		sche->cur_fiber = sche->sche_fiber;
		fiberSwitch(fiber, sche->sche_fiber);
	}
}

static int StackCoSche_update_proc_fiber(StackCoSche_t* sche) {
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
		lcur = lnext;

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

StackCo_t* StackCoSche_timeout_msec(struct StackCoSche_t* sche, long long msec, void(*proc)(struct StackCoSche_t*, void*), void* arg) {
	RBTimerEvent_t* timeout_event;
	StackCoNode_t* co_node;

	timeout_event = (RBTimerEvent_t*)calloc(1, sizeof(RBTimerEvent_t));
	if (!timeout_event) {
		return NULL;
	}
	co_node = alloc_stack_co_node();
	if (!co_node) {
		free(timeout_event);
		return NULL;
	}
	co_node->hdr.type = STACK_CO_HDR_PROC;
	co_node->proc = proc;
	co_node->proc_arg = arg;
	co_node->exec_co_node = co_node;
	co_node->timeout_event = timeout_event;

	timeout_event->timestamp = gmtimeMillisecond() + msec;
	timeout_event->callback = timer_timeout_callback;
	timeout_event->arg = co_node;

	dataqueuePush(&sche->dq, &co_node->hdr.listnode);
	return &co_node->co;
}

StackCo_t* StackCoSche_block_point(StackCoSche_t* sche, long long block_msec) {
	RBTimerEvent_t* e;
	StackCoNode_t* co_node;
	if (sche->cur_fiber == sche->sche_fiber) {
		return NULL;
	}

	if (block_msec > 0) {
		e = (RBTimerEvent_t*)calloc(1, sizeof(RBTimerEvent_t));
		if (!e) {
			return NULL;
		}
	}
	else {
		e = NULL;
	}
	co_node = alloc_stack_co_node();
	if (!co_node) {
		free(e);
		return NULL;
	}
	co_node->co.id = gen_co_id();
	co_node->htnode.key.i32 = co_node->co.id;
	if (hashtableInsertNode(&sche->block_co_htbl, &co_node->htnode) != &co_node->htnode) {
		free(e);
		free(co_node);
		return NULL;
	}
	if (!StackCoSche_update_proc_fiber(sche)) {
		hashtableRemoveNode(&sche->block_co_htbl, &co_node->htnode);
		free(e);
		free(co_node);
		return NULL;
	}
	co_node->fiber = sche->cur_fiber;
	co_node->exec_co_node = sche->exec_co_node;

	if (e) {
		e->timestamp = gmtimeMillisecond() + block_msec;
		e->callback = timer_block_callback;
		e->arg = co_node;
		rbtimerAddEvent(&sche->timer, co_node->timeout_event);
	}

	listPushNodeBack(&sche->exec_co_node->co_list, &co_node->hdr.listnode);
	return &co_node->co;
}

StackCo_t* StackCoSche_sleep_msec(StackCoSche_t* sche, long long msec) {
	RBTimerEvent_t* e;
	StackCoNode_t* co_node;
	if (sche->cur_fiber == sche->sche_fiber) {
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
	if (!StackCoSche_update_proc_fiber(sche)) {
		free(e);
		free(co_node);
		return NULL;
	}
	co_node->fiber = sche->cur_fiber;
	co_node->timeout_event = e;
	co_node->exec_co_node = sche->exec_co_node;

	e->timestamp = gmtimeMillisecond() + msec;
	e->callback = timer_sleep_callback;
	e->arg = co_node;
	rbtimerAddEvent(&sche->timer, co_node->timeout_event);

	listPushNodeBack(&sche->exec_co_node->co_list, &co_node->hdr.listnode);
	return &co_node->co;
}

StackCo_t* StackCoSche_yield(StackCoSche_t* sche) {
	Fiber_t* cur_fiber = sche->cur_fiber;
	sche->cur_fiber = sche->sche_fiber;
	fiberSwitch(cur_fiber, sche->sche_fiber);
	return &sche->resume_co_node->co;
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
	if (co->cancel) {
		return;
	}
	co->cancel = 1;
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
		HashtableNode_t* htnode, *htnext;
		for (htnode = hashtableFirstNode(&sche->block_co_htbl); htnode; htnode = hashtableNextNode(htnode)) {
			StackCoNode_t* co_node = pod_container_of(htnode, StackCoNode_t, htnode);
			co_node->co.cancel = 1;
		}
		for (htnode = hashtableFirstNode(&sche->block_co_htbl); htnode; htnode = htnext) {
			StackCoNode_t* co_node = pod_container_of(htnode, StackCoNode_t, htnode);
			htnext = hashtableNextNode(htnode);

			hashtableRemoveNode(&sche->block_co_htbl, htnode);
			co_node->co.ret = NULL;
			stack_co_switch(sche, co_node);
			fiberFree(co_node->fiber);
			free(co_node);
		}
		fiberFree(sche->sche_fiber);
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
			sche->exec_co_node = co_node;
			sche->proc = co_node->proc;
			sche->proc_arg = co_node->proc_arg;
			listInit(&co_node->co_list);
			co_node->fiber = sche->proc_fiber;
			stack_co_switch(sche, co_node);
			if (!sche->exec_co_node) {
				fiberFree(co_node->fiber);
			}
			free(co_node);
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
				free(co_node->timeout_event);
				co_node->timeout_event = NULL;
			}
			co_node->co.ret = ret;
			stack_co_switch(sche, co_node);
			if (!sche->exec_co_node) {
				fiberFree(co_node->fiber);
			}
			free(co_node);
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
