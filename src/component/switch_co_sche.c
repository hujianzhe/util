//
// Created by hujianzhe
//

#include "../../inc/sysapi/atomic.h"
#include "../../inc/sysapi/time.h"
#include "../../inc/datastruct/hashtable.h"
#include "../../inc/component/switch_co_sche.h"
#include "../../inc/component/dataqueue.h"
#include "../../inc/component/rbtimer.h"
#include <stdlib.h>
#include <limits.h>

typedef struct SwitchCoNode_t {
	SwitchCo_t co;
	RBTimerEvent_t* timeout_event;
	void(*proc)(struct SwitchCoSche_t*, SwitchCo_t*);
	ListNode_t listnode;
	List_t childs_list;
	HashtableNode_t htnode;
	struct SwitchCoNode_t* parent;
} SwitchCoNode_t;

typedef struct SwitchResumeEvent_t {
	ListNode_t listnode;
	int co_id;
	void* ret;
} SwitchResumeEvent_t;

typedef struct SwitchCoSche_t {
	volatile int exit_flag;
	DataQueue_t dq;
	RBTimer_t timer;
	CriticalSection_t resume_lock;
	List_t resume_ev_list;
	List_t root_co_list;
	Hashtable_t block_co_htbl;
	HashtableNode_t* block_co_htbl_bulks[2048];
} SwitchCoSche_t;

