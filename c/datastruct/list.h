//
// Created by hujianzhe
//

#ifndef UTIL_C_DATASTRUCT_LIST_H
#define	UTIL_C_DATASTRUCT_LIST_H

#include "../compiler_define.h"

typedef struct ListNode_t {
	struct ListNode_t *prev, *next;
} ListNode_t;
typedef struct List_t {
	struct ListNode_t *head, *tail;
} List_t;

#ifdef	__cplusplus
extern "C" {
#endif

UTIL_LIBAPI struct List_t* listInit(struct List_t* list);
UTIL_LIBAPI void listInsertNodeFront(struct List_t* list, struct ListNode_t* node, struct ListNode_t* new_node);
UTIL_LIBAPI void listInsertNodeBack(struct List_t* list, struct ListNode_t* node, struct ListNode_t* new_node);
UTIL_LIBAPI void listRemoveNode(struct List_t* list, struct ListNode_t* node);
UTIL_LIBAPI void listReplaceNode(struct List_t* list, struct ListNode_t* node, struct ListNode_t* new_node);
UTIL_LIBAPI void listMerge(struct List_t* to, struct List_t* from);
UTIL_LIBAPI void listReverse(struct List_t* list);
UTIL_LIBAPI struct List_t listSplit(struct List_t* old_list, struct ListNode_t* new_head);

#ifdef	__cplusplus
}
#endif

#endif
