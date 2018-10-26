//
// Created by hujianzhe
//

#include "math.h"

#ifdef	__cplusplus
extern "C" {
#endif

int fequf(float a, float b) {
	float v = a - b;
	return v > -FLT_EPSILON && v < FLT_EPSILON;
}
int fequ(double a, double b) {
	double v = a - b;
	return v > -DBL_EPSILON && v < DBL_EPSILON;
}

float fsqrtf(float x) {
	float xhalf = 0.5f * x;
	int i = *(int*)&x;
	i = 0x5f3759df - (i >> 1);
	x = *(float*)&i;
	x = x * (1.5f - xhalf * x * x);
	return x;
}

double fsqrt(double x) {
	double xhalf = 0.5 * x;
	long long i = *(long long*)&x;
	i = 0x5fe6ec85e7de30da - (i >> 1);
	x = *(double*)&i;
	x = x * (1.5 - xhalf * x * x);
	return x;
}

float mathVec3LenSq(float v[3]) {
	return v[0] * v[0] + v[1] * v[1] + v[2] * v[2];
}

float mathVec3Len(float v[3]) {
	return sqrtf(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
}

float* mathVec3Normalized(float r[3], float v[3]) {
	float len = mathVec3Len(v);
	if (len > 1E-7f) {
		float inv_len = 1.0f / len;
		r[0] = v[0] * inv_len;
		r[1] = v[1] * inv_len;
		r[2] = v[2] * inv_len;
	}
	return r;
}

float* mathVec3Mul(float r[3], float v[3], float n) {
	r[0] = v[0] * n;
	r[1] = v[1] * n;
	r[2] = v[2] * n;
	return r;
}

float mathVec3Dot(float v1[3], float v2[3]) {
	return v1[0] * v2[0] + v1[1] * v2[1] + v1[2] * v2[2];
}

float mathVec3Radian(float v1[3], float v2[3]) {
	return acosf(mathVec3Dot(v1, v2) / mathVec3Len(v1) * mathVec3Len(v2));
}

/* r = v1 X v2 */
float* mathVec3Cross(float r[3], float v1[3], float v2[3]) {
	float x = v1[1] * v2[2] - v1[2] * v2[1];
	float y = v1[2] * v2[0] - v1[0] * v2[2];
	float z = v1[0] * v2[1] - v1[1] * v2[0];
	r[0] = x;
	r[1] = y;
	r[2] = z;
	return r;
}

float* mathQuatFromEuler(float q[4], float e[3], const char order[3]) {
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

/*
float* mathQuatToEuler(float q[4], float e[3]) {
	e[2] = atan2f(2.0f * (q[2]*q[3] + q[0]*q[1]), q[0]*q[0] - q[1]*q[1] - q[2]*q[2] + q[3]*q[3]);
	e[1] = atan2f(2.0f * (q[1]*q[2] + q[0]*q[3]), q[0]*q[0] + q[1]*q[1] - q[2]*q[2] - q[3]*q[3]);
	e[0] = asinf(2.0f * (q[0]*q[2] - q[1]*q[3]));
	return e;
}
*/

float* mathQuatFromAxisRadian(float q[4], float axis[3], float radian) {
	const float half_rad = radian * 0.5f;
	const float s = sinf(half_rad);
	q[0] = axis[0] * s;
	q[1] = axis[1] * s;
	q[2] = axis[2] * s;
	q[3] = cosf(half_rad);
	return q;
}

void mathQuatToAxisRadian(float q[4], float axis[3], float* radian) {
	const qx = q[0], qy = q[1], qz = q[2], qw = q[3];
	const float s2 = qx*qx + qy*qy + qz*qz;
	const float s = 1.0f / sqrtf(s2);
	axis[0] = qx * s;
	axis[1] = qy * s;
	axis[2] = qz * s;
	*radian = atan2f(s2*s, qw) * 2.0f;
}

/* r = -q */
float* mathQuatConjugate(float r[4], float q[4]) {
	r[0] = -q[0];
	r[1] = -q[1];
	r[2] = -q[2];
	r[3] = q[3];
	return r;
}

/* r = q1 * q2 */
float* mathQuatMulQuat(float r[4], float q1[4], float q2[4]) {
	const float q1x = q1[0], q1y = q1[1], q1z = q1[2], q1w = q1[3];
	const float q2x = q2[0], q2y = q2[1], q2z = q2[2], q2w = q2[3];
	r[0] = q1w*q2x + q2w*q1x + q1y*q2z - q2y*q1z;
	r[1] = q1w*q2y + q2w*q1y + q1z*q2x - q2z*q1x;
	r[2] = q1w*q2z + q2w*q1z + q1x*q2y - q2x*q1y;
	r[3] = q1w*q2w - q2x*q1x - q1y*q2y - q2z*q1z;
	return r;
}

/* r = q * v */
float* mathQuatMulVec3(float r[3], float q[4], float v[3]) {
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

int mathRaycastTriangle(float origin[3], float dir[3], float vertices[3][3], float* t, float* u, float* v) {
	float det, inv_det;
	float *v0 = vertices[0], *v1 = vertices[1], *v2 = vertices[2];
	float E1[3] = {
		v1[0] - v0[0],
		v1[1] - v0[1],
		v1[2] - v0[2]
	};
	float E2[3] = {
		v2[0] - v0[0],
		v2[1] - v0[1],
		v2[2] - v0[2]
	};
	float P[3], T[3], Q[3];
	mathVec3Cross(P, dir, E2);
	det = mathVec3Dot(E1, P);
	if (-0.000001f < det && det < 0.000001f)
		return 0;
	inv_det = 1.0f / det;
	T[0] = origin[0] - v0[0];
	T[1] = origin[1] - v0[1];
	T[2] = origin[2] - v0[2];
	*u = mathVec3Dot(T, P) * inv_det;
	if (*u < 0.0f || *u > 1.0f)
		return 0;
	mathVec3Cross(Q, T, E1);
	*v = mathVec3Dot(dir, Q) * inv_det;
	if (*v < 0.0f || *v + *u > 1.0f)
		return 0;
	*t = mathVec3Dot(E2, Q) * inv_det;
	return *t >= 0.0f;/* return 1 */
}

int mathRaycastPlane(float origin[3], float dir[3], float normal[3], float d, float* distance) {
	float dn = mathVec3Dot(dir, normal);
	if (-1E-7f < dn && dn < 1E-7f)
		return 0;
	*distance = (mathVec3Dot(origin, normal) + d) / dn;
	return 1;
}

int mathRaycastSphere(float origin[3], float dir[3], float center[3], float radius, float* near_, float* far_) {
	float radius2 = radius * radius;
	float d, dr2;
	float v[3] = {
		center[0] - origin[0],
		center[1] - origin[1],
		center[2] - origin[2]
	};
	float oc2 = mathVec3LenSq(v);
	float dir_d = mathVec3Dot(dir, v);
	if (oc2 >= radius2 && dir_d < 0.0f)
		return 0;
	dr2 = oc2 - dir_d * dir_d;
	if (dr2 >= radius2)
		return 0;

	d = sqrtf(radius2 - dr2);
	if (oc2 >= radius) {
		*near_ = dir_d - d;
		*far_ = dir_d + d;
	}
	else {
		*near_ = dir_d + d;
		*far_ = dir_d - d;
	}
	return 1;
}

#ifdef	__cplusplus
}
#endif