//
// Created by hujianzhe on 18-4-13.
//

#include "../compiler_define.h"
#include "scene2d_tree.h"
#include <stdlib.h>

#ifdef	__cplusplus
extern "C" {
#endif

static int __subarea_index(shape2d_aabb_t* ap, shape2d_aabb_t* op) {
	if (op->pivot.y - op->half.y >= ap->pivot.y - ap->half.y && op->pivot.y + op->half.y <= ap->pivot.y) {
		if (op->pivot.x - op->half.x >= ap->pivot.x - ap->half.x && op->pivot.x + op->half.x <= ap->pivot.x) {
			return 0;
		}
		if (op->pivot.x - op->half.x >= ap->pivot.x && op->pivot.x + op->half.x <= ap->pivot.x + ap->half.x) {
			return 1;
		}
	}
	else if (op->pivot.y - op->half.y >= ap->pivot.y && op->pivot.y + op->half.y <= ap->pivot.y + ap->half.y) {
		if (op->pivot.x - op->half.x >= ap->pivot.x - ap->half.x && op->pivot.x + op->half.x <= ap->pivot.x) {
			return 2;
		}
		if (op->pivot.x - op->half.x >= ap->pivot.x && op->pivot.x + op->half.x <= ap->pivot.x + ap->half.x) {
			return 3;
		}
	}
	return -1;
}
static void __subarea_setup(struct scene2d_area_t* top, int index) {
	struct scene2d_area_t* sub = top->m_subarea[index];
	switch (index) {
		case 0:
			sub->aabb.pivot.x = top->aabb.pivot.x - top->aabb.half.x / 2;
			sub->aabb.pivot.y = top->aabb.pivot.y - top->aabb.half.y / 2;
			break;
		case 1:
			sub->aabb.pivot.x = top->aabb.pivot.x + top->aabb.half.x / 2;
			sub->aabb.pivot.y = top->aabb.pivot.y - top->aabb.half.y / 2;
			break;
		case 2:
			sub->aabb.pivot.x = top->aabb.pivot.x - top->aabb.half.x / 2;
			sub->aabb.pivot.y = top->aabb.pivot.y + top->aabb.half.y / 2;
			break;
		case 3:
			sub->aabb.pivot.x = top->aabb.pivot.x + top->aabb.half.x / 2;
			sub->aabb.pivot.y = top->aabb.pivot.y + top->aabb.half.y / 2;
			break;
		default:
			return;
	}
	sub->aabb.half.x = top->aabb.half.x / 2;
	sub->aabb.half.y = top->aabb.half.y / 2;
	scene2d_area_init(sub);
	tree_insert_child(&top->m_tree, &sub->m_tree);
	sub->deep = top->deep + 1;
}

static void __area_split(struct scene2d_area_t* area) {
	int i;
	struct list_node_t *cur, *next;
	struct scene2d_area_t** sub_area = area->m_subarea;
	for (cur = area->shape_list.head; cur; cur = next) {
		struct scene2d_shape_t* shape = pod_container_of(cur, struct scene2d_shape_t, m_listnode);
		next = cur->next;
		i = __subarea_index(&area->aabb, &shape->m_aabb);
		if (i < 0 || i > 3) {
			continue;
		}
		if (!sub_area[i]) {
			sub_area[i] = (struct scene2d_area_t*)malloc(sizeof(struct scene2d_area_t));
			if (!sub_area[i]) {
				return;
			}
			__subarea_setup(area, i);
		}

		list_remove_node(&area->shape_list, cur);
		--area->shape_num;
		list_insert_node_back(&sub_area[i]->shape_list, sub_area[i]->shape_list.tail, cur);
		++sub_area[i]->shape_num;
		shape->area = sub_area[i];
	}
}

void scene2d_area_init(struct scene2d_area_t* node) {
	int i;

	node->deep = 0;

	node->shape_num = 0;
	list_init(&node->shape_list);

	tree_init(&node->m_tree);
	for (i = 0; i < sizeof(node->m_subarea) / sizeof(node->m_subarea[0]); ++i) {
		node->m_subarea[i] = (struct scene2d_area_t*)0;
	}
}

static void __scene2d_shape_entry(const struct scene2d_info_t* scinfo, struct scene2d_area_t* area, struct scene2d_shape_t* scshape) {
	if (area->m_tree.child) {
		int i = __subarea_index(&area->aabb, &scshape->m_aabb);
		if (i >= 0) {
			struct scene2d_area_t** sub_area = area->m_subarea;
			if (!sub_area[i]) {
				sub_area[i] = (struct scene2d_area_t*)malloc(sizeof(struct scene2d_area_t));
				if (!sub_area[i]) {
					return;
				}
				__subarea_setup(area, i);
			}
			__scene2d_shape_entry(scinfo, sub_area[i], scshape);
			return;
		}
	}
	list_insert_node_back(&area->shape_list, area->shape_list.tail, &scshape->m_listnode);
	++area->shape_num;
	scshape->area = area;
	if (!area->m_tree.child && area->shape_num >= scinfo->every_area_max_num && area->deep < scinfo->area_max_deep) {
		__area_split(area);
	}
}
void scene2d_shape_entry(const struct scene2d_info_t* scinfo, struct scene2d_area_t* area, struct scene2d_shape_t* scshape) {
	if (shape2d_shape_to_aabb(scshape->shape_type, &scshape->shape, &scshape->m_aabb))
		__scene2d_shape_entry(scinfo, area, scshape);
}

void scene2d_shape_leave(struct scene2d_shape_t* shape) {
	struct scene2d_area_t* area = shape->area;
	struct tree_t* tree = &area->m_tree;
	--area->shape_num;
	list_remove_node(&area->shape_list, &shape->m_listnode);
	shape->area = (struct scene2d_area_t*)0;

	while (tree->parent && !tree->child) {
		int i;
		struct tree_t* parent = tree->parent;
		area = pod_container_of(parent, struct scene2d_area_t, m_tree);
		for (i = 0; i < sizeof(area->m_subarea) / sizeof(area->m_subarea[0]); ++i) {
			struct scene2d_area_t** sub_area = area->m_subarea;
			if (!sub_area[i]) {
				continue;
			}
			if (sub_area[i]->shape_list.head) {
				continue;
			}
			tree_remove(&sub_area[i]->m_tree);
			free(sub_area[i]);
			sub_area[i] = (struct scene2d_area_t*)0;
		}
		tree = parent;
	}
}

void scene2d_shape_move(const struct scene2d_info_t* scinfo, struct scene2d_shape_t* scshape, const vector2_t* pivot) {
	struct scene2d_area_t* top_area = (struct scene2d_area_t*)0;
	struct tree_t* tree = &scshape->area->m_tree;
	if (vector2_equal(&scshape->m_aabb.pivot, pivot))
		return;
	shape2d_move_pivot(SHAPE2D_AABB, (shape2d_t*)&scshape->m_aabb, pivot);
	shape2d_move_pivot(scshape->shape_type, &scshape->shape, pivot);
	while (tree) {
		top_area = pod_container_of(tree, struct scene2d_area_t, m_tree);
		if (shape2d_shape_has_contain_shape(
			SHAPE2D_AABB, (const shape2d_t*)&top_area->aabb,
			SHAPE2D_AABB, (const shape2d_t*)&scshape->m_aabb))
		{
			break;
		}
		if (!tree->parent) {
			break;
		}
		tree = tree->parent;
	}
	if (top_area == scshape->area) {
		return;
	}
	list_remove_node(&scshape->area->shape_list, &scshape->m_listnode);
	--scshape->area->shape_num;
	__scene2d_shape_entry(scinfo, top_area, scshape);
}

void scene2d_overlap(const struct scene2d_area_t* area, int shape_type, const shape2d_t* shape, struct list_t* list) {
	int i;
	struct list_node_t* cur;

	if (!area) {
		return;
	}

	if (!shape2d_has_overlap(SHAPE2D_AABB, (shape2d_t*)&area->aabb, shape_type, shape))
		return;

	for (cur = area->shape_list.head; cur; cur = cur->next) {
		struct scene2d_shape_t* sc_shape = pod_container_of(cur, struct scene2d_shape_t, m_listnode);
		if (!shape2d_has_overlap(sc_shape->shape_type, &sc_shape->shape, shape_type, shape)) {
			continue;
		}
		list_insert_node_back(list, list->tail, &sc_shape->m_foreachnode);
	}
	for (i = 0; i < sizeof(area->m_subarea) / sizeof(area->m_subarea[0]); ++i) {
		scene2d_overlap(area->m_subarea[i], shape_type, shape, list);
	}
}

#ifdef	__cplusplus
}
#endif