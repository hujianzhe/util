//
// Created by hujianzhe on 16-8-29.
//

#include "syslib/error.h"
#include "exception.h"
#include <stdio.h>
#include <string.h>

namespace Util {
void exception::no_memory(void) {
	fputs("memory allocate over\n", stderr);
	exit(1);
}

exception::exception(const std::string& text) :
	std::logic_error(text)
{
}
exception::exception(const char* text, size_t len) :
	std::logic_error(std::string(text, len ? len :strlen(text)))
{
}
exception::exception(const char* fname, unsigned int line, const char* text, bool cr) :
	std::logic_error(getText(fname, line, text, cr))
{
}
exception::exception(const char* fname, unsigned int line, const std::string& text, bool cr) :
	std::logic_error(getText(fname, line, text, cr))
{
}

std::string exception::getText(const char* fname, unsigned int line, const char* text, bool cr) {
	char format[] = "%s:%u: %s(%d)";
	char cr_format[] = "%s:%u: %s(%d)\n";

	char str[256];
	snprintf(str, sizeof(str), cr ? cr_format : format, fname, line, text ? text : "", error_code());
	return str;
}
std::string exception::getText(const char* fname, unsigned int line, const std::string& text, bool cr) {
	char format[] = "%s:%u: %s(%d)";
	char cr_format[] = "%s:%u: %s(%d)\n";

	char str[256];
	snprintf(str, sizeof(str), cr ? cr_format : format, fname, line, text.c_str(), error_code());
	return str;
}
}
