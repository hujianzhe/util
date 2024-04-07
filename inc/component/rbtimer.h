//
// Created by hujianzhe on 18-8-24.
//

#ifndef	UTIL_C_COMPONENT_RBTIMER_H
#define	UTIL_C_COMPONENT_RBTIMER_H

#include "../platform_define.h"
#include "../datastruct/list.h"
#include "../datastruct/rbtree.h"

typedef struct RBTimer_t {
	/* private */
	RBTree_t m_rbtree;
	void* m_first_evlist;
} RBTimer_t;

typedef struct RBTimerEvent_t {
	/* private */
	ListNode_t m_listnode;
	struct RBTimer_t* m_timer;
	void* m_internal_evlist;
	/* public */
	long long timestamp; /* read-only */
	long long interval;
	void(*callback)(RBTimer_t*, struct RBTimerEvent_t*);
	void* arg;
} RBTimerEvent_t;

#ifdef __cplusplus
extern "C" {
#endif

__declspec_dll RBTimer_t* rbtimerInit(RBTimer_t* timer);
__declspec_dll long long rbtimerMiniumTimestamp(RBTimer_t* timer);
__declspec_dll RBTimer_t* rbtimerDueFirst(RBTimer_t* timers[], size_t timer_cnt, long long* min_timestamp);
__declspec_dll RBTimerEvent_t* rbtimerSetEvent(RBTimer_t* timer, RBTimerEvent_t* e, long long timestamp);
__declspec_dll BOOL rbtimerCheckEventScheduled(RBTimerEvent_t* e);
__declspec_dll void rbtimerDetachEvent(RBTimerEvent_t* e);
__declspec_dll RBTimerEvent_t* rbtimerTimeoutPopup(RBTimer_t* timer, long long timestamp);
__declspec_dll void rbtimerDestroy(RBTimer_t* timer);

#ifdef __cplusplus
}
#endif

#endif
