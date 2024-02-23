//
// Created by hujianzhe
//

#ifndef	UTIL_C_CRT_MATH_MATRIX3_H
#define	UTIL_C_CRT_MATH_MATRIX3_H

#include "../compiler_define.h"

/* note: Matrix[column][row] */

#ifdef	__cplusplus
extern "C" {
#endif

__declspec_dll void mathMat44TransformSplit(const float m[16], float T[3], float S[3], float R[9]);
__declspec_dll float* mathMat44SetPositionPart(float m[16], const float p[3]);
__declspec_dll float* mathMat44Element(const float m[16], unsigned int column, unsigned int row);
__declspec_dll float* mathMat44ToMat33(const float m44[16], float m33[9]);
__declspec_dll float* mathMat44Copy(float r[16], const float m[16]);
__declspec_dll float* mathMat44Identity(float m[16]);
__declspec_dll float* mathMat44Add(float r[16], const float m1[16], const float m2[16]);
__declspec_dll float* mathMat44MultiplyScalar(float r[16], const float m[16], float n);
__declspec_dll float* mathMat44MulMat44(float r[16], const float m1[16], const float m2[16]);
__declspec_dll float* mathMat44Transpose(float r[16], const float m[16]);
__declspec_dll float* mathMat44Inverse(float r[16], const float m[16]);
__declspec_dll float* mathMat44TransformVec3(float r[3], const float m[16], const float v[3]);
__declspec_dll float* mathMat44RotateVec3(float r[3], const float m[16], const float v[3]);
__declspec_dll float* mathMat44FromQuat(float m[16], const float q[4]);
__declspec_dll float* mathMat44ToQuat(const float m[16], float q[4]);
__declspec_dll float* mathMat33ToQuat(const float m[9], float q[4]);
__declspec_dll float* mathMat33FromQuat(float m[9], const float q[4]);

#ifdef	__cplusplus
}
#endif

#endif
