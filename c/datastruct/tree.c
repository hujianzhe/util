//
// Created by hujianzhe
//

#include "tree.h"

#ifdef	__cplusplus
extern "C" {
#endif

void treeInit(struct Tree_t* node) {
	node->parent = node->child = node->left = node->right = (struct Tree_t*)0;
}

struct Tree_t* treeRoot(const struct Tree_t* node) {
	for (; node && node->parent; node = node->parent);
	return (struct Tree_t*)node;
}

void treeInsertChild(struct Tree_t* parent_node, struct Tree_t* new_node) {
	if (new_node->parent) {
		return;
	}
	if (parent_node->child) {
		new_node->right = parent_node->child;
		parent_node->child->left = new_node;
	}
	parent_node->child = new_node;
	new_node->parent = parent_node;
}

void treeInsertBrother(struct Tree_t* node, struct Tree_t* new_node) {
	new_node->left = node;
	new_node->right = node->right;
	new_node->parent = node->parent;
	if (node->right) {
		node->right->left = new_node;
	}
	node->right = new_node;
}

void treeRemove(struct Tree_t* node) {
	if (node->left) {
		node->left->right = node->right;
	}
	if (node->right) {
		node->right->left = node->left;
	}
	if (node->parent && node->parent->child == node) {
		node->parent->child = node->right;
	}
	node->left = node->right = node->parent = (struct Tree_t*)0;
}

struct Tree_t* treeBegin(struct Tree_t* node) {
	for (; node && node->child; node = node->child);
	return node;
}
struct Tree_t* treeNext(struct Tree_t* node) {
	if (node->right) {
		node = node->right;
		for (; node && node->child; node = node->child);
		return node;
	}
	return node->parent;
}

#ifdef	__cplusplus
}
#endif
