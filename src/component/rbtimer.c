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

static void rbtimer_remove_evlist(RBTimer_t* timer, RBTimerEvList* evlist) {
	if (timer->m_first_evlist == evlist) {
		RBTreeNode_t* rb_next = rbtreeNextNode(&evlist->m_rbtreenode);
		if (rb_next) {
			RBTimerEvList* evlist_next = pod_container_of(rb_next, RBTimerEvList, m_rbtreenode);
			timer->m_first_evlist = evlist_next;
		}
		else {
			timer->m_first_evlist = NULL;
		}
	}
	rbtreeRemoveNode(&timer->m_rbtree, &evlist->m_rbtreenode);
}

#ifdef __cplusplus
extern "C" {
#endif

RBTimer_t* rbtimerInit(RBTimer_t* timer) {
	rbtreeInit(&timer->m_rbtree, rbtreeDefaultKeyCmpI64);
	timer->m_first_evlist = NULL;
	return timer;
}

long long rbtimerMiniumTimestamp(RBTimer_t* timer) {
	RBTimerEvList* first_evlist = (RBTimerEvList*)timer->m_first_evlist;
	return first_evlist ? first_evlist->timestamp : -1;
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

RBTimerEvent_t* rbtimerSetEvent(RBTimer_t* timer, RBTimerEvent_t* e, long long timestamp) {
	RBTimerEvList* evlist, *detach_old_evlist;
	RBTreeNode_t* exist_node;
	RBTreeNodeKey_t key;

	if (timestamp < 0) {
		return NULL;
	}
	detach_old_evlist = NULL;
	evlist = (RBTimerEvList*)e->m_internal_evlist;
	if (evlist) {
		if (e->m_timer != timer) {
			return NULL;
		}
		if (timestamp == evlist->timestamp) {
			return e;
		}
		detach_old_evlist = evlist;
	}
	key.i64 = timestamp;
	exist_node = rbtreeSearchKey(&timer->m_rbtree, key);
	if (exist_node) {
		if (detach_old_evlist) {
			listRemoveNode(&detach_old_evlist->m_list, &e->m_listnode);
			if (listIsEmpty(&detach_old_evlist->m_list)) {
				rbtimer_remove_evlist(timer, detach_old_evlist);
				free(detach_old_evlist);
			}
		}
		evlist = pod_container_of(exist_node, RBTimerEvList, m_rbtreenode);
		listInsertNodeBack(&evlist->m_list, evlist->m_list.tail, &e->m_listnode);
	}
	else {
		if (!detach_old_evlist) {
			evlist = (RBTimerEvList*)malloc(sizeof(RBTimerEvList));
			if (!evlist) {
				return NULL;
			}
			listInit(&evlist->m_list);
			listInsertNodeBack(&evlist->m_list, evlist->m_list.tail, &e->m_listnode);
		}
		else if (&e->m_listnode == detach_old_evlist->m_list.head &&
				 &e->m_listnode == detach_old_evlist->m_list.tail)
		{
			rbtimer_remove_evlist(timer, detach_old_evlist);
			evlist = detach_old_evlist;
		}
		else {
			evlist = (RBTimerEvList*)malloc(sizeof(RBTimerEvList));
			if (!evlist) {
				return NULL;
			}
			listInit(&evlist->m_list);
			listRemoveNode(&detach_old_evlist->m_list, &e->m_listnode);
			listInsertNodeBack(&evlist->m_list, evlist->m_list.tail, &e->m_listnode);
		}
		evlist->m_rbtreenode.key.i64 = timestamp;
		evlist->timestamp = timestamp;
		rbtreeInsertNode(&timer->m_rbtree, &evlist->m_rbtreenode);
		if (!timer->m_first_evlist || ((RBTimerEvList*)timer->m_first_evlist)->timestamp > timestamp) {
			timer->m_first_evlist = evlist;
		}
	}
	e->timestamp = timestamp;
	e->m_timer = timer;
	e->m_internal_evlist = evlist;
	return e;
}

BOOL rbtimerCheckEventScheduled(RBTimerEvent_t* e) { return e->m_internal_evlist != NULL; }

void rbtimerDetachEvent(RBTimerEvent_t* e) {
	RBTimerEvList* evlist;
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
	rbtimer_remove_evlist(timer, evlist);
	free(evlist);
}

RBTimerEvent_t* rbtimerTimeoutPopup(RBTimer_t* timer, long long timestamp) {
	ListNode_t* lnode;
	RBTimerEvList* evlist;
	RBTimerEvent_t* e;

	evlist = (RBTimerEvList*)timer->m_first_evlist;
	if (!evlist || evlist->timestamp > timestamp) {
		return NULL;
	}
	lnode = listPopNodeFront(&evlist->m_list);
	if (listIsEmpty(&evlist->m_list)) {
		rbtimer_remove_evlist(timer, evlist);
		free(evlist);
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
	timer->m_first_evlist = NULL;
}

#ifdef __cplusplus
}
#endif
