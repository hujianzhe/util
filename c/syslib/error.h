//
// Created by hujianzhe
//

#ifndef	UTIL_C_SYSLIB_ERROR_H
#define	UTIL_C_SYSLIB_ERROR_H

#include "platform_define.h"
#include <errno.h>

#ifdef	__cplusplus
extern "C" {
#endif

int error_code(void);
void clear_error_code(void);
char* error_msg(int errnum, char* buf, size_t bufsize);

#ifdef	__cplusplus
}
#endif

#endif
