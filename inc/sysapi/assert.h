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
fputs(__FILE__"(" MACRO_TOSTRING(__LINE__) "): " #exp "\r\n", stderr);\
abort();\
}

#endif
