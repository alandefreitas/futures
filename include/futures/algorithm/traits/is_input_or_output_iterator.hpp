//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ALGORITHM_TRAITS_IS_INPUT_OR_OUTPUT_ITERATOR_HPP
#define FUTURES_ALGORITHM_TRAITS_IS_INPUT_OR_OUTPUT_ITERATOR_HPP

/**
 *  @file algorithm/traits/is_input_or_output_iterator.hpp
 *  @brief `is_input_or_output_iterator` trait
 *
 *  This file defines the `is_input_or_output_iterator` trait.
 */

#include <type_traits>

namespace futures {
    /** @addtogroup algorithms Algorithms
     *  @{
     */

    /** @addtogroup traits Traits
     *  @{
     */

    /// @brief A type trait equivalent to the `std::is_input_or_output_iterator` concept
    /**
     * @see https://en.cppreference.com/w/cpp/iterator/is_input_or_output_iterator
     */
#ifdef FUTURES_DOXYGEN
    template <class T>
    using is_input_or_output_iterator = std::bool_constant<std::is_input_or_output_iterator<T>>;
#else
    template <class T, class = void>
    struct is_input_or_output_iterator : std::false_type {};

    template <class T>
    struct is_input_or_output_iterator<
        T,
        std::void_t<decltype(*std::declval<T>())>> : std::true_type {};
#endif

    /// @copydoc is_input_or_output_iterator
    template <class T>
    constexpr bool is_input_or_output_iterator_v = is_input_or_output_iterator<
        T>::value;

    /** @} */
    /** @} */
} // namespace futures

#endif // FUTURES_ALGORITHM_TRAITS_IS_INPUT_OR_OUTPUT_ITERATOR_HPP
