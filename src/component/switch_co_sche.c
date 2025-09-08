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

enum {
	SWITCH_CO_HDR_PROC = 1,
	SWITCH_CO_HDR_RESUME = 2,
};

typedef struct SwitchCoHdr_t {
	ListNode_t listnode;
	int type;
} SwitchCoHdr_t;

typedef struct SwitchCoNode_t {
	SwitchCoHdr_t hdr;
	SwitchCo_t co;
	RBTimerEvent_t* timeout_event;
	void(*proc)(struct SwitchCoSche_t*, SwitchCo_t*, void*);
	void* proc_arg;
	void(*fn_proc_arg_free)(void*);
	void(*fn_resume_ret_free)(void*);
	List_t childs_list;
	List_t childs_reuse_list;
	HashtableNode_t htnode;
	struct SwitchCoNode_t* parent;
	struct SwitchCoNode_t* root;
	int ready;
} SwitchCoNode_t;

typedef struct SwitchCoResume_t {
	SwitchCoHdr_t hdr;
	int co_id;
	int status;
	void* ret;
	void(*fn_ret_free)(void*);
} SwitchCoResume_t;

typedef struct SwitchCoSche_t {
	volatile int exit_flag;
	void* userdata;
	void(*fn_at_exit)(struct SwitchCoSche_t*, void*);
	void* fn_at_exit_arg;
	void(*fn_at_exit_arg_free)(void*);

	DataQueue_t dq;
	RBTimer_t timer;
	CriticalSection_t resume_lock;
	SwitchCoNode_t* exec_root_co;
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
	while (0 == (id = xadd32(&s_id, 1) + 1));
	return id;
}

static SwitchCoNode_t* alloc_switch_co_node(void) {
	SwitchCoNode_t* co_node = (SwitchCoNode_t*)calloc(1, sizeof(SwitchCoNode_t));
	if (!co_node) {
		return NULL;
	}
	listInit(&co_node->childs_list);
	listInit(&co_node->childs_reuse_list);
	return co_node;
}

static void free_switch_co_node(SwitchCoSche_t* sche, SwitchCoNode_t* co_node) {
	if (co_node->timeout_event) {
		rbtimerDetachEvent(co_node->timeout_event);
		free(co_node->timeout_event);
	}
	if (co_node->htnode.key.i32) {
		hashtableRemoveNode(&sche->block_co_htbl, &co_node->htnode);
	}
	if (co_node->fn_proc_arg_free) {
		co_node->fn_proc_arg_free(co_node->proc_arg);
	}
	if (co_node->fn_resume_ret_free) {
		co_node->fn_resume_ret_free(co_node->co.resume_ret);
	}
	if (co_node->co.fn_ctx_free) {
		co_node->co.fn_ctx_free(co_node->co.ctx);
	}
	free(co_node);
}

static void free_child_switch_co_nodes(SwitchCoSche_t* sche, SwitchCoNode_t* co_node) {
	ListNode_t *lcur, *lnext;
	for (lcur = co_node->childs_list.head; lcur; lcur = lnext) {
		SwitchCoNode_t* child_co_node = pod_container_of(lcur, SwitchCoNode_t, hdr.listnode);
		lnext = lcur->next;

		free_child_switch_co_nodes(sche, child_co_node);
		free_switch_co_node(sche, child_co_node);
	}
	for (lcur = co_node->childs_reuse_list.head; lcur; lcur = lnext) {
		SwitchCoNode_t* child_co_node = pod_container_of(lcur, SwitchCoNode_t, hdr.listnode);
		lnext = lcur->next;

		free_child_switch_co_nodes(sche, child_co_node);
		free_switch_co_node(sche, child_co_node);
	}
}

static void reset_switch_co_data(SwitchCo_t* co) {
	co->id = 0;
	co->status = SWITCH_STATUS_START;
	co->ctx = NULL;
	co->resume_ret = NULL;
}

