//
// Created by hujianzhe
//

#ifndef	UTIL_SYSLIB_IPC_H_
#define	UTIL_SYSLIB_IPC_H_

#include "basicdef.h"

#if defined(_WIN32) || defined(_WIN64)
	#include <process.h>
	typedef void(*sighandler_t)(int);
	typedef	CRITICAL_SECTION		CSLOCK;
	typedef CONDITION_VARIABLE		CONDITION;
	typedef HANDLE 					MUTEX;
	typedef HANDLE 					SEMID;
	#define	INVALID_SEMID			NULL
	/*typedef SRWLOCK				RWLOCK;*/
	typedef struct RWLOCK {
		volatile LONG __read_cnt;
		HANDLE __read_ev, __write_ev, __wait_ev;
	} RWLOCK;
#else
	#include <sys/ipc.h>
	#include <pthread.h>
	#include <semaphore.h>
	#include <sys/sem.h>
	#include <sys/shm.h>
	#include <sys/time.h>
	typedef	pthread_mutex_t			CSLOCK;
	typedef pthread_cond_t			CONDITION;
	typedef pthread_mutex_t 		MUTEX;
	typedef	sem_t* 					SEMID;
	#define	INVALID_SEMID			SEM_FAILED
	typedef pthread_rwlock_t		RWLOCK;
	typedef pthread_once_t          INIT_ONCE;
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
EXEC_RETURN pipe_Create(FD_HANDLE* r, FD_HANDLE* w);
EXEC_RETURN pipe_NonBlock(FD_HANDLE pipefd, BOOL bool_val);
int pipe_ReadableBytes(FD_HANDLE r);
/* critical section */
EXEC_RETURN cslock_Create(CSLOCK* cs);
EXEC_RETURN cslock_Enter(CSLOCK* cs, BOOL wait_bool);
EXEC_RETURN cslock_Leave(CSLOCK* cs);
EXEC_RETURN cslock_Close(CSLOCK* cs);
/* condition */
EXEC_RETURN condition_Create(CONDITION* condition);
EXEC_RETURN condition_Wait(CONDITION* condition, CSLOCK* cs, int msec);
EXEC_RETURN condition_WakeThread(CONDITION* condition);
EXEC_RETURN condition_WakeAllThread(CONDITION* condition);
EXEC_RETURN condition_Close(CONDITION* condition);
/* mutex */
EXEC_RETURN mutex_Create(MUTEX* mutex);
EXEC_RETURN mutex_Lock(MUTEX* mutex, BOOL wait_bool);
EXEC_RETURN mutex_Unlock(MUTEX* mutex);
EXEC_RETURN mutex_Close(MUTEX* mutex);
/* read/write lock */
EXEC_RETURN rwlock_Create(RWLOCK* rwlock);
EXEC_RETURN rwlock_LockRead(RWLOCK* rwlock, BOOL wait_bool);
EXEC_RETURN rwlock_LockWrite(RWLOCK* rwlock, BOOL wait_bool);
EXEC_RETURN rwlock_UnlockRead(RWLOCK* rwlock);
EXEC_RETURN rwlock_UnlockWrite(RWLOCK* rwlock);
EXEC_RETURN rwlock_Close(RWLOCK* rwlock);
/* semaphore */
SEMID semaphore_Create(const char* name, unsigned short val);
SEMID semaphore_Open(const char* name);
EXEC_RETURN semaphore_Wait(SEMID id, BOOL wait_bool);
EXEC_RETURN semaphore_Post(SEMID id);
EXEC_RETURN semaphore_Close(SEMID id);
EXEC_RETURN semaphore_Unlink(const char* name);
/* once call */
EXEC_RETURN initonce_Call(INIT_ONCE* once, void(*callback)(void));

#ifdef	__cplusplus
}
#endif

#endif
