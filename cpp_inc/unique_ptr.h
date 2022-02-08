//
// Created by hujianzhe on 18-8-25.
//

#ifndef UTIL_CPP_UNIQUE_PTR_H
#define	UTIL_CPP_UNIQUE_PTR_H

#include "cpp_compiler_define.h"
#if __CPP_VERSION >= 2011
#include <memory>
#else
#include "hash.h"
#include "nullptr.h"

namespace std11 {
template<typename T>
struct default_delete {
	void operator()(T* ptr) const { delete ptr; }
};
template<typename T>
struct default_delete<T[]> {
	void operator()(T *ptr) const { delete[] ptr; }
};

template <typename T, typename Deleter = default_delete<T> >
class unique_ptr {
public:
	explicit unique_ptr(T* p = (T*)0, const Deleter& deleter = Deleter()) :
		m_ptr(p),
		m_deleter(deleter),
		m_do_deleter(p != (T*)0)
	{
	}
	~unique_ptr(void) {
		if (m_do_deleter)
			m_deleter(m_ptr);
	}
	// compromise
	unique_ptr(const unique_ptr& p) :
		m_ptr(p.m_ptr),
		m_deleter(p.m_deleter),
		m_do_deleter(p.m_ptr != (T*)0)
	{
		p.m_ptr = (T*)0;
		p.m_do_deleter = false;
	}
	// compromise
	unique_ptr& operator=(const unique_ptr& rhs) {
		if (this != &rhs) {
			this->m_ptr = rhs.m_ptr;
			this->m_deleter = rhs.m_deleter;
			this->m_do_deleter = (rhs.m_ptr != (T*)0);
			rhs.m_ptr = (T*)0;
			rhs.m_do_deleter = false;
		}
		return *this;
	}

	operator bool(void) const { return m_ptr != (T*)0; }
	const T& operator*(void) const { return *m_ptr; }
	T& operator*(void) { return *m_ptr; }
	T* operator->(void) { return m_ptr; }
	const T* operator->(void) const { return m_ptr; }

	T* release(void) {
		T* p = m_ptr;
		m_ptr = (T*)0;
		m_do_deleter = false;
		return p;
	}
	void reset(T* p = (T*)0) {
		if (m_do_deleter)
			m_deleter(m_ptr);
		m_ptr = p;
		m_do_deleter = (p != (T*)0);
	}
	void swap(unique_ptr& other) {
		T* temp_p = other.m_ptr;
		Deleter temp_del = other.m_deleter;
		bool temp_do_del = other.m_do_deleter;
		other.m_ptr = m_ptr;
		other.m_deleter = m_deleter;
		other.m_do_deleter = m_do_deleter;
		m_ptr = temp_p;
		m_deleter = temp_del;
		m_do_deleter = temp_do_del;
	}
	T* get(void) const { return m_ptr; }
	Deleter& get_deleter(void) { return m_deleter; }
	const Deleter& get_deleter(void) const { return m_deleter; }

	// dont't call this function...(for construct shared_ptr)
	T* __release_for_shared_ptr(void) const {
		T* p = m_ptr;
		m_ptr = (T*)0;
		m_do_deleter = false;
		return p;
	}

private:
	mutable T* m_ptr;
	Deleter m_deleter;
	mutable bool m_do_deleter;
};
template <typename T, typename Deleter>
class unique_ptr<T[], Deleter> {
public:
	explicit unique_ptr(T* p = (T*)0, const Deleter& deleter = Deleter()) :
		m_ptr(p),
		m_deleter(deleter),
		m_do_deleter(p != (T*)0)
	{
	}
	~unique_ptr(void) {
		if (m_do_deleter)
			m_deleter(m_ptr);
	}
	// compromise
	unique_ptr(const unique_ptr& p) :
		m_ptr(p.m_ptr),
		m_deleter(p.m_deleter),
		m_do_deleter(p.m_ptr != (T*)0)
	{
		p.m_ptr = (T*)0;
		p.m_do_deleter = false;
	}
	// compromise
	unique_ptr& operator=(const unique_ptr& rhs) {
		if (this != &rhs) {
			this->m_ptr = rhs.m_ptr;
			this->m_deleter = rhs.m_deleter;
			this->m_do_deleter = (rhs.m_ptr != (T*)0);
			rhs.m_ptr = (T*)0;
			rhs.m_do_deleter = false;
		}
		return *this;
	}

	operator bool(void) const { return m_ptr != (T*)0; }

	const T& operator[](size_t i) const { return m_ptr[i]; }
	T& operator[](size_t i) { return m_ptr[i]; }

	T* release(void) {
		T* p = m_ptr;
		m_ptr = (T*)0;
		m_do_deleter = false;
		return p;
	}
	void reset(T* p = (T*)0) {
		if (m_do_deleter)
			m_deleter(m_ptr);
		m_ptr = p;
		m_do_deleter = (p != (T*)0);
	}
	void swap(unique_ptr& other) {
		T* temp_p = other.m_ptr;
		Deleter temp_del = other.m_deleter;
		bool temp_do_del = other.m_do_deleter;
		other.m_ptr = m_ptr;
		other.m_deleter = m_deleter;
		other.m_do_deleter = m_do_deleter;
		m_ptr = temp_p;
		m_deleter = temp_del;
		m_do_deleter = temp_do_del;
	}
	T* get(void) const { return m_ptr; }
	Deleter& get_deleter(void) { return m_deleter; }
	const Deleter& get_deleter(void) const { return m_deleter; }

private:
	mutable T* m_ptr;
	Deleter m_deleter;
	mutable bool m_do_deleter;
};
template <class T1, class D1, class T2, class D2>
bool operator==(const unique_ptr<T1, D1>& x, const unique_ptr<T2, D2>& y) { return x.get() == y.get(); }
template <class T1, class D, class T2>
bool operator==(const unique_ptr<T1, D>& x, const T2* y) { return x.get() == y; }
template <class T1, class D, class T2>
bool operator==(const T2* y, const unique_ptr<T1, D>& x) { return x.get() == y; }
template <class T, class D>
bool operator==(nullptr_t p, const unique_ptr<T, D>& x) { return x.get() == p; }
template <class T, class D>
bool operator==(const unique_ptr<T, D>& x, nullptr_t p) { return x.get() == p; }

template <class T1, class D1, class T2, class D2>
bool operator!=(const unique_ptr<T1, D1>& x, const unique_ptr<T2, D2>& y) { return x.get() != y.get(); }
template <class T1, class D, class T2>
bool operator!=(const unique_ptr<T1, D>& x, const T2* y) { return x.get() != y; }
template <class T1, class D, class T2>
bool operator!=(const T2* y, const unique_ptr<T1, D>& x) { return x.get() != y; }
template <class T, class D>
bool operator!=(nullptr_t p, const unique_ptr<T, D>& x) { return x.get() != p; }
template <class T, class D>
bool operator!=(const unique_ptr<T, D>& x, nullptr_t p) { return x.get() != p; }

template <class T, class D>
unique_ptr<T, D> move(unique_ptr<T, D>& p) { return unique_ptr<T, D>(p.release()); }

template <class T, class D>
void swap(unique_ptr<T, D>& x, unique_ptr<T, D>& y) { x.swap(y); }

template <class T, class D>
struct hash<unique_ptr<T, D> > {
	size_t operator()(const unique_ptr<T, D>& p) const {
		return (size_t)p.get();
	}
};
}
#endif

#endif
