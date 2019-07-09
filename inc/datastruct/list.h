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

__declspec_dll struct List_t* listInit(struct List_t* list);
__declspec_dll struct ListNode_t* listFindNode(struct List_t* list, int(*cmp)(const struct ListNode_t*, const void*), const void* key);
__declspec_dll void listInsertNodeFront(struct List_t* list, struct ListNode_t* node, struct ListNode_t* new_node);
__declspec_dll void listInsertNodeBack(struct List_t* list, struct ListNode_t* node, struct ListNode_t* new_node);
__declspec_dll void listRemoveNode(struct List_t* list, struct ListNode_t* node);
__declspec_dll void listReplaceNode(struct List_t* list, struct ListNode_t* node, struct ListNode_t* new_node);
__declspec_dll void listMerge(struct List_t* to, struct List_t* from);
__declspec_dll void listReverse(struct List_t* list);
__declspec_dll struct List_t listSplit(struct List_t* old_list, struct ListNode_t* new_head);

#ifdef	__cplusplus
}
#endif

#endif
