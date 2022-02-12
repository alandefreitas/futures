//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ALGORITHM_TRAITS_IS_INPUT_OR_OUTPUT_ITERATOR_HPP
#define FUTURES_ALGORITHM_TRAITS_IS_INPUT_OR_OUTPUT_ITERATOR_HPP

#include <type_traits>

namespace futures {
    /** @addtogroup algorithms Algorithms
     *  @{
     */

    /** @addtogroup traits Traits
     *  @{
     */


    /** \brief A C++17 type trait equivalent to the C++20 input_or_output_iterator
     * concept
     */
#ifdef FUTURES_DOXYGEN
    template <class T>
    using is_input_or_output_iterator = __see_below__;
#else
    template <class T, class = void>
    struct is_input_or_output_iterator : std::false_type
    {};

    template <class T>
    struct is_input_or_output_iterator<
        T,
        std::void_t<decltype(*std::declval<T>())>> : std::true_type
    {};
#endif
    template <class T>
    bool constexpr is_input_or_output_iterator_v = is_input_or_output_iterator<
        T>::value;

    /** @}*/
    /** @}*/

} // namespace futures

#endif // FUTURES_ALGORITHM_TRAITS_IS_INPUT_OR_OUTPUT_ITERATOR_HPP
