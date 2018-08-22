//
// Created by hujianzhe
//

#include "../syslib/file.h"
#include "../syslib/process.h"
#include "../syslib/time.h"
#include "log.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct CacheBlock {
	list_node_t m_listnode;
	struct tm dt;
	size_t len;
	char txt[1];
} CacheBlock;

#ifdef	__cplusplus
extern "C" {
#endif

static void log_rotate(Log_t* log, const struct tm* dt, int trunc) {
	char path[256];
	if (log->m_file != INVALID_FD_HANDLE) {
		fdClose(log->m_file);
		log->m_file = INVALID_FD_HANDLE;
	}
	log->m_filesize = 0;
	snprintf(path, sizeof(path), "%s.%d-%d-%d.txt", log->path, dt->tm_year, dt->tm_mon, dt->tm_mday);
	log->m_file = fdOpen(path, FILE_CREAT_BIT|FILE_WRITE_BIT|FILE_APPEND_BIT|(trunc ? FILE_TRUNC_BIT : 0));
	if (log->m_file != INVALID_FD_HANDLE) {
		log->m_filesize = fdGetSize(log->m_file);
	}
}

static void log_write(Log_t* log, const struct tm* dt, const char* content, size_t len) {
	if (log->print_stderr) {
		fputs(content, stderr);
	}
	if (log->print_file) {
		CacheBlock* cache = NULL;
		unsigned char is_async = log->async_print_file;
		if (is_async) {
			cache = (CacheBlock*)malloc(sizeof(CacheBlock) + len);
			if (!cache) {
				return;
			}
			cache->dt = *dt;
			cache->len = len;
			memcpy(cache->txt, content, len);
			cache->txt[len] = 0;
		}

		criticalsectionEnter(&log->m_lock);

		if (is_async) {
			list_insert_node_back(&log->m_cachelist, log->m_cachelist.tail, &cache->m_listnode);
		}
		else {
			/* day rotate */
			if (log->m_days != dt->tm_yday) {
				log->m_days = dt->tm_yday;
				log_rotate(log, dt, 0);
			}
			/* size rotate */
			else if (log->m_filesize + len >= log->m_maxfilesize) {
				log_rotate(log, dt, 1);
			}
			/* io */
			if (INVALID_FD_HANDLE != log->m_file) {
				fdWrite(log->m_file, content, len);
			}
			log->m_filesize += len;
		}

		criticalsectionLeave(&log->m_lock);
	}
}

static void log_build(Log_t* log, const char* priority, const char* format, va_list varg) {
	int len, res;
	char buffer[2048];
	struct tm dt;

	if (!format || 0 == *format) {
		return;
	}
	if (!structtmMake(gmtimeSecond(), &dt)) {
		return;
	}
	structtmNormal(&dt);
	res = snprintf(buffer, sizeof(buffer), "%s|%d-%d-%d %d:%d:%d|%zu|%s|",
					log->ident,
					dt.tm_year, dt.tm_mon, dt.tm_mday,
					dt.tm_hour, dt.tm_min, dt.tm_sec,
					log->m_pid, priority);
	if (res <= 0 || res >= sizeof(buffer)) {
		return;
	}
	len = res;
	res = vsnprintf(buffer + len, sizeof(buffer) - len, format, varg);
	if (res <= 0) {
		return;
	}
	len += res;

	log_write(log, &dt, buffer, len);
}

Log_t* logInit(Log_t* log) {
	log->m_initok = 0;
	if (!criticalsectionCreate(&log->m_lock))
		return NULL;
	log->m_pid = processId();
	log->m_days = -1;
	log->m_file = INVALID_FD_HANDLE;
	log->m_filesize = 0;
	log->m_maxfilesize = ~0;
	list_init(&log->m_cachelist);
	log->m_initok = 1;

	log->print_stderr = 0;
	log->print_file = 0;
	log->async_print_file = 0;

	return log;
}

void logFlush(Log_t* log) {
	char *txt = NULL;
	size_t txtlen = 0;
	list_node_t *cur, *next;

	criticalsectionEnter(&log->m_lock);

	cur = log->m_cachelist.head;
	list_init(&log->m_cachelist);

	criticalsectionLeave(&log->m_lock);

	for (; cur; cur = next) {
		char *p;
		CacheBlock* cache = pod_container_of(cur, CacheBlock, m_listnode);
		next = cur->next;
		/* day rotate */
		if (log->m_days != cache->dt.tm_yday) {
			log->m_days = cache->dt.tm_yday;
			if (txt && log->m_file != INVALID_FD_HANDLE) {
				fdWrite(log->m_file, txt, txtlen);
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
	if (log->m_file != INVALID_FD_HANDLE && txt) {
		fdWrite(log->m_file, txt, txtlen);
	}
	free(txt);
}

void logClear(Log_t* log) {
	list_node_t *cur, *next;

	criticalsectionEnter(&log->m_lock);

	cur = log->m_cachelist.head;
	list_init(&log->m_cachelist);

	criticalsectionLeave(&log->m_lock);

	for (; cur; cur = next) {
		next = cur->next;
		free(pod_container_of(cur, CacheBlock, m_listnode));
	}
}

void logDestroy(Log_t* log) {
	if (log && log->m_initok) {
		logClear(log);
		criticalsectionClose(&log->m_lock);
		if (INVALID_FD_HANDLE != log->m_file)
			fdClose(log->m_file);
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
