//
// Created by hujianzhe on 18-4-13.
//

#include "../compiler_define.h"
#include "scene2d_tree.h"
#include <stdlib.h>

#ifdef	__cplusplus
extern "C" {
#endif

static int __subarea_index(struct shape2d_obb_t* ap, struct shape2d_obb_t* op) {
	if (op->y - op->h >= ap->y - ap->h && op->y + op->h <= ap->y) {
		if (op->x - op->w >= ap->x - ap->w && op->x + op->w <= ap->x) {
			return 0;
		}
		if (op->x - op->w >= ap->x && op->x + op->w <= ap->x + ap->w) {
			return 1;
		}
	}
	else if (op->y - op->h >= ap->y && op->y + op->h <= ap->y + ap->h) {
		if (op->x - op->w >= ap->x - ap->w && op->x + op->w <= ap->x) {
			return 2;
		}
		if (op->x - op->w >= ap->x && op->x + op->w <= ap->x + ap->w) {
			return 3;
		}
	}
	return -1;
}
static void __subarea_setup(struct scene2d_area_t* top, int index) {
	struct scene2d_area_t* sub = top->m_subarea[index];
	struct shape2d_obb_t p;
	switch (index) {
		case 0:
			p.x = top->p.x - top->p.w / 2;
			p.y = top->p.y - top->p.h / 2;
			break;
		case 1:
			p.x = top->p.x + top->p.w / 2;
			p.y = top->p.y - top->p.h / 2;
			break;
		case 2:
			p.x = top->p.x - top->p.w / 2;
			p.y = top->p.y + top->p.h / 2;
			break;
		case 3:
			p.x = top->p.x + top->p.w / 2;
			p.y = top->p.y + top->p.h / 2;
			break;
		default:
			return;
	}
	p.w = top->p.w / 2;
	p.h = top->p.h / 2;
	p.radian = top->p.radian;
	scene2d_area_init(sub, &p);
	tree_insert_child(&top->m_tree, &sub->m_tree);
	sub->deep = top->deep + 1;
}

static int __area_contain(struct shape2d_obb_t* ap, struct shape2d_obb_t* op) {
	return ap->x - ap->w <= op->x - op->w && ap->x + ap->w >= op->x + op->w
		&& ap->y - ap->h <= op->y - op->h && ap->y + ap->h >= op->y + op->h;
}

static void __area_split(struct scene2d_area_t* area) {
	int i;
	struct list_node_t *cur, *next;
	struct scene2d_area_t** sub_area = area->m_subarea;
	for (cur = area->shape_list.head; cur; cur = next) {
		struct scene2d_shape_t* shape = pod_container_of(cur, struct scene2d_shape_t, m_listnode);
		next = cur->next;
		i = __subarea_index(&area->p, &shape->p);
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

void scene2d_area_init(struct scene2d_area_t* node, const struct shape2d_obb_t* p) {
	int i;

	node->p = *p;

	node->deep = 0;

	node->shape_num = 0;
	list_init(&node->shape_list);

	tree_init(&node->m_tree);
	for (i = 0; i < sizeof(node->m_subarea) / sizeof(node->m_subarea[0]); ++i) {
		node->m_subarea[i] = (struct scene2d_area_t*)0;
	}
}

void scene2d_shape_entry(const struct scene2d_info_t* scinfo, struct scene2d_area_t* area, struct scene2d_shape_t* shape) {
	if (area->m_tree.child) {
		int i = __subarea_index(&area->p, &shape->p);
		if (i >= 0) {
			struct scene2d_area_t** sub_area = area->m_subarea;
			if (!sub_area[i]) {
				sub_area[i] = (struct scene2d_area_t*)malloc(sizeof(struct scene2d_area_t));
				if (!sub_area[i]) {
					return;
				}
				__subarea_setup(area, i);
			}
			scene2d_shape_entry(scinfo, sub_area[i], shape);
			return;
		}
	}
	list_insert_node_back(&area->shape_list, area->shape_list.tail, &shape->m_listnode);
	++area->shape_num;
	shape->area = area;
	if (!area->m_tree.child && area->shape_num >= scinfo->every_area_max_num && area->deep < scinfo->area_max_deep) {
		__area_split(area);
	}
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

void scene2d_shape_move(const struct scene2d_info_t* scinfo, struct scene2d_shape_t* shape, double x, double y) {
	struct scene2d_area_t* top_area = (struct scene2d_area_t*)0;
	struct tree_t* tree = &shape->area->m_tree;
	if (shape->p.x == x && shape->p.y == y) {
		return;
	}
	shape->p.x = x;
	shape->p.y = y;
	while (tree) {
		top_area = pod_container_of(tree, struct scene2d_area_t, m_tree);
		if (__area_contain(&top_area->p, &shape->p)) {
			break;
		}
		if (!tree->parent) {
			break;
		}
		tree = tree->parent;
	}
	if (top_area == shape->area) {
		return;
	}
	list_remove_node(&shape->area->shape_list, &shape->m_listnode);
	--shape->area->shape_num;
	scene2d_shape_entry(scinfo, top_area, shape);
}

void scene2d_overlap(const struct scene2d_area_t* area, const struct shape2d_obb_t* p, struct list_t* list) {
	int i;
	struct list_node_t* cur;

	if (!area) {
		return;
	}

	if (!shape2d_obb_is_overlap(&area->p, p)) {
		return;
	}
	for (cur = area->shape_list.head; cur; cur = cur->next) {
		struct scene2d_shape_t* shape = pod_container_of(cur, struct scene2d_shape_t, m_listnode);
		if (!shape2d_obb_is_overlap(&shape->p, p)) {
			continue;
		}
		list_insert_node_back(list, list->tail, &shape->m_foreachnode);
	}
	for (i = 0; i < sizeof(area->m_subarea) / sizeof(area->m_subarea[0]); ++i) {
		scene2d_overlap(area->m_subarea[i], p, list);
	}
}

#ifdef	__cplusplus
}
#endif