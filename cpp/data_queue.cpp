//
// Created by hujianzhe on 16-5-2.
//

#include "data_queue.h"

namespace Util {
DataQueue::DataQueue(void(*deleter)(list_node_t*)) :
	m_forcewakeup(false),
	m_deleter(deleter)
{
	cslock_Create(&m_cslock);
	condition_Create(&m_condition);
	list_init(&m_datalist);
}
DataQueue::~DataQueue(void) {
	_clear();
	cslock_Close(&m_cslock);
	condition_Close(&m_condition);
}

void DataQueue::push(list_node_t* data) {
	if (!data) {
		return;
	}

	cslock_Enter(&m_cslock);

	bool is_empty = !m_datalist.head;
	list_insert_node_back(&m_datalist, m_datalist.tail, data);
	if (is_empty) {
		condition_WakeThread(&m_condition);
	}

	cslock_Leave(&m_cslock);
}

list_node_t* DataQueue::pop(int msec, size_t expect_cnt) {
	list_node_t* res = NULL;
	if (0 == expect_cnt) {
		return res;
	}

	cslock_Enter(&m_cslock);

	while (!m_datalist.head && !m_forcewakeup) {
		if (condition_Wait(&m_condition, &m_cslock, msec) == EXEC_SUCCESS) {
			continue;
		}
		assert_true(error_code() == ETIMEDOUT);
		break;
	}
	m_forcewakeup = false;

	res = m_datalist.head;
	if (~0 == expect_cnt) {
		list_init(&m_datalist);
	}
	else {
		list_node_t *cur;
		for (cur = m_datalist.head; cur && --expect_cnt; cur = cur->next);
		if (0 == expect_cnt && cur && cur->next) {
			m_datalist = list_split(&m_datalist, cur->next);
		}
		else {
			list_init(&m_datalist);
		}
	}

	cslock_Leave(&m_cslock);

	return res;
}

void DataQueue::clear(void) {
	cslock_Enter(&m_cslock);
	_clear();
	cslock_Leave(&m_cslock);
}
void DataQueue::_clear(void) {
	if (m_deleter) {
		for (list_node_t* cur = m_datalist.head; cur; ) {
			list_node_t* next = cur->next;
			m_deleter(cur);
			cur = next;
		}
	}
	list_init(&m_datalist);
}

void DataQueue::weakup(void) {
	m_forcewakeup = true;
	condition_WakeThread(&m_condition);
}
}
