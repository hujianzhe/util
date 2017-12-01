//
// Created by hujianzhe
//

#include "tree.h"

#ifdef	__cplusplus
extern "C" {
#endif

void tree_node_init(struct tree_node_t* node) {
	node->parent = node->child = node->left = node->right = (struct tree_node_t*)0;
}

void tree_node_insert_child(struct tree_node_t* parent_node, struct tree_node_t* new_node) {
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

void tree_node_insert_brother(struct tree_node_t* node, struct tree_node_t* new_node) {
	new_node->left = node;
	new_node->right = node->right;
	if (node->right) {
		node->right->left = new_node;
	}
	node->right = new_node;
}

void tree_node_remove(struct tree_node_t* node) {
	if (node->left) {
		node->left->right = node->right;
	}
	if (node->right) {
		node->right->left = node->left;
	}
	if (node->parent && node->parent->child == node) {
		node->parent->child = node->right;
	}
	node->left = node->right = node->parent = (struct tree_node_t*)0;
}

#ifdef	__cplusplus
}
#endif
