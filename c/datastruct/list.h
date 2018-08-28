//
// Created by hujianzhe
//

#ifndef UTIL_C_DATASTRUCT_LIST_H
#define	UTIL_C_DATASTRUCT_LIST_H

typedef struct ListNode_t {
	struct ListNode_t *prev, *next;
} ListNode_t;
typedef struct List_t {
	struct ListNode_t *head, *tail;
} List_t;

/*
#define list_node_foreach(start, next, cur, list_node) \
	for(cur = start = (list_node); \
		next = cur ? cur->next : cur, cur; \
		cur = (next != start && next != cur ? next : (struct ListNode_t*)0))

#define	list_node_foreach_except(start, next, cur, list_node) \
	for(start = list_node, cur = start ? start->next : start; \
		next = cur ? cur->next : cur, cur != start; \
		cur = next)
*/

#ifdef	__cplusplus
extern "C" {
#endif

struct List_t* listInit(struct List_t* list);
void listInsertNodeFront(struct List_t* list, struct ListNode_t* node, struct ListNode_t* new_node);
void listInsertNodeBack(struct List_t* list, struct ListNode_t* node, struct ListNode_t* new_node);
void listRemoveNode(struct List_t* list, struct ListNode_t* node);
void listReplaceNode(struct List_t* list, struct ListNode_t* node, struct ListNode_t* new_node);
void listMerge(struct List_t* to, struct List_t* from);
void listReverse(struct List_t* list);
struct List_t listSplit(struct List_t* old_list, struct ListNode_t* new_head);

#ifdef	__cplusplus
}
#endif

#endif
