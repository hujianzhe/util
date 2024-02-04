//
// Created by hujianzhe
//

#include "../../../inc/crt/math_vec3.h"
#include "../../../inc/crt/geometry/line_segment.h"
#include <math.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

int mathProjectionRay(const float o[3], const float projection_p[3], const float dir[3], float* dir_d, float projection_v[3]) {
	float v[3];
	mathVec3Sub(v, projection_p, o);
	if (mathVec3IsZero(v)) {
		*dir_d = 0.0f;
		if (projection_v) {
			mathVec3Set(projection_v, 0.0f, 0.0f, 0.0f);
		}
		return 1;
	}
	if (dir) {
		float dot = mathVec3Dot(v, dir);
		if (dot <= CCT_EPSILON) {
			*dir_d = 0.0f;
			return 0;
		}
		*dir_d = mathVec3Normalized(v, v);
		dot = mathVec3Dot(v, dir);
		*dir_d /= dot;
	}
	else {
		*dir_d = mathVec3Normalized(v, v);
	}
	if (projection_v) {
		mathVec3Copy(projection_v, v);
	}
	return 1;
}

float mathPointProjectionLine(const float p[3], const float ls_v[3], const float lsdir[3], float np[3]) {
	float vp[3], dot;
	mathVec3Sub(vp, p, ls_v);
	dot = mathVec3Dot(vp, lsdir);
	mathVec3AddScalar(mathVec3Copy(np, ls_v), lsdir, dot);
	return dot;
}

int mathLineClosestLine(const float lsv1[3], const float lsdir1[3], const float lsv2[3], const float lsdir2[3], float* min_d, float dir_d[2]) {
	float N[3], n[3], v[3], dot, nlen;
	mathVec3Sub(v, lsv2, lsv1);
	mathVec3Cross(n, lsdir1, lsdir2);
	if (mathVec3IsZero(n)) {
		dot = mathVec3Dot(v, lsdir1);
		dot *= dot;
		nlen = mathVec3LenSq(v);
		if (min_d) {
			*min_d = sqrtf(nlen - dot);
		}
		if (nlen > dot + CCT_EPSILON || nlen < dot - CCT_EPSILON) {
			return GEOMETRY_LINE_PARALLEL;
		}
		return GEOMETRY_LINE_OVERLAP;
	}
	nlen = mathVec3Normalized(N, n);
	dot = mathVec3Dot(v, N);
	if (dot < 0.0f) {
		dot = -dot;
	}
	if (min_d) {
		*min_d = dot;
	}
	if (dir_d) {
		float cross_v[3], nlensq_inv = 1.0f / (nlen * nlen);
		dir_d[0] = mathVec3Dot(mathVec3Cross(cross_v, v, lsdir2), n) * nlensq_inv;
		dir_d[1] = mathVec3Dot(mathVec3Cross(cross_v, v, lsdir1), n) * nlensq_inv;
	}
	return dot > CCT_EPSILON ? GEOMETRY_LINE_SKEW : GEOMETRY_LINE_CROSS;
}

int mathLineIntersectLine(const float ls1v[3], const float ls1dir[3], const float ls2v[3], const float ls2dir[3], float distance[2]) {
	float N[3], v[3];
	mathVec3Sub(v, ls1v, ls2v);
	mathVec3Cross(N, ls1dir, ls2dir);
	if (mathVec3IsZero(N)) {
		float dot = mathVec3Dot(v, ls2dir);
		float dot_sq = dot * dot;
		float v_lensq = mathVec3LenSq(v);
		if (dot_sq > v_lensq + CCT_EPSILON || dot_sq < v_lensq - CCT_EPSILON) {
			return GEOMETRY_LINE_PARALLEL;
		}
		return GEOMETRY_LINE_OVERLAP;
	}
	else if (mathVec3IsZero(v)) {
		distance[0] = distance[1] = 0.0f;
		return GEOMETRY_LINE_CROSS;
	}
	else {
		float dot = mathVec3Dot(v, N);
		if (dot < CCT_EPSILON_NEGATE || dot > CCT_EPSILON) {
			return GEOMETRY_LINE_SKEW;
		}
		dot = mathVec3Dot(v, ls2dir);
		mathVec3AddScalar(mathVec3Copy(v, ls2v), ls2dir, dot);
		mathVec3Sub(v, v, ls1v);
		if (mathVec3IsZero(v)) {
			distance[0] = 0.0f;
		}
		else {
			distance[0] = mathVec3Normalized(v, v);
			distance[0] /= mathVec3Dot(v, ls1dir);
		}
		mathVec3Sub(v, ls2v, ls1v);
		dot = mathVec3Dot(v, ls1dir);
		mathVec3AddScalar(mathVec3Copy(v, ls1v), ls1dir, dot);
		mathVec3Sub(v, v, ls2v);
		if (mathVec3IsZero(v)) {
			distance[1] = 0.0f;
		}
		else {
			distance[1] = mathVec3Normalized(v, v);
			distance[1] /= mathVec3Dot(v, ls2dir);
		}
		return GEOMETRY_LINE_CROSS;
	}
}

