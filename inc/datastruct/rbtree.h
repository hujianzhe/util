//
// Created by hujianzhe
//

#ifndef	UTIL_C_DATASTRUCT_RBTREE_H
#define	UTIL_C_DATASTRUCT_RBTREE_H

#include "../compiler_define.h"

typedef union {
	const void* ptr;
	SignedPtr_t int_ptr;
	UnsignedPtr_t uint_ptr;
	int i32;
	unsigned int u32;
	long long i64;
	unsigned long long u64;
} RBTreeNodeKey_t;

struct RBTree_t;
typedef struct RBTreeNode_t {
	RBTreeNodeKey_t key;
	unsigned long rb_color;
	struct RBTreeNode_t *rb_parent;
	struct RBTreeNode_t *rb_right;
	struct RBTreeNode_t *rb_left;
	struct RBTree_t* rb_tree;
} RBTreeNode_t;

typedef struct RBTree_t {
	struct RBTreeNode_t *rb_tree_node;
	int (*keycmp)(const RBTreeNodeKey_t*, const RBTreeNodeKey_t*);
	UnsignedPtr_t count;
} RBTree_t;

#ifdef	__cplusplus
extern "C" {
#endif

__declspec_dll int rbtreeDefaultKeyCmpU32(const RBTreeNodeKey_t*, const RBTreeNodeKey_t*);
__declspec_dll int rbtreeDefaultKeyCmpI32(const RBTreeNodeKey_t*, const RBTreeNodeKey_t*);
__declspec_dll int rbtreeDefaultKeyCmpU64(const RBTreeNodeKey_t*, const RBTreeNodeKey_t*);
__declspec_dll int rbtreeDefaultKeyCmpI64(const RBTreeNodeKey_t*, const RBTreeNodeKey_t*);
__declspec_dll int rbtreeDefaultKeyCmpSSZ(const RBTreeNodeKey_t*, const RBTreeNodeKey_t*);
__declspec_dll int rbtreeDefaultKeyCmpSZ(const RBTreeNodeKey_t*, const RBTreeNodeKey_t*);
__declspec_dll int rbtreeDefaultKeyCmpStr(const RBTreeNodeKey_t*, const RBTreeNodeKey_t*);

__declspec_dll struct RBTree_t* rbtreeInit(struct RBTree_t* root, int(*keycmp)(const RBTreeNodeKey_t*, const RBTreeNodeKey_t*));

__declspec_dll struct RBTreeNode_t* rbtreeInsertNode(struct RBTree_t* root, struct RBTreeNode_t* node);
__declspec_dll void rbtreeReplaceNode(struct RBTreeNode_t* old_node, struct RBTreeNode_t* new_node);
__declspec_dll void rbtreeRemoveNode(struct RBTree_t* root, struct RBTreeNode_t* node);

__declspec_dll struct RBTreeNode_t* rbtreeSearchKey(const struct RBTree_t* root, const RBTreeNodeKey_t key);
__declspec_dll struct RBTreeNode_t* rbtreeLowerBoundKey(const struct RBTree_t* root, const RBTreeNodeKey_t key);
__declspec_dll struct RBTreeNode_t* rbtreeUpperBoundKey(const struct RBTree_t* root, const RBTreeNodeKey_t key);
__declspec_dll struct RBTreeNode_t* rbtreeRemoveKey(struct RBTree_t* root, const RBTreeNodeKey_t key);

__declspec_dll struct RBTreeNode_t* rbtreeNextNode(struct RBTreeNode_t* node);
__declspec_dll struct RBTreeNode_t* rbtreePrevNode(struct RBTreeNode_t* node);
__declspec_dll struct RBTreeNode_t* rbtreeFirstNode(const struct RBTree_t* root);
__declspec_dll struct RBTreeNode_t* rbtreeLastNode(const struct RBTree_t* root);

__declspec_dll void rbtreeSwap(struct RBTree_t* root1, struct RBTree_t* root2);

#ifdef	__cplusplus
}
#endif

#endif
