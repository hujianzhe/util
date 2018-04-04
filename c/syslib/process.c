//
// Created by hujianzhe
//

#include "process.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#if !defined(_WIN32) && !defined(_WIN64)
#include <signal.h>
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
BOOL process_Create(Process_t* p_process, const char* path, const char* cmdarg) {
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

BOOL process_Cancel(Process_t* process) {
#if defined(_WIN32) || defined(_WIN64)
	return TerminateProcess(process->handle, 0);
#else
	return kill(process->id, SIGKILL) == 0;
#endif
}

size_t process_Id(void) {
#if defined(_WIN32) || defined(_WIN64)
	return GetCurrentProcessId();
#else
	return getpid();
#endif
}

BOOL process_TryFreeZombie(Process_t* process, unsigned char* retcode) {
#if defined(_WIN32) || defined(_WIN64)
	DWORD _code;
	if (WaitForSingleObject(process->handle, 0) == WAIT_OBJECT_0) {
		if (retcode && GetExitCodeProcess(process->handle, &_code)) {
			*retcode = (unsigned char)_code;
		}
		return CloseHandle(process->handle);
	}
	return FALSE;
#else
	int status = 0;
	if (waitpid(process->id, &status, WNOHANG) > 0) {
		if (WIFEXITED(status)) {
			*retcode = WEXITSTATUS(status);
			return TRUE;
		}
	}
	return FALSE;
#endif
}

void* process_LoadModule(const char* path) {
#if	defined(_WIN32) || defined(_WIN64)
	if (path) {
		char szFullPath[MAX_PATH];
		return (void*)LoadLibraryA(__win32_path(strcpy(szFullPath, path)));
	}
	else {
		return (void*)GetModuleHandleA(NULL);
	}
#else
	return dlopen(path, RTLD_NOW);
#endif
}

void* process_GetModuleSymbolAddress(void* handle, const char* symbol_name) {
#if	defined(_WIN32) || defined(_WIN64)
	return GetProcAddress(handle, symbol_name);
#else
	return dlsym(handle, symbol_name);
#endif
}

BOOL process_UnloadModule(void* handle) {
	if (NULL == handle) {
		return TRUE;
	}
#if	defined(_WIN32) || defined(_WIN64)
	return FreeLibrary(handle);
#else
	return dlclose(handle) == 0;
#endif
}

/* thread operator */
BOOL thread_Create(Thread_t* p_thread, unsigned int (THREAD_CALL *entry)(void*), void* arg) {
#if defined(_WIN32) || defined(_WIN64)
	HANDLE handle = (HANDLE)_beginthreadex(NULL, 0, entry, arg, 0, NULL);
	if ((HANDLE)-1 == handle) {
		return FALSE;
	}
	if (p_thread) {
		*p_thread = handle;
	}
	return TRUE;
#else
	int res = pthread_create(p_thread, NULL, (void*(*)(void*))entry, arg);
	if (res) {
		errno = res;
		return FALSE;
	}
	return TRUE;
#endif
}

BOOL thread_Detach(Thread_t thread) {
#if defined(_WIN32) || defined(_WIN64)
	return CloseHandle(thread);
#else
	int res = pthread_detach(thread);
	if (res) {
		errno = res;
		return FALSE;
	}
	return TRUE;
#endif
}

BOOL thread_Join(Thread_t thread, unsigned int* retcode) {
#if defined(_WIN32) || defined(_WIN64)
	if (WaitForSingleObject(thread, INFINITE) == WAIT_OBJECT_0) {
		if (retcode && !GetExitCodeThread(thread, (DWORD*)retcode)) {
			return FALSE;
		}
		return CloseHandle(thread);
	}
	return FALSE;
#else
	void* _retcode;
	int res = pthread_join(thread, &_retcode);
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

void thread_Exit(unsigned int retcode) {
#if defined(_WIN32) || defined(_WIN64)
	_endthreadex(retcode);
#else
	pthread_exit((void*)(size_t)retcode);
#endif
}

void thread_Sleep(unsigned int msec) {
#if defined(_WIN32) || defined(_WIN64)
	SleepEx(msec, FALSE);
#else
	struct timeval tval;
	tval.tv_sec = msec / 1000;
	tval.tv_usec = msec % 1000 * 1000;
	select(0, NULL, NULL, NULL, &tval);
#endif
}

void thread_Yield(void) {
#if defined(_WIN32) || defined(_WIN64)
	SwitchToThread();
#elif	__linux__
	int res = pthread_yield();
	if (res)
		errno = res;
#else
	sleep(0);
#endif
}

BOOL thread_SetAffinity(Thread_t thread, unsigned int processor_index) {
	/* processor_index start 0...n */
#if defined(_WIN32) || defined(_WIN64)
	processor_index %= 32;
	return SetThreadAffinityMask(thread, ((DWORD_PTR)1) << processor_index) != 0;
#elif defined(_GNU_SOURCE)
	int res;
	cpu_set_t cpuset;
	CPU_ZERO(&cpuset);
	CPU_SET(processor_index, &cpuset);
	if ((res = pthread_setaffinity_np(thread, sizeof(cpuset), &cpuset))) {
		errno = res;
		return FALSE;
	}
	return TRUE;
#else
	return TRUE;
#endif
}

/* thread local operator */
BOOL thread_AllocLocalKey(Tls_t* key) {
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

BOOL thread_SetLocalValue(Tls_t key, void* value) {
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

void* thread_GetLocalValue(Tls_t key) {
#if defined(_WIN32) || defined(_WIN64)
	return TlsGetValue(key);
#else
	return pthread_getspecific(key);
#endif
}

BOOL thread_FreeLocalKey(Tls_t key) {
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

#ifdef	__cplusplus
}
#endif
