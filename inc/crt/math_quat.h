//
// Created by hujianzhe
//

#ifndef	UTIL_C_CRT_MATH_QUAT_H
#define UTIL_C_CRT_MATH_QUAT_H

#include "../compiler_define.h"

#ifdef	__cplusplus
extern "C" {
#endif

__declspec_dll float* mathQuatSet(float q[4], float x, float y, float z, float w);
__declspec_dll float* mathQuatNormalized(float r[4], const float q[4]);
__declspec_dll float* mathQuatFromEuler(float q[4], const float e[3], const char order[3]);
__declspec_dll float* mathQuatFromUnitVec3(float q[4], const float from[3], const float to[3]);
__declspec_dll float* mathQuatFromAxisRadian(float q[4], const float axis[3], float radian);
__declspec_dll void mathQuatToAxisRadian(const float q[4], float axis[3], float* radian);
__declspec_dll int mathQuatIsZero(const float q[4]);
__declspec_dll int mathQuatEqual(const float q1[4], const float q2[4]);
__declspec_dll float* mathQuatIdentity(float q[4]);
__declspec_dll float* mathQuatToMat44(const float q[4], float m[16]);
__declspec_dll float* mathQuatFromMat33(float q[4], const float m[9]);
__declspec_dll float mathQuatDot(const float q1[4], const float q2[4]);
__declspec_dll float* mathQuatConjugate(float r[4], const float q[4]);
__declspec_dll float* mathQuatMultiplyScalar(float r[4], const float q[4], float n);
__declspec_dll float* mathQuatDivisionScalar(float r[4], const float q[4], float n);
__declspec_dll float* mathQuatMulQuat(float r[4], const float q1[4], const float q2[4]);
__declspec_dll float* mathQuatMulVec3(float r[3], const float q[4], const float v[3]);
__declspec_dll float* mathQuatMulVec3Inv(float r[3], const float q[4], const float v[3]);

#ifdef	__cplusplus
}
#endif

#endif
