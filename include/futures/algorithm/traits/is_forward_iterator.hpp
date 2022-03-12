//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ALGORITHM_TRAITS_IS_FORWARD_ITERATOR_HPP
#define FUTURES_ALGORITHM_TRAITS_IS_FORWARD_ITERATOR_HPP

#include <futures/algorithm/traits/is_derived_from.hpp>
#include <futures/algorithm/traits/is_incrementable.hpp>
#include <futures/algorithm/traits/is_input_iterator.hpp>
#include <futures/algorithm/traits/is_sentinel_for.hpp>
#include <futures/algorithm/traits/iter_concept.hpp>
#include <type_traits>

namespace futures {
    /** @addtogroup algorithms Algorithms
     *  @{
     */

    /** @addtogroup traits Traits
     *  @{
     */


    /** \brief A C++17 type trait equivalent to the C++20 forward_iterator
     * concept
     */
#ifdef FUTURES_DOXYGEN
    template <class I>
    using is_forward_iterator = __see_below__;
#else
    template <class I, class = void>
    struct is_forward_iterator : std::false_type
    {};

    template <class I>
    struct is_forward_iterator<
        I,
        std::enable_if_t<
            // clang-format off
            is_input_iterator_v<I> &&
            is_derived_from_v<iter_concept_t<I>, std::forward_iterator_tag> &&
            is_incrementable_v<I> &&
            is_sentinel_for_v<I, I>
            // clang-format on
            >> : std::true_type
    {};
#endif
    template <class I>
    bool constexpr is_forward_iterator_v = is_forward_iterator<I>::value;
    /** @}*/
    /** @}*/

} // namespace futures

#endif // FUTURES_ALGORITHM_TRAITS_IS_FORWARD_ITERATOR_HPP
