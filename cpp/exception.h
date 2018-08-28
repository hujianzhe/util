//
// Created by hujianzhe on 16-5-2.
//

#ifndef UTIL_CPP_EXCEPTION_H
#define UTIL_CPP_EXCEPTION_H

#include <stdio.h>
#include <exception>
#include <stdexcept>

#define	assert_throw(expression) {\
if (!(expression)) {\
const char* __exp_str = #expression;\
char _str[512];\
snprintf(_str, sizeof(_str), "%s(%d): %s\r\n", __FILE__, __LINE__, __exp_str);\
throw std::logic_error(_str);\
}\
}

#define	logic_throw(expression, fmt, ...) {\
if (!(expression)) {\
const char* __exp_str = #expression;\
char _str[512];\
snprintf(_str, sizeof(_str), "%s(%d): %s\r\n" fmt "\r\n", __FILE__, __LINE__, __exp_str, ##__VA_ARGS__);\
throw std::logic_error(_str);\
}\
}

#endif
