//
// Created by hujianzhe
//

#ifndef	UTIL_C_DATASTRUCT_RBTREE_H
#define	UTIL_C_DATASTRUCT_RBTREE_H

#include "var_type.h"

typedef struct rbtree_node_t {
	unsigned long rb_color;
	struct rbtree_node_t *rb_parent;
	struct rbtree_node_t *rb_right;
	struct rbtree_node_t *rb_left;
	var_t rb_key;
} rbtree_node_t;

typedef struct rbtree_t {
	struct rbtree_node_t *rb_tree_node;
	int (*rb_key_cmp)(var_t, var_t);
} rbtree_t;

#ifdef	__cplusplus
extern "C" {
#endif

struct rbtree_t* rbtree_init(struct rbtree_t* root, int (*cmp)(var_t, var_t));

struct rbtree_node_t* rbtree_insert_node(struct rbtree_t* root, struct rbtree_node_t* node, var_t key);
struct rbtree_node_t* rbtree_replace_node(struct rbtree_t* root, struct rbtree_node_t* node, var_t key);
void rbtree_remove_node(struct rbtree_t* root, struct rbtree_node_t* node);

struct rbtree_node_t* rbtree_search_key(struct rbtree_t* root, var_t key);
struct rbtree_node_t* rbtree_remove_key(struct rbtree_t* root, var_t key);

struct rbtree_node_t* rbtree_next_node(struct rbtree_node_t* node);
struct rbtree_node_t* rbtree_prev_node(struct rbtree_node_t* node);
struct rbtree_node_t* rbtree_first_node(struct rbtree_t* root);
struct rbtree_node_t* rbtree_last_node(struct rbtree_t* root);

#ifdef	__cplusplus
}
#endif

#endif
