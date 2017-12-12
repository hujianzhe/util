//
// Created by hujianzhe on 16-5-2.
//

#include "data_queue.h"

namespace Util {
DataQueue::DataQueue(void(*deleter)(list_node_t*)) :
	m_head(NULL),
	m_tail(NULL),
	m_forcewakeup(false),
	m_deleter(deleter)
{
	cslock_Create(&m_cslock);
	condition_Create(&m_condition);
}
DataQueue::~DataQueue(void) {
	if (m_deleter) {
		list_node_t *next, *cur;
		for (cur = m_head; cur; cur = next) {
			next = cur->next;
			m_deleter(cur);
		}
	}
	m_head = m_tail = NULL;
	cslock_Close(&m_cslock);
	condition_Close(&m_condition);
}

void DataQueue::push(list_node_t* data) {
	if (!data) {
		return;
	}

	list_node_init(data);

	assert_true(cslock_Enter(&m_cslock, TRUE) == EXEC_SUCCESS);

	if (m_tail) {
		list_node_insert_back(m_tail, data);
		m_tail = data;
	}
	else {
		m_head = m_tail = data;
		assert_true(condition_WakeThread(&m_condition) == EXEC_SUCCESS);
	}

	assert_true(cslock_Leave(&m_cslock) == EXEC_SUCCESS);
}

list_node_t* DataQueue::pop(int msec, size_t expect_cnt) {
	list_node_t* res = NULL;
	if (0 == expect_cnt) {
		return res;
	}

	assert_true(cslock_Enter(&m_cslock, TRUE) == EXEC_SUCCESS);
	while (NULL == m_head && !m_forcewakeup) {
		if (condition_Wait(&m_condition, &m_cslock, msec) == EXEC_SUCCESS) {
			continue;
		}
		assert_true(error_code() == ETIMEDOUT);
		break;
	}
	m_forcewakeup = false;

	res = m_head;
	if (~0 == expect_cnt) {
		m_head = m_tail = NULL;
	}
	else {
		list_node_t *cur;
		for (cur = m_head; cur && --expect_cnt; cur = cur->next);
		if (0 == expect_cnt && cur && cur->next) {
			m_head = cur->next;
		}
		else {
			m_head = m_tail = NULL;
		}
	}

	assert_true(cslock_Leave(&m_cslock) == EXEC_SUCCESS);

	return res;
}

void DataQueue::weakup(void) {
	m_forcewakeup = true;
	assert_true(condition_WakeThread(&m_condition) == EXEC_SUCCESS);
}
}