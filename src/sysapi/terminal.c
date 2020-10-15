//
// Created by hujianzhe
//

#include "../../inc/sysapi/terminal.h"

#ifdef	__cplusplus
extern "C" {
#endif

#if	defined(_WIN32) || defined(_WIN64)
#else
#include <sys/ioctl.h>
#include <errno.h>
static void __set_tty_no_canon_echo_isig(int ttyfd, struct termios* old, int min, int time) {
	struct termios new;
	tcgetattr(ttyfd, old);
	if (0 == (old->c_lflag & (ICANON|ECHO|ISIG)) &&
		old->c_cc[VMIN] == min && old->c_cc[VTIME] == time)
	{
		return;
	}
	new = *old;
	new.c_lflag &= ~(ICANON|ECHO|ISIG);
	new.c_cc[VMIN] = min;
	new.c_cc[VTIME] = time;
	tcsetattr(ttyfd, TCSANOW, &new);
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
	DWORD mode;
	if (!GetConsoleMode((HANDLE)fd, &mode))
		return FALSE;
	if (0 == (mode & ENABLE_ECHO_INPUT) && !bval) {
		return TRUE;
	}
	else if ((mode & ENABLE_ECHO_INPUT) && bval) {
		return TRUE;
	}
	else if (bval)
		mode |= ENABLE_ECHO_INPUT;
	else
		mode &= ~ENABLE_ECHO_INPUT;
	return SetConsoleMode((HANDLE)fd, mode);
#else
	struct termios tm;
	if (tcgetattr(fd, &tm))
		return 0;
	if (0 == (tm.c_lflag & ECHO) && !bval) {
		return 1;
	}
	else if ((tm.c_lflag & ECHO) && bval) {
		return 1;
	}
	else if (bval)
		tm.c_lflag |= ECHO;
	else
		tm.c_lflag &= ~ECHO;
	return tcsetattr(fd, TCSANOW, &tm) == 0;
#endif
}

#ifdef	__cplusplus
}
#endif
