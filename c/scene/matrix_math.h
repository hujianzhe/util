//
// Created by hujianzhe on 18-4-24.
//

#ifndef UTIL_C_SCENE_MATRIX_MATH_H
#define	UTIL_C_SCENE_MATRIX_MATH_H

#include <stdlib.h>
#if defined(_WIN32) || defined(_WIN64)
	#include <malloc.h>
	#ifndef alloca
		#define alloca		_alloca
	#endif
#else
	#if	__linux__
		#include <alloca.h>
	#endif
#endif

typedef struct matrix_t {
	unsigned int row, col;
	double* val;
} matrix_t;

#define	matrix_make(m, r, c, allocator)\
m = (struct matrix_t*)(allocator)(sizeof(struct matrix_t) + sizeof(double)*r*c);\
m->row = r;\
m->col = c;\
m->val = (double*)(m + 1)

#define	matrix_assign(m, array_init_expression)\
{\
double m##_[] = array_init_expression;\
size_t m##iter;\
size_t m##cnt = m->row * m->col;\
if (m##cnt > sizeof(m##_) / sizeof(m##_[0]))\
m##cnt = sizeof(m##_) / sizeof(m##_[0]);\
for (m##iter = 0; m##iter < m##cnt; ++m##iter)\
m->val[m##iter] = m##_[m##iter];\
}

#define	matrix_val(m, r, c)		((m)->val[(r) * (m)->col + (c)])

#ifdef	__cplusplus
extern "C" {
#endif

int matrix_size_equal(const struct matrix_t* m1, const struct matrix_t* m2);
int matrix_equal(const struct matrix_t* m1, const struct matrix_t* m2);
struct matrix_t* matrix_copy(struct matrix_t* dst, const struct matrix_t* src);
struct matrix_t* matrix_dup(const struct matrix_t* m);
void matrix_remove_row(struct matrix_t* m, unsigned int row);
void matrix_remove_col(struct matrix_t* m, unsigned int col);
double matrix_det(const struct matrix_t* m);
struct matrix_t* matrix_add(struct matrix_t* res, const struct matrix_t* m1, const struct matrix_t* m2);
struct matrix_t* matrix_sub(struct matrix_t* res, const struct matrix_t* m1, const struct matrix_t* m2);
struct matrix_t* matrix_mulnum(struct matrix_t* m, double number);
struct matrix_t* matrix_divnum(struct matrix_t* m, double number);
struct matrix_t* matrix_mul(struct matrix_t* res, const struct matrix_t* left, const struct matrix_t* right);

#ifdef	__cplusplus
}
#endif

#endif // !UTIL_C_SCENE_MATRIX_MATH_H