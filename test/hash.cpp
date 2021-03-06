// Eggs.Variant
//
// Copyright Agustin K-ballo Berge, Fusion Fenix 2014-2017
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <eggs/variant.hpp>
#include <functional>
#include <string>
#include <type_traits>

#include <eggs/variant/detail/config/prefix.hpp>

#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#if EGGS_CXX11_HAS_SFINAE_FOR_EXPRESSIONS
template <typename ...Ts>
struct _void
{
    using type = void;
};

template <typename T, typename Enable = void>
struct has_hash
  : std::false_type
{};

template <typename T>
struct has_hash<
    T, typename _void<
        decltype(std::hash<T>{}(std::declval<T const&>()))
    >::type
> : std::true_type
{};
#endif

struct NonHashable {};
// explicitly disable (pre C++17)
namespace std
{
    template <> struct hash<NonHashable> {};
}

TEST_CASE("std::hash<variant<Ts...>>", "[variant.hash]")
{
    eggs::variant<int, std::string> const v(42);

    REQUIRE(v.which() == 0u);
    REQUIRE(*v.target<int>() == 42);

    std::hash<eggs::variant<int, std::string>> variant_hasher;
    std::hash<int> int_hasher;

    CHECK(variant_hasher(v) == int_hasher(42));

#if EGGS_CXX11_HAS_SFINAE_FOR_EXPRESSIONS
    // sfinae
    {
        CHECK((
            !std::is_constructible<
                std::hash<eggs::variant<NonHashable>>
            >::value));
        CHECK((
            !std::is_copy_constructible<
                std::hash<eggs::variant<NonHashable>>
            >::value));
        CHECK((
            !std::is_move_constructible<
                std::hash<eggs::variant<NonHashable>>
            >::value));
        CHECK((
            !std::is_copy_assignable<
                std::hash<eggs::variant<NonHashable>>
            >::value));
        CHECK((
            !std::is_move_assignable<
                std::hash<eggs::variant<NonHashable>>
            >::value));
        CHECK((
            !has_hash<
                eggs::variant<NonHashable>
            >::value));
    }
#endif
}
