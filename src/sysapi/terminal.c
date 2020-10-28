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

static BOOL __fill_mouse_event(DevKeyEvent_t* e, DWORD button_state) {
	static int mk_sts[3];
	e->charcode = 0;
	switch (button_state) {
		case 0:
			if (mk_sts[0]) {
				mk_sts[0] = 0;
				e->vkeycode = VK_LBUTTON;
			}
			else if (mk_sts[1]) {
				mk_sts[1] = 0;
				e->vkeycode = VK_RBUTTON;
			}
			else if (mk_sts[2]) {
				mk_sts[2] = 0;
				e->vkeycode = VK_MBUTTON;
			}
			else {
				return FALSE;
			}
			e->keydown = 0;
			break;
		case FROM_LEFT_1ST_BUTTON_PRESSED:
			mk_sts[0] = 1;
			e->keydown = 1;
			e->vkeycode = VK_LBUTTON;
			break;
		case RIGHTMOST_BUTTON_PRESSED:
			mk_sts[1] = 1;
			e->keydown = 1;
			e->vkeycode = VK_RBUTTON;
			break;
		case FROM_LEFT_2ND_BUTTON_PRESSED:
			mk_sts[2] = 1;
			e->keydown = 1;
			e->vkeycode = VK_MBUTTON;
			break;
		default:
			return FALSE;
	}
	return TRUE;
}
#else
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#ifdef	__linux__
#include <poll.h>
#include <linux/input.h>

static int vkey2char(int vkey, int shift_press, int capslock) {
	int alphabet_upper;
	if (!capslock && shift_press)
		alphabet_upper = 1;
	else if (capslock && !shift_press)
		alphabet_upper = 1;
	else
		alphabet_upper = 0;
	switch (vkey) {
		case KEY_0: return shift_press ? ')' : '0';
		case KEY_1: return shift_press ? '!' : '1';
		case KEY_2: return shift_press ? '@' : '2';
		case KEY_3: return shift_press ? '#' : '3';
		case KEY_4: return shift_press ? '$' : '4';
		case KEY_5: return shift_press ? '%' : '5';
		case KEY_6: return shift_press ? '^' : '6';
		case KEY_7: return shift_press ? '&' : '7';
		case KEY_8: return shift_press ? '*' : '8';
		case KEY_9: return shift_press ? '(' : '9';
		case KEY_A: return alphabet_upper ? 'A' : 'a';
		case KEY_B: return alphabet_upper ? 'B' : 'b';
		case KEY_C: return alphabet_upper ? 'C' : 'c';
		case KEY_D: return alphabet_upper ? 'D' : 'd';
		case KEY_E: return alphabet_upper ? 'E' : 'e';
		case KEY_F: return alphabet_upper ? 'F' : 'f';
		case KEY_G: return alphabet_upper ? 'G' : 'g';
		case KEY_H: return alphabet_upper ? 'H' : 'h';
		case KEY_I: return alphabet_upper ? 'I' : 'i';
		case KEY_J: return alphabet_upper ? 'J' : 'j';
		case KEY_K: return alphabet_upper ? 'K' : 'k';
		case KEY_L: return alphabet_upper ? 'L' : 'l';
		case KEY_M: return alphabet_upper ? 'M' : 'm';
		case KEY_N: return alphabet_upper ? 'N' : 'n';
		case KEY_O: return alphabet_upper ? 'O' : 'o';
		case KEY_P: return alphabet_upper ? 'P' : 'p';
		case KEY_Q: return alphabet_upper ? 'Q' : 'q';
		case KEY_R: return alphabet_upper ? 'R' : 'r';
		case KEY_S: return alphabet_upper ? 'S' : 's';
		case KEY_T: return alphabet_upper ? 'T' : 't';
		case KEY_U: return alphabet_upper ? 'U' : 'u';
		case KEY_V: return alphabet_upper ? 'V' : 'v';
		case KEY_W: return alphabet_upper ? 'W' : 'w';
		case KEY_X: return alphabet_upper ? 'X' : 'x';
		case KEY_Y: return alphabet_upper ? 'Y' : 'y';
		case KEY_Z: return alphabet_upper ? 'Z' : 'z';
		case KEY_GRAVE: return shift_press ? '~' : '`';
		case KEY_EQUAL: return shift_press ? '+' : '=';
		case KEY_MINUS: return shift_press ? '_' : '-';
		case KEY_LEFTBRACE: return shift_press ? '{' : '[';
		case KEY_RIGHTBRACE: return shift_press ? '}' : ']';
		case KEY_BACKSLASH: return shift_press ? '|' : '\\';
		case KEY_SEMICOLON: return shift_press ? ':' : ';';
		case KEY_APOSTROPHE: return shift_press ? '"' : '\'';
		case KEY_COMMA: return shift_press ? '<' : ',';
		case KEY_DOT: return shift_press ? '>' : '.';
		case KEY_SLASH: return shift_press ? '?' : '/';
		case KEY_SPACE: return ' ';
		case KEY_TAB: return '\t';
		case KEY_ENTER: return '\n';
		case KEY_BACKSPACE: return 8;
		case KEY_ESC: return 27;
	}
	return 0;
}

