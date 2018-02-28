//
// Created by hujianzhe
//

#ifndef UTIL_C_DATASTRUCT_LIST_H
#define	UTIL_C_DATASTRUCT_LIST_H

typedef struct list_node_t {
	struct list_node_t *prev, *next;
} list_node_t;
typedef struct list_t {
	struct list_node_t *head, *tail;
} list_t;

/*
#define list_node_foreach(start, next, cur, list_node) \
	for(cur = start = (list_node); \
		next = cur ? cur->next : cur, cur; \
		cur = (next != start && next != cur ? next : (struct list_node_t*)0))

#define	list_node_foreach_except(start, next, cur, list_node) \
	for(start = list_node, cur = start ? start->next : start; \
		next = cur ? cur->next : cur, cur != start; \
		cur = next)
*/

#ifdef	__cplusplus
extern "C" {
#endif

struct list_t* list_init(struct list_t* list);
void list_insert_node_front(struct list_t* list, struct list_node_t* node, struct list_node_t* new_node);
void list_insert_node_back(struct list_t* list, struct list_node_t* node, struct list_node_t* new_node);
void list_remove_node(struct list_t* list, struct list_node_t* node);
void list_replace_node(struct list_t* list, struct list_node_t* node, struct list_node_t* new_node);
void list_merge(struct list_t* to, struct list_t* from);
void list_reverse(struct list_t* list);
struct list_t list_split(struct list_t* old_list, struct list_node_t* new_head);

#ifdef	__cplusplus
}
#endif

#endif
