// Eggs.Variant
//
// Copyright Agustin K-ballo Berge, Fusion Fenix 2014-2017
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <eggs/variant.hpp>
#include <string>
#include <typeinfo>

#include <eggs/variant/detail/config/prefix.hpp>

#define CATCH_CONFIG_MAIN
#include "catch.hpp"
#include "constexpr.hpp"
#include "dtor.hpp"
#include "throw.hpp"

EGGS_CXX11_STATIC_CONSTEXPR std::size_t npos = eggs::variant<>::npos;

struct NonAssignable
{
    NonAssignable() {}
    NonAssignable(NonAssignable&&) {}; // not trivially copyable
    NonAssignable& operator=(NonAssignable const&) = delete;
    ~NonAssignable() {}
};

struct NonAssignableTrivial
{
    NonAssignableTrivial() {}
    NonAssignableTrivial(NonAssignableTrivial&&) = default;
    NonAssignableTrivial& operator=(NonAssignableTrivial const&) = delete;
    ~NonAssignableTrivial() = default;
};

TEST_CASE("variant<Ts...>::emplace<I>(Args&&...)", "[variant.assign]")
{
    // empty target
    {
        eggs::variant<int, std::string> v;

        REQUIRE(bool(v) == false);
        REQUIRE(v.which() == npos);

        int& r = v.emplace<0>(42);

        CHECK(bool(v) == true);
        CHECK(v.which() == 0u);
        CHECK(v.target<int>() == &r);
        CHECK(r == 42);

#if EGGS_CXX98_HAS_RTTI
        CHECK(v.target_type() == typeid(int));
#endif

#if EGGS_CXX14_HAS_CONSTEXPR
        // constexpr
        {
            struct test { static constexpr int call()
            {
                eggs::variant<int, ConstexprTrivial> v;
                ConstexprTrivial& r = v.emplace<1>(42);
                return 0;
            }};
            constexpr int c = test::call();
        }
#endif
    }

    // same target
    {
        eggs::variant<int, std::string> v(43);

        REQUIRE(bool(v) == true);
        REQUIRE(v.which() == 0u);
        REQUIRE(*v.target<int>() == 43);

        int& r = v.emplace<0>(42);

        CHECK(bool(v) == true);
        CHECK(v.which() == 0u);
        CHECK(v.target<int>() == &r);
        CHECK(r == 42);

#if EGGS_CXX98_HAS_RTTI
        CHECK(v.target_type() == typeid(int));
#endif

        // dtor
        {
            eggs::variant<int, Dtor> v(eggs::variants::in_place<Dtor>);

            REQUIRE(v.which() == 1u);
            REQUIRE(Dtor::calls == 0u);

            v.emplace<1>();

            CHECK(v.which() == 1u);
            CHECK(Dtor::calls == 1u);
        }
        Dtor::calls = 0u;

#if EGGS_CXX14_HAS_CONSTEXPR
        // constexpr
        {
            struct test { static constexpr int call()
            {
                eggs::variant<int, ConstexprTrivial> v(ConstexprTrivial(43));
                ConstexprTrivial& r = v.emplace<1>(42);
                return 0;
            }};
            constexpr int c = test::call();
        }
#endif
    }

    // different target
    {
        eggs::variant<int, std::string> v(std::string(""));

        REQUIRE(bool(v) == true);
        REQUIRE(v.which() == 1u);
        REQUIRE(*v.target<std::string>() == "");

        int& r = v.emplace<0>(42);

        CHECK(bool(v) == true);
        CHECK(v.which() == 0u);
        CHECK(v.target<int>() == &r);
        CHECK(r == 42);

#if EGGS_CXX98_HAS_RTTI
        CHECK(v.target_type() == typeid(int));
#endif

        // dtor
        {
            eggs::variant<int, Dtor> v(eggs::variants::in_place<Dtor>);

            REQUIRE(v.which() == 1u);
            REQUIRE(Dtor::calls == 0u);

            v.emplace<0>(42);

            CHECK(v.which() == 0u);
            CHECK(Dtor::calls == 1u);
        }
        Dtor::calls = 0u;

#if EGGS_CXX98_HAS_EXCEPTIONS
        // exception-safety
        {
            eggs::variant<Dtor, Throw> v;
            v.emplace<0>();

            REQUIRE(bool(v) == true);
            REQUIRE(v.which() == 0u);
            REQUIRE(Dtor::calls == 0u);

            CHECK_THROWS(v.emplace<1>(0));

            CHECK(bool(v) == false);
            CHECK(v.which() == npos);
            CHECK(Dtor::calls == 1u);
        }
        Dtor::calls = 0u;
#endif

#if EGGS_CXX14_HAS_CONSTEXPR
        // constexpr
        {
            struct test { static constexpr int call()
            {
                eggs::variant<int, ConstexprTrivial> v(43);
                ConstexprTrivial& r = v.emplace<1>(42);
                return 0;
            }};
            constexpr int c = test::call();
        }
#endif
    }
}

