//
// Created by hujianzhe on 18-7-24
//

#ifndef	UTIL_CPP_UNORDERED_SET_H
#define	UTIL_CPP_UNORDERED_SET_H

#include "cpp_compiler_define.h"
#if	__CPP_VERSION >= 2011
#include <unordered_set>
#else
#include "../inc/datastruct/hashtable.h"
#include <cstddef>
#include <string>
#include <utility>

namespace std11 {
template <typename K>
class unordered_set {
public:
	typedef	K			key_type;	
	typedef	K			value_type;
	typedef struct Xnode : public ::HashtableNode_t {
		key_type k;

		Xnode(void) {
			key.ptr = &k;
		}
	} Xnode;

	class iterator {
	friend class unordered_set;
	public:
		iterator(::HashtableNode_t* p = NULL) : x(p) {}
		iterator(const iterator& i) : x(i.x) {}
		iterator& operator=(const iterator& i) {
			x = i.x;
			return *this;
		}

		bool operator==(const iterator& i) {
			return i.x == x;
		}

		bool operator!=(const iterator& i) {
			return !operator==(i);
		}

		key_type& operator*(void) {
			return ((Xnode*)x)->k;
		}

		iterator& operator++(void) {
			x = ::hashtableNextNode(x);
			return *this;
		}
		iterator operator++(int unused) {
			iterator it = *this;
			x = ::hashtableNextNode(x);
			return it;
		}

	private:
		::HashtableNode_t* x;	
	};
	typedef iterator	const_iterator;

private:
	static int keycmp(const HashtableNodeKey_t* node_key, const HashtableNodeKey_t* key) {
		return *(key_type*)(node_key->ptr) != *(key_type*)(key->ptr);
	}

	static unsigned int __key_hash(const std::string& s) {
		const char* str = s.empty() ? "" : s.c_str();
		/* BKDR hash */
		unsigned int seed = 131;
		unsigned int hash = 0;
		while (*str) {
			hash = hash * seed + (*str++);
		}
		return hash & 0x7fffffff;
	}
	static unsigned int __key_hash(size_t n) { return n; }
	static unsigned int __key_hash(const void* p) { return (unsigned int)(size_t)p; }

	static unsigned int keyhash(const HashtableNodeKey_t* key) {
		return __key_hash(*(key_type*)(key->ptr));
	}

public:
	unordered_set(void) {
		::hashtableInit(&m_table, m_buckets, sizeof(m_buckets) / sizeof(m_buckets[0]), keycmp, keyhash);
	}

	unordered_set(const unordered_set<K>& m) {
		::hashtableInit(&m_table, m_buckets, sizeof(m_buckets) / sizeof(m_buckets[0]), keycmp, keyhash);
		for (iterator iter = m.begin(); iter != m.end(); ++iter) {
			Xnode* xnode = new Xnode();
			xnode->k = *iter;
			::hashtableInsertNode(&m_table, xnode);
		}
	}

	unordered_set<K>& operator=(const unordered_set<K>& m) {
		if (this == &m) {
			return *this;
		}
		clear();
		for (iterator iter = m.begin(); iter != m.end(); ++iter) {
			Xnode* xnode = new Xnode();
			xnode->k = *iter;
			::hashtableInsertNode(&m_table, xnode);
		}
		return *this;
	}

	~unordered_set(void) { clear(); }

	void clear(void) {
		::HashtableNode_t *cur, *next;
		for (cur = ::hashtableFirstNode(&m_table); cur; cur = next) {
			next = ::hashtableNextNode(cur);
			delete ((Xnode*)cur);
		}
		::hashtableInit(&m_table, m_buckets, sizeof(m_buckets) / sizeof(m_buckets[0]), keycmp, keyhash);
	}

	size_t size(void) const { return m_table.count; };
	bool empty(void) const { return ::hashtableIsEmpty(&m_table); }

	void erase(iterator iter) {
		::hashtableRemoveNode(&m_table, iter.x);
		delete (Xnode*)(iter.x);
	}

	size_t erase(const key_type& k) {
		::HashtableNodeKey_t hkey;
		hkey.ptr = &k;
		::HashtableNode_t* node = ::hashtableSearchKey(&m_table, hkey);
		if (node) {
			::hashtableRemoveNode(&m_table, node);
			delete (Xnode*)node;
			return 1;
		}
		return 0;
	}

	iterator find(const key_type& k) const {
		::HashtableNodeKey_t hkey;
		hkey.ptr = &k;
		::HashtableNode_t* node = ::hashtableSearchKey(&m_table, hkey);
		return iterator(node);
	}

	std::pair<iterator, bool> insert(const key_type& k) {
		::HashtableNodeKey_t hkey;
		hkey.ptr = &k;
		::HashtableNode_t* n = ::hashtableSearchKey(&m_table, hkey);
		if (n) {
			return std::pair<iterator, bool>(iterator(n), false);
		}
		Xnode* xnode = new Xnode();
		xnode->k = k;
		::hashtableInsertNode(&m_table, xnode);
		return std::pair<iterator, bool>(iterator(xnode), true);
	}

	iterator begin(void) const {
		return iterator(::hashtableFirstNode(&m_table));
	}
	iterator end(void) const {
		return iterator();
	}

private:
	::Hashtable_t m_table;
	::HashtableNode_t* m_buckets[11];
};
}
#endif

#endif
