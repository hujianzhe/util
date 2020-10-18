//
// Created by hujianzhe
//

#include "../../inc/sysapi/terminal.h"

#ifdef	__cplusplus
extern "C" {
#endif

#if	defined(_WIN32) || defined(_WIN64)
static BOOL __set_tty_mode(HANDLE fd, DWORD mask, BOOL bval) {
	DWORD mode, mode_mask;
	if (!GetConsoleMode(fd, &mode))
		return FALSE;
	mode_mask = mode & mask;
	if (0 == mode_mask && !bval)
		return TRUE;
	if (mask == mode_mask && bval)
		return TRUE;
	if (bval)
		mode |= mask;
	else
		mode &= ~mask;
	return SetConsoleMode(fd, mode);
}
#else
#include <sys/ioctl.h>
#include <errno.h>
#include <stdio.h>
static int __set_tty_mode(int fd, tcflag_t mask, int bval) {
	tcflag_t lflag_mask;
	struct termios tm;
	if (tcgetattr(fd, &tm))
		return 0;
	lflag_mask = tm.c_lflag & mask;
	if (0 == lflag_mask && !bval)
		return 1;
	if (mask == lflag_mask && bval)
		return 1;
	if (bval)
		tm.c_lflag |= mask;
	else
		tm.c_lflag &= ~mask;
	return tcsetattr(fd, TCSANOW, &tm) == 0;
}

static void __set_tty_no_canon_echo_isig(int ttyfd, struct termios* old, int min, int time) {
	struct termios newtc;
	tcgetattr(ttyfd, old);
	if (0 == (old->c_lflag & (ICANON|ECHO|ISIG)) &&
		old->c_cc[VMIN] == min && old->c_cc[VTIME] == time)
	{
		return;
	}
	newtc = *old;
	newtc.c_lflag &= ~(ICANON|ECHO|ISIG);
	newtc.c_cc[VMIN] = min;
	newtc.c_cc[VTIME] = time;
	tcsetattr(ttyfd, TCSANOW, &newtc);
}
#endif

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

BOOL terminalFlushInput(FD_t fd) {
#if defined(_WIN32) || defined(_WIN64)
	return FlushConsoleInputBuffer((HANDLE)fd);
#else
	return tcflush(fd, TCIFLUSH) == 0;
#endif
}

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
	/*ctermid(buf);*/
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

int terminalKbhit(void) {
#if defined(_WIN32) || defined(_WIN64)
	return _kbhit();
#else
	int res = -1;
	struct termios old;
	__set_tty_no_canon_echo_isig(STDIN_FILENO, &old, 0, 0);
	ioctl(STDIN_FILENO, FIONREAD, &res);
	tcsetattr(STDIN_FILENO, TCSANOW, &old);
	return res > 0;
#endif
}

int terminalGetch(void) {
#if defined(_WIN32) || defined(_WIN64)
	return _getch();
#else
	int res = -1;
	char c;
	struct termios old;
	__set_tty_no_canon_echo_isig(STDIN_FILENO, &old, 1, 0);
	res = read(STDIN_FILENO, &c, 1);
	if (res >= 0) {
		res = (unsigned char)c;
	}
	tcsetattr(STDIN_FILENO, TCSANOW, &old);
	return c;
#endif
}

BOOL terminalEnableEcho(FD_t fd, BOOL bval) {
#if defined(_WIN32) || defined(_WIN64)
	return __set_tty_mode((HANDLE)fd, ENABLE_ECHO_INPUT, bval);
#else
	return __set_tty_mode(fd, ECHO, bval);
#endif
}

BOOL terminalEnableLineInput(FD_t fd, BOOL bval) {
#if defined(_WIN32) || defined(_WIN64)
	return __set_tty_mode((HANDLE)fd, ENABLE_LINE_INPUT, bval);
#else
	return __set_tty_mode(fd, ICANON | ECHO, bval);
#endif
}

BOOL terminalGetPageSize(FD_t fd, int* x_col, int* y_row) {
#if defined(_WIN32) || defined(_WIN64)
	CONSOLE_SCREEN_BUFFER_INFO ws;
	if (!GetConsoleScreenBufferInfo((HANDLE)fd, &ws))
		return FALSE;
	*y_row = ws.srWindow.Right + 1;
	*x_col = ws.srWindow.Bottom + 1;
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

int terminalReadKey(FD_t fd) {
#if defined(_WIN32) || defined(_WIN64)
	while (1) {
		DWORD n;
		INPUT_RECORD ir;
		if (!ReadConsoleInput((HANDLE)fd, &ir, 1, &n) || 0 == n)
			return 0;
		if (KEY_EVENT != ir.EventType)
			continue;
		if (ir.Event.KeyEvent.bKeyDown) {
			int k = ir.Event.KeyEvent.uChar.UnicodeChar;
			if (!k)
				k = ir.Event.KeyEvent.wVirtualScanCode;
			return k;
		}
	}
#else
	int k = 0;
	if (read(fd, &k, sizeof(k)) < 0)
		return -1;
	return k;
#endif
}

#ifdef	__cplusplus
}
#endif
