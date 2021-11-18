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
}

#endif // !UTIL_CPP_STRING_SPLIT_H