static void reuse_switch_co_nodes(SwitchCoSche_t* sche, SwitchCoNode_t* top_parent_co_node, SwitchCoNode_t* co_node) {
	ListNode_t* lcur, *lnext;
	for (lcur = co_node->childs_list.head; lcur; lcur = lnext) {
		SwitchCoNode_t* child_co_node = pod_container_of(lcur, SwitchCoNode_t, hdr.listnode);
		lnext = lcur->next;
		reuse_switch_co_nodes(sche, top_parent_co_node, child_co_node);
	}

	if (co_node->timeout_event) {
		rbtimerDetachEvent(co_node->timeout_event);
	}
	if (co_node->htnode.key.i32) {
		hashtableRemoveNode(&sche->block_co_htbl, &co_node->htnode);
		co_node->htnode.key.i32 = 0;
	}
	if (co_node->co.fn_ctx_free) {
		co_node->co.fn_ctx_free(co_node->co.ctx);
		co_node->co.fn_ctx_free = NULL;
	}
	if (co_node->fn_resume_ret_free) {
		co_node->fn_resume_ret_free(co_node->co.resume_ret);
		co_node->fn_resume_ret_free = NULL;
	}

	listRemoveNode(&co_node->parent->childs_list, &co_node->hdr.listnode);
	listPushNodeBack(&top_parent_co_node->childs_reuse_list, &co_node->hdr.listnode);
	listAppend(&top_parent_co_node->childs_reuse_list, &co_node->childs_reuse_list);
}

static void empty_proc(SwitchCoSche_t* sche, SwitchCo_t* co, void* arg) {}

static void timer_sleep_callback(RBTimer_t* timer, struct RBTimerEvent_t* e) {
	SwitchCoNode_t* co_node = (SwitchCoNode_t*)e->arg;
	co_node->co.status = SWITCH_STATUS_FINISH;
	if (co_node->root) {
		co_node->root->ready = 1;
	}
}

static void timer_timeout_callback(RBTimer_t* timer, struct RBTimerEvent_t* e) {
	SwitchCoSche_t* sche = pod_container_of(timer, SwitchCoSche_t, timer);
	SwitchCoNode_t* co_node = (SwitchCoNode_t*)e->arg;
	listPushNodeFront(&sche->root_co_list, &co_node->hdr.listnode);
	co_node->ready = 1;
	co_node->timeout_event = NULL;
	free(e);
}

static void timer_block_callback(RBTimer_t* timer, struct RBTimerEvent_t* e) {
	SwitchCoSche_t* sche = pod_container_of(timer, SwitchCoSche_t, timer);
	SwitchCoNode_t* co_node = (SwitchCoNode_t*)e->arg;
	hashtableRemoveNode(&sche->block_co_htbl, &co_node->htnode);
	co_node->htnode.key.i32 = 0;
	co_node->co.status = SWITCH_STATUS_CANCEL;
	if (co_node->root) {
		co_node->root->ready = 1;
	}
}

SwitchCoSche_t* SwitchCoSche_new(void* userdata) {
	int dq_ok = 0, timer_ok = 0;
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
	sche->exit_flag = 0;
	sche->userdata = userdata;
	sche->fn_at_exit = NULL;
	sche->fn_at_exit_arg = NULL;
	sche->fn_at_exit_arg_free = NULL;
	sche->exec_root_co = NULL;
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
	free(sche);
	return NULL;
}

void SwitchCoSche_destroy(SwitchCoSche_t* sche) {
	ListNode_t *lcur, *lnext;

	for (lcur = dataqueueClean(&sche->dq); lcur; lcur = lnext) {
		SwitchCoHdr_t* hdr = pod_container_of(lcur, SwitchCoHdr_t, listnode);
		lnext = lcur->next;

		if (SWITCH_CO_HDR_PROC == hdr->type) {
			SwitchCoNode_t* co_node = pod_container_of(hdr, SwitchCoNode_t, hdr);
			free_switch_co_node(sche, co_node);
		}
		else if (SWITCH_CO_HDR_RESUME == hdr->type) {
			SwitchCoResume_t* e = pod_container_of(hdr, SwitchCoResume_t, hdr);
			free(e);
		}
		else {
			free(hdr);
		}
	}
	while (1) {
		SwitchCoNode_t* co_node;
		RBTimerEvent_t* e = rbtimerTimeoutPopup(&sche->timer, LLONG_MAX);
		if (!e) {
			break;
		}
		co_node = (SwitchCoNode_t*)e->arg;
		free_switch_co_node(sche, co_node);
	}
	rbtimerDestroy(&sche->timer);
	dataqueueDestroy(&sche->dq);
	free(sche);
}

