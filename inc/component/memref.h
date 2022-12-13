//
// Created by hujianzhe
//

#ifndef	UTIL_C_COMPONENT_MEMREF_H
#define	UTIL_C_COMPONENT_MEMREF_H

#include "../../inc/platform_define.h"

struct MemRef_t;

#ifdef	__cplusplus
extern "C" {
#endif

__declspec_dll struct MemRef_t* memrefCreate(void* p, void(*fn_free)(void*));
__declspec_dll void* memrefGetPtr(struct MemRef_t* ref);
__declspec_dll struct MemRef_t* memrefIncr(struct MemRef_t* ref);
__declspec_dll void memrefDecr(struct MemRef_t** p_ref);

__declspec_dll struct MemRef_t* memrefLockWeak(struct MemRef_t* ref);
__declspec_dll struct MemRef_t* memrefIncrWeak(struct MemRef_t* ref);
__declspec_dll void memrefDecrWeak(struct MemRef_t** p_ref);

#ifdef	__cplusplus
}
#endif

#endif
