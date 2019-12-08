/*
 * Copyright (c) 2019 John Drouhard
 * Based on ideas copyright (c) 2017 - 2018 Andrzej Krzemienski
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <string_view>
#include <utility>

namespace jd {

template <std::size_t N, typename T>
class CompileTimeString
{
    static_assert(N != N, "invalid impl tag");
};

namespace detail {

struct literal_tag;
struct char_array_tag;

} // namespace detail

template <std::size_t N>
using StringLiteral = CompileTimeString<N, detail::literal_tag>;

template <std::size_t N>
using StringArray = CompileTimeString<N, detail::char_array_tag>;

namespace detail {

template <std::size_t N1, std::size_t N2, typename TL, typename TR, typename...Is>
constexpr bool stringEqual(CompileTimeString<N1, TL> const & l, CompileTimeString<N2, TR> const & r, Is...) {
    return false;
}

template <std::size_t N, typename TL, typename TR>
constexpr bool stringEqual(CompileTimeString<N, TL> const & l, CompileTimeString<N, TR> const & r) {
    return true;
}

template <std::size_t N, typename TL, typename TR, typename...Is>
constexpr bool stringEqual(CompileTimeString<N, TL> const & l, CompileTimeString<N, TR> const & r, std::size_t i, Is... remaining) {
    return l[i] == r[i] && stringEqual(l, r, remaining...);
}

template <std::size_t N, typename TL, typename TR, std::size_t...Is>
constexpr bool stringEqual(CompileTimeString<N, TL> const & l, CompileTimeString<N, TR> const & r, std::index_sequence<Is...>) {
    return stringEqual(l, r, Is...);
}

constexpr char charToUpper(char c) noexcept { return c >= 'a' && c <= 'z' ? c + 'A' - 'a' : c; }
constexpr char charToLower(char c) noexcept { return c >= 'A' && c <= 'Z' ? c - 'A' + 'a' : c; }

template <std::size_t N, typename Derived>
class CompileTimeStringBase
{
public:
    constexpr char const * c_str() const { return derived().array_; }
    constexpr char operator[](std::size_t index) const { return c_str()[index]; }
    constexpr operator const char * () const { return c_str(); }

    constexpr std::size_t size() const { return N; }
    constexpr bool empty() const { return N == 0; }

    constexpr StringArray<N> toUpper() const { return toUpper(std::make_index_sequence<N>{}); }
    constexpr StringArray<N> toLower() const { return toUpper(std::make_index_sequence<N>{}); }

private:
    constexpr Derived & derived() { return static_cast<Derived &>(*this); }
    constexpr Derived const & derived() const { return static_cast<Derived const &>(*this); }

    template <std::size_t... Is>
    constexpr StringArray<N> toUpper(std::index_sequence<Is...>) const {
        return StringArray<N>(charToUpper((*this)[Is])...);
    }

    template <std::size_t... Is>
    constexpr StringArray<N> toLower(std::index_sequence<Is...>) const {
        return StringArray<N>(charToLower((*this)[Is])...);
    }
};

} // namespace detail

template <std::size_t N>
class CompileTimeString<N, detail::literal_tag>
    : public detail::CompileTimeStringBase<N, CompileTimeString<N, detail::literal_tag>>
{
public:
    friend detail::CompileTimeStringBase<N, CompileTimeString<N, detail::literal_tag>>;
    constexpr CompileTimeString<N, detail::literal_tag>(char const (&s)[N+1]) noexcept
        : array_(s)
    {
    }

    using arr_t = const char[N + 1];
    constexpr operator const arr_t&() const { return array_; }

    constexpr operator std::string_view() const { return array_; }

private:
    char const (&array_)[N + 1];
};

template <std::size_t N>
class CompileTimeString<N, detail::char_array_tag>
    : public detail::CompileTimeStringBase<N, CompileTimeString<N, detail::char_array_tag>>
{
public:
    friend detail::CompileTimeStringBase<N, CompileTimeString<N, detail::char_array_tag>>;

    constexpr CompileTimeString<N, detail::char_array_tag>(CompileTimeString<N, detail::literal_tag> const & s)
        : array_{0}
    {
        for (int i = 0; i < N; ++i) {
            array_[i] = s[i];
        }
    }

    constexpr explicit CompileTimeString<N, detail::char_array_tag>(char c)
        : array_{c, 0}
    {
    }

    template <typename...Cs>
    constexpr explicit CompileTimeString<N, detail::char_array_tag>(Cs... chars)
        : array_{chars..., 0}
    {
    }

    template <std::size_t M, typename TL, typename TR>
    constexpr explicit CompileTimeString<N, detail::char_array_tag>(CompileTimeString<M, TL> const & l, CompileTimeString<N-M, TR> const & r)
        : array_{0}
    {
        int pos = 0;
        for (int i = 0; i < M; ++i, ++pos) {
            array_[pos] = l[i];
        }
        for (int i = 0; i < N-M; ++i, ++pos) {
            array_[pos] = r[i];
        }
    }

private:
    char array_[N+1];
};

template <std::size_t N>
constexpr StringLiteral<N-1> literal(char const (&s)[N])
{
    return StringLiteral<N-1>(s);
}

constexpr StringArray<1> literal(char c)
{
    return StringArray<1>(c);
}

template <std::size_t N1, std::size_t N2, typename TL, typename TR>
constexpr StringArray<N1+N2> operator+(CompileTimeString<N1, TL> const & l, CompileTimeString<N2, TR> const & r) {
    return StringArray<N1+N2>(l, r);
}

template <std::size_t N1, std::size_t N2, typename T>
constexpr StringArray<N1+N2-1> operator+(CompileTimeString<N1, T> const & l, char const (&r)[N2]) {
    return StringArray<N1+N2-1>(l, literal(r));
}

template <std::size_t N1, std::size_t N2, typename T>
constexpr StringArray<N1+N2-1> operator+(const char (&l)[N1], CompileTimeString<N2, T> const & r) {
    return StringArray<N1+N2-1>(literal(l), r);
}

template <std::size_t N, typename T>
constexpr StringArray<N+1> operator+(char l, CompileTimeString<N, T> const & r) {
    return StringArray<N+1>(literal(l), r);
}

template <std::size_t N, typename T>
constexpr StringArray<N+1> operator+(CompileTimeString<N, T> const & l, char r) {
    return StringArray<N+1>(l, literal(r));
}


template <std::size_t N1, std::size_t N2, typename TL, typename TR>
constexpr bool operator==(CompileTimeString<N1, TL> const & l, CompileTimeString<N2, TR> const & r) {
    return detail::stringEqual(l, r, std::make_index_sequence<N1>{});
}

template <std::size_t N1, std::size_t N2, typename T>
constexpr bool operator==(CompileTimeString<N1, T> const & l, const char (&r)[N2]) {
    return l == literal(r);
}

template <std::size_t N1, std::size_t N2, typename T>
constexpr bool operator==(const char (&l)[N1], CompileTimeString<N2, T> const & r) {
    return literal(l) == r;
}

template <std::size_t N1, std::size_t N2, typename TL, typename TR>
constexpr bool operator!=(CompileTimeString<N1, TL> const & l, CompileTimeString<N2, TR> const & r) {
    return !(l == r);
}

template <std::size_t N1, std::size_t N2, typename T>
constexpr bool operator!=(CompileTimeString<N1, T> const & l, const char (&r)[N2]) {
    return !(l == r);
}

template <std::size_t N1, std::size_t N2, typename T>
constexpr bool operator!=(const char (&l)[N1], CompileTimeString<N2, T> const & r) {
    return !(l == r);
}

} // namespace jd
