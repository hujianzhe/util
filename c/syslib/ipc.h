//
// Created by hujianzhe
//

#ifndef	UTIL_C_SYSLIB_IPC_H
#define	UTIL_C_SYSLIB_IPC_H

#include "../platform_define.h"

#if defined(_WIN32) || defined(_WIN64)
	#include <process.h>
	typedef	CRITICAL_SECTION		CriticalSection_t;
	typedef CONDITION_VARIABLE		ConditionVariable_t;
	typedef HANDLE 					Mutex_t;
	typedef HANDLE 					Semaphore_t;
	typedef struct RWLock_t {
		volatile BOOL __exclusive_lock;
		volatile LONG __read_cnt;
		HANDLE __read_ev, __write_ev, __wait_ev;
	} RWLock_t;
	typedef	INIT_ONCE				InitOnce_t;
	#define	SEM_FAILED				NULL
#else
	#include <sys/ipc.h>
	#include <pthread.h>
	#include <semaphore.h>
	#include <sys/sem.h>
	#include <sys/shm.h>
	#include <sys/time.h>
	typedef	pthread_mutex_t			CriticalSection_t;
	typedef pthread_cond_t			ConditionVariable_t;
	typedef pthread_mutex_t 		Mutex_t;
	typedef	sem_t*					Semaphore_t;
	typedef pthread_rwlock_t		RWLock_t;
	typedef pthread_once_t          InitOnce_t;
	#define INIT_ONCE_STATIC_INIT	PTHREAD_ONCE_INIT
#endif
#include <signal.h>
typedef	void(*sighandler_t)(int);
/*
#if defined(__FreeBSD__) || defined(__APPLE__)
	typedef sig_t					sighandler_t;
#endif
*/

#ifdef	__cplusplus
extern "C" {
#endif

/* signal */
__declspec_dll sighandler_t signalRegHandler(int signo, sighandler_t func);
/* pipe */
__declspec_dll BOOL pipeCreate(FD_t* r, FD_t* w);
__declspec_dll BOOL pipeNonBlock(FD_t pipefd, BOOL bool_val);
__declspec_dll int pipeReadableBytes(FD_t r);
/* critical section */
__declspec_dll CriticalSection_t* criticalsectionCreate(CriticalSection_t* cs);
__declspec_dll BOOL criticalsectionTryEnter(CriticalSection_t* cs);
__declspec_dll void criticalsectionEnter(CriticalSection_t* cs);
__declspec_dll void criticalsectionLeave(CriticalSection_t* cs);
__declspec_dll void criticalsectionClose(CriticalSection_t* cs);
/* condition */
__declspec_dll ConditionVariable_t* conditionvariableCreate(ConditionVariable_t* condition);
__declspec_dll BOOL conditionvariableWait(ConditionVariable_t* condition, CriticalSection_t* cs, int msec);
__declspec_dll void conditionvariableSignal(ConditionVariable_t* condition);
__declspec_dll void conditionvariableBroadcast(ConditionVariable_t* condition);
__declspec_dll void conditionvariableClose(ConditionVariable_t* condition);
/* mutex */
__declspec_dll Mutex_t* mutexCreate(Mutex_t* mutex);
__declspec_dll BOOL mutexTryLock(Mutex_t* mutex);
__declspec_dll void mutexLock(Mutex_t* mutex);
__declspec_dll void mutexUnlock(Mutex_t* mutex);
__declspec_dll void mutexClose(Mutex_t* mutex);
/* read/write lock */
__declspec_dll RWLock_t* rwlockCreate(RWLock_t* rwlock);
__declspec_dll void rwlockLockRead(RWLock_t* rwlock);
__declspec_dll void rwlockLockWrite(RWLock_t* rwlock);
__declspec_dll void rwlockUnlock(RWLock_t* rwlock);
__declspec_dll void rwlockClose(RWLock_t* rwlock);
/* semaphore */
__declspec_dll Semaphore_t* semaphoreCreate(Semaphore_t* sem, const char* name, unsigned short val);
__declspec_dll Semaphore_t* semaphoreOpen(Semaphore_t* sem, const char* name);
__declspec_dll BOOL semaphoreTryWait(Semaphore_t* sem);
__declspec_dll void semaphoreWait(Semaphore_t* sem);
__declspec_dll void semaphorePost(Semaphore_t* sem);
__declspec_dll void semaphoreClose(Semaphore_t* sem);
__declspec_dll BOOL semaphoreUnlink(const char* name);
/* once call */
__declspec_dll InitOnce_t* initonceCall(InitOnce_t* once, void(*callback)(void));

#ifdef	__cplusplus
}
#endif

#endif
