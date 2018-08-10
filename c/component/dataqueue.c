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
	if (!criticalsectionCreate(&dq->m_cslock)) {
		return NULL;
	}
	if (!conditionvariableCreate(&dq->m_condition)) {
		criticalsectionClose(&dq->m_cslock);
		return NULL;
	}
	list_init(&dq->m_datalist);
	dq->m_forcewakeup = 0;
	return dq;
}

void dataqueuePush(DataQueue_t* dq, list_node_t* data) {
	int is_empty;
	if (!data) {
		return;
	}

	criticalsectionEnter(&dq->m_cslock);

	is_empty = !dq->m_datalist.head;
	list_insert_node_back(&dq->m_datalist, dq->m_datalist.tail, data);
	if (is_empty) {
		conditionvariableSignal(&dq->m_condition);
	}

	criticalsectionLeave(&dq->m_cslock);
}

void dataqueuePushList(DataQueue_t* dq, list_t* list) {
	int is_empty;
	if (!list->head || !list->tail) {
		return;
	}

	criticalsectionEnter(&dq->m_cslock);

	is_empty = !dq->m_datalist.head;
	list_merge(&dq->m_datalist, list);
	if (is_empty) {
		conditionvariableSignal(&dq->m_condition);
	}

	criticalsectionLeave(&dq->m_cslock);
}

list_node_t* dataqueuePop(DataQueue_t* dq, int msec, size_t expect_cnt) {
	list_node_t* res = NULL;
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
		list_init(&dq->m_datalist);
	}
	else {
		list_node_t *cur;
		for (cur = dq->m_datalist.head; cur && --expect_cnt; cur = cur->next);
		if (0 == expect_cnt && cur && cur->next) {
			dq->m_datalist = list_split(&dq->m_datalist, cur->next);
		}
		else {
			list_init(&dq->m_datalist);
		}
	}

	criticalsectionLeave(&dq->m_cslock);

	return res;
}

void dataqueueWeak(DataQueue_t* dq) {
	dq->m_forcewakeup = 1;
	conditionvariableSignal(&dq->m_condition);
}

void dataqueueClear(DataQueue_t* dq, void(*deleter)(list_node_t*)) {
	if (deleter) {
		for (list_node_t* cur = dq->m_datalist.head; cur; ) {
			list_node_t* next = cur->next;
			deleter(cur);
			cur = next;
		}
	}
	list_init(&dq->m_datalist);
}

void dataqueueDestroy(DataQueue_t* dq, void(*deleter)(list_node_t*)) {
	dataqueueClear(dq, deleter);
	criticalsectionClose(&dq->m_cslock);
	conditionvariableClose(&dq->m_condition);
}

#ifdef __cplusplus
}
#endif
