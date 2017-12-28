//
// Created by hujianzhe
//

#ifndef	UTIL_C_COMPONENT_WAIT_QUEUE_H
#define	UTIL_C_COMPONENT_WAIT_QUEUE_H

#include "../datastruct/list.h"
#include "../syslib/ipc.h"

#ifdef	__cplusplus
extern "C" {
#endif

typedef struct WaitQueue_t {
	CSLock_t m_cslock;
	ConditionVariable_t m_condition;
	list_node_t* m_head, *m_tail;
	volatile BOOL m_forcewakeup;
	BOOL m_initOk;
	void(*m_deleter)(list_node_t*);
} WaitQueue_t;

WaitQueue_t* wait_queue_init(WaitQueue_t* q, void(*deleter)(list_node_t*));
void wait_queue_push(WaitQueue_t* q, list_node_t* data);
list_node_t* wait_queue_pop(WaitQueue_t* q, int msec, size_t expect_cnt);
void wait_queue_weakup(WaitQueue_t* q);
void wait_queue_clear(WaitQueue_t* q);
void wait_queue_destroy(WaitQueue_t* q);

#ifdef	__cplusplus
}
#endif

#endif
