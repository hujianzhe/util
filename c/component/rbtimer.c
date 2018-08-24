//
// Created by hujianzhe on 18-8-24.
//

#include "rbtimer.h"
#include <stdlib.h>

typedef struct RBTimerEvList {
	long long timestamp_msec;
	rbtree_node_t m_rbtreenode;
	list_t m_list;
} RBTimerEvList;

#ifdef __cplusplus
extern "C" {
#endif

static int rbtimer_keycmp(struct rbtree_node_t* node, const void* key) {
	RBTimerEvList* evlist = pod_container_of(node, RBTimerEvList, m_rbtreenode);
	long long res = *(long long*)key - evlist->timestamp_msec;
	if (res < 0)
		return -1;
	else if (res > 0)
		return 1;
	else
		return 0;
}

RBTimer_t* rbtimerInit(RBTimer_t* timer) {
	timer->m_initok = 0;
	if (!criticalsectionCreate(&timer->m_lock))
		return NULL;
	rbtree_init(&timer->m_rbtree, rbtimer_keycmp);
	timer->m_initok = 1;
	return timer;
}

long long rbtimerMiniumTimestamp(RBTimer_t* timer) {
	rbtree_node_t* node;
	long long min_timestamp = -1;

	criticalsectionEnter(&timer->m_lock);

	node = rbtree_first_node(&timer->m_rbtree);
	if (node) {
		RBTimerEvList* evlist = pod_container_of(node, RBTimerEvList, m_rbtreenode);
		min_timestamp = evlist->timestamp_msec;
	}

	criticalsectionLeave(&timer->m_lock);

	return min_timestamp;
}

int rbtimerAddEvent(RBTimer_t* timer, RBTimerEvent_t* e) {
	int ok = 0;
	RBTimerEvList* evlist;
	rbtree_node_t* exist_node;

	if (!e->callback || e->timestamp_msec < 0)
		return 1;

	criticalsectionEnter(&timer->m_lock);

	exist_node = rbtree_search_key(&timer->m_rbtree, &e->timestamp_msec);
	if (exist_node) {
		evlist = pod_container_of(exist_node, RBTimerEvList, m_rbtreenode);
		list_insert_node_back(&evlist->m_list, evlist->m_list.tail, &e->m_listnode);
		ok = 1;
	}
	else {
		evlist = (RBTimerEvList*)malloc(sizeof(RBTimerEvList));
		if (evlist) {
			evlist->m_rbtreenode.key = &evlist->timestamp_msec;
			evlist->timestamp_msec = e->timestamp_msec;
			list_init(&evlist->m_list);
			list_insert_node_back(&evlist->m_list, evlist->m_list.tail, &e->m_listnode);
			rbtree_insert_node(&timer->m_rbtree, &evlist->m_rbtreenode);
			ok = 1;
		}
	}

	criticalsectionLeave(&timer->m_lock);

	return ok;
}

void rbtimerCall(RBTimer_t* timer, long long timestamp_msec, void(*deleter)(RBTimerEvent_t*)) {
	list_node_t *lcur, *lnext;
	rbtree_node_t *rbcur, *rbnext;
	list_t list;
	list_init(&list);

	criticalsectionEnter(&timer->m_lock);

	for (rbcur = rbtree_first_node(&timer->m_rbtree); rbcur; rbcur = rbnext) {
		RBTimerEvList* evlist = pod_container_of(rbcur, RBTimerEvList, m_rbtreenode);
		if (timestamp_msec > 0 && evlist->timestamp_msec > timestamp_msec)
			break;
		rbnext = rbtree_next_node(rbcur);
		rbtree_remove_node(&timer->m_rbtree, rbcur);
		list_merge(&list, &evlist->m_list);
		free(evlist);
	}

	criticalsectionLeave(&timer->m_lock);

	for (lcur = list.head; lcur; lcur = lnext) {
		RBTimerEvent_t* e = pod_container_of(lcur, RBTimerEvent_t, m_listnode);
		lnext = lcur->next;
		if (e->callback(e, e->arg))
			rbtimerAddEvent(timer, e);
		else if (deleter)
			deleter(e);
	}
}

void rbtimerClean(RBTimer_t* timer, void(*deleter)(RBTimerEvent_t*)) {
	rbtree_node_t *rbcur, *rbnext;
	list_t list;
	list_init(&list);

	criticalsectionEnter(&timer->m_lock);

	for (rbcur = rbtree_first_node(&timer->m_rbtree); rbcur; rbcur = rbnext) {
		RBTimerEvList* evlist = pod_container_of(rbcur, RBTimerEvList, m_rbtreenode);
		rbnext = rbtree_next_node(rbcur);
		list_merge(&list, &evlist->m_list);
		free(evlist);
	}
	rbtree_init(&timer->m_rbtree, rbtimer_keycmp);

	criticalsectionLeave(&timer->m_lock);

	if (deleter) {
		list_node_t *cur, *next;
		for (cur = list.head; cur; cur = next) {
			next = cur->next;
			deleter(pod_container_of(cur, RBTimerEvent_t, m_listnode));
		}
	}
}

void rbtimerDestroy(RBTimer_t* timer, void(*deleter)(RBTimerEvent_t*)) {
	if (timer && timer->m_initok) {
		rbtimerClean(timer, deleter);
		criticalsectionClose(&timer->m_lock);
	}
}

#ifdef __cplusplus
}
#endif
