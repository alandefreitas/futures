//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ALGORITHM_TRAITS_IS_INDIRECTLY_READABLE_HPP
#define FUTURES_ALGORITHM_TRAITS_IS_INDIRECTLY_READABLE_HPP

/**
 *  @file algorithm/traits/is_indirectly_readable.hpp
 *  @brief `is_indirectly_readable` trait
 *
 *  This file defines the `is_indirectly_readable` trait.
 */

#include <futures/algorithm/traits/iter_reference.hpp>
#include <futures/algorithm/traits/iter_rvalue_reference.hpp>
#include <futures/algorithm/traits/iter_value.hpp>
#include <futures/detail/traits/std_type_traits.hpp>
#include <type_traits>

namespace futures {
    /** @addtogroup algorithms Algorithms
     *  @{
     */

    /** @addtogroup traits Traits
     *  @{
     */

    /// @brief A type trait equivalent to the `std::indirectly_readable` concept
    /**
     * @see [`std::indirectly_readable`](https://en.cppreference.com/w/cpp/iterator/indirectly_readable)
     */
#ifdef FUTURES_DOXYGEN
    template <class T>
    using is_indirectly_readable = std::bool_constant<
        std::indirectly_readable<T>>;
#else
    template <class T, class = void>
    struct is_indirectly_readable : std::false_type {};

    template <class T>
    struct is_indirectly_readable<
        T,
        detail::void_t<
            iter_value_t<T>,
            iter_reference_t<T>,
            iter_rvalue_reference_t<T>,
            decltype(*std::declval<T>())>> : std::true_type {};
#endif

    /// @copydoc is_indirectly_readable
    template <class T>
    constexpr bool is_indirectly_readable_v = is_indirectly_readable<T>::value;

    /** @} */
    /** @} */
} // namespace futures

#endif // FUTURES_ALGORITHM_TRAITS_IS_INDIRECTLY_READABLE_HPP
