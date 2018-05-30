//
// Created by hujianzhe on 16-5-2.
//

#ifndef UTIL_CPP_TYPE_CAST_H
#define	UTIL_CPP_TYPE_CAST_H

#include <exception>
#include <list>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace Util {
	template<typename T, typename F>
	T type_cast(const F& f) {
		std::stringstream ss;
		ss << f;
		if (!ss) {
			throw std::logic_error("type_cast invalid source type\n");
		}
		T t;
		ss >> t;
		if (!ss) {
			throw std::logic_error("type_cast invalid target type\n");
		}
		return t;
	}

	template<typename T, typename F>
	T type_cast_nothrow(const F& f) {
		std::stringstream ss;
		ss << f;
		if (!ss) {
			return T();
		}
		T t;
		ss >> t;
		if (!ss) {
			return T();
		}
		return t;
	}

	template <typename F>
	void type_cast(const std::vector<F>& f, std::list<F>& t) {
		t.clear();
		for (size_t i = 0; i < f.size(); ++i) {
			t.push_back(f[i]);
		}
	}

	template <typename F>
	void type_cast(const std::list<F>& f, std::vector<F>& t) {
		std::vector<F>().swap(t);
		t.reserve(f.size());
		for (std::list<F>::iterator it = f.begin(); it != f.end(); ++it) {
			t.push_back(*it);
		}
	}
}

#endif // !UTIL_TYPE_CAST_H
