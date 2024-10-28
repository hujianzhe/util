//
// Created by hujianzhe
//

#include "../../inc/datastruct/list.h"
#include "../../inc/sysapi/ipc.h"
#include "../../inc/sysapi/error.h"
#include "../../inc/sysapi/file.h"
#include "../../inc/sysapi/time.h"
#include "../../inc/crt/string.h"
#include "../../inc/component/log.h"
#include <stddef.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct LogFile_t {
	CriticalSection_t lock;
	FD_t fd;
	const char*(*fn_gen_path)(const char* ident, const struct tm* dt);
	void(*fn_free_path)(char*);
	time_t rotate_timestamp;
	int rotate_timelen;
} LogFile_t;

typedef struct Log_t {
	const char* ident;
	FILE* print_stdio;
	int cur_filter_priority;
	int(*fn_priority_filter)(int, int);
	int(*fn_prefix_length)(const LogItemInfo_t*);
	void(*fn_sprintf_prefix)(char*, const LogItemInfo_t*);
	LogFile_t* file;
} Log_t;

typedef struct CacheBlock_t {
	size_t len;
	char txt[1];
} CacheBlock_t;

static void log_rotate(LogFile_t* lf, const char* ident, const struct tm* dt, time_t cur_timestamp) {
	const char* new_path = NULL;
	criticalsectionEnter(&lf->lock);

	if (cur_timestamp >= lf->rotate_timestamp && lf->rotate_timelen > 0) {
		time_t t;
		if (lf->fd != INVALID_FD_HANDLE) {
			fdClose(lf->fd);
			lf->fd = INVALID_FD_HANDLE;
		}
		new_path = lf->fn_gen_path(ident, dt);
		t = (cur_timestamp - lf->rotate_timestamp) / lf->rotate_timelen;
		if (t <= 0) {
			lf->rotate_timestamp += lf->rotate_timelen;
		}
		else {
			lf->rotate_timestamp += (t + 1) * lf->rotate_timelen;
		}
	}
	else if (INVALID_FD_HANDLE == lf->fd) {
		new_path = lf->fn_gen_path(ident, dt);
	}
	if (!new_path) {
		goto end;
	}
	lf->fd = fdOpen(new_path, FILE_CREAT_BIT | FILE_WRITE_BIT | FILE_APPEND_BIT);
	if (INVALID_FD_HANDLE == lf->fd) {
		goto end;
	}
end:
	criticalsectionLeave(&lf->lock);
	lf->fn_free_path((char*)new_path);
	return;
}

static void log_write(Log_t* log, CacheBlock_t* cache, const struct tm* dt, time_t cur_timestamp) {
	if (log->print_stdio) {
		fputs(cache->txt, log->print_stdio);
	}
	if (log->file) {
		log_rotate(log->file, log->ident, dt, cur_timestamp);
		if (log->file->fd != INVALID_FD_HANDLE) {
			fdWrite(log->file->fd, cache->txt, cache->len);
		}
	}
	free(cache);
}

static const char* log_get_priority_str(int level) {
	static const char* s_priority_str[] = {
		"Emerg",
		"Alert",
		"Crit",
		"Err",
		"Warning",
		"Notice",
		"Info",
		"Debug"
	};
	if (level < 0 || level >= sizeof(s_priority_str) / sizeof(s_priority_str[0])) {
		return "";
	}
	return s_priority_str[level];
}

static int log_default_prefix_length(const LogItemInfo_t* item_info) {
	return strFormatLen("%d-%d-%d %d:%d:%d|%s|%s:%u|",
						item_info->dt.tm_year, item_info->dt.tm_mon, item_info->dt.tm_mday,
						item_info->dt.tm_hour, item_info->dt.tm_min, item_info->dt.tm_sec,
						item_info->priority_str, item_info->source_file, item_info->source_line);
}

static void log_default_sprintf_prefix(char* buf, const LogItemInfo_t* item_info) {
	sprintf(buf, "%d-%d-%d %d:%d:%d|%s|%s:%u|",
			item_info->dt.tm_year, item_info->dt.tm_mon, item_info->dt.tm_mday,
			item_info->dt.tm_hour, item_info->dt.tm_min, item_info->dt.tm_sec,
			item_info->priority_str, item_info->source_file, item_info->source_line);
}

static void log_build(Log_t* log, LogItemInfo_t* item_info, int priority, const char* format, va_list ap) {
	va_list varg;
	int len, res, prefix_len;
	char test_buf;
	time_t cur_timestamp;
	CacheBlock_t* cache;

	if (!format || 0 == *format) {
		return;
	}
	cur_timestamp = gmtimeSecond();
	if (!gmtimeLocalTM(cur_timestamp, &item_info->dt)) {
		return;
	}
	structtmNormal(&item_info->dt);
	item_info->ident = log->ident;
	item_info->priority_str = log_get_priority_str(priority);

	prefix_len = log->fn_prefix_length(item_info);
	if (prefix_len < 0) {
		return;
	}
	va_copy(varg, ap);
	res = vsnprintf(&test_buf, 0, format, varg);
	va_end(varg);
	if (res <= 0) {
		return;
	}
	len = prefix_len + res;
	++len;/* append '\n' */
	cache = (CacheBlock_t*)malloc(sizeof(CacheBlock_t) + len);
	if (!cache) {
		return;
	}
	log->fn_sprintf_prefix(cache->txt, item_info);
	va_copy(varg, ap);
	res = vsnprintf(cache->txt + prefix_len, len - prefix_len + 1, format, varg);
	va_end(varg);
	if (res <= 0) {
		free(cache);
		return;
	}
	cache->txt[len - 1] = '\n';
	cache->txt[len] = 0;
	cache->len = len;
	log_write(log, cache, &item_info->dt, cur_timestamp);
}

