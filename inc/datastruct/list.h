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
__declspec_dll struct ListNode_t* listFindNode(const struct List_t* list, int(*cmp)(const struct ListNode_t*, const void*), const void* key);
__declspec_dll void listInsertNodeFront(struct List_t* list, struct ListNode_t* node, struct ListNode_t* new_node);
__declspec_dll void listInsertNodeBack(struct List_t* list, struct ListNode_t* node, struct ListNode_t* new_node);
__declspec_dll void listInsertNodeSorted(struct List_t* list, struct ListNode_t* new_node, int(*cmp)(struct ListNode_t*, struct ListNode_t*));
__declspec_dll void listRemoveNode(struct List_t* list, struct ListNode_t* node);
__declspec_dll void listReplaceNode(struct List_t* list, struct ListNode_t* node, struct ListNode_t* new_node);

__declspec_dll struct List_t* listPushNodeFront(struct List_t* list, struct ListNode_t* node);
__declspec_dll struct List_t* listPushNodeBack(struct List_t* list, struct ListNode_t* node);
__declspec_dll struct ListNode_t* listPopNodeFront(struct List_t* list);
__declspec_dll struct ListNode_t* listPopNodeBack(struct List_t* list);
__declspec_dll struct ListNode_t* listAt(const struct List_t* list, UnsignedPtr_t index);
__declspec_dll struct ListNode_t* listAtMost(const struct List_t* list, UnsignedPtr_t index);
__declspec_dll UnsignedPtr_t listNodeCount(const struct List_t* list);
__declspec_dll int listIsEmpty(const struct List_t* list);

__declspec_dll void listAppend(struct List_t* to, struct List_t* from);
__declspec_dll void listSwap(struct List_t* one, struct List_t* two);
__declspec_dll void listReverse(struct List_t* list);
__declspec_dll struct List_t listSplit(struct List_t* old_list, struct ListNode_t* head, struct ListNode_t* tail);
__declspec_dll struct List_t listSplitByHead(struct List_t* old_list, struct ListNode_t* new_head);
__declspec_dll struct List_t listSplitByTail(struct List_t* old_list, struct ListNode_t* new_tail);

#ifdef	__cplusplus
}
#endif

#endif
