//
// Created by hujianzhe
//

#include "../../inc/sysapi/terminal.h"

#ifdef	__cplusplus
extern "C" {
#endif

#if	defined(_WIN32) || defined(_WIN64)
static DWORD __set_tty_flag(DWORD flag, DWORD mask, BOOL bval) {
	DWORD flag_mask;
	flag_mask = flag & mask;
	if (0 == flag_mask && !bval)
		return flag;
	if (mask == flag_mask && bval)
		return flag;
	if (bval)
		flag |= mask;
	else
		flag &= ~mask;
	return flag;
}
#else
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
static tcflag_t __set_tty_flag(tcflag_t flag, tcflag_t mask, int bval) {
	tcflag_t flag_mask;
	flag_mask = flag & mask;
	if (0 == flag_mask && !bval)
		return flag;
	if (mask == flag_mask && bval)
		return flag;
	if (bval)
		flag |= mask;
	else
		flag &= ~mask;
	return flag;
}
#endif

/***********************************************************************/

char* terminalName(char* buf, size_t buflen) {
#if defined(_WIN32) || defined(_WIN64)
	DWORD res = GetConsoleTitleA(buf, buflen);
	if (res) {
		buf[buflen - 1] = 0;
		if (res < buflen) {
			buf[res] = 0;
		}
		return buf;
	}
	return NULL;
#else
	int res = ttyname_r(STDIN_FILENO, buf, buflen);
	if (res) {
		errno = res;
		return NULL;
	}
	else {
		buf[buflen - 1] = 0;
		return buf;
	}
#endif
}

char* terminalOriginalName(char* buf, size_t buflen) {
#if defined(_WIN32) || defined(_WIN64)
	return GetConsoleOriginalTitleA(buf, buflen) ? buf : NULL;
#else
	if (buflen <= L_ctermid)
		return NULL;
	return ctermid(buf) ? buf : NULL;
#endif
}

FD_t terminalStdin(void) {
#if defined(_WIN32) || defined(_WIN64)
	HANDLE fd = GetStdHandle(STD_INPUT_HANDLE);
	return fd ? (FD_t)fd : (FD_t)INVALID_HANDLE_VALUE;
#else
	return isatty(STDIN_FILENO) ? STDIN_FILENO : INVALID_FD_HANDLE;
#endif
}

FD_t terminalStdout(void) {
#if defined(_WIN32) || defined(_WIN64)
	HANDLE fd = GetStdHandle(STD_OUTPUT_HANDLE);
	return fd ? (FD_t)fd : (FD_t)INVALID_HANDLE_VALUE;
#else
	return isatty(STDOUT_FILENO) ? STDOUT_FILENO : INVALID_FD_HANDLE;
#endif
}

FD_t terminalStderr(void) {
#if defined(_WIN32) || defined(_WIN64)
	HANDLE fd = GetStdHandle(STD_ERROR_HANDLE);
	return fd ? (FD_t)fd : (FD_t)INVALID_HANDLE_VALUE;
#else
	return isatty(STDERR_FILENO) ? STDERR_FILENO : INVALID_FD_HANDLE;
#endif
}

BOOL terminalSetUTF8(void) {
#if defined(_WIN32) || defined(_WIN64)
	return SetConsoleCP(CP_UTF8) && SetConsoleOutputCP(CP_UTF8);
#else
	return 1;
#endif
}

BOOL terminalFlushInput(FD_t fd) {
#if defined(_WIN32) || defined(_WIN64)
	return FlushConsoleInputBuffer((HANDLE)fd);
#else
	return tcflush(fd, TCIFLUSH) == 0;
#endif
}

BOOL terminalEnableEcho(FD_t fd, BOOL bval) {
#if defined(_WIN32) || defined(_WIN64)
	DWORD mode, new_mode;
	if (!GetConsoleMode((HANDLE)fd, &mode))
		return FALSE;
	new_mode = __set_tty_flag(mode, ENABLE_ECHO_INPUT, bval);
	if (new_mode != mode) {
		return SetConsoleMode((HANDLE)fd, new_mode);
	}
	return TRUE;
#else
	tcflag_t new_lflag;
	struct termios tm;
	if (tcgetattr(fd, &tm))
		return 0;
	new_lflag = __set_tty_flag(tm.c_lflag, ECHO, bval);
	if (new_lflag != tm.c_lflag) {
		tm.c_lflag = new_lflag;
		return tcsetattr(fd, TCSANOW, &tm) == 0;
	}
	return 1;
#endif
}

BOOL terminalEnableLineInput(FD_t fd, BOOL bval) {
#if defined(_WIN32) || defined(_WIN64)
	DWORD mode, new_mode;
	if (!GetConsoleMode((HANDLE)fd, &mode))
		return FALSE;
	new_mode = __set_tty_flag(mode, ENABLE_MOUSE_INPUT, !bval);
	new_mode = __set_tty_flag(new_mode, ENABLE_LINE_INPUT, bval);
	if (new_mode != mode) {
		return SetConsoleMode((HANDLE)fd, new_mode);
	}
	return TRUE;
#else
	tcflag_t new_lflag;
	struct termios tm;
	if (tcgetattr(fd, &tm))
		return 0;
	new_lflag = __set_tty_flag(tm.c_lflag, ICANON | ECHO | IEXTEN, bval);
	if (new_lflag != tm.c_lflag) {
		tm.c_lflag = new_lflag;
		return tcsetattr(fd, TCSANOW, &tm) == 0;
	}
	return 1;
#endif
}

BOOL terminalEnableSignal(FD_t fd, BOOL bval) {
#if defined(_WIN32) || defined(_WIN64)
	DWORD mode, new_mode;
	if (!GetConsoleMode((HANDLE)fd, &mode))
		return FALSE;
	new_mode = __set_tty_flag(mode, ENABLE_PROCESSED_INPUT, bval);
	if (new_mode != mode) {
		return SetConsoleMode((HANDLE)fd, new_mode);
	}
	return TRUE;
#else
	tcflag_t new_lflag;
	struct termios tm;
	if (tcgetattr(fd, &tm))
		return 0;
	new_lflag = __set_tty_flag(tm.c_lflag, ISIG, bval);
	if (new_lflag != tm.c_lflag) {
		tm.c_lflag = new_lflag;
		return tcsetattr(fd, TCSANOW, &tm) == 0;
	}
	return 1;
#endif
}

BOOL terminalGetPageSize(FD_t fd, int* x_col, int* y_row) {
#if defined(_WIN32) || defined(_WIN64)
	CONSOLE_SCREEN_BUFFER_INFO ws;
	if (!GetConsoleScreenBufferInfo((HANDLE)fd, &ws))
		return FALSE;
	*x_col = ws.srWindow.Right + 1;
	*y_row = ws.srWindow.Bottom + 1;
	return TRUE;
#else
	struct winsize ws;
	if (ioctl(fd, TIOCGWINSZ, (char*)&ws))
		return FALSE;
	*y_row = ws.ws_row;
	*x_col = ws.ws_col;
	return TRUE;
#endif
}

BOOL terminalSetCursorPos(FD_t fd, int x_col, int y_row) {
#if defined(_WIN32) || defined(_WIN64)
	COORD pos;
	pos.X = x_col;
	pos.Y = y_row;
	return SetConsoleCursorPosition((HANDLE)fd, pos);
#else
	char esc[11 + 11 + 4 + 1];
	int esc_len = sprintf(esc, "\033[%d;%dH", ++y_row, ++x_col);
	if (esc_len <= 0)
		return FALSE;
	esc[esc_len] = 0;
	return write(fd, esc, esc_len) == esc_len;
#endif
}

BOOL terminalShowCursor(FD_t fd, BOOL bval) {
#if defined(_WIN32) || defined(_WIN64)
	CONSOLE_CURSOR_INFO ci;
	ci.dwSize = 100;
	ci.bVisible = bval;
	return SetConsoleCursorInfo((HANDLE)fd, &ci);
#else
	if (bval) {
		char esc[] = "\033[?25h";
		return write(fd, esc, sizeof(esc) - 1) == sizeof(esc) - 1;
	}
	else {
		char esc[] = "\033[?25l";
		return write(fd, esc, sizeof(esc) - 1) == sizeof(esc) - 1;
	}
#endif
}

BOOL terminalClrscr(FD_t fd) {
#if defined(_WIN32) || defined(_WIN64)
	DWORD write_chars_num;
	COORD coord = { 0 };
	CONSOLE_SCREEN_BUFFER_INFO ws;
	if (!GetConsoleScreenBufferInfo((HANDLE)fd, &ws))
		return FALSE;
	write_chars_num = ws.dwSize.X * ws.dwSize.Y;
	return FillConsoleOutputCharacterA((HANDLE)fd, ' ', write_chars_num, coord, &write_chars_num);
#else
	char esc[] = "\033[2J";
	return write(fd, esc, sizeof(esc) - 1) == sizeof(esc) - 1;
#endif
}

#ifdef	__cplusplus
}
#endif
