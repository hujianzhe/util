//
// Created by hujianzhe on 21-12-29
//

#include "../../inc/crt/geometry/aabb.h"
#include "../../inc/crt/octree.h"
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

static size_t octree_level_nodes_cnt(unsigned int deep_num) {
	size_t cnt = 1;
	if (deep_num > 1) {
		size_t i;
		deep_num--;
		for (i = 0; i < deep_num; ++i) {
			cnt *= 8;
		}
	}
	return cnt;
}

static size_t octree_total_nodes_cnt(unsigned int max_deep_num) {
	size_t total_cnt = 0;
	size_t i;
	for (i = 1; i <= max_deep_num; ++i) {
		total_cnt += octree_level_nodes_cnt(i);
	}
	return total_cnt;
}

static void octree_node_init(OctreeNode_t* root, const float pos[3], const float half[3]) {
	root->pos[0] = pos[0];
	root->pos[1] = pos[1];
	root->pos[2] = pos[2];
	root->half[0] = half[0];
	root->half[1] = half[1];
	root->half[2] = half[2];
	listInit(&root->obj_list);
	root->obj_cnt = 0;
	root->deep_num = 0;
	root->parent = NULL;
	root->childs = NULL;
}

static void octree_node_split(Octree_t* tree, OctreeNode_t* root) {
	int i;
	ListNode_t* cur, *next;
	float o[8][3], half[3];

	if (root->childs) {
		return;
	}
	root->childs = &tree->nodes[1 + ((root - tree->nodes) << 3)];
	mathAABBSplit(root->pos, root->half, o, half);
	for (i = 0; i < 8; ++i) {
		OctreeNode_t* child = root->childs + i;
		octree_node_init(child, o[i], half);
		child->parent = root;
		child->deep_num = root->deep_num + 1;
	}
	for (cur = root->obj_list.head; cur; cur = next) {
		OctreeObject_t* obj = pod_container_of(cur, OctreeObject_t, _node);
		next = cur->next;
		for (i = 0; i < 8; ++i) {
			OctreeNode_t* child = root->childs + i;
			if (!mathAABBContainAABB(child->pos, child->half, obj->pos, obj->half)) {
				continue;
			}
			listRemoveNode(&root->obj_list, cur);
			root->obj_cnt--;
			listPushNodeBack(&child->obj_list, cur);
			child->obj_cnt++;
			obj->oct = child;
			break;
		}
	}
}

Octree_t* octreeInit(Octree_t* tree, const float pos[3], const float half[3], unsigned int max_deep_num) {
	OctreeNode_t* nodes;
	size_t nodes_cnt;

	if (max_deep_num <= 0) {
		return NULL;
	}
	nodes_cnt = octree_total_nodes_cnt(max_deep_num);
	nodes = (OctreeNode_t*)calloc(1, sizeof(OctreeNode_t) * nodes_cnt);
	if (!nodes) {
		return NULL;
	}
	tree->nodes_cnt = nodes_cnt;
	tree->nodes = nodes;
	octree_node_init(&tree->nodes[0], pos, half);
	tree->nodes[0].deep_num = 1;
	tree->max_deep_num = max_deep_num;
	tree->max_cnt_per_node = 5;
	return tree;
}

void octreeRemoveObject(OctreeObject_t* obj) {
	OctreeNode_t* oct = obj->oct;
	if (!oct) {
		return;
	}
	listRemoveNode(&oct->obj_list, &obj->_node);
	oct->obj_cnt--;
	obj->oct = NULL;
}

