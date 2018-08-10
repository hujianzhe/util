//
// Created by hujianzhe on 17-2-22.
//

#include "../../c/syslib/assert.h"
#include "../../c/syslib/time.h"
#include "timer_manager.h"

namespace Util {
TimerManager::TimerManager(void(*deleter)(list_node_t*)) :
	m_deleter(deleter)
{
	assertTRUE(mutexCreate(&m_mutex));
}
TimerManager::~TimerManager(void) {
	if (m_deleter) {
		for (task_iter it = m_tasks.begin(); it != m_tasks.end(); ++it) {
			for (list_node_t* cur = it->second.head; cur; ) {
				list_node_t* next = cur->next;
				m_deleter(cur);
				cur = next;
			}
		}
	}
	m_tasks.clear();
	mutexClose(&m_mutex);
}

long long TimerManager::minTimestamp(void) {
	long long min_msec = 0;
	mutexLock(&m_mutex);
	if (!m_tasks.empty()) {
		min_msec = m_tasks.begin()->first;
	}
	mutexUnlock(&m_mutex);
	return min_msec;
}

void TimerManager::reg(long long timestamp_msec, list_node_t* task_node) {
	mutexLock(&m_mutex);
	list_t& list = m_tasks[timestamp_msec];
	list_insert_node_back(&list, list.tail, task_node);
	mutexUnlock(&m_mutex);
}

list_node_t* TimerManager::expire(long long timestamp_msec) {
	list_t to_list;
	list_init(&to_list);
	mutexLock(&m_mutex);
	for (task_iter it = m_tasks.begin(); it != m_tasks.end() && it->first <= timestamp_msec; m_tasks.erase(it++)) {
		list_merge(&to_list, &it->second);
	}
	mutexUnlock(&m_mutex);
	return to_list.head;
}
}
