//
// Created by hujianzhe
//

#include "../../inc/component/memref.h"
#include "../../inc/sysapi/atomic.h"
#include <stdlib.h>

typedef struct MemRef_t {
	void* p;
	void(*fn_free)(void*);
	Atom32_t sp_cnt;
	Atom32_t wp_cnt;
} MemRef_t;

#ifdef	__cplusplus
extern "C" {
#endif

MemRef_t* memrefCreate(void* p, void(*fn_free)(void*)) {
	if (p) {
		MemRef_t* ref = (MemRef_t*)malloc(sizeof(MemRef_t));
		if (!ref) {
			return NULL;
		}
		ref->p = p;
		ref->fn_free = fn_free;
		ref->sp_cnt = 1;
		ref->wp_cnt = 1;
		return ref;
	}
	return NULL;
}

void* memrefGetPtr(MemRef_t* ref) { return ref ? ref->p : NULL; }

MemRef_t* memrefIncr(MemRef_t* ref) {
	if (ref) {
		_xadd32(&ref->sp_cnt, 1);
	}
	return ref;
}

void memrefDecr(MemRef_t** p_ref) {
	Atom32_t sp_cnt;
	MemRef_t* ref = *p_ref;
	if (!ref) {
		return;
	}
	*p_ref = NULL;

	sp_cnt = _xadd32(&ref->sp_cnt, -1);
	if (sp_cnt > 1) {
		return;
	}
	if (ref->fn_free) {
		ref->fn_free(ref->p);
	}
	ref->p = NULL;
	if (_xadd32(&ref->wp_cnt, -1) > 1) {
		return;
	}
	free(ref);
}

MemRef_t* memrefLockWeak(MemRef_t* ref) {
	if (!ref || !ref->p) {
		return NULL;
	}
	while (1) {
		Atom32_t tmp = _xadd32(&ref->sp_cnt, 0);
		if (tmp < 1) {
			return NULL;
		}
		if (_cmpxchg32(&ref->sp_cnt, tmp + 1, tmp) == tmp) {
			return ref;
		}
	}
}

MemRef_t* memrefIncrWeak(MemRef_t* ref) {
	if (ref) {
		_xadd32(&ref->wp_cnt, 1);
	}
	return ref;
}

void memrefDecrWeak(MemRef_t** p_ref) {
	MemRef_t* ref = *p_ref;
	if (!ref) {
		return;
	}
	*p_ref = NULL;

	if (_xadd32(&ref->wp_cnt, -1) > 1) {
		return;
	}
	free(ref);
}

#ifdef	__cplusplus
}
#endif
