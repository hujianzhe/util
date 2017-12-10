//
// Created by hujianzhe
//

#ifndef	UTIL_C_SYSLIB_TIME_H
#define	UTIL_C_SYSLIB_TIME_H

#include "platform_define.h"

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
int gmt_TimeZoneOffsetSecond(void);
#define	gmt_Second()			time(NULL)
long long gmt_Millisecond(void);
char* gmtsecond2localstr(time_t value, char* buf, size_t len);
char* localtm2localstr(struct tm* datetime, char* buf, size_t len);
struct tm* mktm(time_t value, struct tm* datetime);
struct tm* normal_tm(struct tm* datetime);
struct tm* unnormal_tm(struct tm* datetime);

#ifdef	__cplusplus
}
#endif

#endif
