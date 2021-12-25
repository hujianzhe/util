//
// Created by hujianzhe
//

#include "../../inc/datastruct/tree.h"

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

void treeInsertRight(struct Tree_t* node, struct Tree_t* new_node) {
	new_node->left = node;
	new_node->right = node->right;
	new_node->parent = node->parent;
	if (node->right) {
		node->right->left = new_node;
	}
	node->right = new_node;
}

void treeInsertLeft(struct Tree_t* node, struct Tree_t* new_node) {
	new_node->left = node->left;
	new_node->right = node;
	new_node->parent = node->parent;
	if (node->left) {
		node->left->right = new_node;
	}
	node->left = new_node;
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

struct Tree_t* treeLevelBegin(struct Tree_t* node) {
	struct Tree_t* up_node = node->parent;
	if (up_node) {
		struct Tree_t* dst_node = (struct Tree_t*)0;
		do {
			if (up_node->child) {
				dst_node = up_node->child;
			}
			up_node = up_node->left;
		} while (up_node);
		return dst_node;
	}
	else {
		while (node->left) {
			node = node->left;
		}
		return node;
	}
}

struct Tree_t* treeLevelNext(struct Tree_t* node) {
	if (node->right) {
		return node->right;
	}
	node = node->parent;
	if (!node) {
		return (struct Tree_t*)0;
	}
	while (node->right) {
		node = node->right;
		if (node->child) {
			return node->child;
		}
	}
	return (struct Tree_t*)0;
}

#ifdef	__cplusplus
}
#endif
