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

#ifdef	__cplusplus
}
#endif