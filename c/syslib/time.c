//
// Created by hujianzhe
//

#include "time.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#ifdef	__cplusplus
extern "C" {
#endif

int TIMESTAMP_OFFSET_SECOND = 0;

/* time trasform */
int gmt_timezone_offset_second(void) {
	/* time_t local_time = gmt_second() - gmt_timezone_offset_second() */
	static int tm_gmtoff;
	if (0 == tm_gmtoff) {
#if defined(WIN32) || defined(_WIN64)
		TIME_ZONE_INFORMATION tz;
		if (GetTimeZoneInformation(&tz) == TIME_ZONE_ID_INVALID)
			return -1;
		tm_gmtoff = tz.Bias * 60;
#else
		struct timeval tv;
		struct timezone tz;
		if (gettimeofday(&tv, &tz))
			return -1;
		tm_gmtoff = tz.tz_minuteswest * 60;
#endif
	}
	return tm_gmtoff;
}

long long gmt_millisecond(void) {
#if defined(WIN32) || defined(_WIN64)
	/*
	struct __timeb64 tval;
	int res = _ftime64_s(&tval);
	if (res) {
		errno = res;
		return 0;
	}
	else {
		long long sec = tval.time;
		return sec * 1000 + tval.millitm;
	}
	*/
	long long intervals;
	FILETIME  ft;
	GetSystemTimeAsFileTime(&ft);
	intervals = ((long long)ft.dwHighDateTime << 32) | ft.dwLowDateTime;
	intervals -= 116444736000000000;
	return intervals / 10000;
	/*
	tv_sec = intervals / 10000000;
	tv_usec = intervals % 10000000 / 10;
	return tv_sec * 1000 + tv_usec / 1000;
	*/
#else
	struct timeval tval;
	if (0 == gettimeofday(&tval, NULL)) {
		long long sec = tval.tv_sec;/* avoid overflow */
		return sec * 1000 + tval.tv_usec / 1000;
	}
	return 0;
#endif
}

char* gmtsecond2localstr(time_t value, char* buf, size_t len) {
	char* c;
#if defined(WIN32) || defined(_WIN64)
	int res = ctime_s(buf, len, &value);
	if (res) {
		errno = res;
		return NULL;
	}
#else
	if (ctime_r(&value, buf) == NULL)
		return NULL;
#endif
	if ((c = strchr(buf, '\n'))) {
		*c = 0;
	}
	return buf;
}

char* localtm2localstr(struct tm* datetime, char* buf, size_t len) {
	char* c;
#if defined(WIN32) || defined(_WIN64)
	int res = asctime_s(buf, len, datetime);
	if (res) {
		errno = res;
		return NULL;
	}
#else
	if (asctime_r(datetime, buf) == NULL)
		return NULL;
#endif
	if ((c = strchr(buf, '\n'))) {
		*c = 0;
	}
	return buf;
}

struct tm* mktm(time_t value, struct tm* datetime) {
#if defined(WIN32) || defined(_WIN64)
	int res = localtime_s(datetime, &value);
	if (res) {
		errno = res;
		return NULL;
	}
#else
	if (localtime_r(&value, datetime) == NULL)
		return NULL;
#endif
	return datetime;
}

struct tm* normal_tm(struct tm* datetime) {
	if (datetime) {
		datetime->tm_year += 1900;
		datetime->tm_mon += 1;
		if (0 == datetime->tm_wday)
			datetime->tm_wday = 7;
	}
	return datetime;
}

struct tm* unnormal_tm(struct tm* datetime) {
	if (datetime) {
		datetime->tm_year -= 1900;
		datetime->tm_mon -= 1;
		if (7 == datetime->tm_wday)
			datetime->tm_wday = 0;
	}
	return datetime;
}

int tm_cmp(const struct tm* t1, const struct tm* t2) {
	if (t1->tm_yday > t2->tm_yday) {
		return 1;
	}
	if (t1->tm_yday < t2->tm_yday) {
		return -1;
	}
	if (t1->tm_hour > t2->tm_hour) {
		return 1;
	}
	if (t1->tm_hour < t2->tm_hour) {
		return -1;
	}
	if (t1->tm_min > t2->tm_min) {
		return 1;
	}
	if (t1->tm_min < t2->tm_min) {
		return -1;
	}
	if (t1->tm_sec > t2->tm_sec) {
		return 1;
	}
	if (t1->tm_sec < t2->tm_sec) {
		return -1;
	}
	return 0;
}

/* random */
#define	N					16
#define MASK				((1 << (N - 1)) + (1 << (N - 1)) - 1)
#define LOW(x)				((unsigned)(x) & MASK)
#define HIGH(x)				LOW((x) >> N)
#define MUL(x, y, z)		{ int l = (long)(x) * (long)(y); (z)[0] = LOW(l); (z)[1] = HIGH(l); }
#define CARRY(x, y)			((int)(x) + (long)(y) > MASK)
#define ADDEQU(x, y, z)		(z = CARRY(x, (y)), x = LOW(x + (y)))
#define X0					0x330E
#define X1					0xABCD
#define X2					0x1234
#define A0					0xE66D
#define A1					0xDEEC
#define A2					0x5
#define C					0xB
#define SET3(x, x0, x1, x2)	((x)[0] = (x0), (x)[1] = (x1), (x)[2] = (x2))
static void __rand_next(Rand48_t* ctx) {
	unsigned int p[2], q[2], r[2], carry0, carry1;

	MUL(ctx->a[0], ctx->x[0], p);
	ADDEQU(p[0], ctx->c, carry0);
	ADDEQU(p[1], carry0, carry1);
	MUL(ctx->a[0], ctx->x[1], q);
	ADDEQU(p[1], q[0], carry0);
	MUL(ctx->a[1], ctx->x[0], r);
	ctx->x[2] = LOW(carry0 + carry1 + CARRY(p[1], r[0]) + q[1] + r[1] +
				ctx->a[0] * ctx->x[2] + ctx->a[1] * ctx->x[1] + ctx->a[2] * ctx->x[0]);
	ctx->x[1] = LOW(p[1] + r[0]);
	ctx->x[0] = LOW(p[0]);
}

void rand48_seed(Rand48_t* ctx, int seedval) {
	ctx->x[0] = X0;
	ctx->x[1] = LOW(seedval);
	ctx->x[2] = HIGH(seedval);

	ctx->a[0] = A0;
	ctx->a[1] = A1;
	ctx->a[2] = A2;

	ctx->c = C;
}

int rand48_int(Rand48_t* ctx) {
	__rand_next(ctx);
	return (((int)(ctx->x[2]) << (N - 1)) + (ctx->x[1] >> 1));
}

int rand48_int_range(Rand48_t* ctx, int start, int end) {
	/* [start, end) */
	return rand48_int(ctx) % (end - start) + start;
}

void mt19937_seed(RandMT19937_t* ctx, int seedval) {
	int i;
	unsigned long long* x = ctx->x;
	x[0] = seedval;
	for (i = 1; i < sizeof(ctx->x) / sizeof(ctx->x[0]); i++)
		x[i] = 6364136223846793005ULL * (x[i - 1] ^ (x[i - 1] >> (64 - 2))) + i;
	ctx->k = 0;
}

unsigned long long mt19937_ul(RandMT19937_t* ctx) {
	int k = ctx->k;
	unsigned long long* x = ctx->x;
	unsigned long long y, z;

	/* z = (x^u_k | x^l_(k+1))*/
	z = (x[k] & 0xffffffff80000000ULL) | (x[(k + 1) % 312] & 0x7fffffffULL);
	/* x_(k+n) = x_(k+m) |+| z*A */
	x[k] = x[(k + 156) % 312] ^ (z >> 1) ^ (!(z & 1ULL) ? 0ULL : 0xb5026f5aa96619e9ULL);
	/* Tempering */
	y = x[k];
	y ^= (y >> 29) & 0x5555555555555555ULL;
	y ^= (y << 17) & 0x71d67fffeda60000ULL;
	y ^= (y << 37) & 0xfff7eee000000000ULL;
	y ^= y >> 43;

	ctx->k = (k + 1) % 312;
	return y;
}

#ifdef	__cplusplus
}
#endif
