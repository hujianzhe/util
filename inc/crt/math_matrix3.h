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
__declspec_dll float* mathMat44Element(const float m[16], unsigned int column, unsigned int row);
__declspec_dll float* mathMat44ToMat33(const float m44[16], float m33[9]);
__declspec_dll float* mathMat44Copy(float r[16], const float m[16]);
__declspec_dll float* mathMat44Identity(float m[16]);

#ifdef	__cplusplus
}
#endif

#endif
