//
// Created by hujianzhe on 16-5-2.
//

#include "../../c/syslib/assert.h"
#include "data_queue.h"

namespace Util {
DataQueue::DataQueue(void(*deleter)(list_node_t*)) :
	m_forcewakeup(false),
	m_deleter(deleter)
{
	assertTRUE(criticalsectionCreate(&m_cslock));
	assertTRUE(conditionvariableCreate(&m_condition));
	list_init(&m_datalist);
}
DataQueue::~DataQueue(void) {
	_clear();
	criticalsectionClose(&m_cslock);
	conditionvariableClose(&m_condition);
}

void DataQueue::push(list_node_t* data) {
	if (!data) {
		return;
	}

	criticalsectionEnter(&m_cslock);

	bool is_empty = !m_datalist.head;
	list_insert_node_back(&m_datalist, m_datalist.tail, data);
	if (is_empty) {
		conditionvariableSignal(&m_condition);
	}

	criticalsectionLeave(&m_cslock);
}

void DataQueue::push(list_t* list) {
	if (!list->head || !list->tail) {
		return;
	}

	criticalsectionEnter(&m_cslock);

	bool is_empty = !m_datalist.head;
	list_merge(&m_datalist, list);
	if (is_empty) {
		conditionvariableSignal(&m_condition);
	}

	criticalsectionLeave(&m_cslock);
}

list_node_t* DataQueue::pop(int msec, size_t expect_cnt) {
	list_node_t* res = NULL;
	if (0 == expect_cnt) {
		return res;
	}

	criticalsectionEnter(&m_cslock);

	while (!m_datalist.head && !m_forcewakeup) {
		if (conditionvariableWait(&m_condition, &m_cslock, msec)) {
			continue;
		}
		assertTRUE(errnoGet() == ETIMEDOUT);
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

	criticalsectionLeave(&m_cslock);

	return res;
}

void DataQueue::clear(void) {
	criticalsectionEnter(&m_cslock);
	_clear();
	criticalsectionLeave(&m_cslock);
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
	conditionvariableSignal(&m_condition);
}
}
