//
// Created by hujianzhe on 16-5-2.
//

#ifndef UTIL_TYPE_CAST_H
#define	UTIL_TYPE_CAST_H

#include <exception>
#include <sstream>
#include <stdexcept>

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
}

#endif // !UTIL_TYPE_CAST_H
