//
// Created by hujianzhe
//

#ifndef	UTIL_C_SYSLIB_PROCESS_H
#define	UTIL_C_SYSLIB_PROCESS_H

#include "../platform_define.h"

#if defined(_WIN32) || defined(_WIN64)
	#include <process.h>
	#ifndef FIBER_FLAG_FLOAT_SWITCH
		#error "No such macro FIBER_FLAG_FLOAT_SWITCH"
	#endif
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
	typedef struct {
		HANDLE handle;
		unsigned int id;
	} Thread_t;
	typedef struct Fiber_t {
		void* arg;
		struct Fiber_t* m_creater;
		void(*m_entry)(struct Fiber_t*);
		LPVOID m_ctx;
	} Fiber_t;
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
	#include <ucontext.h>
/*
	#include <execinfo.h>	
*/
	typedef struct {
		pid_t id;
	} Process_t;
	typedef struct {
		pthread_t id;
	} Thread_t;
	typedef struct Fiber_t {
		void* arg;
		struct Fiber_t* m_creater;
		void(*m_entry)(struct Fiber_t*);
		ucontext_t m_ctx;
		unsigned char m_stack[1];
	} Fiber_t;
	#define	THREAD_CALL
	typedef	pthread_key_t			Tls_t;
	#define	__declspec_tls			__thread
#endif

#ifdef	__cplusplus
extern "C" {
#endif

/* module oerator */
#if defined(_WIN32) || defined(_WIN64)
#define	moduleLoad(path)							(path ? (void*)LoadLibraryA(path) : (void*)GetModuleHandleA(NULL))
#define	moduleSymbolAddress(module, symbol_name)	GetProcAddress(module, symbol_name)
#define	moduleUnload(module)						(module ? FreeLibrary(module) : TRUE)
#else
#define	moduleLoad(path)							dlopen(path, RTLD_NOW)
#define	moduleSymbolAddress(module, symbol_name)	dlsym(module, symbol_name)
#define	moduleUnload(module)						(module ? (dlclose(module) == 0) : 1)
#endif
/* process operator */
__declspec_dll BOOL processCreate(Process_t* p_process, const char* path, const char* cmdarg);
__declspec_dll BOOL processCancel(Process_t* process);
__declspec_dll size_t processId(void);
__declspec_dll BOOL processWait(Process_t process, unsigned char* retcode);
__declspec_dll BOOL processTryWait(Process_t process, unsigned char* retcode);
/* thread operator */
__declspec_dll BOOL threadCreate(Thread_t* p_thread, unsigned int (THREAD_CALL *entry)(void*), void* arg);
__declspec_dll BOOL threadDetach(Thread_t thread);
__declspec_dll BOOL threadJoin(Thread_t thread, unsigned int* retcode);
__declspec_dll void threadExit(unsigned int retcode);
__declspec_dll BOOL threadEqual(Thread_t t1, Thread_t t2);
__declspec_dll Thread_t threadSelf(void);
__declspec_dll void threadPause(void);
__declspec_dll void threadSleepMillsecond(unsigned int msec);
__declspec_dll void threadYield(void);
__declspec_dll BOOL threadSetAffinity(Thread_t thread, unsigned int processor_index);
/* thread local operator */
__declspec_dll BOOL threadAllocLocalKey(Tls_t* key);
__declspec_dll BOOL threadSetLocalValue(Tls_t key, void* value);
__declspec_dll void* threadGetLocalValue(Tls_t key);
__declspec_dll BOOL threadFreeLocalKey(Tls_t key);
/* fiber operator */
__declspec_dll Fiber_t* fiberFromThread(void);
__declspec_dll Fiber_t* fiberCreate(Fiber_t* cur_fiber, size_t stack_size, void (*entry)(Fiber_t*));
__declspec_dll void fiberSwitch(Fiber_t* from, Fiber_t* to);
__declspec_dll void fiberFree(Fiber_t* fiber);

#ifdef	__cplusplus
}
#endif

#endif
