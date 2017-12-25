//
// Created by hujianzhe
//

#include "hashtable.h"

#ifdef	__cplusplus
extern "C" {
#endif

struct hashtable_node_t** __get_bucket_list_head(struct hashtable_t* hashtable, var_t key) {
	unsigned int bucket_index = hashtable->hash_fn ? hashtable->hash_fn(key) : key.u64 % hashtable->buckets_size;
	return hashtable->buckets + bucket_index;
}

struct hashtable_node_t* __get_node(struct hashtable_node_t** bucket_list_head, var_t key) {
	if (*bucket_list_head) {
		struct hashtable_node_t* node;
		for (node = *bucket_list_head; node; node = node->next) {
			if (node->table->hash_key_cmp(node->hash_key, key)) {
				continue;
			}
			return node;
		}
	}
	return (struct hashtable_node_t*)0;
}

struct hashtable_t* hashtable_init(struct hashtable_t* hashtable,
		struct hashtable_node_t** buckets, unsigned int buckets_size,
		int (*hash_key_cmp)(var_t, var_t), unsigned int (*hash_fn)(var_t))
{
	hashtable->buckets = buckets;
	hashtable->buckets_size = buckets_size;
	hashtable->hash_key_cmp = hash_key_cmp;
	hashtable->hash_fn = hash_fn;
	return hashtable;
}

struct hashtable_node_t* hashtable_insert_node(struct hashtable_t* hashtable, struct hashtable_node_t* node, var_t key) {
	struct hashtable_node_t** bucket_list_head = __get_bucket_list_head(hashtable, key);
	struct hashtable_node_t* exist_node = __get_node(bucket_list_head, key);
	if (exist_node) {
		return exist_node;
	}
	if (*bucket_list_head) {
		(*bucket_list_head)->prev = node;
	}
	node->table = hashtable;
	node->prev = (struct hashtable_node_t*)0;
	node->next = *bucket_list_head;
	node->bucket_index = (unsigned int)(bucket_list_head - hashtable->buckets);
	node->hash_key = key;
	*bucket_list_head = node;
	return node;
}

struct hashtable_node_t* hashtable_replace_node(struct hashtable_t* hashtable, struct hashtable_node_t* node, var_t key) {
	struct hashtable_node_t** bucket_list_head = __get_bucket_list_head(hashtable, key);
	struct hashtable_node_t* exist_node = __get_node(bucket_list_head, key);
	if (exist_node) {
		if (exist_node->prev) {
			exist_node->prev->next = node;
		}
		else {
			*bucket_list_head = node;
		}
		if (exist_node->next) {
			exist_node->next->prev = node;
		}
		*node = *exist_node;
		node->hash_key = key;

		return exist_node;
	}
	if (*bucket_list_head) {
		(*bucket_list_head)->prev = node;
	}
	node->table = hashtable;
	node->prev = (struct hashtable_node_t*)0;
	node->next = *bucket_list_head;
	node->bucket_index = (unsigned int)(bucket_list_head - hashtable->buckets);
	node->hash_key = key;
	*bucket_list_head = node;
	return (struct hashtable_node_t*)0;
}

void hashtable_remove_node(struct hashtable_node_t* node) {
	struct hashtable_node_t** bucket_list_head = node->table->buckets + node->bucket_index;

	if (*bucket_list_head == node) {
		*bucket_list_head = node->next;
	}
	if (node->prev) {
		node->prev->next = node->next;
	}
	if (node->next) {
		node->next->prev = node->prev;
	}
}

struct hashtable_node_t* hashtable_search_key(struct hashtable_t* hashtable, var_t key) {
	return __get_node(__get_bucket_list_head(hashtable, key), key);
}

struct hashtable_node_t* hashtable_remove_key(struct hashtable_t* hashtable, var_t key) {
	struct hashtable_node_t* exist_node = hashtable_search_key(hashtable, key);
	if (exist_node) {
		hashtable_remove_node(exist_node);
	}
	return exist_node;
}

struct hashtable_node_t* hashtable_first_node(struct hashtable_t* hashtable) {
	unsigned int i;
	for (i = 0; i < hashtable->buckets_size; ++i) {
		if (hashtable->buckets[i]) {
			return hashtable->buckets[i];
		}
	}
	return (struct hashtable_node_t*)0;
}

struct hashtable_node_t* hashtable_next_node(struct hashtable_node_t* node) {
	if (node->next) {
		return node->next;
	}
	else {
		unsigned int i;
		for (i = node->bucket_index + 1; i < node->table->buckets_size; ++i) {
			if (node->table->buckets[i]) {
				return node->table->buckets[i];
			}
		}
		return (struct hashtable_node_t*)0;
	}
}

#ifdef	__cplusplus
}
#endif
