//
// Copyright (c) 2021 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_DETAIL_MOVE_IF_NOT_SHARED_HPP
#define FUTURES_DETAIL_MOVE_IF_NOT_SHARED_HPP

#include <futures/traits/is_shared_future.hpp>

namespace futures {
    namespace detail {
        template <class Future>
        constexpr FUTURES_DETAIL(decltype(auto))
        move_if_not_shared_impl(
            std::true_type /* is_shared_future */,
            Future &&f) {
            return std::forward<Future>(f);
        }

        template <class Future>
        constexpr FUTURES_DETAIL(decltype(auto))
        move_if_not_shared_impl(
            std::false_type /* is_shared_future */,
            Future &&f) {
            return std::move(std::forward<Future>(f));
        }

        // Move or share a future, depending on the type of future input
        //
        // Create another future with the state of the f future (usually
        // for a continuation function). This state should be copied to the new
        // callback function. Shared futures can be copied. Normal futures
        // should be moved.
        // @return The moved future or the shared future
        template <class Future>
        constexpr FUTURES_DETAIL(decltype(auto))
        move_if_not_shared(Future &&f) {
            return move_if_not_shared_impl(
                is_shared_future<std::decay_t<Future>>{},
                std::forward<Future>(f));
        }
    } // namespace detail
} // namespace futures
#endif // FUTURES_DETAIL_MOVE_IF_NOT_SHARED_HPP
