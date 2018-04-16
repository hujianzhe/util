//
// Created by hujianzhe on 18-4-13.
//

#ifndef	UTIL_C_SCENE_SCENE2D_TREE_H
#define	UTIL_C_SCENE_SCENE2D_TREE_H

#include "../datastruct/list.h"
#include "../datastruct/tree.h"
#include "scene2d_collision.h"

typedef struct scene2d_area_t {
	struct scene2d_obb_t p;

	unsigned int deep;

	unsigned int shape_num;
	struct list_t shape_list;

	struct tree_t m_tree;
	struct scene2d_area_t* m_subarea[4];
} scene2d_area_t;

typedef struct scene2d_info_t {
	struct scene2d_area_t* root;
	unsigned int every_area_max_num;
	unsigned int area_max_deep;
} scene2d_info_t;

typedef struct scene2d_shape_t {
	struct scene2d_obb_t p;
	void* userdata;
	struct scene2d_area_t* area;
	struct list_node_t m_listnode;
	struct list_node_t m_foreachnode;
} scene2d_shape_t;

void scene2d_area_init(struct scene2d_area_t* node, const struct scene2d_obb_t* p);
void scene2d_shape_entry(const struct scene2d_info_t* scinfo, struct scene2d_area_t* area, struct scene2d_shape_t* shape);
void scene2d_shape_leave(struct scene2d_shape_t* shape);
void scene2d_shape_move(const struct scene2d_info_t* scinfo, struct scene2d_shape_t* shape, double x, double y);
void scene2d_overlap(const struct scene2d_area_t* area, const struct scene2d_obb_t* p, struct list_t* list);

#endif
