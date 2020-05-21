//
// Created by hujianzhe
//

#include "../../inc/sysapi/file.h"
#include "../../inc/sysapi/misc.h"
#include "../../inc/sysapi/process.h"
#include "../../inc/sysapi/time.h"
#include "../../inc/component/log.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct CacheBlock_t {
	ListNode_t m_listnode;
	struct tm dt;
	size_t len;
	char txt[1];
} CacheBlock_t;

#ifdef	__cplusplus
extern "C" {
#endif

static void log_rotate(Log_t* log, const struct tm* dt, int trunc) {
	FD_t fd;
	char* pathname;
	while (1) {
		pathname = strFormat(NULL, "%s.%d-%d-%d.%u.txt", log->pathname, dt->tm_year, dt->tm_mon, dt->tm_mday, log->m_filesegmentseq);
		if (!pathname)
			return;
		if (!fileIsExist(pathname))
			break;
		log->m_filesegmentseq++;
		free(pathname);
	}
	fd = fdOpen(pathname, FILE_CREAT_BIT | FILE_WRITE_BIT | FILE_APPEND_BIT | (trunc ? FILE_TRUNC_BIT : 0));
	free(pathname);
	if (INVALID_FD_HANDLE == fd)
		return;
	if (log->m_fd != INVALID_FD_HANDLE) {
		fdClose(log->m_fd);
	}
	log->m_fd = fd;
	if (trunc)
		log->m_filesize = 0;
	else
		log->m_filesize = fdGetSize(fd);
}

static void log_write(Log_t* log, CacheBlock_t* cache) {
	if (log->print_stderr) {
		fputs(cache->txt, stderr);
	}
	if (log->print_file) {
		unsigned char is_async = log->async_print_file;
		struct tm* dt = &cache->dt;

		criticalsectionEnter(&log->m_lock);

		if (is_async) {
			listInsertNodeBack(&log->m_cachelist, log->m_cachelist.tail, &cache->m_listnode);
		}
		else {
			/* day rotate */
			if (log->m_days != dt->tm_yday) {
				log->m_days = dt->tm_yday;
				log_rotate(log, dt, 0);
			}
			/* size rotate */
			else if (log->m_filesize + cache->len >= log->m_maxfilesize) {
				log_rotate(log, dt, 1);
			}
			/* io */
			if (INVALID_FD_HANDLE != log->m_fd) {
				fdWrite(log->m_fd, cache->txt, cache->len);
			}
			log->m_filesize += cache->len;
		}

		criticalsectionLeave(&log->m_lock);

		if (!is_async)
			free(cache);
	}
}

static void log_build(Log_t* log, const char* priority, const char* format, va_list varg) {
	int len, res;
	char test_buf;
	CacheBlock_t* cache;
	struct tm dt;

	if (!format || 0 == *format) {
		return;
	}
	if (!structtmMake(gmtimeSecond(), &dt)) {
		return;
	}
	structtmNormal(&dt);
	res = strFormatLen("%s|%d-%d-%d %d:%d:%d|%zu|%s|",
						log->ident,
						dt.tm_year, dt.tm_mon, dt.tm_mday,
						dt.tm_hour, dt.tm_min, dt.tm_sec,
						log->m_pid, priority);
	if (res <= 0)
		return;
	len = res;
	res = vsnprintf(&test_buf, 0, format, varg);
	if (res <= 0)
		return;
	len += res;

	cache = (CacheBlock_t*)malloc(sizeof(CacheBlock_t) + len);
	if (!cache)
		return;
	cache->dt = dt;
	cache->len = len;

	res = snprintf(cache->txt, cache->len, "%s|%d-%d-%d %d:%d:%d|%zu|%s|",
					log->ident,
					dt.tm_year, dt.tm_mon, dt.tm_mday,
					dt.tm_hour, dt.tm_min, dt.tm_sec,
					log->m_pid, priority);
	if (res <= 0 || res >= cache->len) {
		free(cache);
		return;
	}
	res = vsnprintf(cache->txt + res, cache->len - res, format, varg);
	if (res <= 0) {
		free(cache);
		return;
	}
	cache->txt[cache->len] = 0;

	log_write(log, cache);
}

Log_t* logInit(Log_t* log, const char ident[64], const char* pathname) {
	log->m_initok = 0;
	log->pathname = strdup(pathname);
	if (!log->pathname)
		return NULL;
	if (!criticalsectionCreate(&log->m_lock)) {
		free(log->pathname);
		return NULL;
	}
	log->m_pid = processId();
	log->m_days = -1;
	log->m_fd = INVALID_FD_HANDLE;
	log->m_filesize = 0;
	log->m_maxfilesize = ~0;
	log->m_filesegmentseq = 0;
	listInit(&log->m_cachelist);
	log->m_initok = 1;

	strncpy(log->ident, ident, sizeof(log->ident) - 1);
	log->ident[sizeof(log->ident) - 1] = 0;
	log->print_stderr = 0;
	log->print_file = 1;
	log->async_print_file = 0;

	return log;
}

void logFlush(Log_t* log) {
	char *txt = NULL;
	size_t txtlen = 0;
	ListNode_t *cur, *next;

	criticalsectionEnter(&log->m_lock);

	cur = log->m_cachelist.head;
	listInit(&log->m_cachelist);

	criticalsectionLeave(&log->m_lock);

	for (; cur; cur = next) {
		char *p;
		CacheBlock_t* cache = pod_container_of(cur, CacheBlock_t, m_listnode);
		next = cur->next;
		/* day rotate */
		if (log->m_days != cache->dt.tm_yday) {
			log->m_days = cache->dt.tm_yday;
			if (txt && log->m_fd != INVALID_FD_HANDLE) {
				fdWrite(log->m_fd, txt, txtlen);
			}
			free(txt);
			txt = NULL;
			txtlen = 0;
			log_rotate(log, &cache->dt, 0);
		}
		/* size rotate */
		else if (cache->len >= log->m_maxfilesize - log->m_filesize) {
			free(txt);
			txt = NULL;
			txtlen = 0;
			log_rotate(log, &cache->dt, 1);
		}
		/* copy data to cache */
		p = (char*)realloc(txt, txtlen + cache->len);
		if (p) {
			txt = p;
			memcpy(txt + txtlen, cache->txt, cache->len);
			txtlen += cache->len;
			log->m_filesize += cache->len;
		}
		else {
			log->m_filesize -= txtlen;
			free(txt);
			txt = NULL;
			txtlen = 0;
		}
		free(cache);
	}
	/* io */
	if (log->m_fd != INVALID_FD_HANDLE && txt) {
		fdWrite(log->m_fd, txt, txtlen);
	}
	free(txt);
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

void logEmerg(Log_t* log, const char* format, ...) {
	va_list varg;
	va_start(varg, format);
	log_build(log, "EMERG", format, varg);
	va_end(varg);
}
void logAlert(Log_t* log, const char* format, ...) {
	va_list varg;
	va_start(varg, format);
	log_build(log, "ALERT", format, varg);
	va_end(varg);
}
void logCrit(Log_t* log, const char* format, ...) {
	va_list varg;
	va_start(varg, format);
	log_build(log, "CRIT", format, varg);
	va_end(varg);
}
void logErr(Log_t* log, const char* format, ...) {
	va_list varg;
	va_start(varg, format);
	log_build(log, "ERR", format, varg);
	va_end(varg);
}
void logWarning(Log_t* log, const char* format, ...) {
	va_list varg;
	va_start(varg, format);
	log_build(log, "WARNING", format, varg);
	va_end(varg);
}
void logNotice(Log_t* log, const char* format, ...) {
	va_list varg;
	va_start(varg, format);
	log_build(log, "NOTICE", format, varg);
	va_end(varg);
}
void logInfo(Log_t* log, const char* format, ...) {
	va_list varg;
	va_start(varg, format);
	log_build(log, "INFO", format, varg);
	va_end(varg);
}
void logDebug(Log_t* log, const char* format, ...) {
	va_list varg;
	va_start(varg, format);
	log_build(log, "DEBUG", format, varg);
	va_end(varg);
}

#ifdef	__cplusplus
}
#endif
