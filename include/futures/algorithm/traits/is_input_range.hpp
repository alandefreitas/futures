//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ALGORITHM_TRAITS_IS_INPUT_RANGE_HPP
#define FUTURES_ALGORITHM_TRAITS_IS_INPUT_RANGE_HPP

#include <futures/algorithm/traits/is_input_iterator.hpp>
#include <futures/algorithm/traits/is_range.hpp>
#include <futures/algorithm/traits/iterator.hpp>
#include <type_traits>

namespace futures {
    /** @addtogroup algorithms Algorithms
     *  @{
     */

    /** @addtogroup traits Traits
     *  @{
     */


    /** \brief A C++17 type trait equivalent to the C++20 input_range concept
     */
#ifdef FUTURES_DOXYGEN
    template <class T>
    using is_input_range = __see_below__;
#else
    template <class T, class = void>
    struct is_input_range : std::false_type {};

    template <class T>
    struct is_input_range<T, std::void_t<iterator_t<T>>>
        : std::conjunction<
              // clang-format off
              is_range<T>,
              is_input_iterator<iterator_t<T>>
              // clang-format off
            >
    {};
#endif

    /// @copydoc is_input_range
    template <class T>
    bool constexpr is_input_range_v = is_input_range<T>::value;

    /** @} */
    /** @} */
} // namespace futures

#endif // FUTURES_ALGORITHM_TRAITS_IS_INPUT_RANGE_HPP
