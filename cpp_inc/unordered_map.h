//
// Created by hujianzhe on 18-7-24
//

#ifndef UTIL_CPP_UNORDERED_MAP_H
#define	UTIL_CPP_UNORDERED_MAP_H

#include "cpp_compiler_define.h"
#if __CPP_VERSION >= 2011
#include <unordered_map>
#else
#include "hash.h"
#include <cstddef>
#include <functional>
#include <list>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace std11 {
template <	typename K,
		 	typename V,
			typename Hash = hash<K>,
			typename Eq = std::equal_to<K>,
			typename Alloc = std::allocator<std::pair<const K, V> > >
class unordered_map {
public:
	typedef K										key_type;
	typedef std::pair<const K, V>					value_type;
	typedef typename std::list<value_type>::iterator 		iterator;
	typedef typename std::list<value_type>::const_iterator 	const_iterator;

	unordered_map() : m_count(0), m_maxLoadFactor(1.0f), m_buckets(11) {
		for (size_t i = 0; i < m_buckets.size(); ++i) {
			m_buckets[i].p_container = &m_elements;
		}
	}

	float load_factor() const { return m_count / (float)m_buckets.size(); }
	float max_load_factor() const { return m_maxLoadFactor; }
	void max_load_factor(float v) { m_maxLoadFactor = v; }

	size_t bucket(const K& k) const {
		Hash h;
		return h(k) % m_buckets.size();
	}
	size_t bucket_count() const { return m_buckets.size(); }
	size_t bucket_size(size_t idx) const {
		if (idx >= m_buckets.size()) {
			return 0;
		}
		return m_buckets[idx].size();
	}
	size_t max_bucket_count() const { return 1000000; }

	iterator begin() { return m_elements.begin(); }
	iterator end() { return m_elements.end(); }
	const_iterator begin() const { return m_elements.begin(); }
	const_iterator end() const { return m_elements.end(); }
	const_iterator cbegin() const { return m_elements.begin(); }
	const_iterator cend() const { return m_elements.end(); }

	iterator find(const K& k) {
		size_t idx = bucket(k);
		return m_buckets[idx].find(k);
	}
	const_iterator find(const K& k) const {
		size_t idx = bucket(k);
		return m_buckets[idx].find(k);
	}

	void erase(iterator iter) {
		size_t idx = bucket(iter->first);
		m_buckets[idx].erase(iter);
		m_elements.erase(iter);
		--m_count;
	}
	size_t erase(const K& k) {
		iterator iter = find(k);
		if (iter == end()) {
			return 0;
		}
		erase(iter);
		return 1;
	}

	std::pair<iterator, bool> insert(const value_type& vt) {
		iterator iter = find(vt.first);
		if (iter == end()) {
			iter = insert_impl(vt);
			return std::pair<iterator, bool>(iter, true);
		}
		return std::pair<iterator, bool>(iter, false);
	}

	V& operator[](const K& k) {
		iterator iter = find(k);
		if (iter != end()) {
			return iter->second;
		}
		iter = insert_impl(value_type(k, V()));
		return iter->second;
	}
	V& at(const K& k) {
		iterator it = find(k);
		if (it == end()) {
			throw std::out_of_range("unordered_map out of range");
		}
		return it->second;
	}
	const V& at(const K& k) const {
		const_iterator it = find(k);
		if (it == end()) {
			throw std::out_of_range("unordered_map out of range");
		}
		return it->second;
	}

	size_t size(void) const { return m_count; };
	bool empty(void) const { return 0 == m_count; }

	void clear() {
		m_count = 0;
		for (size_t i = 0; i < m_buckets.size(); ++i) {
			m_buckets[i].clear();
		}
		m_elements.clear();
	}

	void rehash(size_t n) {
		if (n <= m_buckets.size()) {
			return;
		}
		std::vector<Bucket> bks(n);
		Hash h;
		for (iterator it = m_elements.begin(); it != m_elements.end(); ++it) {
			bks[h(it->first) % n].insert(it);
		}
		for (size_t i = 0; i < bks.size(); ++i) {
			bks[i].p_container = &m_elements;
		}
		m_buckets.swap(bks);
	}

protected:
	struct Bucket {
		std::list<value_type>* p_container;
		std::vector<iterator> nodes;

		Bucket() : p_container(NULL) {}

		const_iterator find(const K& k) const {
			Eq eq;
			for (size_t i = 0; i < nodes.size(); ++i) {
				if (eq(nodes[i]->first, k)) {
					return nodes[i];
				}
			}
			return p_container->end();
		}
		iterator find(const K& k) {
			Eq eq;
			for (size_t i = 0; i < nodes.size(); ++i) {
				if (eq(nodes[i]->first, k)) {
					return nodes[i];
				}
			}
			return p_container->end();
		}

		void insert(iterator iter) {
			nodes.push_back(iter);
		}

		void erase(iterator iter) {
			for (size_t i = 0; i < nodes.size(); ++i) {
				if (nodes[i] == iter) {
					nodes[i] = nodes.back();
					nodes.pop_back();
					return;
				}
			}
		}

		void clear() { nodes.clear(); }
		size_t size() const { return nodes.size(); }
	};

	iterator insert_impl(const value_type& vt) {
		++m_count;
		m_elements.push_front(vt);
		iterator iter = m_elements.begin();
		size_t idx = bucket(vt.first);
		m_buckets[idx].insert(iter);
		return iter;
	}

private:
	size_t m_count;
	float m_maxLoadFactor;
	std::vector<Bucket> m_buckets;
	std::list<value_type> m_elements;
};
}
#endif

#endif
