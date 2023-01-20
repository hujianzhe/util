//
// Created by hujianzhe
//

#include "../../inc/crt/math.h"
#include "../../inc/crt/math_vec3.h"
#include "../../inc/crt/math_matrix3.h"
#include <stddef.h>

#ifdef	__cplusplus
extern "C" {
#endif

void mathMat44TransformSplit(const float m[16], float T[3], float S[3], float R[9]) {
	T[0] = m[12];
	T[1] = m[13];
	T[2] = m[14];
	S[0] = mathVec3Len(m + 0);
	S[1] = mathVec3Len(m + 4);
	S[2] = mathVec3Len(m + 8);
	mathVec3MultiplyScalar(R + 0, m + 0, 1.0f / S[0]);
	mathVec3MultiplyScalar(R + 3, m + 3, 1.0f / S[1]);
	mathVec3MultiplyScalar(R + 6, m + 6, 1.0f / S[2]);
}

float* mathMat44Element(const float m[16], unsigned int column, unsigned int row) {
	return (float*)&m[(column << 2) + row];
}

float* mathMat44ToMat33(const float m44[16], float m33[9]) {
	m33[0] = m44[0];
	m33[1] = m44[1];
	m33[2] = m44[2];

	m33[3] = m44[4];
	m33[4] = m44[5];
	m33[5] = m44[6];

	m33[6] = m44[8];
	m33[7] = m44[9];
	m33[8] = m44[10];
	return m33;
}

float* mathMat44Copy(float r[16], const float m[16]) {
	if (r == m) {
		return r;
	}
	r[0] = m[0]; r[1] = m[1]; r[2] = m[2]; r[3] = m[3];
	r[4] = m[4]; r[5] = m[5]; r[6] = m[6]; r[7] = m[7];
	r[8] = m[8]; r[9] = m[9]; r[10] = m[10]; r[11] = m[11];
	r[12] = m[12]; r[13] = m[13]; r[14] = m[14]; r[15] = m[15];
	return r;
}

float* mathMat44Identity(float m[16]) {
	m[0] = 1.0f;
	m[1] = 0.0f;
	m[2] = 0.0f;
	m[3] = 0.0f;

	m[4] = 0.0f;
	m[5] = 1.0f;
	m[6] = 0.0f;
	m[7] = 0.0f;

	m[8] = 0.0f;
	m[9] = 0.0f;
	m[10] = 1.0f;
	m[11] = 0.0f;

	m[12] = 0.0f;
	m[13] = 0.0f;
	m[14] = 0.0f;
	m[15] = 1.0f;

	return m;
}

float* mathMat44RotatePoint(float r[3], const float m[16], const float v[3]) {
	float x = v[0], y = v[1], z = v[2];
	r[0] = m[0]*x + m[4]*y + m[8]*z + m[12];
	r[1] = m[1]*x + m[5]*y + m[9]*z + m[13];
	r[2] = m[2]*x + m[6]*y + m[10]*z + m[14];
	return r;
}

float* mathMat44RotateVector(float r[3], const float m[16], const float v[3]) {
	float x = v[0], y = v[1], z = v[2];
	r[0] = m[0]*x + m[4]*y + m[8]*z;
	r[1] = m[1]*x + m[5]*y + m[9]*z;
	r[2] = m[2]*x + m[6]*y + m[10]*z;
	return r;
}

#ifdef	__cplusplus
}
#endif
