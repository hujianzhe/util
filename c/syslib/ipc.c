//
// Created by hujianzhe
//

#include "ipc.h"
#include <errno.h>

#ifdef	__cplusplus
extern "C" {
#endif

#if defined(_WIN32) || defined(_WIN64)
#else
#include <fcntl.h>
#include <sys/ioctl.h>
#endif

/* signal */
sighandler_t signal_Handle(int signo, sighandler_t func) {
#if defined(_WIN32) || defined(_WIN64)
	return signal(signo, func);
#else
	struct sigaction act, oact;
	act.sa_handler = func;
	sigfillset(&act.sa_mask);
	act.sa_flags = SA_RESTART;
	return sigaction(signo, &act, &oact) < 0 ? SIG_ERR : oact.sa_handler;
#endif
}

/* pipe */
EXEC_RETURN pipe_Create(FD_t* r, FD_t* w) {
#if defined(_WIN32) || defined(_WIN64)
	/* note: IOCP can't use anonymous pipes(without an overlapped flag) */
	SECURITY_ATTRIBUTES sa = {0};
	sa.nLength = sizeof(sa);
	sa.lpSecurityDescriptor = NULL;
	sa.bInheritHandle = TRUE;
	return CreatePipe((PHANDLE)r, (PHANDLE)w, &sa, 0) ? EXEC_SUCCESS : EXEC_ERROR;
#else
	int fd[2];
	if (pipe(fd)) {
		return EXEC_ERROR;
	}
	*r = fd[0];
	*w = fd[1];
	return EXEC_SUCCESS;
#endif
}

EXEC_RETURN pipe_NonBlock(FD_t pipefd, BOOL bool_val) {
#if defined(_WIN32) || defined(_WIN64)
	DWORD mode = bool_val ? PIPE_NOWAIT : PIPE_WAIT;
	return SetNamedPipeHandleState((HANDLE)pipefd, &mode, NULL, NULL) ? EXEC_SUCCESS : EXEC_ERROR;
#else
	return ioctl(pipefd, FIONBIO, &bool_val) == 0 ? EXEC_SUCCESS : EXEC_ERROR;
#endif
}

int pipe_ReadableBytes(FD_t r) {
#if defined(_WIN32) || defined(_WIN64)
	DWORD bytes;
	return PeekNamedPipe((HANDLE)r, NULL, 0, NULL, &bytes, NULL) ? bytes : -1;
#else
	int bytes;
	return ioctl(r, FIONREAD, &bytes) ? -1 : bytes;
#endif
}

/* critical section */
EXEC_RETURN cslock_Create(CSLock_t* cs) {
#if defined(_WIN32) || defined(_WIN64)
	__try {
		InitializeCriticalSection(cs);
	}
	__except (STATUS_NO_MEMORY) {
		return EXEC_ERROR;
	}
	return EXEC_SUCCESS;
#else
	int res;
	int attr_ok = 0;
	pthread_mutexattr_t attr;
	do {
		res = pthread_mutexattr_init(&attr);
		if (res)
			break;
		attr_ok = 1;
		res = pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
		if (res)
			break;
		res = pthread_mutex_init(cs, &attr);
		if (res)
			break;
	} while (0);
	if (attr_ok) {
		assert_true(0 == pthread_mutexattr_destroy(&attr));
	}
	if (res) {
		errno = res;
		return EXEC_ERROR;
	}
	return EXEC_SUCCESS;
#endif
}

EXEC_RETURN cslock_TryEnter(CSLock_t* cs) {
#if defined(_WIN32) || defined(_WIN64)
	return TryEnterCriticalSection(cs) ? EXEC_SUCCESS : EXEC_ERROR;
#else
	int res = pthread_mutex_trylock(cs);
	if (res) {
		errno = res;
		return EXEC_ERROR;
	}
	return EXEC_SUCCESS;
#endif
}

void cslock_Enter(CSLock_t* cs) {
#if defined(_WIN32) || defined(_WIN64)
	EnterCriticalSection(cs);
#else
	assert_true(pthread_mutex_lock(cs) == 0);
#endif
}

void cslock_Leave(CSLock_t* cs) {
#if defined(_WIN32) || defined(_WIN64)
	LeaveCriticalSection(cs);
#else
	assert_true(pthread_mutex_unlock(cs) == 0);
#endif
}

void cslock_Close(CSLock_t* cs) {
#if defined(_WIN32) || defined(_WIN64)
	DeleteCriticalSection(cs);
#else
	assert_true(pthread_mutex_destroy(cs) == 0);
#endif
}

/* condition */
EXEC_RETURN condition_Create(ConditionVariable_t* condition) {
#if defined(_WIN32) || defined(_WIN64)
	InitializeConditionVariable(condition);
	return EXEC_SUCCESS;
#else
	int res = pthread_cond_init(condition, NULL);
	if (res) {
		errno = res;
		return EXEC_ERROR;
	}
	return EXEC_SUCCESS;
#endif
}

EXEC_RETURN condition_Wait(ConditionVariable_t* condition,CSLock_t* cs, int msec) {
#if defined(_WIN32) || defined(_WIN64)
	return SleepConditionVariableCS(condition, cs, msec) ? EXEC_SUCCESS : EXEC_ERROR;
#else
	int res;
	do {
		if (msec == INFTIM) {
			res = pthread_cond_wait(condition, cs);
		}
		else {
			struct timeval utc = {0};
			struct timezone zone = {0};
			struct timespec time_val = {0};
			if (gettimeofday(&utc, &zone)) {
				res = errno;
				break;
			}
			time_val.tv_sec = utc.tv_sec + msec / 1000;
			msec %= 1000;
			time_val.tv_nsec = utc.tv_usec * 1000 + msec * 1000000;
			time_val.tv_sec += time_val.tv_nsec / 1000000000;
			time_val.tv_nsec %= 1000000000;

			res = pthread_cond_timedwait(condition, cs, &time_val);
			if (res) {
				break;
			}
		}
	} while (0);
	if (res) {
		errno = res;
		return EXEC_ERROR;
	}
	return EXEC_SUCCESS;
#endif
}

void condition_WakeThread(ConditionVariable_t* condition) {
#if defined(_WIN32) || defined(_WIN64)
	WakeConditionVariable(condition);
#else
	assert_true(pthread_cond_signal(condition) == 0);
#endif
}

void condition_WakeAllThread(ConditionVariable_t* condition) {
#if defined(_WIN32) || defined(_WIN64)
	WakeAllConditionVariable(condition);
#else
	assert_true(pthread_cond_broadcast(condition) == 0);
#endif
}

void condition_Close(ConditionVariable_t* condition) {
#if defined(_WIN32) || defined(_WIN64)
#else
	assert_true(pthread_cond_destroy(condition) == 0);
#endif
}

/* mutex */
EXEC_RETURN mutex_Create(Mutex_t* mutex) {
#if defined(_WIN32) || defined(_WIN64)
	/* windows mutex will auto release after it's owner thread exit. */
	/* then...if another thread call WaitForSingleObject for that mutex,it will return WAIT_ABANDONED */
	HANDLE res = CreateEvent(NULL, FALSE, TRUE, NULL);/* so,I use event. */
	if (res) {
		*mutex = res;
		return EXEC_SUCCESS;
	}
	return EXEC_ERROR;
#else
	int res = pthread_mutex_init(mutex, NULL);
	if (res) {
		errno = res;
		return EXEC_ERROR;
	}
	return EXEC_SUCCESS;
#endif
}

EXEC_RETURN mutex_TryLock(Mutex_t* mutex) {
#if defined(_WIN32) || defined(_WIN64)
	return WaitForSingleObject(*mutex, 0) == WAIT_OBJECT_0 ? EXEC_SUCCESS : EXEC_ERROR;
#else
	int res = pthread_mutex_trylock(mutex);
	if (res) {
		errno = res;
		return EXEC_ERROR;
	}
	return EXEC_SUCCESS;
#endif
}

void mutex_Lock(Mutex_t* mutex) {
#if defined(_WIN32) || defined(_WIN64)
	assert_true(WaitForSingleObject(*mutex, INFINITE) == WAIT_OBJECT_0);
#else
	assert_true(pthread_mutex_lock(mutex) == 0);
#endif
}

void mutex_Unlock(Mutex_t* mutex) {
#if defined(_WIN32) || defined(_WIN64)
	assert_true(SetEvent(*mutex));
#else
	assert_true(pthread_mutex_unlock(mutex) == 0);
#endif
}

void mutex_Close(Mutex_t* mutex) {
#if defined(_WIN32) || defined(_WIN64)
	assert_true(CloseHandle(*mutex));
#else
	assert_true(pthread_mutex_destroy(mutex) == 0);
#endif
}

/* read/write lock */
EXEC_RETURN rwlock_Create(RWLock_t* rwlock) {
#if defined(_WIN32) || defined(_WIN64)
	/* I don't use SRW Locks because SRW Locks are neither fair nor FIFO. */
	/*InitializeSRWLock(rwlock);*/
	int read_ev_ok = 0, write_ev_ok = 0;
	do {
		rwlock->__read_ev = CreateEvent(NULL, TRUE, TRUE, NULL);/* manual reset event */
		if (!rwlock->__read_ev) {
			break;
		}
		read_ev_ok = 1;
		rwlock->__write_ev = CreateEvent(NULL, FALSE, TRUE, NULL);/* auto reset event */
		if (!rwlock->__write_ev) {
			break;
		}
		write_ev_ok = 1;
		rwlock->__wait_ev = CreateEvent(NULL, FALSE, TRUE, NULL);/* auto reset event */
		if (!rwlock->__wait_ev) {
			break;
		}
		rwlock->__exclusive_lock = FALSE;
		rwlock->__read_cnt = 0;
		return EXEC_SUCCESS;
	} while (0);
	if (read_ev_ok) {
		assert_true(CloseHandle(rwlock->__read_ev));
	}
	if (write_ev_ok) {
		assert_true(CloseHandle(rwlock->__write_ev));
	}
	return EXEC_ERROR;
#else
	int res;
	#ifdef  __linux__
	int attr_ok = 0;
	pthread_rwlockattr_t attr;
	do {
		res = pthread_rwlockattr_init(&attr);
		if (res) {
			break;
		}
		attr_ok = 1;
		/* write thread avoid starving */
		res = pthread_rwlockattr_setkind_np(&attr, PTHREAD_RWLOCK_PREFER_WRITER_NONRECURSIVE_NP);
		if (res) {
			break;
		}
		res = pthread_rwlock_init(rwlock, &attr);
		if (res) {
			break;
		}
	} while (0);
	if (attr_ok) {
		assert_true(0 == pthread_rwlockattr_destroy(&attr));
	}
	#else
	res = pthread_rwlock_init(rwlock, NULL);
	#endif
	if (res) {
		errno = res;
		return EXEC_ERROR;
	}
	return EXEC_SUCCESS;
#endif
}

void rwlock_LockRead(RWLock_t* rwlock) {
#if defined(_WIN32) || defined(_WIN64)
	/*
	if (wait_bool) {
		AcquireSRWLockShared(rwlock);
		return EXEC_SUCCESS;
	}
	return TryAcquireSRWLockShared(rwlock) ? EXEC_SUCCESS : EXEC_ERROR;
	*/
	assert_true(WaitForSingleObject(rwlock->__read_ev, INFINITE) == WAIT_OBJECT_0);
	assert_true(WaitForSingleObject(rwlock->__wait_ev, INFINITE) == WAIT_OBJECT_0);
	if (0 == rwlock->__read_cnt++) {
		assert_true(WaitForSingleObject(rwlock->__write_ev, INFINITE) == WAIT_OBJECT_0);
	}
	assert_true(SetEvent(rwlock->__wait_ev));
#else
	assert_true(pthread_rwlock_rdlock(rwlock) == 0);
	/*
	int res = pthread_rwlock_tryrdlock(rwlock);
	if (res) {
		errno = res;
		return EXEC_ERROR;
	}
	return EXEC_SUCCESS;
	*/
#endif
}

void rwlock_LockWrite(RWLock_t* rwlock) {
#if defined(_WIN32) || defined(_WIN64)
	/*
	if (wait_bool) {
		AcquireSRWLockExclusive(rwlock);
		return EXEC_SUCCESS;
	}
	return TryAcquireSRWLockExclusive(rwlock) ? EXEC_SUCCESS : EXEC_ERROR;
	*/
	assert_true(ResetEvent(rwlock->__read_ev));
	assert_true(WaitForSingleObject(rwlock->__write_ev, INFINITE) == WAIT_OBJECT_0);
	rwlock->__exclusive_lock = TRUE;
#else
	assert_true(pthread_rwlock_wrlock(rwlock) == 0);
	/*
	int res = pthread_rwlock_trywrlock(rwlock);
	if (res) {
		errno = res;
		return EXEC_ERROR;
	}
	return EXEC_SUCCESS;
	*/
#endif
}

void rwlock_Unlock(RWLock_t* rwlock) {
#if defined(_WIN32) || defined(_WIN64)
	if (rwlock->__exclusive_lock) {
		/*ReleaseSRWLockExclusive(rwlock);*/
		rwlock->__exclusive_lock = FALSE;
		assert_true(SetEvent(rwlock->__write_ev));
		assert_true(SetEvent(rwlock->__read_ev));
	}
	else {
		/*ReleaseSRWLockShared(rwlock);*/
		assert_true(WaitForSingleObject(rwlock->__wait_ev, INFINITE) == WAIT_OBJECT_0);
		if (0 == --rwlock->__read_cnt) {
			assert_true(SetEvent(rwlock->__write_ev));
		}
		assert_true(SetEvent(rwlock->__wait_ev));
	}
#else
	assert_true(pthread_rwlock_unlock(rwlock) == 0);
#endif
}

void rwlock_Close(RWLock_t* rwlock) {
#if defined(_WIN32) || defined(_WIN64)
	assert_true(CloseHandle(rwlock->__read_ev));
	assert_true(CloseHandle(rwlock->__write_ev));
	assert_true(CloseHandle(rwlock->__wait_ev));
#else
	assert_true(pthread_rwlock_destroy(rwlock) == 0);
#endif
}

/* semaphore */
SemId_t semaphore_Create(const char* name, unsigned short val) {
#if defined(_WIN32) || defined(_WIN64)
	SemId_t semid = CreateSemaphoreA(NULL, val, 0x7fffffff, name);
	if (GetLastError() == ERROR_ALREADY_EXISTS) {
		assert_true(CloseHandle(semid));
		return NULL;
	}
	return semid;
#else
	/* max init value at last SEM_VALUE_MAX(32767) */
	return sem_open(name, O_CREAT | O_EXCL, 0666, val);
	/* mac os x has deprecated sem_init */
#endif
}

SemId_t semaphore_Open(const char* name) {
#if defined(_WIN32) || defined(_WIN64)
	return OpenSemaphoreA(SEMAPHORE_ALL_ACCESS, FALSE, name);
#else
	return sem_open(name, 0);
#endif
}

EXEC_RETURN semaphore_TryWait(SemId_t id) {
#if defined(_WIN32) || defined(_WIN64)
	return WaitForSingleObject(id, 0) == WAIT_OBJECT_0 ? EXEC_SUCCESS : EXEC_ERROR;
#else
	return sem_trywait(id) == 0 ? EXEC_SUCCESS : EXEC_ERROR;
#endif
}

void semaphore_Wait(SemId_t id) {
#if defined(_WIN32) || defined(_WIN64)
	assert_true(WaitForSingleObject(id, INFINITE) == WAIT_OBJECT_0);
#else
	assert_true(sem_wait(id) == 0);
#endif
}

void semaphore_Post(SemId_t id) {
#if defined(_WIN32) || defined(_WIN64)
	assert_true(ReleaseSemaphore(id, 1, NULL));
#else
	assert_true(sem_post(id) == 0);
#endif
}

void semaphore_Close(SemId_t id) {
#if defined(_WIN32) || defined(_WIN64)
	assert_true(CloseHandle(id));
#else
	assert_true(sem_close(id) == 0);
#endif
}

EXEC_RETURN semaphore_Unlink(const char* name) {
#if defined(_WIN32) || defined(_WIN64)
	return EXEC_SUCCESS;
#else
	return sem_unlink(name) == 0 ? EXEC_SUCCESS : EXEC_ERROR;
#endif
}

/* once call */
#if defined(_WIN32) || defined(_WIN64)
BOOL CALLBACK __win32InitOnceCallback(PINIT_ONCE InitOnce, PVOID Parameter, PVOID *Context) {
	void(*callback)(void) = (void(*)(void))Parameter;
	callback();
	return TRUE;
}
#endif

EXEC_RETURN initonce_Call(InitOnce_t* once, void(*callback)(void)) {
#if defined(_WIN32) || defined(_WIN64)
	return InitOnceExecuteOnce(once, __win32InitOnceCallback, callback, NULL) ? EXEC_SUCCESS : EXEC_ERROR;
#else
	int res = pthread_once(once, callback);
	if (res) {
		errno = res;
		return EXEC_ERROR;
	}
	return EXEC_SUCCESS;
#endif
}

#ifdef	__cplusplus
}
#endif
