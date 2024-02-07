//
// Created by hujianzhe
//

#include "../../inc/sysapi/process.h"
#include "../../inc/sysapi/assert.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#if !defined(_WIN32) && !defined(_WIN64)
#include <signal.h>
#ifdef	__SANITIZE_ADDRESS__
#include <sanitizer/asan_interface.h>
#endif
#endif

#ifdef	__cplusplus
extern "C" {
#endif

#if	defined(_WIN32) || defined(_WIN64)
static const char* __win32_path(char* path) {
	char* p;
	for (p = path; *p; ++p) {
		if ('/' == *p)
			*p = '\\';
	}
	return path;
}
#endif

/* process operator */
BOOL processCreate(Process_t* p_process, const char* path, const char* cmdarg) {
	char* _cmdarg = NULL;
#if defined(_WIN32) || defined(_WIN64)
	char szFullPath[MAX_PATH];
	PROCESS_INFORMATION pi;
	STARTUPINFOA si = {0};
	si.cb = sizeof(si);
	si.wShowWindow = TRUE;
	si.dwFlags = STARTF_USESHOWWINDOW;
	if (cmdarg && *cmdarg) {
		_cmdarg = (char*)malloc(strlen(cmdarg) + 1);
		if (!_cmdarg) {
			return FALSE;
		}
		strcpy(_cmdarg, cmdarg);
	}
	if (CreateProcessA(__win32_path(strcpy(szFullPath, path)), _cmdarg, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi)) {
		free(_cmdarg);
		CloseHandle(pi.hThread);
		if (p_process) {
			p_process->handle = pi.hProcess;
			p_process->id = pi.dwProcessId;
		}
		return TRUE;
	}
	free(_cmdarg);
	return FALSE;
#else
	int argc = 0;
	char* argv[64] = {0};
	if (cmdarg && *cmdarg) {
		int _argstart = 1;
		size_t i,cmdlen = strlen(cmdarg);
		_cmdarg = (char*)malloc(cmdlen + 1);
		if (!_cmdarg) {
			return FALSE;
		}
		strcpy(_cmdarg, cmdarg);
		for (i = 0; i < cmdlen && argc < sizeof(argv)/sizeof(argv[0]) - 1; ++i) {
			if (' ' == _cmdarg[i]) {
				if (0 == i) {
					argv[argc++] = (char*)"";
				}
				_argstart = 1;
				_cmdarg[i] = 0;
			} else if (_argstart) {
				argv[argc++] = _cmdarg + i;
				_argstart = 0;
			}
		}
	} else {
		argv[0] = (char*)path;
	}
	pid_t pid = fork();
	if (pid < 0) {
		free(_cmdarg);
		return FALSE;
	}
	else if (0 == pid) {
		if (execv(path, argv) < 0) {
			exit(EXIT_FAILURE);
		}
	}
	if (p_process) {
		p_process->id = pid;
	}
	free(_cmdarg);
	return TRUE;
#endif
}

BOOL processCancel(Process_t* process) {
#if defined(_WIN32) || defined(_WIN64)
	return TerminateProcess(process->handle, 0);
#else
	return kill(process->id, SIGKILL) == 0;
#endif
}

size_t processId(void) {
#if defined(_WIN32) || defined(_WIN64)
	return GetCurrentProcessId();
#else
	return getpid();
#endif
}

BOOL processWait(Process_t process, unsigned char* retcode) {
#if defined(_WIN32) || defined(_WIN64)
	DWORD _code;
	if (WaitForSingleObject(process.handle, INFINITE) == WAIT_OBJECT_0) {
		if (retcode && GetExitCodeProcess(process.handle, &_code)) {
			*retcode = (unsigned char)_code;
		}
		return CloseHandle(process.handle);
	}
	return FALSE;
#else
	int status = 0;
	if (waitpid(process.id, &status, 0) > 0) {
		if (WIFEXITED(status)) {
			*retcode = WEXITSTATUS(status);
			return TRUE;
		}
	}
	return FALSE;
#endif
}

BOOL processTryWait(Process_t process, unsigned char* retcode) {
#if defined(_WIN32) || defined(_WIN64)
	DWORD _code;
	if (WaitForSingleObject(process.handle, 0) == WAIT_OBJECT_0) {
		if (retcode && GetExitCodeProcess(process.handle, &_code)) {
			*retcode = (unsigned char)_code;
		}
		return CloseHandle(process.handle);
	}
	return FALSE;
#else
	int status = 0;
	if (waitpid(process.id, &status, WNOHANG) > 0) {
		if (WIFEXITED(status)) {
			*retcode = WEXITSTATUS(status);
			return TRUE;
		}
	}
	return FALSE;
#endif
}

/* thread operator */
#ifdef	_WIN32
static unsigned int __stdcall thread_entry_(void* arg) {
#else
static void* thread_entry_(void* arg) {
	sigset_t ss;
#endif
	void** boot_arg = (void**)arg;
	unsigned int(*f)(void*) = (unsigned int(*)(void*))boot_arg[0];
	void* param = boot_arg[1];
	free(arg);
#ifdef	_WIN32
	return f(param);
#else
	sigfillset(&ss);
	pthread_sigmask(SIG_SETMASK, &ss, NULL);
	return (void*)(size_t)(f(param));
#endif
}

BOOL threadCreate(Thread_t* p_thread, unsigned int(*entry)(void*), void* arg) {
#if defined(_WIN32) || defined(_WIN64)
	HANDLE handle;
	void** boot_arg = (void**)malloc(sizeof(void*) + sizeof(void*));
	if (!boot_arg) {
		return FALSE;
	}
	boot_arg[0] = (void*)entry;
	boot_arg[1] = arg;
	handle = (HANDLE)_beginthreadex(NULL, 0, thread_entry_, boot_arg, 0, &p_thread->id);
	if ((HANDLE)-1 == handle) {
		free(boot_arg);
		return FALSE;
	}
	p_thread->handle = handle;
	return TRUE;
#else
	int res;
	void** boot_arg = (void**)malloc(sizeof(void*) + sizeof(void*));
	if (!boot_arg) {
		return FALSE;
	}
	boot_arg[0] = (void*)entry;
	boot_arg[1] = arg;
	res = pthread_create(&p_thread->id, NULL, thread_entry_, boot_arg);
	if (res) {
		free(boot_arg);
		errno = res;
		return FALSE;
	}
	return TRUE;
#endif
}

BOOL threadDetach(Thread_t thread) {
#if defined(_WIN32) || defined(_WIN64)
	return CloseHandle(thread.handle);
#else
	int res = pthread_detach(thread.id);
	if (res) {
		errno = res;
		return FALSE;
	}
	return TRUE;
#endif
}

BOOL threadJoin(Thread_t thread, unsigned int* retcode) {
#if defined(_WIN32) || defined(_WIN64)
	if (WaitForSingleObject(thread.handle, INFINITE) == WAIT_OBJECT_0) {
		if (retcode && !GetExitCodeThread(thread.handle, (DWORD*)retcode)) {
			return FALSE;
		}
		return CloseHandle(thread.handle);
	}
	return FALSE;
#else
	void* _retcode;
	int res = pthread_join(thread.id, &_retcode);
	if (res) {
		errno = res;
		return FALSE;
	}
	if (retcode) {
		*retcode = (size_t)_retcode;
	}
	return TRUE;
#endif
}

void threadExit(unsigned int retcode) {
#if defined(_WIN32) || defined(_WIN64)
	_endthreadex(retcode);
#else
	pthread_exit((void*)(size_t)retcode);
#endif
}

BOOL threadEqual(Thread_t t1, Thread_t t2) {
#if defined(_WIN32) || defined(_WIN64)
	return t1.id == t2.id;
#else
	return pthread_equal(t1.id, t2.id);
#endif
}

Thread_t threadSelf(void) {
	Thread_t cur;
#if defined(_WIN32) || defined(_WIN64)
	cur.handle = GetCurrentThread();
	cur.id = GetCurrentThreadId();
#else
	cur.id = pthread_self();
#endif
	return cur;
}

void threadPause(void) {
#if defined(_WIN32) || defined(_WIN64)
	SuspendThread(GetCurrentThread());
#else
	pause();
#endif
}

void threadSleepMillsecond(unsigned int msec) {
#if defined(_WIN32) || defined(_WIN64)
	SleepEx(msec, FALSE);
#else
	struct timeval tval;
	tval.tv_sec = msec / 1000;
	tval.tv_usec = msec % 1000 * 1000;
	select(0, NULL, NULL, NULL, &tval);
#endif
}

void threadYield(void) {
#if defined(_WIN32) || defined(_WIN64)
	SwitchToThread();
#elif	__linux__
	/**
	  *	pthread_yield is deprecated
	  *
	int res = pthread_yield();
	if (res)
		errno = res;
	*/
	sched_yield();
#else
	sleep(0);
#endif
}

BOOL threadSetAffinity(Thread_t thread, unsigned int processor_index) {
	/* processor_index start 0...n */
#if defined(_WIN32) || defined(_WIN64)
	processor_index %= 32;
	return SetThreadAffinityMask(thread.handle, ((DWORD_PTR)1) << processor_index) != 0;
#elif defined(_GNU_SOURCE)
	int res;
	cpu_set_t cpuset;
	CPU_ZERO(&cpuset);
	CPU_SET(processor_index, &cpuset);
	if ((res = pthread_setaffinity_np(thread.id, sizeof(cpuset), &cpuset))) {
		errno = res;
		return FALSE;
	}
	return TRUE;
#else
	return TRUE;
#endif
}

/* thread local operator */
BOOL threadAllocLocalKey(Tls_t* key) {
#if defined(_WIN32) || defined(_WIN64)
	*key = TlsAlloc();
	return *key != TLS_OUT_OF_INDEXES;
#else
	int res = pthread_key_create(key, NULL);
	if (res) {
		errno = res;
		return FALSE;
	}
	return TRUE;
#endif
}

BOOL threadSetLocalValue(Tls_t key, void* value) {
#if defined(_WIN32) || defined(_WIN64)
	return TlsSetValue(key, value);
#else
	int res = pthread_setspecific(key, value);
	if (res) {
		errno = res;
		return FALSE;
	}
	return TRUE;
#endif
}

void* threadGetLocalValue(Tls_t key) {
#if defined(_WIN32) || defined(_WIN64)
	return TlsGetValue(key);
#else
	return pthread_getspecific(key);
#endif
}

BOOL threadFreeLocalKey(Tls_t key) {
#if defined(_WIN32) || defined(_WIN64)
	return TlsFree(key);
#else
	int res = pthread_key_delete(key);
	if (res) {
		errno = res;
		return FALSE;
	}
	return TRUE;
#endif
}

/* fiber operator */
#if defined(_WIN32) || defined(_WIN64)
typedef struct Fiber_t {
	int m_dead;
	int m_from_thread;
	void* m_arg;
	void(*m_entry)(struct Fiber_t*, void*);
	LPVOID m_ctx;
} Fiber_t;
static void WINAPI __fiber_start_routine(LPVOID lpFiberParameter) {
	Fiber_t* fiber = (Fiber_t*)lpFiberParameter;
	fiber->m_entry(fiber, fiber->m_arg);
}
#else
typedef struct Fiber_t {
	#ifdef	__SANITIZE_ADDRESS__
	struct {
		void* fake_stack;
		const void* old_stack;
		size_t old_stack_sz;
	} m_asan;
	#endif
	int m_dead;
	int m_from_thread;
	void* m_arg;
	void(*m_entry)(struct Fiber_t*, void*);
	ucontext_t m_ctx;
	unsigned char m_stack[1];
} Fiber_t;
static struct {
	int volatile spinlock;
	Fiber_t* volatile fiber;
	Fiber_t* volatile fiber_creator;
} s_ucontext_make_arg;
static void __fiber_start_routine(void) {
	Fiber_t* fiber = s_ucontext_make_arg.fiber;
	Fiber_t* fiber_creator = s_ucontext_make_arg.fiber_creator;
	__sync_lock_test_and_set(&s_ucontext_make_arg.spinlock, 0);

	#ifdef	__SANITIZE_ADDRESS__
	__sanitizer_finish_switch_fiber(NULL, NULL, NULL);
	__sanitizer_start_switch_fiber(&fiber->m_asan.fake_stack, fiber_creator->m_ctx.uc_stack.ss_sp, fiber_creator->m_ctx.uc_stack.ss_size);
	#endif
	swapcontext(&fiber->m_ctx, &fiber_creator->m_ctx);
	#ifdef	__SANITIZE_ADDRESS__
	__sanitizer_finish_switch_fiber(fiber->m_asan.fake_stack, NULL, NULL);
	#endif
	fiber->m_entry(fiber, fiber->m_arg);
}
#endif

struct Fiber_t* fiberFromThread(void) {
#if defined(_WIN32) || defined(_WIN64)
	Fiber_t* fiber = (Fiber_t*)malloc(sizeof(Fiber_t));
	if (!fiber) {
		return NULL;
	}
	fiber->m_ctx = ConvertThreadToFiberEx(fiber, FIBER_FLAG_FLOAT_SWITCH);
	if (!fiber->m_ctx) {
		free(fiber);
		return NULL;
	}
	fiber->m_arg = NULL;
	fiber->m_entry = NULL;
	fiber->m_dead = 0;
	fiber->m_from_thread = 1;
	return fiber;
#else
	Fiber_t* fiber = (Fiber_t*)calloc(1, sizeof(Fiber_t));
	if (!fiber) {
		return NULL;
	}
	fiber->m_arg = NULL;
	fiber->m_ctx.uc_link = NULL;
	fiber->m_dead = 0;
	fiber->m_from_thread = 1;
	return fiber;
#endif
}

struct Fiber_t* fiberCreate(struct Fiber_t* cur_fiber, size_t stack_size, void (*entry)(struct Fiber_t*, void*), void* arg) {
#if defined(_WIN32) || defined(_WIN64)
	Fiber_t* fiber = (Fiber_t*)malloc(sizeof(Fiber_t));
	if (!fiber) {
		return NULL;
	}
	fiber->m_ctx = CreateFiberEx(0, stack_size, FIBER_FLAG_FLOAT_SWITCH, __fiber_start_routine, fiber);
	if (!fiber->m_ctx) {
		free(fiber);
		return NULL;
	}
	fiber->m_arg = arg;
	fiber->m_entry = entry;
	fiber->m_dead = 0;
	fiber->m_from_thread = 0;
	return fiber;
#else
	Fiber_t* fiber;
	if (stack_size < SIGSTKSZ) { // or MINSIGSTKSZ
		stack_size = SIGSTKSZ;
	}
	fiber = (Fiber_t*)malloc(sizeof(Fiber_t) + stack_size);
	if (!fiber) {
		return NULL;
	}
	memset(&fiber->m_ctx, 0, sizeof(fiber->m_ctx));
	if (-1 == getcontext(&fiber->m_ctx)) {
		free(fiber);
		return NULL;
	}
	fiber->m_ctx.uc_stack.ss_size = stack_size;
	fiber->m_ctx.uc_stack.ss_sp = fiber->m_stack;
	fiber->m_ctx.uc_link = NULL;
	fiber->m_arg = arg;
	fiber->m_entry = entry;
	fiber->m_dead = 0;
	fiber->m_from_thread = 0;
	/* makecontext,, argc -- the number of argument, but sizeof every argument is sizeof(int)
	   so pass pointer is hard,,,use some int merge???,,,forget it.
	   so, use a urgly solution to avoid...\o/
	 */
	makecontext(&fiber->m_ctx, __fiber_start_routine, 0);
	while (__sync_lock_test_and_set(&s_ucontext_make_arg.spinlock, 1));
	s_ucontext_make_arg.fiber = fiber;
	s_ucontext_make_arg.fiber_creator = cur_fiber;

	#ifdef	__SANITIZE_ADDRESS__
	__sanitizer_start_switch_fiber(&cur_fiber->m_asan.fake_stack, fiber->m_ctx.uc_stack.ss_sp, fiber->m_ctx.uc_stack.ss_size);
	#endif
	swapcontext(&cur_fiber->m_ctx, &fiber->m_ctx);
	#ifdef	__SANITIZE_ADDRESS__
	__sanitizer_finish_switch_fiber(cur_fiber->m_asan.fake_stack, NULL, NULL);
	#endif
	return fiber;
#endif
}

void fiberSwitch(struct Fiber_t* cur, struct Fiber_t* to, int cur_dead) {
#if defined(_WIN32) || defined(_WIN64)
	assertTRUE(cur->m_ctx == GetCurrentFiber() && cur->m_ctx != to->m_ctx);
	cur->m_dead = cur_dead;
	SwitchToFiber(to->m_ctx);
#else
	assertTRUE(cur != to);
	cur->m_dead = cur_dead;
	#ifdef	__SANITIZE_ADDRESS__
	if (cur_dead) {
		__sanitizer_start_switch_fiber(NULL, to->m_ctx.uc_stack.ss_sp, to->m_ctx.uc_stack.ss_size);
	}
	else {
		__sanitizer_start_switch_fiber(&cur->m_asan.fake_stack, to->m_ctx.uc_stack.ss_sp, to->m_ctx.uc_stack.ss_size);
	}
	#endif
	swapcontext(&cur->m_ctx, &to->m_ctx);
	#ifdef	__SANITIZE_ADDRESS__
	__sanitizer_finish_switch_fiber(cur->m_asan.fake_stack, NULL, NULL);
	#endif
#endif
}

int fiberIsDead(struct Fiber_t* fiber) { return fiber->m_dead; }

void fiberFree(struct Fiber_t* fiber) {
	if (!fiber) {
		return;
	}
#if defined(_WIN32) || defined(_WIN64)
	if (fiber->m_from_thread) {
		assertTRUE(ConvertFiberToThread());
	}
	else {
		DeleteFiber(fiber->m_ctx);
	}
#endif
	free(fiber);
}

#ifdef	__cplusplus
}
#endif
