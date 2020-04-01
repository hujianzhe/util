//
// Created by hujianzhe
//

#include "../../inc/sysapi/assert.h"
#include "../../inc/sysapi/error.h"
#include "../../inc/component/dataqueue.h"

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
	listAppend(&dq->m_datalist, list);
	if (is_empty) {
		conditionvariableSignal(&dq->m_condition);
	}

	criticalsectionLeave(&dq->m_cslock);
}

ListNode_t* dataqueuePopWait(DataQueue_t* dq, int msec, size_t expect_cnt) {
	ListNode_t* res;
	if (0 == expect_cnt) {
		return NULL;
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
		if (cur)
			res = listSplitByTail(&dq->m_datalist, cur).head;
		else
			listInit(&dq->m_datalist);
	}

	criticalsectionLeave(&dq->m_cslock);

	return res;
}

void dataqueueWake(DataQueue_t* dq) {
	if (criticalsectionTryEnter(&dq->m_cslock)) {
		dq->m_forcewakeup = 1;
		conditionvariableSignal(&dq->m_condition);
		criticalsectionLeave(&dq->m_cslock);
	}
}

ListNode_t* dataqueueClean(DataQueue_t* dq) {
	List_t list;
	listInit(&list);

	criticalsectionEnter(&dq->m_cslock);

	list = dq->m_datalist;
	listInit(&dq->m_datalist);

	criticalsectionLeave(&dq->m_cslock);

	return list.head;
}

ListNode_t* dataqueueDestroy(DataQueue_t* dq) {
	if (dq && dq->m_initok) {
		ListNode_t* head = dataqueueClean(dq);
		criticalsectionClose(&dq->m_cslock);
		conditionvariableClose(&dq->m_condition);
		return head;
	}
	return NULL;
}

#ifdef __cplusplus
}
#endif
