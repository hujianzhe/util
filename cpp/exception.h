//
// Created by hujianzhe on 16-5-2.
//

#ifndef UTIL_CPP_EXCEPTION_H
#define UTIL_CPP_EXCEPTION_H

#include <exception>
#include <stdexcept>
#include <string>

#define	assert_throw(expression) { if (!(expression)) { throw Util::exception(__FILE__, __LINE__, #expression, true); } }
#define	logic_throw(expression, text) { if (!(expression)) { throw Util::exception(__FILE__, __LINE__, text, true); } }

namespace Util {
class exception : public std::logic_error {
public:
	static void no_memory(void);

public:
	exception(const std::string& text);
	exception(const char* text, size_t len = 0);
	exception(const char* fname, unsigned int line, const char* text, bool cr = false);
	exception(const char* fname, unsigned int line, const std::string& text, bool cr = false);

private:
	std::string getText(const char* fname, unsigned int line, const char* text, bool cr);
	std::string getText(const char* fname, unsigned int line, const std::string& text, bool cr);
};
}

#endif
