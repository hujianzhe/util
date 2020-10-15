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

BOOL terminalGetRowColSize(FD_t fd, int* row, int* col) {
#if defined(_WIN32) || defined(_WIN64)
	CONSOLE_SCREEN_BUFFER_INFO ws;
	if (!GetConsoleScreenBufferInfo((HANDLE)fd, &ws))
		return FALSE;
	*row = ws.srWindow.Right + 1;
	*col = ws.srWindow.Bottom + 1;
	return TRUE;
#else
	struct winsize ws;
	if (ioctl(fd, TIOCGWINSZ, (char*)&ws))
		return FALSE;
	*row = ws.ws_row;
	*col = ws.ws_col;
	return TRUE;
#endif
}

#ifdef	__cplusplus
}
#endif
