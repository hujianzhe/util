//
// Created by hujianzhe on 24-6-13.
//

#ifndef	UTIL_CPP_FRACTION_H
#define	UTIL_CPP_FRACTION_H

#include <exception>
#include <numeric>
#include <stdexcept>
#include <string>

namespace util {
template <typename T>
class Fraction {
    template <typename U>
    friend class Fraction;

public:
    typedef T type_t;

    template <typename T, typename U>
    friend bool operator<(T ls, const Fraction<U>& rs);
    template <typename T, typename U>
    friend bool operator>(T ls, const Fraction<U>& rs);
    template <typename T, typename U>
    friend bool operator==(T ls, const Fraction<U>& rs);
    template <typename U>
    friend bool operator==(bool ls, const Fraction<U>& rs);
    template <typename T, typename U>
    friend bool operator!=(T ls, const Fraction<U>& rs);
    template <typename U>
    friend bool operator!=(bool ls, const Fraction<U>& rs);
    template <typename T, typename U>
    friend bool operator<=(T ls, const Fraction<U>& rs);
    template <typename T, typename U>
    friend bool operator>=(T ls, const Fraction<U>& rs);

private:
    T m_numerator;
    T m_denominator;

public:
    void reduce() {
        T v = std::gcd(m_numerator, m_denominator);
        m_numerator /= v;
        m_denominator /= v;
    }

    Fraction(T numerator = 0, T denominator = 1) {
        if (denominator < 0) {
            if (0 == numerator) {
                m_numerator = 0;
                m_denominator = 1;
                return;
            }
            else {
                m_numerator = -numerator;
                m_denominator = -denominator;
            }
        }
        else {
            if (0 == numerator) {
                m_numerator = 0;
                m_denominator = 1;
                return;
            }
            else {
                m_numerator = numerator;
                m_denominator = denominator;
            }
        }
        if (0 == m_denominator) {
            throw std::logic_error("denominator == 0");
        }
        reduce();
    }

    template <typename U>
    Fraction(const Fraction<U>& fraction) :
        m_numerator(fraction.m_numerator), m_denominator(fraction.m_denominator)
    {
        if (0 == m_denominator) {
            throw std::logic_error("denominator == 0");
        }
    }

    template <typename U>
    Fraction& operator=(const Fraction<U>& fraction) {
        if (0 == ((T)fraction.m_denominator)) {
            throw std::logic_error("denominator == 0");
        }
        m_numerator = fraction.m_numerator;
        m_denominator = fraction.m_denominator;
        return *this;
    }

    T numerator() const { return m_numerator; }
    T denominator() const { return m_denominator; }

    template <typename NumberType>
    NumberType to_num() const { return (NumberType)m_numerator / (NumberType)m_denominator; }

    std::string to_str() const {
        if (1 == m_denominator) {
            return std::to_string(m_numerator);
        }
        if (0 == m_numerator) {
            return "0";
        }
        return std::to_string(m_numerator) + '/' + std::to_string(m_denominator);
    }

    Fraction inv() const {
        return Fraction(m_denominator, m_numerator);
    }

    operator bool() const { return m_numerator != 0; }

    template <typename U>
    Fraction& operator+=(const Fraction<U>& rs) {
        if (0 == rs.m_numerator) {
            return *this;
        }
        if (m_denominator == rs.m_denominator) {
            m_numerator += rs.m_numerator;
        }
        else {
            T denominator = m_denominator * rs.m_denominator;
            if (0 == denominator) {
                throw std::logic_error("denominator == 0");
            }
            m_numerator = m_numerator * rs.m_denominator + rs.m_numerator * m_denominator;
            m_denominator = denominator;
        }
        reduce();
        return *this;
    }
    Fraction operator+=(const Fraction& rs) {
        return operator+=<T>(rs);
    }

    template <typename U>
    Fraction operator-=(const Fraction<U>& rs) {
        if (0 == rs.m_numerator) {
            return *this;
        }
        if (m_denominator == rs.m_denominator) {
            m_numerator -= rs.m_numerator;
        }
        else {
            T denominator = m_denominator * rs.m_denominator;
            if (0 == denominator) {
                throw std::logic_error("denominator == 0");
            }
            m_numerator = m_numerator * rs.m_denominator - rs.m_numerator * m_denominator;
            m_denominator = denominator;
        }
        reduce();
        return *this;
    }
    Fraction operator-=(const Fraction& rs) {
        return operator-=<T>(rs);
    }

