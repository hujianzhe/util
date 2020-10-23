//
// Created by hujianzhe
//

#ifndef	UTIL_C_SYSLIB_VKEYCODE_H
#define	UTIL_C_SYSLIB_VKEYCODE_H

#include "../platform_define.h"

#if defined(_WIN32) || defined(_WIN64)
	#define	DEV_VKEY(k)		VK_##k

	#define	VK_BACKSPACE	VK_BACK
	#define	VK_CAPSLOCK		VK_CAPITAL
	#define	VK_ENTER		VK_RETURN
	#define	VK_SCROLLLOCK	VK_SCROLL
	#define	VK_ALT			VK_MENU

#elif	__linux__
	#include <linux/input.h>
	#define	DEV_VKEY(k)		KEY_##k

	#define	KEY_SHIFT		0x10000
	#define	KEY_CONTROL		0x10001
	#define	KEY_ALT			0x10002
	#define	KEY_ESCAPE		KEY_ESC

#else
	// TODO MAC
	#define	DEV_VKEY(k)		_##k

#endif

#endif
