//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ALGORITHM_TRAITS_IS_CONVERTIBLE_TO_HPP
#define FUTURES_ALGORITHM_TRAITS_IS_CONVERTIBLE_TO_HPP

/**
 *  @file algorithm/traits/is_convertible_to.hpp
 *  @brief `is_convertible_to` trait
 *
 *  This file defines the `is_convertible_to` trait.
 */

#include <futures/detail/traits/std_type_traits.hpp>
#include <type_traits>

namespace futures {
    /** @addtogroup algorithms Algorithms
     *  @{
     */

    /** @addtogroup traits Traits
     *  @{
     */


    /// @brief A type trait equivalent to the `std::convertible_to` concept
    /**
     * @see [`std::convertible_to`](https://en.cppreference.com/w/cpp/concepts/convertible_to)
     */
#ifdef FUTURES_DOXYGEN
    template <class From, class To>
    using is_convertible_to = std::bool_constant<std::convertible_to<From, To>>;
#else
    template <class From, class To, class = void>
    struct is_convertible_to : std::false_type {};

    template <class From, class To>
    struct is_convertible_to<
        From,
        To,
        detail::void_t<
            // clang-format off
            std::enable_if_t<detail::is_convertible_v<From, To>>,
            decltype(static_cast<To>(std::declval<From>()))
            // clang-format on
            >> : std::true_type {};
#endif

    /// @copydoc is_convertible_to
    template <class From, class To>
    constexpr bool is_convertible_to_v = is_convertible_to<From, To>::value;
    /** @} */
    /** @} */

} // namespace futures

#endif // FUTURES_ALGORITHM_TRAITS_IS_CONVERTIBLE_TO_HPP
