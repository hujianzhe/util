//
// Created by hujianzhe
//

#include "../../inc/component/memref.h"
#include "../../inc/sysapi/atomic.h"
#include <stdlib.h>

struct MemRef_t {
	void* p;
	Atom32_t sp_cnt;
	Atom32_t wp_cnt;
};

#ifdef	__cplusplus
extern "C" {
#endif

struct MemRef_t* memrefCreate(void* p) {
	if (p) {
		struct MemRef_t* ref = (struct MemRef_t*)malloc(sizeof(struct MemRef_t));
		if (!ref) {
			return NULL;
		}
		ref->p = p;
		ref->sp_cnt = 1;
		ref->wp_cnt = 1;
		return ref;
	}
	return NULL;
}

struct MemRef_t* memrefIncrStrong(struct MemRef_t* ref) {
	if (ref) {
		_xadd32(&ref->sp_cnt, 1);
	}
	return ref;
}

void* memrefDecrStrong(struct MemRef_t** p_ref) {
	struct MemRef_t* ref = *p_ref;
	if (ref) {
		void* p;
		Atom32_t sp_cnt = _xadd32(&ref->sp_cnt, -1);
		if (sp_cnt > 1) {
			return NULL;
		}
		p = ref->p;
		ref->p = NULL;
		if (_xadd32(&ref->wp_cnt, -1) <= 1) {
			free(ref);
			*p_ref = NULL;
		}
		return p;
	}
	return NULL;
}

void* memrefLock(struct MemRef_t* ref) {
	if (!ref || !ref->p) {
		return NULL;
	}
	while (1) {
		Atom32_t tmp = _xadd32(&ref->sp_cnt, 0);
		if (tmp < 1) {
			return NULL;
		}
		if (_cmpxchg32(&ref->sp_cnt, tmp + 1, tmp) == tmp) {
			return ref->p;
		}
	}
}

struct MemRef_t* memrefIncrWeak(struct MemRef_t* ref) {
	if (ref) {
		_xadd32(&ref->wp_cnt, 1);
	}
	return ref;
}

void memrefDecrWeak(struct MemRef_t** p_ref) {
	struct MemRef_t* ref = *p_ref;
	if (!ref) {
		return;
	}
	if (_xadd32(&ref->wp_cnt, -1) <= 1) {
		free(ref);
		*p_ref = NULL;
	}
}

#ifdef	__cplusplus
}
#endif
