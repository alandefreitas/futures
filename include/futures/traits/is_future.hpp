//
// Copyright (c) 2021 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_TRAITS_IS_FUTURE_HPP
#define FUTURES_TRAITS_IS_FUTURE_HPP

/**
 *  @file traits/is_future.hpp
 *  @brief `is_future` trait
 *
 *  This file defines the `is_future` trait.
 */

#include <futures/config.hpp>
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

    /// Customization point to determine if a type is a future type
    /**
     * This trait identifies whether the type represents a future value.
     *
     * Unless the trait is specialized, a type is considered stoppable
     * if it has the `get()` member function.
     *
     * @see
     *      @li has_stop_token
     */
#ifdef FUTURES_DOXYGEN
    template <class T>
    using is_future = __see_below__;
#else
    namespace detail {
        template <class T, class = void>
        struct is_future_impl : std::false_type {};

        template <class T>
        struct is_future_impl<
            T,
            detail::void_t<
                // clang-format off
            decltype(std::declval<T>().get())
                // clang-format on
                >> : std::true_type {};
    } // namespace detail

    template <class T>
    struct is_future : detail::is_future_impl<T> {};
#endif

    /// @copydoc is_future
    template <class T>
    constexpr bool is_future_v = is_future<T>::value;

#ifdef FUTURES_HAS_CONCEPTS
    /// @concept future_like
    /// @brief An object with the common members of a future.
    /**
     * A class is considered future-like when 1) it specializes the
     * `is_future` trait to indicate it is a future type, or 2) it has the
     * a `get()` function to obtain its future value.
     *
     * This allows algorithms to interoperate with future types from other
     * libraries.
     *
     * @tparam T The type being tested for conformance to the future_like
     * concept.
     */
    template <class T>
    concept future_like = is_future_v<std::decay_t<T>>;
#endif

    /** @} */ // @addtogroup future-traits Future Traits
    /** @} */ // @addtogroup futures Futures
} // namespace futures

#endif // FUTURES_TRAITS_IS_FUTURE_HPP
