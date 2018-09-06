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

UTIL_LIBAPI int errnoGet(void);
UTIL_LIBAPI void errnoSet(int errnum);
UTIL_LIBAPI char* errnoText(int errnum, char* buf, size_t bufsize);

#ifdef	__cplusplus
}
#endif

#endif
