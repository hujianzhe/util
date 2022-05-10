//
// Created by hujianzhe
//

#include "../../../inc/crt/math.h"
#include "../../../inc/crt/math_vec3.h"
#include "../../../inc/crt/geometry/collision_detection.h"
#include <stddef.h>

typedef struct GeometryPolygenInner_t {
	const GeometryPolygen_t* polygen;
	const GeometryRect_t* rect;
} GeometryPolygenInner_t;

#ifdef __cplusplus
extern "C" {
#endif

static CCTResult_t* copy_result(CCTResult_t* dst, CCTResult_t* src) {
	if (dst == src) {
		return dst;
	}
	dst->distance = src->distance;
	dst->hit_point_cnt = src->hit_point_cnt;
	if (1 == src->hit_point_cnt) {
		mathVec3Copy(dst->hit_point, src->hit_point);
	}
	mathVec3Copy(dst->hit_normal, src->hit_normal);
	return dst;
}

static CCTResult_t* set_result(CCTResult_t* result, float distance, const float hit_point[3], const float hit_normal[3]) {
	result->distance = distance;
	if (hit_point) {
		mathVec3Copy(result->hit_point, hit_point);
		result->hit_point_cnt = 1;
	}
	else {
		result->hit_point_cnt = -1;
	}
	if (hit_normal) {
		mathVec3Copy(result->hit_normal, hit_normal);
	}
	else {
		mathVec3Set(result->hit_normal, 0.0f, 0.0f, 0.0f);
	}
	return result;
}

static int polygen_inner_has_point(const GeometryPolygenInner_t* inner, const float p[3]) {
	volatile int* debug_ptr = (int*)0;
	if (inner->rect) {
		return mathRectHasPoint(inner->rect, p);
	}
	else if (3 == inner->polygen->v_cnt) {
		return mathTriangleHasPoint(inner->polygen->v, p);
	}
	// not implement ...
	*debug_ptr = 0;
	return 0;
}

/*
static int mathCapsuleHasPoint(const float o[3], const float axis[3], float radius, float half_height, const float p[3]) {
	float cp[3], v[3], dot;
	mathVec3AddScalar(mathVec3Copy(cp, o), axis, -half_height);
	mathVec3Sub(v, p, cp);
	dot = mathVec3Dot(axis, v);
	if (fcmpf(dot, 0.0f, CCT_EPSILON) < 0) {
		return mathSphereHasPoint(cp, radius, p);
	}
	else if (fcmpf(dot, half_height + half_height, CCT_EPSILON) > 0) {
		mathVec3AddScalar(cp, axis, half_height + half_height);
		return mathSphereHasPoint(cp, radius, p);
	}
	else {
		int cmp = fcmpf(mathVec3LenSq(v) - dot * dot, radius * radius, CCT_EPSILON);
		if (cmp > 0)
			return 0;
		return cmp < 0 ? 2 : 1;
	}
}
*/

static int mathSegmentIntersectPlane(const float ls[2][3], const float plane_v[3], const float plane_normal[3], float p[3]) {
	int cmp[2];
	float d[2], lsdir[3], dot;
	mathVec3Sub(lsdir, ls[1], ls[0]);
	dot = mathVec3Dot(lsdir, plane_normal);
	if (fcmpf(dot, 0.0f, CCT_EPSILON) == 0) {
		return mathPlaneHasPoint(plane_v, plane_normal, ls[0]) ? 2 : 0;
	}
	mathPointProjectionPlane(ls[0], plane_v, plane_normal, NULL, &d[0]);
	mathPointProjectionPlane(ls[1], plane_v, plane_normal, NULL, &d[1]);
	cmp[0] = fcmpf(d[0], 0.0f, CCT_EPSILON);
	cmp[1] = fcmpf(d[1], 0.0f, CCT_EPSILON);
	if (cmp[0] * cmp[1] > 0) {
		return 0;
	}
	if (0 == cmp[0]) {
		if (p) {
			mathVec3Copy(p, ls[0]);
		}
		return 1;
	}
	if (0 == cmp[1]) {
		if (p) {
			mathVec3Copy(p, ls[1]);
		}
		return 1;
	}
	if (p) {
		mathVec3Normalized(lsdir, lsdir);
		dot = mathVec3Dot(lsdir, plane_normal);
		mathVec3AddScalar(mathVec3Copy(p, ls[0]), lsdir, d[0] / dot);
	}
	return 1;
}

static int mathSegmentIntersectPolygen(const float ls[2][3], const GeometryPolygenInner_t* gp, float p[3]) {
	int res;
	float point[3];
	const GeometryPolygen_t* polygen = gp->polygen;
	if (!p) {
		p = point;
	}
	res = mathSegmentIntersectPlane(ls, polygen->v[0], polygen->normal, p);
	if (0 == res) {
		return 0;
	}
	if (1 == res) {
		return polygen_inner_has_point(gp, p);
	}
	if (polygen_inner_has_point(gp, ls[0]) || polygen_inner_has_point(gp, ls[1])) {
		return 2;
	}
	else {
		int i;
		for (i = 0; i < polygen->v_cnt; ++i) {
			float edge[2][3];
			mathVec3Copy(edge[0], polygen->v[i]);
			mathVec3Copy(edge[1], polygen->v[(i+1) >= polygen->v_cnt ? 0 : i+1]);
			if (mathSegmentIntersectSegment(ls, (const float(*)[3])edge, NULL, NULL)) {
				return 2;
			}
		}
		return 0;
	}
}

static int mathPolygenIntersectPolygen(const GeometryPolygenInner_t* gp1, const GeometryPolygenInner_t* gp2) {
	int i;
	const GeometryPolygen_t* polygen1 = gp1->polygen;
	const GeometryPolygen_t* polygen2 = gp2->polygen;
	if (!mathPlaneIntersectPlane(polygen1->v[0], polygen1->normal, polygen2->v[0], polygen2->normal)) {
		return 0;
	}
	for (i = 0; i < polygen1->v_cnt; ++i) {
		float edge[2][3];
		mathVec3Copy(edge[0], polygen1->v[i]);
		mathVec3Copy(edge[1], polygen1->v[(i+1) >= polygen1->v_cnt ? 0 : i+1]);
		if (mathSegmentIntersectPolygen((const float(*)[3])edge, gp2, NULL)) {
			return 1;
		}
	}
	return 0;
}

static int mathPolygenIntersectPlane(const GeometryPolygenInner_t* gp, const float plane_v[3], const float plane_n[3], float p[3]) {
	int i, has_gt0, has_le0, idx_0;
	const GeometryPolygen_t* polygen = gp->polygen;
	if (!mathPlaneIntersectPlane(polygen->v[0], polygen->normal, plane_v, plane_n)) {
		return 0;
	}
	idx_0 = -1;
	has_gt0 = has_le0 = 0;
	for (i = 0; i < polygen->v_cnt; ++i) {
		int cmp;
		float d;
		mathPointProjectionPlane(polygen->v[i], plane_v, plane_n, NULL, &d);
		cmp = fcmpf(d, 0.0f, CCT_EPSILON);
		if (cmp > 0) {
			if (has_le0) {
				return 2;
			}
			has_gt0 = 1;
		}
		else if (cmp < 0) {
			if (has_gt0) {
				return 2;
			}
			has_le0 = 1;
		}
		else if (idx_0 >= 0) {
			return 2;
		}
		else {
			idx_0 = i;
		}
	}
	if (idx_0 < 0) {
		return 0;
	}
	if (p) {
		mathVec3Copy(p, polygen->v[idx_0]);
	}
	return 1;
}

static int mathSegmentIntersectRect(const float ls[2][3], const GeometryRect_t* rect, float p[3]) {
	GeometryPolygen_t polygen;
	GeometryPolygenInner_t gp = { &polygen, rect };
	float v[4][3];
	mathRectVertices(rect, v);
	polygen.v_cnt = 4;
	polygen.v = (const float(*)[3])v;
	mathVec3Copy(polygen.normal, rect->normal);
	return mathSegmentIntersectPolygen(ls, &gp, p);
}
/*
static int mathSegmentIntersectTriangle(const float ls[2][3], const float tri[3][3], float p[3]) {
	GeometryPolygen_t polygen;
	GeometryPolygenInner_t gp = { &polygen, NULL };
	polygen.v_cnt = 3;
	polygen.v = tri;
	mathPlaneNormalByVertices3(tri, polygen.normal);
	return mathSegmentIntersectPolygen(ls, &gp, p);
}
*/
static int mathRectIntersectRect(const GeometryRect_t* rect1, const GeometryRect_t* rect2) {
	GeometryPolygen_t polygen1, polygen2;
	GeometryPolygenInner_t gp1 = { &polygen1, rect1 }, gp2 = { &polygen2, rect2 };
	float v1[4][3], v2[4][3];
	mathRectVertices(rect1, v1);
	polygen1.v_cnt = 4;
	polygen1.v = (const float(*)[3])v1;
	mathVec3Copy(polygen1.normal, rect1->normal);
	mathRectVertices(rect2, v2);
	polygen2.v_cnt = 4;
	polygen2.v = (const float(*)[3])v2;
	mathVec3Copy(polygen2.normal, rect2->normal);
	return mathPolygenIntersectPolygen(&gp1, &gp2);
}
/*
static int mathTriangleIntersectTriangle(const float tri1[3][3], const float tri2[3][3]) {
	GeometryPolygen_t polygen1, polygen2;
	GeometryPolygenInner_t gp1 = { &polygen1, NULL }, gp2 = { &polygen2, NULL };
	polygen1.v_cnt = 3;
	polygen1.v = tri1;
	mathPlaneNormalByVertices3(tri1, polygen1.normal);
	polygen2.v_cnt = 3;
	polygen2.v = tri2;
	mathPlaneNormalByVertices3(tri2, polygen2.normal);
	return mathPolygenIntersectPolygen(&gp1, &gp2);
}
*/
static int mathRectIntersectPlane(const GeometryRect_t* rect, const float plane_v[3], const float plane_n[3], float p[3]) {
	GeometryPolygen_t polygen;
	GeometryPolygenInner_t gp = { &polygen, rect };
	float v[4][3];
	mathRectVertices(rect, v);
	polygen.v_cnt = 4;
	polygen.v = (const float(*)[3])v;
	mathVec3Copy(polygen.normal, rect->normal);
	return mathPolygenIntersectPlane(&gp, plane_v, plane_n, p);
}
/*
static int mathTriangleIntersectPlane(const float tri[3][3], const float plane_v[3], const float plane_n[3], float p[3]) {
	GeometryPolygen_t polygen;
	GeometryPolygenInner_t gp = { &polygen, NULL };
	polygen.v_cnt = 3;
	polygen.v = tri;
	mathPlaneNormalByVertices3(tri, polygen.normal);
	return mathPolygenIntersectPlane(&gp, plane_v, plane_n, p);
}
*/
static int mathSphereIntersectLine(const float o[3], float radius, const float ls_vertice[3], const float lsdir[3], float distance[2]) {
	int cmp;
	float vo[3], lp[3], lpo[3], lpolensq, radiussq, dot;
	mathVec3Sub(vo, o, ls_vertice);
	dot = mathVec3Dot(vo, lsdir);
	mathVec3AddScalar(mathVec3Copy(lp, ls_vertice), lsdir, dot);
	mathVec3Sub(lpo, o, lp);
	lpolensq = mathVec3LenSq(lpo);
	radiussq = radius * radius;
	cmp = fcmpf(lpolensq, radiussq, CCT_EPSILON);
	if (cmp > 0) {
		return 0;
	}
	else if (0 == cmp) {
		distance[0] = distance[1] = dot;
		return 1;
	}
	else {
		float d = sqrtf(radiussq - lpolensq);
		distance[0] = dot + d;
		distance[1] = dot - d;
		return 2;
	}
}

static int mathSphereIntersectSegment(const float o[3], float radius, const float ls[2][3], float p[3]) {
	int res;
	float closest_p[3], closest_v[3];
	mathSegmentClosestPointTo(ls, o, closest_p);
	mathVec3Sub(closest_v, closest_p, o);
	res = fcmpf(mathVec3LenSq(closest_v), radius * radius, CCT_EPSILON);
	if (res == 0) {
		if (p) {
			mathVec3Copy(p, closest_p);
		}
		return 1;
	}
	return res < 0 ? 2 : 0;
}

static int mathSphereIntersectPlane(const float o[3], float radius, const float plane_v[3], const float plane_normal[3], float new_o[3], float* new_r) {
	int cmp;
	float pp[3], ppd, ppo[3], ppolensq;
	mathPointProjectionPlane(o, plane_v, plane_normal, pp, &ppd);
	mathVec3Sub(ppo, o, pp);
	ppolensq = mathVec3LenSq(ppo);
	cmp = fcmpf(ppolensq, radius * radius, CCT_EPSILON);
	if (cmp > 0) {
		return 0;
	}
	if (new_o) {
		mathVec3Copy(new_o, pp);
	}
	if (0 == cmp) {
		if (new_r) {
			*new_r = 0.0f;
		}
		return 1;
	}
	else {
		if (new_r) {
			*new_r = sqrtf(radius * radius - ppolensq);
		}
		return 2;
	}
}

static int mathSphereIntersectPolygen(const float o[3], float radius, const GeometryPolygenInner_t* gp, float p[3]) {
	int res, i;
	const GeometryPolygen_t* polygen = gp->polygen;
	float point[3];
	if (!p) {
		p = point;
	}
	res = mathSphereIntersectPlane(o, radius, polygen->v[0], polygen->normal, p, NULL);
	if (0 == res) {
		return 0;
	}
	if (polygen_inner_has_point(gp, p)) {
		return res;
	}
	for (i = 0; i < polygen->v_cnt; ++i) {
		float edge[2][3];
		mathVec3Copy(edge[0], polygen->v[i]);
		mathVec3Copy(edge[1], polygen->v[(i+1) >= polygen->v_cnt ? 0 : i+1]);
		res = mathSphereIntersectSegment(o, radius, (const float(*)[3])edge, p);
		if (res != 0) {
			return res;
		}
	}
	return 0;
}

static int mathSphereIntersectRect(const float o[3], float radius, const GeometryRect_t* rect, float p[3]) {
	GeometryPolygen_t polygen;
	GeometryPolygenInner_t gp = { &polygen, rect };
	float v[4][3];
	mathRectVertices(rect, v);
	polygen.v_cnt = 4;
	polygen.v = (const float(*)[3])v;
	mathVec3Copy(polygen.normal, rect->normal);
	return mathSphereIntersectPolygen(o, radius, &gp, p);
}
/*
static int mathSphereIntersectTriangle(const float o[3], float radius, const float tri[3][3], float p[3]) {
	GeometryPolygen_t polygen;
	GeometryPolygenInner_t gp = { &polygen, NULL };
	polygen.v_cnt = 3;
	polygen.v = tri;
	mathPlaneNormalByVertices3(tri, polygen.normal);
	return mathSphereIntersectPolygen(o, radius, &gp, p);
}
*/
/*
static int mathSphereIntersectCapsule(const float sp_o[3], float sp_radius, const float cp_o[3], const float cp_axis[3], float cp_radius, float cp_half_height, float p[3]) {
	float v[3], cp[3], dot;
	mathVec3AddScalar(mathVec3Copy(cp, cp_o), cp_axis, -cp_half_height);
	mathVec3Sub(v, sp_o, cp);
	dot = mathVec3Dot(cp_axis, v);
	if (fcmpf(dot, 0.0f, CCT_EPSILON) < 0) {
		return mathSphereIntersectSphere(sp_o, sp_radius, cp, cp_radius, p);
	}
	else if (fcmpf(dot, cp_half_height + cp_half_height, CCT_EPSILON) > 0) {
		mathVec3AddScalar(cp, cp_axis, cp_half_height + cp_half_height);
		return mathSphereIntersectSphere(sp_o, sp_radius, cp, cp_radius, p);
	}
	else {
		float radius_sum = sp_radius + cp_radius;
		int cmp = fcmpf(mathVec3LenSq(v) - dot * dot, radius_sum * radius_sum, CCT_EPSILON);
		if (cmp > 0)
			return 0;
		if (cmp < 0)
			return 2;
		else {
			if (p) {
				float pl[3];
				mathVec3AddScalar(mathVec3Copy(pl, cp), cp_axis, dot);
				mathVec3Sub(v, pl, sp_o);
				mathVec3Normalized(v, v);
				mathVec3AddScalar(mathVec3Copy(p, sp_o), v, sp_radius);
			}
			return 1;
		}
	}
}
*/

static int mathAABBIntersectPlane(const float o[3], const float half[3], const float plane_v[3], const float plane_n[3], float p[3]) {
	int i, has_gt0 = 0, has_le0 = 0, idx_0 = -1;
	float vertices[8][3];
	mathAABBVertices(o, half, vertices);
	for (i = 0; i < 8; ++i) {
		int cmp;
		float d;
		mathPointProjectionPlane(vertices[i], plane_v, plane_n, NULL, &d);
		cmp = fcmpf(d, 0.0f, CCT_EPSILON);
		if (cmp > 0) {
			if (has_le0) {
				return 2;
			}
			has_gt0 = 1;
		}
		else if (cmp < 0) {
			if (has_gt0) {
				return 2;
			}
			has_le0 = 1;
		}
		else if (idx_0 >= 0) {
			return 2;
		}
		else {
			idx_0 = i;
		}
	}
	if (idx_0 < 0) {
		return 0;
	}
	if (p) {
		mathVec3Copy(p, vertices[idx_0]);
	}
	return 1;
}

static int mathAABBIntersectSphere(const float aabb_o[3], const float aabb_half[3], const float sp_o[3], float sp_radius) {
	float closest_v[3];
	mathAABBClosestPointTo(aabb_o, aabb_half, sp_o, closest_v);
	mathVec3Sub(closest_v, closest_v, sp_o);
	return mathVec3LenSq(closest_v) <= sp_radius * sp_radius + CCT_EPSILON;
}

static int mathLineIntersectCylinderInfinite(const float ls_v[3], const float lsdir[3], const float cp[3], const float axis[3], float radius, float distance[2]) {
	float new_o[3], new_dir[3], radius_sq = radius * radius;
	float new_axies[3][3];
	float A, B, C;
	int rcnt;
	mathVec3Copy(new_axies[2], axis);
	new_axies[1][0] = 0.0f;
	new_axies[1][1] = -new_axies[2][2];
	new_axies[1][2] = new_axies[2][1];
	if (mathVec3IsZero(new_axies[1])) {
		new_axies[1][0] = -new_axies[2][2];
		new_axies[1][1] = 0.0f;
		new_axies[1][2] = new_axies[2][0];
	}
	mathVec3Normalized(new_axies[1], new_axies[1]);
	mathVec3Cross(new_axies[0], new_axies[1], new_axies[2]);
	mathCoordinateSystemTransformPoint(ls_v, cp, new_axies, new_o);
	mathCoordinateSystemTransformNormalVec3(lsdir, new_axies, new_dir);
	A = new_dir[0] * new_dir[0] + new_dir[1] * new_dir[1];
	B = 2.0f * (new_o[0] * new_dir[0] + new_o[1] * new_dir[1]);
	C = new_o[0] * new_o[0] + new_o[1] * new_o[1] - radius_sq;
	rcnt = mathQuadraticEquation(A, B, C, distance);
	if (0 == rcnt) {
		float v[3], dot;
		mathVec3Sub(v, ls_v, cp);
		dot = mathVec3Dot(v, axis);
		return fcmpf(mathVec3LenSq(v) - dot * dot, radius_sq, CCT_EPSILON) > 0 ? 0 : -1;
	}
	return rcnt;
}

/*
static int mathLineIntersectCapsule(const float ls_v[3], const float lsdir[3], const float o[3], const float axis[3], float radius, float half_height, float distance[2]) {
	int res = mathLineIntersectCylinderInfinite(ls_v, lsdir, o, axis, radius, distance);
	if (0 == res)
		return 0;
	if (1 == res) {
		float p[3];
		mathVec3AddScalar(mathVec3Copy(p, ls_v), lsdir, distance[0]);
		return mathCapsuleHasPoint(o, axis, radius, half_height, p);
	}
	else {
		float sphere_o[3], d[5], *min_d, *max_d;
		int i, j = -1, cnt;
		if (2 == res) {
			cnt = 0;
			for (i = 0; i < 2; ++i) {
				float p[3];
				mathVec3AddScalar(mathVec3Copy(p, ls_v), lsdir, distance[i]);
				if (!mathCapsuleHasPoint(o, axis, radius, half_height, p))
					continue;
				++cnt;
				j = i;
			}
			if (2 == cnt)
				return 2;
		}
		cnt = 0;
		mathVec3AddScalar(mathVec3Copy(sphere_o, o), axis, half_height);
		if (mathSphereIntersectLine(sphere_o, radius, ls_v, lsdir, d))
			cnt += 2;
		mathVec3AddScalar(mathVec3Copy(sphere_o, o), axis, -half_height);
		if (mathSphereIntersectLine(sphere_o, radius, ls_v, lsdir, d + cnt))
			cnt += 2;
		if (0 == cnt)
			return 0;
		min_d = max_d = NULL;
		if (j >= 0)
			d[cnt++] = distance[j];
		for (i = 0; i < cnt; ++i) {
			if (!min_d || *min_d > d[i])
				min_d = d + i;
			if (!max_d || *max_d < d[i])
				max_d = d + i;
		}
		distance[0] = *min_d;
		distance[1] = *max_d;
		return 2;
	}
}

static int mathSegmentIntersectCapsule(const float ls[2][3], const float o[3], const float axis[3], float radius, float half_height, float p[3]) {
	int res[2];
	res[0] = mathCapsuleHasPoint(o, axis, radius, half_height, ls[0]);
	if (2 == res[0])
		return 2;
	res[1] = mathCapsuleHasPoint(o, axis, radius, half_height, ls[1]);
	if (2 == res[1])
		return 2;
	if (res[0] + res[1] >= 2)
		return 2;
	else {
		float lsdir[3], d[2], new_ls[2][3];
		mathVec3Sub(lsdir, ls[1], ls[0]);
		mathVec3Normalized(lsdir, lsdir);
		if (!mathLineIntersectCapsule(ls[0], lsdir, o, axis, radius, half_height, d)) {
			return 0;
		}
		mathVec3AddScalar(mathVec3Copy(new_ls[0], ls[0]), lsdir, d[0]);
		mathVec3AddScalar(mathVec3Copy(new_ls[1], ls[0]), lsdir, d[1]);
		return mathSegmentIntersectSegment(ls, (const float(*)[3])new_ls, p);
	}
}

static int mathCapsuleIntersectPlane(const float cp_o[3], const float cp_axis[3], float cp_radius, float cp_half_height, const float plane_v[3], const float plane_n[3], float p[3]) {
	float sphere_o[2][3], d[2];
	int i, cmp[2];
	for (i = 0; i < 2; ++i) {
		mathVec3AddScalar(mathVec3Copy(sphere_o[i], cp_o), cp_axis, i ? cp_half_height : -cp_half_height);
		mathPointProjectionPlane(sphere_o[i], plane_v, plane_n, NULL, &d[i]);
		cmp[i] = fcmpf(d[i] * d[i], cp_radius * cp_radius, CCT_EPSILON);
		if (cmp[i] < 0)
			return 2;
	}
	if (0 == cmp[0] && 0 == cmp[1])
		return 2;
	else if (0 == cmp[0]) {
		if (p)
			mathVec3AddScalar(mathVec3Copy(p, sphere_o[0]), plane_n, d[0]);
		return 1;
	}
	else if (0 == cmp[1]) {
		if (p)
			mathVec3AddScalar(mathVec3Copy(p, sphere_o[1]), plane_n, d[1]);
		return 1;
	}
	cmp[0] = fcmpf(d[0], 0.0f, CCT_EPSILON);
	cmp[1] = fcmpf(d[1], 0.0f, CCT_EPSILON);
	return cmp[0] * cmp[1] > 0 ? 0 : 2;
}

static int mathAABBIntersectCapsule(const float aabb_o[3], const float aabb_half[3], const float cp_o[3], const float cp_axis[3], float cp_radius, float cp_half_height) {
	if (mathAABBHasPoint(aabb_o, aabb_half, cp_o) || mathCapsuleHasPoint(cp_o, cp_axis, cp_radius, cp_half_height, aabb_o))
		return 1;
	else {
		int i, j;
		float v[8][3];
		mathAABBVertices(aabb_o, aabb_half, v);
		for (i = 0, j = 0; i < sizeof(Box_Triangle_Vertices_Indices) / sizeof(Box_Triangle_Vertices_Indices[0]); i += 6, ++j) {
			if (mathCapsuleIntersectTrianglesPlane(cp_o, cp_axis, cp_radius, cp_half_height, AABB_Plane_Normal[j], (const float(*)[3])v, Box_Triangle_Vertices_Indices + i, 6))
				return 1;
		}
		return 0;
	}
}

static void mathCapsuleAxisSegment(const float o[3], const float axis[3], float half_height, float ls[2][3]) {
	mathVec3Copy(ls[0], o);
	mathVec3AddScalar(ls[0], axis, half_height);
	mathVec3Copy(ls[1], o);
	mathVec3AddScalar(ls[1], axis, -half_height);
}

static int mathCapsuleIntersectCapsule(const float cp1_o[3], const float cp1_axis[3], float cp1_radius, float cp1_half_height, const float cp2_o[3], const float cp2_axis[3], float cp2_radius, float cp2_half_height) {
	int res;
	float ls1[2][3], ls2[2][3], closest_p[2][3], v[3], r;
	mathCapsuleAxisSegment(cp1_o, cp1_axis, cp1_half_height, ls1);
	mathCapsuleAxisSegment(cp2_o, cp2_axis, cp2_half_height, ls2);
	res = mathSegmentClosestSegment((const float(*)[3])ls1, (const float(*)[3])ls2, closest_p);
	if (GEOMETRY_SEGMENT_OVERLAP == res) {
		return 1;
	}
	if (GEOMETRY_SEGMENT_CONTACT == res) {
		return 1;
	}
	mathVec3Sub(v, closest_p[1], closest_p[0]);
	r = cp1_radius + cp2_radius;
	return mathVec3LenSq(v) <= r * r + CCT_EPSILON;
}
*/

static CCTResult_t* mathRaycastSegment(const float o[3], const float dir[3], const float ls[2][3], CCTResult_t* result) {
	float v0[3], v1[3], N[3], dot, d;
	mathVec3Sub(v0, ls[0], o);
	mathVec3Sub(v1, ls[1], o);
	mathVec3Cross(N, v0, v1);
	if (mathVec3IsZero(N)) {
		dot = mathVec3Dot(v0, v1);
		if (dot <= CCT_EPSILON) {
			set_result(result, 0.0f, o, dir);
			return result;
		}
		mathVec3Cross(N, dir, v0);
		if (!mathVec3IsZero(N)) {
			return NULL;
		}
		dot = mathVec3Dot(dir, v0);
		if (dot < -CCT_EPSILON) {
			return NULL;
		}
		if (mathVec3LenSq(v0) < mathVec3LenSq(v1)) {
			d = mathVec3Dot(v0, dir);
			set_result(result, d, v0, dir);
		}
		else {
			d = mathVec3Dot(v1, dir);
			set_result(result, d, v1, dir);
		}
		return result;
	}
	else {
		float lsdir[3], p[3], op[3];
		dot = mathVec3Dot(N, dir);
		if (dot < -CCT_EPSILON || dot > CCT_EPSILON) {
			return NULL;
		}
		mathVec3Sub(lsdir, ls[1], ls[0]);
		mathVec3Normalized(lsdir, lsdir);
		mathPointProjectionLine(o, ls[0], lsdir, p);
		if (!mathProjectionRay(o, p, dir, &d, op)) {
			return NULL;
		}
		mathVec3Copy(p, o);
		mathVec3AddScalar(p, dir, d);
		mathVec3Sub(v0, ls[0], p);
		mathVec3Sub(v1, ls[1], p);
		dot = mathVec3Dot(v0, v1);
		if (dot > CCT_EPSILON) {
			return NULL;
		}
		set_result(result, d, p, op);
		return result;
	}
}

static CCTResult_t* mathRaycastPlane(const float o[3], const float dir[3], const float plane_v[3], const float plane_n[3], CCTResult_t* result) {
	float d, cos_theta;
	mathPointProjectionPlane(o, plane_v, plane_n, NULL, &d);
	if (fcmpf(d, 0.0f, CCT_EPSILON) == 0) {
		set_result(result, 0.0f, o, dir);
		return result;
	}
	cos_theta = mathVec3Dot(dir, plane_n);
	if (fcmpf(cos_theta, 0.0f, CCT_EPSILON) == 0) {
		return NULL;
	}
	d /= cos_theta;
	if (d < -CCT_EPSILON) {
		return NULL;
	}
	mathVec3AddScalar(mathVec3Copy(result->hit_point, o), dir, d);
	set_result(result, d, result->hit_point, plane_n);
	return result;
}

static CCTResult_t* mathRaycastPolygen(const float o[3], const float dir[3], const GeometryPolygenInner_t* gp, CCTResult_t* result) {
	CCTResult_t* p_result;
	const GeometryPolygen_t* polygen = gp->polygen;
	int i;
	float v[4][3], dot;
	if (!mathRaycastPlane(o, dir, polygen->v[0], polygen->normal, result)) {
		return NULL;
	}
	if (result->distance > CCT_EPSILON) {
		return polygen_inner_has_point(gp, result->hit_point) ? result : NULL;
	}
	dot = mathVec3Dot(dir, polygen->normal);
	if (dot < -CCT_EPSILON || dot > CCT_EPSILON) {
		return NULL;
	}
	p_result = NULL;
	for (i = 0; i < polygen->v_cnt; ++i) {
		CCTResult_t result_temp;
		float edge[2][3];
		mathVec3Copy(edge[0], polygen->v[i]);
		mathVec3Copy(edge[1], polygen->v[(i+1) >= polygen->v_cnt ? 0 : i+1]);
		if (!mathRaycastSegment(o, dir, (const float(*)[3])edge, &result_temp)) {
			continue;
		}
		if (!p_result || p_result->distance > result_temp.distance) {
			p_result = result;
			copy_result(result, &result_temp);
		}
	}
	return p_result;
}

static CCTResult_t* mathRaycastRect(const float o[3], const float dir[3], const GeometryRect_t* rect, CCTResult_t* result) {
	GeometryPolygen_t polygen;
	GeometryPolygenInner_t gp = { &polygen, rect };
	float v[4][3];
	mathRectVertices(rect, v);
	polygen.v_cnt = 4;
	polygen.v = (const float(*)[3])v;
	mathVec3Copy(polygen.normal, rect->normal);
	return mathRaycastPolygen(o, dir, &gp, result);
}
/*
static CCTResult_t* mathRaycastTriangle(const float o[3], const float dir[3], const float tri[3][3], CCTResult_t* result) {
	GeometryPolygen_t polygen;
	GeometryPolygenInner_t gp = { &polygen, NULL };
	polygen.v_cnt = 3;
	polygen.v = tri;
	mathPlaneNormalByVertices3(tri, polygen.normal);
	return mathRaycastPolygen(o, dir, &gp, result);
}
*/
static CCTResult_t* mathRaycastAABB(const float o[3], const float dir[3], const float aabb_o[3], const float aabb_half[3], CCTResult_t* result) {
	if (mathAABBHasPoint(aabb_o, aabb_half, o)) {
		set_result(result, 0.0f, o, dir);
		return result;
	}
	else {
		CCTResult_t *p_result = NULL;
		int i;
		for (i = 0; i < 6; ) {
			CCTResult_t result_temp;
			GeometryRect_t rect;
			mathAABBPlaneRect(aabb_o, aabb_half, i, &rect);
			if (!mathRaycastPlane(o, dir, rect.o, rect.normal, &result_temp)) {
				i += 2;
				continue;
			}
			if (!mathRectHasPoint(&rect, result_temp.hit_point)) {
				++i;
				continue;
			}
			if (!p_result || p_result->distance > result_temp.distance) {
				copy_result(result, &result_temp);
				p_result = result;
			}
			++i;
		}
		return p_result;
	}
}

static int mathAABBIntersectSegment(const float o[3], const float half[3], const float ls[2][3]) {
	CCTResult_t result;
	float ls_dir[3], ls_len;
	mathVec3Sub(ls_dir, ls[1], ls[0]);
	ls_len = mathVec3Normalized(ls_dir, ls_dir);
	if (!mathRaycastAABB(ls[0], ls_dir, o, half, &result)) {
		return 0;
	}
	return ls_len >= result.distance - CCT_EPSILON;
}

static int mathAABBIntersectPolygen(const float o[3], const float half[3], const GeometryPolygenInner_t* gp, float p[3]) {
	int res;
	const GeometryPolygen_t* polygen = gp->polygen;
	float point[3];
	if (!p) {
		p = point;
	}
	res = mathAABBIntersectPlane(o, half, polygen->v[0], polygen->normal, p);
	if (0 == res) {
		return 0;
	}
	else if (1 == res) {
		return polygen_inner_has_point(gp, p);
	}
	else {
		int i;
		for (i = 0; i < polygen->v_cnt; ++i) {
			float edge[2][3];
			mathVec3Copy(edge[0], polygen->v[i]);
			mathVec3Copy(edge[1], polygen->v[(i+1) >= polygen->v_cnt ? 0 : i+1]);
			if (mathAABBIntersectSegment(o, half, (const float(*)[3])edge)) {
				return 2;
			}
		}
		return 0;
	}
}

static int mathAABBIntersectRect(const float o[3], const float half[3], const GeometryRect_t* rect, float p[3]) {
	GeometryPolygen_t polygen;
	GeometryPolygenInner_t gp = { &polygen, NULL };
	float v[4][3];
	mathRectVertices(rect, v);
	polygen.v_cnt = 4;
	polygen.v = (const float(*)[3])v;
	mathVec3Copy(polygen.normal, rect->normal);
	return mathAABBIntersectPolygen(o, half, &gp, p);
}
/*
static int mathAABBIntersectTriangle(const float o[3], const float half[3], const float tri[3][3], float p[3]) {
	GeometryPolygen_t polygen;
	GeometryPolygenInner_t gp = { &polygen, NULL };
	polygen.v_cnt = 3;
	polygen.v = tri;
	mathPlaneNormalByVertices3(tri, polygen.normal);
	return mathAABBIntersectPolygen(o, half, &gp, p);
}
*/
static CCTResult_t* mathRaycastSphere(const float o[3], const float dir[3], const float sp_o[3], float sp_radius, CCTResult_t* result) {
	float dr2, oc2, dir_d;
	float oc[3];
	float radius2 = sp_radius * sp_radius;
	mathVec3Sub(oc, sp_o, o);
	oc2 = mathVec3LenSq(oc);
	if (oc2 <= radius2 + CCT_EPSILON) {
		set_result(result, 0.0f, o, dir);
		return result;
	}
	dir_d = mathVec3Dot(dir, oc);
	if (dir_d <= CCT_EPSILON) {
		return NULL;
	}
	dr2 = oc2 - dir_d * dir_d;
	if (dr2 > radius2 + CCT_EPSILON) {
		return NULL;
	}
	dir_d -= sqrtf(radius2 - dr2);
	set_result(result, dir_d, o, NULL);
	mathVec3AddScalar(result->hit_point, dir, dir_d);
	mathVec3Sub(result->hit_normal, result->hit_point, sp_o);
	mathVec3MultiplyScalar(result->hit_normal, result->hit_normal, 1.0f / sp_radius);
	return result;
}

/*
static CCTResult_t* mathRaycastCapsule(const float o[3], const float dir[3], const float cp_o[3], const float cp_axis[3], float cp_radius, float cp_half_height, CCTResult_t* result) {
	if (mathCapsuleHasPoint(cp_o, cp_axis, cp_radius, cp_half_height, o)) {
		result->distance = 0.0f;
		result->hit_point_cnt = 1;
		mathVec3Copy(result->hit_point, o);
		return result;
	}
	else {
		float d[2];
		if (mathLineIntersectCapsule(o, dir, cp_o, cp_axis, cp_radius, cp_half_height, d)) {
			int cmp[2] = {
				fcmpf(d[0], 0.0f, CCT_EPSILON),
				fcmpf(d[1], 0.0f, CCT_EPSILON)
			};
			if (cmp[0] > 0 && cmp[1] > 0) {
				float sphere_o[3], v[3], dot;
				result->distance = d[0] < d[1] ? d[0] : d[1];
				result->hit_point_cnt = 1;
				mathVec3AddScalar(mathVec3Copy(result->hit_point, o), dir, result->distance);

				mathVec3AddScalar(mathVec3Copy(sphere_o, cp_o), cp_axis, -cp_half_height);
				mathVec3Sub(v, result->hit_point, sphere_o);
				dot = mathVec3Dot(v, cp_axis);
				if (fcmpf(dot, 0.0f, CCT_EPSILON) < 0) {
					mathVec3Copy(result->hit_normal, v);
				}
				else if (fcmpf(dot, cp_half_height, CCT_EPSILON) > 0) {
					mathVec3AddScalar(mathVec3Copy(sphere_o, cp_o), cp_axis, cp_half_height);
					mathVec3Sub(v, result->hit_point, sphere_o);
					mathVec3Copy(result->hit_normal, v);
				}
				else {
					mathPointProjectionLine(result->hit_point, cp_o, cp_axis, v);
					mathVec3Sub(result->hit_normal, result->hit_point, v);
					mathVec3Normalized(result->hit_normal, result->hit_normal);
				}
				return result;
			}
		}
		return NULL;
	}
}
*/

static CCTResult_t* mathSegmentcastPlane(const float ls[2][3], const float dir[3], const float vertice[3], const float normal[3], CCTResult_t* result) {
	int res = mathSegmentIntersectPlane(ls, vertice, normal, result->hit_point);
	if (2 == res) {
		set_result(result, 0.0f, NULL, dir);
		return result;
	}
	else if (1 == res) {
		set_result(result, 0.0f, result->hit_point, normal);
		return result;
	}
	else {
		float d[2], min_d;
		float cos_theta = mathVec3Dot(normal, dir);
		if (fcmpf(cos_theta, 0.0f, CCT_EPSILON) == 0) {
			return NULL;
		}
		mathPointProjectionPlane(ls[0], vertice, normal, NULL, &d[0]);
		mathPointProjectionPlane(ls[1], vertice, normal, NULL, &d[1]);
		if (fcmpf(d[0], d[1], CCT_EPSILON) == 0) {
			min_d = d[0];
			min_d /= cos_theta;
			if (min_d < -CCT_EPSILON) {
				return NULL;
			}
			set_result(result, min_d, NULL, normal);
		}
		else {
			const float *p = NULL;
			if (fcmpf(d[0], 0.0f, CCT_EPSILON) > 0) {
				if (d[0] < d[1]) {
					min_d = d[0];
					p = ls[0];
				}
				else {
					min_d = d[1];
					p = ls[1];
				}
			}
			else {
				if (d[0] < d[1]) {
					min_d = d[1];
					p = ls[1];
				}
				else {
					min_d = d[0];
					p = ls[0];
				}
			}
			min_d /= cos_theta;
			if (min_d < -CCT_EPSILON) {
				return NULL;
			}
			set_result(result, min_d, p, normal);
			mathVec3AddScalar(result->hit_point, dir, min_d);
		}
		return result;
	}
}

static CCTResult_t* mathSegmentcastSegment(const float ls1[2][3], const float dir[3], const float ls2[2][3], CCTResult_t* result) {
	int line_mask;
	int res = mathSegmentIntersectSegment(ls1, ls2, result->hit_point, &line_mask);
	if (GEOMETRY_SEGMENT_CONTACT == res) {
		set_result(result, 0.0f, result->hit_point, dir);
		return result;
	}
	else if (GEOMETRY_SEGMENT_OVERLAP == res) {
		set_result(result, 0.0f, NULL, dir);
		return result;
	}
	else if (GEOMETRY_LINE_PARALLEL == line_mask || GEOMETRY_LINE_CROSS == line_mask) {
		CCTResult_t* p_result = NULL;
		float neg_dir[3];
		int i;
		for (i = 0; i < 2; ++i) {
			CCTResult_t result_temp;
			if (!mathRaycastSegment(ls1[i], dir, ls2, &result_temp)) {
				continue;
			}
			if (!p_result || p_result->distance > result_temp.distance) {
				p_result = result;
				copy_result(p_result, &result_temp);
			}
		}
		mathVec3Negate(neg_dir, dir);
		for (i = 0; i < 2; ++i) {
			CCTResult_t result_temp;
			if (!mathRaycastSegment(ls2[i], neg_dir, ls1, &result_temp)) {
				continue;
			}
			if (!p_result || p_result->distance > result_temp.distance) {
				p_result = result;
				copy_result(p_result, &result_temp);
				mathVec3Copy(p_result->hit_point, ls2[i]);
			}
		}
		return p_result;
	}
	else if (GEOMETRY_LINE_OVERLAP == line_mask) {
		float v[3], N[3], closest_p[2][3], dot;
		mathVec3Sub(v, ls1[1], ls1[0]);
		mathVec3Cross(N, v, dir);
		if (!mathVec3IsZero(N)) {
			return NULL;
		}
		mathSegmentClosestSegmentVertice(ls1, ls2, closest_p);
		mathVec3Sub(v, closest_p[1], closest_p[0]);
		dot = mathVec3Dot(v, dir);
		if (dot < -CCT_EPSILON) {
			return NULL;
		}
		set_result(result, dot, closest_p[1], dir);
		return result;
	}
	else {
		float N[3], v[3], neg_dir[3];
		mathVec3Sub(v, ls1[1], ls1[0]);
		mathVec3Cross(N, v, dir);
		if (mathVec3IsZero(N)) {
			return NULL;
		}
		mathVec3Normalized(N, N);
		if (!mathSegmentIntersectPlane(ls2, ls1[0], N, v)) {
			return NULL;
		}
		mathVec3Negate(neg_dir, dir);
		if (!mathRaycastSegment(v, neg_dir, ls1, result)) {
			return NULL;
		}
		mathVec3Copy(result->hit_point, v);
		return result;
	}
}

static CCTResult_t* mathSegmentcastAABB(const float ls[2][3], const float dir[3], const float o[3], const float half[3], CCTResult_t* result) {
	if (mathAABBIntersectSegment(o, half, ls)) {
		set_result(result, 0.0f, NULL, dir);
		return result;
	}
	else {
		CCTResult_t* p_result = NULL;
		int i;
		float v[6][3];
		mathAABBVertices(o, half, v);
		for (i = 0; i < sizeof(Box_Edge_Indices) / sizeof(Box_Edge_Indices[0]); i += 2) {
			float edge[2][3];
			CCTResult_t result_temp;
			mathVec3Copy(edge[0], v[Box_Edge_Indices[i]]);
			mathVec3Copy(edge[1], v[Box_Edge_Indices[i+1]]);
			if (!mathSegmentcastSegment(ls, dir, (const float(*)[3])edge, &result_temp)) {
				continue;
			}
			if (!p_result || p_result->distance > result_temp.distance) {
				p_result = result;
				copy_result(p_result, &result_temp);
			}
		}
		for (i = 0; i < 2; ++i) {
			CCTResult_t result_temp;
			if (!mathRaycastAABB(ls[i], dir, o, half, &result_temp)) {
				continue;
			}
			if (!p_result || p_result->distance > result_temp.distance) {
				p_result = result;
				copy_result(p_result, &result_temp);
			}
		}
		return p_result;
	}
}

static CCTResult_t* mathAABBcastSegment(const float o[3], const float half[3], const float dir[3], const float ls[2][3], CCTResult_t* result) {
	float neg_dir[3];
	mathVec3Negate(neg_dir, dir);
	if (!mathSegmentcastAABB(ls, neg_dir, o, half, result)) {
		return NULL;
	}
	if (result->hit_point_cnt > 0) {
		mathVec3AddScalar(result->hit_point, dir, result->distance);
	}
	return result;
}

static CCTResult_t* mathSegmentcastPolygen(const float ls[2][3], const float dir[3], const GeometryPolygenInner_t* gp, CCTResult_t* result) {
	CCTResult_t *p_result;
	const GeometryPolygen_t* polygen = gp->polygen;
	int i;
	float v[4][3];
	if (!mathSegmentcastPlane(ls, dir, polygen->v[0], polygen->normal, result)) {
		return NULL;
	}
	if (result->hit_point_cnt > 0) {
		if (polygen_inner_has_point(gp, result->hit_point)) {
			return result;
		}
	}
	else {
		for (i = 0; i < 2; ++i) {
			float test_p[3];
			mathVec3Copy(test_p, ls[i]);
			mathVec3AddScalar(test_p, dir, result->distance);
			if (polygen_inner_has_point(gp, test_p)) {
				return result;
			}
		}
	}
	p_result = NULL;
	for (i = 0; i < polygen->v_cnt; ++i) {
		CCTResult_t result_temp;
		float edge[2][3];
		mathVec3Copy(edge[0], polygen->v[i]);
		mathVec3Copy(edge[1], polygen->v[i+1 >= polygen->v_cnt ? 0 : i+1]);
		if (!mathSegmentcastSegment(ls, dir, (const float(*)[3])edge, &result_temp)) {
			continue;
		}
		if (!p_result || p_result->distance > result_temp.distance) {
			p_result = result;
			copy_result(result, &result_temp);
		}
	}
	return p_result;
}

static CCTResult_t* mathPolygencastSegment(const GeometryPolygenInner_t* gp, const float dir[3], const float ls[2][3], CCTResult_t* result) {
	float neg_dir[3];
	mathVec3Negate(neg_dir, dir);
	if (!mathSegmentcastPolygen(ls, neg_dir, gp, result)) {
		return NULL;
	}
	if (result->hit_point_cnt > 0) {
		mathVec3AddScalar(result->hit_point, dir, result->distance);
	}
	return result;
}

static CCTResult_t* mathSegmentcastRect(const float ls[2][3], const float dir[3], const GeometryRect_t* rect, CCTResult_t* result) {
	GeometryPolygen_t polygen;
	GeometryPolygenInner_t gp = { &polygen, rect };
	float v[4][3];
	mathRectVertices(rect, v);
	polygen.v_cnt = 4;
	polygen.v = (const float(*)[3])v;
	mathVec3Copy(polygen.normal, rect->normal);
	return mathSegmentcastPolygen(ls, dir, &gp, result);
}

static CCTResult_t* mathRectcastSegment(const GeometryRect_t* rect, const float dir[3], const float ls[2][3], CCTResult_t* result) {
	float neg_dir[3];
	mathVec3Negate(neg_dir, dir);
	if (!mathSegmentcastRect(ls, neg_dir, rect, result)) {
		return NULL;
	}
	if (result->hit_point_cnt > 0) {
		mathVec3AddScalar(result->hit_point, dir, result->distance);
	}
	return result;
}
/*
static CCTResult_t* mathSegmentcastTriangle(const float ls[2][3], const float dir[3], const float tri[3][3], CCTResult_t* result) {
	GeometryPolygen_t polygen;
	GeometryPolygenInner_t gp = { &polygen, NULL };
	polygen.v_cnt = 3;
	polygen.v = tri;
	mathPlaneNormalByVertices3(tri, polygen.normal);
	return mathSegmentcastPolygen(ls, dir, &gp, result);
}

static CCTResult_t* mathTrianglecastSegment(const float tri[3][3], const float dir[3], const float ls[2][3], CCTResult_t* result) {
	float neg_dir[3];
	mathVec3Negate(neg_dir, dir);
	if (!mathSegmentcastTriangle(ls, neg_dir, tri, result)) {
		return NULL;
	}
	if (result->hit_point_cnt > 0) {
		mathVec3AddScalar(result->hit_point, dir, result->distance);
	}
	return result;
}
*/
static CCTResult_t* mathSegmentcastSphere(const float ls[2][3], const float dir[3], const float center[3], float radius, CCTResult_t* result) {
	int c = mathSphereIntersectSegment(center, radius, ls, result->hit_point);
	if (1 == c) {
		set_result(result, 0.0f, result->hit_point, dir);
		return result;
	}
	else if (2 == c) {
		set_result(result, 0.0f, NULL, dir);
		return result;
	}
	else {
		CCTResult_t results[2];
		float lsdir[3], N[3];
		mathVec3Sub(lsdir, ls[1], ls[0]);
		mathVec3Cross(N, lsdir, dir);
		if (mathVec3IsZero(N)) {
			if (!mathRaycastSphere(ls[0], dir, center, radius, &results[0])) {
				return NULL;
			}
			if (!mathRaycastSphere(ls[1], dir, center, radius, &results[1])) {
				return NULL;
			}
			copy_result(result, results[0].distance < results[1].distance ? &results[0] : &results[1]);
			return result;
		}
		else {
			CCTResult_t* p_result;
			int i, res;
			float circle_o[3], circle_radius;
			mathVec3Normalized(N, N);
			res = mathSphereIntersectPlane(center, radius, ls[0], N, circle_o, &circle_radius);
			if (res) {
				float plo[3], p[3];
				mathVec3Normalized(lsdir, lsdir);
				mathPointProjectionLine(circle_o, ls[0], lsdir, p);
				mathVec3Sub(plo, circle_o, p);
				res = fcmpf(mathVec3LenSq(plo), circle_radius * circle_radius, CCT_EPSILON);
				if (res >= 0) {
					float new_ls[2][3], d;
					if (res > 0) {
						float cos_theta = mathVec3Dot(plo, dir);
						if (fcmpf(cos_theta, 0.0f, CCT_EPSILON) <= 0)
							return NULL;
						d = mathVec3Normalized(plo, plo);
						cos_theta = mathVec3Dot(plo, dir);
						d -= circle_radius;
						mathVec3AddScalar(p, plo, d);
						d /= cos_theta;
						mathVec3AddScalar(mathVec3Copy(new_ls[0], ls[0]), dir, d);
						mathVec3AddScalar(mathVec3Copy(new_ls[1], ls[1]), dir, d);
					}
					else {
						d = 0.0f;
						mathVec3Copy(new_ls[0], ls[0]);
						mathVec3Copy(new_ls[1], ls[1]);
					}
					if (mathSegmentHasPoint((const float(*)[3])new_ls, p)) {
						set_result(result, d, p, plo);
						return result;
					}
				}
				p_result = NULL;
				for (i = 0; i < 2; ++i) {
					if (!mathRaycastSphere(ls[i], dir, center, radius, &results[i])) {
						continue;
					}
					if (!p_result || p_result->distance > results[i].distance) {
						p_result = &results[i];
					}
				}
				if (p_result) {
					copy_result(result, p_result);
					return result;
				}
			}
			return NULL;
		}
	}
}

static CCTResult_t* mathSpherecastSegment(const float o[3], const float radius, const float dir[3], const float ls[2][3], CCTResult_t* result) {
	float neg_dir[3];
	mathVec3Negate(neg_dir, dir);
	if (!mathSegmentcastSphere(ls, neg_dir, o, radius, result)) {
		return NULL;
	}
	if (result->hit_point_cnt > 0) {
		mathVec3AddScalar(result->hit_point, dir, result->distance);
	}
	return result;
}

/*
static CCTResult_t* mathSegmentcastCapsule(const float ls[2][3], const float dir[3], const float cp_o[3], const float cp_axis[3], float cp_radius, float cp_half_height, CCTResult_t* result) {
	int i, res = mathSegmentIntersectCapsule(ls, cp_o, cp_axis, cp_radius, cp_half_height, result->hit_point);
	if (1 == res) {
		result->distance = 0.0f;
		result->hit_point_cnt = 1;
		return result;
	}
	else if (2 == res) {
		result->distance = 0.0f;
		result->hit_point_cnt = -1;
		return result;
	}
	else {
		float lsdir[3], N[3];
		mathVec3Sub(lsdir, ls[1], ls[0]);
		mathVec3Cross(N, lsdir, dir);
		if (mathVec3IsZero(N)) {
			CCTResult_t results[2], *p_result;
			if (!mathRaycastCapsule(ls[0], dir, cp_o, cp_axis, cp_radius, cp_half_height, &results[0]))
				return NULL;
			if (!mathRaycastCapsule(ls[1], dir, cp_o, cp_axis, cp_radius, cp_half_height, &results[1]))
				return NULL;
			p_result = results[0].distance < results[1].distance ? &results[0] : &results[1];
			copy_result(result, p_result);
			return result;
		}
		else {
			CCTResult_t results[4], *p_result;
			float d, dir_d[2];
			int mask;
			mathVec3Normalized(lsdir, lsdir);
			mask = mathLineClosestLine(cp_o, cp_axis, ls[0], lsdir, &d, dir_d);
			if ((GEOMETRY_LINE_SKEW == mask || GEOMETRY_LINE_CROSS == mask) &&
				fcmpf(d, cp_radius, CCT_EPSILON) > 0)
			{
				float closest_p[2][3], closest_v[3], plane_v[3];
				mathVec3AddScalar(mathVec3Copy(closest_p[0], cp_o), cp_axis, dir_d[0]);
				mathVec3AddScalar(mathVec3Copy(closest_p[1], ls[0]), lsdir, dir_d[1]);
				mathVec3Sub(closest_v, closest_p[1], closest_p[0]);
				mathVec3Normalized(closest_v, closest_v);
				mathVec3AddScalar(mathVec3Copy(plane_v, closest_p[0]), closest_v, cp_radius);
				if (!mathSegmentcastPlane(ls, dir, plane_v, closest_v, result))
					return NULL;
				else {
					float new_ls[2][3];
					mathVec3AddScalar(mathVec3Copy(new_ls[0], ls[0]), dir, result->distance);
					mathVec3AddScalar(mathVec3Copy(new_ls[1], ls[1]), dir, result->distance);
					res = mathSegmentIntersectCapsule((const float(*)[3])new_ls, cp_o, cp_axis, cp_radius, cp_half_height, result->hit_point);
					if (1 == res) {
						result->hit_point_cnt = 1;
						return result;
					}
					else if (2 == res) {
						result->hit_point_cnt = -1;
						return result;
					}
				}
			}
			p_result = NULL;
			for (i = 0; i < 2; ++i) {
				float sphere_o[3];
				mathVec3AddScalar(mathVec3Copy(sphere_o, cp_o), cp_axis, i ? cp_half_height : -cp_half_height);
				if (mathSegmentcastSphere(ls, dir, sphere_o, cp_radius, &results[i]) &&
					(!p_result || p_result->distance > results[i].distance))
				{
					p_result = &results[i];
				}
			}
			for (i = 0; i < 2; ++i) {
				if (mathRaycastCapsule(ls[i], dir, cp_o, cp_axis, cp_radius, cp_half_height, &results[i + 2]) &&
					(!p_result || p_result->distance > results[i + 2].distance))
				{
					p_result = &results[i + 2];
				}
			}
			if (p_result) {
				copy_result(result, p_result);
				return result;
			}
			return NULL;
		}
	}
}
*/

static CCTResult_t* mathPolygencastPlane(const GeometryPolygenInner_t* gp, const float dir[3], const float plane_v[3], const float plane_n[3], CCTResult_t* result) {
	int i, has_gt0 = 0, has_le0 = 0, idx_0 = -1, idx_min = -1;
	float min_d, dot;
	const GeometryPolygen_t* polygen = gp->polygen;
	for (i = 0; i < polygen->v_cnt; ++i) {
		int cmp;
		float d;
		mathPointProjectionPlane(polygen->v[i], plane_v, plane_n, NULL, &d);
		cmp = fcmpf(d, 0.0f, CCT_EPSILON);
		if (cmp > 0) {
			if (has_le0) {
				set_result(result, 0.0f, NULL, dir);
				return result;
			}
			has_gt0 = 1;
		}
		else if (cmp < 0) {
			if (has_gt0) {
				set_result(result, 0.0f, NULL, dir);
				return result;
			}
			has_le0 = 1;
		}
		else if (idx_0 >= 0) {
			set_result(result, 0.0f, NULL, dir);
			return result;
		}
		else {
			idx_0 = i;
		}
		if (i && fabsf(min_d) < fabsf(d)) {
			continue;
		}
		min_d = d;
		idx_min = i;
	}
	if (idx_0 >= 0) {
		set_result(result, 0.0f, polygen->v[idx_0], plane_n);
		return result;
	}
	dot = mathVec3Dot(dir, plane_n);
	if (fcmpf(dot, 0.0f, CCT_EPSILON) == 0) {
		return NULL;
	}
	min_d /= dot;
	if (min_d < -CCT_EPSILON) {
		return NULL;
	}
	set_result(result, min_d, polygen->v[idx_min], plane_n);
	mathVec3AddScalar(result->hit_point, dir, min_d);
	return result;
}

static CCTResult_t* mathRectcastPlane(const GeometryRect_t* rect, const float dir[3], const float plane_v[3], const float plane_n[3], CCTResult_t* result) {
	GeometryPolygen_t polygen;
	GeometryPolygenInner_t gp = { &polygen, rect };
	float v[4][3];
	mathRectVertices(rect, v);
	polygen.v_cnt = 4;
	polygen.v = (const float(*)[3])v;
	mathVec3Copy(polygen.normal, rect->normal);
	return mathPolygencastPlane(&gp, dir, plane_v, plane_n, result);
}
/*
static CCTResult_t* mathTrianglecastPlane(const float tri[3][3], const float dir[3], const float plane_v[3], const float plane_n[3], CCTResult_t* result) {
	GeometryPolygen_t polygen;
	GeometryPolygenInner_t gp = { &polygen, NULL };
	polygen.v_cnt = 3;
	polygen.v = tri;
	mathPlaneNormalByVertices3(tri, polygen.normal);
	return mathPolygencastPlane(&gp, dir, plane_v, plane_n, result);
}
*/
static CCTResult_t* mathPolygencastPolygen(const GeometryPolygenInner_t* gp1, const float dir[3], const GeometryPolygenInner_t* gp2, CCTResult_t* result) {
	CCTResult_t* p_result;
	int i, flag;
	float neg_dir[3];
	const GeometryPolygen_t* polygen1 = gp1->polygen;
	const GeometryPolygen_t* polygen2 = gp2->polygen;
	if (mathPolygenIntersectPolygen(gp1, gp2)) {
		set_result(result, 0.0f, NULL, dir);
		return result;
	}
	flag = mathPlaneIntersectPlane(polygen1->v[0], polygen1->normal, polygen2->v[0], polygen2->normal);
	if (0 == flag) {
		float d, dot;
		mathPointProjectionPlane(polygen1->v[0], polygen2->v[0], polygen2->normal, NULL, &d);
		dot = mathVec3Dot(dir, polygen2->normal);
		if (fcmpf(dot, 0.0f, CCT_EPSILON) == 0) {
			return NULL;
		}
		d /= dot;
		if (d < -CCT_EPSILON) {
			return NULL;
		}
	}
	else if (2 == flag) {
		float dot = mathVec3Dot(dir, polygen2->normal);
		if (dot > CCT_EPSILON || dot < -CCT_EPSILON) {
			return NULL;
		}
	}
	p_result = NULL;
	for (i = 0; i < polygen1->v_cnt; ++i) {
		CCTResult_t result_temp;
		float edge[2][3];
		mathVec3Copy(edge[0], polygen1->v[i]);
		mathVec3Copy(edge[1], polygen1->v[i+1 >= polygen1->v_cnt ? 0 : i+1]);
		if (!mathSegmentcastPolygen((const float(*)[3])edge, dir, gp2, &result_temp)) {
			continue;
		}
		if (!p_result || p_result->distance > result_temp.distance) {
			p_result = result;
			copy_result(result, &result_temp);
		}
	}
	if (2 == flag) {
		if (p_result) {
			p_result->hit_point_cnt = -1;
		}
		return p_result;
	}
	mathVec3Negate(neg_dir, dir);
	for (i = 0; i < polygen2->v_cnt; ++i) {
		CCTResult_t result_temp;
		float edge[2][3];
		mathVec3Copy(edge[0], polygen2->v[i]);
		mathVec3Copy(edge[1], polygen2->v[i+1 >= polygen2->v_cnt ? 0 : i+1]);
		if (!mathSegmentcastPolygen((const float(*)[3])edge, neg_dir, gp1, &result_temp)) {
			continue;
		}
		if (!p_result || p_result->distance > result_temp.distance) {
			p_result = result;
			copy_result(result, &result_temp);
		}
	}
	if (p_result) {
		p_result->hit_point_cnt = -1;
	}
	return p_result;
}

static CCTResult_t* mathRectcastRect(const GeometryRect_t* rect1, const float dir[3], const GeometryRect_t* rect2, CCTResult_t* result) {
	float v1[4][3], v2[4][3];
	GeometryPolygen_t polygen1, polygen2;
	GeometryPolygenInner_t gp1 = { &polygen1, rect1 }, gp2 = { &polygen2, rect2 };
	mathRectVertices(rect1, v1);
	polygen1.v_cnt = 4;
	polygen1.v = (const float(*)[3])v1;
	mathVec3Copy(polygen1.normal, rect1->normal);
	mathRectVertices(rect2, v2);
	polygen2.v_cnt = 4;
	polygen2.v = (const float(*)[3])v2;
	mathVec3Copy(polygen2.normal, rect2->normal);
	return mathPolygencastPolygen(&gp1, dir, &gp2, result);
}
/*
static CCTResult_t* mathTrianglecastTriangle(const float tri1[3][3], const float dir[3], const float tri2[3][3], CCTResult_t* result) {
	GeometryPolygen_t polygen1, polygen2;
	GeometryPolygenInner_t gp1 = { &polygen1, NULL }, gp2 = { &polygen2, NULL };
	polygen1.v_cnt = 3;
	polygen1.v = tri1;
	mathPlaneNormalByVertices3(tri1, polygen1.normal);
	polygen2.v_cnt = 3;
	polygen2.v = tri2;
	mathPlaneNormalByVertices3(tri2, polygen2.normal);
	return mathPolygencastPolygen(&gp1, dir, &gp2, result);
}
*/
static CCTResult_t* mathAABBcastPlane(const float o[3], const float half[3], const float dir[3], const float vertice[3], const float normal[3], CCTResult_t* result) {
	int res = mathAABBIntersectPlane(o, half, vertice, normal, result->hit_point);
	if (1 == res) {
		set_result(result, 0.0f, result->hit_point, normal);
		return result;
	}
	else if (2 == res) {
		set_result(result, 0.0f, NULL, dir);
		return result;
	}
	else {
		CCTResult_t* p_result = NULL;
		float v[8][3];
		int i;
		mathAABBVertices(o, half, v);
		for (i = 0; i < sizeof(v) / sizeof(v[0]); ++i) {
			int cmp;
			CCTResult_t result_temp;
			if (!mathRaycastPlane(v[i], dir, vertice, normal, &result_temp)) {
				break;
			}
			if (!p_result) {
				copy_result(result, &result_temp);
				p_result = result;
				continue;
			}
			cmp = fcmpf(p_result->distance, result_temp.distance, CCT_EPSILON);
			if (cmp < 0) {
				continue;
			}
			if (0 == cmp) {
				p_result->hit_point_cnt = -1;
				continue;
			}
			copy_result(result, &result_temp);
		}
		return p_result;
	}
}

static CCTResult_t* mathAABBcastAABB(const float o1[3], const float half1[3], const float dir[3], const float o2[3], const float half2[3], CCTResult_t* result) {
	if (mathAABBIntersectAABB(o1, half1, o2, half2)) {
		set_result(result, 0.0f, NULL, dir);
		return result;
	}
	else {
		CCTResult_t *p_result = NULL;
		int i;
		float v1[6][3], v2[6][3];
		mathAABBPlaneVertices(o1, half1, v1);
		mathAABBPlaneVertices(o2, half2, v2);
		for (i = 0; i < 6; ++i) {
			float new_o1[3];
			CCTResult_t result_temp;
			if (i & 1) {
				if (!mathRaycastPlane(v1[i], dir, v2[i-1], AABB_Plane_Normal[i], &result_temp)) {
					continue;
				}
			}
			else {
				if (!mathRaycastPlane(v1[i], dir, v2[i+1], AABB_Plane_Normal[i], &result_temp)) {
					continue;
				}
			}
			mathVec3Copy(new_o1, o1);
			mathVec3AddScalar(new_o1, dir, result_temp.distance);
			if (!mathAABBIntersectAABB(new_o1, half1, o2, half2)) {
				continue;
			}
			if (!p_result || p_result->distance > result_temp.distance) {
				p_result = result;
				copy_result(p_result, &result_temp);
			}
		}
		return p_result;
	}
}

static CCTResult_t* mathAABBcastPolygen(const float o[3], const float half[3], const float dir[3], const GeometryPolygenInner_t* gp, CCTResult_t* result) {
	int i;
	float v[8][3], neg_dir[3];
	CCTResult_t *p_result;
	const GeometryPolygen_t* polygen = gp->polygen;
	if (!mathAABBcastPlane(o, half, dir, polygen->v[0], polygen->normal, result)) {
		return NULL;
	}
	if (result->hit_point_cnt > 0) {
		if (polygen_inner_has_point(gp, result->hit_point)) {
			return result;
		}
	}
	else {
		mathAABBVertices(o, half, v);
		for (i = 0; i < 8; ++i) {
			if (polygen_inner_has_point(gp, v[i])) {
				return result;
			}
		}
	}
	p_result = NULL;
	mathVec3Negate(neg_dir, dir);
	for (i = 0; i < polygen->v_cnt; ++i) {
		CCTResult_t result_temp;
		float edge[2][3];
		mathVec3Copy(edge[0], polygen->v[i]);
		mathVec3Copy(edge[0], polygen->v[i+1 >= polygen->v_cnt ? 0 : i+1]);
		if (!mathSegmentcastAABB((const float(*)[3])edge, neg_dir, o, half, &result_temp)) {
			continue;
		}
		if (!p_result || p_result->distance > result_temp.distance) {
			p_result = result;	
			copy_result(p_result, &result_temp);
		}
	}
	if (p_result && p_result->hit_point_cnt > 0) {
		mathVec3AddScalar(p_result->hit_point, dir, p_result->distance);
	}
	return p_result;
}

static CCTResult_t* mathPolygencastAABB(const GeometryPolygenInner_t* gp, const float dir[3], const float o[3], const float half[3], CCTResult_t* result) {
	float neg_dir[3];
	mathVec3Negate(neg_dir, dir);
	if (!mathAABBcastPolygen(o, half, neg_dir, gp, result)) {
		return NULL;
	}
	if (result->hit_point_cnt > 0) {
		mathVec3AddScalar(result->hit_point, dir, result->distance);
	}
	return result;
}

static CCTResult_t* mathAABBcastRect(const float o[3], const float half[3], const float dir[3], const GeometryRect_t* rect, CCTResult_t* result) {
	GeometryPolygen_t polygen;
	GeometryPolygenInner_t gp = { &polygen, rect };
	float v[4][3];
	mathRectVertices(rect, v);
	polygen.v_cnt = 4;
	polygen.v = (const float(*)[3])v;
	mathVec3Copy(polygen.normal, rect->normal);
	return mathAABBcastPolygen(o, half, dir, &gp, result);
}

static CCTResult_t* mathRectcastAABB(const GeometryRect_t* rect, const float dir[3], const float o[3], const float half[3], CCTResult_t* result) {
	float neg_dir[3];
	mathVec3Negate(neg_dir, dir);
	if (!mathAABBcastRect(o, half, neg_dir, rect, result)) {
		return NULL;
	}
	if (result->hit_point_cnt > 0) {
		mathVec3AddScalar(result->hit_point, dir, result->distance);
	}
	return result;
}
/*
static CCTResult_t* mathAABBcastTriangle(const float o[3], const float half[3], const float dir[3], const float tri[3][3], CCTResult_t* result) {
	GeometryPolygen_t polygen;
	GeometryPolygenInner_t gp = { &polygen, NULL };
	polygen.v_cnt = 3;
	polygen.v = tri;
	mathPlaneNormalByVertices3(tri, polygen.normal);
	return mathAABBcastPolygen(o, half, dir, &gp, result);
}
*/
static CCTResult_t* mathSpherecastPlane(const float o[3], float radius, const float dir[3], const float plane_v[3], const float plane_n[3], CCTResult_t* result) {
	int res;
	float dn, d, hit_point[3];
	mathPointProjectionPlane(o, plane_v, plane_n, NULL, &dn);
	res = fcmpf(dn * dn, radius * radius, CCT_EPSILON);
	if (res < 0) {
		set_result(result, 0.0f, NULL, dir);
		return result;
	}
	else if (0 == res) {
		mathVec3AddScalar(mathVec3Copy(hit_point, o), plane_n, dn);
		set_result(result, 0.0f, hit_point, dir);
		return result;
	}
	else {
		float dn_abs, cos_theta = mathVec3Dot(plane_n, dir);
		if (cos_theta <= CCT_EPSILON && cos_theta >= -CCT_EPSILON) {
			return NULL;
		}
		d = dn / cos_theta;
		if (d < -CCT_EPSILON) {
			return NULL;
		}
		dn_abs = (dn >= 0.0f ? dn : -dn);
		d -= radius / dn_abs * d;
		mathVec3AddScalar(mathVec3Copy(hit_point, o), dir, d);
		mathVec3AddScalar(hit_point, plane_n, dn >= 0.0f ? radius : -radius);
		set_result(result, d, hit_point, plane_n);
		return result;
	}
}

static CCTResult_t* mathSpherecastSphere(const float o1[3], float r1, const float dir[3], const float o2[3], float r2, CCTResult_t* result) {
	if (!mathRaycastSphere(o1, dir, o2, r1 + r2, result)) {
		return NULL;
	}
	mathVec3Sub(result->hit_normal, result->hit_point, o2);
	mathVec3Normalized(result->hit_normal, result->hit_normal);
	mathVec3AddScalar(result->hit_point, result->hit_normal, -r1);
	return result;
}

static CCTResult_t* mathSpherecastAABB(const float o[3], float radius, const float dir[3], const float center[3], const float half[3], CCTResult_t* result) {
	if (mathAABBIntersectSphere(center, half, o, radius)) {
		set_result(result, 0.0f, NULL, dir);
		return result;
	}
	else {
		CCTResult_t* p_result = NULL;
		float v[8][3], neg_dir[3];
		int i;
		for (i = 0; i < 6; ++i) {
			CCTResult_t result_temp;
			GeometryRect_t rect;
			mathAABBPlaneRect(center, half, i, &rect);
			if (!mathSpherecastPlane(o, radius, dir, rect.o, rect.normal, &result_temp)) {
				continue;
			}
			if (fcmpf(result_temp.distance, 0.0, CCT_EPSILON) == 0) {
				continue;
			}
			if (!mathRectHasPoint(&rect, result_temp.hit_point)) {
				continue;
			}
			if (!p_result || p_result->distance > result_temp.distance) {
				p_result = result;
				copy_result(p_result, &result_temp);
			}
		}
		if (p_result) {
			return p_result;
		}
		mathAABBVertices(center, half, v);
		mathVec3Negate(neg_dir, dir);
		for (i = 0; i < sizeof(Box_Edge_Indices) / sizeof(Box_Edge_Indices[0]); i += 2) {
			float edge[2][3];
			CCTResult_t result_temp;
			mathVec3Copy(edge[0], v[Box_Edge_Indices[i]]);
			mathVec3Copy(edge[1], v[Box_Edge_Indices[i+1]]);
			if (!mathSegmentcastSphere((const float(*)[3])edge, neg_dir, o, radius, &result_temp)) {
				continue;
			}
			if (!p_result || p_result->distance > result_temp.distance) {
				p_result = result;
				copy_result(p_result, &result_temp);
			}
		}
		return p_result;
	}
}

static CCTResult_t* mathAABBcastSphere(const float center[3], const float half[3], const float dir[3], const float o[3], float radius, CCTResult_t* result) {
	float neg_dir[3];
	mathVec3Negate(neg_dir, dir);
	if (!mathSpherecastAABB(o, radius, neg_dir, center, half, result)) {
		return NULL;
	}
	if (result->hit_point_cnt > 0) {
		mathVec3AddScalar(result->hit_point, dir, result->distance);
	}
	return result;
}

static CCTResult_t* mathSpherecastPolygen(const float o[3], float radius, const float dir[3], const GeometryPolygenInner_t* gp, CCTResult_t* result) {
	int i;
	CCTResult_t *p_result = NULL;
	const GeometryPolygen_t* polygen = gp->polygen;
	float neg_dir[3];
	if (!mathSpherecastPlane(o, radius, dir, polygen->v[0], polygen->normal, result)) {
		return NULL;
	}
	if (result->hit_point_cnt > 0) {
		if (polygen_inner_has_point(gp, result->hit_point)) {
			return result;
		}
	}
	p_result = NULL;
	mathVec3Negate(neg_dir, dir);
	for (i = 0; i < polygen->v_cnt; ++i) {
		CCTResult_t result_temp;
		float edge[2][3];
		mathVec3Copy(edge[0], polygen->v[i]);
		mathVec3Copy(edge[1], polygen->v[(i+1) >= polygen->v_cnt ? 0 : i+1]);
		if (!mathSegmentcastSphere((const float(*)[3])edge, neg_dir, o, radius, &result_temp)) {
			continue;
		}
		if (!p_result || p_result->distance > result_temp.distance) {
			p_result = result;
			copy_result(result, &result_temp);
		}
	}
	if (p_result) {
		mathVec3AddScalar(p_result->hit_point, dir, p_result->distance);
	}
	return p_result;
}

static CCTResult_t* mathPolygencastSphere(const GeometryPolygenInner_t* gp, const float dir[3], const float o[3], float radius, CCTResult_t* result) {
	float neg_dir[3];
	mathVec3Negate(neg_dir, dir);
	if (!mathSpherecastPolygen(o, radius, dir, gp, result)) {
		return NULL;
	}
	if (result->hit_point_cnt > 0) {
		mathVec3AddScalar(result->hit_point, dir, result->distance);
	}
	return result;
}

static CCTResult_t* mathSpherecastRect(const float o[3], float radius, const float dir[3], const GeometryRect_t* rect, CCTResult_t* result) {
	GeometryPolygen_t polygen;
	GeometryPolygenInner_t gp = { &polygen, rect };
	float v[4][3];
	mathRectVertices(rect, v);
	polygen.v_cnt = 4;
	polygen.v = (const float(*)[3])v;
	mathVec3Copy(polygen.normal, rect->normal);
	return mathSpherecastPolygen(o, radius, dir, &gp, result);
}

static CCTResult_t* mathRectcastSphere(const GeometryRect_t* rect, const float dir[3], const float o[3], float radius, CCTResult_t* result) {
	float neg_dir[3];
	mathVec3Negate(neg_dir, dir);
	if (!mathSpherecastRect(o, radius, neg_dir, rect, result)) {
		return NULL;
	}
	if (result->hit_point_cnt > 0) {
		mathVec3AddScalar(result->hit_point, dir, result->distance);
	}
	return result;
}
/*
static CCTResult_t* mathSpherecastTriangle(const float o[3], float radius, const float dir[3], const float tri[3][3], CCTResult_t* result) {
	GeometryPolygen_t polygen;
	GeometryPolygenInner_t gp = { &polygen, NULL };
	polygen.v_cnt = 3;
	polygen.v = tri;
	mathPlaneNormalByVertices3(tri, polygen.normal);
	return mathSpherecastPolygen(o, radius, dir, &gp, result);
}

static CCTResult_t* mathTrianglecastSphere(const float tri[3][3], const float dir[3], const float o[3], float radius, CCTResult_t* result) {
	float neg_dir[3];
	mathVec3Negate(neg_dir, dir);
	if (!mathSpherecastTriangle(o, radius, neg_dir, tri, result)) {
		return NULL;
	}
	if (result->hit_point_cnt > 0) {
		mathVec3AddScalar(result->hit_point, dir, result->distance);
	}
	return result;
}
*/
/*
static CCTResult_t* mathSpherecastCapsule(const float sp_o[3], float sp_radius, const float dir[3], const float cp_o[3], const float cp_axis[3], float cp_radius, float cp_half_height, CCTResult_t* result) {
	if (mathRaycastCapsule(sp_o, dir, cp_o, cp_axis, sp_radius + cp_radius, cp_half_height, result)) {
		float new_sp_o[3];
		mathVec3AddScalar(mathVec3Copy(new_sp_o, sp_o), dir, result->distance);
		result->hit_point_cnt = 1;
		mathSphereIntersectCapsule(new_sp_o, sp_radius, cp_o, cp_axis, cp_radius, cp_half_height, result->hit_point);
		return result;
	}
	return NULL;
}

static CCTResult_t* mathCapsulecastPlane(const float cp_o[3], const float cp_axis[3], float cp_radius, float cp_half_height, const float dir[3], const float plane_v[3], const float plane_n[3], CCTResult_t* result) {
	int res = mathCapsuleIntersectPlane(cp_o, cp_axis, cp_radius, cp_half_height, plane_v, plane_n, result->hit_point);
	if (2 == res) {
		result->distance = 0.0f;
		result->hit_point_cnt = -1;
		return result;
	}
	else if (1 == res) {
		result->distance = 0.0f;
		result->hit_point_cnt = 1;
		return result;
	}
	else {
		CCTResult_t results[2], *p_result = NULL;
		int i;
		for (i = 0; i < 2; ++i) {
			float sphere_o[3];
			mathVec3AddScalar(mathVec3Copy(sphere_o, cp_o), cp_axis, i ? cp_half_height : -cp_half_height);
			if (mathSpherecastPlane(sphere_o, cp_radius, dir, plane_v, plane_n, &results[i]) &&
				(!p_result || p_result->distance > results[i].distance))
			{
				p_result = results + i;
			}
		}
		if (p_result) {
			float dot = mathVec3Dot(cp_axis, plane_n);
			if (0 == fcmpf(dot, 0.0f, CCT_EPSILON))
				p_result->hit_point_cnt = -1;
			copy_result(result, p_result);
			return result;
		}
		return NULL;
	}
}

static CCTResult_t* mathCapsulecastAABB(const float cp_o[3], const float cp_axis[3], float cp_radius, float cp_half_height, const float dir[3], const float aabb_o[3], const float aabb_half[3], CCTResult_t* result) {
	if (mathAABBHasPoint(aabb_o, aabb_half, cp_o) || mathCapsuleHasPoint(cp_o, cp_axis, cp_radius, cp_half_height, aabb_o)) {
		result->distance = 0.0f;
		result->hit_point_cnt = -1;
		return result;
	}
	else {
		CCTResult_t *p_result = NULL;
		int i, j;
		float v[8][3];
		mathAABBVertices(aabb_o, aabb_half, v);
		for (i = 0, j = 0; i < sizeof(Box_Triangle_Vertices_Indices) / sizeof(Box_Triangle_Vertices_Indices[0]); i += 6, ++j) {
			CCTResult_t result_temp;
			if (mathCapsulecastTrianglesPlane(cp_o, cp_axis, cp_radius, cp_half_height, dir, AABB_Plane_Normal[j], (const float(*)[3])v, Box_Triangle_Vertices_Indices + i, 6, &result_temp) &&
				(!p_result || p_result->distance > result_temp.distance))
			{
				copy_result(result, &result_temp);
				p_result = result;
			}
		}
		return p_result;
	}
}

static CCTResult_t* mathCapsulecastCapsule(const float cp1_o[3], const float cp1_axis[3], float cp1_radius, float cp1_half_height, const float dir[3], const float cp2_o[3], const float cp2_axis[3], float cp2_radius, float cp2_half_height, CCTResult_t* result) {
	if (mathCapsuleIntersectCapsule(cp1_o, cp1_axis, cp1_radius, cp1_half_height, cp2_o, cp2_axis, cp2_radius, cp2_half_height)) {
		result->distance = 0.0f;
		result->hit_point_cnt = -1;
		return result;
	}
	else {
		CCTResult_t results[4], *p_result;
		int i;
		float N[3];
		mathVec3Cross(N, cp1_axis, dir);
		if (mathVec3IsZero(N)) {
			for (i = 0; i < 2; ++i) {
				float sphere_o[3];
				mathVec3AddScalar(mathVec3Copy(sphere_o, cp1_o), cp1_axis, i ? cp1_half_height : -cp1_half_height);
				if (!mathSpherecastCapsule(sphere_o, cp1_radius, dir, cp2_o, cp2_axis, cp2_radius, cp2_half_height, &results[i]))
					return NULL;
			}
			p_result = results[0].distance < results[1].distance ? &results[0] : &results[1];
			copy_result(result, p_result);
			return result;
		}
		else {
			float min_d, dir_d[2], neg_dir[3];
			int mask = mathLineClosestLine(cp1_o, cp1_axis, cp2_o, cp2_axis, &min_d, dir_d);
			if ((GEOMETRY_LINE_SKEW == mask || GEOMETRY_LINE_CROSS == mask) &&
				fcmpf(min_d, cp1_radius + cp2_radius, CCT_EPSILON) > 0)
			{
				float closest_p[2][3], closest_v[3], plane_p[3];
				mathVec3AddScalar(mathVec3Copy(closest_p[0], cp1_o), cp1_axis, dir_d[0]);
				mathVec3AddScalar(mathVec3Copy(closest_p[1], cp2_o), cp2_axis, dir_d[1]);
				mathVec3Sub(closest_v, closest_p[1], closest_p[0]);
				mathVec3Normalized(closest_v, closest_v);
				mathVec3AddScalar(mathVec3Copy(plane_p, cp2_o), closest_v, -cp2_radius);
				if (!mathCapsulecastPlane(cp1_o, cp1_axis, cp1_radius, cp1_half_height, dir, plane_p, closest_v, result))
					return NULL;
				else {
					float new_cp1_o[3];
					mathVec3AddScalar(mathVec3Copy(new_cp1_o, cp1_o), dir, result->distance);
					if (mathCapsuleIntersectCapsule(new_cp1_o, cp1_axis, cp1_radius, cp1_half_height, cp2_o, cp2_axis, cp2_radius, cp2_half_height)) {
						result->hit_point_cnt = -1;
						return result;
					}
				}
			}
			mathVec3Negate(neg_dir, dir);
			p_result = NULL;
			for (i = 0; i < 2; ++i) {
				float sphere_o[3];
				mathVec3AddScalar(mathVec3Copy(sphere_o, cp2_o), cp2_axis, i ? cp2_half_height : -cp2_half_height);
				if (mathSpherecastCapsule(sphere_o, cp2_radius, neg_dir, cp1_o, cp1_axis, cp1_radius, cp1_half_height, &results[i]) &&
					(!p_result || p_result->distance > results[i].distance))
				{
					p_result = &results[i];
				}
			}
			for (i = 0; i < 2; ++i) {
				float sphere_o[3];
				mathVec3AddScalar(mathVec3Copy(sphere_o, cp1_o), cp1_axis, i ? cp1_half_height : -cp1_half_height);
				if (mathSpherecastCapsule(sphere_o, cp1_radius, dir, cp2_o, cp2_axis, cp2_radius, cp2_half_height, &results[i + 2]) &&
					(!p_result || p_result->distance > results[i + 2].distance))
				{
					p_result = &results[i + 2];
				}
			}
			if (p_result) {
				p_result->hit_point_cnt = -1;
				copy_result(result, p_result);
				return result;
			}
			return NULL;
		}
	}
}
*/

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

GeometryAABB_t* mathCollisionBodyBoundingBox(const GeometryBodyRef_t* b, const float delta_half_v[3], GeometryAABB_t* aabb) {
	switch (b->type) {
		case GEOMETRY_BODY_AABB:
		{
			*aabb = *(b->aabb);
			break;
		}
		case GEOMETRY_BODY_SPHERE:
		{
			mathVec3Copy(aabb->o, b->sphere->o);
			mathVec3Set(aabb->half, b->sphere->radius, b->sphere->radius, b->sphere->radius);
			break;
		}
		/*
		case COLLISION_BODY_CAPSULE:
		{
			float half_d = b->capsule.half_height + b->capsule.radius;
			mathVec3Copy(aabb->pos, b->capsule.pos);
			mathVec3Set(aabb->half, half_d, half_d, half_d);
			break;
		}
		*/
		default:
		{
			return NULL;
		}
	}
	if (delta_half_v) {
		int i;
		mathVec3Add(aabb->o, aabb->o, delta_half_v);
		for (i = 0; i < 3; ++i) {
			aabb->half[i] += (delta_half_v[i] > 0.0f ? delta_half_v[i] : -delta_half_v[i]);
		}
	}
	return aabb;
}

int mathCollisionBodyIntersect(const GeometryBodyRef_t* one, const GeometryBodyRef_t* two) {
	if (one == two) {
		return 1;
	}
	else if (GEOMETRY_BODY_POINT == one->type) {
		switch (two->type) {
			case GEOMETRY_BODY_POINT:
			{
				return mathVec3Equal(one->point, two->point);
			}
			case GEOMETRY_BODY_SEGMENT:
			{
				return mathSegmentHasPoint(two->segment->v, one->point);
			}
			case GEOMETRY_BODY_PLANE:
			{
				return mathPlaneHasPoint(two->plane->v, two->plane->normal, one->point);
			}
			case GEOMETRY_BODY_AABB:
			{
				return mathAABBHasPoint(two->aabb->o, two->aabb->half, one->point);
			}
			case GEOMETRY_BODY_SPHERE:
			{
				return mathSphereHasPoint(two->sphere->o, two->sphere->radius, one->point);
			}
			case GEOMETRY_BODY_RECT:
			{
				return mathRectHasPoint(two->rect, one->point);
			}
			case GEOMETRY_BODY_POLYGEN:
			{
				GeometryPolygenInner_t gp = { two->polygen };
				return polygen_inner_has_point(&gp, one->point);
			}
			default:
				return 0;
		}
	}
	else if (GEOMETRY_BODY_SEGMENT == one->type) {
		switch (two->type) {
			case GEOMETRY_BODY_POINT:
			{
				return mathSegmentHasPoint(one->segment->v, two->point);
			}
			case GEOMETRY_BODY_SEGMENT:
			{
				return mathSegmentIntersectSegment(one->segment->v, two->segment->v, NULL, NULL);
			}
			case GEOMETRY_BODY_PLANE:
			{
				return mathSegmentIntersectPlane(one->segment->v, two->plane->v, two->plane->normal, NULL);
			}
			case GEOMETRY_BODY_AABB:
			{
				return mathAABBIntersectSegment(two->aabb->o, two->aabb->half, one->segment->v);
			}
			case GEOMETRY_BODY_SPHERE:
			{
				return mathSphereIntersectSegment(two->sphere->o, two->sphere->radius, one->segment->v, NULL);
			}
			case GEOMETRY_BODY_RECT:
			{
				return mathSegmentIntersectRect(one->segment->v, two->rect, NULL);
			}
			case GEOMETRY_BODY_POLYGEN:
			{
				GeometryPolygenInner_t gp = { two->polygen };
				return mathSegmentIntersectPolygen(one->segment->v, &gp, NULL);
			}
			default:
				return 0;
		}
	}
	else if (GEOMETRY_BODY_AABB == one->type) {
		switch (two->type) {
			case GEOMETRY_BODY_POINT:
			{
				return mathAABBHasPoint(one->aabb->o, one->aabb->half, two->point);
			}
			case GEOMETRY_BODY_AABB:
			{
				return mathAABBIntersectAABB(one->aabb->o, one->aabb->half, two->aabb->o, two->aabb->half);
			}
			case GEOMETRY_BODY_SPHERE:
			{
				return mathAABBIntersectSphere(one->aabb->o, one->aabb->half, two->sphere->o, two->sphere->radius);
			}
			case GEOMETRY_BODY_PLANE:
			{
				return mathAABBIntersectPlane(one->aabb->o, one->aabb->half, two->plane->v, two->plane->normal, NULL);
			}
			case GEOMETRY_BODY_SEGMENT:
			{
				return mathAABBIntersectSegment(one->aabb->o, one->aabb->half, two->segment->v);
			}
			case GEOMETRY_BODY_RECT:
			{
				return mathAABBIntersectRect(one->aabb->o, one->aabb->half, two->rect, NULL);
			}
			case GEOMETRY_BODY_POLYGEN:
			{
				GeometryPolygenInner_t gp = { two->polygen };
				return mathAABBIntersectPolygen(one->aabb->o, one->aabb->half, &gp, NULL);
			}
			default:
				return 0;
		}
	}
	else if (GEOMETRY_BODY_SPHERE == one->type) {
		switch (two->type) {
			case GEOMETRY_BODY_POINT:
			{
				return mathSphereHasPoint(one->sphere->o, one->sphere->radius, two->point);
			}
			case GEOMETRY_BODY_AABB:
			{
				return mathAABBIntersectSphere(two->aabb->o, two->aabb->half, one->sphere->o, one->sphere->radius);
			}
			case GEOMETRY_BODY_SPHERE:
			{
				return mathSphereIntersectSphere(one->sphere->o, one->sphere->radius, two->sphere->o, two->sphere->radius, NULL);
			}
			case GEOMETRY_BODY_PLANE:
			{
				return mathSphereIntersectPlane(one->sphere->o, one->sphere->radius, two->plane->v, two->plane->normal, NULL, NULL);
			}
			case GEOMETRY_BODY_SEGMENT:
			{
				return mathSphereIntersectSegment(one->sphere->o, one->sphere->radius, two->segment->v, NULL);
			}
			case GEOMETRY_BODY_RECT:
			{
				return mathSphereIntersectRect(one->sphere->o, one->sphere->radius, two->rect, NULL);
			}
			case GEOMETRY_BODY_POLYGEN:
			{
				GeometryPolygenInner_t gp = { two->polygen };
				return mathSphereIntersectPolygen(one->sphere->o, one->sphere->radius, &gp, NULL);
			}
			default:
				return 0;
		}
	}
	else if (GEOMETRY_BODY_PLANE == one->type) {
		switch (two->type) {
			case GEOMETRY_BODY_POINT:
			{
				return mathPlaneHasPoint(one->plane->v, one->plane->normal, two->point);
			}
			case GEOMETRY_BODY_AABB:
			{
				return mathAABBIntersectPlane(two->aabb->o, two->aabb->half, one->plane->v, one->plane->normal, NULL);
			}
			case GEOMETRY_BODY_SPHERE:
			{
				return mathSphereIntersectPlane(two->sphere->o, two->sphere->radius, one->plane->v, one->plane->normal, NULL, NULL);
			}
			case GEOMETRY_BODY_PLANE:
			{
				return mathPlaneIntersectPlane(one->plane->v, one->plane->normal, two->plane->v, two->plane->normal);
			}
			case GEOMETRY_BODY_SEGMENT:
			{
				return mathSegmentIntersectPlane(two->segment->v, one->plane->v, one->plane->normal, NULL);
			}
			case GEOMETRY_BODY_RECT:
			{
				return mathRectIntersectPlane(two->rect, one->plane->v, one->plane->normal, NULL);
			}
			case GEOMETRY_BODY_POLYGEN:
			{
				GeometryPolygenInner_t gp = { two->polygen };
				return mathPolygenIntersectPlane(&gp, one->plane->v, one->plane->normal, NULL);
			}
			default:
				return 0;
		}
	}
	else if (GEOMETRY_BODY_RECT == one->type) {
		switch (two->type) {
			case GEOMETRY_BODY_POINT:
			{
				return mathRectHasPoint(one->rect, two->point);
			}
			case GEOMETRY_BODY_SEGMENT:
			{
				return mathSegmentIntersectRect(two->segment->v, one->rect, NULL);
			}
			case GEOMETRY_BODY_PLANE:
			{
				return mathRectIntersectPlane(one->rect, two->plane->v, two->plane->normal, NULL);
			}
			case GEOMETRY_BODY_SPHERE:
			{
				return mathSphereIntersectRect(one->sphere->o, one->sphere->radius, two->rect, NULL);
			}
			case GEOMETRY_BODY_AABB:
			{
				return mathAABBIntersectRect(one->aabb->o, one->aabb->half, two->rect, NULL);
			}
			case GEOMETRY_BODY_RECT:
			{
				return mathRectIntersectRect(one->rect, two->rect);
			}
			case GEOMETRY_BODY_POLYGEN:
			{
				float v[4][3];
				GeometryPolygen_t polygen1;
				GeometryPolygenInner_t gp1 = { &polygen1, one->rect };
				GeometryPolygenInner_t gp2 = { two->polygen };
				mathRectVertices(one->rect, v);
				polygen1.v_cnt = 4;
				polygen1.v = (const float(*)[3])v;
				mathVec3Copy(polygen1.normal, one->rect->normal);
				return mathPolygenIntersectPolygen(&gp1, &gp2);
			}
			default:
				return 0;
		}
	}
	else if (GEOMETRY_BODY_POLYGEN == one->type) {
		GeometryPolygenInner_t gp = { one->polygen };
		switch (two->type) {
			case GEOMETRY_BODY_POINT:
			{
				return polygen_inner_has_point(&gp, two->point);
			}
			case GEOMETRY_BODY_SEGMENT:
			{
				return mathSegmentIntersectPolygen(two->segment->v, &gp, NULL);
			}
			case GEOMETRY_BODY_PLANE:
			{
				return mathPolygenIntersectPlane(&gp, two->plane->v, two->plane->normal, NULL);
			}
			case GEOMETRY_BODY_SPHERE:
			{
				return mathSphereIntersectPolygen(two->sphere->o, two->sphere->radius, &gp, NULL);
			}
			case GEOMETRY_BODY_AABB:
			{
				return mathAABBIntersectPolygen(two->aabb->o, two->aabb->half, &gp, NULL);
			}
			case GEOMETRY_BODY_RECT:
			{
				float v[4][3];
				GeometryPolygen_t polygen;
				GeometryPolygenInner_t gp2 = { &polygen, two->rect };
				mathRectVertices(two->rect, v);
				polygen.v_cnt = 4;
				polygen.v = (const float(*)[3])v;
				mathVec3Copy(polygen.normal, two->rect->normal);
				return mathPolygenIntersectPolygen(&gp, &gp2);
			}
		}
	}
	return 0;
}

CCTResult_t* mathCollisionBodyCast(const GeometryBodyRef_t* one, const float dir[3], const GeometryBodyRef_t* two, CCTResult_t* result) {
	if (one == two || mathVec3IsZero(dir)) {
		return NULL;
	}
	else if (GEOMETRY_BODY_POINT == one->type) {
		switch (two->type) {
			case GEOMETRY_BODY_AABB:
			{
				return mathRaycastAABB(one->point, dir, two->aabb->o, two->aabb->half, result);
			}
			case GEOMETRY_BODY_SPHERE:
			{
				return mathRaycastSphere(one->point, dir, two->sphere->o, two->sphere->radius, result);
			}
			case GEOMETRY_BODY_PLANE:
			{
				return mathRaycastPlane(one->point, dir, two->plane->v, two->plane->normal, result);
			}
			case GEOMETRY_BODY_SEGMENT:
			{
				return mathRaycastSegment(one->point, dir, two->segment->v, result);
			}
			case GEOMETRY_BODY_RECT:
			{
				return mathRaycastRect(one->point, dir, two->rect, result);
			}
			case GEOMETRY_BODY_POLYGEN:
			{
				GeometryPolygenInner_t gp = { two->polygen };
				return mathRaycastPolygen(one->point, dir, &gp, result);
			}
			default:
				return NULL;
		}
	}
	else if (GEOMETRY_BODY_SEGMENT == one->type) {
		switch (two->type) {
			case GEOMETRY_BODY_SEGMENT:
			{
				return mathSegmentcastSegment(one->segment->v, dir, two->segment->v, result);
			}
			case GEOMETRY_BODY_PLANE:
			{
				return mathSegmentcastPlane(one->segment->v, dir, two->plane->v, two->plane->normal, result);
			}
			case GEOMETRY_BODY_AABB:
			{
				return mathSegmentcastAABB(one->segment->v, dir, two->aabb->o, two->aabb->half, result);
			}
			case GEOMETRY_BODY_SPHERE:
			{
				return mathSegmentcastSphere(one->segment->v, dir, two->sphere->o, two->sphere->radius, result);
			}
			case GEOMETRY_BODY_RECT:
			{
				return mathSegmentcastRect(one->segment->v, dir, two->rect, result);
			}
			case GEOMETRY_BODY_POLYGEN:
			{
				GeometryPolygenInner_t gp = { two->polygen };
				return mathSegmentcastPolygen(one->segment->v, dir, &gp, result);
			}
			default:
				return NULL;
		}
	}
	else if (GEOMETRY_BODY_AABB == one->type) {
		switch (two->type) {
			case GEOMETRY_BODY_AABB:
			{
				return mathAABBcastAABB(one->aabb->o, one->aabb->half, dir, two->aabb->o, two->aabb->half, result);
			}
			case GEOMETRY_BODY_SPHERE:
			{
				return mathAABBcastSphere(one->aabb->o, one->aabb->half, dir, two->sphere->o, two->sphere->radius, result);
			}
			case GEOMETRY_BODY_PLANE:
			{
				return mathAABBcastPlane(one->aabb->o, one->aabb->half, dir, two->plane->v, two->plane->normal, result);
			}
			case GEOMETRY_BODY_SEGMENT:
			{
				return mathAABBcastSegment(one->aabb->o, one->aabb->half, dir, two->segment->v, result);
			}
			case GEOMETRY_BODY_RECT:
			{
				return mathAABBcastRect(one->aabb->o, one->aabb->half, dir, two->rect, result);
			}
			case GEOMETRY_BODY_POLYGEN:
			{
				GeometryPolygenInner_t gp = { two->polygen };
				return mathAABBcastPolygen(one->aabb->o, one->aabb->half, dir, &gp, result);
			}
			default:
				return NULL;
		}
	}
	else if (GEOMETRY_BODY_SPHERE == one->type) {
		switch (two->type) {
			case GEOMETRY_BODY_AABB:
			{
				return mathSpherecastAABB(one->sphere->o, one->sphere->radius, dir, two->aabb->o, two->aabb->half, result);
			}
			case GEOMETRY_BODY_SPHERE:
			{
				return mathSpherecastSphere(one->sphere->o, one->sphere->radius, dir, two->sphere->o, two->sphere->radius, result);
			}
			case GEOMETRY_BODY_PLANE:
			{
				return mathSpherecastPlane(one->sphere->o, one->sphere->radius, dir, two->plane->v, two->plane->normal, result);
			}
			case GEOMETRY_BODY_SEGMENT:
			{
				return mathSpherecastSegment(one->sphere->o, one->sphere->radius, dir, two->segment->v, result);
			}
			case GEOMETRY_BODY_RECT:
			{
				return mathSpherecastRect(one->sphere->o, one->sphere->radius, dir, two->rect, result);
			}
			case GEOMETRY_BODY_POLYGEN:
			{
				GeometryPolygenInner_t gp = { two->polygen };
				return mathSpherecastPolygen(one->sphere->o, one->sphere->radius, dir, &gp, result);
			}
			default:
				return NULL;
		}
	}
	else if (GEOMETRY_BODY_RECT == one->type) {
		switch (two->type) {
			case GEOMETRY_BODY_SEGMENT:
			{
				return mathRectcastSegment(one->rect, dir, two->segment->v, result);
			}
			case GEOMETRY_BODY_PLANE:
			{
				return mathRectcastPlane(one->rect, dir, two->plane->v, two->plane->normal, result);
			}
			case GEOMETRY_BODY_SPHERE:
			{
				return mathRectcastSphere(one->rect, dir, two->sphere->o, two->sphere->radius, result);
			}
			case GEOMETRY_BODY_AABB:
			{
				return mathRectcastAABB(one->rect, dir, two->aabb->o, two->aabb->half, result);
			}
			case GEOMETRY_BODY_RECT:
			{
				return mathRectcastRect(one->rect, dir, two->rect, result);
			}
			case GEOMETRY_BODY_POLYGEN:
			{
				float v[4][3];
				GeometryPolygen_t polygen;
				GeometryPolygenInner_t gp1 = { &polygen, one->rect };
				GeometryPolygenInner_t gp2 = { two->polygen };
				mathRectVertices(one->rect, v);
				polygen.v_cnt = 4;
				polygen.v = (const float(*)[3])v;
				mathVec3Copy(polygen.normal, one->rect->normal);
				return mathPolygencastPolygen(&gp1, dir, &gp2, result);
			}
			default:
				return NULL;
		}
	}
	else if (GEOMETRY_BODY_POLYGEN == one->type) {
		GeometryPolygenInner_t gp = { one->polygen };
		switch (two->type) {
			case GEOMETRY_BODY_SEGMENT:
			{
				return mathPolygencastSegment(&gp, dir, two->segment->v, result);
			}
			case GEOMETRY_BODY_PLANE:
			{
				return mathPolygencastPlane(&gp, dir, two->plane->v, two->plane->normal, result);
			}
			case GEOMETRY_BODY_SPHERE:
			{
				return mathPolygencastSphere(&gp, dir, two->sphere->o, two->sphere->radius, result);
			}
			case GEOMETRY_BODY_AABB:
			{
				return mathPolygencastAABB(&gp, dir, two->aabb->o, two->aabb->half, result);
			}
			case GEOMETRY_BODY_RECT:
			{
				float v[4][3];
				GeometryPolygen_t polygen;
				GeometryPolygenInner_t gp2 = { &polygen, two->rect };
				mathRectVertices(two->rect, v);
				polygen.v_cnt = 4;
				polygen.v = (const float(*)[3])v;
				mathVec3Copy(polygen.normal, two->rect->normal);
				return mathPolygencastPolygen(&gp, dir, &gp2, result);
			}
			case GEOMETRY_BODY_POLYGEN:
			{
				GeometryPolygenInner_t gp2 = { two->polygen };
				return mathPolygencastPolygen(&gp, dir, &gp2, result);
			}
		}
	}
	return NULL;
}

#ifdef __cplusplus
}
#endif
