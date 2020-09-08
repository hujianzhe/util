//
// Created by hujianzhe on 18-8-24.
//

#ifndef	UTIL_C_COMPONENT_RBTIMER_H
#define	UTIL_C_COMPONENT_RBTIMER_H

#include "../datastruct/list.h"
#include "../datastruct/rbtree.h"
#include "../sysapi/ipc.h"

typedef struct RBTimer_t {
	char m_initok;
	char m_uselock;
	CriticalSection_t m_lock;
	RBTree_t m_rbtree;
} RBTimer_t;

typedef struct RBTimerEvent_t {
	ListNode_t m_listnode;
	long long timestamp_msec;
	int(*callback)(RBTimer_t*, struct RBTimerEvent_t*);
	void* arg;
} RBTimerEvent_t;

#ifdef __cplusplus
extern "C" {
#endif

__declspec_dll RBTimer_t* rbtimerInit(RBTimer_t* timer, BOOL uselock);
__declspec_dll long long rbtimerMiniumTimestamp(RBTimer_t* timer);
__declspec_dll RBTimer_t* rbtimerDueFirst(RBTimer_t* timers[], size_t timer_cnt, long long* min_timestamp);
__declspec_dll int rbtimerAddEvent(RBTimer_t* timer, RBTimerEvent_t* e);
__declspec_dll void rbtimerDelEvent(RBTimer_t* timer, RBTimerEvent_t* e);
__declspec_dll ListNode_t* rbtimerTimeout(RBTimer_t* timer, long long timestamp_msec);
__declspec_dll ListNode_t* rbtimerClean(RBTimer_t* timer);
__declspec_dll ListNode_t* rbtimerDestroy(RBTimer_t* timer);

#ifdef __cplusplus
}
#endif

#endif
