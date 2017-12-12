//
// Created by hujianzhe on 17-3-6.
//

#include "../c/syslib/file.h"
#include "../c/syslib/process.h"
#include "../c/syslib/time.h"
#include "logger.h"
#include <iterator>
#include <stdio.h>
#include <string.h>

namespace Util {
Logger::Logger(void) :
	m_async(false),
	m_inputOption(InputOption::CONSOLE),
	m_pid((size_t)process_Id()),
	m_days(-1),
	m_file(INVALID_FD_HANDLE),
	m_filesize(0),
	m_maxfilesize(~0)
{
	m_ident[0] = 0;
	m_path[0] = 0;
	cslock_Create(&m_lock);
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
const char* Logger::ident(void) { return m_ident; }

void Logger::enableAsync(bool b) { m_async = b; }

void Logger::inputOption(int o) { m_inputOption = o; }
short Logger::inputOption(void) { return m_inputOption; }

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
	if (m_async) {
		std::vector<char> cache;
		assert_true(cslock_Enter(&m_lock, TRUE) == EXEC_SUCCESS);
		for (auto it = m_caches.begin(); it != m_caches.end(); m_caches.erase(it++)) {
			// day rotate
			if (m_days != it->first.tm_yday) {
				m_days = it->first.tm_yday;
				if (!cache.empty() && m_file != INVALID_FD_HANDLE) {
					file_Write(m_file, &cache[0], cache.size());
				}
				cache.clear();
				rotate(&it->first, false);
			}
			// size rotate
			else if (m_filesize + it->second.size() >= m_maxfilesize) {
				cache.clear();
				rotate(&it->first, true);
			}
			// copy data to cache
			std::copy(it->second.begin(), it->second.end(), std::back_inserter(cache));
			m_filesize += it->second.size();
		}
		assert_true(cslock_Leave(&m_lock) == EXEC_SUCCESS);
		// io
		if (m_file != INVALID_FD_HANDLE && !cache.empty()) {
			file_Write(m_file, &cache[0], cache.size());
		}
	}
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
	if (snprintf(path, sizeof(path), "%s%s.%d-%d-%d.txt", m_path, ident(), dt->tm_year, dt->tm_mon, dt->tm_mday) < 0) {
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
		assert_true(cslock_Enter(&m_lock, TRUE) == EXEC_SUCCESS);
		if (m_async) {
			m_caches.push_back(std::make_pair(*dt, std::vector<char>(content, content + len)));
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
		assert_true(cslock_Leave(&m_lock) == EXEC_SUCCESS);
	}
}
}
