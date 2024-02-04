//
// Created by hujianzhe
//

#include "../../inc/crt/math_vec3.h"
#include "../../inc/crt/math_matrix3.h"

#ifdef	__cplusplus
extern "C" {
#endif

void mathMat44TransformSplit(const float m[16], float T[3], float S[3], float R[9]) {
	if (T) {
		T[0] = m[12];
		T[1] = m[13];
		T[2] = m[14];
	}
	if (R) {
		float s[3];
		if (!S) {
			S = s;
		}
		S[0] = mathVec3Len(m + 0);
		S[1] = mathVec3Len(m + 4);
		S[2] = mathVec3Len(m + 8);
		mathVec3MultiplyScalar(R + 0, m + 0, 1.0f / S[0]);
		mathVec3MultiplyScalar(R + 3, m + 4, 1.0f / S[1]);
		mathVec3MultiplyScalar(R + 6, m + 8, 1.0f / S[2]);
	}
	else if (S) {
		S[0] = mathVec3Len(m + 0);
		S[1] = mathVec3Len(m + 4);
		S[2] = mathVec3Len(m + 8);
	}
}

float* mathMat44SetPositionPart(float m[16], const float p[3]) {
	m[12] = p[0];
	m[13] = p[1];
	m[14] = p[2];
	return m;
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

float* mathMat44Add(float r[16], const float m1[16], const float m2[16]) {
	int i;
	for (i = 0; i < 16; ++i) {
		r[i] = m1[i] + m2[i];
	}
	return r;
}

float* mathMat44MultiplyScalar(float r[16], const float m[16], float n) {
	int i;
	for (i = 0; i < 16; ++i) {
		r[i] = m[i] * n;
	}
	return r;
}

/* r = m1*m2  */
float* mathMat44MulMat44(float r[16], const float m1[16], const float m2[16]) {
	int i, j;
	for (i = 0; i < 4; ++i) {
		for (j = 0; j < 16; j += 4) {
			r[j+i] = m1[i]*m2[j] + m1[i+4]*m2[1+j] + m1[i+8]*m2[2+j] + m1[i+12]*m2[3+j];
		}
	}
	return r;
}

float* mathMat44Transpose(float r[16], const float m[16]) {
	float t;

	t = m[1];
	r[1] = m[4];
	r[4] = t;

	t = m[2];
	r[2] = m[8];
	r[8] = t;

	t = m[3];
	r[3] = m[12];
	r[12] = t;

	t = m[6];
	r[6] = m[9];
	r[9] = t;

	t = m[7];
	r[7] = m[13];
	r[13] = t;

	t = m[11];
	r[11] = m[14];
	r[14] = t;

	if (r != m) {
		r[0] = m[0];
		r[5] = m[5];
		r[10] = m[10];
		r[15] = m[15];
	}

	return r;
}

float* mathMat44Inverse(float r[16], const float m[16]) {
	float t;

	r[3] = -m[0]*m[12] - m[1]*m[13] - m[2]*m[14];
	r[7] = -m[4]*m[12] - m[5]*m[13] - m[6]*m[14];
	r[11] = -m[8]*m[12] - m[9]*m[13] - m[10]*m[14];

	r[12] = r[3];
	r[13] = r[7];
	r[14] = r[11];
	r[3] = r[7] = r[11] = 0.0f;
	r[15] = 1.0f;

	t = m[1];
	r[1] = m[4];
	r[4] = t;

	t = m[2];
	r[2] = m[8];
	r[8] = t;

	t = m[6];
	r[6] = m[9];
	r[9] = t;

	if (r != m) {
		r[0] = m[0];
		r[5] = m[5];
		r[10] = m[10];
	}

	return r;
}

float* mathMat44TransformVec3(float r[3], const float m[16], const float v[3]) {
	float x = v[0], y = v[1], z = v[2];
	r[0] = m[0]*x + m[4]*y + m[8]*z + m[12];
	r[1] = m[1]*x + m[5]*y + m[9]*z + m[13];
	r[2] = m[2]*x + m[6]*y + m[10]*z + m[14];
	return r;
}

float* mathMat44RotateVec3(float r[3], const float m[16], const float v[3]) {
	float x = v[0], y = v[1], z = v[2];
	r[0] = m[0]*x + m[4]*y + m[8]*z;
	r[1] = m[1]*x + m[5]*y + m[9]*z;
	r[2] = m[2]*x + m[6]*y + m[10]*z;
	return r;
}

#ifdef	__cplusplus
}
#endif
