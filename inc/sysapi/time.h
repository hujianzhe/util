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
__declspec_dll int gmtimeTimezoneOffsetSecond(void);
__declspec_dll time_t gmtimeSecond(void);
__declspec_dll time_t localtimeSecond(void);
__declspec_dll long long gmtimeMillisecond(void);
__declspec_dll struct tm* gmtimeTM(time_t value, struct tm* datetime);
__declspec_dll struct tm* gmtimeLocalTM(time_t value, struct tm* datetime);
__declspec_dll char* structtmText(struct tm* datetime, char* buf, size_t len);
__declspec_dll struct tm* structtmNormal(struct tm* datetime);
__declspec_dll struct tm* structtmUnnormal(struct tm* datetime);
__declspec_dll int structtmCmp(const struct tm* t1, const struct tm* t2);
__declspec_dll long long clockNanosecond(void);

#ifdef	__cplusplus
}
#endif

#endif
