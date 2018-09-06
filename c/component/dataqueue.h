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
	List_t m_datalist;
	volatile char m_forcewakeup;
	char m_initok;
} DataQueue_t;

#ifdef __cplusplus
extern "C" {
#endif

UTIL_LIBAPI DataQueue_t* dataqueueInit(DataQueue_t* dq);
UTIL_LIBAPI void dataqueuePush(DataQueue_t* dq, ListNode_t* data);
UTIL_LIBAPI void dataqueuePushList(DataQueue_t* dq, List_t* list);
UTIL_LIBAPI ListNode_t* dataqueuePop(DataQueue_t* dq, int msec, size_t expect_cnt);
UTIL_LIBAPI void dataqueueWake(DataQueue_t* dq);
UTIL_LIBAPI void dataqueueClean(DataQueue_t* dq, void(*deleter)(ListNode_t*));
UTIL_LIBAPI void dataqueueDestroy(DataQueue_t* dq, void(*deleter)(ListNode_t*));

#ifdef __cplusplus
}
#endif

#endif
