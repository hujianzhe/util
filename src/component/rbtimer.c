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

	if (e->timestamp_msec < 0)
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

void rbtimerDelEvent(RBTimer_t* timer, RBTimerEvent_t* e) {
	int need_free;
	RBTimerEvList* evlist;
	RBTreeNode_t* exist_node;

	if (timer->m_uselock)
		criticalsectionEnter(&timer->m_lock);

	exist_node = rbtreeSearchKey(&timer->m_rbtree, &e->timestamp_msec);
	if (exist_node) {
		evlist = pod_container_of(exist_node, RBTimerEvList, m_rbtreenode);
		listRemoveNode(&evlist->m_list, &e->m_listnode);
		if (evlist->m_list.head)
			need_free = 0;
		else {
			rbtreeRemoveNode(&timer->m_rbtree, exist_node);
			need_free = 1;
		}
	}
	else
		need_free = 0;

	if (timer->m_uselock)
		criticalsectionLeave(&timer->m_lock);

	if (need_free)
		free(evlist);
}

ListNode_t* rbtimerTimeout(RBTimer_t* timer, long long timestamp_msec) {
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

	return list.head;
}

ListNode_t* rbtimerClean(RBTimer_t* timer) {
	RBTreeNode_t *rbcur, *rbnext;
	List_t list;
	listInit(&list);

	if (timer->m_uselock)
		criticalsectionEnter(&timer->m_lock);

	for (rbcur = rbtreeFirstNode(&timer->m_rbtree); rbcur; rbcur = rbnext) {
		RBTimerEvList* evlist = pod_container_of(rbcur, RBTimerEvList, m_rbtreenode);
		rbnext = rbtreeNextNode(rbcur);
		rbtreeRemoveNode(&timer->m_rbtree, rbcur);
		listAppend(&list, &evlist->m_list);
		free(evlist);
	}
	rbtreeInit(&timer->m_rbtree, rbtimer_keycmp);

	if (timer->m_uselock)
		criticalsectionLeave(&timer->m_lock);

	return list.head;
}

ListNode_t* rbtimerDestroy(RBTimer_t* timer) {
	if (timer && timer->m_initok) {
		ListNode_t* head = rbtimerClean(timer);
		if (timer->m_uselock)
			criticalsectionClose(&timer->m_lock);
		return head;
	}
	return NULL;
}

#ifdef __cplusplus
}
#endif
