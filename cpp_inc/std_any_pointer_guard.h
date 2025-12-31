//
// Created by hujianzhe on 24-9-7.
//

#ifndef UTIL_CPP_STD_ANY_POINTER_GUARD_H
#define	UTIL_CPP_STD_ANY_POINTER_GUARD_H

#include <any>
#include <memory>

namespace util {
template <typename T, typename D = std::default_delete<T> >
class StdUniquePtrCopyMove {
public:
	StdUniquePtrCopyMove(::std::unique_ptr<T, D> u) noexcept
		: p(nullptr), d(D{})
	{
		std::swap(u.get_deleter(), d);
		p = u.release();
	}
	StdUniquePtrCopyMove(const StdUniquePtrCopyMove& o) noexcept
		: p(nullptr), d(D{})
	{
		auto& mut_o = const_cast<StdUniquePtrCopyMove&>(o);
		std::swap(mut_o.d, d);
		p = mut_o.p;
		mut_o.p = nullptr;
	}
	~StdUniquePtrCopyMove() noexcept {
		if (p) d(p);
	}

	::std::unique_ptr<T, D> to_unique_ptr() noexcept {
		std::unique_ptr<T, D> u{p, d};
		p = nullptr;
		return u;
	}

private:
	StdUniquePtrCopyMove& operator=(const StdUniquePtrCopyMove&) = delete;

private:
	T* p;
	D d;
};

class StdAnyPointerGuard {
private:
	template <typename T>
	struct Impl {
		Impl(T* v, void(*dt)(T*)) : m_v(v), m_dt(dt) {}
		Impl(const Impl& other) : m_v(other.m_v), m_dt(other.m_dt)
		{
			(const_cast<Impl&>(other)).m_v = nullptr;
		}
		~Impl() {
			if (m_v) {
				m_dt(m_v);
			}
		}
		Impl& operator=(const Impl&) = delete;

		T* m_v;
		void(*m_dt)(T*);
	};

public:
	template <typename T>
	static std::unique_ptr<T, void(*)(T*)> transfer_unique_ptr(const std::any& a) {
		auto& g = const_cast<Impl<T>&>(std::any_cast<const Impl<T>&>(a));
		std::unique_ptr<T, void(*)(T*)> p(g.m_v, g.m_dt);
		g.m_v = nullptr;
		(const_cast<std::any&>(a)).reset();
		return p;
	}

	template <typename T>
	static std::any to_any(T* v, void(*dt)(T*)) {
		return std::any(Impl<T>(v, dt));
	}
};
}

#endif
