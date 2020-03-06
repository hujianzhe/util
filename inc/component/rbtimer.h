//
// Created by hujianzhe on 18-8-24.
//

#ifndef	UTIL_C_COMPONENT_RBTIMER_H
#define	UTIL_C_COMPONENT_RBTIMER_H

#include "../datastruct/list.h"
#include "../datastruct/rbtree.h"
#include "../sysapi/ipc.h"

typedef struct RBTimerEvent_t {
	long long timestamp_msec;
	int(*callback)(struct RBTimerEvent_t*, void*);
	void* arg;
	ListNode_t m_listnode;
} RBTimerEvent_t;

typedef struct RBTimer_t {
	char m_initok;
	char m_uselock;
	CriticalSection_t m_lock;
	RBTree_t m_rbtree;
} RBTimer_t;

#ifdef __cplusplus
extern "C" {
#endif

__declspec_dll RBTimer_t* rbtimerInit(RBTimer_t* timer, BOOL uselock);
__declspec_dll long long rbtimerMiniumTimestamp(RBTimer_t* timer);
__declspec_dll int rbtimerAddEvent(RBTimer_t* timer, RBTimerEvent_t* e);
__declspec_dll void rbtimerCall(RBTimer_t* timer, long long timestamp_msec, void(*deleter)(RBTimerEvent_t*));
__declspec_dll void rbtimerClean(RBTimer_t* timer, void(*deleter)(RBTimerEvent_t*));
__declspec_dll void rbtimerDestroy(RBTimer_t* timer, void(*deleter)(RBTimerEvent_t*));

#ifdef __cplusplus
}
#endif

#endif
