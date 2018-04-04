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
int gmt_timezone_offset_second(void);
#define	gmt_second()	time(NULL)
long long gmt_millisecond(void);
char* gmtsecond2localstr(time_t value, char* buf, size_t len);
char* localtm2localstr(struct tm* datetime, char* buf, size_t len);
struct tm* mktm(time_t value, struct tm* datetime);
struct tm* normal_tm(struct tm* datetime);
struct tm* unnormal_tm(struct tm* datetime);
int tm_cmp(const struct tm* t1, const struct tm* t2);
/* random */
typedef struct Rand48_t { unsigned int x[3], a[3], c; } Rand48_t;
void rand48_seed(Rand48_t* ctx, int seedval);
int rand48_int(Rand48_t* ctx);
int rand48_int_range(Rand48_t* ctx, int start, int end);
typedef struct RandMT19937_t { unsigned long long x[312]; int k; } RandMT19937_t;
void mt19937_seed(RandMT19937_t* ctx, int seedval);
unsigned long long mt19937_ul(RandMT19937_t* ctx);
#define	mt19937_l(ctx)	((long long)mt19937_ul(ctx))

#ifdef	__cplusplus
}
#endif

#endif
