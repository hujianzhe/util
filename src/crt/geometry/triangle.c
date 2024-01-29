//
// Created by hujianzhe
//

#include "../../../inc/crt/math.h"
#include "../../../inc/crt/math_vec3.h"
#include "../../../inc/crt/geometry/plane.h"
#include "../../../inc/crt/geometry/line_segment.h"
#include "../../../inc/crt/geometry/triangle.h"
#include <stddef.h>
#include <stdlib.h>

static const unsigned int DEFAULT_TRIANGLE_VERTICE_INDICES[3] = { 0, 1, 2 };
static const unsigned int DEFAULT_RECT_VERTICE_INDICES[4] = { 0, 1, 2, 3 };

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

void mathRectToPolygon(const GeometryRect_t* rect, GeometryPolygon_t* polygon, float p[4][3]) {
	mathRectVertices(rect, p);
	polygon->v_indices = DEFAULT_RECT_VERTICE_INDICES;
	polygon->v_indices_cnt = 4;
	polygon->v = p;
	mathVec3Copy(polygon->normal, rect->normal);
}

unsigned int mathVerticesDistinctCount(const float(*src_v)[3], unsigned int src_v_cnt) {
	unsigned int i, len = src_v_cnt;
	for (i = 0; i < src_v_cnt; ++i) {
		unsigned int j;
		for (j = i + 1; j < src_v_cnt; ++j) {
			if (mathVec3Equal(src_v[i], src_v[j])) {
				--len;
				break;
			}
		}
	}
	return len;
}

unsigned int mathVerticesMerge(const float(*src_v)[3], unsigned int src_v_cnt, float(*dst_v)[3], unsigned int* indices, unsigned int indices_cnt) {
	unsigned int i, dst_v_cnt = 0;
	for (i = 0; i < src_v_cnt; ++i) {
		unsigned int j;
		for (j = 0; j < dst_v_cnt; ++j) {
			if (mathVec3Equal(src_v[i], dst_v[j])) {
				break;
			}
		}
		if (j < dst_v_cnt) {
			continue;
		}
		for (j = 0; j < indices_cnt; ++j) {
			if (indices[j] == i || mathVec3Equal(src_v[indices[j]], src_v[i])) {
				indices[j] = dst_v_cnt;
			}
		}
		mathVec3Copy(dst_v[dst_v_cnt++], src_v[i]);
	}
	return dst_v_cnt;
}

