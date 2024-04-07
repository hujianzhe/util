//
// Created by hujianzhe on 18-8-24.
//

#include "../../inc/component/rbtimer.h"
#include <stdlib.h>

typedef struct RBTimerEvList {
	long long timestamp;
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

RBTimerEvent_t* rbtimerAddEvent(RBTimer_t* timer, RBTimerEvent_t* e) {
	RBTimerEvList* evlist;
	RBTreeNode_t* exist_node;
	RBTreeNodeKey_t key;

	if (e->timestamp < 0 || e->interval < 0 || !e->callback) {
		return NULL;
	}
	if (e->m_timer) {
		return e->m_timer != timer ? NULL : e;
	}
	key.i64 = e->timestamp;
	exist_node = rbtreeSearchKey(&timer->m_rbtree, key);
	if (exist_node) {
		evlist = pod_container_of(exist_node, RBTimerEvList, m_rbtreenode);
	}
	else {
		evlist = (RBTimerEvList*)malloc(sizeof(RBTimerEvList));
		if (!evlist) {
			return NULL;
		}
		evlist->m_rbtreenode.key.i64 = e->timestamp;
		evlist->timestamp = e->timestamp;
		listInit(&evlist->m_list);
		rbtreeInsertNode(&timer->m_rbtree, &evlist->m_rbtreenode);
	}
	listInsertNodeBack(&evlist->m_list, evlist->m_list.tail, &e->m_listnode);
	if (timer->m_min_timestamp > e->timestamp || timer->m_min_timestamp < 0) {
		timer->m_min_timestamp = e->timestamp;
	}
	e->m_timer = timer;
	e->m_internal_evlist = evlist;
	return e;
}

BOOL rbtimerCheckEventScheduled(RBTimerEvent_t* e) { return e->m_timer != NULL; }

void rbtimerDetachEvent(RBTimerEvent_t* e) {
	RBTimerEvList* evlist;
	RBTreeNode_t* exist_node;
	RBTimer_t* timer;

	timer = e->m_timer;
	if (!timer) {
		return;
	}
	e->m_timer = NULL;
	evlist = (RBTimerEvList*)e->m_internal_evlist;
	if (!evlist) {
		return;
	}
	e->m_internal_evlist = NULL;
	listRemoveNode(&evlist->m_list, &e->m_listnode);
	if (!listIsEmpty(&evlist->m_list)) {
		return;
	}
	rbtreeRemoveNode(&timer->m_rbtree, &evlist->m_rbtreenode);
	free(evlist);

	exist_node = rbtreeFirstNode(&timer->m_rbtree);
	if (exist_node) {
		RBTimerEvList* evlist2 = pod_container_of(exist_node, RBTimerEvList, m_rbtreenode);
		timer->m_min_timestamp = evlist2->timestamp;
	}
	else {
		timer->m_min_timestamp = -1;
	}
}

RBTimerEvent_t* rbtimerTimeoutPopup(RBTimer_t* timer, long long timestamp) {
	RBTreeNode_t *rbcur;
	ListNode_t* lnode;
	RBTimerEvList* evlist;
	RBTimerEvent_t* e;

	if (timer->m_min_timestamp < 0 || timer->m_min_timestamp > timestamp) {
		return NULL;
	}
	rbcur = rbtreeFirstNode(&timer->m_rbtree);
	if (!rbcur) {
		return NULL;
	}
	evlist = pod_container_of(rbcur, RBTimerEvList, m_rbtreenode);
	lnode = listPopNodeFront(&evlist->m_list);
	if (listIsEmpty(&evlist->m_list)) {
		RBTreeNode_t* rbnext = rbtreeNextNode(rbcur);
		rbtreeRemoveNode(&timer->m_rbtree, rbcur);
		free(evlist);
		if (rbnext) {
			evlist = pod_container_of(rbnext, RBTimerEvList, m_rbtreenode);
			timer->m_min_timestamp = evlist->timestamp;
		}
		else {
			timer->m_min_timestamp = -1;
		}
	}
	if (!lnode) {
		return NULL;
	}
	e = pod_container_of(lnode, RBTimerEvent_t, m_listnode);
	e->m_timer = NULL;
	e->m_internal_evlist = NULL;
	return e;
}

void rbtimerDestroy(RBTimer_t* timer) {
	RBTreeNode_t *rbcur, *rbnext;
	for (rbcur = rbtreeFirstNode(&timer->m_rbtree); rbcur; rbcur = rbnext) {
		ListNode_t* lcur, *lnext;
		RBTimerEvList* evlist = pod_container_of(rbcur, RBTimerEvList, m_rbtreenode);
		rbnext = rbtreeNextNode(rbcur);
		rbtreeRemoveNode(&timer->m_rbtree, rbcur);
		for (lcur = evlist->m_list.head; lcur; lcur = lnext) {
			RBTimerEvent_t* e = pod_container_of(lcur, RBTimerEvent_t, m_listnode);
			lnext = lcur->next;
			e->m_timer = NULL;
			e->m_internal_evlist = NULL;
		}
		free(evlist);
	}
}

#ifdef __cplusplus
}
#endif
