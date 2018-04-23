//
// Created by hujianzhe on 18-4-13.
//

#ifndef	UTIL_C_SCENE_SHAPE2D_H
#define	UTIL_C_SCENE_SHAPE2D_H

#include "vector_math.h"

typedef enum shape2d_enum_t {
	SHAPE2D_MIN_ENUM_NUM = -1,
	SHAPE2D_LINESEGMENT,
	SHAPE2D_AABB,
	SHAPE2D_CIRCLE,
	SHAPE2D_POLYGON,
	SHAPE2D_MAX_ENUM_NUM
} shape2d_enum_t;

typedef struct shape2d_linesegment_t {
	struct vector2_t vertices0;
	struct vector2_t vertices1;
} shape2d_linesegment_t;

typedef struct shape2d_aabb_t {
	struct vector2_t pivot;
	struct vector2_t half;
} shape2d_aabb_t;

typedef struct shape2d_circle_t {
	struct vector2_t pivot;
	double radius;
} shape2d_circle_t;

typedef struct shape2d_polygon_t {
	struct vector2_t pivot;
	double radian;
	unsigned int vertice_num;
	struct vector2_t* vertices;
} shape2d_polygon_t;

typedef union shape2d_t {
	shape2d_linesegment_t linesegment;
	shape2d_aabb_t aabb;
	shape2d_circle_t circle;
	shape2d_polygon_t polygon;
} shape2d_t;

#ifdef	__cplusplus
extern "C" {
#endif

struct shape2d_polygon_t* shape2d_polygon_rotate(struct shape2d_polygon_t* c, double radian);
struct shape2d_aabb_t* shape2d_shape_to_aabb(int type, const union shape2d_t* shape, struct shape2d_aabb_t* aabb);
unsigned int shape2d_has_point_n(int type, const union shape2d_t* shape, const struct vector2_t p[], unsigned int n);
int shape2d_has_overlap(int type1, const union shape2d_t* shape1, int type2, const union shape2d_t* shape2);
int shape2d_shape_has_contain_shape(int type1, const union shape2d_t* shape1, int type2, const union shape2d_t* shape2);

#ifdef	__cplusplus
}
#endif

#endif
