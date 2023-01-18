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

#define	mathDegToRad(deg)	(M_PI / 180.0 * (deg))
#define	mathDegToRad_f(deg)	(((float)M_PI) / 180.0f * ((float)(deg)))

#define	mathRadToDeg(rad)	((rad) * M_1_PI * 180.0)
#define	mathRadToDeg_f(rad)	(((float)(rad)) * ((float)M_1_PI) * 180.0f)

#ifdef	__cplusplus
extern "C" {
#endif

__declspec_dll int fcmpf(float a, float b, float epsilon);
__declspec_dll int fcmp(double a, double b, double epsilon);

__declspec_dll float facosf(float x);

__declspec_dll float finvsqrtf(float x);
__declspec_dll double finvsqrt(double x);

__declspec_dll float fsqrtf(float x);
__declspec_dll double fsqrt(double x);

__declspec_dll int mathQuadraticEquation(float a, float b, float c, float r[2]);

#ifdef	__cplusplus
}
#endif

#endif
