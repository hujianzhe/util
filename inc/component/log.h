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

typedef struct LogFileRotateOption_t {
	int rotate_timelen_sec;
	const char*(*fn_new_fullpath)(const char* base_path, const char* key, const struct tm* dt);
	void(*fn_free_path)(char*);
} LogFileRotateOption_t;

typedef struct LogFileOutputOption_t {
	int(*fn_prefix_length)(const LogItemInfo_t*);
	void(*fn_sprintf_prefix)(char*, const LogItemInfo_t*);
} LogFileOutputOption_t;

#ifdef	__cplusplus
extern "C" {
#endif

__declspec_dll struct Log_t* logOpen(void);
__declspec_dll void logDestroy(struct Log_t* log);

__declspec_dll struct Log_t* logEnableFile(struct Log_t* log, const char* key, const char* base_path, const LogFileOutputOption_t* output_opt, const LogFileRotateOption_t* rotate_opt);
__declspec_dll const LogFileOutputOption_t* logFileOutputOptionDefault(void);
__declspec_dll const LogFileRotateOption_t* logFileRotateOptionDefaultDay(void);
__declspec_dll const LogFileRotateOption_t* logFileRotateOptionDefaultHour(void);
__declspec_dll const LogFileRotateOption_t* logFileRotateOptionDefaultMinute(void);

__declspec_dll int logFilterPriorityLess(int log_priority, int filter_priority);
__declspec_dll int logFilterPriorityLessEqual(int log_priority, int filter_priority);
__declspec_dll int logFilterPriorityGreater(int log_priority, int filter_priority);
__declspec_dll int logFilterPriorityGreaterEqual(int log_priority, int filter_priority);
__declspec_dll int logFilterPriorityEqual(int log_priority, int filter_priority);
__declspec_dll int logFilterPriorityNotEqual(int log_priority, int filter_priority);
__declspec_dll void logSetPriorityFilter(struct Log_t* log, int filter_priority, int(*fn_priority_filter)(int, int));
__declspec_dll int logCheckPriorityEnabled(struct Log_t* log, unsigned int priority);

__declspec_dll void logPrintlnNoFilter(struct Log_t* log, const char* key, int priority, LogItemInfo_t* ii, const char* format, ...);

#define logPrintlnTempletePrivate(log, key, priority, format, ...)	\
if (logCheckPriorityEnabled(log, priority))	{ \
	LogItemInfo_t ii; \
	ii.source_file = __FILE__; \
	ii.source_line = __LINE__; \
	logPrintlnNoFilter(log, key, priority, &ii, "" format, ##__VA_ARGS__); \
}

#define	logTrace(log, key, format, ...)		logPrintlnTempletePrivate(log, key, 0, format, ##__VA_ARGS__)
#define	logInfo(log, key, format, ...)		logPrintlnTempletePrivate(log, key, 1, format, ##__VA_ARGS__)
#define	logDebug(log, key, format, ...)		logPrintlnTempletePrivate(log, key, 2, format, ##__VA_ARGS__)
#define	logError(log, key, format, ...)		logPrintlnTempletePrivate(log, key, 3, format, ##__VA_ARGS__)

#ifdef	__cplusplus
}
#endif

#endif
