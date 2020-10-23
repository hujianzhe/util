//
// Created by hujianzhe
//

#ifndef	UTIL_C_SYSLIB_VKEYCODE_H
#define	UTIL_C_SYSLIB_VKEYCODE_H

#include "../platform_define.h"

#if defined(_WIN32) || defined(_WIN64)
	#define	DEV_VKEY(k)	VK_##k
#elif	__linux__
	#include <linux/input.h>
	#define	DEV_VKEY(k)	KEY_##k
#else
	// TODO MAC
	#define	DEV_VKEY(k)	_##k
#endif

#endif
