//! \file eggs/variant/detail/storage.hpp
// Eggs.Variant
//
// Copyright Agustin K-ballo Berge, Fusion Fenix 2014-2017
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef EGGS_VARIANT_DETAIL_STORAGE_HPP
#define EGGS_VARIANT_DETAIL_STORAGE_HPP

#include <eggs/variant/detail/pack.hpp>
#include <eggs/variant/detail/utility.hpp>
#include <eggs/variant/detail/visitor.hpp>

#include <cstddef>
#include <new>
#include <type_traits>
#include <typeinfo>
#include <utility>

#include <eggs/variant/detail/config/prefix.hpp>

namespace eggs { namespace variants { namespace detail
{
    template <typename Ts, bool IsTriviallyDestructible>
    struct _union;

    ///////////////////////////////////////////////////////////////////////////
    template <bool IsTriviallyDestructible>
    struct _union<pack<>, IsTriviallyDestructible>
    {};

    template <typename T, typename ...Ts>
    struct _union<pack<T, Ts...>, true>
    {
        EGGS_CXX11_STATIC_CONSTEXPR std::size_t size = 1 + sizeof...(Ts);

        template <typename ...Args>
        EGGS_CXX11_CONSTEXPR _union(index<0>, Args&&... args)
          : _head(detail::forward<Args>(args)...)
        {}

        template <std::size_t I, typename ...Args>
        EGGS_CXX11_CONSTEXPR _union(index<I>, Args&&... args)
          : _tail(index<I - 1>{}, detail::forward<Args>(args)...)
        {}

        EGGS_CXX14_CONSTEXPR void* target() noexcept
        {
            return &_target;
        }

        EGGS_CXX11_CONSTEXPR void const* target() const noexcept
        {
            return &_target;
        }

        EGGS_CXX14_CONSTEXPR T& get(index<0>) noexcept
        {
            return this->_head;
        }

        EGGS_CXX11_CONSTEXPR T const& get(index<0>) const noexcept
        {
            return this->_head;
        }

        template <
            std::size_t I
          , typename U = typename at_index<I, pack<T, Ts...>>::type
        >
        EGGS_CXX14_CONSTEXPR U& get(index<I>) noexcept
        {
            return this->_tail.get(index<I - 1>{});
        }

        template <
            std::size_t I
          , typename U = typename at_index<I, pack<T, Ts...>>::type
        >
        EGGS_CXX11_CONSTEXPR U const& get(index<I>) const noexcept
        {
            return this->_tail.get(index<I - 1>{});
        }

    private:
        union
        {
            char _target;
            T _head;
            _union<pack<Ts...>, true> _tail;
        };
    };

    template <typename T, typename ...Ts>
    struct _union<pack<T, Ts...>, false>
    {
        EGGS_CXX11_STATIC_CONSTEXPR std::size_t size = 1 + sizeof...(Ts);

        template <typename ...Args>
        EGGS_CXX11_CONSTEXPR _union(index<0>, Args&&... args)
          : _head(detail::forward<Args>(args)...)
        {}

        template <std::size_t I, typename ...Args>
        EGGS_CXX11_CONSTEXPR _union(index<I>, Args&&... args)
          : _tail(index<I - 1>{}, detail::forward<Args>(args)...)
        {}

        ~_union() {}

        EGGS_CXX14_CONSTEXPR void* target() noexcept
        {
            return &_target;
        }

        EGGS_CXX11_CONSTEXPR void const* target() const noexcept
        {
            return &_target;
        }

        EGGS_CXX14_CONSTEXPR T& get(index<0>) noexcept
        {
            return this->_head;
        }

        EGGS_CXX11_CONSTEXPR T const& get(index<0>) const noexcept
        {
            return this->_head;
        }

        template <
            std::size_t I
          , typename U = typename at_index<I, pack<T, Ts...>>::type
        >
        EGGS_CXX14_CONSTEXPR U& get(index<I>) noexcept
        {
            return this->_tail.get(index<I - 1>{});
        }

        template <
            std::size_t I
          , typename U = typename at_index<I, pack<T, Ts...>>::type
        >
        EGGS_CXX11_CONSTEXPR U const& get(index<I>) const noexcept
        {
            return this->_tail.get(index<I - 1>{});
        }

    private:
        union
        {
            char _target;
            T _head;
            _union<pack<Ts...>, false> _tail;
        };
    };

