//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ALGORITHM_TRAITS_IS_INCREMENTABLE_HPP
#define FUTURES_ALGORITHM_TRAITS_IS_INCREMENTABLE_HPP

#include <futures/algorithm/traits/is_regular.hpp>
#include <futures/algorithm/traits/is_weakly_incrementable.hpp>
#include <type_traits>

namespace futures {
    /** \addtogroup algorithms Algorithms
     *  @{
     */

    /** \addtogroup traits Traits
     *  @{
     */


    /** \brief A C++17 type trait equivalent to the C++20 incrementable concept
     */
#ifdef FUTURES_DOXYGEN
    template <class I>
    using is_incrementable = __see_below__;
#else
    template <class I, class = void>
    struct is_incrementable : std::false_type
    {};

    template <class I>
    struct is_incrementable<
        I,
        std::void_t<
            // clang-format off
            decltype(std::declval<I>()++)
            // clang-format on
            >>
        : std::conjunction<
              // clang-format off
              is_regular<I>,
              is_weakly_incrementable<I>,
              std::is_same<decltype(std::declval<I>()++), I>
              // clang-format on
              >
    {};
#endif
    template <class I>
    bool constexpr is_incrementable_v = is_incrementable<I>::value;
    /** @}*/
    /** @}*/

} // namespace futures

#endif // FUTURES_ALGORITHM_TRAITS_IS_INCREMENTABLE_HPP
