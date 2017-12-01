//
// Created by hujianzhe on 17-2-22.
//

#ifndef	UTIL_TIMER_MANAGER_H
#define	UTIL_TIMER_MANAGER_H

#include "syslib/ipc.h"
#include "datastruct/list.h"
#include <list>
#include <map>

namespace Util {
class TimerManager {
public:
	TimerManager(void(*deleter)(list_node_t*) = NULL);
	~TimerManager(void);

	long long minTimestamp(void);

	void reg(long long timestamp_msec, list_node_t* task_node);
	list_node_t* expire(long long timestamp_msec);

private:
	void(*m_deleter)(list_node_t*);
	MUTEX m_mutex;
	std::map<long long, std::pair<list_node_t*, list_node_t*> > m_tasks;//timestamp
};
}

#endif
