// Eggs.Variant
//
// Copyright Agustin K-ballo Berge, Fusion Fenix 2014-2016
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef EGGS_VARIANT_TEST_EXPLICIT_HPP
#define EGGS_VARIANT_TEST_EXPLICIT_HPP

#include <type_traits>
#include <utility>

template <typename T>
struct Explicit
{
    T x;

    template <
        typename U
      , typename Enable = typename std::enable_if<
            std::is_constructible<T, U&&>::value
        >::type
    >
    explicit Explicit(U&& x) : x(std::forward<U>(x)) {}
};

#endif /*EGGS_VARIANT_TEST_EXPLICIT_HPP*/
