//
// Created by hujianzhe
//

#ifndef UTIL_C_SYSLIB_TERMINAL_H
#define	UTIL_C_SYSLIB_TERMINAL_H

#include "platform_define.h"

#if defined(_WIN32) || defined(_WIN64)
	#include <conio.h>
#else
	#include <termios.h>
#endif

#ifdef	__cplusplus
extern "C" {
#endif

int t_Kbhit(void);
int t_Getch(void);

#ifdef	__cplusplus
}
#endif

#endif // !UTIL_C_SYSLIB_TERMINAL_H
