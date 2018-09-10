//
// Created by hujianzhe
//

#ifndef UTIL_C_SYSLIB_STRING_H
#define UTIL_C_SYSLIB_STRING_H

#include "../platform_define.h"
#include <string.h>

#ifdef	__cplusplus
extern "C" {
#endif

#if !defined(__FreeBSD__) && !defined(__APPLE__)
UTIL_LIBAPI char *strnchr(const char* s, const char c, size_t n);
UTIL_LIBAPI char *strnstr(const char* s1, const char* s2, size_t n);
UTIL_LIBAPI char* strlcpy(char* dst, const char* src, size_t size);
#endif
#if defined(_WIN32) || defined(_WIN64)
	#define strcasecmp(s1, s2)		stricmp(s1, s2)
	#define	strncasecmp(s1, s2, n)	strnicmp(s1, s2, n)
	UTIL_LIBAPI char* strndup(const char* s, size_t n);
#else
	#define	stricmp(s1, s2)			strcasecmp(s1, s2)
	#define strnicmp(s1, s2, n)		strncasecmp(s1, s2, n)
#endif
UTIL_LIBAPI void strTrim(const char* str, size_t len, char** newstr, size_t* newlen);
UTIL_LIBAPI size_t strlenUTF8(const char* s);
UTIL_LIBAPI char* strCopy(char* dst, size_t dst_len, const char* src, size_t src_len);

#ifdef	__cplusplus
}
#endif

#endif
