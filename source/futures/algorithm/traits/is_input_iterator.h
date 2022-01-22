//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ALGORITHM_TRAITS_IS_INPUT_ITERATOR_H
#define FUTURES_ALGORITHM_TRAITS_IS_INPUT_ITERATOR_H

#include <futures/algorithm/traits/is_input_or_output_iterator.h>
#include <futures/algorithm/traits/is_indirectly_readable.h>
#include <futures/algorithm/traits/has_iterator_traits_value_type.h>
#include <type_traits>

namespace futures {
    /** A C++17 type trait equivalent to the C++20 input_iterator concept
     */
#ifdef FUTURES_DOXYGEN
    template <class T>
    using is_input_iterator = __see_below__;
#else
    template <class T>
    struct is_input_iterator : std::conjunction<
                                   is_input_or_output_iterator<T>,
                                   is_indirectly_readable<T>>
    {};
#endif
    template <class T>
    bool constexpr is_input_iterator_v = is_input_iterator<T>::value;

} // namespace futures

#endif // FUTURES_ALGORITHM_TRAITS_IS_INPUT_ITERATOR_H
