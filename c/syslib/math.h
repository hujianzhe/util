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
typedef	float		float32_t;
typedef double		float64_t;
#if REAL_FLOAT_BIT == 64
	typedef	float64_t					real_t;
	#define	REAL_EPSILON				DBL_EPSILON
	#define	REAL_MIN					DBL_MIN
	#define	REAL_MAX					DBL_MAX
	#define	REAL_MATH_FUNC(_pf, ...)	_pf(##__VA_ARGS__)
#else
	#define	REAL_FLOAT_BIT				32
	typedef	float32_t					real_t;
	#define	REAL_EPSILON				FLT_EPSILON
	#define	REAL_MIN					FLT_MIN
	#define	REAL_MAX					FLT_MAX
	#define	REAL_MATH_FUNC(_pf, ...)	_pf##f(##__VA_ARGS__)
#endif

#ifdef	__cplusplus
extern "C" {
#endif

int fequf(float a, float b);
int fequ(double a, double b);
float fsqrtf(float x);
double fsqrt(double x);
#define	deg2rad(deg)	(M_PI / 180.0 * (deg))
#define	rad2deg(rad)	((rad) / M_PI * 180.0)

#ifdef	__cplusplus
}
#endif

#endif