TEST_CASE("variant<T, T>::emplace<I>(Args&&...)", "[variant.assign]")
{
    eggs::variant<int, int> v;

    REQUIRE(bool(v) == false);
    REQUIRE(v.which() == npos);

    int& r = v.emplace<0>(42);

    CHECK(bool(v) == true);
    CHECK(v.which() == 0u);
    CHECK(v.target<int>() == &r);
    CHECK(r == 42);

#if EGGS_CXX98_HAS_RTTI
    CHECK(v.target_type() == typeid(int));
#endif
}

TEST_CASE("variant<NonAssignable>::emplace<I>(Args&&...)", "[variant.assign]")
{
    eggs::variant<int, NonAssignable> v(42);

    REQUIRE(v.which() == 0);

    NonAssignable& r = v.emplace<1>();

    CHECK(bool(v) == true);
    CHECK(v.which() == 1u);
    CHECK(v.target<NonAssignable>() == &r);

#if EGGS_CXX11_STD_HAS_IS_TRIVIALLY_COPYABLE
    // trivially_copyable
    {
        eggs::variant<int, NonAssignableTrivial> v(42);

        REQUIRE(v.which() == 0);

        NonAssignableTrivial& r = v.emplace<1>();

        CHECK(bool(v) == true);
        CHECK(v.which() == 1u);
        CHECK(v.target<NonAssignableTrivial>() == &r);
    }
#endif
}

TEST_CASE("variant<Ts...>::emplace<I>(std::initializer_list<U>, Args&&...)", "[variant.assign]")
{
    // empty target
    {
        eggs::variant<int, std::string> v;

        REQUIRE(bool(v) == false);
        REQUIRE(v.which() == npos);

        std::string& r = v.emplace<1>({'4', '2'});

        CHECK(bool(v) == true);
        CHECK(v.which() == 1u);
        CHECK(v.target<std::string>() == &r);
        CHECK(r == "42");

#if EGGS_CXX98_HAS_RTTI
        CHECK(v.target_type() == typeid(std::string));
#endif

#if EGGS_CXX14_HAS_CONSTEXPR
        // constexpr
        {
            struct test { static constexpr int call()
            {
                eggs::variant<int, ConstexprTrivial> v;
                ConstexprTrivial& r = v.emplace<1>({4, 2});
                return 0;
            }};
            constexpr int c = test::call();
        }
#endif
    }

    // same target
    {
        eggs::variant<int, std::string> v(std::string(""));

        REQUIRE(bool(v) == true);
        REQUIRE(v.which() == 1u);
        REQUIRE(*v.target<std::string>() == "");

        std::string& r = v.emplace<1>({'4', '2'});

        CHECK(bool(v) == true);
        CHECK(v.which() == 1u);
        CHECK(v.target<std::string>() == &r);
        CHECK(r == "42");

#if EGGS_CXX98_HAS_RTTI
        CHECK(v.target_type() == typeid(std::string));
#endif

#if EGGS_CXX14_HAS_CONSTEXPR
        // constexpr
        {
            struct test { static constexpr int call()
            {
                eggs::variant<int, ConstexprTrivial> v(ConstexprTrivial(43));
                ConstexprTrivial& r = v.emplace<1>({4, 2});
                return 0;
            }};
            constexpr int c = test::call();
        }
#endif
    }

    // different target
    {
        eggs::variant<int, std::string> v(43);

        REQUIRE(bool(v) == true);
        REQUIRE(v.which() == 0u);
        REQUIRE(*v.target<int>() == 43);

        std::string& r = v.emplace<1>({'4', '2'});

        CHECK(bool(v) == true);
        CHECK(v.which() == 1u);
        CHECK(v.target<std::string>() == &r);
        CHECK(r == "42");

#if EGGS_CXX98_HAS_RTTI
        CHECK(v.target_type() == typeid(std::string));
#endif

#if EGGS_CXX98_HAS_EXCEPTIONS
        // exception-safety
        {
            eggs::variant<Dtor, Throw> v;
            v.emplace<0>();

            REQUIRE(bool(v) == true);
            REQUIRE(v.which() == 0u);
            REQUIRE(Dtor::calls == 0u);

            CHECK_THROWS(v.emplace<1>({0}));

            CHECK(bool(v) == false);
            CHECK(v.which() == npos);
            CHECK(Dtor::calls == 1u);
        }
        Dtor::calls = 0u;
#endif

#if EGGS_CXX14_HAS_CONSTEXPR
        // constexpr
        {
            struct test { static constexpr int call()
            {
                eggs::variant<int, ConstexprTrivial> v(43);
                ConstexprTrivial& r = v.emplace<1>({4, 2});
                return 0;
            }};
            constexpr int c = test::call();
        }
#endif
    }
}

