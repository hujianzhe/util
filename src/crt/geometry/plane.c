//
// Created by hujianzhe
//

#include "../../../inc/crt/math_vec3.h"
#include "../../../inc/crt/geometry/plane.h"

#ifdef __cplusplus
extern "C" {
#endif

void mathPointProjectionPlane(const float p[3], const float plane_v[3], const float plane_n[3], float np[3], float* distance) {
	float pv[3], d;
	mathVec3Sub(pv, plane_v, p);
	d = mathVec3Dot(pv, plane_n);
	if (distance) {
		*distance = d;
	}
	if (np) {
		mathVec3AddScalar(mathVec3Copy(np, p), plane_n, d);
	}
}

float mathPlaneNormalByVertices3(const float v0[3], const float v1[3], const float v2[3], float normal[3]) {
	float v0v1[3], v0v2[3];
	mathVec3Sub(v0v1, v1, v0);
	mathVec3Sub(v0v2, v2, v0);
	mathVec3Cross(normal, v0v1, v0v2);
	return mathVec3Normalized(normal, normal);
}

int mathPlaneHasPoint(const float plane_v[3], const float plane_normal[3], const float p[3]) {
	float v[3], dot;
	mathVec3Sub(v, plane_v, p);
	dot = mathVec3Dot(plane_normal, v);
	return CCT_EPSILON_NEGATE <= dot && dot <= CCT_EPSILON;
}

int mathPlaneIntersectPlane(const float v1[3], const float n1[3], const float v2[3], const float n2[3]) {
	float n[3];
	mathVec3Cross(n, n1, n2);
	if (!mathVec3IsZero(n)) {
		return 1;
	}
	return mathPlaneHasPoint(v1, n1, v2) ? 2 : 0;
}

#ifdef __cplusplus
}
#endif
