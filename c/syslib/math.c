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

float* mathEulerAnglesToQuaternion(float e[3], float q[4]) {
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

float* mathQuaternionToEulerAngles(float q[4], float e[3]) {
	e[2] = atan2f(2.0f * (q[2]*q[3] + q[0]*q[1]), q[0]*q[0] - q[1]*q[1] - q[2]*q[2] + q[3]*q[3]);
	e[1] = atan2f(2.0f * (q[1]*q[2] + q[0]*q[3]), q[0]*q[0] + q[1]*q[1] - q[2]*q[2] - q[3]*q[3]);
	e[0] = asinf(2.0f * (q[0]*q[2] - q[1]*q[3]));
	return e;
}

#ifdef	__cplusplus
}
#endif