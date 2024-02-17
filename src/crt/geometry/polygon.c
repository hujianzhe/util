//
// Created by hujianzhe
//

#include "../../../inc/crt/math_vec3.h"
#include "../../../inc/crt/geometry/vertex.h"
#include "../../../inc/crt/geometry/plane.h"
#include "../../../inc/crt/geometry/polygon.h"
#include <stdlib.h>

static const unsigned int DEFAULT_TRIANGLE_VERTICE_INDICES[3] = { 0, 1, 2 };
static const unsigned int DEFAULT_RECT_VERTICE_INDICES[4] = { 0, 1, 2, 3 };

int Polygon_Convex_HasPoint_InternalProc(const GeometryPolygon_t* polygon, const float p[3]) {
	unsigned int i;
	float v[3], dot;
	float vp[3], eg[3];

	mathVec3Sub(v, polygon->v[polygon->v_indices[0]], p);
	dot = mathVec3Dot(polygon->normal, v);
	if (dot > CCT_EPSILON || dot < CCT_EPSILON_NEGATE) {
		return 0;
	}
	mathVec3Sub(vp, p, polygon->v[polygon->v_indices[0]]);
	mathVec3Sub(eg, polygon->v[polygon->v_indices[0]], polygon->v[polygon->v_indices[polygon->v_indices_cnt - 1]]);
	mathVec3Cross(v, vp, eg);
	if (mathVec3IsZero(v) && mathVec3LenSq(vp) <= mathVec3LenSq(eg)) {
		return 1;
	}
	for (i = 1; i < polygon->v_indices_cnt; ++i) {
		float vi[3];
		mathVec3Sub(vp, p, polygon->v[polygon->v_indices[i]]);
		mathVec3Sub(eg, polygon->v[polygon->v_indices[i]], polygon->v[polygon->v_indices[i - 1]]);
		mathVec3Cross(vi, vp, eg);
		if (mathVec3IsZero(vi) && mathVec3LenSq(vp) <= mathVec3LenSq(eg)) {
			return 1;
		}
		dot = mathVec3Dot(v, vi);
		if (dot <= 0.0f) {
			return 0;
		}
	}
	return 1;
}

