//
// Created by hujianzhe
//

#ifndef	UTIL_C_CRT_GEOMETRY_TRIANGLE_H
#define	UTIL_C_CRT_GEOMETRY_TRIANGLE_H

#include "../../compiler_define.h"

#ifdef	__cplusplus
extern "C" {
#endif

__declspec_dll void mathTriangleGetPoint(const float tri[3][3], float u, float v, float p[3]);
__declspec_dll int mathTriangleHasPoint(const float tri[3][3], const float p[3], float* p_u, float* p_v);

#ifdef	__cplusplus
}
#endif

#endif