SwitchCo_t* SwitchCoSche_root_function(SwitchCoSche_t* sche, void(*proc)(SwitchCoSche_t*, SwitchCo_t*, void*), void* arg, void(*fn_arg_free)(void*)) {
	SwitchCoNode_t* co_node = alloc_switch_co_node();
	if (!co_node) {
		return NULL;
	}
	co_node->hdr.type = SWITCH_CO_HDR_PROC;
	co_node->proc = proc;
	co_node->proc_arg = arg;
	co_node->fn_proc_arg_free = fn_arg_free;

	dataqueuePush(&sche->dq, &co_node->hdr.listnode);
	return &co_node->co;
}

SwitchCo_t* SwitchCoSche_timeout_util(SwitchCoSche_t* sche, long long tm_msec, void(*proc)(struct SwitchCoSche_t*, SwitchCo_t*, void*), void* arg, void(*fn_arg_free)(void*)) {
	SwitchCoNode_t* co_node;
	RBTimerEvent_t* e;

	if (tm_msec < 0) {
		return NULL;
	}
	e = (RBTimerEvent_t*)calloc(1, sizeof(RBTimerEvent_t));
	if (!e) {
		return NULL;
	}
	co_node = alloc_switch_co_node();
	if (!co_node) {
		free(e);
		return NULL;
	}
	co_node->hdr.type = SWITCH_CO_HDR_PROC;
	co_node->proc = proc;
	co_node->proc_arg = arg;
	co_node->fn_proc_arg_free = fn_arg_free;
	co_node->timeout_event = e;

	e->timestamp = tm_msec;
	e->interval = 0;
	e->callback = timer_timeout_callback;
	e->arg = co_node;

	dataqueuePush(&sche->dq, &co_node->hdr.listnode);
	return &co_node->co;
}

SwitchCo_t* SwitchCoSche_new_child_co(SwitchCoSche_t* sche, SwitchCo_t* parent_co) {
	SwitchCoNode_t* parent_co_node = pod_container_of(parent_co, SwitchCoNode_t, co);
	SwitchCoNode_t* co_node;
	if (listIsEmpty(&parent_co_node->childs_reuse_list)) {
		co_node = alloc_switch_co_node();
		if (!co_node) {
			return NULL;
		}
	}
	else {
		ListNode_t* listnode = listPopNodeBack(&parent_co_node->childs_reuse_list);
		co_node = pod_container_of(listnode, SwitchCoNode_t, hdr.listnode);
		reset_switch_co_data(&co_node->co);
	}
	co_node->parent = parent_co_node;
	co_node->root = sche->exec_root_co;
	co_node->proc = empty_proc;

	listPushNodeBack(&parent_co_node->childs_list, &co_node->hdr.listnode);
	return &co_node->co;
}

