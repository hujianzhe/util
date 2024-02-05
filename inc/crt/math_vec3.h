//
// Created by hujianzhe
//

#ifndef UTIL_C_CRT_MATH_VEC3_H
#define	UTIL_C_CRT_MATH_VEC3_H

#include "../compiler_define.h"

#ifndef CCT_EPSILON
	#define	CCT_EPSILON			1E-5f
	#define	CCT_EPSILON_NEGATE	-1E-5f
#endif

#ifdef	__cplusplus
extern "C" {
#endif

__declspec_dll float* mathCoordinateSystemTransform(const float v[3], const float new_origin[3], const float new_axies[3][3], float new_v[3]);

__declspec_dll int mathVec3IsZero(const float v[3]);
__declspec_dll int mathVec3Equal(const float v1[3], const float v2[3]);
__declspec_dll float mathVec3MinElement(const float v[3]);
__declspec_dll float mathVec3MaxElement(const float v[3]);
__declspec_dll float* mathVec3Set(float r[3], float x, float y, float z);
__declspec_dll float* mathVec3Copy(float r[3], const float v[3]);
__declspec_dll float mathVec3LenSq(const float v[3]);
__declspec_dll float mathVec3Len(const float v[3]);
__declspec_dll float mathVec3Normalized(float r[3], const float v[3]);
__declspec_dll float mathVec3Direction(const float end[3], const float start[3], float dir[3]);
__declspec_dll float* mathVec3Negate(float r[3], const float v[3]);
__declspec_dll float* mathVec3Add(float r[3], const float v1[3], const float v2[3]);
__declspec_dll float* mathVec3AddScalar(float r[3], const float v[3], float n);
__declspec_dll float* mathVec3SubScalar(float r[3], const float v[3], float n);
__declspec_dll float* mathVec3Sub(float r[3], const float v1[3], const float v2[3]);
__declspec_dll float* mathVec3MultiplyScalar(float r[3], const float v[3], float n);
__declspec_dll float* mathVec3DivisionScalar(float r[3], const float v[3], float n);
__declspec_dll float mathVec3Dot(const float v1[3], const float v2[3]);
__declspec_dll float mathVec3Radian(const float v1[3], const float v2[3]);
__declspec_dll float* mathVec3Cross(float r[3], const float v1[3], const float v2[3]);
__declspec_dll float* mathVec3Reflect(float r[3], const float v[3], const float n[3]);
__declspec_dll void mathVec3ComputeBasis(const float dir[3], float right[3], float up[3]);
__declspec_dll float* mathVec3DelComponent(float r[3], const float v[3], const float dir[3]);

#ifdef	__cplusplus
}
#endif

#endif // !UTIL_C_CRT_MATH_VEC3_H
