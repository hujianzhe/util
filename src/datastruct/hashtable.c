//
// Created by hujianzhe
//

#include "../../inc/datastruct/hashtable.h"

#ifdef	__cplusplus
extern "C" {
#endif

static struct HashtableNode_t** __get_bucket_list_head(const struct Hashtable_t* hashtable, const HashtableNodeKey_t* key) {
	unsigned int bucket_index = hashtable->keyhash(key) % hashtable->buckets_size;
	return hashtable->buckets + bucket_index;
}

static struct HashtableNode_t* __get_node(struct HashtableNode_t** bucket_list_head, const HashtableNodeKey_t* key) {
	if (*bucket_list_head) {
		struct HashtableNode_t* node;
		for (node = *bucket_list_head; node; node = node->next) {
			if (node->table->keycmp(&node->key, key)) {
				continue;
			}
			return node;
		}
	}
	return (struct HashtableNode_t*)0;
}

int hashtableDefaultKeyCmp32(const HashtableNodeKey_t* node_key, const HashtableNodeKey_t* key) {
	return node_key->u32 != key->u32;
}
unsigned int hashtableDefaultKeyHash32(const HashtableNodeKey_t* key) { return key->u32; }

int hashtableDefaultKeyCmp64(const HashtableNodeKey_t* node_key, const HashtableNodeKey_t* key) {
	return node_key->u64 != key->u64;
}
unsigned int hashtableDefaultKeyHash64(const HashtableNodeKey_t* key) {
	unsigned int hash = 0, keylen = sizeof(key->u64);
	const char* p;
	for (p = (const char*)(&key->u64); keylen; --keylen, ++p) {
		hash += *p;
		hash += hash << 10;
		hash ^= hash >> 6;
	}
	hash += hash << 3;
	hash ^= hash >> 11;
	hash += hash << 15;
	return hash;
}

int hashtableDefaultKeyCmpSZ(const HashtableNodeKey_t* node_key, const HashtableNodeKey_t* key) {
	return node_key->ptr != key->ptr;
}

unsigned int hashtableDefaultKeyHashSZ(const HashtableNodeKey_t* key) {
	if (sizeof(key->ptr) <= sizeof(unsigned int)) {
		return (UnsignedPtr_t)key->ptr;
	}
	else {
		unsigned int hash = 0, keylen = sizeof(key->ptr);
		const char* p;
		for (p = (const char*)(&key->ptr); keylen; --keylen, ++p) {
			hash += *p;
			hash += hash << 10;
			hash ^= hash >> 6;
		}
		hash += hash << 3;
		hash ^= hash >> 11;
		hash += hash << 15;
		return hash;
	}
}

int hashtableDefaultKeyCmpStr(const HashtableNodeKey_t* node_key, const HashtableNodeKey_t* key) {
	const unsigned char* s1 = (const unsigned char*)node_key->ptr;
	const unsigned char* s2 = (const unsigned char*)key->ptr;
	while (*s1 && *s1 == *s2) {
		++s1;
		++s2;
	}
	return *s1 != *s2;
}
unsigned int hashtableDefaultKeyHashStr(const HashtableNodeKey_t* key) {
	const char* str = (const char*)key->ptr;
	unsigned int hash = 0;
	while (*str)
		hash = hash * 131U + (*str++);
	return hash & 0x7fffffff;
}

struct Hashtable_t* hashtableInit(struct Hashtable_t* hashtable,
		struct HashtableNode_t** buckets, unsigned int buckets_size,
		int (*keycmp)(const HashtableNodeKey_t*, const HashtableNodeKey_t*),
		unsigned int (*keyhash)(const HashtableNodeKey_t*))
{
	unsigned int i;
	for (i = 0; i < buckets_size; ++i) {
		buckets[i] = (struct HashtableNode_t*)0;
	}
	hashtable->buckets = buckets;
	hashtable->buckets_size = buckets_size;
	hashtable->keycmp = keycmp;
	hashtable->keyhash = keyhash;
	hashtable->count = 0;
	return hashtable;
}

struct HashtableNode_t* hashtableInsertNode(struct Hashtable_t* hashtable, struct HashtableNode_t* node) {
	struct HashtableNode_t** bucket_list_head = __get_bucket_list_head(hashtable, &node->key);
	struct HashtableNode_t* exist_node = __get_node(bucket_list_head, &node->key);
	if (exist_node) {
		return exist_node;
	}
	++(hashtable->count);
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
		HashtableNodeKey_t key = new_node->key;
		if (old_node->prev) {
			old_node->prev->next = new_node;
		}
		else {
			struct HashtableNode_t** bucket_list_head = __get_bucket_list_head(old_node->table, &old_node->key);
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
		--(hashtable->count);
	}
}

struct HashtableNode_t* hashtableSearchKey(const struct Hashtable_t* hashtable, const HashtableNodeKey_t key) {
	return __get_node(__get_bucket_list_head(hashtable, &key), &key);
}

struct HashtableNode_t* hashtableRemoveKey(struct Hashtable_t* hashtable, const HashtableNodeKey_t key) {
	struct HashtableNode_t* exist_node = hashtableSearchKey(hashtable, key);
	if (exist_node) {
		hashtableRemoveNode(hashtable, exist_node);
	}
	return exist_node;
}

int hashtableIsEmpty(const struct Hashtable_t* hashtable) {
	return 0 == hashtable->count;
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
