//
// Created by hujianzhe on 18-8-24.
//

#include "../../inc/component/rbtimer.h"
#include <stdlib.h>

typedef struct RBTimerEvList {
	long long timestamp_msec;
	RBTreeNode_t m_rbtreenode;
	List_t m_list;
} RBTimerEvList;

#ifdef __cplusplus
extern "C" {
#endif

RBTimer_t* rbtimerInit(RBTimer_t* timer) {
	rbtreeInit(&timer->m_rbtree, rbtreeDefaultKeyCmpI64);
	timer->m_min_timestamp = -1;
	return timer;
}

long long rbtimerMiniumTimestamp(RBTimer_t* timer) {
	return timer->m_min_timestamp;
}

RBTimer_t* rbtimerDueFirst(RBTimer_t* timers[], size_t timer_cnt, long long* min_timestamp) {
	RBTimer_t* timer_result = NULL;
	long long min_timestamp_result = -1;
	size_t i;
	for (i = 0; i < timer_cnt; ++i) {
		long long this_timeout_ts = rbtimerMiniumTimestamp(timers[i]);
		if (this_timeout_ts < 0) {
			continue;
		}
		if (!timer_result || min_timestamp_result > this_timeout_ts) {
			timer_result = timers[i];
			min_timestamp_result = this_timeout_ts;
		}
	}
	if (min_timestamp) {
		*min_timestamp = min_timestamp_result;
	}
	return timer_result;
}

RBTimerEvent_t* rbtimerNewEvent(long long timestamp_msec, long long interval_msec, int(*callback)(RBTimer_t*, RBTimerEvent_t*), void* arg) {
	RBTimerEvent_t* e;
	if (!callback || interval_msec < 0) {
		return NULL;
	}
	e = (RBTimerEvent_t*)malloc(sizeof(RBTimerEvent_t));
	if (!e) {
		return NULL;
	}
	e->timestamp_msec = timestamp_msec;
	e->interval_msec = interval_msec;
	e->callback = callback;
	e->arg = arg;
	e->m_timer = NULL;
	return e;
}

RBTimerEvent_t* rbtimerAddEvent(RBTimer_t* timer, RBTimerEvent_t* e) {
	RBTimerEvList* evlist;
	RBTreeNode_t* exist_node;
	RBTreeNodeKey_t key;

	key.i64 = e->timestamp_msec;
	exist_node = rbtreeSearchKey(&timer->m_rbtree, key);
	if (exist_node) {
		evlist = pod_container_of(exist_node, RBTimerEvList, m_rbtreenode);
	}
	else {
		evlist = (RBTimerEvList*)malloc(sizeof(RBTimerEvList));
		if (!evlist) {
			return NULL;
		}
		evlist->m_rbtreenode.key.i64 = e->timestamp_msec;
		evlist->timestamp_msec = e->timestamp_msec;
		listInit(&evlist->m_list);
		rbtreeInsertNode(&timer->m_rbtree, &evlist->m_rbtreenode);
	}
	listInsertNodeBack(&evlist->m_list, evlist->m_list.tail, &e->m_listnode);
	if (timer->m_min_timestamp > e->timestamp_msec || timer->m_min_timestamp < 0) {
		timer->m_min_timestamp = e->timestamp_msec;
	}
	e->m_timer = timer;
	return e;
}

static void rbtimer_detach_event(RBTimerEvent_t* e) {
	RBTimerEvList* evlist;
	RBTreeNode_t* exist_node;
	RBTreeNodeKey_t key;
	RBTimer_t* timer;

	timer = e->m_timer;
	e->m_timer = NULL;
	key.i64 = e->timestamp_msec;
	exist_node = rbtreeSearchKey(&timer->m_rbtree, key);
	if (!exist_node) {
		return;
	}
	evlist = pod_container_of(exist_node, RBTimerEvList, m_rbtreenode);
	listRemoveNode(&evlist->m_list, &e->m_listnode);
	if (!listIsEmpty(&evlist->m_list)) {
		return;
	}
	rbtreeRemoveNode(&timer->m_rbtree, exist_node);
	free(evlist);

	exist_node = rbtreeFirstNode(&timer->m_rbtree);
	if (exist_node) {
		RBTimerEvList* evlist2 = pod_container_of(exist_node, RBTimerEvList, m_rbtreenode);
		timer->m_min_timestamp = evlist2->timestamp_msec;
	}
	else {
		timer->m_min_timestamp = -1;
	}
}

void rbtimerDelEvent(RBTimerEvent_t* e) {
	if (e->m_timer) {
		rbtimer_detach_event(e);
	}
	free(e);
}

RBTimerEvent_t* rbtimerTimeoutPopup(RBTimer_t* timer, long long timestamp_msec) {
	RBTreeNode_t *rbcur, *rbnext;
	ListNode_t* lnode;
	RBTimerEvList* evlist;
	RBTimerEvent_t* e;

	if (timer->m_min_timestamp < 0 || timer->m_min_timestamp > timestamp_msec) {
		return NULL;
	}
	rbcur = rbtreeFirstNode(&timer->m_rbtree);
	if (!rbcur) {
		return NULL;
	}
	evlist = pod_container_of(rbcur, RBTimerEvList, m_rbtreenode);
	lnode = listPopNodeFront(&evlist->m_list);
	if (listIsEmpty(&evlist->m_list)) {
		rbnext = rbtreeNextNode(rbcur);
		rbtreeRemoveNode(&timer->m_rbtree, rbcur);
		free(evlist);
		if (rbnext) {
			evlist = pod_container_of(rbnext, RBTimerEvList, m_rbtreenode);
			timer->m_min_timestamp = evlist->timestamp_msec;
		}
		else {
			timer->m_min_timestamp = -1;
		}
	}
	if (!lnode) {
		return NULL;
	}
	e = pod_container_of(lnode, RBTimerEvent_t, m_listnode);
	if (e) {
		e->m_timer = NULL;
	}
	return e;
}

ListNode_t* rbtimerClean(RBTimer_t* timer) {
	RBTreeNode_t *rbcur, *rbnext;
	List_t list;
	listInit(&list);

	for (rbcur = rbtreeFirstNode(&timer->m_rbtree); rbcur; rbcur = rbnext) {
		RBTimerEvList* evlist = pod_container_of(rbcur, RBTimerEvList, m_rbtreenode);
		rbnext = rbtreeNextNode(rbcur);
		rbtreeRemoveNode(&timer->m_rbtree, rbcur);
		listAppend(&list, &evlist->m_list);
		free(evlist);
	}
	rbtreeInit(&timer->m_rbtree, rbtreeDefaultKeyCmpI64);
	timer->m_min_timestamp = -1;

	return list.head;
}

ListNode_t* rbtimerDestroy(RBTimer_t* timer) {
	if (!timer) {
		return NULL;
	}
	return rbtimerClean(timer);
}

#ifdef __cplusplus
}
#endif
