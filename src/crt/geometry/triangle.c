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

void mathTriangleToPolygen(const float tri[3][3], GeometryPolygen_t* polygen) {
	polygen->v_indices = DEFAULT_TRIANGLE_VERTICE_INDICES;
	polygen->v_indices_cnt = 3;
	polygen->v = tri;
	mathPlaneNormalByVertices3(tri[0], tri[1], tri[2], polygen->normal);
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
	polygen->v_indices = DEFAULT_RECT_VERTICE_INDICES;
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
	else if (polygen->v_indices == DEFAULT_RECT_VERTICE_INDICES) {
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
		mathVec3Sub(v, polygen->v[polygen->tri_indices[0]], p);
		dot = mathVec3Dot(polygen->normal, v);
		if (dot < -CCT_EPSILON || dot > CCT_EPSILON) {
			return 0;
		}
		for (i = 0; i < polygen->tri_indices_cnt; ) {
			float tri[3][3];
			mathVec3Copy(tri[0], polygen->v[polygen->tri_indices[i++]]);
			mathVec3Copy(tri[1], polygen->v[polygen->tri_indices[i++]]);
			mathVec3Copy(tri[2], polygen->v[polygen->tri_indices[i++]]);
			if (mathTrianglePointUV((const float(*)[3])tri, p, NULL, NULL)) {
				return 1;
			}
		}
	}
	return 0;
}

static int mathPolygenCooking(const float (*v)[3], const unsigned int* tri_indices, unsigned int tri_indices_cnt, unsigned int** v_indices, unsigned int* v_indices_cnt) {
	unsigned int i, s, n, p, last_s, first_s;
	unsigned int* tmp_edge_pair_indices = NULL;
	unsigned int tmp_edge_pair_indices_cnt = 0;
	unsigned int* tmp_edge_indices = NULL;
	unsigned int tmp_edge_indices_cnt = 0;
	unsigned int* ret_v_indices = NULL;
	unsigned int ret_v_indices_cnt = 0;
	float v1[3], v2[3], N[3];

	if (tri_indices_cnt < 3 || tri_indices_cnt % 3 != 0) {
		return 0;
	}
	mathPlaneNormalByVertices3(v[tri_indices[0]], v[tri_indices[1]], v[tri_indices[2]], N);
	if (mathVec3IsZero(N)) {
		return 0;
	}
	if (tri_indices_cnt == 3) {
		ret_v_indices = (unsigned int*)malloc(3 * sizeof(ret_v_indices[0]));
		if (!ret_v_indices) {
			return 0;
		}
		ret_v_indices[0] = tri_indices[0];
		ret_v_indices[1] = tri_indices[1];
		ret_v_indices[2] = tri_indices[2];

		*v_indices = ret_v_indices;
		*v_indices_cnt = 3;
		return 1;
	}
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
					return 0;
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
			return 0;
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
				return 0;
			}
			tmp_edge_pair_indices = ptr;
			tmp_edge_pair_indices[tmp_edge_pair_indices_cnt - 2] = ei[j];
			tmp_edge_pair_indices[tmp_edge_pair_indices_cnt - 1] = ei[j+1];
		}
	}
	if (!tmp_edge_pair_indices) {
		return 0;
	}
	tmp_edge_indices = (unsigned int*)malloc(sizeof(tmp_edge_indices[0]) * tmp_edge_pair_indices_cnt);
	if (!tmp_edge_indices) {
		free(tmp_edge_pair_indices);
		return 0;
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
		return 0;
	}
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
		return 0;
	}
	n = 0;
	for (i = 0; i < tmp_edge_indices_cnt && n < ret_v_indices_cnt; ++i) {
		if (-1 == tmp_edge_indices[i]) {
			continue;
		}
		ret_v_indices[n++] = tmp_edge_indices[i];
	}
	free(tmp_edge_indices);

	*v_indices = ret_v_indices;
	*v_indices_cnt = ret_v_indices_cnt;
	return 1;
}

static GeometryPolygen_t* _insert_tri_indices(GeometryPolygen_t* polygen, const unsigned int* tri_indices) {
	unsigned int cnt = polygen->tri_indices_cnt;
	unsigned int* new_p = (unsigned int*)realloc((void*)polygen->tri_indices, sizeof(polygen->tri_indices[0]) * (cnt + 3));
	if (!new_p) {
		return NULL;
	}
	new_p[cnt++] = tri_indices[0];
	new_p[cnt++] = tri_indices[1];
	new_p[cnt++] = tri_indices[2];
	polygen->tri_indices = new_p;
	polygen->tri_indices_cnt = cnt;
	return polygen;
}

