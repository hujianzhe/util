//
// Created by hujianzhe on 17-2-22.
//

#include "../c/syslib/time.h"
#include "timer_manager.h"

namespace Util {
TimerManager::TimerManager(void(*deleter)(list_node_t*)) :
	m_deleter(deleter)
{
	mutex_Create(&m_mutex);
}
TimerManager::~TimerManager(void) {
	if (m_deleter) {
		for (auto& it : m_tasks) {
			list_node_t *next, *cur;
			for (cur = it.second.first; cur; cur = next) {
				next = cur->next;
				m_deleter(cur);
			}
		}
	}
	m_tasks.clear();
	mutex_Close(&m_mutex);
}

long long TimerManager::minTimestamp(void) {
	long long min_msec = 0;
	mutex_Lock(&m_mutex);
	if (!m_tasks.empty()) {
		min_msec = m_tasks.begin()->first;
	}
	mutex_Unlock(&m_mutex);
	return min_msec;
}

void TimerManager::reg(long long timestamp_msec, list_node_t* task_node) {
	list_node_init(task_node);
	mutex_Lock(&m_mutex);
	auto& list = m_tasks[timestamp_msec];
	if (list.second) {
		list_node_insert_back(list.second, task_node);
		list.second = task_node;
	}
	else {
		list.first = list.second = task_node;
	}
	mutex_Unlock(&m_mutex);
}

list_node_t* TimerManager::expire(long long timestamp_msec) {
	list_node_t *head = NULL, *tail = NULL;
	mutex_Lock(&m_mutex);
	for (auto it = m_tasks.begin(); it != m_tasks.end() && it->first <= timestamp_msec; m_tasks.erase(it++)) {
		auto& list = it->second;
		if (tail) {
			list_node_merge(tail, list.first);
		}
		else {
			head = list.first;
		}
		tail = list.second;
	}
	mutex_Unlock(&m_mutex);
	return head;
}
}
