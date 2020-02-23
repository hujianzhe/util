//
// Created by hujianzhe on 20-2-22.
//

#ifndef UTIL_CPP_STRING_SPLIT_H
#define	UTIL_CPP_STRING_SPLIT_H

#ifdef __cplusplus

#include "cpp_compiler_define.h"
#include <string>
#include <vector>

namespace util {
void string_split(const char* str, const char* delim, std::vector<std::string>& v) {
	const char* p, *dp;
	for (p = str; *str; ++str) {
		for (dp = delim; *dp; ++dp) {
			if (*dp == *str) {
				v.push_back(std::string(p, str - p));
				p = str + 1;
				break;
			}
		}
	}
	if (str != p)
		v.push_back(std::string(p, str - p));
}
}

#endif

#endif // !UTIL_CPP_STRING_SPLIT_H
