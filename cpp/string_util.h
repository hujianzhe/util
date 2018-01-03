//
// Created by hujianzhe on 16-5-2.
//

#ifndef UTIL_CPP_STRING_H
#define UTIL_CPP_STRING_H

#include <list>
#include <string>
#include <vector>

namespace Util {
namespace String {
	std::string join(const char c, const std::vector<std::string>& str);
	std::string join(const char c, const std::list<std::string>& str);
	std::string join(const char c, const std::string str[], size_t cnt);
	std::vector<std::string> split(const std::string& str, const char c);
	std::vector<std::string> split(const char* str, const char c);
	std::vector<std::string> split(const char* str, const char* s);
	std::string trim(const std::string& str);
}
}

#endif //LIBC_UTIL_H
