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

__declspec_dll char* terminalName(char* buf, size_t buflen);
__declspec_dll char* terminalOriginalName(char* buf, size_t buflen);
__declspec_dll FD_t terminalStdin(void);
__declspec_dll FD_t terminalStdout(void);
__declspec_dll FD_t terminalStderr(void);
__declspec_dll BOOL terminalSetUTF8(void);
__declspec_dll BOOL terminalFlushInput(FD_t fd);
__declspec_dll BOOL terminalEnableEcho(FD_t fd, BOOL bval);
__declspec_dll BOOL terminalEnableLineInput(FD_t fd, BOOL bval);
__declspec_dll BOOL terminalEnableSignal(FD_t fd, BOOL bval);
__declspec_dll BOOL terminalGetPageSize(FD_t fd, int* x_col, int* y_row);
__declspec_dll BOOL terminalSetCursorPos(FD_t fd, int x_col, int y_row);
__declspec_dll BOOL terminalShowCursor(FD_t fd, BOOL bval);
__declspec_dll BOOL terminalClrscr(FD_t fd);

#ifdef	__cplusplus
}
#endif

#endif // !UTIL_C_SYSLIB_TERMINAL_H
