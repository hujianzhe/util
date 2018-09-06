//
// Created by hujianzhe on 18-8-24.
//

#ifndef	UTIL_C_COMPONENT_RBTIMER_H
#define	UTIL_C_COMPONENT_RBTIMER_H

#include "../datastruct/list.h"
#include "../datastruct/rbtree.h"
#include "../syslib/ipc.h"

typedef struct RBTimerEvent_t {
	long long timestamp_msec;
	int(*callback)(struct RBTimerEvent_t*, void*);
	void* arg;
	ListNode_t m_listnode;
} RBTimerEvent_t;

typedef struct RBTimer_t {
	char m_initok;
	CriticalSection_t m_lock;
	RBTree_t m_rbtree;
} RBTimer_t;

#ifdef __cplusplus
extern "C" {
#endif

UTIL_LIBAPI RBTimer_t* rbtimerInit(RBTimer_t* timer);
UTIL_LIBAPI long long rbtimerMiniumTimestamp(RBTimer_t* timer);
UTIL_LIBAPI int rbtimerAddEvent(RBTimer_t* timer, RBTimerEvent_t* e);
UTIL_LIBAPI void rbtimerCall(RBTimer_t* timer, long long timestamp_msec, void(*deleter)(RBTimerEvent_t*));
UTIL_LIBAPI void rbtimerClean(RBTimer_t* timer, void(*deleter)(RBTimerEvent_t*));
UTIL_LIBAPI void rbtimerDestroy(RBTimer_t* timer, void(*deleter)(RBTimerEvent_t*));

#ifdef __cplusplus
}
#endif

#endif
