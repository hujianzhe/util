//
// Created by hujianzhe
//

#ifndef	UTIL_C_SYSLIB_IPC_H
#define	UTIL_C_SYSLIB_IPC_H

#include "../platform_define.h"

#if defined(_WIN32) || defined(_WIN64)
	#include <process.h>
	typedef void(*sighandler_t)(int);
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
UTIL_LIBAPI sighandler_t signalRegHandler(int signo, sighandler_t func);
/* pipe */
UTIL_LIBAPI BOOL pipeCreate(FD_t* r, FD_t* w);
UTIL_LIBAPI BOOL pipeNonBlock(FD_t pipefd, BOOL bool_val);
UTIL_LIBAPI int pipeReadableBytes(FD_t r);
/* critical section */
UTIL_LIBAPI CriticalSection_t* criticalsectionCreate(CriticalSection_t* cs);
UTIL_LIBAPI BOOL criticalsectionTryEnter(CriticalSection_t* cs);
UTIL_LIBAPI void criticalsectionEnter(CriticalSection_t* cs);
UTIL_LIBAPI void criticalsectionLeave(CriticalSection_t* cs);
UTIL_LIBAPI void criticalsectionClose(CriticalSection_t* cs);
/* condition */
UTIL_LIBAPI ConditionVariable_t* conditionvariableCreate(ConditionVariable_t* condition);
UTIL_LIBAPI BOOL conditionvariableWait(ConditionVariable_t* condition, CriticalSection_t* cs, int msec);
UTIL_LIBAPI void conditionvariableSignal(ConditionVariable_t* condition);
UTIL_LIBAPI void conditionvariableBroadcast(ConditionVariable_t* condition);
UTIL_LIBAPI void conditionvariableClose(ConditionVariable_t* condition);
/* mutex */
UTIL_LIBAPI Mutex_t* mutexCreate(Mutex_t* mutex);
UTIL_LIBAPI BOOL mutexTryLock(Mutex_t* mutex);
UTIL_LIBAPI void mutexLock(Mutex_t* mutex);
UTIL_LIBAPI void mutexUnlock(Mutex_t* mutex);
UTIL_LIBAPI void mutexClose(Mutex_t* mutex);
/* read/write lock */
UTIL_LIBAPI RWLock_t* rwlockCreate(RWLock_t* rwlock);
UTIL_LIBAPI void rwlockLockRead(RWLock_t* rwlock);
UTIL_LIBAPI void rwlockLockWrite(RWLock_t* rwlock);
UTIL_LIBAPI void rwlockUnlock(RWLock_t* rwlock);
UTIL_LIBAPI void rwlockClose(RWLock_t* rwlock);
/* semaphore */
UTIL_LIBAPI Semaphore_t* semaphoreCreate(Semaphore_t* sem, const char* name, unsigned short val);
UTIL_LIBAPI Semaphore_t* semaphoreOpen(Semaphore_t* sem, const char* name);
UTIL_LIBAPI BOOL semaphoreTryWait(Semaphore_t* sem);
UTIL_LIBAPI void semaphoreWait(Semaphore_t* sem);
UTIL_LIBAPI void semaphorePost(Semaphore_t* sem);
UTIL_LIBAPI void semaphoreClose(Semaphore_t* sem);
UTIL_LIBAPI BOOL semaphoreUnlink(const char* name);
/* once call */
UTIL_LIBAPI InitOnce_t* initonceCall(InitOnce_t* once, void(*callback)(void));

#ifdef	__cplusplus
}
#endif

#endif
