//
// Created by hujianzhe on 16-5-2.
//

#include "string_util.h"
#include <string.h>

namespace Util {
namespace string {
std::string join(const char c, const std::vector<std::string>& str) {
	return str.empty() ? "" : join(c, &str[0], str.size());
}

std::string join(const char c, const std::list<std::string>& str) {
	std::string res;
	for (std::list<std::string>::const_iterator it = str.begin();
			it != str.end(); ++it) {
		res += *it;
		res += c;
	}
	if (!res.empty()) {
		res = res.substr(0, res.size() - 1);
	}
	return res;
}
std::string join(const char c, const std::string str[], size_t cnt) {
	std::string res;
	for (size_t i = 0; i < cnt; ++i) {
		res += str[i];
		res += c;
	}
	if (!res.empty()) {
		res = res.substr(0, res.size() - 1);
	}
	return res;
}
std::vector<std::string> split(const std::string& str, const char c) {
	std::vector<std::string> res;
	std::string temp;
	for (size_t i = 0; i < str.size(); ++i) {
		if (str[i] == c) {
			res.resize(res.size() + 1);
			res[res.size() - 1] = temp;
			temp.clear();
		}
		else {
			temp += str[i];
		}
	}
	if (!temp.empty()) {
		res.resize(res.size() + 1);
		res[res.size() - 1] = temp;
	}
	return res;
}
std::vector<std::string> split(const char* str, const char c) {
	std::vector<std::string> res;
	if (!str) {
		return res;
	}
	std::string temp;
	for (size_t i = 0; str[i]; ++i) {
		if (str[i] == c) {
			res.resize(res.size() + 1);
			res[res.size() - 1] = temp;
			temp.clear();
		}
		else {
			temp += str[i];
		}
	}
	if (!temp.empty()) {
		res.resize(res.size() + 1);
		res[res.size() - 1] = temp;
	}
	return res;
}
std::vector<std::string> split(const char* str, const char* s) {
	std::vector<std::string> res;
	size_t len = s ? strlen(s) : 0;
	const char* p;
	while ((p = strstr(str, s))) {
		res.push_back(std::string(str, p - str));
		p += len;
		str = p;
	}
	if (str && *str) {
		res.push_back(str);
	}
	return res;
}
std::string trim(const std::string& str) {
	std::string res;
	size_t start = 0, end = str.size();
	bool start_find = false, end_find = false;
	while (start < end && !start_find && !end_find) {
		if (!start_find) {
			switch (str[start]) {
				case ' ':
				case '\r':
				case '\n':
				case '\v':
				case '\f':
				case '\b':
				case '\t':
					++start;
					break;
				default:
					start_find = true;
			}
		}
		if (!end_find) {
			switch (str[end - 1]) {
				case ' ':
				case '\r':
				case '\n':
				case '\v':
				case '\f':
				case '\b':
				case '\t':
					--end;
					break;
				default:
					end_find = true;
			}
		}
	}
	while (start < end) {
		res.push_back(str[start++]);
	}
	return res;
}
}
}