#ifdef __cplusplus
extern "C" {
#endif

static int gen_co_id() {
	static Atom32_t s_id = 0;
	int id;
	while (0 == (id = _xadd32(&s_id, 1) + 1));
	return id;
}

static SwitchCoNode_t* SwitchCoSche_alloc_co_node(void) {
	SwitchCoNode_t* co_node = (SwitchCoNode_t*)calloc(1, sizeof(SwitchCoNode_t));
	if (!co_node) {
		return NULL;
	}
	listInit(&co_node->childs_list);
	return co_node;
}

static void SwitchCoSche_free_co_node(SwitchCoSche_t* sche, SwitchCoNode_t* co_node) {
	if (co_node->timeout_event) {
		rbtimerDetachEvent(co_node->timeout_event);
		free(co_node->timeout_event);
	}
	if (co_node->htnode.key.i32) {
		hashtableRemoveNode(&sche->block_co_htbl, &co_node->htnode);
	}
	free(co_node);
}

void SwitchCoSche_free_child_co_nodes(SwitchCoSche_t* sche, SwitchCoNode_t* co_node) {
	ListNode_t *lcur, *lnext;
	for (lcur = co_node->childs_list.head; lcur; lcur = lnext) {
		SwitchCoNode_t* child_co_node = pod_container_of(lcur, SwitchCoNode_t, listnode);
		lnext = lcur->next;

		SwitchCoSche_free_child_co_nodes(sche, child_co_node);
		SwitchCoSche_free_co_node(sche, child_co_node);
	}
	listInit(&co_node->childs_list);
}

static void empty_proc(SwitchCoSche_t* sche, SwitchCo_t* co) {}

static void timer_sleep_callback(RBTimer_t* timer, struct RBTimerEvent_t* e) {
	SwitchCoNode_t* co_node = (SwitchCoNode_t*)e->arg;
	co_node->co.status = SWITCH_STATUS_FINISH;
	co_node->timeout_event = NULL;
	free(e);
}

static void timer_timeout_callback(RBTimer_t* timer, struct RBTimerEvent_t* e) {
	SwitchCoSche_t* sche = pod_container_of(timer, SwitchCoSche_t, timer);
	SwitchCoNode_t* co_node = (SwitchCoNode_t*)e->arg;
	co_node->timeout_event = NULL;
	listPushNodeFront(&sche->root_co_list, &co_node->listnode);
	free(e);
}

static void timer_block_callback(RBTimer_t* timer, struct RBTimerEvent_t* e) {
	SwitchCoSche_t* sche = pod_container_of(timer, SwitchCoSche_t, timer);
	SwitchCoNode_t* co_node = (SwitchCoNode_t*)e->arg;
	co_node->co.status = SWITCH_STATUS_CANCEL;
	hashtableRemoveNode(&sche->block_co_htbl, &co_node->htnode);
	co_node->htnode.key.i32 = 0;
	co_node->timeout_event = NULL;
	free(e);
}

SwitchCoSche_t* SwitchCoSche_new(void) {
	int dq_ok = 0, timer_ok = 0, lock_ok = 0;
	SwitchCoSche_t* sche = (SwitchCoSche_t*)malloc(sizeof(SwitchCoSche_t));
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
	if (!criticalsectionCreate(&sche->resume_lock)) {
		goto err;
	}
	lock_ok = 1;
	sche->exit_flag = 0;
	listInit(&sche->resume_ev_list);
	listInit(&sche->root_co_list);
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
	if (lock_ok) {
		criticalsectionClose(&sche->resume_lock);
	}
	free(sche);
	return NULL;
}

void SwitchCoSche_destroy(SwitchCoSche_t* sche) {
	ListNode_t *lcur, *lnext;

	for (lcur = sche->resume_ev_list.head; lcur; lcur = lnext) {
		SwitchResumeEvent_t* e = pod_container_of(lcur, SwitchResumeEvent_t, listnode);
		lnext = lcur->next;
		free(e);
	}
	while (1) {
		SwitchCoNode_t* co_node;
		RBTimerEvent_t* e = rbtimerTimeoutPopup(&sche->timer, LLONG_MAX);
		if (!e) {
			break;
		}
		co_node = (SwitchCoNode_t*)e->arg;
		SwitchCoSche_free_co_node(sche, co_node);
	}
	rbtimerDestroy(&sche->timer);
	dataqueueDestroy(&sche->dq);
	criticalsectionClose(&sche->resume_lock);
	free(sche);
}

SwitchCo_t* SwitchCoSche_timeout_msec(struct SwitchCoSche_t* sche, long long msec, void(*proc)(struct SwitchCoSche_t*, SwitchCo_t*), void* arg) {
	SwitchCoNode_t* co_node;
	RBTimerEvent_t* e = (RBTimerEvent_t*)calloc(1, sizeof(RBTimerEvent_t));
	if (!e) {
		return NULL;
	}
	co_node = SwitchCoSche_alloc_co_node();
	if (!co_node) {
		free(e);
		return NULL;
	}
	co_node->proc = proc;
	co_node->co.arg = arg;
	co_node->timeout_event = e;

	e->timestamp = gmtimeMillisecond() + msec;
	e->interval = 0;
	e->callback = timer_timeout_callback;
	e->arg = co_node;

	dataqueuePush(&sche->dq, &co_node->listnode);
	return &co_node->co;
}

SwitchCo_t* SwitchCoSche_root_function(SwitchCoSche_t* sche, void(*proc)(SwitchCoSche_t*, SwitchCo_t*), void* arg, void* ret) {
	SwitchCoNode_t* co_node = SwitchCoSche_alloc_co_node();
	if (!co_node) {
		return NULL;
	}
	co_node->proc = proc;
	co_node->co.arg = arg;
	co_node->co.ret = ret;

	dataqueuePush(&sche->dq, &co_node->listnode);
	return &co_node->co;
}

SwitchCo_t* SwitchCoSche_new_child_co(SwitchCo_t* parent_co) {
	SwitchCoNode_t* co_node = SwitchCoSche_alloc_co_node();
	if (!co_node) {
		return NULL;
	}
	co_node->parent = pod_container_of(parent_co, SwitchCoNode_t, co);

	listPushNodeBack(&co_node->parent->childs_list, &co_node->listnode);
	return &co_node->co;
}

SwitchCo_t* SwitchCoSche_sleep_msec(SwitchCoSche_t* sche, SwitchCo_t* parent_co, long long msec) {
	SwitchCoNode_t* co_node;
	RBTimerEvent_t* e = (RBTimerEvent_t*)calloc(1, sizeof(RBTimerEvent_t));
	if (!e) {
		return NULL;
	}
	co_node = SwitchCoSche_alloc_co_node();
	if (!co_node) {
		free(e);
		return NULL;
	}
	co_node->proc = empty_proc;
	co_node->timeout_event = e;
	co_node->parent = pod_container_of(parent_co, SwitchCoNode_t, co);

	e->timestamp = gmtimeMillisecond() + msec;
	e->interval = 0;
	e->callback = timer_sleep_callback;
	e->arg = co_node;
	rbtimerAddEvent(&sche->timer, co_node->timeout_event);

	listPushNodeBack(&co_node->parent->childs_list, &co_node->listnode);
	return &co_node->co;
}

SwitchCo_t* SwitchCoSche_block_point(struct SwitchCoSche_t* sche, SwitchCo_t* parent_co, long long block_msec) {
	SwitchCoNode_t* co_node;
	RBTimerEvent_t* e;

	if (block_msec >= 0) {
		e = (RBTimerEvent_t*)calloc(1, sizeof(RBTimerEvent_t));
		if (!e) {
			return NULL;
		}
	}
	else {
		e = NULL;
	}
	co_node = SwitchCoSche_alloc_co_node();
	if (!co_node) {
		free(e);
		return NULL;
	}
	co_node->proc = empty_proc;
	co_node->timeout_event = e;
	co_node->parent = pod_container_of(parent_co, SwitchCoNode_t, co);
	co_node->co.id = gen_co_id();
	co_node->htnode.key.i32 = co_node->co.id;
	if (hashtableInsertNode(&sche->block_co_htbl, &co_node->htnode) != &co_node->htnode) {
		free(e);
		free(co_node);
		return NULL;
	}

	if (e) {
		e->timestamp = gmtimeMillisecond() + block_msec;
		e->interval = 0;
		e->callback = timer_block_callback;
		e->arg = co_node;
		rbtimerAddEvent(&sche->timer, co_node->timeout_event);
	}

	listPushNodeBack(&co_node->parent->childs_list, &co_node->listnode);
	return &co_node->co;
}

void SwitchCoSche_resume_co(SwitchCoSche_t* sche, int co_id, void* ret) {
	SwitchResumeEvent_t* e = (SwitchResumeEvent_t*)malloc(sizeof(SwitchResumeEvent_t));
	if (!e) {
		return;
	}
	e->co_id = co_id;
	e->ret = ret;

	criticalsectionEnter(&sche->resume_lock);
	listPushNodeBack(&sche->resume_ev_list, &e->listnode);
	criticalsectionLeave(&sche->resume_lock);

	dataqueueWake(&sche->dq);
}

void SwitchCoSche_cancel_co(struct SwitchCoSche_t* sche, SwitchCo_t* co) {
	SwitchCoNode_t* co_node;
	int status = co->status;
	if (status < 0) {
		return;
	}
	co_node = pod_container_of(co, SwitchCoNode_t, co);
	if (co_node->timeout_event) {
		rbtimerDetachEvent(co_node->timeout_event);
		free(co_node->timeout_event);
		co_node->timeout_event = NULL;
	}
	if (co_node->htnode.key.i32) {
		hashtableRemoveNode(&sche->block_co_htbl, &co_node->htnode);
		co_node->htnode.key.i32 = 0;
	}
	co->status = SWITCH_STATUS_CANCEL;
	if (status > 0) {
		co_node->proc(sche, co);
	}
}

void SwitchCoSche_free_child_co(SwitchCoSche_t* sche, SwitchCo_t* co) {
	ListNode_t *lcur, *lnext;
	SwitchCoNode_t* co_node;
	if (!co || co->status >= 0) {
		return;
	}
	co_node = pod_container_of(co, SwitchCoNode_t, co);
	SwitchCoSche_free_child_co_nodes(sche, co_node);
}

int SwitchCoSche_sche(SwitchCoSche_t* sche, int idle_msec) {
	int i;
	long long wait_msec;
	long long cur_msec, min_t;
	ListNode_t *lcur, *lnext;

	if (sche->exit_flag) {
		for (lcur = sche->root_co_list.head; lcur; lcur = lnext) {
			SwitchCoNode_t* co_node = pod_container_of(lcur, SwitchCoNode_t, listnode);
			int status = co_node->co.status;
			lnext = lcur->next;

			if (status >= 0) {
				co_node->co.status = SWITCH_STATUS_CANCEL;
				if (status > 0) {
					co_node->proc(sche, &co_node->co);
				}
			}
			SwitchCoSche_free_child_co_nodes(sche, co_node);
			SwitchCoSche_free_co_node(sche, co_node);
		}
		listInit(&sche->root_co_list);
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
		SwitchCoNode_t* co_node = pod_container_of(lcur, SwitchCoNode_t, listnode);
		lnext = lcur->next;

		if (co_node->timeout_event) {
			rbtimerAddEvent(&sche->timer, co_node->timeout_event);
		}
		else {
			listPushNodeFront(&sche->root_co_list, lcur);
		}
	}

	criticalsectionEnter(&sche->resume_lock);
	lcur = sche->resume_ev_list.head;
	listInit(&sche->resume_ev_list);
	criticalsectionLeave(&sche->resume_lock);
	for (; lcur; lcur = lnext) {
		SwitchResumeEvent_t* e = pod_container_of(lcur, SwitchResumeEvent_t, listnode);
		lnext = lcur->next;

		do {
			HashtableNodeKey_t key;
			HashtableNode_t* htnode;
			SwitchCoNode_t* co_node;

			key.i32 = e->co_id;
			htnode = hashtableRemoveKey(&sche->block_co_htbl, key);
			if (!htnode) {
				break;
			}
			co_node = pod_container_of(htnode, SwitchCoNode_t, htnode);
			co_node->htnode.key.i32 = 0;
			if (co_node->timeout_event) {
				rbtimerDetachEvent(co_node->timeout_event);
				free(co_node->timeout_event);
				co_node->timeout_event = NULL;
			}
			if (co_node->co.status < 0) {
				break;
			}
			co_node->co.status = SWITCH_STATUS_FINISH;
			co_node->co.ret = e->ret;
		} while (0);

		free(e);
	}

	cur_msec = gmtimeMillisecond();
	for (i = 0; i < 100; ++i) {
		RBTimerEvent_t* e = rbtimerTimeoutPopup(&sche->timer, cur_msec);
		if (!e) {
			break;
		}
		e->callback(&sche->timer, e);
	}

	for (lcur = sche->root_co_list.head; lcur; lcur = lnext) {
		SwitchCoNode_t* co_node = pod_container_of(lcur, SwitchCoNode_t, listnode);
		lnext = lcur->next;

		co_node->proc(sche, &co_node->co);
		if (co_node->co.status < 0) {
			listRemoveNode(&sche->root_co_list, lcur);
			SwitchCoSche_free_child_co_nodes(sche, co_node);
			SwitchCoSche_free_co_node(sche, co_node);
		}
	}

	return 0;
}

void SwitchCoSche_wake_up(SwitchCoSche_t* sche) {
	dataqueueWake(&sche->dq);
}

void SwitchCoSche_exit(SwitchCoSche_t* sche) {
	sche->exit_flag = 1;
	dataqueueWake(&sche->dq);
}

#ifdef __cplusplus
}
#endif
