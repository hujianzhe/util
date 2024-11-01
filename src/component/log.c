//
// Created by hujianzhe
//

#include "../../inc/datastruct/list.h"
#include "../../inc/sysapi/atomic.h"
#include "../../inc/sysapi/ipc.h"
#include "../../inc/sysapi/error.h"
#include "../../inc/sysapi/file.h"
#include "../../inc/sysapi/process.h"
#include "../../inc/sysapi/time.h"
#include "../../inc/crt/string.h"
#include "../../inc/component/dataqueue.h"
#include "../../inc/component/log.h"
#include <stddef.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct LogAsyncOutputThread_t {
	Thread_t tid;
	long long next_output_msec;
	volatile char exit_flag;
	DataQueue_t cache_dq;
} LogAsyncOutputThread_t;

typedef struct LogFile_t {
	FD_t fd;
	const char* key;
	const char* base_path;
	CriticalSection_t lock;
	time_t rotate_timestamp_sec;
	LogAsyncOutputThread_t* output_thrd;
	LogFileOutputOption_t output_opt;
	LogFileRotateOption_t rotate_opt;
} LogFile_t;

typedef struct Log_t {
	int enable_priority[4];
	LogFile_t** files;
	size_t files_cnt;
	size_t async_output_select_idx;
	LogAsyncOutputThread_t** async_output_thrds;
	size_t async_output_thrds_cnt;
} Log_t;

typedef struct CacheBlock_t {
	size_t len;
	char txt[1];
} CacheBlock_t;

typedef struct AsyncCacheBlock_t {
	ListNode_t _lnode;
	LogFile_t* lf;
	LogItemInfo_t item_info;
	int prefix_len;
	size_t txt_len;
	char txt[1];
} AsyncCacheBlock_t;

static int init_async_output_thread(LogAsyncOutputThread_t* thrd) {
	if (!dataqueueInit(&thrd->cache_dq)) {
		return 0;
	}
	thrd->exit_flag = 0;
	thrd->next_output_msec = 0;
	return 1;
}

void free_async_output_thread_cache_list(ListNode_t* head) {
	ListNode_t* lcur, *lnext;
	for (lcur = head; lcur; lcur = lnext) {
		AsyncCacheBlock_t* cache = pod_container_of(lcur, AsyncCacheBlock_t, _lnode);
		lnext = lcur->next;
		free(cache);
	}
}

void exit_free_async_output_thread(LogAsyncOutputThread_t* thrd) {
	thrd->exit_flag = 1;
	dataqueueWake(&thrd->cache_dq);
	threadJoin(thrd->tid, NULL);
	free_async_output_thread_cache_list(dataqueueDestroy(&thrd->cache_dq));
	free(thrd);
}

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
	free((char*)lf->key);
	free((char*)lf->base_path);
	free(lf);
}

static void log_rotate(LogFile_t* lf, const struct tm* dt, time_t cur_sec) {
	const char* new_path = NULL;
	const LogFileRotateOption_t* opt = &lf->rotate_opt;

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
		return;
	}
	lf->fd = fdOpen(new_path, FILE_CREAT_BIT | FILE_WRITE_BIT | FILE_APPEND_BIT);
	opt->fn_free_path((char*)new_path);
}

