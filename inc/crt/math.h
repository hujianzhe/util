//
// Created by hujianzhe
//

#ifndef UTIL_C_CRT_MATH_H
#define	UTIL_C_CRT_MATH_H

#include "../compiler_define.h"

#ifndef	_USE_MATH_DEFINES
	#define	_USE_MATH_DEFINES
#endif
#include <float.h>
#include <math.h>
#ifndef CCT_EPSILON
	#define	CCT_EPSILON					1E-5f
#endif

#ifdef	__cplusplus
extern "C" {
#endif

__declspec_dll int fcmpf(float a, float b, float epsilon);
__declspec_dll int fcmp(double a, double b, double epsilon);
__declspec_dll float finvsqrtf(float x);
__declspec_dll float fsqrtf(float x);
__declspec_dll double finvsqrt(double x);
__declspec_dll double fsqrt(double x);

#define	mathDegToRad(deg)	(M_PI / 180.0 * (deg))
#define	mathRadToDeg(rad)	((rad) * M_1_PI * 180.0)
#define	mathDegToRad_f(deg)	(((float)M_PI) / 180.0f * ((float)(deg)))
#define	mathRadToDeg_f(rad)	(((float)(rad)) * ((float)M_1_PI) * 180.0f)

__declspec_dll int mathQuadraticEquation(float a, float b, float c, float r[2]);

__declspec_dll int mathVec3IsZero(const float v[3]);
__declspec_dll int mathVec3Equal(const float v1[3], const float v2[3]);
__declspec_dll float* mathVec3Copy(float r[3], const float v[3]);
__declspec_dll float mathVec3LenSq(const float v[3]);
__declspec_dll float mathVec3Len(const float v[3]);
__declspec_dll float mathVec3Normalized(float r[3], const float v[3]);
__declspec_dll float* mathVec3Negate(float r[3], const float v[3]);
__declspec_dll float* mathVec3Add(float r[3], const float v1[3], const float v2[3]);
__declspec_dll float* mathVec3AddScalar(float r[3], const float v[3], float n);
__declspec_dll float* mathVec3Sub(float r[3], const float v1[3], const float v2[3]);
__declspec_dll float* mathVec3MultiplyScalar(float r[3], const float v[3], float n);
__declspec_dll float mathVec3Dot(const float v1[3], const float v2[3]);
__declspec_dll float mathVec3Radian(const float v1[3], const float v2[3]);
__declspec_dll float* mathVec3Cross(float r[3], const float v1[3], const float v2[3]);

__declspec_dll float* mathCoordinateSystemTransform(const float v[3], const float new_origin[3], float new_axies[3][3], float new_v[3]);

__declspec_dll float* mathQuatNormalized(float r[4], const float q[4]);
__declspec_dll float* mathQuatFromEuler(float q[4], const float e[3], const char order[3]);
__declspec_dll float* mathQuatFromUnitVec3(float q[4], const float from[3], const float to[3]);
__declspec_dll float* mathQuatFromAxisRadian(float q[4], const float axis[3], float radian);
__declspec_dll void mathQuatToAxisRadian(float q[4], float axis[3], float* radian);
__declspec_dll float* mathQuatIdentity(float q[4]);
__declspec_dll float* mathQuatConjugate(float r[4], const float q[4]);
__declspec_dll float* mathQuatMulQuat(float r[4], const float q1[4], const float q2[4]);
__declspec_dll float* mathQuatMulVec3(float r[3], const float q[4], const float v[3]);

#ifdef	__cplusplus
}
#endif

#endif