static int mathSegmentIntersectSegmentWhenInSameLine(const float ls1[2][3], const float ls2[2][3], float p[3]) {
	float dot, lsdir1[3], lsdir2[3];
	mathVec3Sub(lsdir1, ls1[1], ls1[0]);
	mathVec3Sub(lsdir2, ls2[1], ls2[0]);
	if (mathVec3Equal(ls1[0], ls2[0])) {
		dot = mathVec3Dot(lsdir2, lsdir1);
		if (dot <= CCT_EPSILON) {
			if (p) {
				mathVec3Copy(p, ls1[0]);
			}
			return GEOMETRY_SEGMENT_CONTACT;
		}
		return GEOMETRY_SEGMENT_OVERLAP;
	}
	else if (mathVec3Equal(ls1[0], ls2[1])) {
		dot = mathVec3Dot(lsdir2, lsdir1);
		if (dot >= CCT_EPSILON) {
			if (p) {
				mathVec3Copy(p, ls1[0]);
			}
			return GEOMETRY_SEGMENT_CONTACT;
		}
		return GEOMETRY_SEGMENT_OVERLAP;
	}
	else if (mathVec3Equal(ls1[1], ls2[0])) {
		dot = mathVec3Dot(lsdir2, lsdir1);
		if (dot >= CCT_EPSILON) {
			if (p) {
				mathVec3Copy(p, ls1[1]);
			}
			return GEOMETRY_SEGMENT_CONTACT;
		}
		return GEOMETRY_SEGMENT_OVERLAP;
	}
	else if (mathVec3Equal(ls1[1], ls2[1])) {
		dot = mathVec3Dot(lsdir2, lsdir1);
		if (dot <= CCT_EPSILON) {
			if (p) {
				mathVec3Copy(p, ls1[1]);
			}
			return GEOMETRY_SEGMENT_CONTACT;
		}
		return GEOMETRY_SEGMENT_OVERLAP;
	}
	else {
		float v1[3], v2[3];
		int i;
		for (i = 0; i < 2; ++i) {
			mathVec3Sub(v1, ls1[0], ls2[i]);
			mathVec3Sub(v2, ls1[1], ls2[i]);
			dot = mathVec3Dot(v1, v2);
			if (dot < CCT_EPSILON_NEGATE) {
				return GEOMETRY_SEGMENT_OVERLAP;
			}
		}
		for (i = 0; i < 2; ++i) {
			mathVec3Sub(v1, ls2[0], ls1[i]);
			mathVec3Sub(v2, ls2[1], ls1[i]);
			dot = mathVec3Dot(v1, v2);
			if (dot < CCT_EPSILON_NEGATE) {
				return GEOMETRY_SEGMENT_OVERLAP;
			}
		}
		return 0;
	}
}

void mathSegmentClosestSegmentVertice(const float ls1[2][3], const float ls2[2][3], float closest_p[2][3]) {
	int i;
	for (i = 0; i < 2; ++i) {
		int j;
		for (j = 0; j < 2; ++j) {
			float v[3];
			mathVec3Sub(v, ls2[j], ls1[i]);
			if (i || j) {
				float dq = mathVec3LenSq(v);
				mathVec3Sub(v, closest_p[1], closest_p[0]);
				if (dq >= mathVec3LenSq(v)) {
					continue;
				}
			}
			mathVec3Copy(closest_p[0], ls1[i]);
			mathVec3Copy(closest_p[1], ls2[j]);
		}
	}
}

