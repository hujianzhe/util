//
// Created by hujianzhe
//

#include "list.h"

#ifdef	__cplusplus
extern "C" {
#endif

struct list_t* list_init(struct list_t* list) {
	list->head = list->tail = (struct list_node_t*)0;
	return list;
}

void list_insert_node_front(struct list_t* list, struct list_node_t* node, struct list_node_t* new_node) {
	if (!list->head) {
		list->head = list->tail = new_node;
		new_node->prev = new_node->next = (struct list_node_t*)0;
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

void list_insert_node_back(struct list_t* list, struct list_node_t* node, struct list_node_t* new_node) {
	if (!list->tail) {
		list->head = list->tail = new_node;
		new_node->prev = new_node->next = (struct list_node_t*)0;
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

void list_remove_node(struct list_t* list, struct list_node_t* node) {
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

void list_replace_node(struct list_t* list, struct list_node_t* node, struct list_node_t* new_node) {
	if (list->head == node) {
		list->head = new_node;
	}
	if (list->tail == node) {
		list->tail = new_node;
	}
	if (!node) {
		new_node->prev = new_node->next = (struct list_node_t*)0;
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

void list_merge(struct list_t* to, struct list_t* from) {
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
	from->head = from->tail = (struct list_node_t*)0;
}

struct list_t list_split(struct list_t* old_list, struct list_node_t* new_head) {
	struct list_t new_list;
	new_list.head = new_head;
	new_list.tail = old_list->tail;
	old_list->tail = new_head->prev;
	if (new_head->prev) {
		new_head->prev->next = (struct list_node_t*)0;
		new_head->prev = (struct list_node_t*)0;
	}
	else {
		old_list->head = (struct list_node_t*)0;
	}
	return new_list;
}

#ifdef	__cplusplus
}
#endif
