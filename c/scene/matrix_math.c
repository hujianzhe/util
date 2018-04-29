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

int matrix_size_equal(const matrix_t* m1, const matrix_t* m2) {
	return m1->row == m2->row && m1->col == m2->col;
}

int matrix_equal(const matrix_t* m1, const matrix_t* m2) {
	unsigned int r, c;

	if (m1 == m2)
		return 1;

	if (!matrix_size_equal(m1, m2))
		return 0;

	for (r = 0; r < m1->row; ++r) {
		for (c = 0; c < m1->col; ++c) {
			if (matrix_val(m1, r, c) != matrix_val(m2, r, c))
				return 0;
		}
	}
	return 1;
}

matrix_t* matrix_copy(matrix_t* dst, const matrix_t* src) {
	unsigned int r, c;

	if (dst == src)
		return dst;

	if (!matrix_size_equal(dst, src))
		return NULL;

	for (r = 0; r < src->row; ++r) {
		for (c = 0; c < src->col; ++r) {
			matrix_val(dst, r, c) = matrix_val(src, r, c);
		}
	}
	return dst;
}

matrix_t* matrix_dup(const matrix_t* m) {
	matrix_t* newm = (matrix_t*)malloc(sizeof(matrix_t) + sizeof(double) * m->row * m->col);
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

void matrix_remove_row(matrix_t* m, unsigned int row) {
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

void matrix_remove_col(matrix_t* m, unsigned int col) {
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
static double __matrix_det(const matrix_t* src, matrix_t* m) {
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

double matrix_det(const matrix_t* m) {
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

matrix_t* matrix_identity(const matrix_t* m, matrix_t* mi) {
	unsigned int r, c;

	if (m->row != m->col || mi->row != mi->col)
		return NULL;

	for (r = 0; r < m->row; ++r) {
		for (c = 0; c < m->col; ++c) {
			matrix_val(mi, r, c) = (c == r);
		}
	}

	return mi;
}

matrix_t* matrix_add(matrix_t* res, const matrix_t* m1, const matrix_t* m2) {
	unsigned int r, c;
	
	if (!matrix_size_equal(m1, m2) || !matrix_size_equal(res, m1))
		return NULL;

	for (r = 0; r < m1->row; ++r) {
		for (c = 0; c < m1->col; ++c) {
			matrix_val(res, r, c) = matrix_val(m1, r, c) + matrix_val(m2, r, c);
		}
	}
	return res;
}

matrix_t* matrix_sub(matrix_t* res, const matrix_t* m1, const matrix_t* m2) {
	unsigned int r, c;

	if (!matrix_size_equal(m1, m2) || !matrix_size_equal(res, m1))
		return NULL;

	for (r = 0; r < m1->row; ++r) {
		for (c = 0; c < m1->col; ++c) {
			matrix_val(res, r, c) = matrix_val(m1, r, c) - matrix_val(m2, r, c);
		}
	}
	return res;
}

matrix_t* matrix_mulnum(matrix_t* m, double number) {
	unsigned int r, c;
	for (r = 0; r < m->row; ++r) {
		for (c = 0; c < m->col; ++c) {
			matrix_val(m, r, c) *= number;
		}
	}
	return m;
}

matrix_t* matrix_divnum(matrix_t* m, double number) {
	return matrix_mulnum(m, 1.0 / number);
}

matrix_t* matrix_mul(matrix_t* res, const matrix_t* left, const matrix_t* right) {
	unsigned int r, c;

	if (left->col != right->row || res->row != left->row || res->col != right->col || res == left || res == right)
		return NULL;

	for (r = 0; r < res->row; ++r) {
		for (c = 0; c < res->col; ++c) {
			double v = 0.0;
			unsigned int i;
			for (i = 0; i < left->col; ++i) {
				v += matrix_val(left, r, i) * matrix_val(right, i, c);
			}
			matrix_val(res, r, c) = v;
		}
	}

	return res;
}

#ifdef	__cplusplus
}
#endif