void octreeUpdateObject(Octree_t* tree, OctreeObject_t* obj) {
	OctreeNode_t* root = &tree->nodes[0];
	OctreeNode_t* obj_oct = obj->oct;
	OctreeNode_t* oct = obj_oct ? obj_oct : root;
	while (oct) {
		if (oct->childs) {
			int i, find = 0;
			for (i = 0; i < 8; ++i) {
				OctreeNode_t* child = oct->childs + i;
				if (!mathAABBContainAABB(child->pos, child->half, obj->pos, obj->half)) {
					continue;
				}
				if (child->childs) {
					oct = child;
					break;
				}
				if (obj_oct) {
					listRemoveNode(&obj_oct->obj_list, &obj->_node);
					obj_oct->obj_cnt--;
				}
				listPushNodeBack(&child->obj_list, &obj->_node);
				child->obj_cnt++;
				obj->oct = child;
				find = 1;
				break;
			}
			if (find) {
				break;
			}
			if (i != 8) {
				continue;
			}
		}
		if (mathAABBContainAABB(oct->pos, oct->half, obj->pos, obj->half)) {
			if (oct == obj_oct) {
				return;
			}
			else if (obj_oct) {
				listRemoveNode(&obj_oct->obj_list, &obj->_node);
				obj_oct->obj_cnt--;
			}
			listPushNodeBack(&oct->obj_list, &obj->_node);
			oct->obj_cnt++;
			obj->oct = oct;
			break;
		}
		oct = oct->parent;
	}
	if (!obj->oct) {
		listPushNodeBack(&root->obj_list, &obj->_node);
		root->obj_cnt++;
		obj->oct = root;
		return;
	}
	if (obj->oct->obj_cnt > tree->max_cnt_per_node && obj->oct->deep_num < tree->max_deep_num) {
		octree_node_split(tree, obj->oct);
	}
}

OctreeFinder_t* octreeFinderInit(const Octree_t* tree, OctreeFinder_t* finder) {
	finder->nodes = (OctreeNode_t**)malloc(sizeof(OctreeNode_t*) * tree->nodes_cnt);
	if (!finder->nodes) {
		return NULL;
	}
	finder->cnt = 0;
	return finder;
}

void octreeFinderDestroy(OctreeFinder_t* finder) {
	if (finder->nodes) {
		free(finder->nodes);
		finder->nodes = NULL;
	}
	finder->cnt = 0;
}

void octreeFindNodes(OctreeNode_t* root, const float pos[3], const float half[3], OctreeFinder_t* finder) {
	size_t pop_idx;
	finder->cnt = 0;
	if (!mathAABBIntersectAABB(root->pos, root->half, pos, half)) {
		return;
	}
	finder->nodes[finder->cnt++] = root;
	pop_idx = 0;
	while (pop_idx < finder->cnt) {
		int i;
		OctreeNode_t* oct = finder->nodes[pop_idx++];
		if (!oct->childs) {
			continue;
		}
		for (i = 0; i < 8; ++i) {
			OctreeNode_t* child = oct->childs + i;
			if (!mathAABBIntersectAABB(child->pos, child->half, pos, half)) {
				continue;
			}
			finder->nodes[finder->cnt++] = child;
		}
	}
}

void octreeClear(Octree_t* tree) {
	size_t i;
	for (i = 0; i < tree->nodes_cnt; ++i) {
		OctreeNode_t* node = tree->nodes + i;
		ListNode_t* cur;
		for (cur = node->obj_list.head; cur; cur = cur->next) {
			OctreeObject_t* obj = pod_container_of(cur, OctreeObject_t, _node);
			obj->oct = NULL;
		}
		listInit(&node->obj_list);
		node->obj_cnt = 0;
		node->parent = node->childs = NULL;
	}
}

void octreeDestroy(Octree_t* tree) {
	size_t i;
	for (i = 0; i < tree->nodes_cnt; ++i) {
		OctreeNode_t* node = tree->nodes + i;
		ListNode_t* cur;
		for (cur = node->obj_list.head; cur; cur = cur->next) {
			OctreeObject_t* obj = pod_container_of(cur, OctreeObject_t, _node);
			obj->oct = NULL;
		}
	}
	free(tree->nodes);
	tree->nodes = NULL;
	tree->nodes_cnt = 0;
}

#ifdef __cplusplus
}
#endif
