//
// Created by hujianzhe
//

#include "hashtable.h"

#ifdef	__cplusplus
extern "C" {
#endif

struct hashtable_node_t** __get_bucket_list_head(const struct hashtable_t* hashtable, const void* key) {
	unsigned int bucket_index = hashtable->keyhash(key) % hashtable->buckets_size;
	return hashtable->buckets + bucket_index;
}

struct hashtable_node_t* __get_node(struct hashtable_node_t** bucket_list_head, const void* key) {
	if (*bucket_list_head) {
		struct hashtable_node_t* node;
		for (node = *bucket_list_head; node; node = node->next) {
			if (node->table->keycmp(node, key)) {
				continue;
			}
			return node;
		}
	}
	return (struct hashtable_node_t*)0;
}

struct hashtable_t* hashtable_init(struct hashtable_t* hashtable,
		struct hashtable_node_t** buckets, unsigned int buckets_size,
		int (*keycmp)(struct hashtable_node_t*, const void*),
		unsigned int (*keyhash)(const void*))
{
	unsigned int i;
	for (i = 0; i < buckets_size; ++i) {
		buckets[i] = (struct hashtable_node_t*)0;
	}
	hashtable->buckets = buckets;
	hashtable->buckets_size = buckets_size;
	hashtable->keycmp = keycmp;
	hashtable->keyhash = keyhash;
	return hashtable;
}

struct hashtable_node_t* hashtable_insert_node(struct hashtable_t* hashtable, struct hashtable_node_t* node) {
	struct hashtable_node_t** bucket_list_head = __get_bucket_list_head(hashtable, node->key);
	struct hashtable_node_t* exist_node = __get_node(bucket_list_head, node->key);
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
	*bucket_list_head = node;
	return node;
}

void hashtable_replace_node(struct hashtable_node_t* old_node, struct hashtable_node_t* new_node) {
	if (old_node && old_node != new_node) {
		const void* key = new_node->key;
		if (old_node->prev) {
			old_node->prev->next = new_node;
		}
		else {
			struct hashtable_node_t** bucket_list_head = __get_bucket_list_head(old_node->table, old_node->key);
			*bucket_list_head = new_node;
		}
		if (old_node->next) {
			old_node->next->prev = new_node;
		}
		*new_node = *old_node;
		new_node->key = key;
	}
}

void hashtable_remove_node(struct hashtable_t* hashtable, struct hashtable_node_t* node) {
	if (hashtable == node->table) {
		struct hashtable_node_t** bucket_list_head = hashtable->buckets + node->bucket_index;

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
}

struct hashtable_node_t* hashtable_search_key(const struct hashtable_t* hashtable, const void* key) {
	return __get_node(__get_bucket_list_head(hashtable, key), key);
}

struct hashtable_node_t* hashtable_remove_key(struct hashtable_t* hashtable, const void* key) {
	struct hashtable_node_t* exist_node = hashtable_search_key(hashtable, key);
	if (exist_node) {
		hashtable_remove_node(hashtable, exist_node);
	}
	return exist_node;
}

struct hashtable_node_t* hashtable_first_node(const struct hashtable_t* hashtable) {
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
