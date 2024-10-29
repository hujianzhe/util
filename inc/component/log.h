//
// Created by hujianzhe
//

#ifndef	UTIL_C_COMPONENT_LOG_H
#define	UTIL_C_COMPONENT_LOG_H

#include "../../inc/platform_define.h"
#include <stdio.h>
#include <time.h>

struct Log_t;

typedef struct LogItemInfo_t {
	const char* priority_str;
	const char* source_file;
	unsigned int source_line;
	struct tm dt;
} LogItemInfo_t;

typedef struct LogFileOption_t {
	int rotate_timelen_sec;
	int(*fn_output_prefix_length)(const LogItemInfo_t*);
	void(*fn_output_sprintf_prefix)(char*, const LogItemInfo_t*);
	const char*(*fn_new_fullpath)(const char* base_path, const char* key, const struct tm* dt);
	void(*fn_free_path)(char*);
} LogFileOption_t;

#ifdef	__cplusplus
extern "C" {
#endif

__declspec_dll struct Log_t* logOpen(void);
__declspec_dll void logDestroy(struct Log_t* log);

__declspec_dll const LogFileOption_t* logFileOptionDefaultDay(void);
__declspec_dll const LogFileOption_t* logFileOptionDefaultHour(void);
__declspec_dll const LogFileOption_t* logFileOptionDefaultMinute(void);
__declspec_dll struct Log_t* logEnableFile(struct Log_t* log, const char* key, const LogFileOption_t* opt, const char* base_path);

__declspec_dll int logFilterPriorityLess(int log_priority, int filter_priority);
__declspec_dll int logFilterPriorityLessEqual(int log_priority, int filter_priority);
__declspec_dll int logFilterPriorityGreater(int log_priority, int filter_priority);
__declspec_dll int logFilterPriorityGreaterEqual(int log_priority, int filter_priority);
__declspec_dll int logFilterPriorityEqual(int log_priority, int filter_priority);
__declspec_dll int logFilterPriorityNotEqual(int log_priority, int filter_priority);
__declspec_dll void logSetPriorityFilter(struct Log_t* log, int filter_priority, int(*fn_priority_filter)(int, int));
__declspec_dll int logCheckPriorityFilter(struct Log_t* log, int priority);

__declspec_dll void logPrintlnNoFilter(struct Log_t* log, const char* key, int priority, LogItemInfo_t* ii, const char* format, ...);
__declspec_dll void logPrintRaw(struct Log_t* log, const char* key, int priority, const char* format, ...);

#define logPrintlnTempletePrivate(log, key, priority, format, ...)	\
if (!logCheckPriorityFilter(log, priority))	{ \
	LogItemInfo_t ii; \
	ii.source_file = __FILE__; \
	ii.source_line = __LINE__; \
	logPrintlnNoFilter(log, key, priority, &ii, format, ##__VA_ARGS__); \
}

#define	logEmerg(log, key, format, ...)		logPrintlnTempletePrivate(log, key, 0, format, ##__VA_ARGS__)
#define	logAlert(log, key, format, ...)		logPrintlnTempletePrivate(log, key, 1, format, ##__VA_ARGS__)
#define	logCrit(log, key, format, ...)		logPrintlnTempletePrivate(log, key, 2, format, ##__VA_ARGS__)
#define	logErr(log, key, format, ...)		logPrintlnTempletePrivate(log, key, 3, format, ##__VA_ARGS__)
#define	logWarn(log, key, format, ...)		logPrintlnTempletePrivate(log, key, 4, format, ##__VA_ARGS__)
#define	logNotice(log, key, format, ...)	logPrintlnTempletePrivate(log, key, 5, format, ##__VA_ARGS__)
#define	logInfo(log, key, format, ...)		logPrintlnTempletePrivate(log, key, 6, format, ##__VA_ARGS__)
#define	logDebug(log, key, format, ...)		logPrintlnTempletePrivate(log, key, 7, format, ##__VA_ARGS__)

#ifdef	__cplusplus
}
#endif

#endif
