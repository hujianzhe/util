//
// Created by hujianzhe
//

#ifndef UTIL_C_SYSLIB_MATH_MATRIX_H
#define	UTIL_C_SYSLIB_MATH_MATRIX_H

#include "math.h"

typedef struct matrix_t {
	unsigned int row, col;
	real_t* val;
} matrix_t;

#define	matrix_val(m, r, c)		((m)->val[(r) * (m)->col + (c)])

#ifdef	__cplusplus
extern "C" {
#endif

__declspec_dll int matrix_size_equal(const matrix_t* m1, const matrix_t* m2);
__declspec_dll int matrix_equal(const matrix_t* m1, const matrix_t* m2);
__declspec_dll matrix_t* matrix_copy(matrix_t* dst, const matrix_t* src);
__declspec_dll matrix_t* matrix_dup(const matrix_t* m);
__declspec_dll void matrix_delrow(matrix_t* m, unsigned int row);
__declspec_dll void matrix_delcol(matrix_t* m, unsigned int col);
__declspec_dll real_t matrix_det(const matrix_t* m);
__declspec_dll matrix_t* matrix_identity(matrix_t* mi, const matrix_t* m);
__declspec_dll matrix_t* matrix_add(matrix_t* res, const matrix_t* m1, const matrix_t* m2);
__declspec_dll matrix_t* matrix_sub(matrix_t* res, const matrix_t* m1, const matrix_t* m2);
__declspec_dll matrix_t* matrix_mulnum(matrix_t* m, real_t number);
__declspec_dll matrix_t* matrix_divnum(matrix_t* m, real_t number);
__declspec_dll matrix_t* matrix_mul(matrix_t* res, const matrix_t* left, const matrix_t* right);

#ifdef	__cplusplus
}
#endif

#endif // !UTIL_C_SCENE_MATRIX_MATH_H