//
// Created by hujianzhe
//

#include "list.h"

#ifdef	__cplusplus
extern "C" {
#endif

void list_node_init(struct list_node_t* node) {
	node->prev = node->next = (struct list_node_t*)0;
}

void list_node_insert_front(struct list_node_t* node, struct list_node_t* new_node) {
	new_node->next = node;
	new_node->prev = node->prev;
	if (node->prev) {
		node->prev->next = new_node;
	}
	node->prev = new_node;
}

void list_node_insert_back(struct list_node_t* node, struct list_node_t* new_node) {
	new_node->prev = node;
	new_node->next = node->next;
	if (node->next) {
		node->next->prev = new_node;
	}
	node->next = new_node;
}

void list_node_remove(struct list_node_t* node) {
	if (node->prev) {
		node->prev->next = node->next;
	}
	if (node->next) {
		node->next->prev = node->prev;
	}
}

void list_node_replace(struct list_node_t* node, struct list_node_t* new_node) {
	/*
	if (node->prev == node || node->next == node) {
		return;
	}
	*/
	if (node->prev) {
		node->prev->next = new_node;
	}
	if (node->next) {
		node->next->prev = new_node;
	}
	*new_node = *node;
}

void list_node_merge(struct list_node_t* tail, struct list_node_t* head) {
	/*
	if (tail->next) {
		tail->next->prev = head->prev;
	}
	if (head->prev) {
		head->prev->next = tail->next;
	}
	*/
	tail->next = head;
	head->prev = tail;
}

struct list_node_t* list_node_split(struct list_node_t* new_head) {
	struct list_node_t* tail = new_head->prev;
	if (tail) {
		new_head->prev = (struct list_node_t*)0;
		tail->next = (struct list_node_t*)0;
	}
	return tail;
}

#ifdef	__cplusplus
}
#endif