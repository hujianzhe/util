//
// Created by hujianzhe on 20-2-22.
//

#ifndef UTIL_CPP_STRING_SPLIT_H
#define	UTIL_CPP_STRING_SPLIT_H

#include "cpp_compiler_define.h"
#include <string>
#include <vector>

namespace util {
void string_split(const char* str, const char* delim, std::vector<std::string>& v);
void string_splits(const char* str, const char* delim, std::vector<std::string>& v);
std::string string_format(const char* format, ...);
/* MacOS clang++ already has std::to_string, so use util namespace */
std::string to_string(int value);
std::string to_string(long value);
std::string to_string(long long value);
std::string to_string(unsigned int value);
std::string to_string(unsigned long value);
std::string to_string(unsigned long long value);
std::string to_string(float value);
std::string to_string(double value);
std::string to_string(long double value);
std::string to_string(const char* value);
std::string to_string(const std::string& value);
}

#endif // !UTIL_CPP_STRING_SPLIT_H