TEST_CASE("variant<T, T>::emplace<I>(std::initializer_list<U>, Args&&...)", "[variant.assign]")
{
    eggs::variant<std::string, std::string> v;

    REQUIRE(bool(v) == false);
    REQUIRE(v.which() == npos);

    std::string& r = v.emplace<1>({'4', '2'});

    CHECK(bool(v) == true);
    CHECK(v.which() == 1u);
    CHECK(v.target<std::string>() == &r);
    CHECK(r == "42");

#if EGGS_CXX98_HAS_RTTI
    CHECK(v.target_type() == typeid(std::string));
#endif
}

TEST_CASE("variant<Ts...>::emplace<T>(Args&&...)", "[variant.assign]")
{
    // empty target
    {
        eggs::variant<int, std::string> v;

        REQUIRE(bool(v) == false);
        REQUIRE(v.which() == npos);

        int& r = v.emplace<int>(42);

        CHECK(bool(v) == true);
        CHECK(v.which() == 0u);
        CHECK(v.target<int>() == &r);
        CHECK(r == 42);

#if EGGS_CXX98_HAS_RTTI
        CHECK(v.target_type() == typeid(int));
#endif

#if EGGS_CXX14_HAS_CONSTEXPR
        // constexpr
        {
            struct test { static constexpr int call()
            {
                eggs::variant<int, ConstexprTrivial> v;
                ConstexprTrivial& r = v.emplace<ConstexprTrivial>(42);
                return 0;
            }};
            constexpr int c = test::call();
        }
#endif
    }

    // same target
    {
        eggs::variant<int, std::string> v(43);

        REQUIRE(bool(v) == true);
        REQUIRE(v.which() == 0u);
        REQUIRE(*v.target<int>() == 43);

        int& r = v.emplace<int>(42);

        CHECK(bool(v) == true);
        CHECK(v.which() == 0u);
        CHECK(v.target<int>() == &r);
        CHECK(r == 42);

#if EGGS_CXX98_HAS_RTTI
        CHECK(v.target_type() == typeid(int));
#endif

        // dtor
        {
            eggs::variant<int, Dtor> v(eggs::variants::in_place<Dtor>);

            REQUIRE(v.which() == 1u);
            REQUIRE(Dtor::calls == 0u);

            v.emplace<Dtor>();

            CHECK(v.which() == 1u);
            CHECK(Dtor::calls == 1u);
        }
        Dtor::calls = 0u;

#if EGGS_CXX14_HAS_CONSTEXPR
        // constexpr
        {
            struct test { static constexpr int call()
            {
                eggs::variant<int, ConstexprTrivial> v(ConstexprTrivial(43));
                ConstexprTrivial& r = v.emplace<ConstexprTrivial>(42);
                return 0;
            }};
            constexpr int c = test::call();
        }
#endif
    }

    // different target
    {
        eggs::variant<int, std::string> v(std::string(""));

        REQUIRE(bool(v) == true);
        REQUIRE(v.which() == 1u);
        REQUIRE(*v.target<std::string>() == "");

        int& r = v.emplace<int>(42);

        CHECK(bool(v) == true);
        CHECK(v.which() == 0u);
        CHECK(v.target<int>() == &r);
        CHECK(r == 42);

#if EGGS_CXX98_HAS_RTTI
        CHECK(v.target_type() == typeid(int));
#endif

        // dtor
        {
            eggs::variant<int, Dtor> v(eggs::variants::in_place<Dtor>);

            REQUIRE(v.which() == 1u);
            REQUIRE(Dtor::calls == 0u);

            v.emplace<int>(42);

            CHECK(v.which() == 0u);
            CHECK(Dtor::calls == 1u);
        }
        Dtor::calls = 0u;

#if EGGS_CXX98_HAS_EXCEPTIONS
        // exception-safety
        {
            eggs::variant<Dtor, Throw> v;
            v.emplace<0>();

            REQUIRE(bool(v) == true);
            REQUIRE(v.which() == 0u);
            REQUIRE(Dtor::calls == 0u);

            CHECK_THROWS(v.emplace<Throw>(0));

            CHECK(bool(v) == false);
            CHECK(v.which() == npos);
            CHECK(Dtor::calls == 1u);
        }
        Dtor::calls = 0u;
#endif

#if EGGS_CXX14_HAS_CONSTEXPR
        // constexpr
        {
            struct test { static constexpr int call()
            {
                eggs::variant<int, ConstexprTrivial> v(43);
                ConstexprTrivial& r = v.emplace<ConstexprTrivial>(42);
                return 0;
            }};
            constexpr int c = test::call();
        }
#endif
    }
}