int mathPolygonIsConvex(const float(*v)[3], const unsigned int* v_indices, unsigned int v_indices_cnt) {
	float e1[3], e2[3], test_n[3];
	unsigned int i, has_test_n;
	if (v_indices_cnt < 3) {
		return 0;
	}
	has_test_n = 0;
	for (i = 0; i < v_indices_cnt; ++i) {
		float n[3];
		unsigned int v_idx1, v_idx2;
		if (i) {
			v_idx1 = v_indices[i - 1];
			v_idx2 = v_indices[i + 1 < v_indices_cnt ? i + 1 : 0];
		}
		else {
			v_idx1 = v_indices[v_indices_cnt - 1];
			v_idx2 = v_indices[1];
		}
		mathVec3Sub(e1, v[v_indices[i]], v[v_idx1]);
		mathVec3Sub(e2, v[v_idx2], v[v_indices[i]]);
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
	else if (3 == polygon->v_indices_cnt) {
		float tri[3][3];
		mathVec3Copy(tri[0], polygon->v[polygon->v_indices[0]]);
		mathVec3Copy(tri[1], polygon->v[polygon->v_indices[1]]);
		mathVec3Copy(tri[2], polygon->v[polygon->v_indices[2]]);
		return mathTrianglePointUV((const float(*)[3])tri, p, NULL, NULL);
	}
	else if (polygon->v_indices == DEFAULT_RECT_VERTICE_INDICES) {
		float ls_vec[3], v[3], dot;
		mathVec3Sub(v, p, polygon->v[polygon->v_indices[0]]);
		dot = mathVec3Dot(polygon->normal, v);
		if (dot < -CCT_EPSILON || dot > CCT_EPSILON) {
			return 0;
		}
		mathVec3Sub(ls_vec, polygon->v[polygon->v_indices[1]], polygon->v[polygon->v_indices[0]]);
		dot = mathVec3Dot(ls_vec, v);
		if (dot < -CCT_EPSILON || dot > mathVec3LenSq(ls_vec)) {
			return 0;
		}
		mathVec3Sub(ls_vec, polygon->v[polygon->v_indices[3]], polygon->v[polygon->v_indices[0]]);
		dot = mathVec3Dot(ls_vec, v);
		if (dot < -CCT_EPSILON || dot > mathVec3LenSq(ls_vec)) {
			return 0;
		}
		return 1;
	}
	else if (polygon->tri_indices && polygon->tri_indices_cnt >= 3) {
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
	else {
		/* only convex polygon */
		unsigned int i;
		float v[3], dot;
		float vp[3], eg[3];
		mathVec3Sub(v, polygon->v[polygon->v_indices[0]], p);
		dot = mathVec3Dot(polygon->normal, v);
		if (dot > CCT_EPSILON || dot < -CCT_EPSILON) {
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
	return 0;
}

static GeometryPolygon_t* PolygonCooking_InternalProc(const float (*v)[3], const unsigned int* tri_indices, unsigned int tri_indices_cnt, GeometryPolygon_t* polygon) {
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
			tri_indices[i], tri_indices[i+1],
			tri_indices[i+1], tri_indices[i+2],
			tri_indices[i+2], tri_indices[i]
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
				if (!mathPlaneHasPoint(v[tri_indices[i]], N, v[tri_indices[j+k]])) {
					free(tmp_edge_pair_indices);
					return NULL;
				}
			}
			ej[0] = tri_indices[j]; ej[1] = tri_indices[j+1];
			ej[2] = tri_indices[j+1]; ej[3] = tri_indices[j+2];
			ej[4] = tri_indices[j+2]; ej[5] = tri_indices[j];
			for (k = 0; k < 6; k += 2) {
				unsigned int m;
				if (same[k>>1]) {
					continue;
				}
				for (m = 0; m < 6; m += 2) {
					if (ei[k] == ej[m] || mathVec3Equal(v[ei[k]], v[ej[m]])) {
						same[k>>1] = (ei[k+1] == ej[m+1] || mathVec3Equal(v[ei[k+1]], v[ej[m+1]]));
					}
					else if (ei[k] == ej[m+1] || mathVec3Equal(v[ei[k]], v[ej[m+1]])) {
						same[k>>1] = (ei[k+1] == ej[m] || mathVec3Equal(v[ei[k+1]], v[ej[m]]));
					}
					else {
						continue;
					}
					if (same[k>>1]) {
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
			if (same[j>>1]) {
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
			tmp_edge_pair_indices[tmp_edge_pair_indices_cnt - 1] = ei[j+1];
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
			n = tmp_edge_pair_indices[i+1];
		}
		else if (n == tmp_edge_pair_indices[i+1] || mathVec3Equal(v[n], v[tmp_edge_pair_indices[i+1]])) {
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
		mathVec3Sub(v2, v[tmp_edge_indices[i]], v[tmp_edge_indices[i+1]]);
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

GeometryPolygon_t* mathPolygonCooking(const float (*v)[3], unsigned int v_cnt, const unsigned int* tri_indices, unsigned int tri_indices_cnt, GeometryPolygon_t* polygon) {
	float(*dup_v)[3] = NULL;
	unsigned int i, dup_v_cnt;
	unsigned int* dup_tri_indices = NULL;

	if (v_cnt < 3 || tri_indices_cnt < 3) {
		return NULL;
	}
	dup_v_cnt = mathVerticesDistinctCount(v, v_cnt);
	if (dup_v_cnt < 3) {
		return NULL;
	}
	dup_v = (float(*)[3])malloc(dup_v_cnt * sizeof(dup_v[0]));
	if (!dup_v) {
		goto err;
	}
	dup_tri_indices = (unsigned int*)malloc(sizeof(tri_indices[0]) * tri_indices_cnt);
	if (!dup_tri_indices) {
		goto err;
	}
	for (i = 0; i < tri_indices_cnt; ++i) {
		dup_tri_indices[i] = tri_indices[i];
	}
	mathVerticesMerge(v, v_cnt, dup_v, dup_tri_indices, tri_indices_cnt);
	if (!PolygonCooking_InternalProc(dup_v, dup_tri_indices, tri_indices_cnt, polygon)) {
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

static int Mesh_Cooking_Edge_InternalProc(const float (*v)[3], GeometryMesh_t* mesh) {
	unsigned int i;
	unsigned int* ret_edge_indices = NULL;
	unsigned int ret_edge_indices_cnt = 0;
	const GeometryPolygon_t* polygons = mesh->polygons;
	unsigned int polygons_cnt = mesh->polygons_cnt;

	for (i = 0; i < polygons_cnt; ++i) {
		const GeometryPolygon_t* polygon = polygons + i;
		unsigned int j;
		for (j = 0; j < polygon->v_indices_cnt; ) {
			unsigned int k;
			unsigned int s = polygon->v_indices[j++];
			unsigned int e = polygon->v_indices[j >= polygon->v_indices_cnt ? 0 : j];
			unsigned int* new_p;
			for (k = 0; k < ret_edge_indices_cnt; k += 2) {
				if (s == ret_edge_indices[k] || mathVec3Equal(v[s], v[ret_edge_indices[k]])) {
					if (e == ret_edge_indices[k+1] || mathVec3Equal(v[e], v[ret_edge_indices[k+1]])) {
						break;
					}
				}
				else if (s == ret_edge_indices[k+1] || mathVec3Equal(v[s], v[ret_edge_indices[k+1]])) {
					if ((e == ret_edge_indices[k] || mathVec3Equal(v[e], v[ret_edge_indices[k]]))) {
						break;
					}
				}
			}
			if (k != ret_edge_indices_cnt) {
				continue;
			}
			ret_edge_indices_cnt += 2;
			new_p = (unsigned int*)realloc(ret_edge_indices, ret_edge_indices_cnt * sizeof(ret_edge_indices[0]));
			if (!new_p) {
				free(ret_edge_indices);
				return 0;
			}
			ret_edge_indices = new_p;
			ret_edge_indices[ret_edge_indices_cnt - 2] = s;
			ret_edge_indices[ret_edge_indices_cnt - 1] = e;
		}
	}

	mesh->edge_indices = ret_edge_indices;
	mesh->edge_indices_cnt = ret_edge_indices_cnt;
	return 1;
}

static GeometryPolygon_t* _insert_tri_indices(GeometryPolygon_t* polygon, const unsigned int* tri_indices) {
	unsigned int cnt = polygon->tri_indices_cnt;
	unsigned int* new_p = (unsigned int*)realloc((void*)polygon->tri_indices, sizeof(polygon->tri_indices[0]) * (cnt + 3));
	if (!new_p) {
		return NULL;
	}
	new_p[cnt++] = tri_indices[0];
	new_p[cnt++] = tri_indices[1];
	new_p[cnt++] = tri_indices[2];
	polygon->tri_indices = new_p;
	polygon->tri_indices_cnt = cnt;
	return polygon;
}

static int _polygon_can_merge_triangle(GeometryPolygon_t* polygon, const float p0[3], const float p1[3], const float p2[3]) {
	unsigned int i, n = 0;
	const float* tri_p[] = { p0, p1, p2 };
	for (i = 0; i < 3; ++i) {
		unsigned int j;
		if (!mathPlaneHasPoint(polygon->v[polygon->tri_indices[0]], polygon->normal, tri_p[i])) {
			return 0;
		}
		if (n >= 2) {
			continue;
		}
		for (j = 0; j < polygon->tri_indices_cnt; ++j) {
			const float* pv = polygon->v[polygon->tri_indices[j]];
			if (mathVec3Equal(pv, tri_p[i])) {
				++n;
				break;
			}
		}
	}
	return n >= 2;
}

static int Mesh_Cooking_Polygen_InternalProc(const float (*v)[3], const unsigned int* tri_indices, unsigned int tri_indices_cnt, GeometryMesh_t* mesh) {
	unsigned int i;
	GeometryPolygon_t* tmp_polygons = NULL;
	unsigned int tmp_polygons_cnt = 0;
	unsigned int tri_cnt;
	char* tri_merge_bits = NULL;

	tri_cnt = tri_indices_cnt / 3;
	if (tri_cnt < 1) {
		goto err;
	}
	tri_merge_bits = (char*)calloc(1, tri_cnt / 8 + (tri_cnt % 8 ? 1 : 0));
	if (!tri_merge_bits) {
		goto err;
	}
	/* Merge triangles on the same plane */
	for (i = 0; i < tri_indices_cnt; i += 3) {
		unsigned int j, tri_idx;
		float N[3];
		GeometryPolygon_t* tmp_parr, * new_pg;

		tri_idx = i / 3;
		if (tri_merge_bits[tri_idx / 8] & (1 << (tri_idx % 8))) {
			continue;
		}
		mathPlaneNormalByVertices3(v[tri_indices[i]], v[tri_indices[i + 1]], v[tri_indices[i + 2]], N);
		if (mathVec3IsZero(N)) {
			goto err;
		}
		tmp_parr = (GeometryPolygon_t*)realloc(tmp_polygons, (tmp_polygons_cnt + 1) * sizeof(GeometryPolygon_t));
		if (!tmp_parr) {
			goto err;
		}
		tmp_polygons = tmp_parr;
		new_pg = tmp_polygons + tmp_polygons_cnt;
		tmp_polygons_cnt++;

		new_pg->v_indices = NULL;
		new_pg->v_indices_cnt = 0;
		new_pg->tri_indices = NULL;
		new_pg->tri_indices_cnt = 0;
		if (!_insert_tri_indices(new_pg, tri_indices + i)) {
			goto err;
		}
		new_pg->v = (float(*)[3])v;
		mathVec3Normalized(new_pg->normal, N);

		tri_merge_bits[tri_idx / 8] |= (1 << (tri_idx % 8));
		for (j = 0; j < tri_indices_cnt; j += 3) {
			tri_idx = j / 3;
			if (tri_merge_bits[tri_idx / 8] & (1 << tri_idx % 8)) {
				continue;
			}
			if (!_polygon_can_merge_triangle(new_pg,
				v[tri_indices[j]], v[tri_indices[j + 1]], v[tri_indices[j + 2]]))
			{
				continue;
			}
			if (!_insert_tri_indices(new_pg, tri_indices + j)) {
				goto err;
			}
			tri_merge_bits[tri_idx / 8] |= (1 << (tri_idx % 8));
			j = 0;
		}
	}
	free(tri_merge_bits);
	tri_merge_bits = NULL;
	/* Cooking all polygen */
	for (i = 0; i < tmp_polygons_cnt; ++i) {
		GeometryPolygon_t* polygon = tmp_polygons + i;
		if (!PolygonCooking_InternalProc((const float(*)[3])polygon->v, polygon->tri_indices, polygon->tri_indices_cnt, polygon)) {
			goto err;
		}
	}
	mesh->polygons = tmp_polygons;
	mesh->polygons_cnt = tmp_polygons_cnt;
	return 1;
err:
	if (tmp_polygons) {
		for (i = 0; i < tmp_polygons_cnt; ++i) {
			tmp_polygons[i].v = NULL;
			mathPolygonFreeCookingData(tmp_polygons + i);
		}
		free(tmp_polygons);
	}
	free(tri_merge_bits);
	return 0;
}

GeometryMesh_t* mathMeshCooking(const float (*v)[3], unsigned int v_cnt, const unsigned int* tri_indices, unsigned int tri_indices_cnt, GeometryMesh_t* mesh) {
	float(*dup_v)[3] = NULL;
	unsigned int i, dup_v_cnt;
	unsigned int* dup_v_indices = NULL;
	unsigned int* dup_tri_indices = NULL;

	if (v_cnt < 3 || tri_indices_cnt < 3) {
		goto err_0;
	}
	dup_v_cnt = mathVerticesDistinctCount(v, v_cnt);
	if (dup_v_cnt < 3) {
		goto err_0;
	}
	dup_v = (float(*)[3])malloc(dup_v_cnt * sizeof(dup_v[0]));
	if (!dup_v) {
		goto err_0;
	}
	dup_v_indices = (unsigned int*)malloc(sizeof(dup_v_indices[0]) * dup_v_cnt);
	if (!dup_v_indices) {
		goto err_0;
	}
	dup_tri_indices = (unsigned int*)malloc(sizeof(tri_indices[0]) * tri_indices_cnt);
	if (!dup_tri_indices) {
		goto err_0;
	}
	for (i = 0; i < tri_indices_cnt; ++i) {
		dup_tri_indices[i] = tri_indices[i];
	}
	mathVerticesMerge(v, v_cnt, dup_v, dup_tri_indices, tri_indices_cnt);

	if (!Mesh_Cooking_Polygen_InternalProc(dup_v, dup_tri_indices, tri_indices_cnt, mesh)) {
		goto err_0;
	}
	if (!Mesh_Cooking_Edge_InternalProc(dup_v, mesh)) {
		goto err_1;
	}
	free(dup_tri_indices);
	for (i = 0; i < dup_v_cnt; ++i) {
		dup_v_indices[i] = i;
	}
	mesh->v = dup_v;
	mesh->v_indices = dup_v_indices;
	mesh->v_indices_cnt = dup_v_cnt;
	return mesh;
err_1:
	for (i = 0; i < mesh->polygons_cnt; ++i) {
		mesh->polygons[i].v = NULL;
		mathPolygonFreeCookingData(mesh->polygons + i);
	}
err_0:
	free(dup_v);
	free(dup_v_indices);
	free(dup_tri_indices);
	return NULL;
}

void mathMeshFreeCookingData(GeometryMesh_t* mesh) {
	unsigned int i;
	if (!mesh) {
		return;
	}
	for (i = 0; i < mesh->polygons_cnt; ++i) {
		mesh->polygons[i].v = NULL;
		mathPolygonFreeCookingData(mesh->polygons + i);
	}
	if (mesh->polygons) {
		free(mesh->polygons);
		mesh->polygons_cnt = 0;
	}
	if (mesh->edge_indices) {
		free((void*)mesh->edge_indices);
		mesh->edge_indices_cnt = 0;
	}
	if (mesh->v_indices) {
		free((void*)mesh->v_indices);
		mesh->v_indices_cnt = 0;
	}
	if (mesh->v) {
		free(mesh->v);
		mesh->v = NULL;
	}
}

int mathMeshIsClosed(const GeometryMesh_t* mesh) {
	unsigned int i;
	if (mesh->v_indices_cnt < 3) {
		return 0;
	}
	for (i = 0; i < mesh->v_indices_cnt; ++i) {
		unsigned int j, cnt = 0;
		unsigned int vi = mesh->v_indices[i];
		for (j = 0; j < mesh->polygons_cnt; ++j) {
			unsigned int k;
			const GeometryPolygon_t* polygon = mesh->polygons + j;
			for (k = 0; k < polygon->v_indices_cnt; ++k) {
				unsigned int vk = polygon->v_indices[k];
				if (vi == vk || mathVec3Equal(mesh->v[vi], polygon->v[vk])) {
					++cnt;
					break;
				}
			}
			if (cnt >= 3) {
				break;
			}
		}
		if (cnt < 3) {
			return 0;
		}
	}
	return 1;
}

int mathMeshIsConvex(const GeometryMesh_t* mesh) {
	unsigned int i;
	if (mesh->polygons_cnt < 4) {
		return 0;
	}
	for (i = 0; i < mesh->polygons_cnt; ++i) {
		const GeometryPolygon_t* polygon = mesh->polygons + i;
		const float(*N)[3] = (const float(*)[3])polygon->normal;
		const float(*P)[3] = (const float(*)[3])polygon->v[polygon->v_indices[0]];
		unsigned int j, has_test_dot = 0;
		float test_dot;
		for (j = 0; j < mesh->v_indices_cnt; ++j) {
			float vj[3], dot;
			mathVec3Sub(vj, mesh->v[mesh->v_indices[j]], *P);
			dot = mathVec3Dot(*N, vj);
			if (dot <= CCT_EPSILON && dot >= -CCT_EPSILON) {
				continue;
			}
			if (!has_test_dot) {
				has_test_dot = 1;
				test_dot = dot;
				continue;
			}
			if (test_dot > 0.0f && dot < 0.0f) {
				return 0;
			}
			if (test_dot < 0.0f && dot > 0.0f) {
				return 0;
			}
		}
	}
	return 1;
}

#ifdef __cplusplus
}
#endif
