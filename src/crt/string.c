//
// Created by hujianzhe on 22-10-14
//

#include "../../inc/crt/string.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

int strFormatLen(const char* format, ...) {
	char test_buf;
	int len;
	va_list varg;
	va_start(varg, format);
	len = vsnprintf(&test_buf, 0, format, varg);
	va_end(varg);
	return len;
}

char* strFormat(int* out_len, const char* format, ...) {
	char test_buf;
	char* buf;
	int len;
	va_list varg;
	va_start(varg, format);
	len = vsnprintf(&test_buf, 0, format, varg);
	va_end(varg);
	if (len < 0) {
		return NULL;
	}
	buf = (char*)malloc(len + 1);
	if (!buf) {
		return NULL;
	}
	va_start(varg, format);
	len = vsnprintf(buf, len + 1, format, varg);
	va_end(varg);
	if (len < 0) {
		free(buf);
		return NULL;
	}
	if (out_len) {
		*out_len = len;
	}
	buf[len] = 0;
	return buf;
}

void strFreeMemory(char* s) { free(s); }

#ifdef __cplusplus
}
#endif