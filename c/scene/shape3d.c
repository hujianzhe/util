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
	return 0;
}

static unsigned int shape3d_linesegment_has_point_n(const shape3d_linesegment_t* ls, const vector3_t point[], unsigned int n) {
	unsigned int i;
	for (i = 0; i < n; ++i) {
		if (!shape3d_linesegment_has_point(&ls->vertices0, &ls->vertices1, point + i))
			break;
	}
	return i;
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

#ifdef	__cplusplus
}
#endif