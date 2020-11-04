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
#include "vkeycode.h"

typedef struct DevKeyEvent_t {
	unsigned char keydown;
	unsigned int vkeycode;
#if defined(_WIN32) || defined(_WIN64)
	unsigned int charcode;
#else
	union {
		unsigned char __charbuf[8];
		unsigned int charcode;
	};
#endif
} DevKeyEvent_t;

#ifdef	__cplusplus
extern "C" {
#endif

__declspec_dll char* terminalName(char* buf, size_t buflen);
__declspec_dll char* terminalOriginalName(char* buf, size_t buflen);
__declspec_dll FD_t terminalStdin(void);
__declspec_dll FD_t terminalStdout(void);
__declspec_dll FD_t terminalStderr(void);
__declspec_dll BOOL terminalFlushInput(FD_t fd);
__declspec_dll BOOL terminalEnableEcho(FD_t fd, BOOL bval);
__declspec_dll BOOL terminalEnableLineInput(FD_t fd, BOOL bval);
__declspec_dll BOOL terminalEnableSignal(FD_t fd, BOOL bval);
__declspec_dll BOOL terminalGetPageSize(FD_t fd, int* x_col, int* y_row);
__declspec_dll BOOL terminalSetCursorPos(FD_t fd, int x_col, int y_row);
__declspec_dll BOOL terminalShowCursor(FD_t fd, BOOL bval);
__declspec_dll BOOL terminalClrscr(FD_t fd);
__declspec_dll BOOL terminalKeyDevScan(void);
__declspec_dll BOOL terminalReadKey(FD_t fd, DevKeyEvent_t* e);
__declspec_dll BOOL terminalReadKeyNonBlock(FD_t fd, DevKeyEvent_t* e);

#ifdef	__cplusplus
}
#endif

#endif // !UTIL_C_SYSLIB_TERMINAL_H
