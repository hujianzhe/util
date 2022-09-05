//
// Created by hujianzhe
//

#include "../../inc/crt/math.h"
#include "../../inc/crt/math_vec3.h"
#include "../../inc/crt/math_quat.h"
#include <stddef.h>

#ifdef	__cplusplus
extern "C" {
#endif

float* mathQuatNormalized(float r[4], const float q[4]) {
	float m = q[0] * q[0] + q[1] * q[1] + q[2] * q[2] + q[3] * q[3];
	if (m > CCT_EPSILON) {
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

float* mathQuatIdentity(float q[4]) {
	q[0] = q[1] = q[2] = 0.0f;
	q[3] = 1.0f;
	return q;
}

/* r = -q */
float* mathQuatConjugate(float r[4], const float q[4]) {
	r[0] = -q[0];
	r[1] = -q[1];
	r[2] = -q[2];
	r[3] = q[3];
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

#ifdef	__cplusplus
}
#endif
