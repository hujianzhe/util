//
// Created by hujianzhe
//

#ifndef	UTIL_C_COMPONENT_LOG_H
#define	UTIL_C_COMPONENT_LOG_H

#include "../datastruct/list.h"
#include "../syslib/ipc.h"
#include <stddef.h>

typedef struct Log_t {
	char ident[64];
	char rootpath[256], name[64];
	unsigned char print_stderr;
	unsigned char print_file;
	unsigned char async_print_file;
/* private */
	unsigned char m_initok;
	size_t m_pid;
	int m_days;
	FD_t m_file;
	size_t m_filesize;
	size_t m_maxfilesize;
	list_t m_cachelist;
	CriticalSection_t m_lock;
} Log_t;

#ifdef	__cplusplus
extern "C" {
#endif

Log_t* logInit(Log_t* log);
void logFlush(Log_t* log);
void logClear(Log_t* log);
void logDestroy(Log_t* log);

void logEmerg(Log_t* log, const char* format, ...);
void logAlert(Log_t* log, const char* format, ...);
void logCrit(Log_t* log, const char* format, ...);
void logErr(Log_t* log, const char* format, ...);
void logWarning(Log_t* log, const char* format, ...);
void logNotice(Log_t* log, const char* format, ...);
void logInfo(Log_t* log, const char* format, ...);
void logDebug(Log_t* log, const char* format, ...);

#ifdef	__cplusplus
}
#endif

#endif
