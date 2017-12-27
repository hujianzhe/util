//
// Created by hujianzhe
//

#include "block_linked_queue.h"

#ifdef	__cplusplus
extern "C" {
#endif

void block_linked_queue_init(block_linked_queue_t* q, void(*deleter)(list_node_t*)) {
	q->m_head = q->m_tail = NULL;
	q->m_forcewakeup = FALSE;
	q->m_deleter = deleter;
	cslock_Create(&q->m_cslock);
	condition_Create(&q->m_condition);
}

void block_linked_queue_push(block_linked_queue_t* q, list_node_t* data) {
	if (!data) {
		return;
	}

	list_node_init(data);

	assert_true(cslock_Enter(&q->m_cslock, TRUE) == EXEC_SUCCESS);

	if (m_tail) {
		list_node_insert_back(q->m_tail, data);
		q->m_tail = data;
	}
	else {
		q->m_head = q->m_tail = data;
		assert_true(condition_WakeThread(&q->m_condition) == EXEC_SUCCESS);
	}

	assert_true(cslock_Leave(&q->m_cslock) == EXEC_SUCCESS);
}

list_node_t* block_linked_queue_pop(block_linked_queue_t* q, int msec, size_t expect_cnt) {
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
	q->m_forcewakeup = false;

	res = q->m_head;
	if (~0 == expect_cnt) {
		q->m_head = q->m_tail = NULL;
	}
	else {
		list_node_t *cur;
		for (cur = m_head; cur && --expect_cnt; cur = cur->next);
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

void block_linked_queue_weakup(block_linked_queue_t* q) {
	q->m_forcewakeup = true;
	assert_true(condition_WakeThread(&q->m_condition) == EXEC_SUCCESS);
}

static void __block_linked_queue_clear(block_linked_queue_t* q) {
	if (q->m_deleter) {
		list_node_t *next, *cur;
		for (cur = q->m_head; cur; cur = next) {
			next = cur->next;
			q->m_deleter(cur);
		}
	}
	q->m_head = q->m_tail = NULL;
}
void block_linked_queue_clear(block_linked_queue_t* q) {
	assert_true(cslock_Enter(&q->m_cslock, TRUE) == EXEC_SUCCESS);
	__block_linked_queue_clear(q);
	assert_true(cslock_Leave(&q->m_cslock) == EXEC_SUCCESS);
}

void block_linked_queue_destroy(block_linked_queue_t* q) {
	__block_linked_queue_clear(q);
	cslock_Close(&q->m_cslock);
	condition_Close(&q->m_condition);
}

#ifdef	__cplusplus
}
#endif
