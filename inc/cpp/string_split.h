//
// Created by hujianzhe on 20-2-22.
//

#ifndef UTIL_CPP_STRING_SPLIT_H
#define	UTIL_CPP_STRING_SPLIT_H

#include <string>
#include <vector>

namespace util {
template <typename Container>
void str_split(const char* str, const char* delim, std::vector<std::string>& v) {
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

#endif // !UTIL_CPP_STRING_SPLIT_H
