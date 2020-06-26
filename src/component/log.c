//
// Created by hujianzhe
//

#include "../../inc/sysapi/error.h"
#include "../../inc/sysapi/file.h"
#include "../../inc/sysapi/misc.h"
#include "../../inc/sysapi/time.h"
#include "../../inc/component/log.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct CacheBlock_t {
	ListNode_t m_listnode;
	size_t len;
	char txt[1];
} CacheBlock_t;

#ifdef	__cplusplus
extern "C" {
#endif

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
	unsigned char is_async = log->async_print_file;

	if (log->print_stderr) {
		fputs(cache->txt, stderr);
	}

	criticalsectionEnter(&log->m_lock);

	if (is_async) {
		listInsertNodeBack(&log->m_cachelist, log->m_cachelist.tail, &cache->m_listnode);
	}
	else {
		log_do_write_cache(log, cache);
	}

	criticalsectionLeave(&log->m_lock);

	if (!is_async)
		free(cache);
}

#define log_build(log, priority, format) do {\
	va_list varg;\
	int len, res;\
	char test_buf;\
	CacheBlock_t* cache;\
	struct tm dt;\
\
	if (!format || 0 == *format)\
		return;\
	if (log->fn_priority_filter && log->fn_priority_filter(priority))\
		return;\
	if (!structtmMake(gmtimeSecond(), &dt))\
		return;\
	structtmNormal(&dt);\
	res = strFormatLen("%s|%d-%d-%d %d:%d:%d|%s|",\
						log->ident,\
						dt.tm_year, dt.tm_mon, dt.tm_mday,\
						dt.tm_hour, dt.tm_min, dt.tm_sec,\
						priority);\
	if (res <= 0)\
		return;\
	len = res;\
	va_start(varg, format);\
	res = vsnprintf(&test_buf, 0, format, varg);\
	va_end(varg);\
	if (res <= 0)\
		return;\
	len += res;\
	++len;/* append '\n' */\
	cache = (CacheBlock_t*)malloc(sizeof(CacheBlock_t) + len);\
	if (!cache)\
		return;\
	res = snprintf(cache->txt, len + 1, "%s|%d-%d-%d %d:%d:%d|%s|",\
					log->ident,\
					dt.tm_year, dt.tm_mon, dt.tm_mday,\
					dt.tm_hour, dt.tm_min, dt.tm_sec,\
					priority);\
	if (res <= 0 || res >= len) {\
		free(cache);\
		return;\
	}\
	va_start(varg, format);\
	res = vsnprintf(cache->txt + res, len - res + 1, format, varg);\
	va_end(varg);\
	if (res <= 0) {\
		free(cache);\
		return;\
	}\
	cache->txt[len - 1] = '\n';\
	cache->txt[len] = 0;\
	cache->len = len;\
	log_write(log, cache);\
} while (0)

Log_t* logInit(Log_t* log, const char ident[64], const char* pathname) {
	log->m_initok = 0;
	log->pathname = strdup(pathname);
	if (!log->pathname)
		return NULL;
	if (!criticalsectionCreate(&log->m_lock)) {
		free(log->pathname);
		return NULL;
	}
	log->m_fd = INVALID_FD_HANDLE;
	log->m_filesize = 0;
	log->m_maxfilesize = ~0;
	log->m_filesegmentseq = 0;
	listInit(&log->m_cachelist);
	log->m_initok = 1;

	strncpy(log->ident, ident, sizeof(log->ident) - 1);
	log->ident[sizeof(log->ident) - 1] = 0;
	log->print_stderr = 0;
	log->async_print_file = 0;
	log->fn_priority_filter = NULL;

	return log;
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
	if (log && log->m_initok) {
		logClear(log);
		criticalsectionClose(&log->m_lock);
		free(log->pathname);
		if (INVALID_FD_HANDLE != log->m_fd)
			fdClose(log->m_fd);
	}
}

void logPrintRaw(Log_t* log, const char* priority, const char* format, ...) {
	va_list varg;
	int len;
	char test_buf;
	CacheBlock_t* cache;

	if (!format || 0 == *format)
		return;
	if (log->fn_priority_filter && log->fn_priority_filter(priority ? priority : ""))
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

void logPrintln(Log_t* log, const char* priority, const char* format, ...) {
	if (!priority)
		priority = "";
	log_build(log, priority, format);
}

void logEmerg(Log_t* log, const char* format, ...) {
	log_build(log, "EMERG", format);
}
void logAlert(Log_t* log, const char* format, ...) {
	log_build(log, "ALERT", format);
}
void logCrit(Log_t* log, const char* format, ...) {
	log_build(log, "CRIT", format);
}
void logErr(Log_t* log, const char* format, ...) {
	log_build(log, "ERR", format);
}
void logWarning(Log_t* log, const char* format, ...) {
	log_build(log, "WARNING", format);
}
void logNotice(Log_t* log, const char* format, ...) {
	log_build(log, "NOTICE", format);
}
void logInfo(Log_t* log, const char* format, ...) {
	log_build(log, "INFO", format);
}
void logDebug(Log_t* log, const char* format, ...) {
	log_build(log, "DEBUG", format);
}

#ifdef	__cplusplus
}
#endif
