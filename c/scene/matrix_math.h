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

void matrix_remove_row(struct matrix_t* m, unsigned int row);
void matrix_remove_col(struct matrix_t* m, unsigned int col);
double matrix_det(const struct matrix_t* m);

#ifdef	__cplusplus
}
#endif

#endif // !UTIL_C_SCENE_MATRIX_MATH_H