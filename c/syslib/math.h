//
// Created by hujianzhe
//

#ifndef UTIL_C_SYSLIB_MATH_H
#define	UTIL_C_SYSLIB_MATH_H

#include "../compiler_define.h"

#ifndef	_USE_MATH_DEFINES
	#define	_USE_MATH_DEFINES
#endif
#include <float.h>
#include <math.h>

#ifdef	__cplusplus
extern "C" {
#endif

int fequf(float a, float b);
int fequ(double a, double b);

#ifdef	__cplusplus
}
#endif

typedef	float		float32_t;
typedef double		float64_t;

#ifdef REAL_USE_FLOAT64
	typedef	float64_t			real_t;
	#define	REAL_EPSILON		DBL_EPSILON
	#define	REAL_MIN			DBL_MIN
	#define	REAL_MAX			DBL_MAX
	#define	real_fn(_pf, ...)	_pf(##__VA_ARGS__)
#else
	typedef	float32_t			real_t;
	#define	REAL_EPSILON		FLT_EPSILON
	#define	REAL_MIN			FLT_MIN
	#define	REAL_MAX			FLT_MAX
	#define	real_fn(_pf, ...)	_pf##f(##__VA_ARGS__)
#endif

#endif
