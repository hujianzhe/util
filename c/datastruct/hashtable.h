//
// Created by hujianzhe
//

#ifndef	UTIL_C_DATASTRUCT_HASHTABLE_H
#define	UTIL_C_DATASTRUCT_HASHTABLE_H

struct Hashtable_t;
typedef struct HashtableNode_t {
	const void* key;
	struct Hashtable_t* table;
	struct HashtableNode_t *prev, *next;
	unsigned int bucket_index;
} HashtableNode_t;

typedef struct Hashtable_t {
	struct HashtableNode_t** buckets;
	unsigned int buckets_size;
	int (*keycmp)(struct HashtableNode_t*, const void*);
	unsigned int (*keyhash)(const void*);
} Hashtable_t;

#ifdef	__cplusplus
extern "C" {
#endif

struct Hashtable_t* hashtableInit(struct Hashtable_t* hashtable,
		struct HashtableNode_t** buckets, unsigned int buckets_size,
		int (*keycmp)(struct HashtableNode_t*, const void*),
		unsigned int (*keyhash)(const void*));

struct HashtableNode_t* hashtableInsertNode(struct Hashtable_t* hashtable, struct HashtableNode_t* node);
void hashtableReplaceNode(struct HashtableNode_t* old_node, struct HashtableNode_t* new_node);
void hashtableRemoveNode(struct Hashtable_t* hashtable, struct HashtableNode_t* node);

struct HashtableNode_t* hashtableSearchKey(const struct Hashtable_t* hashtable, const void* key);
struct HashtableNode_t* hashtableRemoveKey(struct Hashtable_t* hashtable, const void* key);

struct HashtableNode_t* hashtableFirstNode(const struct Hashtable_t* hashtable);
struct HashtableNode_t* hashtableNextNode(struct HashtableNode_t* node);

#ifdef	__cplusplus
}
#endif

#endif
