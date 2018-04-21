//
// Created by hujianzhe on 18-4-13.
//

#ifndef	UTIL_C_SCENE_SHAPE2D_H
#define	UTIL_C_SCENE_SHAPE2D_H

#include "vector_math.h"

typedef enum shape2d_enum_t {
	SHAPE2D_LINESEGMENT,
	SHAPE2D_AABB,
	SHAPE2D_CIRCLE,
	SHAPE2D_POLYGON
} shape2d_enum_t;

typedef struct shape2d_linesegment_t {
	struct vector2_t p1, p2;
} shape2d_linesegment_t;

typedef struct shape2d_aabb_t {
	struct vector2_t pivot;
	struct vector2_t half;
} shape2d_aabb_t;

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

int shape2d_linesegment_has_point(const struct vector2_t* s, const struct vector2_t* e, const struct vector2_t* p);
unsigned int shape2d_linesegment_has_point_n(const struct vector2_t* s, const struct vector2_t* e, const struct vector2_t p[], unsigned int n);
int shape2d_linesegment_has_intersect(const struct vector2_t* s1, const struct vector2_t* e1, const struct vector2_t* s2, const struct vector2_t* e2);

int shape2d_aabb_has_point(const struct shape2d_aabb_t* ab, const struct vector2_t* p);
unsigned int shape2d_aabb_has_point_n(const struct shape2d_aabb_t* ab, const struct vector2_t p[], unsigned int n);
int shape2d_aabb_aabb_has_overlap(const struct shape2d_aabb_t* ab1, const struct shape2d_aabb_t* ab2);
int shape2d_aabb_has_contain_aabb(const struct shape2d_aabb_t* ab1, const struct shape2d_aabb_t* ab2);
int shape2d_aabb_has_contain_circle(const struct shape2d_aabb_t* ab, const struct shape2d_circle_t* c);
int shape2d_aabb_has_contain_polygon(const struct shape2d_aabb_t* ab, const struct shape2d_polygon_t* p);
struct shape2d_polygon_t* shape2d_aabb_to_polygon(const struct shape2d_aabb_t* aabb, struct shape2d_polygon_t* c);
struct shape2d_aabb_t* shape2d_polygon_to_aabb(const struct shape2d_polygon_t* c, struct shape2d_aabb_t* aabb);
struct shape2d_aabb_t* shape2d_circle_to_aabb(const struct shape2d_circle_t* c, struct shape2d_aabb_t* aabb);

int shape2d_circle_has_point(const struct shape2d_circle_t* c, const struct vector2_t* p);
unsigned int shape2d_circle_has_point_n(const struct shape2d_circle_t* c, const struct vector2_t p[], unsigned int n);
int shape2d_circle_circle_has_overlap(const struct shape2d_circle_t* c1, const struct shape2d_circle_t* c2);
int shape2d_circle_linesegment_has_overlap(const struct shape2d_circle_t* c, const struct vector2_t* p1, const struct vector2_t* p2);
int shape2d_circle_has_contain_circle(const struct shape2d_circle_t* c1, const struct shape2d_circle_t* c2);
int shape2d_circle_has_contain_aabb(const struct shape2d_circle_t* c, const struct shape2d_aabb_t* ab);
int shape2d_circle_has_contain_polygon(const struct shape2d_circle_t* c, const struct shape2d_polygon_t* p);

int shape2d_obb_is_overlap(const struct shape2d_obb_t* o1, const struct shape2d_obb_t* o2);

void shape2d_polygon_rotate(struct shape2d_polygon_t* c, double radian);
int shape2d_polygon_has_point(const struct shape2d_polygon_t* c, const struct vector2_t* point);
unsigned int shape2d_polygon_has_point_n(const struct shape2d_polygon_t* c, const struct vector2_t p[], unsigned int n);
int shape2d_polygon_polygon_has_overlap(const struct shape2d_polygon_t* c1, const struct shape2d_polygon_t* c2);
int shape2d_polygon_circle_has_overlap(const struct shape2d_polygon_t* c, const struct shape2d_circle_t* circle);
int shape2d_polygon_linesegment_has_overlap(const struct shape2d_polygon_t* c, const struct vector2_t* s, const struct vector2_t* e);

#ifdef	__cplusplus
}
#endif

#endif
