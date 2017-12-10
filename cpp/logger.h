//
// Created by hujianzhe on 17-3-6.
//

#ifndef	UTIL_CPP_LOGGER_H
#define	UTIL_CPP_LOGGER_H

#include "../c/syslib/ipc.h"
#include <list>
#include <utility>
#include <vector>
#include <stdarg.h>
#include <stddef.h>
#include <time.h>

namespace Util {
class Logger {
public:
	/*
	struct Priority { enum {
		EMERG,
		ALERT,
		CRIT,
		ERR,
		WARNING,
		NOTICE,
		INFO,
		DEBUG
	};};
	*/
	struct InputOption { enum {
		CONSOLE	= 1 << 0,
		FILE	= 1 << 1
	};};

	Logger(void);
	virtual ~Logger(void);

	void ident(const char* format, ...);
	const char* ident(void);

	void enableAsync(bool b);

	void inputOption(int o);
	short inputOption(void);

	void path(const char* format, ...);
	const char* path(void);

	void emerg(const char* format, ...);
	void alert(const char* format, ...);
	void crit(const char* format, ...);
	void err(const char* format, ...);
	void warning(const char* format, ...);
	void notice(const char* format, ...);
	void info(const char* format, ...);
	void debug(const char* format, ...);

	void flush(void);

private:
	void rotate(const struct tm* dt, bool trunc);

	void build(const char* priority, const char* format, va_list varg);
	virtual void onWrite(const struct tm* dt, const char* content, size_t len);

private:
	bool m_async;
	short m_inputOption;
	size_t m_pid;
	char m_ident[64];
	char m_path[64];
	int m_days;
	FD_HANDLE m_file;
	size_t m_filesize;
	size_t m_maxfilesize;
	std::list<std::pair<struct tm, std::vector<char> > > m_caches;
	CSLOCK m_lock;
};
}

#endif
