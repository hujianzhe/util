//
// Created by hujianzhe on 18-4-13.
//

#ifndef	UTIL_C_SCENE_SHAPE2D_H
#define	UTIL_C_SCENE_SHAPE2D_H

#include "vector_math.h"

typedef enum shape2d_enum_t {
	SHAPE2D_LINE,
	SHAPE2D_CIRCLE,
	SHAPE2D_POLYGON
} shape2d_enum_t;

typedef struct shape2d_circle_t {
	struct vector2_t pivot;
	double radius;
} shape2d_circle_t;

typedef struct shape2d_obb_t {
	double x, y, w, h;
	double radian;
} shape2d_obb_t;

typedef struct shape2d_polygon_t {
	struct vector2_t pivot;
	double radian;
	unsigned int vertice_num;
	struct vector2_t* vertices;
} shape2d_polygon_t;

#ifdef	__cplusplus
extern "C" {
#endif

int shape2d_line_is_intersect(const struct vector2_t* s1, const struct vector2_t* e1, const struct vector2_t* s2, const struct vector2_t* e2);
int shape2d_circle_has_overlap(const struct shape2d_circle_t* c1, const struct shape2d_circle_t* c2);
int shape2d_circle_line_has_overlap(const struct shape2d_circle_t* c, const struct vector2_t* p1, const struct vector2_t* p2);
int shape2d_obb_is_overlap(const struct shape2d_obb_t* o1, const struct shape2d_obb_t* o2);
void shape2d_polygon_rotate(struct shape2d_polygon_t* c, double radian);
int shape2d_polygon_contain_point(const struct shape2d_polygon_t* c, struct vector2_t* point);
int shape2d_polygon_has_overlap(const struct shape2d_polygon_t* c1, const struct shape2d_polygon_t* c2);
int shape2d_polygon_circle_has_overlap(const struct shape2d_polygon_t* c, const struct shape2d_circle_t* circle);
int shape2d_polygon_line_has_overlap(const struct shape2d_polygon_t* c, const struct vector2_t* s, const struct vector2_t* e);

#ifdef	__cplusplus
}
#endif

#endif
