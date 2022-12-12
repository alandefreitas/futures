//
// Copyright (c) 2021 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_TRAITS_IS_STOPPABLE_HPP
#define FUTURES_TRAITS_IS_STOPPABLE_HPP

/**
 *  @file traits/is_stoppable.hpp
 *  @brief `is_stoppable` trait
 *
 *  This file defines the `is_stoppable` trait.
 */

#include <futures/config.hpp>
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

    /// @brief Customization point to define future as stoppable
    /**
     * This trait identifies whether the future is stoppable, which means
     * the future has a `request_stop` function to stop the underlying task.
     *
     * @note Not all stoppable futures have stops token, which can be shared
     * with other futures to create a common thread of futures that can be
     * stopped with the same token.
     *
     * Unless the trait is specialized, a type is considered stoppable
     * if it has the `request_stop()` member function.
     *
     * @see
     *      @li @ref has_stop_token
     */
#ifdef FUTURES_DOXYGEN
    template <class T>
    using is_stoppable = __see_below__;
#else
    template <class T, class = void>
    struct is_stoppable : std::false_type {};

    template <class T>
    struct is_stoppable<
        T,
        std::void_t<
            // clang-format off
            decltype(std::declval<T>().request_stop())
            // clang-format on
            >> : std::true_type {};
#endif

    /// @copydoc is_stoppable
    template <class T>
    constexpr bool is_stoppable_v = is_stoppable<T>::value;

    /** @} */ // @addtogroup future-traits Future Traits
    /** @} */ // @addtogroup futures Futures
} // namespace futures

#endif // FUTURES_TRAITS_IS_STOPPABLE_HPP
