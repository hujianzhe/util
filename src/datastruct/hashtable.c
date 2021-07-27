//
// Created by hujianzhe
//

#include "../../inc/datastruct/hashtable.h"

#ifdef	__cplusplus
extern "C" {
#endif

static struct HashtableNode_t** __get_bucket_list_head(const struct Hashtable_t* hashtable, const void* key) {
	unsigned int bucket_index = hashtable->keyhash(key) % hashtable->buckets_size;
	return hashtable->buckets + bucket_index;
}

static struct HashtableNode_t* __get_node(struct HashtableNode_t** bucket_list_head, const void* key) {
	if (*bucket_list_head) {
		struct HashtableNode_t* node;
		for (node = *bucket_list_head; node; node = node->next) {
			if (node->table->keycmp(node->key, key)) {
				continue;
			}
			return node;
		}
	}
	return (struct HashtableNode_t*)0;
}

int hashtableDefaultKeyCmp(const void* node_key, const void* key) { return node_key != key; }
unsigned int hashtableDefaultKeyHash(const void* key) { return (ptrlen_t)key; }

int hashtableDefaultKeyCmpStr(const void* node_key, const void* key) {
	const unsigned char* s1 = (const unsigned char*)node_key;
	const unsigned char* s2 = (const unsigned char*)key;
	while (*s1 && *s1 == *s2) {
		++s1;
		++s2;
	}
	return *s1 != *s2;
}
unsigned int hashtableDefaultKeyHashStr(const void* key) {
	const char* str = (const char*)key;
	unsigned int hash = 0;
	while (*str)
		hash = hash * 131U + (*str++);
	return hash & 0x7fffffff;
}

struct Hashtable_t* hashtableInit(struct Hashtable_t* hashtable,
		struct HashtableNode_t** buckets, unsigned int buckets_size,
		int (*keycmp)(const void*, const void*),
		unsigned int (*keyhash)(const void*))
{
	unsigned int i;
	for (i = 0; i < buckets_size; ++i) {
		buckets[i] = (struct HashtableNode_t*)0;
	}
	hashtable->buckets = buckets;
	hashtable->buckets_size = buckets_size;
	hashtable->keycmp = keycmp;
	hashtable->keyhash = keyhash;
	return hashtable;
}

struct HashtableNode_t* hashtableInsertNode(struct Hashtable_t* hashtable, struct HashtableNode_t* node) {
	struct HashtableNode_t** bucket_list_head = __get_bucket_list_head(hashtable, node->key);
	struct HashtableNode_t* exist_node = __get_node(bucket_list_head, node->key);
	if (exist_node) {
		return exist_node;
	}
	if (*bucket_list_head) {
		(*bucket_list_head)->prev = node;
	}
	node->table = hashtable;
	node->prev = (struct HashtableNode_t*)0;
	node->next = *bucket_list_head;
	node->bucket_index = (unsigned int)(bucket_list_head - hashtable->buckets);
	*bucket_list_head = node;
	return node;
}

void hashtableReplaceNode(struct HashtableNode_t* old_node, struct HashtableNode_t* new_node) {
	if (old_node && old_node != new_node) {
		const void* key = new_node->key;
		if (old_node->prev) {
			old_node->prev->next = new_node;
		}
		else {
			struct HashtableNode_t** bucket_list_head = __get_bucket_list_head(old_node->table, old_node->key);
			*bucket_list_head = new_node;
		}
		if (old_node->next) {
			old_node->next->prev = new_node;
		}
		*new_node = *old_node;
		new_node->key = key;
	}
}

void hashtableRemoveNode(struct Hashtable_t* hashtable, struct HashtableNode_t* node) {
	if (hashtable == node->table) {
		struct HashtableNode_t** bucket_list_head = hashtable->buckets + node->bucket_index;

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

struct HashtableNode_t* hashtableSearchKey(const struct Hashtable_t* hashtable, const void* key) {
	return __get_node(__get_bucket_list_head(hashtable, key), key);
}

struct HashtableNode_t* hashtableRemoveKey(struct Hashtable_t* hashtable, const void* key) {
	struct HashtableNode_t* exist_node = hashtableSearchKey(hashtable, key);
	if (exist_node) {
		hashtableRemoveNode(hashtable, exist_node);
	}
	return exist_node;
}

struct HashtableNode_t* hashtableFirstNode(const struct Hashtable_t* hashtable) {
	unsigned int i;
	for (i = 0; i < hashtable->buckets_size; ++i) {
		if (hashtable->buckets[i]) {
			return hashtable->buckets[i];
		}
	}
	return (struct HashtableNode_t*)0;
}

struct HashtableNode_t* hashtableNextNode(struct HashtableNode_t* node) {
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
		return (struct HashtableNode_t*)0;
	}
}

void hashtableRehash(struct Hashtable_t* hashtable, struct HashtableNode_t** buckets, unsigned int buckets_size) {
	struct HashtableNode_t* cur, *next;
	struct Hashtable_t tb;
	hashtableInit(&tb, buckets, buckets_size, hashtable->keycmp, hashtable->keyhash);
	for (cur = hashtableFirstNode(hashtable); cur; cur = next) {
		next = hashtableNextNode(cur);
		hashtableRemoveNode(hashtable, cur);
		hashtableInsertNode(&tb, cur);
		cur->table = hashtable;
	}
	hashtable->buckets = buckets;
	hashtable->buckets_size = buckets_size;
}

#ifdef	__cplusplus
}
#endif
