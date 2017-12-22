//
// Created by hujianzhe
//

#ifndef	UTIL_C_SYSLIB_PROCESS_H
#define	UTIL_C_SYSLIB_PROCESS_H

#include "platform_define.h"

#if defined(_WIN32) || defined(_WIN64)
	#include <process.h>
	typedef struct {
		HANDLE handle;
		DWORD id;
	} PROCESS;
	#define	__dllexport				__declspec(dllexport)
	#define	__dllimport				__declspec(dllimport)
	typedef HANDLE					THREAD;
	#define	THREAD_CALL				__stdcall
	typedef	DWORD					THREAD_LOCAL_KEY;
	#define	__tls					__declspec(thread)
#else
	#include <dlfcn.h>
	#include <pthread.h>
	#include <sys/select.h>
	#include <sys/time.h>
	#include <sys/wait.h>
	#include <ucontext.h>
	typedef struct {
		pid_t id;
	} PROCESS;
	#define	__dllexport
	#define	__dllimport
	typedef pthread_t				THREAD;
	#define	THREAD_CALL
	typedef	pthread_key_t			THREAD_LOCAL_KEY;
	#define	__tls					__thread
#endif

#ifdef	__cplusplus
extern "C" {
#endif

/* process operator */
EXEC_RETURN process_Create(PROCESS* p_process, const char* path, const char* cmdarg);
EXEC_RETURN process_Cancel(PROCESS* process);
size_t process_Id(void);
EXEC_RETURN process_TryFreeZombie(PROCESS* process, unsigned char* retcode);
void* process_LoadModule(const char* path);
void* process_GetModuleSymbolAddress(void* handle, const char* symbol_name);
EXEC_RETURN process_UnloadModule(void* handle);
/* thread operator */
EXEC_RETURN thread_Create(THREAD* p_thread, unsigned int (THREAD_CALL *entry)(void*), void* arg);
EXEC_RETURN thread_Detach(THREAD thread);
EXEC_RETURN thread_Join(THREAD thread, unsigned int* retcode);
void thread_Exit(unsigned int retcode);
#if defined(_WIN32) || defined(_WIN64)
#define	thread_Self()		GetCurrentThread()
#define	thread_Pause()		SuspendThread(GetCurrentThread())
#else
#define	thread_Self()		pthread_self()
#define	thread_Pause()		pause()
#endif
void thread_Sleep(unsigned int msec);
void thread_Yield(void);
EXEC_RETURN thread_SetAffinity(THREAD thread, unsigned int processor_index);
/* thread local operator */
EXEC_RETURN thread_AllocLocalKey(THREAD_LOCAL_KEY* key);
EXEC_RETURN thread_SetLocalValue(THREAD_LOCAL_KEY key, void* value);
void* thread_GetLocalValue(THREAD_LOCAL_KEY key);
EXEC_RETURN thread_FreeLocalKey(THREAD_LOCAL_KEY key);

#ifdef	__cplusplus
}
#endif

#endif
