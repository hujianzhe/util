//
// Created by hujianzhe
//

#ifndef	UTIL_C_COMPONENT_LOG_H
#define	UTIL_C_COMPONENT_LOG_H

#include "../datastruct/list.h"
#include "../sysapi/ipc.h"
#include <stddef.h>

typedef struct Log_t {
	char ident[64];
	char* pathname;
	unsigned char print_stderr;
	unsigned char async_print_file;
	int(*fn_priority_filter)(int priority);
/* private */
	unsigned char m_initok;
	FD_t m_fd;
	size_t m_filesize;
	size_t m_maxfilesize;
	unsigned int m_filesegmentseq;
	List_t m_cachelist;
	CriticalSection_t m_lock;
} Log_t;

#ifdef	__cplusplus
extern "C" {
#endif

__declspec_dll Log_t* logInit(Log_t* log, size_t maxfilesize, const char ident[64], const char* pathname);
__declspec_dll void logFlush(Log_t* log);
__declspec_dll void logClear(Log_t* log);
__declspec_dll void logDestroy(Log_t* log);

__declspec_dll void logPrintRawNoFilter(Log_t* log, int priority, const char* format, ...);
__declspec_dll void logPrintlnNoFilter(Log_t* log, int priority, const char* format, ...);
__declspec_dll void logPrintRaw(Log_t* log, int priority, const char* format, ...);
__declspec_dll void logPrintln(Log_t* log, int priority, const char* format, ...);

#define logPrintlnTempletePrivate(log, priority, format, ...)	\
if (!(log)->fn_priority_filter || !(log)->fn_priority_filter(priority))	\
	logPrintlnNoFilter(log, priority, "%s(%d)|" format, __FILE__, __LINE__, ##__VA_ARGS__)

#define	logEmerg(log, format, ...)	logPrintlnTempletePrivate(log, 0, format, ##__VA_ARGS__)
#define	logAlert(log, format, ...)	logPrintlnTempletePrivate(log, 1, format, ##__VA_ARGS__)
#define	logCrit(log, format, ...)	logPrintlnTempletePrivate(log, 2, format, ##__VA_ARGS__)
#define	logErr(log, format, ...)	logPrintlnTempletePrivate(log, 3, format, ##__VA_ARGS__)
#define	logWarn(log, format, ...)	logPrintlnTempletePrivate(log, 4, format, ##__VA_ARGS__)
#define	logNotice(log, format, ...)	logPrintlnTempletePrivate(log, 5, format, ##__VA_ARGS__)
#define	logInfo(log, format, ...)	logPrintlnTempletePrivate(log, 6, format, ##__VA_ARGS__)
#define	logDebug(log, format, ...)	logPrintlnTempletePrivate(log, 7, format, ##__VA_ARGS__)

#ifdef	__cplusplus
}
#endif

#endif
