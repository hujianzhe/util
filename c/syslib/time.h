//
// Created by hujianzhe
//

#ifndef	UTIL_C_SYSLIB_TIME_H
#define	UTIL_C_SYSLIB_TIME_H

#include "../platform_define.h"

#if defined(_WIN32) || defined(_WIN64)
	#include <sys/timeb.h>
#else
	#include <sys/time.h>
#endif
#include <time.h>

#ifdef	__cplusplus
extern "C" {
#endif

extern int TIMESTAMP_OFFSET_SECOND;

/* time trasform */
UTIL_LIBAPI int gmtimeTimezoneOffsetSecond(void);
#define	gmtimeSecond()	time(NULL)
UTIL_LIBAPI long long gmtimeMillisecond(void);
UTIL_LIBAPI char* structtmText(struct tm* datetime, char* buf, size_t len);
UTIL_LIBAPI struct tm* structtmMake(time_t value, struct tm* datetime);
UTIL_LIBAPI struct tm* structtmNormal(struct tm* datetime);
UTIL_LIBAPI struct tm* structtmUnnormal(struct tm* datetime);
UTIL_LIBAPI int structtmCmp(const struct tm* t1, const struct tm* t2);

#ifdef	__cplusplus
}
#endif

#endif