static unsigned int log_async_output_thrd_entry(void* arg) {
	LogAsyncOutputThread_t* output_thrd = (LogAsyncOutputThread_t*)arg;
	ListNode_t* lcur = NULL;
	char* prefix_buffer = NULL;
	size_t max_prefix_buffer_len = 0;
	while (!output_thrd->exit_flag) {
		lcur = dataqueuePopWait(&output_thrd->cache_dq, -1, -1);
		while (lcur) {
			AsyncCacheBlock_t* cache = pod_container_of(lcur, AsyncCacheBlock_t, _lnode);
			LogFile_t* lf = cache->lf;
			lcur = lcur->next;
			log_rotate(lf, &cache->item_info.dt, cache->item_info.timestamp_sec);
			if (lf->fd != INVALID_FD_HANDLE) {
				Iobuf_t iov[2];
				/* fill prefix */
				if (cache->prefix_len > 0 && lf->output_opt.fn_sprintf_prefix) {
					if (max_prefix_buffer_len < cache->prefix_len + 1) {
						char* tmp_ptr = (char*)realloc(prefix_buffer, cache->prefix_len + 1);
						if (!tmp_ptr) {
							free(cache);
							continue;
						}
						prefix_buffer = tmp_ptr;
						max_prefix_buffer_len = cache->prefix_len + 1;
					}
					lf->output_opt.fn_sprintf_prefix(prefix_buffer, &cache->item_info);
					iobufPtr(&iov[0]) = prefix_buffer;
					iobufLen(&iov[0]) = cache->prefix_len;
				}
				else {
					iobufPtr(&iov[0]) = NULL;
					iobufLen(&iov[0]) = 0;
				}
				/* io write */
				iobufPtr(&iov[1]) = cache->txt;
				iobufLen(&iov[1]) = cache->txt_len;
				fdWritev(lf->fd, iov, sizeof(iov) / sizeof(iov[0]));
			}
			free(cache);
			if (output_thrd->exit_flag) {
				goto end;
			}
		}
	}
end:
	free(prefix_buffer);
	free_async_output_thread_cache_list(lcur);
	return 0;
}

