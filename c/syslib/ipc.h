//
// Created by hujianzhe
//

#ifndef	UTIL_C_SYSLIB_IPC_H
#define	UTIL_C_SYSLIB_IPC_H

#include "platform_define.h"

#if defined(_WIN32) || defined(_WIN64)
	#include <process.h>
	typedef void(*sighandler_t)(int);
	typedef	CRITICAL_SECTION		CSLock_t;
	typedef CONDITION_VARIABLE		ConditionVariable_t;
	typedef HANDLE 					Mutex_t;
	typedef HANDLE 					SemId_t;
	#define	INVALID_SEMID			NULL
	/*typedef SRWLOCK				RWLock_t;*/
	typedef struct RWLock_t {
		volatile BOOL __exclusive_lock;
		volatile LONG __read_cnt;
		HANDLE __read_ev, __write_ev, __wait_ev;
	} RWLock_t;
	typedef	INIT_ONCE				InitOnce_t;
#else
	#include <sys/ipc.h>
	#include <pthread.h>
	#include <semaphore.h>
	#include <sys/sem.h>
	#include <sys/shm.h>
	#include <sys/time.h>
	typedef	pthread_mutex_t			CSLock_t;
	typedef pthread_cond_t			ConditionVariable_t;
	typedef pthread_mutex_t 		Mutex_t;
	typedef	sem_t* 					SemId_t;
	#define	INVALID_SEMID			SEM_FAILED
	typedef pthread_rwlock_t		RWLock_t;
	typedef pthread_once_t          InitOnce_t;
	#define INIT_ONCE_STATIC_INIT	PTHREAD_ONCE_INIT
#endif
#include <signal.h>
#if defined(__FreeBSD__) || defined(__APPLE__)
	typedef sig_t					sighandler_t;
#endif

#ifdef	__cplusplus
extern "C" {
#endif

/* signal */
sighandler_t signal_Handle(int signo, sighandler_t func);
/* pipe */
BOOL pipe_Create(FD_t* r, FD_t* w);
BOOL pipe_NonBlock(FD_t pipefd, BOOL bool_val);
int pipe_ReadableBytes(FD_t r);
/* critical section */
BOOL cslock_Create(CSLock_t* cs);
BOOL cslock_TryEnter(CSLock_t* cs);
void cslock_Enter(CSLock_t* cs);
void cslock_Leave(CSLock_t* cs);
void cslock_Close(CSLock_t* cs);
/* condition */
BOOL condition_Create(ConditionVariable_t* condition);
BOOL condition_Wait(ConditionVariable_t* condition, CSLock_t* cs, int msec);
void condition_WakeThread(ConditionVariable_t* condition);
void condition_WakeAllThread(ConditionVariable_t* condition);
void condition_Close(ConditionVariable_t* condition);
/* mutex */
BOOL mutex_Create(Mutex_t* mutex);
BOOL mutex_TryLock(Mutex_t* mutex);
void mutex_Lock(Mutex_t* mutex);
void mutex_Unlock(Mutex_t* mutex);
void mutex_Close(Mutex_t* mutex);
/* read/write lock */
BOOL rwlock_Create(RWLock_t* rwlock);
void rwlock_LockRead(RWLock_t* rwlock);
void rwlock_LockWrite(RWLock_t* rwlock);
void rwlock_Unlock(RWLock_t* rwlock);
void rwlock_Close(RWLock_t* rwlock);
/* semaphore */
SemId_t semaphore_Create(const char* name, unsigned short val);
SemId_t semaphore_Open(const char* name);
BOOL semaphore_TryWait(SemId_t id);
void semaphore_Wait(SemId_t id);
void semaphore_Post(SemId_t id);
void semaphore_Close(SemId_t id);
BOOL semaphore_Unlink(const char* name);
/* once call */
BOOL initonce_Call(InitOnce_t* once, void(*callback)(void));

#ifdef	__cplusplus
}
#endif

#endif
