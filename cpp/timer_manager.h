//
// Created by hujianzhe on 17-2-22.
//

#ifndef	UTIL_CPP_TIMER_MANAGER_H
#define	UTIL_CPP_TIMER_MANAGER_H

#include "../c/datastruct/list.h"
#include "../c/syslib/ipc.h"
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
	Mutex_t m_mutex;
	std::map<long long, std::pair<list_node_t*, list_node_t*> > m_tasks;//timestamp
};
}

#endif
