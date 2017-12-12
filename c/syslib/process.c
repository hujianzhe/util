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
EXEC_RETURN process_Create(PROCESS* p_process, const char* path, const char* cmdarg) {
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
			return EXEC_ERROR;
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
		return EXEC_SUCCESS;
	}
	free(_cmdarg);
	return EXEC_ERROR;
#else
	int argc = 0;
	char* argv[64] = {0};
	if (cmdarg && *cmdarg) {
		int _argstart = 1;
		size_t i,cmdlen = strlen(cmdarg);
		_cmdarg = (char*)malloc(cmdlen + 1);
		if (!_cmdarg) {
			return EXEC_ERROR;
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
		return EXEC_ERROR;
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
	return EXEC_SUCCESS;
#endif
}

EXEC_RETURN process_Cancel(PROCESS* process) {
#if defined(_WIN32) || defined(_WIN64)
	return TerminateProcess(process->handle, 0) ? EXEC_SUCCESS : EXEC_ERROR;
#else
	return kill(process->id, SIGKILL) == 0 ? EXEC_SUCCESS : EXEC_ERROR;
#endif
}

size_t process_Id(void) {
#if defined(_WIN32) || defined(_WIN64)
	return GetCurrentProcessId();
#else
	return getpid();
#endif
}

EXEC_RETURN process_TryFreeZombie(PROCESS* process, unsigned char* retcode) {
#if defined(_WIN32) || defined(_WIN64)
	DWORD _code;
	if (WaitForSingleObject(process->handle, 0) == WAIT_OBJECT_0) {
		if (retcode && GetExitCodeProcess(process->handle, &_code)) {
			*retcode = (unsigned char)_code;
		}
		return CloseHandle(process->handle) ? EXEC_SUCCESS : EXEC_ERROR;
	}
	return EXEC_ERROR;
#else
	int status = 0;
	if (waitpid(process->id, &status, WNOHANG) > 0) {
		if (WIFEXITED(status)) {
			*retcode = WEXITSTATUS(status);
			return EXEC_SUCCESS;
		}
	}
	return EXEC_ERROR;
#endif
}

void* process_LoadDLL(const char* path) {
#if	defined(_WIN32) || defined(_WIN64)
	char szFullPath[MAX_PATH];
	return (void*)LoadLibraryA(__win32_path(strcpy(szFullPath, path)));
#else
	return dlopen(path, RTLD_NOW);
#endif
}

void* process_GetDLLProcAddress(void* handle, const char* proc_name) {
#if	defined(_WIN32) || defined(_WIN64)
	return GetProcAddress(handle, proc_name);
#else
	return dlsym(handle, proc_name);
#endif
}

EXEC_RETURN process_UnloadDLL(void* handle) {
	if (NULL == handle) {
		return EXEC_SUCCESS;
	}
#if	defined(_WIN32) || defined(_WIN64)
	return FreeLibrary(handle) ? EXEC_SUCCESS : EXEC_ERROR;
#else
	return dlclose(handle) ? EXEC_ERROR : EXEC_SUCCESS;
#endif
}

/* thread operator */
EXEC_RETURN thread_Create(THREAD* p_thread, unsigned int (THREAD_CALL *entry)(void*), void* arg) {
#if defined(_WIN32) || defined(_WIN64)
	HANDLE handle = (HANDLE)_beginthreadex(NULL, 0, entry, arg, 0, NULL);
	if ((HANDLE)-1 == handle) {
		return EXEC_ERROR;
	}
	if (p_thread) {
		*p_thread = handle;
	}
	return EXEC_SUCCESS;
#else
	int res = pthread_create(p_thread, NULL, (void*(*)(void*))entry, arg);
	if (res) {
		errno = res;
		return EXEC_ERROR;
	}
	return EXEC_SUCCESS;
#endif
}

EXEC_RETURN thread_Detach(THREAD thread) {
#if defined(_WIN32) || defined(_WIN64)
	return CloseHandle(thread) ? EXEC_SUCCESS : EXEC_ERROR;
#else
	int res = pthread_detach(thread);
	if (res) {
		errno = res;
		return EXEC_ERROR;
	}
	return EXEC_SUCCESS;
#endif
}

EXEC_RETURN thread_Join(THREAD thread, unsigned int* retcode) {
#if defined(_WIN32) || defined(_WIN64)
	if (WaitForSingleObject(thread, INFINITE) == WAIT_OBJECT_0) {
		if (retcode && !GetExitCodeThread(thread, (DWORD*)retcode)) {
			return EXEC_ERROR;
		}
		return CloseHandle(thread) ? EXEC_SUCCESS : EXEC_ERROR;
	}
	return EXEC_ERROR;
#else
	void* _retcode;
	int res = pthread_join(thread, &_retcode);
	if (res) {
		errno = res;
		return EXEC_ERROR;
	}
	if (retcode) {
		*retcode = (size_t)_retcode;
	}
	return EXEC_SUCCESS;
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

EXEC_RETURN thread_SetAffinity(THREAD thread, unsigned int processor_index) {
	/* processor_index start 0...n */
#if defined(_WIN32) || defined(_WIN64)
	processor_index %= 32;
	return SetThreadAffinityMask(thread, ((DWORD_PTR)1) << processor_index) ? EXEC_SUCCESS : EXEC_ERROR;
#elif defined(_GNU_SOURCE)
	int res;
	cpu_set_t cpuset;
	CPU_ZERO(&cpuset);
	CPU_SET(processor_index, &cpuset);
	if ((res = pthread_setaffinity_np(thread, sizeof(cpuset), &cpuset))) {
		errno = res;
		return EXEC_ERROR;
	}
	return EXEC_SUCCESS;
#else
	return EXEC_SUCCESS;
#endif
}

/* thread local operator */
EXEC_RETURN thread_AllocLocalKey(THREAD_LOCAL_KEY* key) {
#if defined(_WIN32) || defined(_WIN64)
	*key = TlsAlloc();
	return *key != TLS_OUT_OF_INDEXES ? EXEC_SUCCESS : EXEC_ERROR;
#else
	int res = pthread_key_create(key, NULL);
	if (res) {
		errno = res;
		return EXEC_ERROR;
	}
	return EXEC_SUCCESS;
#endif
}

EXEC_RETURN thread_SetLocalValue(THREAD_LOCAL_KEY key, void* value) {
#if defined(_WIN32) || defined(_WIN64)
	return TlsSetValue(key, value) ? EXEC_SUCCESS : EXEC_ERROR;
#else
	int res = pthread_setspecific(key, value);
	if (res) {
		errno = res;
		return EXEC_ERROR;
	}
	return EXEC_SUCCESS;
#endif
}

void* thread_GetLocalValue(THREAD_LOCAL_KEY key) {
#if defined(_WIN32) || defined(_WIN64)
	return TlsGetValue(key);
#else
	return pthread_getspecific(key);
#endif
}

EXEC_RETURN thread_FreeLocalKey(THREAD_LOCAL_KEY key) {
#if defined(_WIN32) || defined(_WIN64)
	return TlsFree(key) ? EXEC_SUCCESS:EXEC_ERROR;
#else
	int res = pthread_key_delete(key);
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
