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

struct matrix_t* matrix_copy(struct matrix_t* dst, const struct matrix_t* src) {
	unsigned int min_row = dst->row < src->row ? dst->row : src->row;
	unsigned int min_col = dst->col < src->col ? dst->col : src->col;
	unsigned int r, c;
	for (r = 0; r < min_row; ++r) {
		for (c = 0; c < min_col; ++r) {
			matrix_val(dst, r, c) = matrix_val(src, r, c);
		}
	}
	return dst;
}

struct matrix_t* matrix_dup(const struct matrix_t* m) {
	struct matrix_t* newm = (struct matrix_t*)malloc(sizeof(struct matrix_t) + sizeof(double) * m->row * m->col);
	if (newm) {
		unsigned int r, c;
		newm->val = (double*)(newm + 1);
		newm->row = m->row;
		newm->col = m->col;
		for (r = 0; r < m->row; ++r) {
			for (c = 0; c < m->col; ++c) {
				matrix_val(newm, r, c) = matrix_val(m, r, c);
			}
		}
	}
	return newm;
}

void matrix_remove_row(struct matrix_t* m, unsigned int row) {
	unsigned int i, j;

	if (m->row <= row || 1 == m->row)
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

	if (m->col <= col || 1 == m->col)
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

/*
static double __matrix_det(const struct matrix_t* src, struct matrix_t* m) {
	double res = 0.0;
	unsigned int c;
	for (c = 0; c < m->col; ++c) {
		double det;
		double base = matrix_val(m, 0, c);
		matrix_remove_col(m, c);
		matrix_remove_row(m, 0);
		det = __matrix_det(src, m);
		if (c & 1)
			res -= base * det;
		else
			res += base * det;
	}
	return res;
}
*/

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
			// not implement
			return 0.0;
		}
	}
}

#ifdef	__cplusplus
}
#endif