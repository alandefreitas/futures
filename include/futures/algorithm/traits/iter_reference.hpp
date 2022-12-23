//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ALGORITHM_TRAITS_ITER_REFERENCE_HPP
#define FUTURES_ALGORITHM_TRAITS_ITER_REFERENCE_HPP

/**
 *  @file algorithm/traits/iter_reference.hpp
 *  @brief `iter_reference` trait
 *
 *  This file defines the `iter_reference` trait.
 */

#include <futures/algorithm/traits/iter_value.hpp>
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


    /// @brief A type trait equivalent to `std::iter_reference`
    /**
     * @see https://en.cppreference.com/w/cpp/iterator/iter_t
     */
#ifdef FUTURES_DOXYGEN
    template <class T>
    using iter_reference = std::iter_reference;
#else
    template <class T, class = void>
    struct iter_reference {};

    template <class T>
    struct iter_reference<T, detail::void_t<iter_value_t<T>>> {
        using type = std::add_lvalue_reference<iter_value_t<T>>;
    };
#endif

    /// @copydoc iter_reference
    template <class T>
    using iter_reference_t = typename iter_reference<T>::type;

    /** @} */
    /** @} */
} // namespace futures

#endif // FUTURES_ALGORITHM_TRAITS_ITER_REFERENCE_HPP
