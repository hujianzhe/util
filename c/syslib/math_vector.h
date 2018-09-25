//
// Created by hujianzhe
//

#ifndef	UTIL_C_SYSLIB_MATH_VECTOR_H
#define	UTIL_C_SYSLIB_MATH_VECTOR_H

#include "math.h"

typedef union vector2_t {
	struct {
		real_t x, y;
	};
	real_t val[2];
} vector2_t;
typedef union vector3_t {
	struct {
		real_t x, y, z;
	};
	real_t val[3];
} vector3_t;
typedef union vector4_t {
	struct {
		real_t x, y, z, w;
	};
	real_t val[4];
} vector4_t;

#ifdef	__cplusplus
extern "C" {
#endif

__declspec_dll real_t* vector_assign(real_t* v, const real_t val[], int dimension);
__declspec_dll vector2_t* vector2_assign(vector2_t* v, real_t x, real_t y);
__declspec_dll vector3_t* vector3_assign(vector3_t* v, real_t x, real_t y, real_t z);

__declspec_dll int vector_is_zero(const real_t* v, int dimension);
__declspec_dll int vector3_is_zero(const vector3_t* v);

__declspec_dll int vector_equal(const real_t* v1, const real_t* v2, int dimension);
__declspec_dll int vector2_equal(const vector2_t* v1, const vector2_t* v2);
__declspec_dll int vector3_equal(const vector3_t* v1, const vector3_t* v2);

__declspec_dll real_t vector_dot(const real_t* v1, const real_t* v2, int dimension);
__declspec_dll real_t vector2_dot(const vector2_t* v1, const vector2_t* v2);
__declspec_dll real_t vector3_dot(const vector3_t* v1, const vector3_t* v2);

__declspec_dll real_t vector2_cross(const vector2_t* v1, const vector2_t* v2);
__declspec_dll vector3_t* vector3_cross(vector3_t* res, const vector3_t* v1, const vector3_t* v2);

__declspec_dll real_t vector_lensq(const real_t* v, int dimension);
__declspec_dll real_t vector2_lensq(const vector2_t* v);
__declspec_dll real_t vector3_lensq(const vector3_t* v);
__declspec_dll real_t vector_len(const real_t* v, int dimension);
__declspec_dll real_t vector2_len(const vector2_t* v);
__declspec_dll real_t vector3_len(const vector3_t* v);

__declspec_dll real_t vector2_radian(const vector2_t* v1, const vector2_t* v2);
__declspec_dll real_t vector3_radian(const vector3_t* v1, const vector3_t* v2);

__declspec_dll real_t* vector_normalized(real_t* n, const real_t* v, int dimension);
__declspec_dll vector2_t* vector2_normalized(vector2_t* n, const vector2_t* v);
__declspec_dll vector3_t* vector3_normalized(vector3_t* n, const vector3_t* v);

#ifdef	__cplusplus
}
#endif

#endif