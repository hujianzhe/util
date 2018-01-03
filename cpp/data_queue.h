//
// Created by hujianzhe on 16-5-2.
//

#ifndef UTIL_CPP_DATA_QUEUE_H
#define UTIL_CPP_DATA_QUEUE_H

#include "../c/datastruct/list.h"
#include "../c/syslib/error.h"
#include "../c/syslib/ipc.h"

namespace Util {
class DataQueue {
public:
	DataQueue(void(*deleter)(list_node_t*) = NULL);
	virtual ~DataQueue(void);

	void push(list_node_t* data);
	list_node_t* pop(int msec, size_t expect_cnt = ~0);
	void clear(void);

	void weakup(void);

private:
	void _clear(void);

private:
	CSLock_t m_cslock;
	ConditionVariable_t m_condition;
	list_t m_datalist;
	bool m_forcewakeup;
	void(*m_deleter)(list_node_t*);
};
}

#endif
