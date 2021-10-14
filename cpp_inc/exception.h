//
// Created by hujianzhe on 16-5-2.
//

#ifndef UTIL_CPP_EXCEPTION_H
#define UTIL_CPP_EXCEPTION_H

#ifdef __cplusplus

#include "cpp_compiler_define.h"
#include <stdio.h>
#include <exception>
#include <stdexcept>

#define	assert_throw(exp)\
if (!(exp)) {\
throw std::logic_error(__FILE__"(" MACRO_TOSTRING(__LINE__) "): " #exp "\r\n");\
}

#define	logic_throw(exp, fmt, ...)\
if (!(exp)) {\
char _str[512];\
snprintf(_str, sizeof(_str), __FILE__"(" MACRO_TOSTRING(__LINE__) "): %s\r\n" fmt "\r\n", #exp, ##__VA_ARGS__);\
throw std::logic_error(_str);\
}

#endif

#endif
