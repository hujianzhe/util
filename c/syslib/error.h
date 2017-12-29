//
// Created by hujianzhe
//

#ifndef	UTIL_C_SYSLIB_ERROR_H
#define	UTIL_C_SYSLIB_ERROR_H

#include "platform_define.h"
#include <errno.h>

#ifdef _MSC_VER
	#pragma warning(disable:4091)/* avoid bug(dbghelp.h warning C4091: "typedef ") */
#endif
#if defined(_WIN32) || defined(_WIN64)
	#include <Dbghelp.h>
	#pragma comment(lib, "Dbghelp.lib")
#else
	#include <execinfo.h>
#endif
#ifdef _MSC_VER
	#pragma warning(default:4091)
#endif

#ifdef	__cplusplus
extern "C" {
#endif

int error_code(void);
void clear_error_code(void);
char* error_msg(int errnum, char* buf, size_t bufsize);

void error_set_handler(void(*handler)(const char*, unsigned int, size_t, size_t));
void error_call_handler(const char* file, unsigned int line, size_t wparam, size_t lparam);
#define	error_call_handler(wparam, lparam)	(error_call_handler)(__FILE__, __LINE__, (size_t)(wparam), (size_t)(lparam))

#ifdef	__cplusplus
}
#endif

#endif
