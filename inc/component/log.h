//
// Created by hujianzhe
//

#ifndef	UTIL_C_COMPONENT_LOG_H
#define	UTIL_C_COMPONENT_LOG_H

#include "../../inc/platform_define.h"
#include <time.h>

struct Log_t;

typedef struct LogItemInfo_t {
	const char* priority_str;
	const char* source_file;
	const char* func_name;
	unsigned int source_line;
	struct tm dt;
} LogItemInfo_t;

#ifdef	__cplusplus
extern "C" {
#endif

__declspec_dll struct Log_t* logOpen(size_t maxfilesize, const char* pathname);
__declspec_dll void logPrefix(struct Log_t* log, int(*fn_prefix_length)(LogItemInfo_t*), void(*fn_sprintf_prefix)(char*, LogItemInfo_t*));
__declspec_dll void logEnableFile(struct Log_t* log, int enabled);
__declspec_dll void logEnableStdio(struct Log_t* log, int enabled);
__declspec_dll void logDestroy(struct Log_t* log);

__declspec_dll int logFilterPriorityLess(int log_priority, int filter_priority);
__declspec_dll int logFilterPriorityLessEqual(int log_priority, int filter_priority);
__declspec_dll int logFilterPriorityGreater(int log_priority, int filter_priority);
__declspec_dll int logFilterPriorityGreaterEqual(int log_priority, int filter_priority);
__declspec_dll int logFilterPriorityEqual(int log_priority, int filter_priority);
__declspec_dll int logFilterPriorityNotEqual(int log_priority, int filter_priority);
__declspec_dll void logSetPriorityFilter(struct Log_t* log, int filter_priority, int(*fn_priority_filter)(int, int));
__declspec_dll int logCheckPriorityFilter(struct Log_t* log, int priority);

__declspec_dll void logPrintRawNoFilter(struct Log_t* log, int priority, const char* format, ...);
__declspec_dll void logPrintlnNoFilter(struct Log_t* log, int priority, const char* format, ...);
__declspec_dll void logPrintRaw(struct Log_t* log, int priority, const char* format, ...);
__declspec_dll void logPrintln(struct Log_t* log, int priority, const char* format, ...);

__declspec_dll struct Log_t* logSaveSourceFile(struct Log_t* log, const char* source_file, const char* func_name, unsigned int source_line);
#define logPrintlnTempletePrivate(log, priority, format, ...)	\
if (!logCheckPriorityFilter(log, priority))	\
	logPrintlnNoFilter(logSaveSourceFile(log, __FILE__, __func__, __LINE__), priority, "" format, ##__VA_ARGS__)

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
