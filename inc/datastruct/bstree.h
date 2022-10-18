//
// Created by hujianzhe
//

#ifndef	UTIL_C_DATASTRUCT_BSTREE_H
#define	UTIL_C_DATASTRUCT_BSTREE_H

#include "../compiler_define.h"

typedef union {
	const void* ptr;
	SignedPtr_t int_ptr;
	UnsignedPtr_t uint_ptr;
	int i32;
	unsigned int u32;
	long long i64;
	unsigned long long u64;
} BSTreeNodeKey_t;

typedef struct BSTreeNode_t {
	BSTreeNodeKey_t key;
	struct BSTreeNode_t* bs_left;
	struct BSTreeNode_t* bs_right;
	struct BSTreeNode_t* bs_parent;
} BSTreeNode_t;

typedef struct BSTree_t {
	struct BSTreeNode_t* bs_root;
	int (*keycmp)(const BSTreeNodeKey_t*, const BSTreeNodeKey_t*);
	UnsignedPtr_t count;
} BSTree_t;

#ifdef	__cplusplus
extern "C" {
#endif

__declspec_dll int bstreeDefaultKeyCmpU32(const BSTreeNodeKey_t*, const BSTreeNodeKey_t*);
__declspec_dll int bstreeDefaultKeyCmpI32(const BSTreeNodeKey_t*, const BSTreeNodeKey_t*);
__declspec_dll int bstreeDefaultKeyCmpU64(const BSTreeNodeKey_t*, const BSTreeNodeKey_t*);
__declspec_dll int bstreeDefaultKeyCmpI64(const BSTreeNodeKey_t*, const BSTreeNodeKey_t*);
__declspec_dll int bstreeDefaultKeyCmpSSZ(const BSTreeNodeKey_t*, const BSTreeNodeKey_t*);
__declspec_dll int bstreeDefaultKeyCmpSZ(const BSTreeNodeKey_t*, const BSTreeNodeKey_t*);
__declspec_dll int bstreeDefaultKeyCmpStr(const BSTreeNodeKey_t*, const BSTreeNodeKey_t*);

__declspec_dll struct BSTree_t* bstreeInit(struct BSTree_t* tree, int(*keycmp)(const BSTreeNodeKey_t*, const BSTreeNodeKey_t*));
__declspec_dll struct BSTreeNode_t* bstreeInsertNode(struct BSTree_t* tree, struct BSTreeNode_t* node);
__declspec_dll void bstreeReplaceNode(struct BSTree_t* tree, struct BSTreeNode_t* old_node, struct BSTreeNode_t* new_node);
__declspec_dll void bstreeRemoveNode(struct BSTree_t* tree, struct BSTreeNode_t* node);
__declspec_dll struct BSTreeNode_t* bstreeMinNode(const struct BSTreeNode_t* node);
__declspec_dll struct BSTreeNode_t* bstreeMaxNode(const struct BSTreeNode_t* node);
__declspec_dll struct BSTreeNode_t* bstreeSearchKey(struct BSTree_t* tree, const BSTreeNodeKey_t key);
__declspec_dll struct BSTreeNode_t* bstreeRemoveKey(struct BSTree_t* tree, const BSTreeNodeKey_t key);
__declspec_dll struct BSTreeNode_t* bstreeFirstNode(const struct BSTree_t* tree);
__declspec_dll struct BSTreeNode_t* bstreeLastNode(const struct BSTree_t* tree);
__declspec_dll struct BSTreeNode_t* bstreeNextNode(struct BSTreeNode_t* node);
__declspec_dll struct BSTreeNode_t* bstreePrevNode(struct BSTreeNode_t* node);

#ifdef	__cplusplus
}
#endif

#endif
