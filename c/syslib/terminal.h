//
// Created by hujianzhe
//

#ifndef UTIL_C_SYSLIB_TERMINAL_H
#define	UTIL_C_SYSLIB_TERMINAL_H

#include "../platform_define.h"

#if defined(_WIN32) || defined(_WIN64)
	#include <conio.h>
#else
	#include <termios.h>
#endif

#ifdef	__cplusplus
extern "C" {
#endif

UTIL_LIBAPI char* terminalName(char* buf, size_t buflen);
UTIL_LIBAPI int terminalKbhit(void);
UTIL_LIBAPI int terminalGetch(void);

#ifdef	__cplusplus
}
#endif

#endif // !UTIL_C_SYSLIB_TERMINAL_H
