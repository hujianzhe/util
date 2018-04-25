//
// Created by hujianzhe on 18-4-24.
//

#ifndef UTIL_C_SCENE_MATRIX_MATH_H
#define	UTIL_C_SCENE_MATRIX_MATH_H

typedef struct matrix_t {
	unsigned int row, col;
	double* val;
} matrix_t;

#define	matrix_val(m, r, c)	((m)->val[(r) * (m)->col + (c)])

#ifdef	__cplusplus
extern "C" {
#endif

struct matrix_t* matrix_copy(struct matrix_t* dst, const struct matrix_t* src);
struct matrix_t* matrix_dup(const struct matrix_t* m);
void matrix_remove_row(struct matrix_t* m, unsigned int row);
void matrix_remove_col(struct matrix_t* m, unsigned int col);
double matrix_det(const struct matrix_t* m);
struct matrix_t* matrix_mulnum(struct matrix_t* m, double number);

#ifdef	__cplusplus
}
#endif

#endif // !UTIL_C_SCENE_MATRIX_MATH_H