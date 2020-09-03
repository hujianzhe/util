//
// Created by hujianzhe on 20-9-3
//

#ifndef	UTIL_CPP_TYPE_IS_CLASS_H
#define	UTIL_CPP_TYPE_IS_CLASS_H

#ifdef	__cplusplus
#include "cpp_compiler_define.h"
namespace util {
template <typename T>
struct IsClass {
private:
	typedef char Yes;
	typedef int No;
	template <typename C> static Yes test(int C::*);
	template <typename C> static No  test(...);

public:
	enum { YES = (sizeof(test<T>(0)) == sizeof(Yes)) };
	enum { NO  = !YES };
};

template <typename T>
bool is_class_type(void) { return IsClass<T>::YES; }

template <typename T, bool isClass = IsClass<T>::YES> struct IsInherit;
template <typename T>
struct IsInherit <T, true> {
private:
	typedef char Yes;
	typedef int No;
	template <typename C> static Yes test(const T*);
	template <typename C> static No  test(...);

public:
	template <typename C>
	struct Check {
	private:
		static C* getType();
	public:
		enum { YES = (sizeof(test<T>(getType())) == sizeof(Yes)) };
		enum { NO  = !YES };
	};
};
template <typename T>
struct IsInherit <T, false> {
public:
	template <typename C>
	struct Check {
	public:
		enum { YES = false };
		enum { NO  = !YES };
	};
};
template <class Sub, class Parent>
bool is_inhert_type(void) { return typename IsInherit<Parent>::template Check<Sub>().YES; }
}
#endif

#endif
