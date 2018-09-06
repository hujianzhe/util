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
sighandler_t signalRegHandler(int signo, sighandler_t func);
/* pipe */
BOOL pipeCreate(FD_t* r, FD_t* w);
BOOL pipeNonBlock(FD_t pipefd, BOOL bool_val);
int pipeReadableBytes(FD_t r);
/* critical section */
CriticalSection_t* criticalsectionCreate(CriticalSection_t* cs);
BOOL criticalsectionTryEnter(CriticalSection_t* cs);
void criticalsectionEnter(CriticalSection_t* cs);
void criticalsectionLeave(CriticalSection_t* cs);
void criticalsectionClose(CriticalSection_t* cs);
/* condition */
ConditionVariable_t* conditionvariableCreate(ConditionVariable_t* condition);
BOOL conditionvariableWait(ConditionVariable_t* condition, CriticalSection_t* cs, int msec);
void conditionvariableSignal(ConditionVariable_t* condition);
void conditionvariableBroadcast(ConditionVariable_t* condition);
void conditionvariableClose(ConditionVariable_t* condition);
/* mutex */
Mutex_t* mutexCreate(Mutex_t* mutex);
BOOL mutexTryLock(Mutex_t* mutex);
void mutexLock(Mutex_t* mutex);
void mutexUnlock(Mutex_t* mutex);
void mutexClose(Mutex_t* mutex);
/* read/write lock */
RWLock_t* rwlockCreate(RWLock_t* rwlock);
void rwlockLockRead(RWLock_t* rwlock);
void rwlockLockWrite(RWLock_t* rwlock);
void rwlockUnlock(RWLock_t* rwlock);
void rwlockClose(RWLock_t* rwlock);
/* semaphore */
Semaphore_t* semaphoreCreate(Semaphore_t* sem, const char* name, unsigned short val);
Semaphore_t* semaphoreOpen(Semaphore_t* sem, const char* name);
BOOL semaphoreTryWait(Semaphore_t* sem);
void semaphoreWait(Semaphore_t* sem);
void semaphorePost(Semaphore_t* sem);
void semaphoreClose(Semaphore_t* sem);
BOOL semaphoreUnlink(const char* name);
/* once call */
InitOnce_t* initonceCall(InitOnce_t* once, void(*callback)(void));

#ifdef	__cplusplus
}
#endif

#endif
