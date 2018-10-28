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

__declspec_dll int fcmpf(float a, float b, float epsilon);
__declspec_dll int fcmp(double a, double b, double epsilon);
__declspec_dll float fsqrtf(float x);
__declspec_dll double fsqrt(double x);

__declspec_dll int mathVec3IsZero(float v[3]);
__declspec_dll float mathVec3LenSq(float v[3]);
__declspec_dll float mathVec3Len(float v[3]);
__declspec_dll float* mathVec3Normalized(float r[3], float v[3]);
__declspec_dll float* mathVec3Negate(float r[3], float v[3]);
__declspec_dll float* mathVec3Mul(float r[3], float v[3], float n);
__declspec_dll float mathVec3Dot(float v1[3], float v2[3]);
__declspec_dll float mathVec3Radian(float v1[3], float v2[3]);
__declspec_dll float* mathVec3Cross(float r[3], float v1[3], float v2[3]);
__declspec_dll float* mathQuatFromEuler(float q[4], float e[3], const char order[3]);
__declspec_dll float* mathQuatFromAxisRadian(float q[4], float axis[3], float radian);
__declspec_dll void mathQuatToAxisRadian(float q[4], float axis[3], float* radian);
__declspec_dll float* mathQuatConjugate(float r[4], float q[4]);
__declspec_dll float* mathQuatMulQuat(float r[4], float q1[4], float q2[4]);
__declspec_dll float* mathQuatMulVec3(float r[3], float q[4], float v[3]);
__declspec_dll int mathRaycastTriangle(float origin[3], float dir[3], float vertices[3][3], float* t, float* u, float* v);
__declspec_dll int mathRaycastPlane(float origin[3], float dir[3], float vertices[3][3], float* t);
__declspec_dll int mathRaycastPlaneByNormalDistance(float origin[3], float dir[3], float normal[3], float d, float* distance);
__declspec_dll int mathRaycastSphere(float origin[3], float dir[3], float center[3], float radius, float* near_, float* far_);
#define	mathDegToRad(deg)	(M_PI / 180.0 * (deg))
#define	mathRadToDeg(rad)	((rad) * M_1_PI * 180.0)
#define	mathDegToRad_f(deg)	(((float)M_PI) / 180.0f * ((float)(deg)))
#define	mathRadToDeg_f(rad)	(((float)(rad)) * ((float)M_1_PI) * 180.0f)

#ifdef	__cplusplus
}
#endif

#endif