SwitchCo_t* SwitchCoSche_sleep_util(SwitchCoSche_t* sche, SwitchCo_t* parent_co, long long tm_msec) {
	SwitchCoNode_t* parent_co_node = pod_container_of(parent_co, SwitchCoNode_t, co);
	SwitchCoNode_t* co_node = NULL;
	RBTimerEvent_t* e = NULL;
	int co_node_alloc = 0, timeout_event_alloc = 0;

	if (listIsEmpty(&parent_co_node->childs_reuse_list)) {
		co_node = alloc_switch_co_node();
		if (!co_node) {
			goto err;
		}
		co_node_alloc = 1;
	}
	else {
		ListNode_t* listnode = listPopNodeBack(&parent_co_node->childs_reuse_list);
		co_node = pod_container_of(listnode, SwitchCoNode_t, hdr.listnode);
		reset_switch_co_data(&co_node->co);
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
	co_node->proc = empty_proc;
	co_node->parent = parent_co_node;
	co_node->root = sche->exec_root_co;

	if (e) {
		e->callback = timer_sleep_callback;
		e->arg = co_node;
		rbtimerSetEvent(&sche->timer, e, tm_msec);
	}

	listPushNodeBack(&parent_co_node->childs_list, &co_node->hdr.listnode);
	return &co_node->co;
err:
	if (co_node_alloc) {
		free(co_node);
	}
	else if (co_node) {
		listPushNodeBack(&parent_co_node->childs_reuse_list, &co_node->hdr.listnode);
	}
	if (timeout_event_alloc) {
		free(e);
	}
	return NULL;
}

SwitchCo_t* SwitchCoSche_block_point_util(SwitchCoSche_t* sche, SwitchCo_t* parent_co, long long tm_msec) {
	SwitchCoNode_t* parent_co_node = pod_container_of(parent_co, SwitchCoNode_t, co);
	SwitchCoNode_t* co_node = NULL;
	RBTimerEvent_t* e = NULL;
	int co_node_alloc = 0, timeout_event_alloc = 0;

	if (listIsEmpty(&parent_co_node->childs_reuse_list)) {
		co_node = alloc_switch_co_node();
		if (!co_node) {
			goto err;
		}
		co_node_alloc = 1;
	}
	else {
		ListNode_t* listnode = listPopNodeBack(&parent_co_node->childs_reuse_list);
		co_node = pod_container_of(listnode, SwitchCoNode_t, hdr.listnode);
		reset_switch_co_data(&co_node->co);
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
	co_node->proc = empty_proc;
	co_node->parent = parent_co_node;
	co_node->root = sche->exec_root_co;

	if (e) {
		e->callback = timer_block_callback;
		e->arg = co_node;
		rbtimerSetEvent(&sche->timer, e, tm_msec);
	}

	listPushNodeBack(&parent_co_node->childs_list, &co_node->hdr.listnode);
	return &co_node->co;
err:
	if (co_node_alloc) {
		free(co_node);
	}
	else if (co_node) {
		listPushNodeBack(&parent_co_node->childs_reuse_list, &co_node->hdr.listnode);
	}
	if (timeout_event_alloc) {
		free(e);
	}
	return NULL;
}

void SwitchCoSche_no_arg_free(SwitchCo_t* co) {
	SwitchCoNode_t* co_node = pod_container_of(co, SwitchCoNode_t, co);
	co_node->fn_proc_arg_free = NULL;
}

void* SwitchCoSche_pop_resume_ret(SwitchCo_t* co) {
	SwitchCoNode_t* co_node = pod_container_of(co, SwitchCoNode_t, co);
	void* ret = co->resume_ret;
	co->resume_ret = NULL;
	co_node->fn_resume_ret_free = NULL;
	return ret;
}

void SwitchCoSche_reuse_co(SwitchCoSche_t* sche, SwitchCo_t* co) {
	SwitchCoNode_t* co_node = pod_container_of(co, SwitchCoNode_t, co);
	if (!co_node->parent) {
		return;
	}
	reuse_switch_co_nodes(sche, co_node->parent, co_node);
	co_node->parent = NULL;
}

SwitchCo_t* SwitchCoSche_call_co(SwitchCoSche_t* sche, SwitchCo_t* co) {
	if (co->status >= 0) {
		SwitchCoNode_t* co_node = pod_container_of(co, SwitchCoNode_t, co);
		co_node->proc(sche, co, co_node->proc_arg);
		if (co->status < 0 && co->fn_ctx_free) {
			co->fn_ctx_free(co->ctx);
			co->fn_ctx_free = NULL;
		}
	}
	return co;
}

void SwitchCoSche_resume_co_by_id(SwitchCoSche_t* sche, int co_id, int status, void* ret, void(*fn_ret_free)(void*)) {
	SwitchCoResume_t* e;
	if (status >= 0) {
		return;
	}
	e = (SwitchCoResume_t*)malloc(sizeof(SwitchCoResume_t));
	if (!e) {
		return;
	}
	e->hdr.type = SWITCH_CO_HDR_RESUME;
	e->co_id = co_id;
	e->status = status;
	e->ret = ret;
	e->fn_ret_free = fn_ret_free;

	dataqueuePush(&sche->dq, &e->hdr.listnode);
}

int SwitchCoSche_sche(SwitchCoSche_t* sche, int idle_msec) {
	int i;
	long long wait_msec;
	long long cur_msec, min_t;
	ListNode_t *lcur, *lnext;

	if (sche->exit_flag) {
		for (lcur = sche->root_co_list.head; lcur; lcur = lnext) {
			SwitchCoNode_t* co_node = pod_container_of(lcur, SwitchCoNode_t, hdr.listnode);
			int status = co_node->co.status;
			lnext = lcur->next;

			if (status >= 0) {
				co_node->co.status = SWITCH_STATUS_CANCEL;
				if (status > 0) {
					sche->exec_root_co = co_node;
					co_node->proc(sche, &co_node->co, co_node->proc_arg);
				}
			}
			free_child_switch_co_nodes(sche, co_node);
			free_switch_co_node(sche, co_node);
		}
		listInit(&sche->root_co_list);

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
		SwitchCoHdr_t* hdr = pod_container_of(lcur, SwitchCoHdr_t, listnode);
		lnext = lcur->next;

		if (SWITCH_CO_HDR_PROC == hdr->type) {
			SwitchCoNode_t* co_node = pod_container_of(hdr, SwitchCoNode_t, hdr);
			RBTimerEvent_t* timeout_event = co_node->timeout_event;
			if (timeout_event) {
				rbtimerSetEvent(&sche->timer, timeout_event, timeout_event->timestamp);
				continue;
			}
			co_node->ready = 1;
			listPushNodeFront(&sche->root_co_list, lcur);
		}
		else if (SWITCH_CO_HDR_RESUME == hdr->type) {
			SwitchCoResume_t* co_resume = pod_container_of(hdr, SwitchCoResume_t, hdr);
			do {
				HashtableNodeKey_t key;
				HashtableNode_t* htnode;
				SwitchCoNode_t* co_node;

				key.i32 = co_resume->co_id;
				htnode = hashtableRemoveKey(&sche->block_co_htbl, key);
				if (!htnode) {
					if (co_resume->fn_ret_free) {
						co_resume->fn_ret_free(co_resume->ret);
					}
					break;
				}
				htnode->key.i32 = 0;
				co_node = pod_container_of(htnode, SwitchCoNode_t, htnode);
				if (co_node->timeout_event) {
					rbtimerDetachEvent(co_node->timeout_event);
				}
				if (co_node->co.status < 0) {
					if (co_resume->fn_ret_free) {
						co_resume->fn_ret_free(co_resume->ret);
					}
					break;
				}
				co_node->fn_resume_ret_free = co_resume->fn_ret_free;
				co_node->co.resume_ret = co_resume->ret;
				co_node->co.status = co_resume->status;
				if (co_node->root) {
					co_node->root->ready = 1;
				}
			} while (0);
			free(co_resume);
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
		SwitchCoNode_t* co_node = pod_container_of(lcur, SwitchCoNode_t, hdr.listnode);
		lnext = lcur->next;

		if (!co_node->ready) {
			continue;
		}
		co_node->ready = 0;
		sche->exec_root_co = co_node;
		co_node->proc(sche, &co_node->co, co_node->proc_arg);
		if (co_node->co.status < 0) {
			listRemoveNode(&sche->root_co_list, lcur);
			free_child_switch_co_nodes(sche, co_node);
			free_switch_co_node(sche, co_node);
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

void* SwitchCoSche_userdata(SwitchCoSche_t* sche) {
	return sche->userdata;
}

void SwitchCoSche_at_exit(SwitchCoSche_t* sche, void(*fn_at_exit)(SwitchCoSche_t*, void*), void* arg, void(*fn_arg_free)(void*)) {
	sche->fn_at_exit = fn_at_exit;
	sche->fn_at_exit_arg = arg;
	sche->fn_at_exit_arg_free = fn_arg_free;
}

#ifdef __cplusplus
}
#endif
