//
// Created by hujianzhe
//

#include "../../../inc/crt/math.h"
#include "../../../inc/crt/math_vec3.h"
#include "../../../inc/crt/geometry/aabb.h"
#include <stddef.h>

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

void mathPlaneNormalByVertices3(const float vertices[3][3], float normal[3]) {
	float v0v1[3], v0v2[3];
	mathVec3Sub(v0v1, vertices[1], vertices[0]);
	mathVec3Sub(v0v2, vertices[2], vertices[0]);
	mathVec3Cross(normal, v0v1, v0v2);
	mathVec3Normalized(normal, normal);
}

int mathPlaneHasPoint(const float plane_v[3], const float plane_normal[3], const float p[3]) {
	float v[3];
	mathVec3Sub(v, plane_v, p);
	return fcmpf(mathVec3Dot(plane_normal, v), 0.0f, CCT_EPSILON) == 0;
}

int mathPlaneIntersectPlane(const float v1[3], const float n1[3], const float v2[3], const float n2[3]) {
	float n[3];
	mathVec3Cross(n, n1, n2);
	if (!mathVec3IsZero(n)) {
		return 1;
	}
	return mathPlaneHasPoint(v1, n1, v2);
}

int mathRectHasPoint(const GeometryRect_t* rect, const float p[3]) {
	float v[3], dot;
	mathVec3Sub(v, p, rect->o);
	dot = mathVec3Dot(rect->normal, v);
	if (fcmpf(dot, 0.0f, CCT_EPSILON)) {
		return 0;
	}
	dot = mathVec3Dot(rect->h_axis, v);
	if (dot > rect->half_h + CCT_EPSILON || dot < -rect->half_h - CCT_EPSILON) {
		return 0;
	}
	return mathVec3LenSq(v) - dot * dot <= rect->half_w * rect->half_w + CCT_EPSILON;
}

void mathRectVertices(const GeometryRect_t* rect, float p[4][3]) {
	float w_axis[3];
	mathVec3Cross(w_axis, rect->h_axis, rect->normal);
	//mathVec3Normalized(w_axis, w_axis);

	mathVec3Copy(p[0], rect->o);
	mathVec3AddScalar(p[0], rect->h_axis, rect->half_h);
	mathVec3AddScalar(p[0], w_axis, rect->half_w);
	mathVec3Copy(p[1], rect->o);
	mathVec3AddScalar(p[1], rect->h_axis, rect->half_h);
	mathVec3AddScalar(p[1], w_axis, -rect->half_w);
	mathVec3Copy(p[2], rect->o);
	mathVec3AddScalar(p[2], rect->h_axis, -rect->half_h);
	mathVec3AddScalar(p[2], w_axis, -rect->half_w);
	mathVec3Copy(p[3], rect->o);
	mathVec3AddScalar(p[3], rect->h_axis, -rect->half_h);
	mathVec3AddScalar(p[3], w_axis, rect->half_w);
}

#ifdef __cplusplus
}
#endif
