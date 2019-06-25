//
// Created by hujianzhe
//

#include "../../../inc/c/datastruct/list.h"

#ifdef	__cplusplus
extern "C" {
#endif

struct List_t* listInit(struct List_t* list) {
	list->head = list->tail = (struct ListNode_t*)0;
	return list;
}

void listInsertNodeFront(struct List_t* list, struct ListNode_t* node, struct ListNode_t* new_node) {
	if (!list->head) {
		list->head = list->tail = new_node;
		new_node->prev = new_node->next = (struct ListNode_t*)0;
		return;
	}
	if (list->head == node) {
		list->head = new_node;
	}

	new_node->next = node;
	new_node->prev = node->prev;
	if (node->prev) {
		node->prev->next = new_node;
	}
	node->prev = new_node;
}

void listInsertNodeBack(struct List_t* list, struct ListNode_t* node, struct ListNode_t* new_node) {
	if (!list->tail) {
		list->head = list->tail = new_node;
		new_node->prev = new_node->next = (struct ListNode_t*)0;
		return;
	}
	if (list->tail == node) {
		list->tail = new_node;
	}

	new_node->prev = node;
	new_node->next = node->next;
	if (node->next) {
		node->next->prev = new_node;
	}
	node->next = new_node;
}

void listRemoveNode(struct List_t* list, struct ListNode_t* node) {
	if (list->head == node) {
		list->head = node->next;
	}
	if (list->tail == node) {
		list->tail = node->prev;
	}

	if (node->prev) {
		node->prev->next = node->next;
	}
	if (node->next) {
		node->next->prev = node->prev;
	}
}

void listReplaceNode(struct List_t* list, struct ListNode_t* node, struct ListNode_t* new_node) {
	if (list->head == node) {
		list->head = new_node;
	}
	if (list->tail == node) {
		list->tail = new_node;
	}
	if (!node) {
		new_node->prev = new_node->next = (struct ListNode_t*)0;
		return;
	}

	if (node->prev) {
		node->prev->next = new_node;
	}
	if (node->next) {
		node->next->prev = new_node;
	}
	*new_node = *node;
}

void listMerge(struct List_t* to, struct List_t* from) {
	if (!to->head) {
		to->head = from->head;
	}
	if (to->tail) {
		to->tail->next = from->head;
	}
	if (from->head) {
		from->head->prev = to->tail;
	}
	if (from->tail) {
		to->tail = from->tail;
	}
	from->head = from->tail = (struct ListNode_t*)0;
}

void listReverse(struct List_t* list) {
	struct ListNode_t* cur = list->head;
	list->tail = cur;
	while (cur) {
		struct ListNode_t* p = cur->next;
		list->head = cur;
		cur->next = cur->prev;
		cur->prev = p;
		cur = cur->prev;
	}
}

struct List_t listSplit(struct List_t* old_list, struct ListNode_t* new_head) {
	if (new_head) {
		struct List_t new_list;
		new_list.head = new_head;
		new_list.tail = old_list->tail;
		old_list->tail = new_head->prev;
		if (new_head->prev) {
			new_head->prev->next = (struct ListNode_t*)0;
			new_head->prev = (struct ListNode_t*)0;
		}
		else {
			old_list->head = (struct ListNode_t*)0;
		}
		return new_list;
	}
	else {
		return *old_list;
	}
}

#ifdef	__cplusplus
}
#endif
