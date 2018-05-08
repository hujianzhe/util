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

static shape3d_plane_t* shape3d_plane_build_by_point(shape3d_plane_t* plane, const vector3_t* p1, const vector3_t* p2, const vector3_t* p3) {
	vector3_t p12 = { p2->x - p1->x, p2->y - p1->y, p2->z - p1->z };
	vector3_t p13 = { p3->x - p1->x, p3->y - p1->y, p3->z - p1->z };
	vector3_cross(&plane->normal, &p12, &p13);
	if (vector3_is_zero(&plane->normal))
		return NULL;
	vector3_normalized(&plane->normal, &plane->normal);
	plane->pivot = *p1;
	return plane;
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

static int shape3d_linesegment_has_intersect(const vector3_t* s1, const vector3_t* e1, const vector3_t* s2, const vector3_t* e2) {
	int l1 = 0, l2 = 0;
	double min_x, min_y, min_z, max_x, max_y, max_z, d;
	vector3_t res1, res2;
	vector3_t s1e1 = { e1->x - s1->x, e1->y - s1->y, e1->z - s1->z };
	vector3_t s1s2 = { s2->x - s1->x, s2->y - s1->y, s2->z - s1->z };
	vector3_t s1e2 = { e2->x - s1->x, e2->y - s1->y, e2->z - s1->z };
	vector3_t s2e2 = { e2->x - s2->x, e2->y - s2->y, e2->z - s2->z };
	vector3_t s2s1 = { s1->x - s2->x, s1->y - s2->y, s1->z - s2->y };
	vector3_t s2e1 = { e1->x - s2->x, e1->y - s2->y, e1->z - s2->z };
	
	min_x = fmin(s1->x, e1->x);
	min_y = fmin(s1->y, e1->y);
	min_z = fmin(s1->z, e1->z);
	max_x = fmax(s1->x, e1->x);
	max_y = fmax(s1->y, e1->y);
	max_z = fmax(s1->z, e1->z);
	if (vector3_is_zero(vector3_cross(&res1, &s1e1, &s1s2))) {
		if (s2->x >= min_x && s2->x <= max_x &&
			s2->y >= min_y && s2->y <= max_y &&
			s2->z >= min_z && s2->z <= max_z)
		{
			return 1;
		}
		l1 = 1;
	}
	if (vector3_is_zero(vector3_cross(&res1, &s1e1, &s1e2))) {
		if (e2->x >= min_x && e2->x <= max_x &&
			e2->y >= min_y && e2->y <= max_y &&
			e2->z >= min_z && e2->z <= max_z)
		{
			return 1;
		}
		l2 = 1;
	}

	min_x = fmin(s2->x, e2->x);
	min_y = fmin(s2->y, e2->y);
	min_z = fmin(s2->z, e2->z);
	max_x = fmax(s2->x, e2->x);
	max_y = fmax(s2->y, e2->y);
	max_z = fmax(s2->z, e2->z);
	if (vector3_is_zero(vector3_cross(&res1, &s2e2, &s2s1)) &&
		s1->x >= min_x && s1->x <= max_x &&
		s1->y >= min_y && s1->y <= max_y &&
		s1->z >= min_z && s1->z <= max_z)
	{
		return 1;
	}
	if (vector3_is_zero(vector3_cross(&res1, &s2e2, &s2e1)) &&
		e1->x >= min_x && e1->x <= max_x &&
		e1->y >= min_y && e1->y <= max_y &&
		e1->z >= min_z && e1->z <= max_z)
	{
		return 1;
	}

	if (l1 && l2) {
		return 0;
	}
	else if (l1) {
		vector3_cross(&res1, &s1e1, &s1e2);
		vector3_normalized(&res1, &res1);
		d = vector3_dot(&res1, &s1s2);
	}
	else {
		vector3_cross(&res1, &s1e1, &s1s2);
		vector3_normalized(&res1, &res1);
		d = vector3_dot(&res1, &s1e2);
	}
	if (d <= -DBL_EPSILON || d >= DBL_EPSILON) {
		return 0;
	}
	return vector3_dot(vector3_cross(&res1, &s1s2, &s1e1), vector3_cross(&res2, &s1e2, &s1e1)) < 0
		&& vector3_dot(vector3_cross(&res1, &s2s1, &s2e2), vector3_cross(&res2, &s2e1, &s2e2)) < 0;
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