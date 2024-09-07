//
// Created by hujianzhe on 24-9-7.
//

#ifndef UTIL_CPP_STD_ANY_POINTER_GUARD_H
#define	UTIL_CPP_STD_ANY_POINTER_GUARD_H

#include <any>

namespace util {
template <typename T, typename D = void(*)(T*)>
class StdAnyPointerGuard {
public:
	StdAnyPointerGuard(const StdAnyPointerGuard& other) : m_v(other.m_v), m_dt(other.m_dt)
	{
		(const_cast<StdAnyPointerGuard&>(other)).m_v = nullptr;
	}
	~StdAnyPointerGuard() {
		if (m_v) {
			m_dt(m_v);
		}
	}

	static std::unique_ptr<T,D> transfer_unique_ptr(const std::any& a) {
		auto& g = const_cast<StdAnyPointerGuard&>(std::any_cast<const StdAnyPointerGuard&>(a));
		std::unique_ptr<T,D> uptr(nullptr, g.m_dt);
		uptr.reset(g.release());
		return uptr;
	}
	static std::any to_any(T* v, const D& dt = D()) {
		return std::any(StdAnyPointerGuard(v, dt));
	}

	T* release() {
		T* v = m_v;
		m_v = nullptr;
		return v;
	}

private:
	StdAnyPointerGuard(T* v, const D& dt) : m_v(v), m_dt(dt) {}

private:
	T* m_v;
	D m_dt;
};
}

#endif
