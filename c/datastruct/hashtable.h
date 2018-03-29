//
// Created by hujianzhe
//

#ifndef	UTIL_C_DATASTRUCT_HASHTABLE_H
#define	UTIL_C_DATASTRUCT_HASHTABLE_H

#include "var_type.h"

struct hashtable_t;
typedef struct hashtable_node_t {
	struct hashtable_t* table;
	struct hashtable_node_t *prev, *next;
	unsigned int bucket_index;
	var_t hash_key;
} hashtable_node_t;

typedef struct hashtable_t {
	struct hashtable_node_t** buckets;
	unsigned int buckets_size;
	int (*hash_key_cmp)(var_t, var_t);
	unsigned int (*hash_fn)(var_t);
} hashtable_t;

#ifdef	__cplusplus
extern "C" {
#endif

struct hashtable_t* hashtable_init(struct hashtable_t* hashtable,
		struct hashtable_node_t** buckets, unsigned int buckets_size,
		int (*hash_key_cmp)(var_t, var_t), unsigned int (*hash_fn)(var_t));

struct hashtable_node_t* hashtable_insert_node(struct hashtable_t* hashtable, struct hashtable_node_t* node, var_t key);
struct hashtable_node_t* hashtable_replace_node(struct hashtable_t* hashtable, struct hashtable_node_t* node, var_t key);
void hashtable_remove_node(struct hashtable_t* hashtable, struct hashtable_node_t* node);

struct hashtable_node_t* hashtable_search_key(struct hashtable_t* hashtable, var_t key);
struct hashtable_node_t* hashtable_remove_key(struct hashtable_t* hashtable, var_t key);

struct hashtable_node_t* hashtable_first_node(struct hashtable_t* hashtable);
struct hashtable_node_t* hashtable_next_node(struct hashtable_node_t* node);

#ifdef	__cplusplus
}
#endif

#endif
