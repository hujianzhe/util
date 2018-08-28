//
// Created by hujianzhe
//

#ifndef UTIL_C_SYSLIB_ASSERT_H
#define	UTIL_C_SYSLIB_ASSERT_H

#include "../platform_define.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#define	assertTRUE(exp)\
if (!(exp)) {\
const char* __exp_str = #exp;\
fprintf(stderr, "%s(%d):%s\n", __FILE__, __LINE__, __exp_str);\
abort();\
}

#endif
