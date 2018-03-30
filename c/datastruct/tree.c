//
// Created by hujianzhe
//

#include "tree.h"

#ifdef	__cplusplus
extern "C" {
#endif

void tree_init(struct tree_t* node) {
	node->parent = node->child = node->left = node->right = (struct tree_t*)0;
}

struct tree_t* tree_root(const struct tree_t* node) {
	for (; node && node->parent; node = node->parent);
	return (struct tree_t*)node;
}

void tree_insert_child(struct tree_t* parent_node, struct tree_t* new_node) {
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

void tree_insert_brother(struct tree_t* node, struct tree_t* new_node) {
	new_node->left = node;
	new_node->right = node->right;
	new_node->parent = node->parent;
	if (node->right) {
		node->right->left = new_node;
	}
	node->right = new_node;
}

void tree_remove(struct tree_t* node) {
	if (node->left) {
		node->left->right = node->right;
	}
	if (node->right) {
		node->right->left = node->left;
	}
	if (node->parent && node->parent->child == node) {
		node->parent->child = node->right;
	}
	node->left = node->right = node->parent = (struct tree_t*)0;
}

#ifdef	__cplusplus
}
#endif
