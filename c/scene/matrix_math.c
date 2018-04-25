//
// Created by hujianzhe on 18-4-24.
//

#include "matrix_math.h"

#if defined(_WIN32) || defined(_WIN64)
#ifndef	_USE_MATH_DEFINES
	#define	_USE_MATH_DEFINES
#endif
#endif
#include <math.h>
#include <stdlib.h>

#ifdef	__cplusplus
extern "C" {
#endif

void matrix_remove_row(struct matrix_t* m, unsigned int row) {
	unsigned int i, j;

	if (m->row <= row)
		return;

	for (i = row; i < m->row && i + 1 < m->row; ++i) {
		for (j = 0; j < m->col; ++j) {
			m->val[i * m->col + j] = m->val[(i + 1) * m->col + j];
		}
	}
	m->row -= 1;
}

void matrix_remove_col(struct matrix_t* m, unsigned int col) {
	unsigned int i, j;

	if (m->col <= col)
		return;

	for (i = 0, j = 0; i < m->row * m->col; ++i) {
		if (i % m->col == col) {
			++j;
			continue;
		}
		m->val[i - j] = m->val[i];
	}

	m->col -= 1;
}

double matrix_det(const struct matrix_t* m) {
	if (m->row != m->col || !m->row)
		return NAN;
	switch (m->row) {
		case 1:
			return	matrix_val(m,0,0);
		case 2:
			return	matrix_val(m,0,0)*matrix_val(m,1,1) - matrix_val(m,0,1)*matrix_val(m,1,0);
		case 3:
			return	matrix_val(m,0,0)*matrix_val(m,1,1)*matrix_val(m,2,2) -
					matrix_val(m,0,0)*matrix_val(m,1,2)*matrix_val(m,2,1) -
					matrix_val(m,0,1)*matrix_val(m,1,0)*matrix_val(m,2,2) +
					matrix_val(m,0,1)*matrix_val(m,1,2)*matrix_val(m,2,0) +
					matrix_val(m,0,2)*matrix_val(m,1,0)*matrix_val(m,2,1) -
					matrix_val(m,0,2)*matrix_val(m,1,1)*matrix_val(m,2,0);
		default:
		{
			return 0.0;
		}
	}
}

#ifdef	__cplusplus
}
#endif