//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ALGORITHM_TRAITS_IS_INPUT_ITERATOR_HPP
#define FUTURES_ALGORITHM_TRAITS_IS_INPUT_ITERATOR_HPP

#include <futures/algorithm/traits/has_iterator_traits_value_type.hpp>
#include <futures/algorithm/traits/is_indirectly_readable.hpp>
#include <futures/algorithm/traits/is_input_or_output_iterator.hpp>
#include <type_traits>

namespace futures {
    /** @addtogroup algorithms Algorithms
     *  @{
     */

    /** @addtogroup traits Traits
     *  @{
     */


    /** \brief A C++17 type trait equivalent to the C++20 input_iterator concept
     */
#ifdef FUTURES_DOXYGEN
    template <class T>
    using is_input_iterator = __see_below__;
#else
    template <class T>
    struct is_input_iterator
        : std::conjunction<
              is_input_or_output_iterator<T>,
              is_indirectly_readable<T>> {};
#endif
    template <class T>
    constexpr bool is_input_iterator_v = is_input_iterator<T>::value;
    /** @}*/
    /** @}*/

} // namespace futures

#endif // FUTURES_ALGORITHM_TRAITS_IS_INPUT_ITERATOR_HPP
