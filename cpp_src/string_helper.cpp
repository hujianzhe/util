//
// Created by hujianzhe on 20-2-22.
//

#include "../cpp_inc/string_helper.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

void string_splits(const char* str, const char* delim, std::vector<std::string>& v) {
	const char* p;
	size_t delim_len = strlen(delim);
	while (p = strstr(str, delim)) {
		v.push_back(std::string(str, p - str));
		str = p + delim_len;
	}
	if (*str)
		v.push_back(str);
}

std::string string_format(const char* format, ...) {
	char test_buf;
	char* buf;
	int len;
	va_list varg;
	va_start(varg, format);
	len = vsnprintf(&test_buf, 0, format, varg);
	va_end(varg);
	if (len <= 0) {
		return std::string();
	}
	buf = (char*)malloc(len + 1);
	if (!buf) {
		return std::string();
	}
	va_start(varg, format);
	len = vsnprintf(buf, len + 1, format, varg);
	va_end(varg);
	if (len <= 0) {
		free(buf);
		return std::string();
	}
	std::string s(buf, len);
	free(buf);
	return s;
}
}