typedef struct DevFdSet_t {
	nfds_t nfds;
	struct pollfd fds[1];
} DevFdSet_t;
static DevFdSet_t* s_DevFdSet;

static void free_dev_fd_set(DevFdSet_t* ds) {
    if (ds) {
        nfds_t i;
        for (i = 0; i < ds->nfds; ++i) {
            close(ds->fds[i].fd);
        }
        free(ds);
    }
}

static DevFdSet_t* scan_dev_fd_set(DevFdSet_t* ds) {
    DIR* dir;
    struct dirent* dir_item;
	nfds_t nfds;

    dir = opendir("/dev/input");
    if (!dir) {
		free_dev_fd_set(ds);
        return NULL;
    }
	nfds = 0;
    while (dir_item = readdir(dir)) {
        int fd, i;
        long evs[EV_MAX/(sizeof(long)*8) + 1] = { 0 };
        char str_data[32];

        if (!strstr(dir_item->d_name, "event"))
            continue;
        i = sprintf(str_data, "/dev/input/%s", dir_item->d_name);
        if (i < 0)
            continue;
        str_data[i] = 0;
        fd = open(str_data, O_RDONLY);
        if (fd < 0)
            continue;
        /*
        ioctl(fd, EVIOCGNAME(sizeof(str_data) - 1), str_data);
        str_data[sizeof(str_data) - 1] = 0;
        */
        for (i = 0; i < sizeof(evs) / sizeof(evs[0]); ++i) {
            evs[i] = 0;
        }
        if (ioctl(fd, EVIOCGBIT(0, EV_MAX), evs) < 0)
            continue;
        if (evs[EV_KEY / (sizeof(long) * 8)] & (1L << EV_KEY % (sizeof(long) * 8))) {
			struct pollfd* pfd;
            DevFdSet_t* new_ds = (DevFdSet_t*)realloc(ds, sizeof(*ds) + (1 + nfds) * sizeof(struct pollfd));
            if (!new_ds)
                break;
			ds = new_ds;
			pfd = &ds->fds[nfds++];
			pfd->fd = fd;
			pfd->events = POLLIN;
			pfd->revents = 0;
        }
    }
    if (dir_item) {
        free_dev_fd_set(ds);
        return NULL;
    }
	ds->fds[nfds].events = POLLIN;
	ds->nfds = nfds;
    return ds;
}

static void __fill_key_event(DevKeyEvent_t* e, const struct input_event* input_ev) {
	static int shift_press, capslock_open;
	e->keydown = (input_ev->value != 0);
	switch (input_ev->code) {
		case KEY_LEFTSHIFT:
		case KEY_RIGHTSHIFT:
		{
			e->vkeycode = VKEY_SHIFT;
			shift_press = e->keydown;
			break;
		}
		case KEY_LEFTCTRL:
		case KEY_RIGHTCTRL:
		{
			e->vkeycode = VKEY_CTRL;
			break;
		}
		case KEY_LEFTALT:
		case KEY_RIGHTALT:
		{
			e->vkeycode = VKEY_ALT;
			break;
		}
		case KEY_CAPSLOCK:
		{
			if (e->keydown) {
				if (0 == capslock_open)
					capslock_open = 1;
					else if (2 == capslock_open)
						capslock_open = 0;
			}
			else if (!e->keydown && 1 == capslock_open)
				capslock_open = 2;
		}
		default:
			e->vkeycode = input_ev->code;
	}
	e->charcode = vkey2char(e->vkeycode, shift_press, capslock_open);
}
#endif
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
	return GetConsoleOriginalTitleA(name, buflen) ? name : NULL;
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

BOOL terminalKeyDevScan(void) {
#if defined(_WIN32) || defined(_WIN64)
	return TRUE;
#elif __linux__
	s_DevFdSet = scan_dev_fd_set(s_DevFdSet);
	return s_DevFdSet != NULL;
#else
	return TRUE;
#endif
}

