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
	FD_t fd;
	const char* key;
	const char* base_path;
	CriticalSection_t lock;
	time_t rotate_timestamp_sec;
	LogFileOutputOption_t output_opt;
	LogFileRotateOption_t rotate_opt;
} LogFile_t;

typedef struct Log_t {
	int enable_priority[4];
	LogFile_t** files;
	size_t files_cnt;
} Log_t;

typedef struct CacheBlock_t {
	size_t len;
	char txt[1];
} CacheBlock_t;

static LogFile_t* get_log_file(Log_t* log, const char* key) {
	size_t i;
	if (!key) {
		key = "";
	}
	for (i = 0; i < log->files_cnt; ++i) {
		LogFile_t* lf = log->files[i];
		if (!strcmp(lf->key, key)) {
			return lf;
		}
	}
	return NULL;
}

static void free_log_file(LogFile_t* lf) {
	if (INVALID_FD_HANDLE != lf->fd) {
		fdClose(lf->fd);
	}
	criticalsectionClose(&lf->lock);
	free((void*)lf->key);
	free((void*)lf->base_path);
	free(lf);
}

static void log_rotate(LogFile_t* lf, const struct tm* dt, time_t cur_sec) {
	const char* new_path = NULL;
	const LogFileRotateOption_t* opt = &lf->rotate_opt;
	criticalsectionEnter(&lf->lock);

	if (cur_sec >= lf->rotate_timestamp_sec && opt->rotate_timelen_sec > 0) {
		time_t t;
		if (lf->fd != INVALID_FD_HANDLE) {
			fdClose(lf->fd);
			lf->fd = INVALID_FD_HANDLE;
		}
		new_path = opt->fn_new_fullpath(lf->base_path, lf->key, dt);
		t = (cur_sec - lf->rotate_timestamp_sec) / opt->rotate_timelen_sec;
		if (t <= 0) {
			lf->rotate_timestamp_sec += opt->rotate_timelen_sec;
		}
		else {
			lf->rotate_timestamp_sec += (t + 1) * opt->rotate_timelen_sec;
		}
	}
	else if (INVALID_FD_HANDLE == lf->fd) {
		new_path = opt->fn_new_fullpath(lf->base_path, lf->key, dt);
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
	opt->fn_free_path((char*)new_path);
	return;
}

static void log_write(Log_t* log, CacheBlock_t* cache, LogFile_t* lf, const struct tm* dt, time_t cur_sec) {
	log_rotate(lf, dt, cur_sec);
	if (lf->fd != INVALID_FD_HANDLE) {
		fdWrite(lf->fd, cache->txt, cache->len);
	}
	free(cache);
}

static const char* log_get_priority_str(unsigned int level) {
	static const char* s_priority_str[] = {
		"Trace",
		"Info",
		"Debug"
		"Error",
	};
	if (level >= sizeof(s_priority_str) / sizeof(s_priority_str[0])) {
		return "";
	}
	return s_priority_str[level];
}

static const char* default_gen_path_minute(const char* base_path, const char* key, const struct tm* dt) {
	if (key && key[0]) {
		return strFormat(NULL, "%s%s_%d%02d%02d_%d_%d.log", base_path, key, dt->tm_year, dt->tm_mon, dt->tm_mday, dt->tm_hour, dt->tm_min);
	}
	else {
		return strFormat(NULL, "%s%d%02d%02d_%d_%d.log", base_path, dt->tm_year, dt->tm_mon, dt->tm_mday, dt->tm_hour, dt->tm_min);
	}
}

static const char* default_gen_path_hour(const char* base_path, const char* key, const struct tm* dt) {
	if (key && key[0]) {
		return strFormat(NULL, "%s%s_%d%02d%02d_%d.log", base_path, key, dt->tm_year, dt->tm_mon, dt->tm_mday, dt->tm_hour);
	}
	else {
		return strFormat(NULL, "%s%d%02d%02d_%d.log", base_path, dt->tm_year, dt->tm_mon, dt->tm_mday, dt->tm_hour);
	}
}

static const char* default_gen_path_day(const char* base_path, const char* key, const struct tm* dt) {
	if (key && key[0]) {
		return strFormat(NULL, "%s%s_%d%02d%02d.log", base_path, key, dt->tm_year, dt->tm_mon, dt->tm_mday);
	}
	else {
		return strFormat(NULL, "%s%d%02d%02d.log", base_path, dt->tm_year, dt->tm_mon, dt->tm_mday);
	}
}

static void default_free_path(char* path) { free(path); }

static int default_prefix_length(const LogItemInfo_t* item_info) {
	return strFormatLen("%d-%d-%d %d:%d:%d|%s|%s:%u|",
						item_info->dt.tm_year, item_info->dt.tm_mon, item_info->dt.tm_mday,
						item_info->dt.tm_hour, item_info->dt.tm_min, item_info->dt.tm_sec,
						item_info->priority_str, item_info->source_file, item_info->source_line);
}

static void default_sprintf_prefix(char* buf, const LogItemInfo_t* item_info) {
	sprintf(buf, "%d-%d-%d %d:%d:%d|%s|%s:%u|",
			item_info->dt.tm_year, item_info->dt.tm_mon, item_info->dt.tm_mday,
			item_info->dt.tm_hour, item_info->dt.tm_min, item_info->dt.tm_sec,
			item_info->priority_str, item_info->source_file, item_info->source_line);
}

static void log_build(Log_t* log, LogFile_t* lf, int priority, LogItemInfo_t* item_info, const char* format, va_list ap) {
	va_list varg;
	int len, res, prefix_len;
	char test_buf;
	time_t cur_sec;
	CacheBlock_t* cache;
	const LogFileOutputOption_t* opt;

	if (!format || 0 == *format) {
		return;
	}
	cur_sec = gmtimeSecond();
	if (!gmtimeLocalTM(cur_sec, &item_info->dt)) {
		return;
	}
	structtmNormal(&item_info->dt);
	item_info->priority_str = log_get_priority_str(priority);

	opt = &lf->output_opt;
	if (opt->fn_prefix_length) {
		prefix_len = opt->fn_prefix_length(item_info);
		if (prefix_len < 0) {
			return;
		}
	}
	else {
		prefix_len = 0;
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
	if (prefix_len > 0 && opt->fn_sprintf_prefix) {
		opt->fn_sprintf_prefix(cache->txt, item_info);
	}
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
	log_write(log, cache, lf, &item_info->dt, cur_sec);
}

#ifdef	__cplusplus
extern "C" {
#endif

Log_t* logOpen(void) {
	size_t i;
	Log_t* log = (Log_t*)malloc(sizeof(Log_t));
	if (!log) {
		return NULL;
	}
	for (i = 0; i < sizeof(log->enable_priority) / sizeof(log->enable_priority[0]); ++i) {
		log->enable_priority[i] = 1;
	}
	log->files = NULL;
	log->files_cnt = 0;
	return log;
}

const LogFileOutputOption_t* logFileOutputOptionDefault(void) {
	static LogFileOutputOption_t opt = {
		default_prefix_length,
		default_sprintf_prefix
	};
	return &opt;
}

const LogFileRotateOption_t* logFileRotateOptionDefaultDay(void) {
	static LogFileRotateOption_t opt = {
		86400,
		default_gen_path_day,
		default_free_path
	};
	return &opt;
}

const LogFileRotateOption_t* logFileRotateOptionDefaultHour(void) {
	static LogFileRotateOption_t opt = {
		3600,
		default_gen_path_hour,
		default_free_path
	};
	return &opt;
}

const LogFileRotateOption_t* logFileRotateOptionDefaultMinute(void) {
	static LogFileRotateOption_t opt = {
		60,
		default_gen_path_minute,
		default_free_path
	};
	return &opt;
}

Log_t* logEnableFile(struct Log_t* log, const char* key, const char* base_path, const LogFileOutputOption_t* output_opt, const LogFileRotateOption_t* rotate_opt) {
	LogFile_t* lf, **p;
	lf = get_log_file(log, key);
	if (lf) {
		return NULL;
	}
	/* new log file */
	p = (LogFile_t**)realloc(log->files, sizeof(LogFile_t*) * (log->files_cnt + 1));
	if (!p) {
		goto err;
	}
	lf = (LogFile_t*)malloc(sizeof(LogFile_t));
	if (!lf) {
		goto err;
	}
	if (!key) {
		key = "";
	}
	lf->key = strdup(key);
	if (!lf->key) {
		goto err;
	}
	if (!base_path || !base_path[0]) {
		base_path = "./";
	}
	lf->base_path = strdup(base_path);
	if (!lf->base_path) {
		goto err;
	}
	if (!criticalsectionCreate(&lf->lock)) {
		goto err;
	}
	lf->fd = INVALID_FD_HANDLE;
	/* output option */
	if (output_opt) {
		lf->output_opt = *output_opt;
	}
	else {
		memset(&lf->output_opt, 0, sizeof(lf->output_opt));
	}
	/* rotate option */
	if (rotate_opt->rotate_timelen_sec > 0) {
		time_t t = localtimeSecond() / rotate_opt->rotate_timelen_sec * rotate_opt->rotate_timelen_sec + gmtimeTimezoneOffsetSecond();
		lf->rotate_timestamp_sec = t + rotate_opt->rotate_timelen_sec;
	}
	else {
		lf->rotate_timestamp_sec = 0;
	}
	lf->rotate_opt = *rotate_opt;
	/* save */
	p[log->files_cnt++] = lf;
	log->files = p;
	return log;
err:
	if (lf) {
		free((void*)lf->key);
		free((void*)lf->base_path);
		free(lf);
	}
	return NULL;
}

void logDestroy(Log_t* log) {
	size_t i;
	if (!log) {
		return;
	}
	for (i = 0; i < log->files_cnt; ++i) {
		free_log_file(log->files[i]);
	}
	free(log->files);
	free(log);
}

void logPrintlnNoFilter(Log_t* log, const char* key, int priority, LogItemInfo_t* ii, const char* format, ...) {
	va_list varg;
	LogFile_t* lf = get_log_file(log, key);
	if (!lf) {
		return;
	}
	va_start(varg, format);
	log_build(log, lf, priority, ii, format, varg);
	va_end(varg);
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
	size_t i;
	for (i = 0; i < sizeof(log->enable_priority) / sizeof(log->enable_priority[0]); ++i) {
		log->enable_priority[i] = !fn_priority_filter(i, filter_priority);
	}
}

int logCheckPriorityEnabled(Log_t* log, unsigned int priority) {
	if (priority >= sizeof(log->enable_priority) / sizeof(log->enable_priority[0])) {
		return 0;
	}
	return log->enable_priority[priority];
}

#ifdef	__cplusplus
}
#endif
