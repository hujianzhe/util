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

typedef struct Log_t {
	char* pathname;
	unsigned char print_file;
	unsigned char print_stdio;
	unsigned char async_print;
	int filter_priority;
	const char* source_file;
	const char* func_name;
	unsigned int source_line;
	int(*fn_priority_filter)(int, int);
	int(*fn_prefix_length)(LogItemInfo_t*);
	void(*fn_sprintf_prefix)(char*, LogItemInfo_t*);
/* private */
	FD_t m_fd;
	size_t m_filesize;
	size_t m_maxfilesize;
	unsigned int m_filesegmentseq;
	List_t m_cachelist;
	CriticalSection_t m_lock;
} Log_t;

typedef struct CacheBlock_t {
	ListNode_t m_listnode;
	size_t len;
	char txt[1];
} CacheBlock_t;

static int log_rotate(Log_t* log) {
	FD_t fd;
	char* pathname;
	long long filesz;
	while (1) {
		pathname = strFormat(NULL, "%s.%u.txt", log->pathname, log->m_filesegmentseq);
		if (!pathname)
			return 0;
		fd = fdOpen(pathname, FILE_READ_BIT);
		if (fd != INVALID_FD_HANDLE) {
			filesz = fdGetSize(fd);
			fdClose(fd);
			if (filesz < 0) {
				free(pathname);
				return 0;
			}
		}
		else if (errnoGet() == ENOENT) {
			filesz = 0;
		}
		else {
			free(pathname);
			return 0;
		}
		log->m_filesegmentseq++;
		if (filesz < log->m_maxfilesize)
			break;
		free(pathname);
	}
	fd = fdOpen(pathname, FILE_CREAT_BIT | FILE_WRITE_BIT | FILE_APPEND_BIT);
	free(pathname);
	if (INVALID_FD_HANDLE == fd) {
		log->m_filesegmentseq--;
		return 0;
	}
	if (log->m_fd != INVALID_FD_HANDLE) {
		fdClose(log->m_fd);
	}
	log->m_fd = fd;
	log->m_filesize = filesz;
	return 1;
}

static void log_do_write_cache(Log_t* log, CacheBlock_t* cache) {
	/* force open fd */
	if (INVALID_FD_HANDLE == log->m_fd) {
		if (!log_rotate(log))
			return;
	}
	/* size rotate */
	while (log->m_maxfilesize <= log->m_filesize || cache->len > log->m_maxfilesize - log->m_filesize) {
		if (!log_rotate(log))
			break;
	}
	/* io */
	if (INVALID_FD_HANDLE != log->m_fd) {
		int res = fdWrite(log->m_fd, cache->txt, cache->len);
		if (res > 0)
			log->m_filesize += res;
	}
}