int mathSegmentClosestSegment(const float ls1[2][3], const float ls2[2][3], float closest_p[2][3]) {
	float dir1[3], dir2[3], lslen1, lslen2, dir_d[2];
	int res, i, has_p;
	mathVec3Sub(dir1, ls1[1], ls1[0]);
	mathVec3Sub(dir2, ls2[1], ls2[0]);
	lslen1 = mathVec3Normalized(dir1, dir1);
	lslen2 = mathVec3Normalized(dir2, dir2);
	res = mathLineClosestLine(ls1[0], dir1, ls2[0], dir2, NULL, dir_d);
	if (GEOMETRY_LINE_PARALLEL == res) {
		for (i = 0; i < 2; ++i) {
			float v[3], dot;
			mathVec3Sub(v, ls1[i], ls2[0]);
			dot = mathVec3Dot(v, dir2);
			if (dot >= CCT_EPSILON_NEGATE && dot <= lslen2 + CCT_EPSILON) {
				mathVec3Copy(closest_p[0], ls1[i]);
				mathVec3Copy(closest_p[1], ls2[0]);
				mathVec3AddScalar(closest_p[1], dir2, dot);
				return 0;
			}
		}
	}
	else if (GEOMETRY_LINE_CROSS == res || GEOMETRY_LINE_SKEW == res) {
		if (dir_d[0] >= CCT_EPSILON && dir_d[0] <= lslen1 + CCT_EPSILON &&
			dir_d[1] >= CCT_EPSILON && dir_d[1] <= lslen2 + CCT_EPSILON)
		{
			mathVec3Copy(closest_p[0], ls1[0]);
			mathVec3AddScalar(closest_p[0], dir1, dir_d[0]);
			mathVec3Copy(closest_p[1], ls2[0]);
			mathVec3AddScalar(closest_p[1], dir2, dir_d[1]);
			return GEOMETRY_LINE_CROSS == res ? GEOMETRY_SEGMENT_CONTACT : 0;
		}
		has_p = 0;
		for (i = 0; i < 2; ++i) {
			float v[3], dot;
			mathVec3Sub(v, ls1[i], ls2[0]);
			dot = mathVec3Dot(v, dir2);
			if (dot < CCT_EPSILON_NEGATE || dot > lslen2 + CCT_EPSILON) {
				continue;
			}
			if (has_p) {
				float dq = mathVec3LenSq(v) - dot * dot;
				mathVec3Sub(v, closest_p[1], closest_p[0]);
				if (dq >= mathVec3LenSq(v)) {
					continue;
				}
			}
			has_p = 1;
			mathVec3Copy(closest_p[0], ls1[i]);
			mathVec3Copy(closest_p[1], ls2[0]);
			mathVec3AddScalar(closest_p[1], dir2, dot);
		}
		for (i = 0; i < 2; ++i) {
			float v[3], dot;
			mathVec3Sub(v, ls2[i], ls1[0]);
			dot = mathVec3Dot(v, dir1);
			if (dot < CCT_EPSILON_NEGATE || dot > lslen1 + CCT_EPSILON) {
				continue;
			}
			if (has_p) {
				float dq = mathVec3LenSq(v) - dot * dot;
				mathVec3Sub(v, closest_p[1], closest_p[0]);
				if (dq >= mathVec3LenSq(v)) {
					continue;
				}
			}
			has_p = 1;
			mathVec3Copy(closest_p[1], ls2[i]);
			mathVec3Copy(closest_p[0], ls1[0]);
			mathVec3AddScalar(closest_p[0], dir1, dot);
		}
		if (has_p) {
			return 0;
		}
	}
	else {
		float p[3];
		res = mathSegmentIntersectSegmentWhenInSameLine(ls1, ls2, p);
		if (GEOMETRY_SEGMENT_CONTACT == res) {
			mathVec3Copy(closest_p[0], p);
			mathVec3Copy(closest_p[1], p);
			return res;
		}
		if (GEOMETRY_SEGMENT_OVERLAP == res) {
			return res;
		}
	}
	mathSegmentClosestSegmentVertice(ls1, ls2, closest_p);
	return 0;
}