TEST_CASE("variant<NonAssignable>::emplace<T>(Args&&...)", "[variant.assign]")
{
    eggs::variant<int, NonAssignable> v(42);

    REQUIRE(v.which() == 0);

    NonAssignable& r = v.emplace<NonAssignable>();

    CHECK(bool(v) == true);
    CHECK(v.which() == 1u);
    CHECK(v.target<NonAssignable>() == &r);

#if EGGS_CXX11_STD_HAS_IS_TRIVIALLY_COPYABLE
    // trivially_copyable
    {
        eggs::variant<int, NonAssignableTrivial> v(42);

        REQUIRE(v.which() == 0);

        NonAssignableTrivial& r = v.emplace<NonAssignableTrivial>();

        CHECK(bool(v) == true);
        CHECK(v.which() == 1u);
        CHECK(v.target<NonAssignableTrivial>() == &r);
    }
#endif
}

TEST_CASE("variant<Ts...>::emplace<T>(std::initializer_list<U>, Args&&...)", "[variant.assign]")
{
    // empty target
    {
        eggs::variant<int, std::string> v;

        REQUIRE(bool(v) == false);
        REQUIRE(v.which() == npos);

        std::string& r = v.emplace<std::string>({'4', '2'});

        CHECK(bool(v) == true);
        CHECK(v.which() == 1u);
        CHECK(v.target<std::string>() == &r);
        CHECK(r == "42");

#if EGGS_CXX98_HAS_RTTI
        CHECK(v.target_type() == typeid(std::string));
#endif

#if EGGS_CXX14_HAS_CONSTEXPR
        // constexpr
        {
            struct test { static constexpr int call()
            {
                eggs::variant<int, ConstexprTrivial> v;
                ConstexprTrivial& r = v.emplace<ConstexprTrivial>({4, 2});
                return 0;
            }};
            constexpr int c = test::call();
        }
#endif
    }

    // same target
    {
        eggs::variant<int, std::string> v(std::string(""));

        REQUIRE(bool(v) == true);
        REQUIRE(v.which() == 1u);
        REQUIRE(*v.target<std::string>() == "");

        std::string& r = v.emplace<std::string>({'4', '2'});

        CHECK(bool(v) == true);
        CHECK(v.which() == 1u);
        CHECK(v.target<std::string>() == &r);
        CHECK(r == "42");

#if EGGS_CXX98_HAS_RTTI
        CHECK(v.target_type() == typeid(std::string));
#endif

#if EGGS_CXX14_HAS_CONSTEXPR
        // constexpr
        {
            struct test { static constexpr int call()
            {
                eggs::variant<int, ConstexprTrivial> v(ConstexprTrivial(43));
                ConstexprTrivial& r = v.emplace<ConstexprTrivial>({4, 2});
                return 0;
            }};
            constexpr int c = test::call();
        }
#endif
    }

    // different target
    {
        eggs::variant<int, std::string> v(43);

        REQUIRE(bool(v) == true);
        REQUIRE(v.which() == 0u);
        REQUIRE(*v.target<int>() == 43);

        std::string& r = v.emplace<std::string>({'4', '2'});

        CHECK(bool(v) == true);
        CHECK(v.which() == 1u);
        CHECK(v.target<std::string>() == &r);
        CHECK(r == "42");

#if EGGS_CXX98_HAS_RTTI
        CHECK(v.target_type() == typeid(std::string));
#endif

#if EGGS_CXX98_HAS_EXCEPTIONS
        // exception-safety
        {
            eggs::variant<Dtor, Throw> v;
            v.emplace<0>();

            REQUIRE(bool(v) == true);
            REQUIRE(v.which() == 0u);
            REQUIRE(Dtor::calls == 0u);

            CHECK_THROWS(v.emplace<Throw>({0}));

            CHECK(bool(v) == false);
            CHECK(v.which() == npos);
            CHECK(Dtor::calls == 1u);
        }
        Dtor::calls = 0u;
#endif

#if EGGS_CXX14_HAS_CONSTEXPR
        // constexpr
        {
            struct test { static constexpr int call()
            {
                eggs::variant<int, ConstexprTrivial> v(43);
                ConstexprTrivial& r = v.emplace<ConstexprTrivial>({4, 2});
                return 0;
            }};
            constexpr int c = test::call();
        }
#endif
    }
}
