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

#ifdef	__cplusplus
}
#endif