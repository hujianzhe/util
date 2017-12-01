//
// Created by hujianzhe
//

#ifndef UTIL_DATASTRUCT_LIST_H_
#define	UTIL_DATASTRUCT_LIST_H_

typedef struct list_node_t {
	struct list_node_t *prev, *next;
} list_node_t;

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

void list_node_init(struct list_node_t* node);
void list_node_insert_front(struct list_node_t* node, struct list_node_t* new_node);
void list_node_insert_back(struct list_node_t* node, struct list_node_t* new_node);
void list_node_remove(struct list_node_t* node);
void list_node_replace(struct list_node_t* node, struct list_node_t* new_node);
void list_node_merge(struct list_node_t* tail, struct list_node_t* head);
struct list_node_t* list_node_split(struct list_node_t* new_head);

#ifdef	__cplusplus
}
#endif

#endif
