//
// Created by hujianzhe
//

#ifndef	UTIL_C_DATASTRUCT_RBTREE_H
#define	UTIL_C_DATASTRUCT_RBTREE_H

struct rbtree_t;
typedef struct rbtree_node_t {
	const void* key;
	unsigned long rb_color;
	struct rbtree_node_t *rb_parent;
	struct rbtree_node_t *rb_right;
	struct rbtree_node_t *rb_left;
	struct rbtree_t* rb_tree;
} rbtree_node_t;

typedef struct rbtree_t {
	struct rbtree_node_t *rb_tree_node;
	int (*keycmp)(struct rbtree_node_t*, const void*);
} rbtree_t;

#ifdef	__cplusplus
extern "C" {
#endif

struct rbtree_t* rbtree_init(struct rbtree_t* root, int(*keycmp)(struct rbtree_node_t*, const void*));

struct rbtree_node_t* rbtree_insert_node(struct rbtree_t* root, struct rbtree_node_t* node);
void rbtree_replace_node(struct rbtree_node_t* old_node, struct rbtree_node_t* new_node);
void rbtree_remove_node(struct rbtree_t* root, struct rbtree_node_t* node);

struct rbtree_node_t* rbtree_search_key(const struct rbtree_t* root, const void* key);
struct rbtree_node_t* rbtree_remove_key(struct rbtree_t* root, const void* key);

struct rbtree_node_t* rbtree_next_node(struct rbtree_node_t* node);
struct rbtree_node_t* rbtree_prev_node(struct rbtree_node_t* node);
struct rbtree_node_t* rbtree_first_node(const struct rbtree_t* root);
struct rbtree_node_t* rbtree_last_node(const struct rbtree_t* root);

#ifdef	__cplusplus
}
#endif

#endif
