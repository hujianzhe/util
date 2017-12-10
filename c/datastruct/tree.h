//
// Created by hujianzhe
//

#ifndef UTIL_C_DATASTRUCT_TREE_H
#define	UTIL_C_DATASTRUCT_TREE_H

typedef struct tree_node_t {
	struct tree_node_t *parent, *child, *left, *right;
} tree_node_t;

#ifdef	__cplusplus
extern "C" {
#endif

void tree_node_init(struct tree_node_t* node);
void tree_node_insert_child(struct tree_node_t* parent_node, struct tree_node_t* new_node);
void tree_node_insert_brother(struct tree_node_t* node, struct tree_node_t* new_node);
void tree_node_remove(struct tree_node_t* node);

#ifdef	__cplusplus
}
#endif

#endif // !UTIL_DATASTRUCT_TREE_H_