int mathSegmentHasPoint(const float ls[2][3], const float p[3]) {
	float pv1[3], pv2[3], N[3], dot;
	mathVec3Sub(pv1, ls[0], p);
	mathVec3Sub(pv2, ls[1], p);
	mathVec3Cross(N, pv1, pv2);
	if (!mathVec3IsZero(N)) {
		return 0;
	}
	dot = mathVec3Dot(pv1, pv2);
	return dot <= CCT_EPSILON;
}

int mathSegmentIsSame(const float ls1[2][3], const float ls2[2][3]) {
	if (mathVec3Equal(ls1[0], ls2[0])) {
		return mathVec3Equal(ls1[1], ls2[1]);
	}
	if (mathVec3Equal(ls1[0], ls2[1])) {
		return mathVec3Equal(ls1[1], ls2[0]);
	}
	return 0;
}

void mathSegmentClosestPointTo(const float ls[2][3], const float p[3], float closest_p[3]) {
	float lsdir[3], lslen, dot;
	mathVec3Sub(lsdir, ls[1], ls[0]);
	lslen = mathVec3Normalized(lsdir, lsdir);
	dot = mathPointProjectionLine(p, ls[0], lsdir, closest_p);
	if (dot < CCT_EPSILON_NEGATE) {
		mathVec3Copy(closest_p, ls[0]);
	}
	else if (dot > lslen + CCT_EPSILON) {
		mathVec3Copy(closest_p, ls[1]);
	}
}

int mathSegmentContainSegment(const float ls1[2][3], const float ls2[2][3]) {
	int i;
	float v1[3], v2[3], N[3];
	mathVec3Sub(v1, ls1[1], ls1[0]);
	mathVec3Sub(v2, ls2[1], ls2[0]);
	mathVec3Cross(N, v1, v2);
	if (!mathVec3IsZero(N)) {
		return 0;
	}
	for (i = 0; i < 2; ++i) {
		float dot;
		if (mathVec3Equal(ls1[0], ls2[i]) || mathVec3Equal(ls1[1], ls2[i])) {
			continue;
		}
		mathVec3Sub(v1, ls1[0], ls2[i]);
		mathVec3Sub(v2, ls1[1], ls2[i]);
		dot = mathVec3Dot(v1, v2);
		if (dot > CCT_EPSILON) {
			return 0;
		}
	}
	return 1;
}

int mathSegmentIntersectSegment(const float ls1[2][3], const float ls2[2][3], float p[3], int* line_mask) {
	int res;
	float lsdir1[3], lsdir2[3], d[2], lslen1, lslen2;
	mathVec3Sub(lsdir1, ls1[1], ls1[0]);
	mathVec3Sub(lsdir2, ls2[1], ls2[0]);
	lslen1 = mathVec3Normalized(lsdir1, lsdir1);
	lslen2 = mathVec3Normalized(lsdir2, lsdir2);
	res = mathLineIntersectLine(ls1[0], lsdir1, ls2[0], lsdir2, d);
	if (line_mask) {
		*line_mask = res;
	}
	if (GEOMETRY_LINE_PARALLEL == res || GEOMETRY_LINE_SKEW == res) {
		return 0;
	}
	else if (GEOMETRY_LINE_CROSS == res) {
		if (d[0] < CCT_EPSILON_NEGATE || d[1] < CCT_EPSILON_NEGATE ||
			d[0] > lslen1 + CCT_EPSILON || d[1] > lslen2 + CCT_EPSILON)
		{
			return 0;
		}
		if (p) {
			mathVec3AddScalar(mathVec3Copy(p, ls1[0]), lsdir1, d[0]);
		}
		return GEOMETRY_SEGMENT_CONTACT;
	}
	else {
		return mathSegmentIntersectSegmentWhenInSameLine(ls1, ls2, p);
	}
}

#ifdef __cplusplus
}
#endif
