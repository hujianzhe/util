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

typedef struct StackCoNode_t {
	StackCo_t co;
	Fiber_t* fiber;
	RBTimerEvent_t* timeout_event;
	HashtableNode_t htnode;
} StackCoNode_t;

enum {
	STACK_CO_EVENT_PROC,
	STACK_CO_EVENT_RESUME,
	STACK_CO_EVENT_TIMEOUT
};

typedef struct StackCoEvent_t {
	ListNode_t listnode;
	int type;
	union {
		struct {
			int co_id;
			void* ret;
		};
		struct {
			void(*proc)(struct StackCoSche_t*, void*);
			void* proc_arg;
			RBTimerEvent_t* timeout_event;
		};
	};
} StackCoEvent_t;

typedef struct StackCoSche_t {
	volatile int exit_flag;
	DataQueue_t dq;
	RBTimer_t timer;
	Fiber_t* proc_fiber;
	Fiber_t* cur_fiber;
	Fiber_t* sche_fiber;
	int return_flag;
	int stack_size;
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

static void do_fiber_switch(StackCoSche_t* sche, Fiber_t* dst_fiber) {
	Fiber_t* cur_fiber = sche->cur_fiber;
	sche->cur_fiber = dst_fiber;
	fiberSwitch(cur_fiber, dst_fiber);
}

static void timer_sleep_callback(RBTimer_t* timer, struct RBTimerEvent_t* e) {
	StackCoSche_t* sche = pod_container_of(timer, StackCoSche_t, timer);
	StackCoNode_t* co_node = (StackCoNode_t*)e->arg;
	co_node->timeout_event = NULL;
	free(e);
	do_fiber_switch(sche, co_node->fiber);
}

static void timer_block_callback(RBTimer_t* timer, struct RBTimerEvent_t* e) {
	StackCoSche_t* sche = pod_container_of(timer, StackCoSche_t, timer);
	StackCoNode_t* co_node = (StackCoNode_t*)e->arg;
	hashtableRemoveNode(&sche->block_co_htbl, &co_node->htnode);
	co_node->htnode.key.i32 = 0;
	co_node->timeout_event = NULL;
	co_node->co.cancel = 1;
	free(e);
	do_fiber_switch(sche, co_node->fiber);
}

static void timer_timeout_callback(RBTimer_t* timer, struct RBTimerEvent_t* e) {
	StackCoSche_t* sche = pod_container_of(timer, StackCoSche_t, timer);
	StackCoEvent_t* co_event = (StackCoEvent_t*)e->arg;
	sche->proc = co_event->proc;
	sche->proc_arg = co_event->proc_arg;
	free(co_event);
	free(e);
	do_fiber_switch(sche, sche->proc_fiber);
}

static void FiberProcEntry(Fiber_t* fiber) {
	StackCoSche_t* sche = (StackCoSche_t*)fiber->arg;
	while (1) {
		void(*proc)(StackCoSche_t*, void*) = sche->proc;
		void* proc_arg = sche->proc_arg;
		sche->proc = NULL;
		sche->proc_arg = NULL;
		proc(sche, proc_arg);
		sche->resume_co_node = NULL;
		do_fiber_switch(sche, sche->sche_fiber);
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

StackCoSche_t* StackCoSche_new(Fiber_t* sche_fiber, size_t stack_size) {
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
	sche->proc_fiber = fiberCreate(sche_fiber, stack_size, FiberProcEntry);
	if (!sche->proc_fiber) {
		goto err;
	}
	sche->proc_fiber->arg = sche;
	sche->cur_fiber = sche_fiber;
	sche->sche_fiber = sche_fiber;
	sche->stack_size = stack_size;
	sche->resume_co_node = NULL;
	sche->proc_arg = NULL;
	sche->proc = NULL;
	sche->exit_flag = 0;
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
	dataqueueDestroy(&sche->dq);
	rbtimerDestroy(&sche->timer);
}

void StackCoSche_function(StackCoSche_t* sche, void(*proc)(StackCoSche_t*, void*), void* arg) {
	StackCoEvent_t* e = (StackCoEvent_t*)malloc(sizeof(StackCoEvent_t));
	if (!e) {
		return;
	}
	e->type = STACK_CO_EVENT_PROC;
	e->proc = proc;
	e->proc_arg = arg;
	dataqueuePush(&sche->dq, &e->listnode);
}

void StackCoSche_timeout_msec(struct StackCoSche_t* sche, long long msec, void(*proc)(struct StackCoSche_t*, void*), void* arg) {
	RBTimerEvent_t* timeout_event;
	StackCoEvent_t* co_event;

	timeout_event = (RBTimerEvent_t*)calloc(1, sizeof(RBTimerEvent_t));
	if (!timeout_event) {
		return;
	}
	co_event = (StackCoEvent_t*)malloc(sizeof(StackCoEvent_t));
	if (!co_event) {
		free(timeout_event);
		return;
	}
	co_event->type = STACK_CO_EVENT_TIMEOUT;
	co_event->proc = proc;
	co_event->proc_arg = arg;
	co_event->timeout_event = timeout_event;

	timeout_event->timestamp = gmtimeMillisecond() + msec;
	timeout_event->callback = timer_timeout_callback;
	timeout_event->arg = co_event;

	dataqueuePush(&sche->dq, &co_event->listnode);
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

	if (e) {
		e->timestamp = gmtimeMillisecond() + block_msec;
		e->callback = timer_block_callback;
		e->arg = co_node;
		rbtimerAddEvent(&sche->timer, co_node->timeout_event);
	}
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

	e->timestamp = gmtimeMillisecond() + msec;
	e->callback = timer_sleep_callback;
	e->arg = co_node;
	rbtimerAddEvent(&sche->timer, co_node->timeout_event);

	return &co_node->co;
}

StackCo_t* StackCoSche_yield(StackCoSche_t* sche) {
	do_fiber_switch(sche, sche->sche_fiber);
	if (sche->resume_co_node) {
		return &sche->resume_co_node->co;
	}
	return NULL;
}

void StackCoSche_resume_co(StackCoSche_t* sche, int co_id, void* ret) {
	StackCoEvent_t* e = (StackCoEvent_t*)malloc(sizeof(StackCoEvent_t));
	if (!e) {
		return;
	}
	e->type = STACK_CO_EVENT_RESUME;
	e->co_id = co_id;
	e->ret = ret;
	dataqueuePush(&sche->dq, &e->listnode);
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

	if (sche->exit_flag) {
		HashtableNode_t* htnode, *htnext;
		for (htnode = hashtableFirstNode(&sche->block_co_htbl); htnode; htnode = hashtableNextNode(htnode)) {
			StackCoNode_t* co_node = pod_container_of(htnode, StackCoNode_t, htnode);
			co_node->co.cancel = 1;
		}
		sche->resume_co_node = NULL;
		for (htnode = hashtableFirstNode(&sche->block_co_htbl); htnode; htnode = htnext) {
			StackCoNode_t* co_node = pod_container_of(htnode, StackCoNode_t, htnode);
			htnext = hashtableNextNode(htnode);

			hashtableRemoveNode(&sche->block_co_htbl, htnode);
			co_node->co.ret = NULL;
			do_fiber_switch(sche, co_node->fiber);
			fiberFree(co_node->fiber);
			free(co_node);
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
		StackCoEvent_t* e = pod_container_of(lcur, StackCoEvent_t, listnode);
		lnext = lcur->next;

		if (STACK_CO_EVENT_PROC == e->type) {
			sche->proc = e->proc;
			sche->proc_arg = e->proc_arg;
			free(e);
			do_fiber_switch(sche, sche->proc_fiber);
		}
		else if (STACK_CO_EVENT_TIMEOUT == e->type) {
			rbtimerAddEvent(&sche->timer, e->timeout_event);
		}
		else if (STACK_CO_EVENT_RESUME == e->type) {
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
			sche->resume_co_node = co_node;
			do_fiber_switch(sche, co_node->fiber);
			if (!sche->resume_co_node) {
				fiberFree(co_node->fiber);
			}
			sche->resume_co_node = NULL;
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
