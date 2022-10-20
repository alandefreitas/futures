//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ALGORITHM_TRAITS_IS_EQUALITY_COMPARABLE_HPP
#define FUTURES_ALGORITHM_TRAITS_IS_EQUALITY_COMPARABLE_HPP

#include <type_traits>

namespace futures {
    /** @addtogroup algorithms Algorithms
     *  @{
     */

    /** @addtogroup traits Traits
     *  @{
     */


    /** \brief A C++17 type trait equivalent to the C++20 equality_comparable
     * concept
     */
#ifdef FUTURES_DOXYGEN
    template <class T>
    using is_equality_comparable = __see_below__;
#else
    template <class T, class = void>
    struct is_equality_comparable : std::false_type {};

    template <class T>
    struct is_equality_comparable<
        T,
        std::void_t<
            // clang-format off
            decltype(std::declval< std::remove_reference_t<T> const &>() == std::declval< std::remove_reference_t<T> const &>()),
            decltype(std::declval< std::remove_reference_t<T> const &>() != std::declval< std::remove_reference_t<T> const &>())
            // clang-format on
            >> : std::true_type {};
#endif

    /// @copydoc is_equality_comparable
    template <class T>
    constexpr bool is_equality_comparable_v = is_equality_comparable<T>::value;

    /** @} */
    /** @} */
} // namespace futures

#endif // FUTURES_ALGORITHM_TRAITS_IS_EQUALITY_COMPARABLE_HPP