static void log_sync_write(Log_t* log, const char* txt, size_t txt_len, LogFile_t* lf, const LogItemInfo_t* item_info) {
	criticalsectionEnter(&lf->lock);
	log_rotate(lf, &item_info->dt, item_info->timestamp_sec);
	if (lf->fd != INVALID_FD_HANDLE) {
		fdWrite(lf->fd, txt, txt_len);
	}
	criticalsectionLeave(&lf->lock);
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

static LogItemInfo_t* fill_log_item_info(LogItemInfo_t* item_info, int priority) {
	item_info->timestamp_sec = gmtimeSecond();
	if (!gmtimeLocalTM(item_info->timestamp_sec, &item_info->dt)) {
		return NULL;
	}
	structtmNormal(&item_info->dt);
	item_info->priority_str = log_get_priority_str(priority);
	return item_info;
}

static void log_async_build(Log_t* log, LogFile_t* lf, const LogItemInfo_t* item_info, const char* format, va_list ap) {
	va_list varg;
	ssize_t txt_len;
	char test_buf;
	int prefix_len;
	AsyncCacheBlock_t* cache;
	LogAsyncOutputThread_t* output_thrd;
	const LogFileOutputOption_t* opt = &lf->output_opt;
	/* calculate buffer total length */
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
	txt_len = vsnprintf(&test_buf, 0, format, varg);
	va_end(varg);
	if (txt_len <= 0) {
		return;
	}
	++txt_len;/* append '\n' */
	if (txt_len <= 0) {
		return;
	}
	/* alloc buffer */
	cache = (AsyncCacheBlock_t*)malloc(sizeof(AsyncCacheBlock_t) + txt_len);
	if (!cache) {
		return;
	}
	/* fill buffer */
	va_copy(varg, ap);
	if (vsnprintf(cache->txt, txt_len + 1, format, varg) <= 0) {
		free(cache);
		va_end(varg);
		return;
	}
	va_end(varg);
	cache->txt[txt_len - 1] = '\n';
	cache->txt[txt_len] = 0;
	cache->txt_len = txt_len;
	cache->prefix_len = prefix_len;
	cache->lf = lf;
	cache->item_info = *item_info;
	/* insert cache list */
	output_thrd = lf->output_thrd;
	dataqueuePush(&output_thrd->cache_dq, &cache->_lnode);
}

static void log_sync_build(Log_t* log, LogFile_t* lf, const LogItemInfo_t* item_info, const char* format, va_list ap) {
	va_list varg;
	ssize_t len;
	char test_buf;
	int prefix_len;
	CacheBlock_t* cache;
	const LogFileOutputOption_t* opt = &lf->output_opt;
	/* calculate buffer total length */
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
	len = vsnprintf(&test_buf, 0, format, varg);
	va_end(varg);
	if (len <= 0) {
		return;
	}
	len += prefix_len;
	++len;/* append '\n' */
	if (len <= 0) {
		return;
	}
	/* alloc buffer */
	cache = (CacheBlock_t*)malloc(sizeof(CacheBlock_t) + len);
	if (!cache) {
		return;
	}
	/* fill buffer */
	if (prefix_len > 0 && opt->fn_sprintf_prefix) {
		opt->fn_sprintf_prefix(cache->txt, item_info);
	}
	va_copy(varg, ap);
	if (vsnprintf(cache->txt + prefix_len, len - prefix_len + 1, format, varg) <= 0) {
		free(cache);
		va_end(varg);
		return;
	}
	va_end(varg);
	cache->txt[len - 1] = '\n';
	cache->txt[len] = 0;
	cache->len = len;
	/* io write */
	log_sync_write(log, cache->txt, cache->len, lf, item_info);
	free(cache);
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
	log->async_output_select_idx = 0;
	log->async_output_thrds = NULL;
	log->async_output_thrds_cnt = 0;
	return log;
}

Log_t* logEnableAsyncOuputThreads(Log_t* log, size_t thrd_cnt) {
	size_t i;
	LogAsyncOutputThread_t** thrds;
	if (log->async_output_thrds_cnt > 0) {
		return log;
	}
	thrds = (LogAsyncOutputThread_t**)malloc(sizeof(LogAsyncOutputThread_t*) * thrd_cnt);
	if (!thrds) {
		return NULL;
	}
	for (i = 0; i < thrd_cnt; ++i) {
		LogAsyncOutputThread_t* t = (LogAsyncOutputThread_t*)malloc(sizeof(LogAsyncOutputThread_t));
		if (!t) {
			goto err;
		}
		if (!init_async_output_thread(t)) {
			free(t);
			goto err;
		}
		if (!threadCreate(&t->tid, 0, log_async_output_thrd_entry, t)) {
			free(t);
			goto err;
		}
		thrds[i] = t;
	}
	log->async_output_thrds = thrds;
	log->async_output_thrds_cnt = thrd_cnt;
	return log;
err:
	while (i > 0) {
		exit_free_async_output_thread(thrds[--i]);
	}
	free(thrds);
	return NULL;
}

const LogFileOutputOption_t* logFileOutputOptionDefault(void) {
	static LogFileOutputOption_t opt = {
		default_prefix_length,
		default_sprintf_prefix,
		0
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
	if (lf->output_opt.async_output && log->async_output_thrds_cnt > 0) {
		size_t idx = (log->async_output_select_idx++) % log->async_output_thrds_cnt;
		lf->output_thrd = log->async_output_thrds[idx];
	}
	else {
		lf->output_thrd = NULL;
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

	for (i = 0; i < log->async_output_thrds_cnt; ++i) {
		exit_free_async_output_thread(log->async_output_thrds[i]);
	}
	free(log->async_output_thrds);

	for (i = 0; i < log->files_cnt; ++i) {
		free_log_file(log->files[i]);
	}
	free(log->files);

	free(log);
}

void logPrintlnNoFilter(Log_t* log, const char* key, int priority, LogItemInfo_t* ii, const char* format, ...) {
	va_list varg;
	LogFile_t* lf;

	if (!format || 0 == *format) {
		return;
	}
	lf = get_log_file(log, key);
	if (!lf) {
		return;
	}
	if (!fill_log_item_info(ii, priority)) {
		return;
	}
	va_start(varg, format);
	if (lf->output_thrd) {
		log_async_build(log, lf, ii, format, varg);
	}
	else {
		log_sync_build(log, lf, ii, format, varg);
	}
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
