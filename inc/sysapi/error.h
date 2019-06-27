//
// Created by hujianzhe
//

#ifndef	UTIL_C_SYSLIB_ERROR_H
#define	UTIL_C_SYSLIB_ERROR_H

#include "../platform_define.h"
#include <errno.h>

#ifdef	__cplusplus
extern "C" {
#endif

__declspec_dll int errnoGet(void);
__declspec_dll void errnoSet(int errnum);
__declspec_dll char* errnoText(int errnum, char* buf, size_t bufsize);

#ifdef	__cplusplus
}
#endif

#endif
