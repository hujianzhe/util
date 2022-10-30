//
// Created by hujianzhe
//

#include "../../inc/sysapi/time.h"
#include "../../inc/component/switch_co_sche.h"
#include "../../inc/component/dataqueue.h"
#include "../../inc/component/rbtimer.h"
#include <stdlib.h>
#include <limits.h>

typedef struct SwitchCoScheNode_t {
	SwitchCo_t co;
	RBTimerEvent_t* timeout_event;
	void(*proc)(struct SwitchCoSche_t*, SwitchCo_t*);
	ListNode_t _;
	struct SwitchCoScheNode_t* parent;
} SwitchCoScheNode_t;

typedef struct SwitchCoSche_t {
	DataQueue_t dq;
	RBTimer_t timer;
	List_t root_co_list;
	SwitchCoScheNode_t* cur_co_node;
} SwitchCoSche_t;

#ifdef __cplusplus
extern "C" {
#endif

static SwitchCoScheNode_t* SwitchCoSche_alloc_co_node(void) {
	return (SwitchCoScheNode_t*)calloc(1, sizeof(SwitchCoScheNode_t));
}

static void SwitchCoSche_free_co_node(SwitchCoScheNode_t* co_node) {
	if (co_node->timeout_event) {
		rbtimerDetachEvent(co_node->timeout_event);
		free(co_node->timeout_event);
		co_node->timeout_event = NULL;
	}
	free(co_node);
}

static void timer_sleep_callback(RBTimer_t* timer, struct RBTimerEvent_t* e) {
	SwitchCoScheNode_t* co_node = (SwitchCoScheNode_t*)e->arg;
	if (co_node->parent) {
		co_node->co.status = SWITCH_STATUS_FINISH;
		co_node->timeout_event = NULL;
	}
	else {
		free(co_node);
	}
	free(e);
}

static void timer_timeout_callback(RBTimer_t* timer, struct RBTimerEvent_t* e) {
	SwitchCoSche_t* sche = pod_container_of(timer, SwitchCoSche_t, timer);
	SwitchCoScheNode_t* co_node = (SwitchCoScheNode_t*)e->arg;
	co_node->timeout_event = NULL;
	listPushNodeFront(&sche->root_co_list, &co_node->_);
	free(e);
}

SwitchCoSche_t* SwitchCoSche_new(void) {
	int dq_ok = 0, timer_ok = 0;
	SwitchCoSche_t* sche = (SwitchCoSche_t*)malloc(sizeof(SwitchCoSche_t));
	if (!sche) {
		return NULL;
	}
	if (!dataqueueInit(&sche->dq)) {
		goto err;
	}
	if (!rbtimerInit(&sche->timer)) {
		goto err;
	}
	listInit(&sche->root_co_list);
	sche->cur_co_node = NULL;
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

void SwitchCoSche_destroy(SwitchCoSche_t* sche) {
	RBTimerEvent_t* e;
	ListNode_t *lcur, *lnext;
	for (lcur = sche->root_co_list.head; lcur; lcur = lnext) {
		SwitchCoScheNode_t* co_node = pod_container_of(lcur, SwitchCoScheNode_t, _);
		int status = co_node->co.status;
		lnext = lcur->next;

		if (status >= 0) {
			co_node->co.status = SWITCH_STATUS_CANCEL;
			if (status > 0 && co_node->proc) {
				co_node->proc(sche, &co_node->co);
			}
		}
		SwitchCoSche_free_co_node(co_node);
	}
	while (e = rbtimerTimeoutPopup(&sche->timer, LLONG_MAX)) {
		SwitchCoScheNode_t* co_node = (SwitchCoScheNode_t*)e->arg;
		SwitchCoSche_free_co_node(co_node);
	}
	rbtimerDestroy(&sche->timer);
	dataqueueDestroy(&sche->dq);
	free(sche);
}

SwitchCo_t* SwitchCoSche_sleep_msec(SwitchCoSche_t* sche, long long msec) {
	SwitchCoScheNode_t* co_node;
	RBTimerEvent_t* e = (RBTimerEvent_t*)calloc(1, sizeof(RBTimerEvent_t));
	if (!e) {
		return NULL;
	}
	co_node = SwitchCoSche_alloc_co_node();
	if (!co_node) {
		free(e);
		return NULL;
	}
	co_node->timeout_event = e;
	co_node->parent = sche->cur_co_node;

	e->timestamp = gmtimeMillisecond() + msec;
	e->interval = 0;
	e->callback = timer_sleep_callback;
	e->arg = co_node;

	if (!co_node->parent) {
		dataqueuePush(&sche->dq, &co_node->_);
	}
	else {
		rbtimerAddEvent(&sche->timer, co_node->timeout_event);
	}
	return &co_node->co;
}

SwitchCo_t* SwitchCoSche_timeout_msec(struct SwitchCoSche_t* sche, void(*proc)(struct SwitchCoSche_t*, SwitchCo_t*), long long msec, void* arg) {
	SwitchCoScheNode_t* co_node;
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

	dataqueuePush(&sche->dq, &co_node->_);
	return &co_node->co;
}

SwitchCo_t* SwitchCoSche_function(SwitchCoSche_t* sche, void(*proc)(SwitchCoSche_t*, SwitchCo_t*), void* arg, void* ret) {
	SwitchCoScheNode_t* co_node = SwitchCoSche_alloc_co_node();
	if (!co_node) {
		return NULL;
	}
	co_node->proc = proc;
	co_node->co.arg = arg;
	co_node->co.ret = ret;
	co_node->parent = sche->cur_co_node;

	if (!co_node->parent) {
		dataqueuePush(&sche->dq, &co_node->_);
	}
	return &co_node->co;
}

void SwitchCoSche_cancel_co(struct SwitchCoSche_t* sche, SwitchCo_t* co) {
	SwitchCoScheNode_t* co_node;
	int status = co->status;
	if (status < 0) {
		return;
	}
	co_node = pod_container_of(co, SwitchCoScheNode_t, co);
	if (co_node->timeout_event) {
		rbtimerDetachEvent(co_node->timeout_event);
		free(co_node->timeout_event);
		co_node->timeout_event = NULL;
	}
	co->status = SWITCH_STATUS_CANCEL;
	if (status > 0 && co_node->proc) {
		co_node->proc(sche, co);
	}
}

void SwitchCoSche_free_co(SwitchCo_t* co) {
	SwitchCoScheNode_t* co_node;
	if (!co) {
		return;
	}
	co_node = pod_container_of(co, SwitchCoScheNode_t, co);
	SwitchCoSche_free_co_node(co_node);
}

int SwitchCoSche_sche(SwitchCoSche_t* sche, int idle_msec) {
	int i;
	long long cur_msec;
	ListNode_t *lcur, *lnext;
	long long wait_msec;
	long long min_t = rbtimerMiniumTimestamp(&sche->timer);

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
		SwitchCoScheNode_t* co_node = pod_container_of(lcur, SwitchCoScheNode_t, _);
		lnext = lcur->next;

		if (co_node->timeout_event) {
			rbtimerAddEvent(&sche->timer, co_node->timeout_event);
		}
		else {
			listPushNodeFront(&sche->root_co_list, lcur);
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

	for (lcur = sche->root_co_list.head; lcur; lcur = lnext) {
		SwitchCoScheNode_t* co_node = pod_container_of(lcur, SwitchCoScheNode_t, _);
		lnext = lcur->next;

		sche->cur_co_node = co_node;
		co_node->proc(sche, &co_node->co);
		if (co_node->co.status < 0) {
			listRemoveNode(&sche->root_co_list, lcur);
			SwitchCoSche_free_co_node(co_node);
		}
	}
	sche->cur_co_node = NULL;

	return 0;
}

void SwitchCoSche_wake_up(struct SwitchCoSche_t* sche) {
	dataqueueWake(&sche->dq);
}

#ifdef __cplusplus
}
#endif
