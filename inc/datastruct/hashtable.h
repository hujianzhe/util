//
// Created by hujianzhe
//

#ifndef	UTIL_C_DATASTRUCT_HASHTABLE_H
#define	UTIL_C_DATASTRUCT_HASHTABLE_H

#include "../compiler_define.h"

typedef union {
	const void* ptr;
	SignedPtr_t int_ptr;
	UnsignedPtr_t uint_ptr;
	int i32;
	unsigned int u32;
	long long i64;
	unsigned long long u64;
} HashtableNodeKey_t;

struct Hashtable_t;
typedef struct HashtableNode_t {
	HashtableNodeKey_t key;
	struct HashtableNode_t *prev, *next;
	struct HashtableNode_t *ele_prev, *ele_next;
	unsigned int bucket_index;
} HashtableNode_t;

typedef struct Hashtable_t {
	struct HashtableNode_t** buckets;
	unsigned int buckets_size;
	int (*keycmp)(const HashtableNodeKey_t*, const HashtableNodeKey_t*);
	unsigned int (*keyhash)(const HashtableNodeKey_t*);
	struct HashtableNode_t* head;
	UnsignedPtr_t count;
} Hashtable_t;

#ifdef	__cplusplus
extern "C" {
#endif

__declspec_dll int hashtableDefaultKeyCmp32(const HashtableNodeKey_t*, const HashtableNodeKey_t*);
__declspec_dll unsigned int hashtableDefaultKeyHash32(const HashtableNodeKey_t*);
__declspec_dll int hashtableDefaultKeyCmp64(const HashtableNodeKey_t*, const HashtableNodeKey_t*);
__declspec_dll unsigned int hashtableDefaultKeyHash64(const HashtableNodeKey_t*);
__declspec_dll int hashtableDefaultKeyCmpSZ(const HashtableNodeKey_t*, const HashtableNodeKey_t*);
__declspec_dll unsigned int hashtableDefaultKeyHashSZ(const HashtableNodeKey_t*);
__declspec_dll int hashtableDefaultKeyCmpStr(const HashtableNodeKey_t*, const HashtableNodeKey_t*);
__declspec_dll unsigned int hashtableDefaultKeyHashStr(const HashtableNodeKey_t*);

__declspec_dll struct Hashtable_t* hashtableInit(struct Hashtable_t* hashtable,
		struct HashtableNode_t** buckets, unsigned int buckets_size,
		int (*keycmp)(const HashtableNodeKey_t*, const HashtableNodeKey_t*),
		unsigned int (*keyhash)(const HashtableNodeKey_t*));

__declspec_dll struct HashtableNode_t* hashtableInsertNode(struct Hashtable_t* hashtable, struct HashtableNode_t* node);
__declspec_dll void hashtableReplaceNode(struct Hashtable_t* hashtable, struct HashtableNode_t* old_node, struct HashtableNode_t* new_node);
__declspec_dll void hashtableRemoveNode(struct Hashtable_t* hashtable, struct HashtableNode_t* node);

__declspec_dll struct HashtableNode_t* hashtableSearchKey(const struct Hashtable_t* hashtable, const HashtableNodeKey_t key);
__declspec_dll struct HashtableNode_t* hashtableRemoveKey(struct Hashtable_t* hashtable, const HashtableNodeKey_t key);

__declspec_dll int hashtableIsEmpty(const struct Hashtable_t* hashtable);
__declspec_dll struct HashtableNode_t* hashtableFirstNode(const struct Hashtable_t* hashtable);
__declspec_dll struct HashtableNode_t* hashtableNextNode(struct HashtableNode_t* node);

__declspec_dll void hashtableRehash(struct Hashtable_t* hashtable, struct HashtableNode_t** buckets, unsigned int buckets_size);
__declspec_dll void hashtableSwap(struct Hashtable_t* h1, struct Hashtable_t* h2);

#ifdef	__cplusplus
}
#endif

#endif
