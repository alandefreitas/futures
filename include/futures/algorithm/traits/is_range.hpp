//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ALGORITHM_TRAITS_IS_RANGE_HPP
#define FUTURES_ALGORITHM_TRAITS_IS_RANGE_HPP

/**
 *  @file algorithm/traits/is_range.hpp
 *  @brief `is_range` trait
 *
 *  This file defines the `is_range` trait.
 */

#include <futures/detail/traits/std_type_traits.hpp>
#include <iterator>
#include <type_traits>

namespace futures {
    /** @addtogroup algorithms Algorithms
     *  @{
     */

    /** @addtogroup traits Traits
     *  @{
     */

    /// @brief A type trait equivalent to the `std::range`
    /// concept
    /**
     * @see
     * [`std::ranges::range`](https://en.cppreference.com/w/cpp/ranges/range)
     */
#ifdef FUTURES_DOXYGEN
    template <class T>
    using is_range = std::bool_constant<std::range<T>>;
#else
    template <class T, class = void>
    struct is_range : std::false_type {};

    template <class T>
    struct is_range<
        T,
        detail::void_t<
            // clang-format off
            decltype(*std::begin(std::declval<T>())),
            decltype(*std::end(std::declval<T>()))
            // clang-format on
            >> : std::true_type {};
#endif

    /// @copydoc is_range
    template <class T>
    constexpr bool is_range_v = is_range<T>::value;

    /** @} */
    /** @} */
} // namespace futures

#endif // FUTURES_ALGORITHM_TRAITS_IS_RANGE_HPP
