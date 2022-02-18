//
// Copyright (c) 2021 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_FUTURES_AWAIT_HPP
#define FUTURES_FUTURES_AWAIT_HPP

#include <futures/futures/traits/future_value.hpp>
#include <futures/futures/traits/is_future.hpp>
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
     *  @note This function only participates in overload resolutions if all
     *  types are futures.
     *
     *  @tparam Future A future type
     *
     * @return The result of the future object
     **/
    template <
        typename Future
#ifndef FUTURES_DOXYGEN
        ,
        std::enable_if_t<
            // clang-format off
            is_future_v<std::decay_t<Future>>
            // clang-format on
            ,
            int> = 0
#endif
        >
    decltype(auto)
    await(Future &&f) {
        return f.get();
    }

    namespace detail {
        template <
            typename Future
#ifndef FUTURES_DOXYGEN
            ,
            std::enable_if_t<
                // clang-format off
                is_future_v<std::decay_t<Future>>
                // clang-format on
                ,
                int> = 0
#endif
            >
        decltype(auto)
        await_tuple(Future &&f1) {
            if constexpr (std::is_void_v<future_value_t<std::decay_t<Future>>>)
            {
                std::forward<Future>(f1).get();
                return std::make_tuple();
            } else {
                return std::make_tuple(std::forward<Future>(f1).get());
            }
        }

        template <
            typename Future,
            typename... Futures
#ifndef FUTURES_DOXYGEN
            ,
            std::enable_if_t<
                // clang-format off
                is_future_v<std::decay_t<Future>> &&
                std::conjunction_v<is_future<std::decay_t<Futures>>...>
                // clang-format on
                ,
                int> = 0
#endif
            >
        decltype(auto)
        await_tuple(Future &&f1, Futures &&...fs) {
            return std::tuple_cat(
                await_tuple(std::forward<Future>(f1)),
                await_tuple(std::forward<Futures>(fs)...));
        }
    } // namespace detail


    /// Wait for future types and retrieve their values as a tuple
    /**
     * @note This function only participates in overload resolutions if all
     * types are futures.
     *
     * @tparam Future A future type
     * @tparam Futures Future types
     *
     * @return The result of the future object
     **/
    template <
        typename... Futures
#ifndef FUTURES_DOXYGEN
        ,
        std::enable_if_t<
            // clang-format off
            std::conjunction_v<is_future<std::decay_t<Futures>>...>
            // clang-format on
            ,
            int> = 0
#endif
        >
    decltype(auto)
    await(Futures &&...fs) {
        return detail::await_tuple(std::forward<Futures>(fs)...);
    }

    /** @} */
    /** @} */
} // namespace futures

#endif // FUTURES_FUTURES_AWAIT_HPP
