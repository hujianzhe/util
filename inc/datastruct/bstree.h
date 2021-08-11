//
// Created by hujianzhe
//

#ifndef	UTIL_C_DATASTRUCT_BSTREE_H
#define	UTIL_C_DATASTRUCT_BSTREE_H

#include "../compiler_define.h"

struct BSTree_t;
typedef struct BSTreeNode_t {
	const void* key;
	struct BSTreeNode_t* bs_left;
	struct BSTreeNode_t* bs_right;
	struct BSTreeNode_t* bs_parent;
	struct BSTree_t* bs_tree;
} BSTreeNode_t;

typedef struct BSTree_t {
	struct BSTreeNode_t* bs_root;
	int (*keycmp)(const void*, const void*);
	UnsignedPtr_t count;
} BSTree_t;

#ifdef	__cplusplus
extern "C" {
#endif

__declspec_dll struct BSTree_t* bstreeInit(struct BSTree_t* tree, int(*keycmp)(const void*, const void*));
__declspec_dll struct BSTreeNode_t* bstreeInsertNode(struct BSTree_t* tree, struct BSTreeNode_t* node);
__declspec_dll void bstreeReplaceNode(struct BSTreeNode_t* old_node, struct BSTreeNode_t* new_node);
__declspec_dll void bstreeRemoveNode(struct BSTree_t* tree, struct BSTreeNode_t* node);
__declspec_dll struct BSTreeNode_t* bstreeMinNode(const struct BSTreeNode_t* node);
__declspec_dll struct BSTreeNode_t* bstreeMaxNode(const struct BSTreeNode_t* node);
__declspec_dll struct BSTreeNode_t* bstreeSearchKey(struct BSTree_t* tree, const void* key);
__declspec_dll struct BSTreeNode_t* bstreeRemoveKey(struct BSTree_t* tree, const void* key);
__declspec_dll struct BSTreeNode_t* bstreeFirstNode(const struct BSTree_t* tree);
__declspec_dll struct BSTreeNode_t* bstreeLastNode(const struct BSTree_t* tree);
__declspec_dll struct BSTreeNode_t* bstreeNextNode(struct BSTreeNode_t* node);
__declspec_dll struct BSTreeNode_t* bstreePrevNode(struct BSTreeNode_t* node);

#ifdef	__cplusplus
}
#endif

#endif
