//
// Created by hujianzhe
//

#include "../../inc/sysapi/time.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#if	defined(__APPLE__) || defined(__MACH__)
#include <mach/clock.h>
#include <mach/mach.h>
#include <mach/mach_time.h>
#endif

#ifdef	__cplusplus
extern "C" {
#endif

int TIMESTAMP_OFFSET_SECOND = 0;

/* time trasform */
int gmtimeTimezoneOffsetSecond(void) {
	/* time_t local_time = gmtimeSecond() - gmtimeTimezoneOffsetSecond() */
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

time_t gmtimeSecond(void) {
	time_t v;
	return time(&v);
}

long long gmtimeMillisecond(void) {
#if defined(WIN32) || defined(_WIN64)
	long long intervals;
	FILETIME  ft;
	GetSystemTimeAsFileTime(&ft);
	intervals = ((long long)ft.dwHighDateTime << 32) | ft.dwLowDateTime;
	intervals -= 116444736000000000;
	return intervals / 10000;
#else
	struct timeval tval;
	if (0 == gettimeofday(&tval, NULL)) {
		long long sec = tval.tv_sec;/* avoid overflow */
		return sec * 1000 + tval.tv_usec / 1000;
	}
	return 0;
#endif
}

struct tm* gmtimeTM(time_t value, struct tm* datetime) {
#if defined(WIN32) || defined(_WIN64)
	int res = gmtime_s(datetime, &value);
	if (res) {
		errno = res;
		return NULL;
	}
#else
	if (gmtime_r(&value, datetime) == NULL)
		return NULL;
#endif
	return datetime;
}

struct tm* gmtimeLocalTM(time_t value, struct tm* datetime) {
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

char* structtmText(struct tm* datetime, char* buf, size_t len) {
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

struct tm* structtmNormal(struct tm* datetime) {
	if (datetime) {
		datetime->tm_year += 1900;
		datetime->tm_mon += 1;
		if (0 == datetime->tm_wday)
			datetime->tm_wday = 7;
	}
	return datetime;
}

struct tm* structtmUnnormal(struct tm* datetime) {
	if (datetime) {
		datetime->tm_year -= 1900;
		datetime->tm_mon -= 1;
		if (7 == datetime->tm_wday)
			datetime->tm_wday = 0;
	}
	return datetime;
}

int structtmCmp(const struct tm* t1, const struct tm* t2) {
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

long long clockNanosecond(void) {
#if defined(WIN32) || defined(_WIN64)
	static LARGE_INTEGER s_frep;
	LARGE_INTEGER counter;
	if (0 == s_frep.QuadPart) {
		if (!QueryPerformanceFrequency(&s_frep) || 0 == s_frep.QuadPart) {
			return -1;
		}
	}
	if (!QueryPerformanceCounter(&counter))
		return -1;
	return (double)counter.QuadPart / s_frep.QuadPart * 1000000000;
#elif __linux__
	struct timespec ts;
	if (clock_gettime(CLOCK_MONOTONIC, &ts) < 0)
		return -1;
	return ts.tv_sec * 1000000000LL + ts.tv_nsec;
#elif __FreeBSD__
	struct timespec ts;
	if (clock_gettime(CLOCK_UPTIME, &ts) < 0)
		return -1;
	return ts.tv_sec * 1000000000LL + ts.tv_nsec;
#elif defined(__APPLE__) || defined(__MACH__)
	static mach_timebase_info_data_t s_timebase_info;
	if (0 == s_timebase_info.denom) {
		mach_timebase_info(&s_timebase_info);
		if (0 == s_timebase_info.denom)
			return -1;
	}
	return mach_absolute_time() * s_timebase_info.numer / s_timebase_info.denom;
#else
	return -1;
#endif
}

#ifdef	__cplusplus
}
#endif
