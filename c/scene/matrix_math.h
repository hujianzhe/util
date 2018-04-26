//
// Created by hujianzhe on 18-4-24.
//

#ifndef UTIL_C_SCENE_MATRIX_MATH_H
#define	UTIL_C_SCENE_MATRIX_MATH_H

typedef struct matrix_t {
	unsigned int row, col;
	double* val;
} matrix_t;

#define	matrix_make(m, r, c, allocator)\
m = (struct matrix_t*)allocator(sizeof(struct matrix_t) + sizeof(double)*r*c);\
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
struct matrix_t* matrix_identity(const struct matrix_t* m, struct matrix_t* mi);
struct matrix_t* matrix_add(struct matrix_t* res, const struct matrix_t* m1, const struct matrix_t* m2);
struct matrix_t* matrix_sub(struct matrix_t* res, const struct matrix_t* m1, const struct matrix_t* m2);
struct matrix_t* matrix_mulnum(struct matrix_t* m, double number);
struct matrix_t* matrix_divnum(struct matrix_t* m, double number);
struct matrix_t* matrix_mul(struct matrix_t* res, const struct matrix_t* left, const struct matrix_t* right);

#ifdef	__cplusplus
}
#endif

#endif // !UTIL_C_SCENE_MATRIX_MATH_H