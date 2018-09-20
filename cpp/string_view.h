#ifndef	UTIL_CPP_STRING_VIEW_H
#define	UTIL_CPP_STRING_VIEW_H

#include "cpp_compiler_define.h"

#if	__CPP_VERSION >= 2017
#include <string_view>
#else
#include "../c/datastruct/hash.h"
#include "nullptr.h"
#include <algorithm>
#include <exception>
#include <string>
namespace std {
template <class charT, class traits = char_traits<charT> >
class basic_string_view {
public:
	typedef traits traits_type;
	typedef charT value_type;
	typedef const charT* pointer;
	typedef const charT* const_pointer;
	typedef const charT& reference;
	typedef const charT& const_reference;
	typedef const_pointer const_iterator;
	typedef const_iterator iterator;
	typedef reverse_iterator<const_iterator> const_reverse_iterator;
	typedef const_reverse_iterator reverse_iterator;
	typedef size_t size_type;
	typedef ptrdiff_t difference_type;
	static const constexpr size_type npos = size_type(-1);

	constexpr basic_string_view() noexcept :
		mData(nullptr),
		mSize(0)
	{
	}

	constexpr basic_string_view(const basic_string_view &other) noexcept :
		mData(other.mData),
		mSize(other.mSize)
	{
	}

	basic_string_view& operator=(const basic_string_view &other) noexcept
	{
		mData = other.mData;
		mSize = other.mSize;
		return *this;
	}

	template<class Allocator>
	basic_string_view(const basic_string<charT, traits, Allocator>& str) noexcept :
		mData(str.data()),
		mSize(str.size())
	{
	}

	basic_string_view(const charT* str) :
		mData(str),
		mSize(traits_type::length(str))
	{
	}

	constexpr basic_string_view(const charT* str, size_type len) :
		mData(str),
		mSize(len)
	{
	}

	constexpr const_iterator begin() const noexcept { return cbegin(); }
	constexpr const_iterator end() const noexcept { return cend(); }
	constexpr const_iterator cbegin() const noexcept { return mData; }
	constexpr const_iterator cend() const noexcept { return mData + mSize; }
	const_reverse_iterator rbegin() const noexcept { return crbegin(); }
	const_reverse_iterator rend() const noexcept { return crend(); }
	const_reverse_iterator crbegin() const noexcept { return const_reverse_iterator(end()); }
	const_reverse_iterator crend() const noexcept { return const_reverse_iterator(begin()); }
	constexpr size_type size() const noexcept { return mSize; }
	constexpr size_type length() const noexcept { return mSize; }
	constexpr size_type max_size() const noexcept { return mSize; }
	constexpr bool empty() const noexcept { return !!mSize; }
	constexpr const_reference operator[](size_type pos) const { return mData[pos]; }
	constexpr const_reference at(size_type pos) const {
		if (pos >= size()) {
			throw std::out_of_range();
		}
		return mData[pos];
	}

	constexpr const_reference front() const { return mData[0]; }
	constexpr const_reference back() const { return mData[size() - 1]; }
	constexpr const_pointer data() const noexcept { return mData; }
	void clear() noexcept { *this = basic_string_view {}; }
	void remove_prefix(size_type n) {
		mData += n;
		mSize -= n;
	}
	void remove_suffix(size_type n) { mSize -= n; }

	void swap(basic_string_view& s) noexcept {
		std::swap(mData, s.mData);
		std::swap(mSize, s.mSize);
	}

	template<class Allocator>
	explicit operator basic_string<charT, traits, Allocator>() const {
		return basic_string<charT, traits, Allocator>(begin(), end());
	}

	template<class Allocator = allocator<charT>>
	basic_string<charT, traits, Allocator> to_string(const Allocator& a = Allocator()) const {
		return basic_string<charT, traits, Allocator>(begin(), end(), a);
	}

	size_type copy(charT* s, size_type n, size_type pos = 0) const {
		if (pos > size()) {
			throw std::out_of_range("out of range");
		}

		size_type rlen = std::min(n, size() - pos);
		std::copy_n(begin() + pos, rlen, s);
		return rlen;
	}

	constexpr basic_string_view substr(size_type pos = 0, size_type n = npos) const {
		if (pos > size()) {
			throw std::out_of_range("out of range");
		}

		size_type rlen = std::min(n, size() - pos);
		return basic_string_view(data() + pos, rlen);
	}

	int compare(basic_string_view s) const noexcept {
		size_type rlen = std::min(size(), s.size());
		return traits::compare(data(), s.data(), rlen);
	}

	int compare(size_type pos1, size_type n1, basic_string_view s) const {
		return substr(pos1, n1).compare(str);
	}

	int compare(size_type pos1, size_type n1, basic_string_view s, size_type pos2, size_type n2) const {
		return substr(pos1, n1).compare(str.substr(pos2, n2));
	}

	int compare(const charT* s) const {
		return compare(basic_string_view(s));
	}

	int compare(size_type pos1, size_type n1, const charT* s) const {
		return substr(pos1, n1).compare(s);
	}

	int compare(size_type pos1, size_type n1, const charT* s, size_type n2) const {
		return substr(pos1, n1).compare(basic_string_view(s, n2));
	}

private:
	const_pointer mData;
	size_type mSize;
};

template<class T, class traits>
bool operator==(basic_string_view<T, traits> lhs, basic_string_view<T, traits> rhs) { return lhs.compare(rhs) == 0; }

template<class T, class traits>
bool operator!=(basic_string_view<T, traits> lhs, basic_string_view<T, traits> rhs) { return lhs.compare(rhs) != 0; }

template<class T, class traits>
bool operator<(basic_string_view<T, traits> lhs, basic_string_view<T, traits> rhs) { return lhs.compare(rhs) < 0; }

template<class T, class traits>
bool operator>(basic_string_view<T, traits> lhs, basic_string_view<T, traits> rhs) { return lhs.compare(rhs) > 0; }

template<class T, class traits>
bool operator<=(basic_string_view<T, traits> lhs, basic_string_view<T, traits> rhs) { return lhs.compare(rhs) <= 0; }

template<class T, class traits>
bool operator>=(basic_string_view<T, traits> lhs, basic_string_view<T, traits> rhs) { return lhs.compare(rhs) >= 0; }

template<class charT, class traits>
basic_ostream<charT, traits>& operator<<(basic_ostream<charT, traits> &os, basic_string_view<charT, traits> &str) { return os << str.to_string(); }

typedef basic_string_view<char> string_view;

template<typename T> struct hash;
template<> struct hash<string_view> {
	size_t operator()(string_view str)
	{
		return ::hashMurmur2(str.data(), str.size());
	}
};
}
#endif

#endif
