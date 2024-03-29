//
// Copyright (c) 2021 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_AWAIT_HPP
#define FUTURES_AWAIT_HPP

/**
 *  @file await.hpp
 *  @brief Helper function to wait for futures
 *
 *  This file defines syntax sugar to wait for futures.
 */

#include <futures/traits/future_value.hpp>
#include <futures/traits/is_future_like.hpp>
#include <futures/detail/utility/regular_void.hpp>
#include <type_traits>

namespace futures {
    /** @addtogroup futures Futures
     *  @{
     */

    /** @addtogroup waiting Waiting
     *
     * \brief Basic function to wait for futures
     *
     * This module defines a variety of auxiliary functions to wait for futures.
     *
     *  @{
     */

    /// Wait for future types and retrieve their values
    /**
     *  This syntax is most useful for cases where we are immediately requesting
     *  the future result.
     *
     *  The function also makes the syntax optionally a little closer to
     *  languages such as javascript.
     *
     *  @note This function only participates in overload resolution if all
     *  types are futures.
     *
     *  @param f A future object
     *
     *  @return The result of the future object
     */
#ifdef FUTURES_HAS_CONCEPTS
    template <future_like Future>
#else
    template <
        class Future,
        std::enable_if_t<is_future_like_v<std::decay_t<Future>>, int> = 0>
#endif
    FUTURES_DETAIL(decltype(auto))
    await(Future &&f) {
        return f.get();
    }

    /// Wait for future types and retrieve their values as a tuple
    /**
     * @note This function only participates in overload resolution if all
     * types are futures.
     *
     * @param fs Future objects
     *
     * @return The result of the future object
     */
#ifdef FUTURES_HAS_CONCEPTS
    template <future_like... Futures>
#else
    template <
        class... Futures,
        std::enable_if_t<
            detail::conjunction_v<is_future_like<std::decay_t<Futures>>...>,
            int>
        = 0>
#endif
    FUTURES_DETAIL(decltype(auto))
    await(Futures &&...fs) {
        return detail::make_irregular_tuple(detail::regular_void_invoke(
            &std::decay_t<Futures>::get,
            std::forward<Futures>(fs))...);
    }

    /** @} */
    /** @} */
} // namespace futures

#endif // FUTURES_AWAIT_HPP
