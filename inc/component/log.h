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
	unsigned char print_file;
	unsigned char async_print_file;
/* private */
	unsigned char m_initok;
	size_t m_pid;
	int m_days;
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

__declspec_dll Log_t* logInit(Log_t* log, const char ident[64], const char* pathname);
__declspec_dll void logFlush(Log_t* log);
__declspec_dll void logClear(Log_t* log);
__declspec_dll void logDestroy(Log_t* log);

__declspec_dll void logEmerg(Log_t* log, const char* format, ...);
__declspec_dll void logAlert(Log_t* log, const char* format, ...);
__declspec_dll void logCrit(Log_t* log, const char* format, ...);
__declspec_dll void logErr(Log_t* log, const char* format, ...);
__declspec_dll void logWarning(Log_t* log, const char* format, ...);
__declspec_dll void logNotice(Log_t* log, const char* format, ...);
__declspec_dll void logInfo(Log_t* log, const char* format, ...);
__declspec_dll void logDebug(Log_t* log, const char* format, ...);

#ifdef	__cplusplus
}
#endif

#endif
