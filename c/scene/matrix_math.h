//
// Created by hujianzhe on 18-4-24.
//

#ifndef UTIL_C_SCENE_MATRIX_MATH_H
#define	UTIL_C_SCENE_MATRIX_MATH_H

typedef struct matrix_t {
	unsigned int row, col;
	double* val;
} matrix_t;

/*
#define	matrix_make(m, r, c, allocator)\
m = (matrix_t*)allocator(sizeof(matrix_t) + sizeof(double)*r*c);\
m->row = r;\
m->col = c;\
m->val = (double*)(m + 1)

#define	matrix_assign(m, ...)\
{\
double _##m[] = {0,##__VA_ARGS__};\
double *_##p##m = _##m + 1;\
size_t _i##m;\
size_t _cnt##m = m->row * m->col;\
if (_cnt##m > sizeof(_##m) / sizeof(_##m[0]) - 1)\
_cnt##m = sizeof(_##m) / sizeof(_##m[0]) - 1;\
for (_i##m = 0; _i##m < _cnt##m; ++_i##m)\
m->val[_i##m] = _##p##m[_i##m];\
}
*/

#define	matrix_val(m, r, c)		((m)->val[(r) * (m)->col + (c)])

#ifdef	__cplusplus
extern "C" {
#endif

int matrix_size_equal(const matrix_t* m1, const matrix_t* m2);
int matrix_equal(const matrix_t* m1, const matrix_t* m2);
matrix_t* matrix_copy(matrix_t* dst, const matrix_t* src);
matrix_t* matrix_dup(const matrix_t* m);
void matrix_remove_row(matrix_t* m, unsigned int row);
void matrix_remove_col(matrix_t* m, unsigned int col);
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