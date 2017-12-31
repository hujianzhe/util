//
// Created by hujianzhe on 17-3-6.
//

#include "../c/syslib/file.h"
#include "../c/syslib/process.h"
#include "../c/syslib/time.h"
#include "logger.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

namespace Util {
Logger::Logger(void) :
	m_async(false),
	m_inputOption(InputOption::CONSOLE),
	m_pid(process_Id()),
	m_days(-1),
	m_file(INVALID_FD_HANDLE),
	m_filesize(0),
	m_maxfilesize(~0),
	m_cachehead(NULL),
	m_cachetail(NULL)
{
	m_ident[0] = 0;
	m_path[0] = 0;
	assert_true(cslock_Create(&m_lock) == EXEC_SUCCESS);
}
Logger::~Logger(void) {
	if (m_file != INVALID_FD_HANDLE) {
		file_Close(m_file);
	}
	cslock_Close(&m_lock);
}

void Logger::ident(const char* format, ...) {
	va_list varg;
	va_start(varg, format);
	vsnprintf(m_ident, sizeof(m_ident), format, varg);
	va_end(varg);
}

void Logger::path(const char* format, ...) {
	va_list varg;
	va_start(varg, format);
	vsnprintf(m_path, sizeof(m_path), format, varg);
	va_end(varg);
}
const char* Logger::path(void) { return m_path; }

void Logger::emerg(const char* format, ...) {
	va_list varg;
	va_start(varg, format);
	build("EMERG", format, varg);
	va_end(varg);
}
void Logger::alert(const char* format, ...) {
	va_list varg;
	va_start(varg, format);
	build("ALERT", format, varg);
	va_end(varg);
}
void Logger::crit(const char* format, ...) {
	va_list varg;
	va_start(varg, format);
	build("CRIT", format, varg);
	va_end(varg);
}
void Logger::err(const char* format, ...) {
	va_list varg;
	va_start(varg, format);
	build("ERR", format, varg);
	va_end(varg);
}
void Logger::warning(const char* format, ...) {
	va_list varg;
	va_start(varg, format);
	build("WARNING", format, varg);
	va_end(varg);
}
void Logger::notice(const char* format, ...) {
	va_list varg;
	va_start(varg, format);
	build("NOTICE", format, varg);
	va_end(varg);
}
void Logger::info(const char* format, ...) {
	va_list varg;
	va_start(varg, format);
	build("INFO", format, varg);
	va_end(varg);
}
void Logger::debug(const char* format, ...) {
	va_list varg;
	va_start(varg, format);
	build("DEBUG", format, varg);
	va_end(varg);
}

void Logger::flush(void) {
	list_node_t* cachehead;

	cslock_Enter(&m_lock);

	cachehead = m_cachehead;
	m_cachehead = m_cachetail = NULL;

	cslock_Leave(&m_lock);

	char* txt = NULL;
	size_t txtlen = 0;
	for (list_node_t* node = cachehead; node; ) {
		list_node_t* next_node = node->next;
		CacheBlock* cache = field_container(node, CacheBlock, m_listnode);
		// day rotate
		if (m_days != cache->dt.tm_yday) {
			m_days = cache->dt.tm_yday;
			if (txt && m_file != INVALID_FD_HANDLE) {
				file_Write(m_file, txt, txtlen);
			}
			free(txt);
			txt = NULL;
			txtlen = 0;
			rotate(&cache->dt, false);
		}
		// size rotate
		else if (m_filesize + cache->len >= m_maxfilesize) {
			free(txt);
			txt = NULL;
			txtlen = 0;
			rotate(&cache->dt, true);
		}
		// copy data to cache
		char* p = (char*)realloc(txt, txtlen + cache->len);
		if (p) {
			txt = p;
			memcpy(txt + txtlen, cache->txt, cache->len);
			txtlen += cache->len;
			m_filesize += cache->len;
		}
		else {
			free(txt);
			txt = NULL;
			txtlen = 0;
		}
		free(cache);

		node = next_node;
	}
	// io
	if (m_file != INVALID_FD_HANDLE && txt) {
		file_Write(m_file, txt, txtlen);
	}
	free(txt);
}

void Logger::build(const char* priority, const char* format, va_list varg) {
	char buffer[2048];
	struct tm dt;
	if (!format || 0 == *format) {
		return;
	}
	if (!mktm(gmt_Second(), &dt)) {
		return;
	}
	normal_tm(&dt);
	int len = 0, res;
	res = snprintf(buffer, sizeof(buffer), "%s|%d-%d-%d %d:%d:%d|%zu|%s|",
					m_ident,
					dt.tm_year, dt.tm_mon, dt.tm_mday,
					dt.tm_hour, dt.tm_min, dt.tm_sec,
					m_pid, priority);
	if (res <= 0 || res >= sizeof(buffer)) {
		return;
	}
	len += res;
	res = vsnprintf(buffer + len, sizeof(buffer) - len, format, varg);
	if (res <= 0) {
		return;
	}
	len += res;

	onWrite(&dt, buffer, len);
}

void Logger::rotate(const struct tm* dt, bool trunc) {
	if (m_file != INVALID_FD_HANDLE) {
		file_Close(m_file);
		m_file = INVALID_FD_HANDLE;
	}
	m_filesize = 0;
	char path[256];
	if (snprintf(path, sizeof(path), "%s%s.%d-%d-%d.txt", m_path, m_ident, dt->tm_year, dt->tm_mon, dt->tm_mday) < 0) {
		return;
	}
	m_file = file_Open(path, FILE_CREAT_BIT|FILE_WRITE_BIT|FILE_APPEND_BIT|(trunc ? FILE_TRUNC_BIT : 0));
	if (m_file != INVALID_FD_HANDLE) {
		m_filesize = file_Size(m_file);
	}
}

void Logger::onWrite(const struct tm* dt, const char* content, size_t len) {
	if (m_inputOption & InputOption::CONSOLE) {
		fputs(content, stderr);
	}
	if (m_inputOption & InputOption::FILE) {
		CacheBlock* cache = NULL;
		bool is_async = m_async;
		if (is_async) {
			cache = (CacheBlock*)malloc(sizeof(CacheBlock) + len);
			if (!cache) {
				return;
			}
			cache->dt = *dt;
			cache->len = len;
			memcpy(cache->txt, content, len);
		}

		cslock_Enter(&m_lock);

		if (is_async) {
			if (m_cachetail) {
				list_node_insert_back(m_cachetail, &cache->m_listnode);
				m_cachetail = &cache->m_listnode;
			}
			else {
				list_node_init(&cache->m_listnode);
				m_cachehead = m_cachetail = &cache->m_listnode;
			}
		}
		else {
			// day rotate
			if (m_days != dt->tm_yday) {
				m_days = dt->tm_yday;
				rotate(dt, false);
			}
			// size rotate
			else if (m_filesize + len >= m_maxfilesize) {
				rotate(dt, true);
			}
			// io
			if (INVALID_FD_HANDLE != m_file) {
				file_Write(m_file, content, len);
			}
			m_filesize += len;
		}

		cslock_Leave(&m_lock);
	}
}
}