int mathTriangleMeshCooking(const float (*v)[3], const unsigned int* tri_indices, unsigned int tri_indices_cnt, GeometryPolygen_t** polygens, unsigned int* polygen_cnt) {
	unsigned int i, pi;
	GeometryPolygen_t** tmp_polygens = NULL;
	unsigned int tmp_polygens_arrcnt = 0;
	GeometryPolygen_t* tmp_ret_polygens = NULL;
	GeometryPolygen_t** tmp_ret_polygens_unique = NULL;
	unsigned int tmp_ret_polygens_cnt = 0;

	if (tri_indices_cnt < 3 || tri_indices_cnt % 3 != 0) {
		return 0;
	}
	tmp_polygens_arrcnt = tri_indices_cnt / 3;
	tmp_polygens = (GeometryPolygen_t**)calloc(1, tmp_polygens_arrcnt * sizeof(tmp_polygens[0]));
	if (!tmp_polygens) {
		return 0;
	}
	for (pi = i = 0; i < tri_indices_cnt; i += 3, pi++) {
		unsigned int j, pj;
		float N[3];
		mathPlaneNormalByVertices3(v[tri_indices[i]], v[tri_indices[i+1]], v[tri_indices[i+2]], N);
		if (mathVec3IsZero(N)) {
			goto err;
		}
		if (!tmp_polygens[pi]) {
			GeometryPolygen_t *new_p, **new_parr;
			new_p = (GeometryPolygen_t*)calloc(1, sizeof(GeometryPolygen_t));
			if (!new_p) {
				goto err;
			}
			tmp_polygens[pi] = new_p;
			new_p->v = v;
			mathVec3Normalized(new_p->normal, N);

			new_parr = (GeometryPolygen_t**)realloc(tmp_ret_polygens_unique, (tmp_ret_polygens_cnt + 1) * sizeof(tmp_ret_polygens_unique[0]));
			if (!new_parr) {
				goto err;
			}
			tmp_ret_polygens_unique = new_parr;
			tmp_ret_polygens_unique[tmp_ret_polygens_cnt++] = tmp_polygens[pi];
		}
		if (!_insert_tri_indices(tmp_polygens[pi], tri_indices + i)) {
			goto err;
		}
		for (pj = j = 0; j < tri_indices_cnt; j += 3, pj++) {
			unsigned int k, same_cnt;
			if (i == j) {
				continue;
			}
			same_cnt = 0;
			for (k = 0; k < 3; ++k) {
				if (!mathPlaneHasPoint(v[tri_indices[i]], N, v[tri_indices[j+k]])) {
					break;
				}
				if (mathVec3Equal(v[tri_indices[i]], v[tri_indices[j+k]]) ||
					mathVec3Equal(v[tri_indices[i+1]], v[tri_indices[j+k]]) ||
					mathVec3Equal(v[tri_indices[i+2]], v[tri_indices[j+k]]))
				{
					same_cnt++;
				}
			}
			if (k < 3) {
				continue;
			}
			if (3 == same_cnt) {
				goto err;
			}
			if (2 != same_cnt) {
				continue;
			}
			tmp_polygens[pj] = tmp_polygens[pi];
		}
	}
	free(tmp_polygens);
	tmp_polygens = NULL;

	tmp_ret_polygens = (GeometryPolygen_t*)malloc(tmp_ret_polygens_cnt * sizeof(tmp_ret_polygens[0]));
	if (!tmp_ret_polygens) {
		goto err;
	}
	for (i = 0; i < tmp_ret_polygens_cnt; ++i) {
		GeometryPolygen_t* polygen = tmp_ret_polygens_unique[i];
		if (!mathPolygenCooking(polygen->v, polygen->tri_indices, polygen->tri_indices_cnt, (unsigned int**)&polygen->v_indices, &polygen->v_indices_cnt)) {
			goto err;
		}
		tmp_ret_polygens[i] = *polygen;
		free(polygen);
	}
	free(tmp_ret_polygens_unique);

	*polygens = tmp_ret_polygens;
	*polygen_cnt = tmp_ret_polygens_cnt;
	return 1;
err:
	if (tmp_polygens) {
		for (i = 0; i < tmp_polygens_arrcnt; ++i) {
			if (!tmp_polygens[i]) {
				continue;
			}
			mathTriangleMeshFreePolygenData(tmp_polygens[i]);
		}
		free(tmp_polygens);
	}
	free(tmp_ret_polygens_unique);
	free(tmp_ret_polygens);
	return 0;
}

void mathTriangleMeshFreePolygenData(GeometryPolygen_t* polygen) {
	free((void*)polygen->tri_indices);
	polygen->tri_indices = NULL;
	polygen->tri_indices_cnt = 0;

	free((void*)polygen->v_indices);
	polygen->v_indices = NULL;
	polygen->v_indices_cnt = 0;
}

#ifdef __cplusplus
}
#endif
