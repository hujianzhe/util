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

__declspec_dll FD_t terminalStdin(void);
__declspec_dll FD_t terminalStdout(void);
__declspec_dll FD_t terminalStderr(void);
__declspec_dll char* terminalName(char* buf, size_t buflen);
__declspec_dll int terminalKbhit(void);
__declspec_dll int terminalGetch(void);

#ifdef	__cplusplus
}
#endif

#endif // !UTIL_C_SYSLIB_TERMINAL_H
