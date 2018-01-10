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
char _str[512];\
snprintf(_str, sizeof(_str), "%s(%d): " #expression "\n", __FILE__, __LINE__); \
throw std::logic_error(_str);\
}\
}

#define	logic_throw(expression, fmt, ...) {\
if (!(expression)) {\
char _str[512];\
snprintf(_str, sizeof(_str), "%s(%d): " #expression "\n" fmt "\n", __FILE__, __LINE__, __VA_ARGS__);\
throw std::logic_error(_str);\
}\
}

#endif
