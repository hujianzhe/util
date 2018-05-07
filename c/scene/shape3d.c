//
// Created by hujianzhe on 18-4-30.
//

#include "shape3d.h"
#include <errno.h>
#include <float.h>
#if defined(_WIN32) || defined(_WIN64)
	#ifndef	_USE_MATH_DEFINES
		#define	_USE_MATH_DEFINES
	#endif
#endif
#include <math.h>

#ifdef	__cplusplus
extern "C" {
#endif

static int shape3d_linesegment_has_point(const vector3_t* s, const vector3_t* e, const vector3_t* p) {
	vector3_t res;
	vector3_t se = { e->x - s->x, e->y - s->y, e->z - s->z };
	vector3_t sp = { p->x - s->x, p->y - s->y, p->z - s->z };
	if (!vector3_is_zero(vector3_cross(&res, &se, &sp)))
		return 0;
	return p->x >= fmin(s->x, e->x) && p->x <= fmax(s->x, e->x)
		&& p->y >= fmin(s->y, e->y) && p->y <= fmax(s->y, e->y)
		&& p->z >= fmin(s->z, e->z) && p->z <= fmax(s->z, e->z);
}

static unsigned int shape3d_linesegment_has_point_n(const shape3d_linesegment_t* ls, const vector3_t point[], unsigned int n) {
	unsigned int i;
	for (i = 0; i < n; ++i) {
		if (!shape3d_linesegment_has_point(&ls->vertices0, &ls->vertices1, point + i))
			break;
	}
	return i;
}

static int shape3d_plane_has_point(const shape3d_plane_t* plane, const vector3_t* point) {
	vector3_t v = { point->x - plane->pivot.x, point->y - plane->pivot.y, point->z - plane->pivot.z };
	double d = vector3_dot(&plane->normal, point);
	return d > -DBL_EPSILON && d < DBL_EPSILON;
}

static unsigned int shape3d_plane_has_point_n(const shape3d_plane_t* plane, const vector3_t point[], unsigned int n) {
	unsigned int i;
	for (i = 0; i < n; ++i) {
		if (!shape3d_plane_has_point(plane, point + i))
			break;
	}
	return i;
}

static unsigned int shape3d_plane_has_contain_linesegment(const shape3d_plane_t* plane, const shape3d_linesegment_t* ls) {
	return shape3d_plane_has_point_n(plane, &ls->vertices0, 2) == 2;
}

static int shape3d_aabb_has_point(const shape3d_aabb_t* aabb, const vector3_t* point) {
	return aabb->pivot.x - aabb->half.x <= point->x
		&& aabb->pivot.x + aabb->half.x >= point->x
		&& aabb->pivot.y - aabb->half.y <= point->y
		&& aabb->pivot.y + aabb->half.y >= point->y
		&& aabb->pivot.z - aabb->half.z <= point->z
		&& aabb->pivot.z + aabb->half.z >= point->z;
}

static unsigned int shape3d_aabb_has_point_n(const shape3d_aabb_t* aabb, const vector3_t point[], unsigned int n) {
	unsigned int i;
	for (i = 0; i < n; ++i) {
		if (!shape3d_aabb_has_point(aabb, point + i))
			break;
	}
	return i;
}

static int shape3d_sphere_has_point(const shape3d_sphere_t* sphere, const vector3_t* point) {
	vector3_t v = { point->x - sphere->pivot.x, point->y - sphere->pivot.y, point->z - sphere->pivot.z };
	return vector3_lensq(&v) <= sphere->radius * sphere->radius;
}

static unsigned int shape3d_sphere_has_point_n(const shape3d_sphere_t* sphere, const vector3_t point[], unsigned int n) {
	unsigned int i;
	for (i = 0; i < n; ++i) {
		if (!shape3d_sphere_has_point(sphere, point + i))
			break;
	}
	return i;
}

static int shape3d_cylinder_has_point(const shape3d_cylinder_t* cylinder, const vector3_t* point) {
	double d;
	vector3_t v = { point->x - cylinder->pivot0.x, point->y - cylinder->pivot0.y, point->z - cylinder->pivot0.z };
	vector3_t p = { cylinder->pivot1.x - cylinder->pivot0.x, cylinder->pivot1.y - cylinder->pivot0.y, cylinder->pivot1.z - cylinder->pivot0.z };
	vector3_normalized(&p, &p);
	d = vector3_dot(&p, &v);
	p.x = p.x * d + cylinder->pivot0.x;
	p.y = p.y * d + cylinder->pivot0.y;
	p.z = p.z * d + cylinder->pivot0.z;
	if (point->x >= fmin(cylinder->pivot0.x, cylinder->pivot1.x) &&
		point->x <= fmax(cylinder->pivot0.x, cylinder->pivot1.x) &&
		point->y >= fmin(cylinder->pivot0.y, cylinder->pivot1.y) &&
		point->y <= fmax(cylinder->pivot0.y, cylinder->pivot1.y) &&
		point->z >= fmin(cylinder->pivot0.z, cylinder->pivot1.z) &&
		point->z <= fmax(cylinder->pivot0.z, cylinder->pivot1.z))
	{
		v.x = point->x - p.x;
		v.y = point->y - p.y;
		v.z = point->z - p.z;
		return vector3_lensq(&v) <= cylinder->radius * cylinder->radius;
	}
	return 0;
}

static unsigned int shape3d_cylinder_has_point_n(const shape3d_cylinder_t* cylinder, const vector3_t point[], unsigned int n) {
	unsigned int i;
	for (i = 0; i < n; ++i) {
		if (!shape3d_cylinder_has_point(cylinder, point + i))
			break;
	}
	return i;
}

#ifdef	__cplusplus
}
#endif