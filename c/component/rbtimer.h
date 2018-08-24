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
	list_node_t m_listnode;
} RBTimerEvent_t;

typedef struct RBTimer_t {
	char m_initok;
	CriticalSection_t m_lock;
	rbtree_t m_rbtree;
} RBTimer_t;

#ifdef __cplusplus
extern "C" {
#endif

RBTimer_t* rbtimerInit(RBTimer_t* timer);
long long rbtimerMiniumTimestamp(RBTimer_t* timer);
int rbtimerAddEvent(RBTimer_t* timer, RBTimerEvent_t* e);
void rbtimerCall(RBTimer_t* timer, long long timestamp_msec, void(*deleter)(RBTimerEvent_t*));
void rbtimerClean(RBTimer_t* timer, void(*deleter)(RBTimerEvent_t*));
void rbtimerDestroy(RBTimer_t* timer, void(*deleter)(RBTimerEvent_t*));

#ifdef __cplusplus
}
#endif

#endif
