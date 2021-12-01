//
// Created by hujianzhe
//

#include "../../inc/crt/math.h"
#include <stddef.h>

#ifdef	__cplusplus
extern "C" {
#endif

/*
a == b return =0, a > b return >0, a < b return <0;
*/
int fcmpf(float a, float b, float epsilon) {
	if (a > b + epsilon) {
		return 1;
	}
	if (a < b - epsilon) {
		return -1;
	}
	return 0;
}

/*
a == b return 0, a > b return >0, a < b return <0;
*/
int fcmp(double a, double b, double epsilon) {
	if (a > b + epsilon) {
		return 1;
	}
	if (a < b - epsilon) {
		return -1;
	}
	return 0;
}

float finvsqrtf(float x) {
	float xhalf = 0.5f * x;
	int i = *(int*)&x;
	i = 0x5f3759df - (i >> 1);
	x = *(float*)&i;
	x = x * (1.5f - xhalf * x * x);
	x = x * (1.5f - xhalf * x * x);
	x = x * (1.5f - xhalf * x * x);
	return x;
}

double finvsqrt(double x) {
	double xhalf = 0.5 * x;
	long long i = *(long long*)&x;
	i = 0x5fe6ec85e7de30da - (i >> 1);
	x = *(double*)&i;
	x = x * (1.5 - xhalf * x * x);
	x = x * (1.5 - xhalf * x * x);
	x = x * (1.5 - xhalf * x * x);
	return x;
}

float fsqrtf(float x) { return 1.0f / finvsqrtf(x); }

double fsqrt(double x) { return 1.0 / finvsqrt(x); }

#ifdef	__cplusplus
}
#endif
