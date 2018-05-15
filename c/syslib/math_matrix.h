//
// Created by hujianzhe
//

#ifndef UTIL_C_SYSLIB_MATH_MATRIX_H
#define	UTIL_C_SYSLIB_MATH_MATRIX_H

#include "math.h"

typedef struct matrix_t {
	unsigned int row, col;
	double* val;
} matrix_t;

#define	matrix_val(m, r, c)		((m)->val[(r) * (m)->col + (c)])

#ifdef	__cplusplus
extern "C" {
#endif

int matrix_size_equal(const matrix_t* m1, const matrix_t* m2);
int matrix_equal(const matrix_t* m1, const matrix_t* m2);
matrix_t* matrix_copy(matrix_t* dst, const matrix_t* src);
matrix_t* matrix_dup(const matrix_t* m);
void matrix_delrow(matrix_t* m, unsigned int row);
void matrix_delcol(matrix_t* m, unsigned int col);
double matrix_det(const matrix_t* m);
matrix_t* matrix_identity(const matrix_t* m, matrix_t* mi);
matrix_t* matrix_add(matrix_t* res, const matrix_t* m1, const matrix_t* m2);
matrix_t* matrix_sub(matrix_t* res, const matrix_t* m1, const matrix_t* m2);
matrix_t* matrix_mulnum(matrix_t* m, double number);
matrix_t* matrix_divnum(matrix_t* m, double number);
matrix_t* matrix_mul(matrix_t* res, const matrix_t* left, const matrix_t* right);

#ifdef	__cplusplus
}
#endif

#endif // !UTIL_C_SCENE_MATRIX_MATH_H