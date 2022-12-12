//
// Copyright (c) 2021 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_TRAITS_FUTURE_VALUE_HPP
#define FUTURES_TRAITS_FUTURE_VALUE_HPP

/**
 *  @file traits/future_value.hpp
 *  @brief `future_value` trait
 *
 *  This file defines the `future_value` trait.
 */

#include <futures/config.hpp>
#include <futures/traits/is_future.hpp>
#include <futures/detail/traits/future_value.hpp>

namespace futures {
    /** @addtogroup futures Futures
     *  @{
     */
    /** @addtogroup future-traits Future Traits
     *  @{
     */


    /// Determine type the future object holds
    /**
     *  Primary template handles non-future types
     *
     *  @note Not to be confused with continuation unwrapping
     */
#ifdef FUTURES_DOXYGEN
    template <class T>
    using future_value = __see_below__;
#else
    template <class T, class = void>
    struct future_value {};

    // specialization for types that implement get()
    template <class Future>
    struct future_value<
        Future,
        std::enable_if_t<
            is_future_v<Future>
            && detail::has_get<std::decay_t<Future>>::value>> {
        using type = std::decay_t<
            decltype(std::declval<std::decay_t<Future>>().get())>;
    };
#endif

    /// @copydoc future_value
    template <class T>
    using future_value_t = typename future_value<T>::type;

    /** @} */ // @addtogroup future-traits Future Traits
    /** @} */ // @addtogroup futures Futures
} // namespace futures

#endif // FUTURES_TRAITS_FUTURE_VALUE_HPP
