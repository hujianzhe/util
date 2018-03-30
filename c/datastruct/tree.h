//
// Created by hujianzhe
//

#ifndef UTIL_C_DATASTRUCT_TREE_H
#define	UTIL_C_DATASTRUCT_TREE_H

typedef struct tree_t {
	struct tree_t *parent, *child, *left, *right;
} tree_t;

#ifdef	__cplusplus
extern "C" {
#endif

void tree_init(struct tree_t* node);
struct tree_t* tree_root(const struct tree_t* node);
void tree_insert_child(struct tree_t* parent_node, struct tree_t* new_node);
void tree_insert_brother(struct tree_t* node, struct tree_t* new_node);
void tree_remove(struct tree_t* node);

#ifdef	__cplusplus
}
#endif

#endif // !UTIL_DATASTRUCT_TREE_H_
