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

static int rbtimer_keycmp(const void* node_key, const void* key) {
	long long res = *(const long long*)key - *(const long long*)node_key;
	if (res < 0)
		return -1;
	else if (res > 0)
		return 1;
	else
		return 0;
}

RBTimer_t* rbtimerInit(RBTimer_t* timer, BOOL uselock) {
	timer->m_initok = 0;
	if (uselock && !criticalsectionCreate(&timer->m_lock))
		return NULL;
	rbtreeInit(&timer->m_rbtree, rbtimer_keycmp);
	timer->m_initok = 1;
	timer->m_uselock = uselock;
	return timer;
}

long long rbtimerMiniumTimestamp(RBTimer_t* timer) {
	RBTreeNode_t* node;
	long long min_timestamp = -1;

	if (timer->m_uselock)
		criticalsectionEnter(&timer->m_lock);

	node = rbtreeFirstNode(&timer->m_rbtree);
	if (node) {
		RBTimerEvList* evlist = pod_container_of(node, RBTimerEvList, m_rbtreenode);
		min_timestamp = evlist->timestamp_msec;
	}

	if (timer->m_uselock)
		criticalsectionLeave(&timer->m_lock);

	return min_timestamp;
}

int rbtimerAddEvent(RBTimer_t* timer, RBTimerEvent_t* e) {
	int ok = 0;
	RBTimerEvList* evlist;
	RBTreeNode_t* exist_node;

	if (!e->callback || e->timestamp_msec < 0)
		return 1;

	if (timer->m_uselock)
		criticalsectionEnter(&timer->m_lock);

	exist_node = rbtreeSearchKey(&timer->m_rbtree, &e->timestamp_msec);
	if (exist_node) {
		evlist = pod_container_of(exist_node, RBTimerEvList, m_rbtreenode);
		listInsertNodeBack(&evlist->m_list, evlist->m_list.tail, &e->m_listnode);
		ok = 1;
	}
	else {
		evlist = (RBTimerEvList*)malloc(sizeof(RBTimerEvList));
		if (evlist) {
			evlist->m_rbtreenode.key = &evlist->timestamp_msec;
			evlist->timestamp_msec = e->timestamp_msec;
			listInit(&evlist->m_list);
			listInsertNodeBack(&evlist->m_list, evlist->m_list.tail, &e->m_listnode);
			rbtreeInsertNode(&timer->m_rbtree, &evlist->m_rbtreenode);
			ok = 1;
		}
	}

	if (timer->m_uselock)
		criticalsectionLeave(&timer->m_lock);

	return ok;
}

void rbtimerCall(RBTimer_t* timer, long long timestamp_msec, void(*deleter)(RBTimerEvent_t*)) {
	ListNode_t *lcur, *lnext;
	RBTreeNode_t *rbcur, *rbnext;
	List_t list;
	listInit(&list);

	if (timer->m_uselock)
		criticalsectionEnter(&timer->m_lock);

	for (rbcur = rbtreeFirstNode(&timer->m_rbtree); rbcur; rbcur = rbnext) {
		RBTimerEvList* evlist = pod_container_of(rbcur, RBTimerEvList, m_rbtreenode);
		if (timestamp_msec > 0 && evlist->timestamp_msec > timestamp_msec)
			break;
		rbnext = rbtreeNextNode(rbcur);
		rbtreeRemoveNode(&timer->m_rbtree, rbcur);
		listAppend(&list, &evlist->m_list);
		free(evlist);
	}

	if (timer->m_uselock)
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
	RBTreeNode_t *rbcur, *rbnext;
	List_t list;
	listInit(&list);

	if (timer->m_uselock)
		criticalsectionEnter(&timer->m_lock);

	for (rbcur = rbtreeFirstNode(&timer->m_rbtree); rbcur; rbcur = rbnext) {
		RBTimerEvList* evlist = pod_container_of(rbcur, RBTimerEvList, m_rbtreenode);
		rbnext = rbtreeNextNode(rbcur);
		listAppend(&list, &evlist->m_list);
		free(evlist);
	}
	rbtreeInit(&timer->m_rbtree, rbtimer_keycmp);

	if (timer->m_uselock)
		criticalsectionLeave(&timer->m_lock);

	if (deleter) {
		ListNode_t *cur, *next;
		for (cur = list.head; cur; cur = next) {
			next = cur->next;
			deleter(pod_container_of(cur, RBTimerEvent_t, m_listnode));
		}
	}
}

void rbtimerDestroy(RBTimer_t* timer, void(*deleter)(RBTimerEvent_t*)) {
	if (timer && timer->m_initok) {
		rbtimerClean(timer, deleter);
		if (timer->m_uselock)
			criticalsectionClose(&timer->m_lock);
	}
}

#ifdef __cplusplus
}
#endif
