//
// Created by hujianzhe on 20-2-22.
//

#ifndef UTIL_CPP_CONTAINER_CAST_H
#define	UTIL_CPP_CONTAINER_CAST_H

#ifdef __cplusplus

#include "cpp_compiler_define.h"
#include <vector>
#include <list>
#include <set>
#include <map>
#include <utility>

namespace util {
/* vector */
template <typename T, typename E>
std::list<E>& vector_shift_list(const std::vector<T>& v, std::list<E>& l) {
	for (typename std::vector<T>::const_iterator it = v.begin(); it != v.end(); ++it)
		l.push_back(*it);
	return l;
}

template <typename T, typename E>
std::set<E>& vector_shift_set(const std::vector<T>& v, std::set<E>& s) {
	for (typename std::vector<T>::const_iterator it = v.begin(); it != v.end(); ++it)
		s.insert(*it);
	return s;
}

template <typename K, typename V, typename T, typename E>
std::map<T, E>& vector_shift_map(const std::vector<std::pair<K, V> >& v, std::map<T, E>& m) {
	for (typename std::vector<std::pair<K, V> >::const_iterator it = v.begin(); it != v.end(); ++it)
		m.insert(*it);
	return m;
}
/* list */
template <typename T, typename E>
std::vector<E>& list_shift_vector(const std::list<T>& l, std::vector<E>& v) {
	for (typename std::list<T>::const_iterator it = l.begin(); it != l.end(); ++it)
		v.push_back(*it);
	return v;
}

template <typename T, typename E>
std::set<E>& list_shift_set(const std::list<T>& s, std::set<E>& s) {
	for (typename std::list<T>::const_iterator it = l.begin(); it != l.end(); ++it)
		s.insert(*it);
	return s;
}

template <typename K, typename V, typename T, typename E>
std::map<T, E>& list_shift_map(const std::list<std::pair<K, V> >& l, std::map<T, E>& m) {
	for (typename std::list<std::pair<K, V> >::const_iterator it = l.begin(); it != l.end(); ++it)
		m.insert(*it);
	return m;
}
/* set */
template <typename T, typename E>
std::vector<E>& set_shift_vector(const std::set<T>& s, std::vector<E>& v) {
	for (typename std::set<T>::const_iterator it = s.begin(); it != s.end(); ++it)
		v.push_back(*it);
	return v;
}

template <typename T, typename E>
std::list<E>& set_shift_list(const std::set<T>& s, std::list<E>& l) {
	for (typename std::set<T>::const_iterator it = s.begin(); it != s.end(); ++it)
		l.push_back(*it);
	return l;
}

template <typename K, typename V, typename T, typename E>
std::map<T, E>& set_shift_map(const std::set<std::pair<K, V> >& s, std::map<T, E>& m) {
	for (typename std::set<std::pair<K, V> >::const_iterator it = s.begin(); it != s.end(); ++it)
		m.insert(*it);
	return m;
}
/* map */
template <typename K, typename V, typename T, typename E>
std::vector<std::pair<T, E> >& map_shift_vector(const std::map<K, V>& m, std::vector<std::pair<T, E> >& v) {
	for (typename std::map<K, V>::const_iterator it = m.begin(); it != m.end(); ++it)
		v.push_back(*it);
	return v;
}

template <typename K, typename V, typename T, typename E>
std::list<std::pair<T, E> >& map_shift_list(const std::map<K, V>& m, std::list<std::pair<T, E> >& l) {
	for (typename std::map<K, V>::const_iterator it = m.begin(); it != m.end(); ++it)
		l.push_back(*it);
	return l;
}

template <typename K, typename V, typename T, typename E>
std::set<std::pair<T, E> >& map_shift_set(const std::map<K, V>& m, std::set<std::pair<T, E> >& s) {
	for (typename std::map<K, V>::const_iterator it = m.begin(); it != m.end(); ++it)
		s.insert(*it);
	return s;
}
}

#endif

#endif // !UTIL_CPP_CONTAINER_CAST_H
