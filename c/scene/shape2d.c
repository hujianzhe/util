//
// Created by hujianzhe on 18-4-13.
//

#include "shape2d.h"
#include <errno.h>
#include <float.h>
#if defined(_WIN32) || defined(_WIN64)
	#ifndef	_USE_MATH_DEFINES
		#define	_USE_MATH_DEFINES
	#endif
#endif
#include <math.h>
#include <stdlib.h>
#if defined(_WIN32) || defined(_WIN64)
	#include <malloc.h>
	#ifndef alloca
		#define alloca		_alloca
	#endif
#else
	#if	__linux__
		#include <alloca.h>
	#endif
#endif

#ifdef	__cplusplus
extern "C" {
#endif

int shape2d_linesegment_has_point(const struct vector2_t* s, const struct vector2_t* e, const struct vector2_t* p) {
	struct vector2_t se = { e->x - s->x, e->y - s->y };
	struct vector2_t sp = { p->x - s->x, p->y - s->y };
	double res = vector2_cross(&se, &sp);
	if (res < -DBL_EPSILON || res > DBL_EPSILON)
		return 0;
	return p->x >= fmin(s->x, e->x) && p->x <= fmax(s->x, e->x) && p->y >= fmin(s->y, e->y) && p->y <= fmax(s->y, e->y);
}
int shape2d_linesegment_has_intersect(const struct vector2_t* s1, const struct vector2_t* e1, const struct vector2_t* s2, const struct vector2_t* e2) {
	struct vector2_t s1e1 = { e1->x - s1->x, e1->y - s1->y };
	struct vector2_t s1s2 = { s2->x - s1->x, s2->y - s1->y };
	struct vector2_t s1e2 = { e2->x - s1->x, e2->y - s1->y };
	struct vector2_t s2e2 = { e2->x - s2->x, e2->y - s2->y };
	struct vector2_t s2s1 = { s1->x - s2->x, s1->y - s2->y };
	struct vector2_t s2e1 = { e1->x - s2->x, e1->y - s2->y };
	double d12 = vector2_cross(&s1s2, &s1e1) * vector2_cross(&s1e2, &s1e1);
	double d34 = vector2_cross(&s2s1, &s2e2) * vector2_cross(&s2e1, &s2e2);
	if (d12 < -DBL_EPSILON && d34 < -DBL_EPSILON)
		return 1;
	if (d12 > DBL_EPSILON || d34 > DBL_EPSILON)
		return 0;
	if (shape2d_linesegment_has_point(s1, e1, s2) || shape2d_linesegment_has_point(s1, e1, e2) ||
		shape2d_linesegment_has_point(s2, e2, s1) || shape2d_linesegment_has_point(s2, e2, e1))
		return 2;
	return 0;
}

int shape2d_aabb_has_point(const struct shape2d_aabb_t* ab, const struct vector2_t* p) {
	return ab->pivot.x - ab->half.x < p->x
		&& ab->pivot.x + ab->half.x > p->x
		&& ab->pivot.y - ab->half.y < p->y
		&& ab->pivot.y + ab->half.y > p->y;
}
int shape2d_aabb_has_overlap(const struct shape2d_aabb_t* ab1, const struct shape2d_aabb_t* ab2) {
	return fabs(ab1->pivot.x - ab2->pivot.x) < ab1->half.x + ab2->half.x
		&& fabs(ab1->pivot.y - ab2->pivot.y) < ab1->half.y + ab2->half.y;
}
int shape2d_aabb_has_contain(const struct shape2d_aabb_t* ab1, const struct shape2d_aabb_t* ab2) {
	return ab1->pivot.x - ab1->half.x <= ab2->pivot.x - ab2->half.x
		&& ab1->pivot.x + ab1->half.x >= ab2->pivot.x + ab2->half.x
		&& ab1->pivot.y - ab1->half.y <= ab2->pivot.y - ab2->half.y
		&& ab1->pivot.y + ab1->half.y >= ab2->pivot.y + ab2->half.y;
}
struct shape2d_polygon_t* shape2d_aabb_to_polygon(const struct shape2d_aabb_t* aabb, struct shape2d_polygon_t* c) {
	c->radian = 0.0;
	c->pivot = aabb->pivot;
	c->vertice_num = 4;
	c->vertices[0].x = aabb->pivot.x - aabb->half.x;
	c->vertices[0].y = aabb->pivot.y - aabb->half.y;
	c->vertices[1].x = aabb->pivot.x + aabb->half.x;
	c->vertices[1].y = aabb->pivot.y - aabb->half.y;
	c->vertices[2].x = aabb->pivot.x + aabb->half.x;
	c->vertices[2].y = aabb->pivot.y + aabb->half.y;
	c->vertices[3].x = aabb->pivot.x - aabb->half.x;
	c->vertices[4].y = aabb->pivot.y + aabb->half.y;
	return c;
}
struct shape2d_aabb_t* shape2d_polygon_to_aabb(const struct shape2d_polygon_t* c, struct shape2d_aabb_t* aabb) {
	double min_x = c->vertices[0].x;
	double max_x = min_x;
	double min_y = c->vertices[0].y;
	double max_y = min_y;
	unsigned int i;
	for (i = 1; i < c->vertice_num; ++i) {
		if (c->vertices[i].x > max_x)
			max_x = c->vertices[i].x;
		if (c->vertices[i].x < min_x)
			min_x = c->vertices[i].x;
		if (c->vertices[i].y > max_y)
			max_y = c->vertices[i].y;
		if (c->vertices[i].y < min_y)
			min_y = c->vertices[i].y;
	}
	aabb->pivot.x = (min_x + max_x) * 0.5;
	aabb->pivot.y = (min_y + max_y) * 0.5;
	aabb->half.x = fabs(max_x - min_x) * 0.5;
	aabb->half.y = fabs(max_y - min_y) * 0.5;
	return aabb;
}
struct shape2d_aabb_t* shape2d_circle_to_aabb(const struct shape2d_circle_t* c, struct shape2d_aabb_t* aabb) {
	aabb->pivot = c->pivot;
	aabb->half.x = aabb->half.y = c->radius;
	return aabb;
}

int shape2d_circle_has_point(const struct shape2d_circle_t* c, const struct vector2_t* p) {
	struct vector2_t v = { p->x - c->pivot.x, p->y - c->pivot.y };
	return vector2_lensq(&v) < c->radius * c->radius;
}
int shape2d_circle_has_overlap(const struct shape2d_circle_t* c1, const struct shape2d_circle_t* c2) {
	double rd = c1->radius + c2->radius;
	double xd = c1->pivot.x - c2->pivot.x;
	double yd = c1->pivot.y - c2->pivot.y;
	return xd * xd + yd * yd < rd * rd;
}
int shape2d_circle_has_contain(const struct shape2d_circle_t* c1, const struct shape2d_circle_t* c2) {
	struct vector2_t v;
	double d = c1->radius - c2->radius;
	if (d < DBL_EPSILON)
		return 0;
	v.x = c2->pivot.x - c1->pivot.x;
	v.y = c2->pivot.y - c1->pivot.y;
	return vector2_lensq(&v) <= d * d;
}
int shape2d_circle_linesegment_has_overlap(const struct shape2d_circle_t* c, const struct vector2_t* p1, const struct vector2_t* p2) {
	double d = c->radius * c->radius;
	struct vector2_t v, p;
	v.x = c->pivot.x - p1->x;
	v.y = c->pivot.y - p1->y;
	if (vector2_lensq(&v) < d) {
		return 1;
	}
	v.x = c->pivot.x - p2->x;
	v.y = c->pivot.y - p2->y;
	if (vector2_lensq(&v) < d) {
		return 1;
	}
	//
	v.x = c->pivot.x - p1->x;
	v.y = c->pivot.y - p1->y;
	p.x = p2->x - p1->x;
	p.y = p2->y - p1->y;
	vector2_normalized(&p, &p);
	d = vector2_dot(&v, &p);
	p.x = p.x * d + p1->x;
	p.y = p.y * d + p1->y;
	
	if (p.x >= fmin(p1->x, p2->x) && p.x <= fmax(p1->x, p2->x) && p.y >= fmin(p1->y, p2->y) && p.y <= fmax(p1->y, p2->y)) {
		v.x = c->pivot.x - p.x;
		v.y = c->pivot.y - p.y;
		d = vector2_lensq(&v);
	}
	else {
		double d1, d2;
		v.x = c->pivot.x - p1->x;
		v.y = c->pivot.y - p1->y;
		d1 = vector2_lensq(&v);
		v.x = c->pivot.x - p2->x;
		v.y = c->pivot.y - p2->y;
		d2 = vector2_lensq(&v);
		d = fmin(d1, d2);
	}
	return d < c->radius*c->radius;
}

static double __projection(const struct shape2d_obb_t* p, const struct vector2_t pv[], const struct vector2_t* v) {
	return p->w * fabs(vector2_dot(v, &pv[0])) + p->h * fabs(vector2_dot(v, &pv[1]));
}
int shape2d_obb_is_overlap(const struct shape2d_obb_t* o1, const struct shape2d_obb_t* o2) {
	int i;
	struct vector2_t v = { o1->x - o2->x, o1->y - o2->y };
	struct vector2_t pv[4];
	// o1
	pv[0].x = cos(o1->radian);
	pv[0].y = sin(o1->radian);
	pv[1].x = -pv[0].y;
	pv[1].y = pv[0].x;
	// o2
	pv[2].x = cos(o2->radian);
	pv[2].y = sin(o2->radian);
	pv[3].x = -pv[2].y;
	pv[3].y = pv[2].x;
	for (i = 0; i < sizeof(pv) / sizeof(pv[0]); ++i) {
		double r1 = __projection(o1, &pv[0], &pv[i]);
		double r2 = __projection(o2, &pv[2], &pv[i]);
		double d = fabs(vector2_dot(&v, &pv[i]));
		if (r1 + r2 <= d)
			return 0;
	}
	return 1;
}

static int __cross(const struct vector2_t* vi, const struct vector2_t* vj, const struct vector2_t* point) {
	return	((vi->y > point->y) != (vj->y > point->y)) &&
		(point->x < (vj->x - vi->x) * (point->y - vi->y) / (vj->y - vi->y) + vi->x);
}
void shape2d_polygon_rotate(struct shape2d_polygon_t* c, double radian) {
	unsigned int i;
	for (i = 0; i < c->vertice_num; ++i) {
		double x = (c->vertices[i].x - c->pivot.x) * cos(radian) - (c->vertices[i].y - c->pivot.y) * sin(radian) + c->pivot.x;
		double y = (c->vertices[i].x - c->pivot.x) * sin(radian) + (c->vertices[i].y - c->pivot.y) * cos(radian) + c->pivot.y;
		c->vertices[i].x = x;
		c->vertices[i].y = y;
	}
	c->radian = radian;
}
int shape2d_polygon_has_point(const struct shape2d_polygon_t* c, struct vector2_t* point) {
	unsigned int i, j, b = 0;
	for (i = 0, j = c->vertice_num - 1; i < c->vertice_num; j = i++) {
		const struct vector2_t* vi = c->vertices + i;
		const struct vector2_t* vj = c->vertices + j;
		if (shape2d_linesegment_has_point(vi, vj, point))
			return 0;
		if (__cross(vi, vj, point))
			b = !b;
	}
	return b;
}
int shape2d_polygon_has_overlap(const struct shape2d_polygon_t* c1, const struct shape2d_polygon_t* c2) {
	int use_malloc = 0;
	unsigned int i, k, j = 0;
	unsigned int axes_num = c1->vertice_num + c2->vertice_num;
	struct vector2_t* axes;
	if (axes_num > 1400) {
		use_malloc = 1;
	}
	axes = use_malloc ? (struct vector2_t*)malloc(sizeof(struct vector2_t) * axes_num) : 
						(struct vector2_t*)alloca(sizeof(struct vector2_t) * axes_num);
	if (!axes) {
		return 0;
	}
	
	for (i = 0, k = c1->vertice_num - 1; i < c1->vertice_num; k = i++, ++j) {
		struct vector2_t v = { c1->vertices[k].x - c1->vertices[i].x, c1->vertices[k].y - c1->vertices[i].y};
		vector2_normalized(&v, &v);
		axes[j].x = -v.y;
		axes[j].y = v.x;
	}
	for (i = 0, k = c2->vertice_num - 1; i < c2->vertice_num; k = i++, ++j) {
		struct vector2_t v = { c2->vertices[k].x - c2->vertices[i].x, c2->vertices[k].y - c2->vertices[i].y };
		vector2_normalized(&v, &v);
		axes[j].x = -v.y;
		axes[j].y = v.x;
	}
	for (i = 0; i < axes_num; ++i) {
		double r;
		double min1 = DBL_MAX, max1 = -DBL_MAX, ret1;
		double min2 = DBL_MAX, max2 = -DBL_MAX, ret2;
		for (j = 0; j < c1->vertice_num; ++j) {
			ret1 = vector2_dot(c1->vertices + j, axes + i);
			min1 = min1 > ret1 ? ret1 : min1;
			max1 = max1 < ret1 ? ret1 : max1;
		}
		for (j = 0; j < c2->vertice_num; ++j) {
			ret2 = vector2_dot(c2->vertices + j, axes + i);
			min2 = min2 > ret2 ? ret2 : min2;
			max2 = max2 < ret2 ? ret2 : max2;
		}
		r = (max1 > max2 ? max1 : max2) - (min1 < min2 ? min1 : min2);
		if ((max1 - min1) + (max2 - min2) <= r) {
			if (use_malloc) {
				free(axes);
			}
			return 0;
		}
	}
	if (use_malloc) {
		free(axes);
	}
	return 1;
}
int shape2d_polygon_circle_has_overlap(const struct shape2d_polygon_t* c, const struct shape2d_circle_t* circle) {
	unsigned int i, j, b = 0;
	for (i = 0, j = c->vertice_num - 1; i < c->vertice_num; j = i++) {
		const struct vector2_t* vi = c->vertices + i;
		const struct vector2_t* vj = c->vertices + j;
		if (shape2d_circle_linesegment_has_overlap(circle, vi, vj))
			return 1;
		if (__cross(vi, vj, &circle->pivot))
			b = !b;
	}
	return b;
}
int shape2d_polygon_linesegment_has_overlap(const struct shape2d_polygon_t* c, const struct vector2_t* s, const struct vector2_t* e) {
	unsigned int i, j, b1 = 0, b2 = 0;
	for (i = 0, j = c->vertice_num - 1; i < c->vertice_num; j = i++) {
		const struct vector2_t* vi = c->vertices + i;
		const struct vector2_t* vj = c->vertices + j;
		if (shape2d_linesegment_has_intersect(vi, vj, s, e))
			return 1;
		if (__cross(vi, vj, s))
			b1 = !b1;
		if (__cross(vi, vj, e))
			b2 = !b2;
	}
	return b1 && b2;
}

#ifdef	__cplusplus
}
#endif
