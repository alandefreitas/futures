//
// Copyright (c) 2021 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_TRAITS_HAS_STOP_TOKEN_HPP
#define FUTURES_TRAITS_HAS_STOP_TOKEN_HPP

/**
 *  @file traits/has_stop_token.hpp
 *  @brief `has_stop_token` trait
 *
 *  This file defines the @ref has_stop_token trait.
 */

#include <futures/config.hpp>
#include <futures/traits/is_stoppable.hpp>
#include <futures/detail/traits/std_type_traits.hpp>
#include <type_traits>

namespace futures {
    /** @addtogroup futures Futures
     *  @{
     */
    /** @addtogroup future-traits Future Traits
     *
     * \brief Determine properties of future types
     *
     *  @{
     */

    /// Customization point to define future as having a common stop token
    /**
     * Besides being stoppable, this trait identifies whether the future
     * has a stop token, which means this token can be shared with other
     * futures to create a common thread of futures that can be stopped
     * with the same token.
     *
     * Unless the trait is specialized, a type is considered to have a
     * stop token if it has the `get_stop_source()` and `get_stop_token()`
     * member functions.
     *
     * @see
     *      @li @ref is_stoppable
     */
#ifdef FUTURES_DOXYGEN
    template <class T>
    using has_stop_token = __see_below__;
#else
    namespace detail {
        template <class T, class = void>
        struct has_stop_token_impl : std::false_type {};

        template <class T>
        struct has_stop_token_impl<
            T,
            detail::void_t<
                // clang-format off
            decltype(std::declval<T>().get_stop_source()),
            decltype(std::declval<T>().get_stop_token())
                // clang-format on
                >> : is_stoppable<T> {};
    }
    template <class T>
    struct has_stop_token : detail::has_stop_token_impl<T> {};
#endif

    /// @copydoc has_stop_token
    template <class T>
    constexpr bool has_stop_token_v = has_stop_token<T>::value;

    /** @} */ // @addtogroup future-traits Future Traits
    /** @} */ // @addtogroup futures Futures
} // namespace futures

#endif // FUTURES_TRAITS_HAS_STOP_TOKEN_HPP
