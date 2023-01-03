//
// Copyright (c) 2021 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_DETAIL_SHARE_IF_NOT_SHARED_HPP
#define FUTURES_DETAIL_SHARE_IF_NOT_SHARED_HPP

#include <futures/traits/is_future_like.hpp>
#include <futures/traits/is_shared_future.hpp>
#include <futures/detail/traits/std_type_traits.hpp>

namespace futures {
    namespace detail {
        // Check if a type implements the share function
        // This is what we use to identify the return type of the future type
        // candidate.
        template <class T, typename = void>
        struct has_share : std::false_type {};

        template <class T>
        struct has_share<T, void_t<decltype(std::declval<T>().share())>>
            : std::true_type {};

        template <class Future>
        constexpr FUTURES_DETAIL(decltype(auto))
        share_if_not_shared_impl(mp_int<0> /* is_shared_future */, Future &&f) {
            return std::forward<Future>(f);
        }

        template <class Future>
        constexpr FUTURES_DETAIL(decltype(auto))
        share_if_not_shared_impl(mp_int<1> /* has_share */, Future &&f) {
            return std::forward<Future>(f).share();
        }

        template <class Future>
        constexpr FUTURES_DETAIL(decltype(auto))
        share_if_not_shared_impl(
            mp_int<2> /* !is_shared_future && !has_share */,
            Future &&f) {
            return std::move(std::forward<Future>(f));
        }

        template <class Future>
        constexpr FUTURES_DETAIL(decltype(auto))
        share_if_not_shared(Future &&f) {
            return share_if_not_shared_impl(
                mp_int < is_shared_future_v<std::decay_t<Future>> ?
                    0 :
                detail::has_share<std::decay_t<Future>>::value ?
                    1 :
                    2 > {},
                std::forward<Future>(f));
        }
    } // namespace detail
} // namespace futures
#endif // FUTURES_DETAIL_SHARE_IF_NOT_SHARED_HPP
