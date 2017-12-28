//
// Created by hujianzhe
//

#include "wait_queue.h"

#ifdef	__cplusplus
extern "C" {
#endif

WaitQueue_t* wait_queue_init(WaitQueue_t* q, void(*deleter)(list_node_t*)) {
	q->m_head = q->m_tail = NULL;
	q->m_forcewakeup = FALSE;
	q->m_deleter = deleter;
	cslock_Create(&q->m_cslock);
	condition_Create(&q->m_condition);
	return q;
}

void wait_queue_push(WaitQueue_t* q, list_node_t* data) {
	if (!data) {
		return;
	}

	list_node_init(data);

	assert_true(cslock_Enter(&q->m_cslock, TRUE) == EXEC_SUCCESS);

	if (q->m_tail) {
		list_node_insert_back(q->m_tail, data);
		q->m_tail = data;
	}
	else {
		q->m_head = q->m_tail = data;
		assert_true(condition_WakeThread(&q->m_condition) == EXEC_SUCCESS);
	}

	assert_true(cslock_Leave(&q->m_cslock) == EXEC_SUCCESS);
}

list_node_t* wait_queue_pop(WaitQueue_t* q, int msec, size_t expect_cnt) {
	list_node_t* res = NULL;
	if (0 == expect_cnt) {
		return res;
	}

	assert_true(cslock_Enter(&q->m_cslock, TRUE) == EXEC_SUCCESS);
	while (NULL == q->m_head && !q->m_forcewakeup) {
		if (condition_Wait(&q->m_condition, &q->m_cslock, msec) == EXEC_SUCCESS) {
			continue;
		}
		assert_true(error_code() == ETIMEDOUT);
		break;
	}
	q->m_forcewakeup = FALSE;

	res = q->m_head;
	if (~0 == expect_cnt) {
		q->m_head = q->m_tail = NULL;
	}
	else {
		list_node_t *cur;
		for (cur = q->m_head; cur && --expect_cnt; cur = cur->next);
		if (0 == expect_cnt && cur && cur->next) {
			q->m_head = cur->next;
		}
		else {
			q->m_head = q->m_tail = NULL;
		}
	}

	assert_true(cslock_Leave(&q->m_cslock) == EXEC_SUCCESS);

	return res;
}

void wait_queue_weakup(WaitQueue_t* q) {
	q->m_forcewakeup = TRUE;
	assert_true(condition_WakeThread(&q->m_condition) == EXEC_SUCCESS);
}

static void __wait_queue_clear(WaitQueue_t* q) {
	if (q->m_deleter) {
		list_node_t *next, *cur;
		for (cur = q->m_head; cur; cur = next) {
			next = cur->next;
			q->m_deleter(cur);
		}
	}
	q->m_head = q->m_tail = NULL;
}
void wait_queue_clear(WaitQueue_t* q) {
	assert_true(cslock_Enter(&q->m_cslock, TRUE) == EXEC_SUCCESS);
	__wait_queue_clear(q);
	assert_true(cslock_Leave(&q->m_cslock) == EXEC_SUCCESS);
}

void wait_queue_destroy(WaitQueue_t* q) {
	__wait_queue_clear(q);
	cslock_Close(&q->m_cslock);
	condition_Close(&q->m_condition);
}

#ifdef	__cplusplus
}
#endif
