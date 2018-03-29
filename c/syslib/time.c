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
int gmt_TimeZoneOffsetSecond(void) {
	static int tm_gmtoff;
	if (0 == tm_gmtoff) {
#if defined(WIN32) || defined(_WIN64)
		TIME_ZONE_INFORMATION tz;
		if (GetTimeZoneInformation(&tz) == TIME_ZONE_ID_INVALID)
			return -1;
		tm_gmtoff = -(tz.Bias * 60);
#else
		struct timeval tv;
		struct timezone tz;
		if (gettimeofday(&tv, &tz))
			return -1;
		tm_gmtoff = -tz.tz_minuteswest * 60;
#endif
	}
	return tm_gmtoff;
}

long long gmt_Millisecond(void) {
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

#ifdef	__cplusplus
}
#endif
