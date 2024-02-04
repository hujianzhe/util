//
// Created by hujianzhe
//

#include "../../inc/crt/math_vec3.h"
#include "../../inc/crt/math_quat.h"
#include <math.h>

#ifdef	__cplusplus
extern "C" {
#endif

float* mathQuatSet(float q[4], float x, float y, float z, float w) {
	q[0] = x;
	q[1] = y;
	q[2] = z;
	q[3] = w;
	return q;
}

float* mathQuatNormalized(float r[4], const float q[4]) {
	float m = q[0] * q[0] + q[1] * q[1] + q[2] * q[2] + q[3] * q[3];
	if (m > 0.0f) {
		m = 1.0f / sqrtf(m);
		r[0] = q[0] * m;
		r[1] = q[1] * m;
		r[2] = q[2] * m;
		r[3] = q[3] * m;
	}
	return r;
}

float* mathQuatFromEuler(float q[4], const float e[3], const char order[3]) {
	float pitch_x = e[0];
	float yaw_y = e[1];
	float roll_z = e[2];

	float c1 = cosf(pitch_x * 0.5f);
	float c2 = cosf(yaw_y * 0.5f);
	float c3 = cosf(roll_z * 0.5f);
	float s1 = sinf(pitch_x * 0.5f);
	float s2 = sinf(yaw_y * 0.5f);
	float s3 = sinf(roll_z * 0.5f);

	if (order[0] == 'X' && order[1] == 'Y' && order[2] == 'Z') {
		q[0] = s1 * c2 * c3 + c1 * s2 * s3;
		q[1] = c1 * s2 * c3 - s1 * c2 * s3;
		q[2] = c1 * c2 * s3 + s1 * s2 * c3;
		q[3] = c1 * c2 * c3 - s1 * s2 * s3;
	}
	else if (order[0] == 'Y' && order[1] == 'X' && order[2] == 'Z') {
		q[0] = s1 * c2 * c3 + c1 * s2 * s3;
		q[1] = c1 * s2 * c3 - s1 * c2 * s3;
		q[2] = c1 * c2 * s3 - s1 * s2 * c3;
		q[3] = c1 * c2 * c3 + s1 * s2 * s3;
	}
	else if (order[0] == 'Z' && order[1] == 'X' && order[2] == 'Y') {
		q[0] = s1 * c2 * c3 - c1 * s2 * s3;
		q[1] = c1 * s2 * c3 + s1 * c2 * s3;
		q[2] = c1 * c2 * s3 + s1 * s2 * c3;
		q[3] = c1 * c2 * c3 - s1 * s2 * s3;
	}
	else if (order[0] == 'Z' && order[1] == 'Y' && order[2] == 'X') {
		q[0] = s1 * c2 * c3 - c1 * s2 * s3;
		q[1] = c1 * s2 * c3 + s1 * c2 * s3;
		q[2] = c1 * c2 * s3 - s1 * s2 * c3;
		q[3] = c1 * c2 * c3 + s1 * s2 * s3;
	}
	else if (order[0] == 'Y' && order[1] == 'Z' && order[2] == 'X') {
		q[0] = s1 * c2 * c3 + c1 * s2 * s3;
		q[1] = c1 * s2 * c3 + s1 * c2 * s3;
		q[2] = c1 * c2 * s3 - s1 * s2 * c3;
		q[3] = c1 * c2 * c3 - s1 * s2 * s3;
	}
	else if (order[0] == 'X' && order[1] == 'Z' && order[2] == 'Y') {
		q[0] = s1 * c2 * c3 - c1 * s2 * s3;
		q[1] = c1 * s2 * c3 - s1 * c2 * s3;
		q[2] = c1 * c2 * s3 + s1 * s2 * c3;
		q[3] = c1 * c2 * c3 + s1 * s2 * s3;
	}
	else {
		q[0] = q[1] = q[2] = 0.0f;
		q[3] = 1.0f;
	}
	return q;
}

float* mathQuatFromUnitVec3(float q[4], const float from[3], const float to[3]) {
	float v[3];
	float w = mathVec3Dot(from, to) + 1.0f;
	if (w < 1E-7f) {
		float from_abs_x = from[0] > 0.0f ? from[0] : -from[0];
		float from_abs_z = from[2] > 0.0f ? from[2] : -from[2];
		if (from_abs_x > from_abs_z) {
			v[0] = -from[1];
			v[1] = from[0];
			v[2] = 0.0f;
		}
		else {
			v[0] = 0.0f;
			v[1] = -from[2];
			v[2] = from[1];
		}
		w = 0.0f;
	}
	else {
		mathVec3Cross(v, from, to);
	}

	q[0] = v[0];
	q[1] = v[1];
	q[2] = v[2];
	q[3] = w;
	return mathQuatNormalized(q, q);
}

float* mathQuatFromAxisRadian(float q[4], const float axis[3], float radian) {
	const float half_rad = radian * 0.5f;
	const float s = sinf(half_rad);
	q[0] = axis[0] * s;
	q[1] = axis[1] * s;
	q[2] = axis[2] * s;
	q[3] = cosf(half_rad);
	return q;
}

void mathQuatToAxisRadian(const float q[4], float axis[3], float* radian) {
	const float qx = q[0], qy = q[1], qz = q[2], qw = q[3];
	const float s2 = qx*qx + qy*qy + qz*qz;
	const float s = 1.0f / sqrtf(s2);
	axis[0] = qx * s;
	axis[1] = qy * s;
	axis[2] = qz * s;
	*radian = atan2f(s2*s, qw) * 2.0f;
}

int mathQuatIsZero(const float q[4]) {
	return 	q[0] <= CCT_EPSILON && q[1] <= CCT_EPSILON && q[2] <= CCT_EPSILON && q[3] <= CCT_EPSILON &&
			q[0] >= CCT_EPSILON_NEGATE && q[1] >= CCT_EPSILON_NEGATE && q[2] >= CCT_EPSILON_NEGATE && q[3] >= CCT_EPSILON_NEGATE;
}

int mathQuatEqual(const float q1[4], const float q2[4]) {
	float delta;

	delta = q1[0] - q2[0];
	if (delta > CCT_EPSILON || delta < CCT_EPSILON_NEGATE) {
		return 0;
	}
	delta = q1[1] - q2[1];
	if (delta > CCT_EPSILON || delta < CCT_EPSILON_NEGATE) {
		return 0;
	}
	delta = q1[2] - q2[2];
	if (delta > CCT_EPSILON || delta < CCT_EPSILON_NEGATE) {
		return 0;
	}
	delta = q1[3] - q2[3];
	if (delta > CCT_EPSILON || delta < CCT_EPSILON_NEGATE) {
		return 0;
	}
	return 1;
}

float* mathQuatIdentity(float q[4]) {
	q[0] = q[1] = q[2] = 0.0f;
	q[3] = 1.0f;
	return q;
}

float* mathQuatToMat44(const float q[4], float m[16]) {
	float x = q[0];
	float y = q[1];
	float z = q[2];
	float w = q[3];

	float x2 = x + x;
	float y2 = y + y;
	float z2 = z + z;

	float xx = x2 * x;
	float yy = y2 * y;
	float zz = z2 * z;

	float xy = x2 * y;
	float xz = x2 * z;
	float xw = x2 * w;

	float yz = y2 * z;
	float yw = y2 * w;
	float zw = z2 * w;

	/* column 0 */
	m[0] = 1.0f - yy - zz;
	m[1] = xy + zw;
	m[2] = xz - yw;
	m[3] = 0.0f;
	/* column 1 */
	m[4] = xy - zw;
	m[5] = 1.0f - xx - zz;
	m[6] = yz + xw;
	m[7] = 0.0f;
	/* column 2 */
	m[8] = xz + yw;
	m[9] = yz - xw;
	m[10] = 1.0f - xx - yy;
	m[11] = 0.0f;
	/* column 3 */
	m[12] = 0.0f;
	m[13] = 0.0f;
	m[14] = 0.0f;
	m[15] = 1.0f;
	return m;
}

float* mathQuatFromMat33(float q[4], const float m[9]) {
	float t;
	if (m[8] < 0.0f) {
		if (m[0] > m[4]) {
			t = 1 + m[0] - m[4] - m[8];
			mathQuatSet(q, t, m[1]+m[3], m[6]+m[2], m[5]-m[7]);
		}
		else {
			t = 1 - m[0] + m[4] - m[8];
			mathQuatSet(q, m[1]+m[3], t, m[5]+m[7], m[6]-m[2]);
		}
	}
	else {
		if (m[0] < -m[4]) {
			t = 1 - m[0] - m[4] + m[8];
			mathQuatSet(q, m[6]+m[2], m[5]+m[7], t, m[1]-m[3]);
		}
		else {
			t = 1 + m[0] + m[4] + m[8];
			mathQuatSet(q, m[5]-m[7], m[6]-m[2], m[1]-m[3], t);
		}
	}
	mathQuatMultiplyScalar(q, q, 0.5f / sqrtf(t));
	return q;
}

float mathQuatDot(const float q1[4], const float q2[4]) {
	return q1[0]* q2[0] + q1[1]*q2[1] + q1[2]*q2[2] + q1[3]*q2[3];
}

/* r = -q */
float* mathQuatConjugate(float r[4], const float q[4]) {
	r[0] = -q[0];
	r[1] = -q[1];
	r[2] = -q[2];
	r[3] = q[3];
	return r;
}

/* r = q*n */
float* mathQuatMultiplyScalar(float r[4], const float q[4], float n) {
	r[0] = q[0] * n;
	r[1] = q[1] * n;
	r[2] = q[2] * n;
	r[3] = q[3] * n;
	return r;
}

/* r = q/n */
float* mathQuatDivisionScalar(float r[4], const float q[4], float n) {
	if (n > 1e-6f || n < -1e-6f) {
		float inv = 1.0f / n;
		r[0] = q[0] * inv;
		r[1] = q[1] * inv;
		r[2] = q[2] * inv;
		r[3] = q[3] * inv;
	}
	return r;
}

/* r = q1 * q2 */
float* mathQuatMulQuat(float r[4], const float q1[4], const float q2[4]) {
	const float q1x = q1[0], q1y = q1[1], q1z = q1[2], q1w = q1[3];
	const float q2x = q2[0], q2y = q2[1], q2z = q2[2], q2w = q2[3];
	r[0] = q1w*q2x + q2w*q1x + q1y*q2z - q2y*q1z;
	r[1] = q1w*q2y + q2w*q1y + q1z*q2x - q2z*q1x;
	r[2] = q1w*q2z + q2w*q1z + q1x*q2y - q2x*q1y;
	r[3] = q1w*q2w - q2x*q1x - q1y*q2y - q2z*q1z;
	return r;
}

/* r = q * v */
float* mathQuatMulVec3(float r[3], const float q[4], const float v[3]) {
	const float vx = 2.0f * v[0];
	const float vy = 2.0f * v[1];
	const float vz = 2.0f * v[2];
	const float qx = q[0], qy = q[1], qz = q[2], qw = q[3];
	const float qw2 = qw*qw - 0.5f;
	const float dot2 = qx*vx + qy*vy + qz*vz;
	r[0] = vx*qw2 + (qy * vz - qz * vy)*qw + qx*dot2;
	r[1] = vy*qw2 + (qz * vx - qx * vz)*qw + qy*dot2;
	r[2] = vz*qw2 + (qx * vy - qy * vx)*qw + qz*dot2;
	return r;
}

/* r = q * v */
float* mathQuatMulVec3Inv(float r[3], const float q[4], const float v[3]) {
	const float vx = 2.0f * v[0];
	const float vy = 2.0f * v[1];
	const float vz = 2.0f * v[2];
	const float qx = q[0], qy = q[1], qz = q[2], qw = q[3];
	const float qw2 = qw*qw - 0.5f;
	const float dot2 = qx*vx + qy*vy + qz*vz;
	r[0] = vx*qw2 - (qy * vz - qz * vy)*qw + qx*dot2;
	r[1] = vy*qw2 - (qz * vx - qx * vz)*qw + qy*dot2;
	r[2] = vz*qw2 - (qx * vy - qy * vx)*qw + qz*dot2;
	return r;
}

#ifdef	__cplusplus
}
#endif
