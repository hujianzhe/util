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
	struct vector2_t vertices[2];
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

/*
typedef struct shape2d_obb_t {
double x, y, w, h;
double radian;
} shape2d_obb_t;
*/

#ifdef	__cplusplus
extern "C" {
#endif

/* linesegment */
//int shape2d_linesegment_has_point(const shape2d_linesegment_t* ls, const struct vector2_t* p);
//unsigned int shape2d_linesegment_has_point_n(const shape2d_linesegment_t* ls, const struct vector2_t p[], unsigned int n);
/* aabb */
struct shape2d_polygon_t* shape2d_aabb_to_polygon(const struct shape2d_aabb_t* aabb, struct shape2d_polygon_t* c);
//struct shape2d_aabb_t* shape2d_linesegment_to_aabb(const struct shape2d_linesegment_t* ls, struct shape2d_aabb_t* aabb);
//struct shape2d_aabb_t* shape2d_aabb_to_aabb(const struct shape2d_aabb_t* ab1, struct shape2d_aabb_t* ab2);
//struct shape2d_aabb_t* shape2d_polygon_to_aabb(const struct shape2d_polygon_t* c, struct shape2d_aabb_t* aabb);
//struct shape2d_aabb_t* shape2d_circle_to_aabb(const struct shape2d_circle_t* c, struct shape2d_aabb_t* aabb);
//int shape2d_aabb_has_point(const struct shape2d_aabb_t* ab, const struct vector2_t* p);
//unsigned int shape2d_aabb_has_point_n(const struct shape2d_aabb_t* ab, const struct vector2_t p[], unsigned int n);
//int shape2d_aabb_has_contain_aabb(const struct shape2d_aabb_t* ab1, const struct shape2d_aabb_t* ab2);
//int shape2d_aabb_has_contain_circle(const struct shape2d_aabb_t* ab, const struct shape2d_circle_t* c);
//int shape2d_aabb_has_contain_polygon(const struct shape2d_aabb_t* ab, const struct shape2d_polygon_t* p);
/* circle */
//int shape2d_circle_has_point(const struct shape2d_circle_t* c, const struct vector2_t* p);
//unsigned int shape2d_circle_has_point_n(const struct shape2d_circle_t* c, const struct vector2_t p[], unsigned int n);
//int shape2d_circle_has_contain_circle(const struct shape2d_circle_t* c1, const struct shape2d_circle_t* c2);
//int shape2d_circle_has_contain_aabb(const struct shape2d_circle_t* c, const struct shape2d_aabb_t* ab);
//int shape2d_circle_has_contain_polygon(const struct shape2d_circle_t* c, const struct shape2d_polygon_t* p);
//int shape2d_obb_is_overlap(const struct shape2d_obb_t* o1, const struct shape2d_obb_t* o2);
/* polygon */
void shape2d_polygon_rotate(struct shape2d_polygon_t* c, double radian);
//int shape2d_polygon_has_point(const struct shape2d_polygon_t* c, const struct vector2_t* point);
//unsigned int shape2d_polygon_has_point_n(const struct shape2d_polygon_t* c, const struct vector2_t p[], unsigned int n);

/*
int shape2d_linesegment_has_intersect(const shape2d_linesegment_t* ls1, const shape2d_linesegment_t* ls2);
int shape2d_aabb_aabb_has_overlap(const struct shape2d_aabb_t* ab1, const struct shape2d_aabb_t* ab2);
int shap2d_aabb_linesegment_has_overlap(const struct shape2d_aabb_t* ab, const shape2d_linesegment_t* ls);
int shape2d_aabb_circle_has_overlap(const struct shape2d_aabb_t* ab, const shape2d_circle_t* c);
int shape2d_aabb_polygon_has_overlap(const struct shape2d_aabb_t* ab, const shape2d_polygon_t* p);
int shape2d_circle_circle_has_overlap(const struct shape2d_circle_t* c1, const struct shape2d_circle_t* c2);
int shape2d_circle_linesegment_has_overlap(const struct shape2d_circle_t* c, const struct vector2_t* p1, const struct vector2_t* p2);
int shape2d_polygon_polygon_has_overlap(const struct shape2d_polygon_t* c1, const struct shape2d_polygon_t* c2);
int shape2d_polygon_circle_has_overlap(const struct shape2d_polygon_t* c, const struct shape2d_circle_t* circle);
int shape2d_polygon_linesegment_has_overlap(const struct shape2d_polygon_t* c, const shape2d_linesegment_t* ls);
*/

/* interface */
struct shape2d_aabb_t* shape2d_shape_to_aabb(int type, const union shape2d_t* shape, struct shape2d_aabb_t* aabb);
unsigned int shape2d_has_point_n(int type, const union shape2d_t* shape, const struct vector2_t p[], unsigned int n);
int shape2d_has_overlap(int type1, const union shape2d_t* shape1, int type2, const union shape2d_t* shape2);
int shape2d_shape_has_contain_shape(int type1, const union shape2d_t* shape1, int type2, const union shape2d_t* shape2);

#ifdef	__cplusplus
}
#endif

#endif
