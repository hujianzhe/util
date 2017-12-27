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

typedef struct block_linked_queue_t {
	CSLOCK m_cslock;
	CONDITION m_condition;
	list_node_t* m_head, *m_tail;
	BOOL m_forcewakeup;
	void(*m_deleter)(list_node_t*);
} block_linked_queue_t;

void block_linked_queue_init(block_linked_queue_t* q, void(*deleter)(list_node_t*));
void block_linked_queue_push(block_linked_queue_t* q, list_node_t* data);
list_node_t* block_linked_queue_pop(block_linked_queue_t* q, int msec, size_t expect_cnt);
void block_linked_queue_weakup(block_linked_queue_t* q);
void block_linked_queue_clear(block_linked_queue_t* q);
void block_linked_queue_destroy(block_linked_queue_t* q);

#ifdef	__cplusplus
}
#endif

#endif
