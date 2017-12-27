//
// Created by hujianzhe
//

#ifndef	UTIL_C_COMPONENT_BLOCK_LINKED_QUEUE_H
#define	UTIL_C_COMPONENT_BLOCK_LINKED_QUEUE_H

#include "../datastruct/list.h"
#include "../syslib/error.h"
#include "../syslib/ipc.h"

#ifdef	__cplusplus
extern "C" {
#endif

typedef struct BlockLinkedQueue_t {
	CSLock_t m_cslock;
	ConditionVariable_t m_condition;
	list_node_t* m_head, *m_tail;
	BOOL m_forcewakeup;
	void(*m_deleter)(list_node_t*);
} BlockLinkedQueue_t;

void block_linked_queue_init(BlockLinkedQueue_t* q, void(*deleter)(list_node_t*));
void block_linked_queue_push(BlockLinkedQueue_t* q, list_node_t* data);
list_node_t* block_linked_queue_pop(BlockLinkedQueue_t* q, int msec, size_t expect_cnt);
void block_linked_queue_weakup(BlockLinkedQueue_t* q);
void block_linked_queue_clear(BlockLinkedQueue_t* q);
void block_linked_queue_destroy(BlockLinkedQueue_t* q);

#ifdef	__cplusplus
}
#endif

#endif
