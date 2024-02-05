//
// Created by hujianzhe
//

#include "../../inc/crt/math_vec3.h"
#include <math.h>

#ifdef	__cplusplus
extern "C" {
#endif

float* mathCoordinateSystemTransform(const float v[3], const float new_origin[3], const float new_axies[3][3], float new_v[3]) {
	float t[3];
	mathVec3Sub(t, v, new_origin);
	new_v[0] = mathVec3Dot(t, new_axies[0]);
	new_v[1] = mathVec3Dot(t, new_axies[1]);
	new_v[2] = mathVec3Dot(t, new_axies[2]);
	return new_v;
}

float* mathVec3Set(float r[3], float x, float y, float z) {
	r[0] = x;
	r[1] = y;
	r[2] = z;
	return r;
}

int mathVec3IsZero(const float v[3]) {
	return	v[0] <= CCT_EPSILON && v[1] <= CCT_EPSILON && v[2] <= CCT_EPSILON &&
			v[0] >= CCT_EPSILON_NEGATE && v[1] >= CCT_EPSILON_NEGATE && v[2] >= CCT_EPSILON_NEGATE;
}

int mathVec3Equal(const float v1[3], const float v2[3]) {
	float delta;

	delta = v1[0] - v2[0];
	if (delta > CCT_EPSILON || delta < CCT_EPSILON_NEGATE) {
		return 0;
	}
	delta = v1[1] - v2[1];
	if (delta > CCT_EPSILON || delta < CCT_EPSILON_NEGATE) {
		return 0;
	}
	delta = v1[2] - v2[2];
	if (delta > CCT_EPSILON || delta < CCT_EPSILON_NEGATE) {
		return 0;
	}
	return 1;
}

float mathVec3MinElement(const float v[3]) {
	if (v[0] < v[1]) {
		return v[0] < v[2] ? v[0] : v[2];
	}
	return v[1] < v[2] ? v[1] : v[2];
}

float mathVec3MaxElement(const float v[3]) {
	if (v[0] > v[1]) {
		return v[0] > v[2] ? v[0] : v[2];
	}
	return v[1] > v[2] ? v[1] : v[2];
}

/* r = v */
float* mathVec3Copy(float r[3], const float v[3]) {
	r[0] = v[0];
	r[1] = v[1];
	r[2] = v[2];
	return r;
}

float mathVec3LenSq(const float v[3]) {
	return v[0] * v[0] + v[1] * v[1] + v[2] * v[2];
}

float mathVec3Len(const float v[3]) {
	return sqrtf(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
}

float mathVec3Normalized(float r[3], const float v[3]) {
	float len_sq = v[0] * v[0] + v[1] * v[1] + v[2] * v[2];
	if (len_sq > 0.0f) {
		float len = sqrtf(len_sq);
		float inv_len = 1.0f / len;
		r[0] = v[0] * inv_len;
		r[1] = v[1] * inv_len;
		r[2] = v[2] * inv_len;
		return len;
	}
	return 0.0f;
}

float mathVec3Direction(const float end[3], const float start[3], float dir[3]) {
	float v[3];
	if (!dir) {
		dir = v;
	}
	mathVec3Sub(dir, end, start);
	if (mathVec3IsZero(dir)) {
		return 0.0f;
	}
	return mathVec3Normalized(dir, dir);
}

/* r = -v */
float* mathVec3Negate(float r[3], const float v[3]) {
	r[0] = -v[0];
	r[1] = -v[1];
	r[2] = -v[2];
	return r;
}

/* r = v1 + v2 */
float* mathVec3Add(float r[3], const float v1[3], const float v2[3]) {
	r[0] = v1[0] + v2[0];
	r[1] = v1[1] + v2[1];
	r[2] = v1[2] + v2[2];
	return r;
}

/* r += v * n */
float* mathVec3AddScalar(float r[3], const float v[3], float n) {
	r[0] += v[0] * n;
	r[1] += v[1] * n;
	r[2] += v[2] * n;
	return r;
}

/* r -= v * n */
float* mathVec3SubScalar(float r[3], const float v[3], float n) {
	r[0] -= v[0] * n;
	r[1] -= v[1] * n;
	r[2] -= v[2] * n;
	return r;
}

/* r = v1 - v2 */
float* mathVec3Sub(float r[3], const float v1[3], const float v2[3]) {
	r[0] = v1[0] - v2[0];
	r[1] = v1[1] - v2[1];
	r[2] = v1[2] - v2[2];
	return r;
}

/* r = v*n */
float* mathVec3MultiplyScalar(float r[3], const float v[3], float n) {
	r[0] = v[0] * n;
	r[1] = v[1] * n;
	r[2] = v[2] * n;
	return r;
}

/* r = v/n */
float* mathVec3DivisionScalar(float r[3], const float v[3], float n) {
	if (n > 1e-6f || n < -1e-6f) {
		float inv = 1.0f / n;
		r[0] = v[0] * inv;
		r[1] = v[1] * inv;
		r[2] = v[2] * inv;
	}
	return r;
}

/* r = v1 * v2, r = |v1|*|v2|*cos@ */
float mathVec3Dot(const float v1[3], const float v2[3]) {
	return v1[0] * v2[0] + v1[1] * v2[1] + v1[2] * v2[2];
}

float mathVec3Radian(const float v1[3], const float v2[3]) {
	return acosf(mathVec3Dot(v1, v2) / sqrtf(mathVec3LenSq(v1) * mathVec3LenSq(v2)));
}

/* r = v1 X v2, r = |v1|*|v2|*sin@ */
float* mathVec3Cross(float r[3], const float v1[3], const float v2[3]) {
	float x = v1[1] * v2[2] - v1[2] * v2[1];
	float y = v1[2] * v2[0] - v1[0] * v2[2];
	float z = v1[0] * v2[1] - v1[1] * v2[0];
	r[0] = x;
	r[1] = y;
	r[2] = z;
	return r;
}

float* mathVec3Reflect(float r[3], const float v[3], const float n[3]) {
	float dot = mathVec3Dot(v, n);
	mathVec3MultiplyScalar(r, n, dot * 2.0f);
	return mathVec3Sub(r, v, r);
}

void mathVec3ComputeBasis(const float dir[3], float right[3], float up[3]) {
	/* these code is copy from PhysX-3.4 */
	if (dir[1] <= 0.9999f && dir[1] >= -0.9999f) {
		mathVec3Set(right, dir[2], 0.0f, -dir[0]);
		mathVec3Normalized(right, right);
		mathVec3Cross(up, dir, right);
	}
	else {
		mathVec3Set(right, 1.0f, 0.0f, 0.0f);
		mathVec3Set(up, 0.0f, dir[2], -dir[1]);
		mathVec3Normalized(up, up);
	}
}

float* mathVec3DelComponent(float r[3], const float v[3], const float dir[3]) {
	float va[3];
	float d = mathVec3Dot(v, dir);
	mathVec3MultiplyScalar(va, dir, d);
	return mathVec3Sub(r, v, va);
}

#ifdef	__cplusplus
}
#endif
