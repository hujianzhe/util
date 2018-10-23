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

__declspec_dll float* mathQuatConjugate(float q[4], float r[4]);
__declspec_dll float* mathEulerToQuat(float e[3], float q[4], const char order[3]);
__declspec_dll float* mathAxisRadianToQuat(float axis[3], float radian, float q[4]);
__declspec_dll float* mathQuatToAxisRadian(float q[4], float* radian, float axis[3]);
__declspec_dll float* mathQuatMulQuat(float q1[4], float q2[4], float r[4]);
__declspec_dll float* mathQuatRotateVec3(float q[4], float v[3], float r[3]);
__declspec_dll int mathRaycastTriangle(float origin[3], float dir[3], float vertices[3][3], float* t, float* u, float* v);
#define	mathDegToRad(deg)	(M_PI / 180.0 * (deg))
#define	mathRadToDeg(rad)	((rad) * M_1_PI * 180.0)
#define	mathDegToRad_f(deg)	(((float)M_PI) / 180.0f * ((float)(deg)))
#define	mathRadToDeg_f(rad)	(((float)(rad)) * ((float)M_1_PI) * 180.0f)

#ifdef	__cplusplus
}
#endif

#endif
