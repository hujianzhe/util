//
// Created by hujianzhe
//

#include "../../../inc/crt/math.h"
#include "../../../inc/crt/math_vec3.h"
#include "../../../inc/crt/geometry/plane.h"
#include "../../../inc/crt/geometry/triangle.h"
#include <stddef.h>

static const unsigned int DEFAULT_TRIANGLE_POLYGEN_VERTICE[3] = { 0, 1, 2 };
static const unsigned int DEFAULT_RECT_POLYGEN_VERTICE[4] = { 0, 1, 2, 3 };

#ifdef __cplusplus
extern "C" {
#endif

void mathTriangleGetPoint(const float tri[3][3], float u, float v, float p[3]) {
	float v0[3], v1[3], v2[3];
	mathVec3MultiplyScalar(v0, tri[0], 1.0f - u - v);
	mathVec3MultiplyScalar(v1, tri[1], u);
	mathVec3MultiplyScalar(v2, tri[2], v);
	mathVec3Add(p, mathVec3Add(p, v0, v1), v2);
}

int mathTrianglePointUV(const float tri[3][3], const float p[3], float* p_u, float* p_v) {
	float ap[3], ab[3], ac[3], N[3];
	mathVec3Sub(ap, p, tri[0]);
	mathVec3Sub(ab, tri[1], tri[0]);
	mathVec3Sub(ac, tri[2], tri[0]);
	mathVec3Cross(N, ab, ac);
	if (fcmpf(mathVec3Dot(N, ap), 0.0f, CCT_EPSILON)) {
		return 0;
	}
	else {
		float u, v;
		float dot_ac_ac = mathVec3Dot(ac, ac);
		float dot_ac_ab = mathVec3Dot(ac, ab);
		float dot_ac_ap = mathVec3Dot(ac, ap);
		float dot_ab_ab = mathVec3Dot(ab, ab);
		float dot_ab_ap = mathVec3Dot(ab, ap);
		float tmp = 1.0f / (dot_ac_ac * dot_ab_ab - dot_ac_ab * dot_ac_ab);
		u = (dot_ab_ab * dot_ac_ap - dot_ac_ab * dot_ab_ap) * tmp;
		if (fcmpf(u, 0.0f, CCT_EPSILON) < 0 || fcmpf(u, 1.0f, CCT_EPSILON) > 0) {
			return 0;
		}
		v = (dot_ac_ac * dot_ab_ap - dot_ac_ab * dot_ac_ap) * tmp;
		if (fcmpf(v, 0.0f, CCT_EPSILON) < 0 || fcmpf(v + u, 1.0f, CCT_EPSILON) > 0) {
			return 0;
		}
		if (p_u) {
			*p_u = u;
		}
		if (p_v) {
			*p_v = v;
		}
		return 1;
	}
}

int mathTriangleHasPoint(const float tri[3][3], const float p[3]) {
	return mathTrianglePointUV(tri, p, NULL, NULL);
}

void mathTriangleToPolygen(const float tri[3][3], GeometryPolygen_t* polygen) {
	polygen->v_indices = DEFAULT_TRIANGLE_POLYGEN_VERTICE;
	polygen->v_indices_cnt = 3;
	polygen->v = tri;
	mathPlaneNormalByVertices3(tri, polygen->normal);
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
	mathVec3Copy(p[0], rect->o);
	mathVec3AddScalar(p[0], rect->h_axis, rect->half_h);
	mathVec3AddScalar(p[0], rect->w_axis, rect->half_w);
	mathVec3Copy(p[1], rect->o);
	mathVec3AddScalar(p[1], rect->h_axis, rect->half_h);
	mathVec3AddScalar(p[1], rect->w_axis, -rect->half_w);
	mathVec3Copy(p[2], rect->o);
	mathVec3AddScalar(p[2], rect->h_axis, -rect->half_h);
	mathVec3AddScalar(p[2], rect->w_axis, -rect->half_w);
	mathVec3Copy(p[3], rect->o);
	mathVec3AddScalar(p[3], rect->h_axis, -rect->half_h);
	mathVec3AddScalar(p[3], rect->w_axis, rect->half_w);
}

void mathRectToPolygen(const GeometryRect_t* rect, GeometryPolygen_t* polygen, float p[4][3]) {
	mathRectVertices(rect, p);
	polygen->v_indices = DEFAULT_RECT_POLYGEN_VERTICE;
	polygen->v_indices_cnt = 4;
	polygen->v = (const float(*)[3])p;
	mathVec3Copy(polygen->normal, rect->normal);
}

int mathPolygenHasPoint(const GeometryPolygen_t* polygen, const float p[3]) {
	if (polygen->v_indices_cnt < 3) {
		return 0;
	}
	else if (3 == polygen->v_indices_cnt) {
		float tri[3][3];
		mathVec3Copy(tri[0], polygen->v[polygen->v_indices[0]]);
		mathVec3Copy(tri[1], polygen->v[polygen->v_indices[1]]);
		mathVec3Copy(tri[2], polygen->v[polygen->v_indices[2]]);
		return mathTrianglePointUV((const float(*)[3])tri, p, NULL, NULL);
	}
	else if (polygen->v_indices == DEFAULT_RECT_POLYGEN_VERTICE) {
		float ls_vec[3], v[3], dot;
		mathVec3Sub(v, p, polygen->v[polygen->v_indices[0]]);
		dot = mathVec3Dot(polygen->normal, v);
		if (dot < -CCT_EPSILON || dot > CCT_EPSILON) {
			return 0;
		}
		mathVec3Sub(ls_vec, polygen->v[polygen->v_indices[1]], polygen->v[polygen->v_indices[0]]);
		dot = mathVec3Dot(ls_vec, v);
		if (dot < -CCT_EPSILON || dot > mathVec3LenSq(ls_vec)) {
			return 0;
		}
		mathVec3Sub(ls_vec, polygen->v[polygen->v_indices[3]], polygen->v[polygen->v_indices[0]]);
		dot = mathVec3Dot(ls_vec, v);
		if (dot < -CCT_EPSILON || dot > mathVec3LenSq(ls_vec)) {
			return 0;
		}
		return 1;
	}
	else {
		unsigned int i;
		float v[3], dot;
		mathVec3Sub(v, polygen->v[polygen->v_indices[0]], p);
		dot = mathVec3Dot(polygen->normal, v);
		if (dot < -CCT_EPSILON || dot > CCT_EPSILON) {
			return 0;
		}
		for (i = 2; i < polygen->v_indices_cnt; ++i) {
			float tri[3][3];
			mathVec3Copy(tri[0], polygen->v[polygen->v_indices[0]]);
			mathVec3Copy(tri[1], polygen->v[polygen->v_indices[i - 1]]);
			mathVec3Copy(tri[2], polygen->v[polygen->v_indices[i]]);
			if (mathTrianglePointUV((const float(*)[3])tri, p, NULL, NULL)) {
				return 1;
			}
		}
	}
	return 0;
}

#ifdef __cplusplus
}
#endif
