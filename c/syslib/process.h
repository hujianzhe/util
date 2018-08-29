//
// Created by hujianzhe
//

#ifndef	UTIL_C_SYSLIB_PROCESS_H
#define	UTIL_C_SYSLIB_PROCESS_H

#include "../platform_define.h"

#if defined(_WIN32) || defined(_WIN64)
	#include <process.h>
/*
	#ifdef _MSC_VER
		#pragma warning(disable:4091)// avoid bug(dbghelp.h warning C4091: "typedef ")
	#endif
	#include <Dbghelp.h>
	#ifdef _MSC_VER
		#pragma warning(default:4091)
	#endif
*/
	typedef struct {
		HANDLE handle;
		DWORD id;
	} Process_t;
	#define	__declspec_dllexport	__declspec(dllexport)
	#define	__declspec_dllimport	__declspec(dllimport)
	#define	DLL_CALL				__stdcall
	typedef HANDLE					Thread_t;
	#define	THREAD_CALL				__stdcall
	typedef	DWORD					Tls_t;
	#define	__declspec_tls			__declspec(thread)
	#pragma comment(lib, "Dbghelp.lib")
#else
	#include <dlfcn.h>
	#include <pthread.h>
	#include <sys/select.h>
	#include <sys/time.h>
	#include <sys/wait.h>
/*
	#include <execinfo.h>
	#include <ucontext.h>
*/
	typedef struct {
		pid_t id;
	} Process_t;
	#define	__declspec_dllexport
	#define	__declspec_dllimport
	#define	DLL_CALL
	typedef pthread_t				Thread_t;
	#define	THREAD_CALL
	typedef	pthread_key_t			Tls_t;
	#define	__declspec_tls			__thread
#endif

#ifdef	__cplusplus
extern "C" {
#endif

/* process operator */
BOOL processCreate(Process_t* p_process, const char* path, const char* cmdarg);
BOOL processCancel(Process_t* process);
size_t processId(void);
BOOL processWait(Process_t* process, unsigned char* retcode);
BOOL processTryWait(Process_t* process, unsigned char* retcode);
void* processLoadModule(const char* path);
void* processGetModuleSymbolAddress(void* handle, const char* symbol_name);
BOOL processUnloadModule(void* handle);
/* thread operator */
BOOL threadCreate(Thread_t* p_thread, unsigned int (THREAD_CALL *entry)(void*), void* arg);
BOOL threadDetach(Thread_t thread);
BOOL threadJoin(Thread_t thread, unsigned int* retcode);
void threadExit(unsigned int retcode);
#if defined(_WIN32) || defined(_WIN64)
#define	threadSelf()		GetCurrentThread()
#define	threadPause()		SuspendThread(GetCurrentThread())
#else
#define	threadSelf()		pthread_self()
#define	threadPause()		pause()
#endif
void threadSleepMillsecond(unsigned int msec);
void threadYield(void);
BOOL threadSetAffinity(Thread_t thread, unsigned int processor_index);
/* thread local operator */
BOOL threadAllocLocalKey(Tls_t* key);
BOOL threadSetLocalValue(Tls_t key, void* value);
void* threadGetLocalValue(Tls_t key);
BOOL threadFreeLocalKey(Tls_t key);

#ifdef	__cplusplus
}
#endif

#endif
