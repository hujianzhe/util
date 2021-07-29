//
// Created by hujianzhe
//

#include "../../inc/datastruct/rbtree.h"

#define	RB_RED		1
#define	RB_BLACK	0

#ifdef	__cplusplus
extern "C" {
#endif

static void __rb_rotate_left(struct RBTreeNode_t *node, struct RBTree_t *root)
{
	struct RBTreeNode_t *right = node->rb_right;
	struct RBTreeNode_t *parent = node->rb_parent;

	if ((node->rb_right = right->rb_left))
		right->rb_left->rb_parent = node;
	right->rb_left = node;

	right->rb_parent = parent;

	if (parent)
	{
		if (node == parent->rb_left)
			parent->rb_left = right;
		else
			parent->rb_right = right;
	}
	else
		root->rb_tree_node = right;
	node->rb_parent = right;
}

static void __rb_rotate_right(struct RBTreeNode_t *node, struct RBTree_t *root)
{
	struct RBTreeNode_t *left = node->rb_left;
	struct RBTreeNode_t *parent = node->rb_parent;

	if ((node->rb_left = left->rb_right))
		left->rb_right->rb_parent = node;
	left->rb_right = node;

	left->rb_parent = parent;

	if (parent)
	{
		if (node == parent->rb_right)
			parent->rb_right = left;
		else
			parent->rb_left = left;
	}
	else
		root->rb_tree_node = left;
	node->rb_parent = left;
}

static void __rb_insert_color(struct RBTreeNode_t *node, struct RBTree_t *root)
{
	struct RBTreeNode_t *parent, *gparent;

	while ((parent = node->rb_parent) && parent->rb_color == RB_RED)
	{
		gparent = parent->rb_parent;
		
		if (parent == gparent->rb_left)
		{
			{
				register struct RBTreeNode_t *uncle = gparent->rb_right;
				if (uncle && uncle->rb_color == RB_RED)
				{
					uncle->rb_color = RB_BLACK;
					parent->rb_color = RB_BLACK;
					gparent->rb_color = RB_RED;
					node = gparent;
					continue;
				}
			}

			if (parent->rb_right == node)
			{
				register struct RBTreeNode_t *tmp;
				__rb_rotate_left(parent, root);
				tmp = parent;
				parent = node;
				node = tmp;
			}

			parent->rb_color = RB_BLACK;
			gparent->rb_color = RB_RED;
			__rb_rotate_right(gparent, root);
		} else {
			{
				register struct RBTreeNode_t *uncle = gparent->rb_left;
				if (uncle && uncle->rb_color == RB_RED)
				{
					uncle->rb_color = RB_BLACK;
					parent->rb_color = RB_BLACK;
					gparent->rb_color = RB_RED;
					node = gparent;
					continue;
				}
			}

			if (parent->rb_left == node)
			{
				register struct RBTreeNode_t *tmp;
				__rb_rotate_right(parent, root);
				tmp = parent;
				parent = node;
				node = tmp;
			}

			parent->rb_color = RB_BLACK;
			gparent->rb_color = RB_RED;
			__rb_rotate_left(gparent, root);
		}
	}

	root->rb_tree_node->rb_color = RB_BLACK;
}

int rbtreeDefaultKeyCmpU32(const RBTreeNodeKey_t* node_key, const RBTreeNodeKey_t* key)
{
	if (key->u32 == node_key->u32)
		return 0;
	else if (key->u32 < node_key->u32)
		return -1;
	else
		return 1;
}

int rbtreeDefaultKeyCmpI32(const RBTreeNodeKey_t* node_key, const RBTreeNodeKey_t* key)
{
	if (key->i32 == node_key->i32)
		return 0;
	else if (key->i32 < node_key->i32)
		return -1;
	else
		return 1;
}

int rbtreeDefaultKeyCmpU64(const RBTreeNodeKey_t* node_key, const RBTreeNodeKey_t* key)
{
	if (key->u64 == node_key->u64)
		return 0;
	else if (key->u64 < node_key->u64)
		return -1;
	else
		return 1;
}

int rbtreeDefaultKeyCmpI64(const RBTreeNodeKey_t* node_key, const RBTreeNodeKey_t* key)
{
	if (key->i64 == node_key->i64)
		return 0;
	else if (key->i64 < node_key->i64)
		return -1;
	else
		return 1;
}

int rbtreeDefaultKeyCmpSSZ(const RBTreeNodeKey_t* node_key, const RBTreeNodeKey_t* key) {
	if (key->int_ptr == node_key->int_ptr)
		return 0;
	else if (key->int_ptr < node_key->int_ptr)
		return -1;
	else
		return 1;
}

int rbtreeDefaultKeyCmpSZ(const RBTreeNodeKey_t* node_key, const RBTreeNodeKey_t* key)
{
	if (key->ptr == node_key->ptr)
		return 0;
	else if (key->ptr < node_key->ptr)
		return -1;
	else
		return 1;
}

int rbtreeDefaultKeyCmpStr(const RBTreeNodeKey_t* node_key, const RBTreeNodeKey_t* key)
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

struct RBTree_t* rbtreeInit(struct RBTree_t* root, int(*keycmp)(const RBTreeNodeKey_t*, const RBTreeNodeKey_t*))
{
	root->rb_tree_node = (struct RBTreeNode_t*)0;
	root->keycmp = keycmp;
	return root;
}

struct RBTreeNode_t* rbtreeInsertNode(struct RBTree_t* root, struct RBTreeNode_t* node)
{
	struct RBTreeNode_t* parent = root->rb_tree_node;
	while (parent) {
		int res = root->keycmp(&parent->key, &node->key);
		if (res < 0) {
			if (parent->rb_left) {
				parent = parent->rb_left;
			}
			else {
				parent->rb_left = node;
				break;
			}
		}
		else if (res > 0) {
			if (parent->rb_right) {
				parent = parent->rb_right;
			}
			else {
				parent->rb_right = node;
				break;
			}
		}
		else {
			return (struct RBTreeNode_t*)parent;
		}
	}
	node->rb_color = RB_RED;
	node->rb_left = node->rb_right = (struct RBTreeNode_t*)0;
	node->rb_parent = parent;
	node->rb_tree = root;
	if (!parent) {
		root->rb_tree_node = node;
	}
	__rb_insert_color(node, root);
	return node;
}

void rbtreeReplaceNode(struct RBTreeNode_t* old_node, struct RBTreeNode_t* new_node) {
	if (old_node && old_node != new_node) {
		RBTreeNodeKey_t key = new_node->key;
		if (old_node->rb_left) {
			old_node->rb_left->rb_parent = new_node;
		}
		if (old_node->rb_right) {
			old_node->rb_right->rb_parent = new_node;
		}
		if (old_node->rb_parent) {
			if (old_node->rb_parent->rb_left == old_node) {
				old_node->rb_parent->rb_left = new_node;
			}
			if (old_node->rb_parent->rb_right == old_node) {
				old_node->rb_parent->rb_right = new_node;
			}
		}
		else {
			old_node->rb_tree->rb_tree_node = new_node;
		}
		*new_node = *old_node;
		new_node->key = key;
	}
}

static void __rb_remove_color(struct RBTreeNode_t *node, struct RBTreeNode_t *parent, struct RBTree_t *root)
{
	struct RBTreeNode_t *other;

	while ((!node || node->rb_color == RB_BLACK) && node != root->rb_tree_node)
	{
		if (parent->rb_left == node)
		{
			other = parent->rb_right;
			if (other->rb_color == RB_RED)
			{
				other->rb_color = RB_BLACK;
				parent->rb_color = RB_RED;
				__rb_rotate_left(parent, root);
				other = parent->rb_right;
			}
			if ((!other->rb_left || other->rb_left->rb_color == RB_BLACK) &&
			    (!other->rb_right || other->rb_right->rb_color == RB_BLACK))
			{
				other->rb_color = RB_RED;
				node = parent;
				parent = node->rb_parent;
			}
			else
			{
				if (!other->rb_right || other->rb_right->rb_color == RB_BLACK)
				{
					other->rb_left->rb_color = RB_BLACK;
					other->rb_color = RB_RED;
					__rb_rotate_right(other, root);
					other = parent->rb_right;
				}
				other->rb_color = parent->rb_color;
				parent->rb_color = RB_BLACK;
				other->rb_right->rb_color = RB_BLACK;
				__rb_rotate_left(parent, root);
				node = root->rb_tree_node;
				break;
			}
		}
		else
		{
			other = parent->rb_left;
			if (other->rb_color == RB_RED)
			{
				other->rb_color = RB_BLACK;
				parent->rb_color = RB_RED;
				__rb_rotate_right(parent, root);
				other = parent->rb_left;
			}
			if ((!other->rb_left || other->rb_left->rb_color == RB_BLACK) &&
			    (!other->rb_right || other->rb_right->rb_color == RB_BLACK))
			{
				other->rb_color = RB_RED;
				node = parent;
				parent = node->rb_parent;
			}
			else
			{
				if (!other->rb_left || other->rb_left->rb_color == RB_BLACK)
				{
					other->rb_right->rb_color = RB_BLACK;
					other->rb_color = RB_RED;
					__rb_rotate_left(other, root);
					other = parent->rb_left;
				}
				other->rb_color = parent->rb_color;
				parent->rb_color = RB_BLACK;
				other->rb_left->rb_color = RB_BLACK;
				__rb_rotate_right(parent, root);
				node = root->rb_tree_node;
				break;
			}
		}
	}
	if (node)
		node->rb_color = RB_BLACK;
}

void rbtreeRemoveNode(struct RBTree_t* root, struct RBTreeNode_t* node)
{
	struct RBTreeNode_t *child, *parent;
	int color;

	if (!node->rb_left)
		child = node->rb_right;
	else if (!node->rb_right)
		child = node->rb_left;
	else
	{
		struct RBTreeNode_t *old = node, *left;

		node = node->rb_right;
		while ((left = node->rb_left))
			node = left;
		child = node->rb_right;
		parent = node->rb_parent;
		color = node->rb_color;

		if (child)
			child->rb_parent = parent;
		if (parent == old) {
			parent->rb_right = child;
			parent = node;
		} else
			parent->rb_left = child;

		node->rb_parent = old->rb_parent;
		node->rb_color = old->rb_color;
		node->rb_right = old->rb_right;
		node->rb_left = old->rb_left;

		if (old->rb_parent)
		{
			if (old->rb_parent->rb_left == old)
				old->rb_parent->rb_left = node;
			else
				old->rb_parent->rb_right = node;
		} else
			root->rb_tree_node = node;

		old->rb_left->rb_parent = node;
		if (old->rb_right)
			old->rb_right->rb_parent = node;
		goto color;
	}

	parent = node->rb_parent;
	color = node->rb_color;

	if (child)
		child->rb_parent = parent;
	if (parent)
	{
		if (parent->rb_left == node)
			parent->rb_left = child;
		else
			parent->rb_right = child;
	}
	else
		root->rb_tree_node = child;

color:
	if (color == RB_BLACK)
		__rb_remove_color(child, parent, root);
}

struct RBTreeNode_t* rbtreeSearchKey(const struct RBTree_t* root, const RBTreeNodeKey_t key)
{
	struct RBTreeNode_t *node = root->rb_tree_node;
	while (node) {
		int res = root->keycmp(&node->key, &key);
		if (res < 0) {
			node = node->rb_left;
		}
		else if (res > 0) {
			node = node->rb_right;
		}
		else {
			break;
		}
	}
	return node;
}

struct RBTreeNode_t* rbtreeLowerBoundKey(const struct RBTree_t* root, const RBTreeNodeKey_t key)
{
	struct RBTreeNode_t *great_equal_node = (struct RBTreeNode_t*)0;
	struct RBTreeNode_t *node = root->rb_tree_node;
	while (node) {
		int res = root->keycmp(&node->key, &key);
		if (res < 0) {
			great_equal_node = node;
			node = node->rb_left;
		}
		else if (res > 0) {
			node = node->rb_right;
		}
		else {
			great_equal_node = node;
			break;
		}
	}
	return great_equal_node;
}

struct RBTreeNode_t* rbtreeUpperBoundKey(const struct RBTree_t* root, const RBTreeNodeKey_t key)
{
	struct RBTreeNode_t *great_node = (struct RBTreeNode_t*)0;
	struct RBTreeNode_t *node = root->rb_tree_node;
	while (node) {
		int res = root->keycmp(&node->key, &key);
		if (res < 0) {
			great_node = node;
			node = node->rb_left;
		}
		else {
			node = node->rb_right;
		}
	}
	return great_node;
}

struct RBTreeNode_t* rbtreeRemoveKey(struct RBTree_t* root, const RBTreeNodeKey_t key)
{
	struct RBTreeNode_t *node = rbtreeSearchKey(root, key);
	if (node) {
		rbtreeRemoveNode(root, node);
	}
	return node;
}

/*
 * This function returns the first node (in sort order) of the tree.
 */
struct RBTreeNode_t* rbtreeFirstNode(const struct RBTree_t* root)
{
	struct RBTreeNode_t *n;

	n = root->rb_tree_node;
	if (n) {
		while (n->rb_left)
			n = n->rb_left;
	}
	return n;
}

struct RBTreeNode_t* rbtreeLastNode(const struct RBTree_t* root)
{
	struct RBTreeNode_t *n;

	n = root->rb_tree_node;
	if (n) {
		while (n->rb_right)
			n = n->rb_right;
	}
	return n;
}

struct RBTreeNode_t* rbtreeNextNode(struct RBTreeNode_t* node)
{
	struct RBTreeNode_t *parent;

	if (node->rb_parent == node)
		return (struct RBTreeNode_t*)0;

	/* If we have a right-hand child, go down and then left as far
	   as we can. */
	if (node->rb_right) {
		node = node->rb_right; 
		while (node->rb_left)
			node=node->rb_left;
		return node;
	}

	/* No right-hand children.  Everything down and left is
	   smaller than us, so any 'next' node must be in the general
	   direction of our parent. Go up the tree; any time the
	   ancestor is a right-hand child of its parent, keep going
	   up. First time it's a left-hand child of its parent, said
	   parent is our 'next' node. */
	while ((parent = node->rb_parent) && node == parent->rb_right)
		node = parent;

	return parent;
}

struct RBTreeNode_t* rbtreePrevNode(struct RBTreeNode_t* node)
{
	struct RBTreeNode_t *parent;

	if (node->rb_parent == node)
		return (struct RBTreeNode_t*)0;

	/* If we have a left-hand child, go down and then right as far
	   as we can. */
	if (node->rb_left) {
		node = node->rb_left; 
		while (node->rb_right)
			node=node->rb_right;
		return node;
	}

	/* No left-hand children. Go up till we find an ancestor which
	   is a right-hand child of its parent */
	while ((parent = node->rb_parent) && node == parent->rb_left)
		node = parent;

	return parent;
}

void rbtreeSwap(struct RBTree_t* root1, struct RBTree_t* root2)
{
	struct RBTree_t temp_root1 = *root1;
	struct RBTreeNode_t *cur;
	for (cur = rbtreeFirstNode(root1); cur; cur = rbtreeNextNode(cur)) {
		cur->rb_tree = root2;
	}
	for (cur = rbtreeFirstNode(root2); cur; cur = rbtreeNextNode(cur)) {
		cur->rb_tree = root1;
	}
	*root1 = *root2;
	*root2 = temp_root1;
}

#ifdef	__cplusplus
}
#endif
