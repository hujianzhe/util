//
// Created by hujianzhe
//

#include "../../../inc/crt/math.h"
#include "../../../inc/crt/math_vec3.h"
#include "../../../inc/crt/geometry/collision_intersect.h"
#include "../../../inc/crt/geometry/collision_detection.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

extern int mathSegmentIntersectPlane(const float ls[2][3], const float plane_v[3], const float plane_normal[3], float p[3]);

extern int mathPolygenIntersectPolygen(const GeometryPolygen_t* polygen1, const GeometryPolygen_t* polygen2);

extern int mathSphereIntersectSegment(const float o[3], float radius, const float ls[2][3], float p[3]);

extern int mathSphereIntersectPlane(const float o[3], float radius, const float plane_v[3], const float plane_normal[3], float new_o[3], float* new_r);

extern int mathAABBIntersectPlane(const float o[3], const float half[3], const float plane_v[3], const float plane_n[3], float p[3]);

extern int mathAABBIntersectSphere(const float aabb_o[3], const float aabb_half[3], const float sp_o[3], float sp_radius);

extern int mathAABBIntersectSegment(const float o[3], const float half[3], const float ls[2][3]);

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

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

static CCTResult_t* mathRaycastPolygen(const float o[3], const float dir[3], const GeometryPolygen_t* polygen, CCTResult_t* result) {
	CCTResult_t* p_result;
	int i;
	float dot;
	if (!mathRaycastPlane(o, dir, polygen->v[polygen->v_indices[0]], polygen->normal, result)) {
		return NULL;
	}
	if (result->distance > CCT_EPSILON) {
		return mathPolygenHasPoint(polygen, result->hit_point) ? result : NULL;
	}
	dot = mathVec3Dot(dir, polygen->normal);
	if (dot < -CCT_EPSILON || dot > CCT_EPSILON) {
		return NULL;
	}
	p_result = NULL;
	for (i = 0; i < polygen->v_indices_cnt; ) {
		CCTResult_t result_temp;
		float edge[2][3];
		mathVec3Copy(edge[0], polygen->v[polygen->v_indices[i++]]);
		mathVec3Copy(edge[1], polygen->v[polygen->v_indices[i >= polygen->v_indices_cnt ? 0 : i]]);
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

static CCTResult_t* mathRaycastOBB(const float o[3], const float dir[3], const GeometryOBB_t* obb, CCTResult_t* result) {
	if (mathOBBHasPoint(obb, o)) {
		set_result(result, 0.0f, o, dir);
		return result;
	}
	else {
		CCTResult_t *p_result = NULL;
		int i;
		for (i = 0; i < 6; ) {
			CCTResult_t result_temp;
			GeometryRect_t rect;
			mathOBBPlaneRect(obb, i, &rect);
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

static CCTResult_t* mathRaycastAABB(const float o[3], const float dir[3], const float aabb_o[3], const float aabb_half[3], CCTResult_t* result) {
	GeometryOBB_t obb;
	mathOBBFromAABB(&obb, aabb_o, aabb_half);
	return mathRaycastOBB(o, dir, &obb, result);
}

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
		float v[8][3];
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

static CCTResult_t* mathSegmentcastPolygen(const float ls[2][3], const float dir[3], const GeometryPolygen_t* polygen, CCTResult_t* result) {
	CCTResult_t *p_result;
	int i;
	if (!mathSegmentcastPlane(ls, dir, polygen->v[polygen->v_indices[0]], polygen->normal, result)) {
		return NULL;
	}
	if (result->hit_point_cnt > 0) {
		if (mathPolygenHasPoint(polygen, result->hit_point)) {
			return result;
		}
	}
	else {
		for (i = 0; i < 2; ++i) {
			float test_p[3];
			mathVec3Copy(test_p, ls[i]);
			mathVec3AddScalar(test_p, dir, result->distance);
			if (mathPolygenHasPoint(polygen, test_p)) {
				return result;
			}
		}
	}
	p_result = NULL;
	for (i = 0; i < polygen->v_indices_cnt; ) {
		CCTResult_t result_temp;
		float edge[2][3];
		mathVec3Copy(edge[0], polygen->v[polygen->v_indices[i++]]);
		mathVec3Copy(edge[1], polygen->v[polygen->v_indices[i >= polygen->v_indices_cnt ? 0 : i]]);
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

static CCTResult_t* mathPolygencastSegment(const GeometryPolygen_t* polygen, const float dir[3], const float ls[2][3], CCTResult_t* result) {
	float neg_dir[3];
	mathVec3Negate(neg_dir, dir);
	if (!mathSegmentcastPolygen(ls, neg_dir, polygen, result)) {
		return NULL;
	}
	if (result->hit_point_cnt > 0) {
		mathVec3AddScalar(result->hit_point, dir, result->distance);
	}
	return result;
}

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

static CCTResult_t* mathPolygencastPlane(const GeometryPolygen_t* polygen, const float dir[3], const float plane_v[3], const float plane_n[3], CCTResult_t* result) {
	int i, has_gt0 = 0, has_le0 = 0, idx_0 = -1, idx_min = -1;
	float min_d, dot;
	for (i = 0; i < polygen->v_indices_cnt; ++i) {
		int cmp;
		float d;
		mathPointProjectionPlane(polygen->v[polygen->v_indices[i]], plane_v, plane_n, NULL, &d);
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
		set_result(result, 0.0f, polygen->v[polygen->v_indices[idx_0]], plane_n);
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
	set_result(result, min_d, polygen->v[polygen->v_indices[idx_min]], plane_n);
	mathVec3AddScalar(result->hit_point, dir, min_d);
	return result;
}

static CCTResult_t* mathPolygencastPolygen(const GeometryPolygen_t* polygen1, const float dir[3], const GeometryPolygen_t* polygen2, CCTResult_t* result) {
	CCTResult_t* p_result;
	int i, flag;
	float neg_dir[3];
	if (mathPolygenIntersectPolygen(polygen1, polygen2)) {
		set_result(result, 0.0f, NULL, dir);
		return result;
	}
	flag = mathPlaneIntersectPlane(polygen1->v[polygen1->v_indices[0]], polygen1->normal, polygen2->v[polygen2->v_indices[0]], polygen2->normal);
	if (0 == flag) {
		float d, dot;
		mathPointProjectionPlane(polygen1->v[polygen1->v_indices[0]], polygen2->v[polygen2->v_indices[0]], polygen2->normal, NULL, &d);
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
	for (i = 0; i < polygen1->v_indices_cnt; ) {
		CCTResult_t result_temp;
		float edge[2][3];
		mathVec3Copy(edge[0], polygen1->v[polygen1->v_indices[i++]]);
		mathVec3Copy(edge[1], polygen1->v[polygen1->v_indices[i >= polygen1->v_indices_cnt ? 0 : i]]);
		if (!mathSegmentcastPolygen((const float(*)[3])edge, dir, polygen2, &result_temp)) {
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
	for (i = 0; i < polygen2->v_indices_cnt; ) {
		CCTResult_t result_temp;
		float edge[2][3];
		mathVec3Copy(edge[0], polygen2->v[polygen2->v_indices[i++]]);
		mathVec3Copy(edge[1], polygen2->v[polygen2->v_indices[i >= polygen2->v_indices_cnt ? 0 : i]]);
		if (!mathSegmentcastPolygen((const float(*)[3])edge, neg_dir, polygen1, &result_temp)) {
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

static CCTResult_t* mathBoxCastPlane(const float v[8][3], const float dir[3], const float plane_v[3], const float plane_n[3], CCTResult_t* result) {
	CCTResult_t* p_result = NULL;
	int i, unhit = 0;
	for (i = 0; i < 8; ++i) {
		int cmp;
		CCTResult_t result_temp;
		if (!mathRaycastPlane(v[i], dir, plane_v, plane_n, &result_temp)) {
			if (p_result) {
				set_result(result, 0.0f, NULL, plane_n);
				return result;
			}
			unhit = 1;
			continue;
		}
		if (unhit) {
			set_result(result, 0.0f, NULL, plane_n);
			return result;
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

static CCTResult_t* mathAABBcastPlane(const float o[3], const float half[3], const float dir[3], const float plane_v[3], const float plane_n[3], CCTResult_t* result) {
	float v[8][3];
	mathAABBVertices(o, half, v);
	return mathBoxCastPlane((const float(*)[3])v, dir, plane_v, plane_n, result);
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

static CCTResult_t* mathAABBcastPolygen(const float o[3], const float half[3], const float dir[3], const GeometryPolygen_t* polygen, CCTResult_t* result) {
	int i;
	float v[8][3], neg_dir[3];
	CCTResult_t *p_result;
	if (!mathAABBcastPlane(o, half, dir, polygen->v[polygen->v_indices[0]], polygen->normal, result)) {
		return NULL;
	}
	if (result->hit_point_cnt > 0) {
		if (mathPolygenHasPoint(polygen, result->hit_point)) {
			return result;
		}
	}
	else {
		mathAABBVertices(o, half, v);
		for (i = 0; i < 8; ++i) {
			float test_p[3];
			mathVec3Copy(test_p, v[i]);
			mathVec3AddScalar(test_p, dir, result->distance);
			if (mathPolygenHasPoint(polygen, test_p)) {
				return result;
			}
		}
	}
	p_result = NULL;
	mathVec3Negate(neg_dir, dir);
	for (i = 0; i < polygen->v_indices_cnt; ) {
		CCTResult_t result_temp;
		float edge[2][3];
		mathVec3Copy(edge[0], polygen->v[polygen->v_indices[i++]]);
		mathVec3Copy(edge[1], polygen->v[polygen->v_indices[i >= polygen->v_indices_cnt ? 0 : i]]);
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

static CCTResult_t* mathPolygencastAABB(const GeometryPolygen_t* polygen, const float dir[3], const float o[3], const float half[3], CCTResult_t* result) {
	float neg_dir[3];
	mathVec3Negate(neg_dir, dir);
	if (!mathAABBcastPolygen(o, half, neg_dir, polygen, result)) {
		return NULL;
	}
	if (result->hit_point_cnt > 0) {
		mathVec3AddScalar(result->hit_point, dir, result->distance);
	}
	return result;
}

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

static CCTResult_t* mathSpherecastPolygen(const float o[3], float radius, const float dir[3], const GeometryPolygen_t* polygen, CCTResult_t* result) {
	int i;
	CCTResult_t *p_result = NULL;
	float neg_dir[3];
	if (!mathSpherecastPlane(o, radius, dir, polygen->v[polygen->v_indices[0]], polygen->normal, result)) {
		return NULL;
	}
	if (result->hit_point_cnt > 0) {
		if (mathPolygenHasPoint(polygen, result->hit_point)) {
			return result;
		}
	}
	p_result = NULL;
	mathVec3Negate(neg_dir, dir);
	for (i = 0; i < polygen->v_indices_cnt; ) {
		CCTResult_t result_temp;
		float edge[2][3];
		mathVec3Copy(edge[0], polygen->v[polygen->v_indices[i++]]);
		mathVec3Copy(edge[1], polygen->v[polygen->v_indices[i >= polygen->v_indices_cnt ? 0 : i]]);
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

static CCTResult_t* mathPolygencastSphere(const GeometryPolygen_t* polygen, const float dir[3], const float o[3], float radius, CCTResult_t* result) {
	float neg_dir[3];
	mathVec3Negate(neg_dir, dir);
	if (!mathSpherecastPolygen(o, radius, dir, polygen, result)) {
		return NULL;
	}
	if (result->hit_point_cnt > 0) {
		mathVec3AddScalar(result->hit_point, dir, result->distance);
	}
	return result;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

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
			case GEOMETRY_BODY_OBB:
			{
				return mathRaycastOBB(one->point, dir, two->obb, result);
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
			case GEOMETRY_BODY_POLYGEN:
			{
				return mathRaycastPolygen(one->point, dir, two->polygen, result);
			}
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
			case GEOMETRY_BODY_POLYGEN:
			{
				return mathSegmentcastPolygen(one->segment->v, dir, two->polygen, result);
			}
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
			case GEOMETRY_BODY_POLYGEN:
			{
				return mathAABBcastPolygen(one->aabb->o, one->aabb->half, dir, two->polygen, result);
			}
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
			case GEOMETRY_BODY_POLYGEN:
			{
				return mathSpherecastPolygen(one->sphere->o, one->sphere->radius, dir, two->polygen, result);
			}
		}
	}
	else if (GEOMETRY_BODY_POLYGEN == one->type) {
		switch (two->type) {
			case GEOMETRY_BODY_SEGMENT:
			{
				return mathPolygencastSegment(one->polygen, dir, two->segment->v, result);
			}
			case GEOMETRY_BODY_PLANE:
			{
				return mathPolygencastPlane(one->polygen, dir, two->plane->v, two->plane->normal, result);
			}
			case GEOMETRY_BODY_SPHERE:
			{
				return mathPolygencastSphere(one->polygen, dir, two->sphere->o, two->sphere->radius, result);
			}
			case GEOMETRY_BODY_AABB:
			{
				return mathPolygencastAABB(one->polygen, dir, two->aabb->o, two->aabb->half, result);
			}
			case GEOMETRY_BODY_POLYGEN:
			{
				return mathPolygencastPolygen(one->polygen, dir, two->polygen, result);
			}
		}
	}
	return NULL;
}

#ifdef __cplusplus
}
#endif
