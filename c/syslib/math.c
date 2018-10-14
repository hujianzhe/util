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

float* mathEulerToQuat(float e[3], float q[4]) {
	float pitch_x = e[0];
	float yaw_y = e[1];
	float roll_z = e[2];

	float cos_roll = cosf(roll_z * 0.5f);
	float sin_roll = sinf(roll_z * 0.5f);

	float cos_pitch = cosf(pitch_x * 0.5f);
	float sin_pitch = sinf(pitch_x * 0.5f);

	float cos_yaw = cosf(yaw_y * 0.5f);
	float sin_yaw = sinf(yaw_y * 0.5f);

	q[0] = cos_roll * cos_pitch * cos_yaw + sin_roll * sin_pitch * sin_yaw;
	q[1] = sin_roll * cos_pitch * cos_yaw - cos_roll * sin_pitch * sin_yaw;
	q[2] = cos_roll * sin_pitch * cos_yaw + sin_roll * cos_pitch * sin_yaw;
	q[3] = cos_roll * cos_pitch * sin_yaw - sin_roll * sin_pitch * cos_yaw;
	return q;
}

float* mathQuatToEuler(float q[4], float e[3]) {
	e[2] = atan2f(2.0f * (q[2]*q[3] + q[0]*q[1]), q[0]*q[0] - q[1]*q[1] - q[2]*q[2] + q[3]*q[3]);
	e[1] = atan2f(2.0f * (q[1]*q[2] + q[0]*q[3]), q[0]*q[0] + q[1]*q[1] - q[2]*q[2] - q[3]*q[3]);
	e[0] = asinf(2.0f * (q[0]*q[2] - q[1]*q[3]));
	return e;
}

float* mathUnitAxisRadianToQuat(float unitaxis[3], float radian, float q[4]) {
	const float half_rad = radian * 0.5f;
	const float s = sinf(half_rad);
	q[0] = unitaxis[0] * s;
	q[1] = unitaxis[1] * s;
	q[2] = unitaxis[2] * s;
	q[3] = cosf(half_rad);
	return q;
}

float* mathQuatToUnitAxisRadian(float q[4], float* radian, float axis[3]) {
	const qx = q[0], qy = q[1], qz = q[2], qw = q[3];
	const float s2 = qx*qx + qy*qy + qz*qz;
	const float s = 1.0f/sqrtf(s2);
	axis[0] = qx * s;
	axis[1] = qy * s;
	axis[2] = qz * s;
	*radian = atan2f(s2*s, qw) * 2.0f;
	return q;
}

float* mathQuatMulQuat(float q1[4], float q2[4], float r[4]) {
	const float q1x = q1[0], q1y = q1[1], q1z = q1[2], q1w = q1[3];
	const float q2x = q2[0], q2y = q2[1], q2z = q2[2], q2w = q2[3];
	r[0] = q1w*q2x + q2w*q1x + q1y*q2z - q2y*q1z;
	r[1] = q1w*q2y + q2w*q1y + q1z*q2x - q2z*q1x;
	r[2] = q1w*q2z + q2w*q1z + q1x*q2y - q2x*q1y;
	r[3] = q1w*q2w - q2x*q1x - q1y*q2y - q2z*q1z;
	return r;
}

float* mathQuatRotateVec3(float q[4], float v[3], float r[3]) {
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