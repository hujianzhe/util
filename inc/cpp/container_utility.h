//
// Created by hujianzhe on 20-2-22.
//

#ifndef UTIL_CPP_CONTAINER_UTILITY_H
#define	UTIL_CPP_CONTAINER_UTILITY_H

#ifdef __cplusplus

#include "unordered_set.h"
#include "unordered_map.h"
#include <vector>
#include <list>
#include <set>
#include <map>
#include <utility>

namespace util {
/* unify insert */
template <typename T, typename E>
std::vector<T>& container_insert(std::vector<T>& v, const E& e) {
	v.push_back(e);
	return v;
}
template <typename T, typename E>
std::list<T>& container_insert(std::list<T>& l, const E& e) {
	l.push_back(e);
	return l;
}
template <typename T, typename E>
std::set<T>& container_insert(std::set<T>& s, const E& e) {
	s.insert(e);
	return s;
}
template <typename T, typename E>
std::unordered_set<T>& container_insert(std::unordered_set<T>& s, const E& e) {
	s.insert(e);
	return s;
}
template <typename K, typename V, typename T, typename E>
std::map<K, V>& container_insert(std::map<K, V>& m, const std::pair<T, E>& e) {
	m.insert(e);
	return m;
}
template <typename K, typename V, typename T, typename E>
std::map<K, V>& container_insert(std::map<K, V>& m, const T& k, const E& v) {
	m.insert(std::make_pair(k, v));
	return m;
}
template <typename K, typename V, typename T, typename E>
std::unordered_map<K, V>& container_insert(std::unordered_map<K, V>& m, const std::pair<T, E>& e) {
	m.insert(e);
	return m;
}
template <typename K, typename V, typename T, typename E>
std::unordered_map<K, V>& container_insert(std::unordered_map<K, V>& m, const T& k, const E& v) {
	m.insert(std::make_pair(k, v));
	return m;
}
/* container shift */
template <typename ContainerFrom, typename ContainerTo>
ContainerTo& container_shift(const ContainerFrom& from, ContainerTo& to) {
	for (typename ContainerFrom::const_iterator it = from.begin(); it != from.end(); ++it)
		container_insert(to, *it);
	return to;
}

}

#endif

#endif // !UTIL_CPP_CONTAINER_UTILITY_H
