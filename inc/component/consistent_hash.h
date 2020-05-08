//
// Created by hujianzhe
//

#ifndef UTIL_C_COMPONENT_CONSISTENT_HASH_H
#define	UTIL_C_COMPONENT_CONSISTENT_HASH_H

#include "../platform_define.h"
#include "../datastruct/rbtree.h"

typedef RBTree_t ConsistentHash_t;

#ifdef __cplusplus
extern "C" {
#endif

__declspec_dll void consistenthashInit(ConsistentHash_t* ch);
__declspec_dll void consistenthashReg(ConsistentHash_t* ch, unsigned int key, void* value);
__declspec_dll void* consistenthashSelect(ConsistentHash_t* ch, unsigned int key);
__declspec_dll void consistenthashDelValue(ConsistentHash_t* ch, void* value);
__declspec_dll void consistenthashDelKey(ConsistentHash_t* ch, unsigned int key);
__declspec_dll void consistenthashFree(ConsistentHash_t* ch);

#ifdef __cplusplus
}
#endif

#endif // !UTIL_C_COMPONENT_CONSISTENT_HASH_H
