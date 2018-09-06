//
// Created by hujianzhe
//

#ifndef	UTIL_C_DATASTRUCT_RBTREE_H
#define	UTIL_C_DATASTRUCT_RBTREE_H

#include "../compiler_define.h"

struct RBTree_t;
typedef struct RBTreeNode_t {
	const void* key;
	unsigned long rb_color;
	struct RBTreeNode_t *rb_parent;
	struct RBTreeNode_t *rb_right;
	struct RBTreeNode_t *rb_left;
	struct RBTree_t* rb_tree;
} RBTreeNode_t;

typedef struct RBTree_t {
	struct RBTreeNode_t *rb_tree_node;
	int (*keycmp)(struct RBTreeNode_t*, const void*);
} RBTree_t;

#ifdef	__cplusplus
extern "C" {
#endif

UTIL_LIBAPI struct RBTree_t* rbtreeInit(struct RBTree_t* root, int(*keycmp)(struct RBTreeNode_t*, const void*));

UTIL_LIBAPI struct RBTreeNode_t* rbtreeInsertNode(struct RBTree_t* root, struct RBTreeNode_t* node);
UTIL_LIBAPI void rbtreeReplaceNode(struct RBTreeNode_t* old_node, struct RBTreeNode_t* new_node);
UTIL_LIBAPI void rbtreeRemoveNode(struct RBTree_t* root, struct RBTreeNode_t* node);

UTIL_LIBAPI struct RBTreeNode_t* rbtreeSearchKey(const struct RBTree_t* root, const void* key);
UTIL_LIBAPI struct RBTreeNode_t* rbtreeRemoveKey(struct RBTree_t* root, const void* key);

UTIL_LIBAPI struct RBTreeNode_t* rbtreeNextNode(struct RBTreeNode_t* node);
UTIL_LIBAPI struct RBTreeNode_t* rbtreePrevNode(struct RBTreeNode_t* node);
UTIL_LIBAPI struct RBTreeNode_t* rbtreeFirstNode(const struct RBTree_t* root);
UTIL_LIBAPI struct RBTreeNode_t* rbtreeLastNode(const struct RBTree_t* root);

#ifdef	__cplusplus
}
#endif

#endif
