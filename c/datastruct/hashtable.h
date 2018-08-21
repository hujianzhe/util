//
// Created by hujianzhe
//

#ifndef	UTIL_C_DATASTRUCT_HASHTABLE_H
#define	UTIL_C_DATASTRUCT_HASHTABLE_H

struct hashtable_t;
typedef struct hashtable_node_t {
	const void* key;
	struct hashtable_t* table;
	struct hashtable_node_t *prev, *next;
	unsigned int bucket_index;
} hashtable_node_t;

typedef struct hashtable_t {
	struct hashtable_node_t** buckets;
	unsigned int buckets_size;
	int (*keycmp)(struct hashtable_node_t*, const void*);
	unsigned int (*keyhash)(const void*);
} hashtable_t;

#ifdef	__cplusplus
extern "C" {
#endif

struct hashtable_t* hashtable_init(struct hashtable_t* hashtable,
		struct hashtable_node_t** buckets, unsigned int buckets_size,
		int (*keycmp)(struct hashtable_node_t*, const void*),
		unsigned int (*keyhash)(const void*));

struct hashtable_node_t* hashtable_insert_node(struct hashtable_t* hashtable, struct hashtable_node_t* node);
void hashtable_replace_node(struct hashtable_node_t* old_node, struct hashtable_node_t* new_node);
void hashtable_remove_node(struct hashtable_t* hashtable, struct hashtable_node_t* node);

struct hashtable_node_t* hashtable_search_key(const struct hashtable_t* hashtable, const void* key);
struct hashtable_node_t* hashtable_remove_key(struct hashtable_t* hashtable, const void* key);

struct hashtable_node_t* hashtable_first_node(const struct hashtable_t* hashtable);
struct hashtable_node_t* hashtable_next_node(struct hashtable_node_t* node);

#ifdef	__cplusplus
}
#endif

#endif