    ///////////////////////////////////////////////////////////////////////////
#if EGGS_CXX11_STD_HAS_IS_TRIVIALLY_COPYABLE && EGGS_CXX11_STD_HAS_IS_TRIVIALLY_DESTRUCTIBLE
    using std::is_trivially_copyable;
#else
    template <typename T>
    struct is_trivially_copyable
      : std::is_pod<T>
    {};
#endif

#if EGGS_CXX11_STD_HAS_IS_TRIVIALLY_DESTRUCTIBLE
    using std::is_trivially_destructible;
#else
    template <typename T>
    struct is_trivially_destructible
      : std::is_pod<T>
    {};
#endif

    ///////////////////////////////////////////////////////////////////////////
    template <typename Ts, bool TriviallyCopyable, bool TriviallyDestructible>
    struct _storage;

    template <typename ...Ts>
    struct _storage<pack<Ts...>, true, true>
      : _union<
            pack<Ts...>
          , all_of<pack<is_trivially_destructible<Ts>...>>::value
        >
    {
        using base_type = _union<
            pack<Ts...>
          , all_of<pack<is_trivially_destructible<Ts>...>>::value
        >;

        EGGS_CXX11_CONSTEXPR _storage() noexcept
          : base_type{index<0>{}}
          , _which{0}
        {}

        _storage(_storage const& rhs) = default;
        _storage(_storage&& rhs) = default;

        void _move(_storage& rhs)
        {
            detail::move_construct{}(
                pack<Ts...>{}, rhs._which
              , target(), rhs.target()
            );
            _which = rhs._which;
        }

        template <std::size_t I, typename ...Args>
        EGGS_CXX11_CONSTEXPR _storage(index<I> which, Args&&... args)
          : base_type{which, detail::forward<Args>(args)...}
          , _which{I}
        {}

        template <std::size_t I, typename ...Args>
        EGGS_CXX14_CONSTEXPR void _emplace(
            /*is_copy_assignable<Ts...>=*/std::true_type
          , index<I> which, Args&&... args)
        {
            *this = _storage(which, detail::forward<Args>(args)...);
        }

        template <
            std::size_t I, typename ...Args
          , typename T = typename at_index<I, pack<Ts...>>::type
        >
        void _emplace(
            /*is_copy_assignable<Ts...>=*/std::false_type
          , index<I> /*which*/, Args&&... args)
        {
            ::new (target()) T(detail::forward<Args>(args)...);
            _which = I;
        }

        template <
            std::size_t I, typename ...Args
          , typename T = typename at_index<I, pack<Ts...>>::type
        >
        EGGS_CXX14_CONSTEXPR T& emplace(index<I> which, Args&&... args)
        {
            using is_copy_assignable = all_of<pack<std::is_copy_assignable<Ts>...>>;
            _emplace(
                is_copy_assignable{}
              , which, detail::forward<Args>(args)...);
            return get(which);
        }

        _storage& operator=(_storage const& rhs) = default;
        _storage& operator=(_storage&& rhs) = default;

        EGGS_CXX14_CONSTEXPR void _swap(
            /*is_copy_assignable<Ts...>=*/std::true_type
          , _storage& rhs)
        {
            _storage tmp(detail::move(*this));
            *this = detail::move(rhs);
            rhs = detail::move(tmp);
        }

        void _swap(
            /*is_copy_assignable<Ts...>=*/std::false_type
          , _storage& rhs)
        {
            if (_which == rhs._which)
            {
                detail::swap{}(
                    pack<Ts...>{}, _which
                  , target(), rhs.target()
                );
            } else {
                _storage tmp(std::move(*this));
                _move(rhs);
                rhs._move(tmp);
            }
        }

        EGGS_CXX14_CONSTEXPR void swap(_storage& rhs)
        {
            _swap(
                all_of<pack<std::is_copy_assignable<Ts>...>>{}
              , rhs);
        }

        EGGS_CXX11_CONSTEXPR std::size_t which() const
        {
            return _which;
        }

        using base_type::target;
        using base_type::get;

    protected:
        std::size_t _which;
    };