static void log_write(Log_t* log, CacheBlock_t* cache) {
	unsigned char is_async = log->async_print;

	if (log->print_stdio) {
		fputs(cache->txt, stderr);
	}

	if (log->print_file) {
		criticalsectionEnter(&log->m_lock);
		if (is_async) {
			listInsertNodeBack(&log->m_cachelist, log->m_cachelist.tail, &cache->m_listnode);
			criticalsectionLeave(&log->m_lock);
		}
		else {
			log_do_write_cache(log, cache);
			criticalsectionLeave(&log->m_lock);
			free(cache);
		}
	}
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

static int log_default_prefix_length(LogItemInfo_t* item_info) {
	return strFormatLen("%d-%d-%d %d:%d:%d|%s|%s(%u)|",
						item_info->dt.tm_year, item_info->dt.tm_mon, item_info->dt.tm_mday,
						item_info->dt.tm_hour, item_info->dt.tm_min, item_info->dt.tm_sec,
						item_info->priority_str, item_info->source_file, item_info->source_line);
}

static void log_default_sprintf_prefix(char* buf, LogItemInfo_t* item_info) {
	sprintf(buf, "%d-%d-%d %d:%d:%d|%s|%s(%u)|",
			item_info->dt.tm_year, item_info->dt.tm_mon, item_info->dt.tm_mday,
			item_info->dt.tm_hour, item_info->dt.tm_min, item_info->dt.tm_sec,
			item_info->priority_str, item_info->source_file, item_info->source_line);
}

static void log_build(Log_t* log, int priority, const char* format, va_list ap) {
	va_list varg;
	int len, res, prefix_len;
	char test_buf;
	CacheBlock_t* cache;
	LogItemInfo_t item_info;

	if (!format || 0 == *format) {
		return;
	}
	if (!gmtimeLocalTM(gmtimeSecond(), &item_info.dt)) {
		return;
	}
	structtmNormal(&item_info.dt);
	item_info.priority_str = log_get_priority_str(priority);
	item_info.source_file = log->source_file;
	item_info.source_line = log->source_line;
	item_info.func_name = log->func_name;

	prefix_len = log->fn_prefix_length(&item_info);
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
	log->fn_sprintf_prefix(cache->txt, &item_info);
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
	log_write(log, cache);
}

#ifdef	__cplusplus
extern "C" {
#endif

Log_t* logOpen(size_t maxfilesize, const char* pathname) {
	Log_t* log = (Log_t*)malloc(sizeof(Log_t));
	if (!log) {
		return NULL;
	}
	log->pathname = strdup(pathname ? pathname : "");
	if (!log->pathname) {
		free(log);
		return NULL;
	}
	if (!criticalsectionCreate(&log->m_lock)) {
		free(log->pathname);
		free(log);
		return NULL;
	}
	log->m_fd = INVALID_FD_HANDLE;
	log->m_filesize = 0;
	log->m_maxfilesize = maxfilesize;
	log->m_filesegmentseq = 0;
	listInit(&log->m_cachelist);

	if (log->pathname[0] && log->m_maxfilesize > 0) {
		log->print_file = 1;
	}
	log->print_stdio = 0;
	log->async_print = 0;
	log->filter_priority = -1;
	log->source_file = "";
	log->func_name = "";
	log->source_line = 0;
	log->fn_priority_filter = NULL;
	log->fn_prefix_length = log_default_prefix_length;
	log->fn_sprintf_prefix = log_default_sprintf_prefix;

	return log;
}

void logPrefix(Log_t* log, int(*fn_prefix_length)(LogItemInfo_t*), void(*fn_sprintf_prefix)(char*, LogItemInfo_t*)) {
	log->fn_prefix_length = fn_prefix_length;
	log->fn_sprintf_prefix = fn_sprintf_prefix;
}

void logEnableFile(struct Log_t* log, int enabled) {
	if (enabled) {
		if (!log->pathname || 0 == log->pathname[0] || log->m_maxfilesize <= 0) {
			return;
		}
	}
	log->print_file = enabled;
}

void logEnableStdio(Log_t* log, int enabled) {
	log->print_stdio = enabled;
}

void logFlush(Log_t* log) {
	ListNode_t *cur, *next;

	criticalsectionEnter(&log->m_lock);

	cur = log->m_cachelist.head;
	listInit(&log->m_cachelist);

	criticalsectionLeave(&log->m_lock);

	for (; cur; cur = next) {
		CacheBlock_t* cache = pod_container_of(cur, CacheBlock_t, m_listnode);
		next = cur->next;
		log_do_write_cache(log, cache);
		free(cache);
	}
}

void logClear(Log_t* log) {
	ListNode_t *cur, *next;

	criticalsectionEnter(&log->m_lock);

	cur = log->m_cachelist.head;
	listInit(&log->m_cachelist);

	criticalsectionLeave(&log->m_lock);

	for (; cur; cur = next) {
		next = cur->next;
		free(pod_container_of(cur, CacheBlock_t, m_listnode));
	}
}

void logDestroy(Log_t* log) {
	if (log) {
		logClear(log);
		criticalsectionClose(&log->m_lock);
		free(log->pathname);
		if (INVALID_FD_HANDLE != log->m_fd) {
			fdClose(log->m_fd);
		}
		free(log);
	}
}

void logPrintRawNoFilter(Log_t* log, int priority, const char* format, ...) {
	va_list varg;
	int len;
	char test_buf;
	CacheBlock_t* cache;

	if (!format || 0 == *format)
		return;
	va_start(varg, format);
	len = vsnprintf(&test_buf, 0, format, varg);
	va_end(varg);
	if (len <= 0)
		return;
	cache = (CacheBlock_t*)malloc(sizeof(CacheBlock_t) + len);
	if (!cache)
		return;
	va_start(varg, format);
	len = vsnprintf(cache->txt, len + 1, format, varg);
	va_end(varg);
	if (len <= 0) {
		free(cache);
		return;
	}
	cache->txt[len] = 0;
	cache->len = len;
	log_write(log, cache);
}

void logPrintlnNoFilter(Log_t* log, int priority, const char* format, ...) {
	va_list varg;
	va_start(varg, format);
	log_build(log, priority, format, varg);
	va_end(varg);
}

void logPrintRaw(Log_t* log, int priority, const char* format, ...) {
	va_list varg;
	int len;
	char test_buf;
	CacheBlock_t* cache;

	if (!format || 0 == *format)
		return;
	if (logCheckPriorityFilter(log, priority))
		return;
	va_start(varg, format);
	len = vsnprintf(&test_buf, 0, format, varg);
	va_end(varg);
	if (len <= 0)
		return;
	cache = (CacheBlock_t*)malloc(sizeof(CacheBlock_t) + len);
	if (!cache)
		return;
	va_start(varg, format);
	len = vsnprintf(cache->txt, len + 1, format, varg);
	va_end(varg);
	if (len <= 0) {
		free(cache);
		return;
	}
	cache->txt[len] = 0;
	cache->len = len;
	log_write(log, cache);
}

void logPrintln(Log_t* log, int priority, const char* format, ...) {
	va_list varg;
	if (logCheckPriorityFilter(log, priority)) {
		return;
	}
	va_start(varg, format);
	log_build(log, priority, format, varg);
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
	log->filter_priority = filter_priority;
	log->fn_priority_filter = fn_priority_filter;
}

int logCheckPriorityFilter(Log_t* log, int priority) {
	if (log->fn_priority_filter && log->fn_priority_filter(priority, log->filter_priority)) {
		return 1;
	}
	return 0;
}

Log_t* logSaveSourceFile(Log_t* log, const char* source_file, const char* func_name, unsigned int source_line) {
	log->source_file = source_file;
	log->func_name = func_name;
	log->source_line = source_line;
	return log;
}

#ifdef	__cplusplus
}
#endif
