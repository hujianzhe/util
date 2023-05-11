//
// Created by hujianzhe
//

#include "../../../inc/crt/math.h"
#include "../../../inc/crt/math_vec3.h"
#include "../../../inc/crt/geometry/sphere.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

int mathSphereHasPoint(const float o[3], float radius, const float p[3]) {
	float op[3];
	int cmp = fcmpf(mathVec3LenSq(mathVec3Sub(op, p, o)), radius * radius, CCT_EPSILON);
	if (cmp > 0) {
		return 0;
	}
	if (0 == cmp) {
		return 1;
	}
	else {
		return 2;
	}
}

int mathSphereIntersectSphere(const float o1[3], float r1, const float o2[3], float r2, float p[3]) {
	int cmp;
	float o1o2[3], radius_sum = r1 + r2;
	mathVec3Sub(o1o2, o2, o1);
	cmp = fcmpf(mathVec3LenSq(o1o2), radius_sum * radius_sum, CCT_EPSILON);
	if (cmp > 0) {
		return 0;
	}
	if (cmp < 0) {
		return 2;
	}
	if (p) {
		mathVec3Normalized(o1o2, o1o2);
		mathVec3AddScalar(mathVec3Copy(p, o1), o1o2, r1);
	}
	return 1;
}

int mathSphereContainSphere(const float o1[3], float r1, const float o2[3], float r2) {
	float o1o2[3], d;
	if (r1 < r2) {
		return 0;
	}
	mathVec3Sub(o1o2, o2, o1);
	d = mathVec3LenSq(o1o2);
	if (d <= CCT_EPSILON) {
		return r2 <= r1;
	}
	if (d >= r1 * r1) {
		return 0;
	}
	d = sqrtf(d);
	return d + r2 <= r1;
}

#ifdef __cplusplus
}
#endif
