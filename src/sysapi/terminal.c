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
static void __unix_set_tty(int ttyfd, struct termios* old, int min, int time) {
	struct termios newt;
	tcgetattr(ttyfd, old);
	newt = *old;
	newt.c_lflag &= ~ICANON;
	newt.c_lflag &= ~ECHO;
	newt.c_lflag &= ~ISIG;
	newt.c_cc[VMIN] = min;
	newt.c_cc[VTIME] = time;
	tcsetattr(ttyfd, TCSANOW, &newt);
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
	__unix_set_tty(STDIN_FILENO, &old, 0, 0);
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
	__unix_set_tty(STDIN_FILENO, &old, 1, 0);
	res = read(STDIN_FILENO, &c, 1);
	if (res >= 0) {
		res = (unsigned char)c;
	}
	tcsetattr(STDIN_FILENO, TCSANOW, &old);
	return c;
#endif
}

#ifdef	__cplusplus
}
#endif
