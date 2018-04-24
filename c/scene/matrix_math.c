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

#ifdef	__cplusplus
extern "C" {
#endif

double matrix_det(const struct matrix_t* m) {
	//return __matrix_det(m, 0, 0, 0);
	return 0.0;
}

#ifdef	__cplusplus
}
#endif