#ifdef	__cplusplus
extern "C" {
#endif

Log_t* logOpen(const char* ident) {
	Log_t* log = (Log_t*)malloc(sizeof(Log_t));
	if (!log) {
		return NULL;
	}
	log->ident = strdup(ident ? ident : "");
	if (!log->ident || !log->ident[0]) {
		goto err;
	}
	log->file = NULL;
	log->print_stdio = NULL;
	log->cur_filter_priority = -1;
	log->fn_priority_filter = NULL;
	log->fn_prefix_length = log_default_prefix_length;
	log->fn_sprintf_prefix = log_default_sprintf_prefix;
	return log;
err:
	free((void*)log->ident);
	free(log);
	return NULL;
}

void logSetOutputPrefix(Log_t* log, int(*fn_prefix_length)(const LogItemInfo_t*), void(*fn_sprintf_prefix)(char*, const LogItemInfo_t*)) {
	log->fn_prefix_length = fn_prefix_length;
	log->fn_sprintf_prefix = fn_sprintf_prefix;
}

Log_t* logEnableFile(struct Log_t* log, int rotate_timelen_sec, const char*(*fn_gen_path)(const char* ident, const struct tm* dt), void(*fn_free_path)(char*)) {
	LogFile_t* lf;
	if (log->file) {
		return log;
	}
	lf = (LogFile_t*)malloc(sizeof(LogFile_t));
	if (!lf) {
		return NULL;
	}
	if (!criticalsectionCreate(&lf->lock)) {
		goto err;
	}
	lf->fn_gen_path = fn_gen_path;
	lf->fn_free_path = fn_free_path;
	lf->fd = INVALID_FD_HANDLE;
	if (rotate_timelen_sec > 0) {
		time_t t = localtimeSecond() / rotate_timelen_sec * rotate_timelen_sec + gmtimeTimezoneOffsetSecond();
		lf->rotate_timelen = rotate_timelen_sec;
		lf->rotate_timestamp = t + rotate_timelen_sec;
	}
	else {
		lf->rotate_timelen = 0;
		lf->rotate_timestamp = 0;
	}
	log->file = lf;
	return log;
err:
	free(lf);
	return NULL;
}

void logSetStdioFile(struct Log_t* log, FILE* p_file) {
	log->print_stdio = p_file;
}

void logDestroy(Log_t* log) {
	LogFile_t* lf;
	if (!log) {
		return;
	}
	lf = log->file;
	if (lf) {
		criticalsectionClose(&lf->lock);
		if (INVALID_FD_HANDLE != lf->fd) {
			fdClose(lf->fd);
		}
		free(lf);
	}
	free((void*)log->ident);
	free(log);
}

void logPrintlnNoFilter(Log_t* log, LogItemInfo_t* ii, int priority, const char* format, ...) {
	va_list varg;
	va_start(varg, format);
	log_build(log, ii, priority, format, varg);
	va_end(varg);
}

void logPrintRaw(Log_t* log, int priority, const char* format, ...) {
	va_list varg;
	int len;
	char test_buf;
	time_t cur_timestamp;
	struct tm dt;
	CacheBlock_t* cache;

	if (!format || 0 == *format) {
		return;
	}
	if (logCheckPriorityFilter(log, priority)) {
		return;
	}
	cur_timestamp = gmtimeSecond();
	if (!gmtimeLocalTM(cur_timestamp, &dt)) {
		return;
	}
	structtmNormal(&dt);

	va_start(varg, format);
	len = vsnprintf(&test_buf, 0, format, varg);
	va_end(varg);
	if (len <= 0) {
		return;
	}
	cache = (CacheBlock_t*)malloc(sizeof(CacheBlock_t) + len);
	if (!cache) {
		return;
	}
	va_start(varg, format);
	len = vsnprintf(cache->txt, len + 1, format, varg);
	va_end(varg);
	if (len <= 0) {
		free(cache);
		return;
	}
	cache->txt[len] = 0;
	cache->len = len;
	log_write(log, cache, &dt, cur_timestamp);
}

int logFilterPriorityLess(int log_priority, int filter_priority) {
	return log_priority < filter_priority;
}

int logFilterPriorityLessEqual(int log_priority, int filter_priority) {
	return log_priority <= filter_priority;
}

int logFilterPriorityGreater(int log_priority, int filter_priority) {
	return log_priority > filter_priority;
}

int logFilterPriorityGreaterEqual(int log_priority, int filter_priority) {
	return log_priority >= filter_priority;
}

int logFilterPriorityEqual(int log_priority, int filter_priority) {
	return log_priority == filter_priority;
}

int logFilterPriorityNotEqual(int log_priority, int filter_priority) {
	return log_priority != filter_priority;
}

void logSetPriorityFilter(Log_t* log, int filter_priority, int(*fn_priority_filter)(int, int)) {
	log->cur_filter_priority = filter_priority;
	log->fn_priority_filter = fn_priority_filter;
}

int logCheckPriorityFilter(Log_t* log, int priority) {
	return log->fn_priority_filter && log->fn_priority_filter(priority, log->cur_filter_priority);
}

#ifdef	__cplusplus
}
#endif
