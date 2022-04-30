//
// Created by hujianzhe
//

#include "../../../inc/crt/math.h"
#include "../../../inc/crt/math_vec3.h"
#include "../../../inc/crt/geometry/line_segment.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

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
		if (min_d) {
			dot = mathVec3Dot(v, lsdir1);
			*min_d = sqrtf(mathVec3LenSq(v) - dot * dot);
		}
		return 0;
	}
	nlen = mathVec3Normalized(N, n);
	dot = mathVec3Dot(v, N);
	if (dot < -CCT_EPSILON) {
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
	return 1;
}

int mathLineIntersectLine(const float ls1v[3], const float ls1dir[3], const float ls2v[3], const float ls2dir[3], float distance[2]) {
	float N[3], v[3];
	mathVec3Sub(v, ls1v, ls2v);
	mathVec3Cross(N, ls1dir, ls2dir);
	if (mathVec3IsZero(N)) {
		float dot = mathVec3Dot(v, ls2dir);
		return fcmpf(dot * dot, mathVec3LenSq(v), CCT_EPSILON) ? 0 : 2;
	}
	else if (mathVec3IsZero(v)) {
		distance[0] = distance[1] = 0.0f;
		return 1;
	}
	else {
		float dot = mathVec3Dot(v, N);
		if (fcmpf(dot, 0.0f, CCT_EPSILON)) {
			return 0;
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
		return 1;
	}
}

int mathSegmentHasPoint(const float ls[2][3], const float p[3]) {
	float pv1[3], pv2[3], N[3];
	mathVec3Sub(pv1, ls[0], p);
	mathVec3Sub(pv2, ls[1], p);
	mathVec3Cross(N, pv1, pv2);
	if (!mathVec3IsZero(N)) {
		return 0;
	}
	else if (mathVec3Equal(ls[0], p)) {
		return 1;
	}
	else if (mathVec3Equal(ls[1], p)) {
		return 1;
	}
	else {
		float dot = mathVec3Dot(pv1, pv2);
		if (fcmpf(dot, 0.0f, CCT_EPSILON) > 0) {
			return 0;
		}
		return 1;
	}
}

void mathSegmentClosetPointTo(const float ls[2][3], const float p[3], float closet_p[3]) {
	float lsdir[3], lslen, dot;
	mathVec3Sub(lsdir, ls[1], ls[0]);
	lslen = mathVec3Normalized(lsdir, lsdir);
	dot = mathPointProjectionLine(p, ls[0], lsdir, closet_p);
	if (dot < -CCT_EPSILON) {
		mathVec3Copy(closet_p, ls[0]);
	}
	else if (dot > lslen + CCT_EPSILON) {
		mathVec3Copy(closet_p, ls[1]);
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
		if (fcmpf(dot, 0.0f, CCT_EPSILON) > 0) {
			return 0;
		}
	}
	return 1;
}

int mathSegmentIntersectSegment(const float ls1[2][3], const float ls2[2][3], float p[3]) {
	int res;
	float lsdir1[3], lsdir2[3], d[2], lslen1, lslen2;
	mathVec3Sub(lsdir1, ls1[1], ls1[0]);
	mathVec3Sub(lsdir2, ls2[1], ls2[0]);
	lslen1 = mathVec3Normalized(lsdir1, lsdir1);
	lslen2 = mathVec3Normalized(lsdir2, lsdir2);
	res = mathLineIntersectLine(ls1[0], lsdir1, ls2[0], lsdir2, d);
	if (0 == res) {
		return 0;
	}
	else if (1 == res) {
		if (d[0] < -CCT_EPSILON || d[1] < -CCT_EPSILON) {
			return 0;
		}
		if (d[0] > lslen1 + CCT_EPSILON || d[1] > lslen2 + CCT_EPSILON) {
			return 0;
		}
		if (p) {
			mathVec3AddScalar(mathVec3Copy(p, ls1[0]), lsdir1, d[0]);
		}
		return 1;
	}
	else {
		float dot;
		if (mathVec3Equal(ls1[0], ls2[0])) {
			dot = mathVec3Dot(lsdir2, lsdir1);
			if (dot <= CCT_EPSILON) {
				if (p) {
					mathVec3Copy(p, ls1[0]);
				}
				return 1;
			}
			return 2;
		}
		else if (mathVec3Equal(ls1[0], ls2[1])) {
			dot = mathVec3Dot(lsdir2, lsdir1);
			if (dot >= CCT_EPSILON) {
				if (p) {
					mathVec3Copy(p, ls1[0]);
				}
				return 1;
			}
			return 2;
		}
		else if (mathVec3Equal(ls1[1], ls2[0])) {
			dot = mathVec3Dot(lsdir2, lsdir1);
			if (dot >= CCT_EPSILON) {
				if (p) {
					mathVec3Copy(p, ls1[1]);
				}
				return 1;
			}
			return 2;
		}
		else if (mathVec3Equal(ls1[1], ls2[1])) {
			dot = mathVec3Dot(lsdir2, lsdir1);
			if (dot <= CCT_EPSILON) {
				if (p) {
					mathVec3Copy(p, ls1[1]);
				}
				return 1;
			}
			return 2;
		}
		else {
			float v1[3], v2[3];
			int i;
			for (i = 0; i < 2; ++i) {
				mathVec3Sub(v1, ls1[0], ls2[i]);
				mathVec3Sub(v2, ls1[1], ls2[i]);
				dot = mathVec3Dot(v1, v2);
				if (dot < -CCT_EPSILON) {
					return 2;
				}
			}
			for (i = 0; i < 2; ++i) {
				mathVec3Sub(v1, ls2[0], ls1[i]);
				mathVec3Sub(v2, ls2[1], ls1[i]);
				dot = mathVec3Dot(v1, v2);
				if (dot < -CCT_EPSILON) {
					return 2;
				}
			}
			return 0;
		}
	}
}

#ifdef __cplusplus
}
#endif
