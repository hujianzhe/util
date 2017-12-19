//
// Created by hujianzhe
//

#include "terminal.h"

#ifdef	__cplusplus
extern "C" {
#endif

#if	defined(_WIN32) || defined(_WIN64)
#else
static int __unix_set_tty(int ttyfd, struct termios* old, int min, int time) {
	tcgetattr(ttyfd, old);
	struct termios newt = *old;
	newt.c_lflag &= ~ICANON;
	newt.c_lflag &= ~ECHO;
	newt.c_lflag &= ~ISIG;
	newt.c_cc[VMIN] = min;
	newt.c_cc[VTIME] = time;
	tcsetattr(ttyfd, TCSANOW, &newt);
	return 0;
}
#endif

int t_Kbhit(void) {
#if defined(_WIN32) || defined(_WIN64)
	return _kbhit();
#else
	int res = -1;
	char c;
	struct termios old;
	__unix_set_tty(STDIN_FILENO, &old, 0, 0);
	res = read(STDIN_FILENO, &c, 1);
	if (res >= 0) {
		res = (unsigned char)c;
	}
	tcsetattr(STDIN_FILENO, TCSANOW, &old);
	return res;
#endif
}

int t_Getch(void) {
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
