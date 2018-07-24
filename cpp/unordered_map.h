//
// Created by hujianzhe on 18-7-24
//

#ifndef UTIL_CPP_UNORDERED_MAP_H
#define	UTIL_CPP_UNORDERED_MAP_H

#include "cpp_compiler_define.h"
#if __CPP_VERSION >= 2011
#include <unordered_map>
#else
#include "../c/datastruct/hashtable.h"
#include <utility>
#include <stddef.h>
namespace std {
template <typename K, typename V>
class unordered_map {
public:
	typedef K			key_type;
	typedef pair<K, V>	value_type;
	typedef struct Xnode : public hashtable_node_t {
		value_type v;
		std::pair<const K&, V&> iter;

		Xnode(void) {
			key = &v.k;
			iter.first = v.first;
			iter.second = v.second;
		}
	} Xnode;

	class iterator {
	public:
		iterator(hashtable_node_t* p = NULL) : x(p) {}
		iterator(const iterator& i) : x(i.x) {}
		iterator& opeartor=(const iterator& i) {
			x = i.x;
			return *this;
		}

		std::pair<const K&, V&>* operator->(void) const {
			return &((Xnode*)x)->iter;
		}
		std::pair<const K&, V&>& operator*(void) const {
			return ((Xnode*)x)->iter;
		}
		iterator opeartor++(int unused) const {
			iterator it = *this;
			this->x = hashtable_next_node(x);
			return it;
		}
		iterator& opeartor++(void) const {
			this->x = hashtable_next_node(x);
			return *this;
		}

	private:
		hashtable_node_t* x;
	};

private:
	static int keycmp(hashtable_node_t* _n, void* key) {
		return !(((value_type*)_n)->k == *(K*)key);
	}

	static unsigned int keyhash(void* key) {
		return (unsigned int)(size_t)(*(K*)key);
	}

public:
	unordered_map(void) :
		m_size(0)
	{
		hashtable_init(&m_table, buckets, sizeof(m_buckets) / sizeof(m_buckets[0]), keycmp, keyhash);
	}

	~unordered_map(void) { clear(); }

	void clear(void) {
		hashtable_node_t *cur, *next;
		for (cur = hashtable_first_node(&m_table); cur; cur = next) {
			next = hashtable_next_node(cur);
			delete ((Xnode*)cur);
		}
		hashtable_init(&m_table, buckets, sizeof(m_buckets) / sizeof(m_buckets[0]), keycmp, keyhash);
		m_size = 0;
	}

	size_t size(void) const { return m_size; };
	bool empty(void) const { return hashtable_first_node(&m_table) == NULL; }

	V& operator[](const K& k) {
		hashtable_node_t* n = hashtable_search_key(&m_table, &k);
		if (n) {
			return ((Xnode*)n)->v.second;
		}
		Xnode* node = new Xnode();
		hashtable_insert_node(&m_table, node);
		return node->v.second;
	}

private:
	hashtable_t m_table;
	hashtable_node_t* m_buckets[11];
	size_t m_size;
};
}
#endif

#endif