BOOL terminalReadKey(FD_t fd, DevKeyEvent_t* e) {
#if defined(_WIN32) || defined(_WIN64)
	while (1) {
		DWORD n;
		INPUT_RECORD ir;
		if (!ReadConsoleInput((HANDLE)fd, &ir, 1, &n) || 0 == n)
			return FALSE;
		if (KEY_EVENT == ir.EventType) {
			e->keydown = ir.Event.KeyEvent.bKeyDown;
			e->charcode = ir.Event.KeyEvent.uChar.UnicodeChar;
			e->vkeycode = ir.Event.KeyEvent.wVirtualKeyCode;
			return TRUE;
		}
		else if (MOUSE_EVENT == ir.EventType) {
			if (ir.Event.MouseEvent.dwEventFlags) {
				continue;
			}
			if (!__fill_mouse_event(e, ir.Event.MouseEvent.dwButtonState)) {
				continue;
			}
			return TRUE;
		}
	}
#elif __linux__
	DevFdSet_t* ds = s_DevFdSet;
	struct pollfd* in_pfd = &ds->fds[ds->nfds];
	in_pfd->fd = fd;
	in_pfd->revents = 0;
	while (1) {
		int i;
		i = poll(ds->fds, ds->nfds + 1, -1);
		if (i < 0)
			return 0;
		else if (0 == i)
			continue;
		if ((in_pfd->revents & POLLIN) && 1 == i) {
			int k = 0;
			int len = read(fd, &k, sizeof(k));
			if (len <= 0) {
				continue;
			}
			e->keydown = 1;
			e->charcode = k;
			e->vkeycode = 0;
			return 1;
		}
		for (i = 0; i < ds->nfds; ++i) {
			struct input_event input_ev;
			int len;
			if (!(ds->fds[i].revents & POLLIN))
				continue;
			len = read(ds->fds[i].fd, &input_ev, sizeof(input_ev));
			if (len < 0)
				continue;
			else if (len != sizeof(input_ev))
				continue;
			else if (EV_KEY != input_ev.type)
				continue;
			__fill_key_event(e, &input_ev);
			if (in_pfd->revents & POLLIN) {
				int k = 0;
				read(fd, &k, sizeof(k));
			}
			return 1;
		}
	}
#else
	// TODO MAC
	e->keydown = 0;
	e->charcode = 0;
	e->vkeycode = 0;
	return FALSE;
#endif
}

BOOL terminalReadKeyNonBlock(FD_t fd, DevKeyEvent_t* e) {
#if defined(_WIN32) || defined(_WIN64)
	DWORD n = 0;
	INPUT_RECORD ir;
	while (PeekConsoleInput((HANDLE)fd, &ir, 1, &n) && n > 0) {
		if (!ReadConsoleInput((HANDLE)fd, &ir, 1, &n) || 0 == n)
			return FALSE;
		if (KEY_EVENT == ir.EventType) {
			e->keydown = ir.Event.KeyEvent.bKeyDown;
			e->charcode = ir.Event.KeyEvent.uChar.UnicodeChar;
			e->vkeycode = ir.Event.KeyEvent.wVirtualKeyCode;
			return TRUE;
		}
		else if (MOUSE_EVENT == ir.EventType) {
			if (ir.Event.MouseEvent.dwEventFlags) {
				continue;
			}
			if (!__fill_mouse_event(e, ir.Event.MouseEvent.dwButtonState)) {
				continue;
			}
			return TRUE;
		}
		n = 0;
	}
	return n > 0;
#elif __linux__
	DevFdSet_t* ds = s_DevFdSet;
	struct pollfd* in_pfd = &ds->fds[ds->nfds];
	in_pfd->fd = fd;
	in_pfd->revents = 0;
	while (1) {
		int i;
		i = poll(ds->fds, ds->nfds + 1, 0);
		if (i < 0)
			return 0;
		else if (0 == i)
			return 0;
		if ((in_pfd->revents & POLLIN) && 1 == i) {
			int k = 0;
			int len = read(fd, &k, sizeof(k));
			if (len <= 0) {
				return 0;
			}
			e->keydown = 1;
			e->charcode = k;
			e->vkeycode = 0;
			return 1;
		}
		for (i = 0; i < ds->nfds; ++i) {
			struct input_event input_ev;
			int len;
			if (!(ds->fds[i].revents & POLLIN))
				continue;
			len = read(ds->fds[i].fd, &input_ev, sizeof(input_ev));
			if (len < 0)
				continue;
			else if (len != sizeof(input_ev))
				continue;
			else if (EV_KEY != input_ev.type)
				continue;
			__fill_key_event(e, &input_ev);
			if (in_pfd->revents & POLLIN) {
				int k = 0;
				read(fd, &k, sizeof(k));
			}
			return 1;
		}
	}
#else
	// TODO MAC
	return FALSE;
#endif
}

#ifdef	__cplusplus
}
#endif
