//
// Created by hujianzhe on 20-2-22.
//

#ifndef UTIL_CPP_STRING_HELPER_H
#define	UTIL_CPP_STRING_HELPER_H

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <vector>

namespace util {
class string {
public:
	static void split(const char* str, int delim, std::vector<std::string>& v) {
		const char* p;
		for (p = str; *str; ++str) {
			if (*p == delim) {
				v.push_back(std::string(p, str - p));
				p = str + 1;
			}
		}
		if (str != p) {
			v.push_back(std::string(p, str - p));
		}
	}

	static void split(const char* str, const char* delim, std::vector<std::string>& v) {
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
		if (str != p) {
			v.push_back(std::string(p, str - p));
		}
	}

	static void splits(const char* str, const char* delim, std::vector<std::string>& v) {
		const char* p;
		size_t delim_len = strlen(delim);
		while ((p = strstr(str, delim))) {
			v.push_back(std::string(str, p - str));
			str = p + delim_len;
		}
		if (*str) {
			v.push_back(str);
		}
	}

	static std::string format(const char* format, ...) {
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
};
}

#endif // !UTIL_CPP_STRING_HELPER_H
