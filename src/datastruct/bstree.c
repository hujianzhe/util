//
// Created by hujianzhe
//

#include "../../inc/datastruct/bstree.h"

#ifdef	__cplusplus
extern "C" {
#endif

int bstreeDefaultKeyCmpU32(const BSTreeNodeKey_t* node_key, const BSTreeNodeKey_t* key)
{
	if (key->u32 == node_key->u32)
		return 0;
	else if (key->u32 < node_key->u32)
		return -1;
	else
		return 1;
}

int bstreeDefaultKeyCmpI32(const BSTreeNodeKey_t* node_key, const BSTreeNodeKey_t* key)
{
	if (key->i32 == node_key->i32)
		return 0;
	else if (key->i32 < node_key->i32)
		return -1;
	else
		return 1;
}

int bstreeDefaultKeyCmpU64(const BSTreeNodeKey_t* node_key, const BSTreeNodeKey_t* key)
{
	if (key->u64 == node_key->u64)
		return 0;
	else if (key->u64 < node_key->u64)
		return -1;
	else
		return 1;
}

int bstreeDefaultKeyCmpI64(const BSTreeNodeKey_t* node_key, const BSTreeNodeKey_t* key)
{
	if (key->i64 == node_key->i64)
		return 0;
	else if (key->i64 < node_key->i64)
		return -1;
	else
		return 1;
}

int bstreeDefaultKeyCmpSSZ(const BSTreeNodeKey_t* node_key, const BSTreeNodeKey_t* key) {
	if (key->int_ptr == node_key->int_ptr)
		return 0;
	else if (key->int_ptr < node_key->int_ptr)
		return -1;
	else
		return 1;
}

int bstreeDefaultKeyCmpSZ(const BSTreeNodeKey_t* node_key, const BSTreeNodeKey_t* key)
{
	if (key->ptr == node_key->ptr)
		return 0;
	else if (key->ptr < node_key->ptr)
		return -1;
	else
		return 1;
}

int bstreeDefaultKeyCmpStr(const BSTreeNodeKey_t* node_key, const BSTreeNodeKey_t* key)
{
	const unsigned char* s1 = (const unsigned char*)node_key->ptr;
	const unsigned char* s2 = (const unsigned char*)key->ptr;
	while (*s1 && *s1 == *s2) {
		++s1;
		++s2;
	}
	if (*s1 > *s2)
		return -1;
	else if (*s1 < *s2)
		return 1;
	else
		return 0;
}

struct BSTree_t* bstreeInit(struct BSTree_t* tree, int(*keycmp)(const BSTreeNodeKey_t*, const BSTreeNodeKey_t*)) {
	tree->bs_root = (struct BSTreeNode_t*)0;
	tree->keycmp = keycmp;
	tree->count = 0;
	return tree;
}

struct BSTreeNode_t* bstreeInsertNode(struct BSTree_t* tree, struct BSTreeNode_t* node) {
	struct BSTreeNode_t* parent = tree->bs_root;
	while (parent) {
		int res = tree->keycmp(&parent->key, &node->key);
		if (res < 0) {
			if (parent->bs_left) {
				parent = parent->bs_left;
			}
			else {
				parent->bs_left = node;
				break;
			}
		}
		else if (res > 0) {
			if (parent->bs_right) {
				parent = parent->bs_right;
			}
			else {
				parent->bs_right = node;
				break;
			}
		}
		else {
			return parent;
		}
	}
	++(tree->count);
	if (!parent) {
		tree->bs_root = node;
	}
	node->bs_parent = parent;
	node->bs_left = node->bs_right = (struct BSTreeNode_t*)0;
	return node;
}

void bstreeReplaceNode(struct BSTree_t* tree, struct BSTreeNode_t* old_node, struct BSTreeNode_t* new_node) {
	if (old_node && old_node != new_node) {
		BSTreeNodeKey_t key = new_node->key;
		if (old_node->bs_left)
			old_node->bs_left->bs_parent = new_node;
		if (old_node->bs_right)
			old_node->bs_right->bs_parent = new_node;
		if (old_node->bs_parent) {
			if (old_node->bs_parent->bs_left == old_node)
				old_node->bs_parent->bs_left = new_node;
			if (old_node->bs_parent->bs_right == old_node)
				old_node->bs_parent->bs_right = new_node;
		}
		else {
			tree->bs_root = new_node;
		}
		*new_node = *old_node;
		new_node->key = key;
	}
}

void bstreeRemoveNode(struct BSTree_t* tree, struct BSTreeNode_t* node) {
	if (node->bs_right) {
		struct BSTreeNode_t* min_node = node->bs_right;
		while (min_node->bs_left)
			min_node = min_node->bs_left;
		if (node->bs_parent) {
			if (node->bs_parent->bs_left == node)
				node->bs_parent->bs_left = min_node;
			else
				node->bs_parent->bs_right = min_node;
		}
		else
			tree->bs_root = min_node;
		if (node->bs_left)
			node->bs_left->bs_parent = min_node;
		min_node->bs_left = node->bs_left;
		if (min_node->bs_parent != node) {
			node->bs_right->bs_parent = min_node;
			min_node->bs_parent->bs_left = min_node->bs_right;
			if (min_node->bs_right)
				min_node->bs_right->bs_parent = min_node->bs_parent;
			min_node->bs_right = node->bs_right;
		}
		min_node->bs_parent = node->bs_parent;
	}
	else if (node->bs_left) {
		node->bs_left->bs_parent = node->bs_parent;
		if (node->bs_parent) {
			if (node->bs_parent->bs_left == node)
				node->bs_parent->bs_left = node->bs_left;
			else
				node->bs_parent->bs_right = node->bs_left;
		}
		else {
			tree->bs_root = node->bs_left;
		}
	}
	else if (node->bs_parent) {
		if (node->bs_parent->bs_left == node)
			node->bs_parent->bs_left = (struct BSTreeNode_t*)0;
		else
			node->bs_parent->bs_right = (struct BSTreeNode_t*)0;
	}
	else {
		tree->bs_root = (struct BSTreeNode_t*)0;
	}
	--(tree->count);
}

struct BSTreeNode_t* bstreeMinNode(const struct BSTreeNode_t* node) {
	while (node->bs_left)
		node = node->bs_left;
	return (struct BSTreeNode_t*)node;
}

struct BSTreeNode_t* bstreeMaxNode(const struct BSTreeNode_t* node) {
	while (node->bs_right)
		node = node->bs_right;
	return (struct BSTreeNode_t*)node;
}

struct BSTreeNode_t* bstreeSearchKey(struct BSTree_t* tree, const BSTreeNodeKey_t key) {
	struct BSTreeNode_t* node = tree->bs_root;
	while (node) {
		int res = tree->keycmp(&node->key, &key);
		if (res < 0)
			node = node->bs_left;
		else if (res > 0)
			node = node->bs_right;
		else
			break;
	}
	return node;
}

struct BSTreeNode_t* bstreeRemoveKey(struct BSTree_t* tree, const BSTreeNodeKey_t key) {
	struct BSTreeNode_t* exist_node = bstreeSearchKey(tree, key);
	if (exist_node)
		bstreeRemoveNode(tree, exist_node);
	return exist_node;
}

struct BSTreeNode_t* bstreeFirstNode(const struct BSTree_t* tree) {
	if (tree->bs_root)
		return bstreeMinNode(tree->bs_root);
	return (struct BSTreeNode_t*)0;
}

struct BSTreeNode_t* bstreeLastNode(const struct BSTree_t* tree) {
	if (tree->bs_root)
		return bstreeMaxNode(tree->bs_root);
	return (struct BSTreeNode_t*)0;
}

struct BSTreeNode_t* bstreeNextNode(struct BSTreeNode_t* node) {
	struct BSTreeNode_t* parent;
	if (node->bs_right)
		return bstreeMinNode(node->bs_right);
	while ((parent = node->bs_parent) && node == parent->bs_right)
		node = parent;
	return parent;
}

struct BSTreeNode_t* bstreePrevNode(struct BSTreeNode_t* node) {
	struct BSTreeNode_t* parent;
	if (node->bs_left)
		return bstreeMaxNode(node->bs_left);
	while ((parent = node->bs_parent) && node == parent->bs_left)
		node = parent;
	return parent;
}

#ifdef	__cplusplus
}
#endif
