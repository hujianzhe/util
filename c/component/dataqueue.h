//
// Created by hujianzhe
//

#ifndef	UTIL_C_COMPONENT_DATAQUEUE_H
#define	UTIL_C_COMPONENT_DATAQUEUE_H

#include "../datastruct/list.h"
#include "../syslib/ipc.h"

typedef struct DataQueue_t {
	CriticalSection_t m_cslock;
	ConditionVariable_t m_condition;
	list_t m_datalist;
	volatile char m_forcewakeup;
	char m_initok;
} DataQueue_t;

#ifdef __cplusplus
extern "C" {
#endif

DataQueue_t* dataqueueInit(DataQueue_t* dq);
void dataqueuePush(DataQueue_t* dq, list_node_t* data);
void dataqueuePushList(DataQueue_t* dq, list_t* list);
list_node_t* dataqueuePop(DataQueue_t* dq, int msec, size_t expect_cnt);
void dataqueueWake(DataQueue_t* dq);
void dataqueueClean(DataQueue_t* dq, void(*deleter)(list_node_t*));
void dataqueueDestroy(DataQueue_t* dq, void(*deleter)(list_node_t*));

#ifdef __cplusplus
}
#endif

#endif
