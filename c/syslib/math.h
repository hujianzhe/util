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
	#define	REAL_MATH_FUNC(fn)			fn
#else
	#define	REAL_FLOAT_BIT				32
	typedef	float32_t					real_t;
	#define	REAL_EPSILON				FLT_EPSILON
	#define	REAL_MIN					FLT_MIN
	#define	REAL_MAX					FLT_MAX
	#define	REAL_MATH_FUNC(fn)			fn##f
#endif

#ifdef	__cplusplus
extern "C" {
#endif

__declspec_dll int fequf(float a, float b);
__declspec_dll int fequ(double a, double b);
__declspec_dll float fsqrtf(float x);
__declspec_dll double fsqrt(double x);
__declspec_dll float* mathEulerAnglesToQuaternion(float e[3], float q[4]);
__declspec_dll float* mathQuaternionToEulerAngles(float q[4], float e[3]);
#define	mathDegToRad(deg)	(M_PI / 180.0 * (deg))
#define	mathRadToDeg(rad)	((rad) / M_PI * 180.0)

#ifdef	__cplusplus
}
#endif

#endif