GeometryPolygon_t* PolygonCooking_InternalProc(const float(*v)[3], const unsigned int* tri_indices, unsigned int tri_indices_cnt, GeometryPolygon_t* polygon) {
	unsigned int i, s, n, p, last_s, first_s;
	unsigned int* tmp_edge_pair_indices = NULL;
	unsigned int tmp_edge_pair_indices_cnt = 0;
	unsigned int* tmp_edge_indices = NULL;
	unsigned int tmp_edge_indices_cnt = 0;
	unsigned int* ret_v_indices = NULL;
	unsigned int ret_v_indices_cnt = 0;
	float v1[3], v2[3], N[3];

	if (tri_indices_cnt < 3 || tri_indices_cnt % 3 != 0) {
		return NULL;
	}
	mathPlaneNormalByVertices3(v[tri_indices[0]], v[tri_indices[1]], v[tri_indices[2]], N);
	if (mathVec3IsZero(N)) {
		return NULL;
	}
	mathVec3Copy(polygon->normal, N);
	if (tri_indices_cnt == 3) {
		ret_v_indices = (unsigned int*)malloc(3 * sizeof(ret_v_indices[0]));
		if (!ret_v_indices) {
			return NULL;
		}
		ret_v_indices[0] = tri_indices[0];
		ret_v_indices[1] = tri_indices[1];
		ret_v_indices[2] = tri_indices[2];

		polygon->v_indices = ret_v_indices;
		polygon->v_indices_cnt = 3;
		return polygon;
	}
	/* Filters all share triangle edges, leaving all non-shared edges */
	for (i = 0; i < tri_indices_cnt; i += 3) {
		unsigned int ei[6] = {
			tri_indices[i], tri_indices[i + 1],
			tri_indices[i + 1], tri_indices[i + 2],
			tri_indices[i + 2], tri_indices[i]
		};
		int same[3] = { 0 };
		unsigned int j;
		for (j = 0; j < tri_indices_cnt; j += 3) {
			unsigned int k;
			unsigned int ej[6];
			if (i == j) {
				continue;
			}
			for (k = 0; k < 3; ++k) {
				if (!mathPlaneHasPoint(v[tri_indices[i]], N, v[tri_indices[j + k]])) {
					free(tmp_edge_pair_indices);
					return NULL;
				}
			}
			ej[0] = tri_indices[j]; ej[1] = tri_indices[j + 1];
			ej[2] = tri_indices[j + 1]; ej[3] = tri_indices[j + 2];
			ej[4] = tri_indices[j + 2]; ej[5] = tri_indices[j];
			for (k = 0; k < 6; k += 2) {
				unsigned int m;
				if (same[k >> 1]) {
					continue;
				}
				for (m = 0; m < 6; m += 2) {
					if (ei[k] == ej[m] || mathVec3Equal(v[ei[k]], v[ej[m]])) {
						same[k >> 1] = (ei[k + 1] == ej[m + 1] || mathVec3Equal(v[ei[k + 1]], v[ej[m + 1]]));
					}
					else if (ei[k] == ej[m + 1] || mathVec3Equal(v[ei[k]], v[ej[m + 1]])) {
						same[k >> 1] = (ei[k + 1] == ej[m] || mathVec3Equal(v[ei[k + 1]], v[ej[m]]));
					}
					else {
						continue;
					}
					if (same[k >> 1]) {
						break;
					}
				}
			}
			if (same[0] && same[1] && same[2]) {
				break;
			}
		}
		if (j != tri_indices_cnt) {
			continue;
		}
		if (!same[0] && !same[1] && !same[2]) {
			free(tmp_edge_pair_indices);
			return NULL;
		}
		for (j = 0; j < 6; j += 2) {
			unsigned int* ptr;
			if (same[j >> 1]) {
				continue;
			}
			tmp_edge_pair_indices_cnt += 2;
			ptr = (unsigned int*)realloc(tmp_edge_pair_indices, tmp_edge_pair_indices_cnt * sizeof(tmp_edge_pair_indices[0]));
			if (!ptr) {
				free(tmp_edge_pair_indices);
				return NULL;
			}
			tmp_edge_pair_indices = ptr;
			tmp_edge_pair_indices[tmp_edge_pair_indices_cnt - 2] = ei[j];
			tmp_edge_pair_indices[tmp_edge_pair_indices_cnt - 1] = ei[j + 1];
		}
	}
	if (!tmp_edge_pair_indices) {
		return NULL;
	}
	/* Calculates the order of edge vertex traversal */
	tmp_edge_indices = (unsigned int*)malloc(sizeof(tmp_edge_indices[0]) * tmp_edge_pair_indices_cnt);
	if (!tmp_edge_indices) {
		free(tmp_edge_pair_indices);
		return NULL;
	}
	s = tmp_edge_pair_indices[0];
	n = tmp_edge_pair_indices[1];
	tmp_edge_indices[tmp_edge_indices_cnt++] = s;
	tmp_edge_indices[tmp_edge_indices_cnt++] = n;
	p = 0;
	for (i = 2; i < tmp_edge_pair_indices_cnt; i += 2) {
		if (i == p) {
			continue;
		}
		if (n == tmp_edge_pair_indices[i] || mathVec3Equal(v[n], v[tmp_edge_pair_indices[i]])) {
			n = tmp_edge_pair_indices[i + 1];
		}
		else if (n == tmp_edge_pair_indices[i + 1] || mathVec3Equal(v[n], v[tmp_edge_pair_indices[i + 1]])) {
			n = tmp_edge_pair_indices[i];
		}
		else {
			continue;
		}
		tmp_edge_indices[tmp_edge_indices_cnt++] = n;
		if (s == n) {
			break;
		}
		if (mathVec3Equal(v[s], v[n])) {
			n = s;
			break;
		}
		p = i;
		i = 0;
	}
	free(tmp_edge_pair_indices);
	if (s != n || tmp_edge_indices_cnt < 2) {
		free(tmp_edge_indices);
		return NULL;
	}
	/* Merge vertices on the same edge */
	--tmp_edge_indices_cnt;
	++ret_v_indices_cnt;
	last_s = 0;
	first_s = -1;
	for (i = 1; i < tmp_edge_indices_cnt; ++i) {
		mathVec3Sub(v1, v[tmp_edge_indices[i]], v[tmp_edge_indices[last_s]]);
		mathVec3Sub(v2, v[tmp_edge_indices[i]], v[tmp_edge_indices[i + 1]]);
		mathVec3Cross(N, v1, v2);
		if (!mathVec3IsZero(N)) {
			last_s = i;
			if (-1 == first_s) {
				first_s = i;
			}
			++ret_v_indices_cnt;
			continue;
		}
		tmp_edge_indices[i] = -1;
	}
	mathVec3Sub(v1, v[tmp_edge_indices[0]], v[tmp_edge_indices[last_s]]);
	mathVec3Sub(v2, v[tmp_edge_indices[0]], v[tmp_edge_indices[first_s]]);
	mathVec3Cross(N, v1, v2);
	if (mathVec3IsZero(N)) {
		tmp_edge_indices[0] = -1;
		--ret_v_indices_cnt;
	}
	ret_v_indices = (unsigned int*)malloc(ret_v_indices_cnt * sizeof(ret_v_indices[0]));
	if (!ret_v_indices) {
		free(tmp_edge_indices);
		return NULL;
	}
	n = 0;
	for (i = 0; i < tmp_edge_indices_cnt && n < ret_v_indices_cnt; ++i) {
		if (-1 == tmp_edge_indices[i]) {
			continue;
		}
		ret_v_indices[n++] = tmp_edge_indices[i];
	}
	free(tmp_edge_indices);
	/* save result */
	polygon->v_indices = ret_v_indices;
	polygon->v_indices_cnt = ret_v_indices_cnt;
	return polygon;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#ifdef	__cplusplus
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
	float ap[3], ab[3], ac[3], N[3], dot;
	mathVec3Sub(ap, p, tri[0]);
	mathVec3Sub(ab, tri[1], tri[0]);
	mathVec3Sub(ac, tri[2], tri[0]);
	mathVec3Cross(N, ab, ac);
	dot = mathVec3Dot(N, ap);
	if (dot > CCT_EPSILON || dot < CCT_EPSILON_NEGATE) {
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
		if (u < 0.0f || u > 1.0f) {
			return 0;
		}
		v = (dot_ac_ac * dot_ab_ap - dot_ac_ab * dot_ac_ap) * tmp;
		if (v < 0.0f || v + u > 1.0f) {
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

void mathTriangleToPolygon(const float tri[3][3], GeometryPolygon_t* polygon) {
	polygon->v_indices = DEFAULT_TRIANGLE_VERTICE_INDICES;
	polygon->v_indices_cnt = 3;
	polygon->v = (float(*)[3])tri;
	mathPlaneNormalByVertices3(tri[0], tri[1], tri[2], polygon->normal);
}

int mathRectHasPoint(const GeometryRect_t* rect, const float p[3]) {
	float v[3], dot;
	mathVec3Sub(v, p, rect->o);
	dot = mathVec3Dot(rect->normal, v);
	if (dot > CCT_EPSILON || dot < CCT_EPSILON_NEGATE) {
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

void mathRectToPolygon(const GeometryRect_t* rect, GeometryPolygon_t* polygon, float buf_points[4][3]) {
	mathRectVertices(rect, buf_points);
	polygon->v_indices = DEFAULT_RECT_VERTICE_INDICES;
	polygon->v_indices_cnt = 4;
	polygon->v = buf_points;
	mathVec3Copy(polygon->normal, rect->normal);
}

GeometryRect_t* mathAABBPlaneRect(const float o[3], const float half[3], unsigned int idx, GeometryRect_t* rect) {
	if (idx < 2) {
		mathVec3Copy(rect->o, o);
		if (0 == idx) {
			rect->o[2] += half[2];
			mathVec3Set(rect->normal, 0.0f, 0.0f, 1.0f);
		}
		else {
			rect->o[2] -= half[2];
			mathVec3Set(rect->normal, 0.0f, 0.0f, -1.0f);
		}
		rect->half_w = half[0];
		rect->half_h = half[1];
		mathVec3Set(rect->w_axis, 1.0f, 0.0f, 0.0f);
		mathVec3Set(rect->h_axis, 0.0f, 1.0f, 0.0f);
		return rect;
	}
	else if (idx < 4) {
		mathVec3Copy(rect->o, o);
		if (2 == idx) {
			rect->o[0] += half[0];
			mathVec3Set(rect->normal, 1.0f, 0.0f, 0.0f);
		}
		else {
			rect->o[0] -= half[0];
			mathVec3Set(rect->normal, -1.0f, 0.0f, 0.0f);
		}
		rect->half_w = half[2];
		rect->half_h = half[1];
		mathVec3Set(rect->w_axis, 0.0f, 0.0f, 1.0f);
		mathVec3Set(rect->h_axis, 0.0f, 1.0f, 0.0f);
		return rect;
	}
	else if (idx < 6) {
		mathVec3Copy(rect->o, o);
		if (4 == idx) {
			rect->o[1] += half[1];
			mathVec3Set(rect->normal, 0.0f, 1.0f, 0.0f);
		}
		else {
			rect->o[1] -= half[1];
			mathVec3Set(rect->normal, 0.0f, -1.0f, 0.0f);
		}
		rect->half_w = half[2];
		rect->half_h = half[0];
		mathVec3Set(rect->w_axis, 0.0f, 0.0f, 1.0f);
		mathVec3Set(rect->h_axis, 1.0f, 0.0f, 0.0f);
		return rect;
	}
	return NULL;
}

GeometryRect_t* mathOBBPlaneRect(const GeometryOBB_t* obb, unsigned int idx, GeometryRect_t* rect) {
	float extend[3];
	if (idx < 2) {
		mathVec3MultiplyScalar(extend, obb->axis[2], obb->half[2]);
		if (0 == idx) {
			mathVec3Add(rect->o, obb->o, extend);
			rect->normal[0] = obb->axis[2][0];
			rect->normal[1] = obb->axis[2][1];
			rect->normal[2] = obb->axis[2][2];
		}
		else {
			mathVec3Sub(rect->o, obb->o, extend);
			rect->normal[0] = -obb->axis[2][0];
			rect->normal[1] = -obb->axis[2][1];
			rect->normal[2] = -obb->axis[2][2];
		}
		rect->half_w = obb->half[0];
		rect->half_h = obb->half[1];
		mathVec3Copy(rect->w_axis, obb->axis[0]);
		mathVec3Copy(rect->h_axis, obb->axis[1]);
		return rect;
	}
	else if (idx < 4) {
		mathVec3MultiplyScalar(extend, obb->axis[0], obb->half[0]);
		if (2 == idx) {
			mathVec3Add(rect->o, obb->o, extend);
			rect->normal[0] = obb->axis[0][0];
			rect->normal[1] = obb->axis[0][1];
			rect->normal[2] = obb->axis[0][2];
		}
		else {
			mathVec3Sub(rect->o, obb->o, extend);
			rect->normal[0] = -obb->axis[0][0];
			rect->normal[1] = -obb->axis[0][1];
			rect->normal[2] = -obb->axis[0][2];
		}
		rect->half_w = obb->half[2];
		rect->half_h = obb->half[1];
		mathVec3Copy(rect->w_axis, obb->axis[2]);
		mathVec3Copy(rect->h_axis, obb->axis[1]);
		return rect;
	}
	else if (idx < 6) {
		mathVec3MultiplyScalar(extend, obb->axis[1], obb->half[1]);
		if (4 == idx) {
			mathVec3Add(rect->o, obb->o, extend);
			rect->normal[0] = obb->axis[1][0];
			rect->normal[1] = obb->axis[1][1];
			rect->normal[2] = obb->axis[1][2];
		}
		else {
			mathVec3Sub(rect->o, obb->o, extend);
			rect->normal[0] = -obb->axis[1][0];
			rect->normal[1] = -obb->axis[1][1];
			rect->normal[2] = -obb->axis[1][2];
		}
		rect->half_w = obb->half[2];
		rect->half_h = obb->half[0];
		mathVec3Copy(rect->w_axis, obb->axis[2]);
		mathVec3Copy(rect->h_axis, obb->axis[0]);
		return rect;
	}
	return NULL;
}

int mathPolygonIsConvex(const GeometryPolygon_t* polygon) {
	float e1[3], e2[3], test_n[3];
	unsigned int i, has_test_n;
	if (polygon->v_indices_cnt < 3) {
		return 0;
	}
	has_test_n = 0;
	for (i = 0; i < polygon->v_indices_cnt; ++i) {
		float n[3];
		unsigned int v_idx1, v_idx2;
		if (i) {
			v_idx1 = polygon->v_indices[i - 1];
			v_idx2 = polygon->v_indices[i + 1 < polygon->v_indices_cnt ? i + 1 : 0];
		}
		else {
			v_idx1 = polygon->v_indices[polygon->v_indices_cnt - 1];
			v_idx2 = polygon->v_indices[1];
		}
		mathVec3Sub(e1, polygon->v[polygon->v_indices[i]], polygon->v[v_idx1]);
		mathVec3Sub(e2, polygon->v[v_idx2], polygon->v[polygon->v_indices[i]]);
		if (!has_test_n) {
			mathVec3Cross(test_n, e1, e2);
			if (!mathVec3IsZero(test_n)) {
				has_test_n = 1;
			}
			continue;
		}
		mathVec3Cross(n, e1, e2);
		if (mathVec3Dot(test_n, n) < 0.0f) {
			return 0;
		}
	}
	return 1;
}

int mathPolygonHasPoint(const GeometryPolygon_t* polygon, const float p[3]) {
	if (polygon->v_indices_cnt < 3) {
		return 0;
	}
	if (3 == polygon->v_indices_cnt) {
		float tri[3][3];
		mathVec3Copy(tri[0], polygon->v[polygon->v_indices[0]]);
		mathVec3Copy(tri[1], polygon->v[polygon->v_indices[1]]);
		mathVec3Copy(tri[2], polygon->v[polygon->v_indices[2]]);
		return mathTrianglePointUV((const float(*)[3])tri, p, NULL, NULL);
	}
	if (polygon->v_indices == DEFAULT_RECT_VERTICE_INDICES) {
		float ls_vec[3], v[3], dot;
		mathVec3Sub(v, p, polygon->v[polygon->v_indices[0]]);
		dot = mathVec3Dot(polygon->normal, v);
		if (dot < CCT_EPSILON_NEGATE || dot > CCT_EPSILON) {
			return 0;
		}
		mathVec3Sub(ls_vec, polygon->v[polygon->v_indices[1]], polygon->v[polygon->v_indices[0]]);
		dot = mathVec3Dot(ls_vec, v);
		if (dot < CCT_EPSILON_NEGATE || dot > mathVec3LenSq(ls_vec)) {
			return 0;
		}
		mathVec3Sub(ls_vec, polygon->v[polygon->v_indices[3]], polygon->v[polygon->v_indices[0]]);
		dot = mathVec3Dot(ls_vec, v);
		if (dot < CCT_EPSILON_NEGATE || dot > mathVec3LenSq(ls_vec)) {
			return 0;
		}
		return 1;
	}
	if (polygon->tri_indices && polygon->tri_indices_cnt >= 3) {
		unsigned int i;
		for (i = 0; i < polygon->tri_indices_cnt; ) {
			float tri[3][3];
			mathVec3Copy(tri[0], polygon->v[polygon->tri_indices[i++]]);
			mathVec3Copy(tri[1], polygon->v[polygon->tri_indices[i++]]);
			mathVec3Copy(tri[2], polygon->v[polygon->tri_indices[i++]]);
			if (mathTrianglePointUV((const float(*)[3])tri, p, NULL, NULL)) {
				return 1;
			}
		}
		return 0;
	}
	return Polygon_Convex_HasPoint_InternalProc(polygon, p);
}

int mathPolygonContainPolygon(const GeometryPolygon_t* polygon1, const GeometryPolygon_t* polygon2) {
	int ret = mathPlaneIntersectPlane(polygon1->v[polygon1->v_indices[0]], polygon1->normal, polygon2->v[polygon2->v_indices[0]], polygon2->normal);
	if (2 == ret) {
		unsigned int i;
		for (i = 0; i < polygon2->v_indices_cnt; ++i) {
			const float* p = polygon2->v[polygon2->v_indices[i]];
			if (!mathPolygonHasPoint(polygon1, p)) {
				return 0;
			}
		}
		return 1;
	}
	return 0;
}

GeometryPolygon_t* mathPolygonCooking(const float(*v)[3], unsigned int v_cnt, const unsigned int* tri_indices, unsigned int tri_indices_cnt, GeometryPolygon_t* polygon) {
	float(*dup_v)[3] = NULL;
	unsigned int dup_v_cnt;
	unsigned int* dup_tri_indices = NULL;

	if (v_cnt < 3 || tri_indices_cnt < 3) {
		return NULL;
	}
	dup_v_cnt = mathVerticesDistinctCount(v, v_cnt);
	if (dup_v_cnt < 3) {
		return NULL;
	}
	dup_v = (float(*)[3])malloc(sizeof(dup_v[0]) * dup_v_cnt);
	if (!dup_v) {
		goto err;
	}
	dup_tri_indices = (unsigned int*)malloc(sizeof(tri_indices[0]) * tri_indices_cnt);
	if (!dup_tri_indices) {
		goto err;
	}
	mathVerticesMerge(v, v_cnt, tri_indices, tri_indices_cnt, dup_v, dup_tri_indices);
	if (!PolygonCooking_InternalProc((const float(*)[3])dup_v, dup_tri_indices, tri_indices_cnt, polygon)) {
		goto err;
	}
	polygon->v = dup_v;
	polygon->tri_indices = dup_tri_indices;
	polygon->tri_indices_cnt = tri_indices_cnt;
	return polygon;
err:
	free(dup_v);
	free(dup_tri_indices);
	return NULL;
}

void mathPolygonFreeCookingData(GeometryPolygon_t* polygon) {
	if (!polygon) {
		return;
	}
	if (polygon->tri_indices) {
		free((void*)polygon->tri_indices);
		polygon->tri_indices = NULL;
		polygon->tri_indices_cnt = 0;
	}
	if (polygon->v_indices) {
		free((void*)polygon->v_indices);
		polygon->v_indices = NULL;
		polygon->v_indices_cnt = 0;
	}
	if (polygon->v) {
		free(polygon->v);
		polygon->v = NULL;
	}
}

#ifdef	__cplusplus
}
#endif
