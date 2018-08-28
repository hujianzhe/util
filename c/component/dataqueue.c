//
// Created by hujianzhe
//

#include "../syslib/assert.h"
#include "../syslib/error.h"
#include "dataqueue.h"

#ifdef __cplusplus
extern "C" {
#endif

DataQueue_t* dataqueueInit(DataQueue_t* dq) {
	dq->m_initok = 0;
	if (!criticalsectionCreate(&dq->m_cslock)) {
		return NULL;
	}
	if (!conditionvariableCreate(&dq->m_condition)) {
		criticalsectionClose(&dq->m_cslock);
		return NULL;
	}
	listInit(&dq->m_datalist);
	dq->m_forcewakeup = 0;
	dq->m_initok = 1;
	return dq;
}

void dataqueuePush(DataQueue_t* dq, ListNode_t* data) {
	int is_empty;
	if (!data) {
		return;
	}

	criticalsectionEnter(&dq->m_cslock);

	is_empty = !dq->m_datalist.head;
	listInsertNodeBack(&dq->m_datalist, dq->m_datalist.tail, data);
	if (is_empty) {
		conditionvariableSignal(&dq->m_condition);
	}

	criticalsectionLeave(&dq->m_cslock);
}

void dataqueuePushList(DataQueue_t* dq, List_t* list) {
	int is_empty;
	if (!list->head || !list->tail) {
		return;
	}

	criticalsectionEnter(&dq->m_cslock);

	is_empty = !dq->m_datalist.head;
	listMerge(&dq->m_datalist, list);
	if (is_empty) {
		conditionvariableSignal(&dq->m_condition);
	}

	criticalsectionLeave(&dq->m_cslock);
}

ListNode_t* dataqueuePop(DataQueue_t* dq, int msec, size_t expect_cnt) {
	ListNode_t* res = NULL;
	if (0 == expect_cnt) {
		return res;
	}

	criticalsectionEnter(&dq->m_cslock);

	while (!dq->m_datalist.head && !dq->m_forcewakeup) {
		if (conditionvariableWait(&dq->m_condition, &dq->m_cslock, msec)) {
			continue;
		}
		assertTRUE(errnoGet() == ETIMEDOUT);
		break;
	}
	dq->m_forcewakeup = 0;

	res = dq->m_datalist.head;
	if (~0 == expect_cnt) {
		listInit(&dq->m_datalist);
	}
	else {
		ListNode_t *cur;
		for (cur = dq->m_datalist.head; cur && --expect_cnt; cur = cur->next);
		if (0 == expect_cnt && cur && cur->next) {
			dq->m_datalist = listSplit(&dq->m_datalist, cur->next);
		}
		else {
			listInit(&dq->m_datalist);
		}
	}

	criticalsectionLeave(&dq->m_cslock);

	return res;
}

void dataqueueWake(DataQueue_t* dq) {
	dq->m_forcewakeup = 1;
	conditionvariableSignal(&dq->m_condition);
}

void dataqueueClean(DataQueue_t* dq, void(*deleter)(ListNode_t*)) {
	criticalsectionEnter(&dq->m_cslock);

	if (deleter) {
		ListNode_t* cur;
		for (cur = dq->m_datalist.head; cur; ) {
			ListNode_t* next = cur->next;
			deleter(cur);
			cur = next;
		}
	}
	listInit(&dq->m_datalist);

	criticalsectionLeave(&dq->m_cslock);
}

void dataqueueDestroy(DataQueue_t* dq, void(*deleter)(ListNode_t*)) {
	if (dq && dq->m_initok) {
		dataqueueClean(dq, deleter);
		criticalsectionClose(&dq->m_cslock);
		conditionvariableClose(&dq->m_condition);
	}
}

#ifdef __cplusplus
}
#endif
