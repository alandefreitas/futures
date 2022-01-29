//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ALGORITHM_TRAITS_IS_WEAKLY_INCREMENTABLE_H
#define FUTURES_ALGORITHM_TRAITS_IS_WEAKLY_INCREMENTABLE_H

#include <futures/algorithm/traits/is_movable.hpp>
#include <futures/algorithm/traits/iter_difference.hpp>
#include <type_traits>

namespace futures {
    /** \addtogroup algorithms Algorithms
     *  @{
     */

    /** \addtogroup traits Traits
     *  @{
     */


    /** \brief A C++17 type trait equivalent to the C++20 weakly_incrementable concept
     */
#ifdef FUTURES_DOXYGEN
    template <class I>
    using is_weakly_incrementable = __see_below__;
#else
    template <class I, class = void>
    struct is_weakly_incrementable : std::false_type
    {};

    template <class I>
    struct is_weakly_incrementable<
        I,
        std::void_t<
            // clang-format off
            decltype(std::declval<I>()++),
            decltype(++std::declval<I>()),
            iter_difference_t<I>
            // clang-format on
            >>
        : std::conjunction<
              // clang-format off
              is_movable<I>,
              std::is_same<decltype(++std::declval<I>()), I&>
              // clang-format on
              >
    {};
#endif
    template <class I>
    bool constexpr is_weakly_incrementable_v = is_weakly_incrementable<I>::value;

    /** @}*/
    /** @}*/

} // namespace futures

#endif // FUTURES_ALGORITHM_TRAITS_IS_WEAKLY_INCREMENTABLE_H
