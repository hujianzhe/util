//
// Created by hujianzhe on 18-4-13.
//

#ifndef	UTIL_C_SCENE_SCENE2D_COLLISION_H
#define	UTIL_C_SCENE_SCENE2D_COLLISION_H

#include "vector_math.h"

typedef struct scene2d_circle_t {
	double x, y;
	double radius;
} scene2d_circle_t;

typedef struct scene2d_obb_t {
	double x, y, w, h;
	double radian;
} scene2d_obb_t;

typedef struct scene2d_convex_t {
	double x, y;
	double radian;
	unsigned int vertice_num;
	struct vector2_t* vertices;
} scene2d_convex_t;

int scene2d_circle_is_overlap(const struct scene2d_circle_t* c1, const struct scene2d_circle_t* c2);
int scene2d_obb_is_overlap(const struct scene2d_obb_t* o1, const struct scene2d_obb_t* o2);
void scene2d_convex_rotate(struct scene2d_convex_t* c, double radian);
int scene2d_convex_is_overlap(const struct scene2d_convex_t* c1, const struct scene2d_convex_t* c2);

#endif