    template <typename ...Ts>
    struct _storage<pack<Ts...>, false, true>
      : _storage<pack<Ts...>, true, true>
    {
        using base_type = _storage<pack<Ts...>, true, true>;

        _storage() = default;

        _storage(_storage const& rhs)
#if EGGS_CXX11_STD_HAS_IS_NOTHROW_TRAITS
            noexcept(all_of<pack<
                std::is_nothrow_copy_constructible<Ts>...
            >>::value)
#endif
          : base_type{}
        {
            detail::copy_construct{}(
                pack<Ts...>{}, rhs._which
              , target(), rhs.target()
            );
            _which = rhs._which;
        }

        _storage(_storage&& rhs)
#if EGGS_CXX11_STD_HAS_IS_NOTHROW_TRAITS
            noexcept(all_of<pack<
                std::is_nothrow_move_constructible<Ts>...
            >>::value)
#endif
          : base_type{}
        {
            detail::move_construct{}(
                pack<Ts...>{}, rhs._which
              , target(), rhs.target()
            );
            _which = rhs._which;
        }

        template <std::size_t I, typename ...Args>
        EGGS_CXX11_CONSTEXPR _storage(index<I> which, Args&&... args)
          : base_type{which, detail::forward<Args>(args)...}
        {}

        void _copy(_storage const& rhs)
        {
            _destroy();
            detail::copy_construct{}(
                pack<Ts...>{}, rhs._which
              , target(), rhs.target()
            );
            _which = rhs._which;
        }

        void _move(_storage& rhs)
        {
            _destroy();
            detail::move_construct{}(
                pack<Ts...>{}, rhs._which
              , target(), rhs.target()
            );
            _which = rhs._which;
        }

        template <
            std::size_t I, typename ...Args
          , typename T = typename at_index<I, pack<Ts...>>::type
        >
        T& emplace(index<I> which, Args&&... args)
        {
            _destroy();
            ::new (target()) T(detail::forward<Args>(args)...);
            _which = I;
            return get(which);
        }

        _storage& operator=(_storage const& rhs)
#if EGGS_CXX11_STD_HAS_IS_NOTHROW_TRAITS
            noexcept(all_of<pack<
                std::is_nothrow_copy_assignable<Ts>...
              , std::is_nothrow_copy_constructible<Ts>...
            >>::value)
#endif
        {
            if (_which == rhs._which)
            {
                detail::copy_assign{}(
                    pack<Ts...>{}, _which
                  , target(), rhs.target()
                );
            } else {
                _copy(rhs);
            }
            return *this;
        }

        _storage& operator=(_storage&& rhs)
#if EGGS_CXX11_STD_HAS_IS_NOTHROW_TRAITS
            noexcept(all_of<pack<
                std::is_nothrow_move_assignable<Ts>...
              , std::is_nothrow_move_constructible<Ts>...
            >>::value)
#endif
        {
            if (_which == rhs._which)
            {
                detail::move_assign{}(
                    pack<Ts...>{}, _which
                  , target(), rhs.target()
                );
            } else {
                _move(rhs);
            }
            return *this;
        }

        void swap(_storage& rhs)
        {
            if (_which == rhs._which)
            {
                detail::swap{}(
                    pack<Ts...>{}, _which
                  , target(), rhs.target()
                );
            } else if (_which == 0) {
                _move(rhs);
                rhs._destroy();
            } else if (rhs._which == 0) {
                rhs._move(*this);
                _destroy();
            } else {
                _storage tmp(std::move(*this));
                _move(rhs);
                rhs._move(tmp);
                tmp._destroy();
            }
        }

        using base_type::which;
        using base_type::target;
        using base_type::get;

    protected:
        void _destroy(
            /*is_trivially_destructible<Ts...>=*/std::true_type)
        {}

        void _destroy(
            /*is_trivially_destructible<Ts...>=*/std::false_type)
        {
            detail::destroy{}(
                pack<Ts...>{}, _which
              , target()
            );
        }

        void _destroy()
        {
            _destroy(all_of<pack<is_trivially_destructible<Ts>...>>{});
            _which = 0;
        }

    protected:
        using base_type::_which;
    };

    template <typename ...Ts>
    struct _storage<pack<Ts...>, false, false>
      : _storage<pack<Ts...>, false, true>
    {
        using base_type = _storage<pack<Ts...>, false, true>;

        _storage() = default;
        _storage(_storage const& rhs) = default;
        _storage(_storage&& rhs) = default;

        template <std::size_t I, typename ...Args>
        EGGS_CXX11_CONSTEXPR _storage(index<I> which, Args&&... args)
          : base_type{which, detail::forward<Args>(args)...}
        {}

        ~_storage()
        {
            base_type::_destroy();
        }

        using base_type::emplace;

        _storage& operator=(_storage const& rhs) = default;
        _storage& operator=(_storage&& rhs) = default;

        using base_type::swap;

        using base_type::which;
        using base_type::target;
        using base_type::get;

    protected:
        using base_type::_which;
    };

    template <typename ...Ts>
    using storage = _storage<
        pack<empty, Ts...>
      , all_of<pack<is_trivially_copyable<Ts>...>>::value
      , all_of<pack<is_trivially_destructible<Ts>...>>::value
    >;

    struct empty_storage
    {
        EGGS_CXX11_STATIC_CONSTEXPR std::size_t size = 1;

        static EGGS_CXX11_CONSTEXPR std::size_t which()
        {
            return 0;
        }
    };
}}}

#include <eggs/variant/detail/config/suffix.hpp>

#endif /*EGGS_VARIANT_DETAIL_STORAGE_HPP*/
