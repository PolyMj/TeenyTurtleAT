#ifndef _VEC_H_
#define _VEC_H_

#include <cmath>
#include <iostream>
#include <cstdint>
using namespace std; 


template<typename T> 
struct VecLength { using type = float; }; 

template<> 
struct VecLength<double> { using type = double; }; 


template<typename T = float>
struct vec2
{
    T x{}, y{};

    vec2() = default;
    vec2(T a): x(a), y(a) {};
    vec2(const T &x, const T &y): x(x), y(y) {}
    vec2(const vec2<T> &o): x(o.x), y(o.y) {}

    template<typename U>
    vec2(const vec2<U> &o): x(T{o.x}), y(T{o.y}) {}

    template<typename U>
    auto operator+(vec2<U> &o) const -> vec2<decltype(T{} + U{})>
    {
        return { (x + o.x), (y + o.y) };
    }

    template<typename U>
    auto operator-(vec2<U> o) const -> vec2<decltype(T{} - U{})>
    {
        return { (x - o.x), (y - o.y) };
    }

    // Dot product
    template<typename U>
    auto operator*(vec2<U> o) const -> decltype(T{} * U{})
    {
        return (x*o.x) + (y*o.y);
    }

    // Scalar multiplication
    template<typename U>
    auto operator*(U s) const -> vec2<decltype(T{} * U{})>
    {
        return { (x*s), (y*s) };
    }

    template<typename U>
    vec2<T>& operator=(vec2<U> &o)
    {
        x = o.x;
        y = o.y;
        return *this;
    }

    template<typename U>
    bool operator==(vec2<U> &o)
    {
        return (x == o.x) && (y == o.y);
    }

    template<typename U>
    vec2<T>& operator+=(vec2<U> &o)
    {
        x += o.x;
        y += o.y;
        return *this;
    }


    template<typename U>
    vec2<T>& operator-=(vec2<U> &o)
    {
        x -= o.x;
        y -= o.y;
        return *this;
    }

    // Scalar multiplication
    template<typename U>
    vec2<T>& operator*=(U s)
    {
        x *= s;
        y *= s;
        return *this;
    }

    // Euclidian length
    auto length() const -> typename VecLength<T>::type { 
        return sqrt(static_cast<typename VecLength<T>::type>(x*x + y*y)); 
    }; 

    // Returns a normalized version of the vector
    auto normalize() const -> vec2<typename VecLength<T>::type> { 
        auto len = length(); 
        return { (x / len), (y / len) }; 
    }; 

    void normalize_self()
    {
        auto len = length();
        x /= len;
        y /= len;
    }

    friend ostream& operator<<(ostream& os, vec2& o) { 
        os << "(" << o.x << "," << o.y << ")"; 
        return os; 
    }; 
};

using vec2f = vec2<float>;
using vec2d = vec2<double>;
using vec2i = vec2<int32_t>;

// Used for pixel coordinates
using vec2p = vec2<uint16_t>;


#endif
