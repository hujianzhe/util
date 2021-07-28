//
// Created by hujianzhe on 18-7-24
//

#ifndef UTIL_CPP_UNORDERED_MAP_H
#define	UTIL_CPP_UNORDERED_MAP_H

#ifdef __cplusplus

#include "cpp_compiler_define.h"
#if __CPP_VERSION >= 2011
#include <unordered_map>
#else
#include "../datastruct/hashtable.h"
#include <cstddef>
#include <utility>
#include <string>
namespace std {
template <typename K, typename V>
class unordered_map {
public:
	typedef K			key_type;
	typedef pair<K, V>	value_type;
	typedef struct Xnode : public ::HashtableNode_t {
		value_type v;

		Xnode(void) {
			key.ptr = &v.first;
		}
	} Xnode;

	class iterator {
	friend class unordered_map;
	public:
		iterator(::HashtableNode_t* p = NULL) : x(p) {}
		iterator(const iterator& i) : x(i.x) {}
		iterator& operator=(const iterator& i) {
			x = i.x;
			return *this;
		}

		bool operator==(const iterator& i) const {
			return i.x == x;
		}

		bool operator!=(const iterator& i) const {
			return !operator==(i);
		}

		value_type* operator->(void) const {
			return &((Xnode*)x)->v;
		}
		value_type& operator*(void) const {
			return ((Xnode*)x)->v;
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
	unordered_map(void) :
		m_size(0)
	{
		::hashtableInit(&m_table, m_buckets, sizeof(m_buckets) / sizeof(m_buckets[0]), keycmp, keyhash);
	}

	unordered_map(const unordered_map<K, V>& m) :
		m_size(0)
	{
		::hashtableInit(&m_table, m_buckets, sizeof(m_buckets) / sizeof(m_buckets[0]), keycmp, keyhash);
		for (iterator iter = m.begin(); iter != m.end(); ++iter) {
			Xnode* xnode = new Xnode();
			xnode->v = *iter;
			::hashtableInsertNode(&m_table, xnode);
			++m_size;
		}
	}

	unordered_map<K, V>& operator=(const unordered_map<K, V>& m) {
		if (this == &m) {
			return *this;
		}
		clear();
		for (iterator iter = m.begin(); iter != m.end(); ++iter) {
			Xnode* xnode = new Xnode();
			xnode->v = *iter;
			::hashtableInsertNode(&m_table, xnode);
			++m_size;
		}
		return *this;
	}

	~unordered_map(void) { clear(); }

	void clear(void) {
		::HashtableNode_t *cur, *next;
		for (cur = ::hashtableFirstNode(&m_table); cur; cur = next) {
			next = ::hashtableNextNode(cur);
			delete ((Xnode*)cur);
		}
		::hashtableInit(&m_table, m_buckets, sizeof(m_buckets) / sizeof(m_buckets[0]), keycmp, keyhash);
		m_size = 0;
	}

	size_t size(void) const { return m_size; };
	bool empty(void) const { return ::hashtableFirstNode(&m_table) == NULL; }

	V& operator[](const key_type& k) {
		::HashtableNodeKey_t hkey;
		hkey.ptr = &k;
		::HashtableNode_t* n = ::hashtableSearchKey(&m_table, hkey);
		if (n) {
			return ((Xnode*)n)->v.second;
		}
		Xnode* xnode = new Xnode();
		xnode->v.first = k;
		::hashtableInsertNode(&m_table, xnode);
		++m_size;
		return xnode->v.second;
	}
	
	void erase(iterator iter) {
		--m_size;
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
			--m_size;
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

	pair<iterator, bool> insert(const value_type& vt) {
		::HashtableNodeKey_t hkey;
		hkey.ptr = &vt.first;
		::HashtableNode_t* n = ::hashtableSearchKey(&m_table, hkey);
		if (n) {
			return pair<iterator, bool>(iterator(n), false);
		}
		Xnode* xnode = new Xnode();
		xnode->v = vt;
		::hashtableInsertNode(&m_table, xnode);
		++m_size;
		return pair<iterator, bool>(iterator(xnode), true);
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
	size_t m_size;
};
}
#endif

#endif

#endif