    template <typename U>
    Fraction operator*=(const Fraction<U>& rs) {
        if (0 == rs.m_numerator) {
            m_numerator = 0;
            m_denominator = 1;
            return *this;
        }
        T v;
        v = std::gcd(m_numerator, rs.m_denominator);
        T lsn = m_numerator / v;
        T rsd = rs.m_denominator / v;
        v = std::gcd(rs.m_numerator, m_denominator);
        T lsd = m_denominator / v;
        T rsn = rs.m_numerator / v;
        T den = lsd * rsd;
        if (0 == den) {
            throw std::logic_error("denominator == 0");
        }
        m_numerator = lsn * rsn;
        m_denominator = den;
        reduce();
        return *this;
    }
    Fraction operator*=(const Fraction& rs) {
        return operator*=<T>(rs);
    }

    template <typename U>
    Fraction operator/=(const Fraction<U>& rs) {
        return operator*=<T>(rs.inv());
    }
    Fraction operator/=(const Fraction& rs) {
        return operator*=<T>(rs.inv());
    }

    template <typename U>
    bool operator<(const Fraction<U>& rs) const {
        if (m_denominator == rs.m_denominator) {
            return m_numerator < rs.m_numerator;
        }
        if (m_numerator == rs.m_numerator) {
            return m_denominator > rs.m_denominator;
        }
        return m_numerator * rs.m_denominator < rs.m_numerator * m_denominator;
    }
    template <typename U>
    bool operator<(U rs) const { return *this < Fraction<U>(rs); }

    template <typename U>
    bool operator>(const Fraction<U>& rs) const {
        if (m_denominator == rs.m_denominator) {
            return m_numerator > rs.m_numerator;
        }
        if (m_numerator == rs.m_numerator) {
            return m_denominator < rs.m_denominator;
        }
        return m_numerator * rs.m_denominator > rs.m_numerator * m_denominator;
    }
    template <typename U>
    bool operator>(U rs) const { return *this > Fraction<U>(rs); }

    bool operator==(bool v) const { return v == (m_numerator != 0); }
    template <typename U>
    bool operator==(const Fraction<U>& rs) const {
        return m_numerator == rs.m_numerator && m_denominator == rs.m_denominator;
    }
    template <typename U>
    bool operator==(U rs) const { return *this == Fraction<U>(rs); }

    bool operator!=(bool v) const { return v != (m_numerator != 0); }
    template <typename U>
    bool operator!=(const Fraction<U>& rs) const {
        return m_numerator != rs.m_numerator || m_denominator != rs.m_denominator;
    }
    template <typename U>
    bool operator!=(U rs) const { return *this != Fraction<U>(rs); }

    template <typename U>
    bool operator<=(const Fraction<U>& rs) const { return operator==(rs) || operator<(rs); }
    template <typename U>
    bool operator<=(U rs) const { return *this <= Fraction<U>(rs); }

    template <typename U>
    bool operator>=(const Fraction<U>& rs) const { return operator==(rs) || operator>(rs); }
    template <typename U>
    bool operator>=(U rs) const { return *this >= Fraction<U>(rs); }
};
template <typename T, typename U>
bool operator<(T ls, const Fraction<U>& rs) { return Fraction<T>(ls) < rs; }
template <typename T, typename U>
bool operator>(T ls, const Fraction<U>& rs) { return Fraction<T>(ls) > rs; }
template <typename T, typename U>
bool operator==(T ls, const Fraction<U>& rs) { return Fraction<T>(ls) == rs; }
template <typename U>
bool operator==(bool ls, const Fraction<U>& rs) { return rs == ls; }
template <typename T, typename U>
bool operator!=(T ls, const Fraction<U>& rs) { return Fraction<T>(ls) != rs; }
template <typename U>
bool operator!=(bool ls, const Fraction<U>& rs) { return rs != ls; }
template <typename T, typename U>
bool operator<=(T ls, const Fraction<U>& rs) { return Fraction<T>(ls) <= rs; }
template <typename T, typename U>
bool operator>=(T ls, const Fraction<U>& rs) { return Fraction<T>(ls) >= rs; }
}

#endif