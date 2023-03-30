//
// Created by hujianzhe
//

#ifndef	UTIL_C_COMPONENT_LOG_H
#define	UTIL_C_COMPONENT_LOG_H

#include "../../inc/platform_define.h"

struct Log_t;

#ifdef	__cplusplus
extern "C" {
#endif

__declspec_dll struct Log_t* logOpen(size_t maxfilesize, const char ident[64], const char* pathname);
__declspec_dll void logEnableStdio(struct Log_t* log, int enabled);
__declspec_dll void logFlush(struct Log_t* log);
__declspec_dll void logClear(struct Log_t* log);
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

#define logPrintlnTempletePrivate(log, priority, format, ...)	\
if (!logCheckPriorityFilter(log, priority))	\
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
