//
// Created by hujianzhe
//

#include "hash.h"

#ifdef	__cplusplus
extern "C" {
#endif

unsigned int hashBKDR(const char* str) {
	/* seed is 131 */
	unsigned int hash = 0;
	while (*str)
		hash = hash * 131U + (*str++);
	return hash & 0x7fffffff;
}

unsigned int hashDJB(const char *str) {
	unsigned int hash = 5381;
	while (*str)
		hash = ((hash << 5) + hash) + (*str++);
	return hash & 0x7fffffff;
}

unsigned int hashJenkins(const char* key, unsigned int kenlen) {
	unsigned int i = 0;
	unsigned int hash = 0;
	while (i < kenlen) {
		hash += key[i++];
		hash += hash << 10;
		hash ^= hash >> 6;
	}
	hash += hash << 3;
	hash ^= hash >> 11;
	hash += hash << 15;
	return hash;
}

unsigned int hashMurmur2(const char *key, unsigned int keylen) {
	unsigned int h, k;
	h = 0 ^ keylen;

	while (keylen >= 4) {
		k = key[0];
		k |= key[1] << 8;
		k |= key[2] << 16;
		k |= key[3] << 24;

		k *= 0x5bd1e995;
		k ^= k >> 24;
		k *= 0x5bd1e995;

		h *= 0x5bd1e995;
		h ^= k;

		key += 4;
		keylen -= 4;
	}

	switch (keylen) {
		case 3:
			h ^= key[2] << 16;
			/* no break */
		case 2:
			h ^= key[1] << 8;
			/* no break */
		case 1:
			h ^= key[0];
			h *= 0x5bd1e995;
	}

	h ^= h >> 13;
	h *= 0x5bd1e995;
	h ^= h >> 15;

	return h;
}

#ifdef	__cplusplus
}
#endif