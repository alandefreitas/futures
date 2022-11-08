//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ALGORITHM_TRAITS_ITER_VALUE_HPP
#define FUTURES_ALGORITHM_TRAITS_ITER_VALUE_HPP

/**
 *  @file algorithm/traits/iter_value.hpp
 *  @brief `iter_value` trait
 *
 *  This file defines the `iter_value` trait.
 */

#include <futures/algorithm/traits/remove_cvref.hpp>
#include <futures/algorithm/traits/detail/has_element_type.hpp>
#include <futures/algorithm/traits/detail/has_iterator_traits_value_type.hpp>
#include <futures/algorithm/traits/detail/has_value_type.hpp>
#include <futures/detail/deps/boost/mp11/utility.hpp>
#include <iterator>
#include <type_traits>

namespace futures {
    /** @addtogroup algorithms Algorithms
     *  @{
     */

    /** @addtogroup traits Traits
     *  @{
     */

    /// @brief A type trait equivalent to `std::iter_value`
    /**
     * @see https://en.cppreference.com/w/cpp/iterator/iter_t
     */
#ifdef FUTURES_DOXYGEN
    template <class T>
    using iter_value = std::iter_value;
#else
    template <class T>
    using iter_value = detail::mp_cond<
        detail::has_iterator_traits_value_type<remove_cvref_t<T>>,
        detail::mp_defer<
            detail::nested_iterator_traits_value_type,
            remove_cvref_t<T>>,
        std::is_pointer<T>,
        std::remove_cv_t<T>,
        std::is_array<T>,
        std::remove_cv<std::remove_extent_t<T>>,
        detail::has_value_type<T>,
        detail::mp_defer<detail::nested_value_type_t, T>,
        detail::has_element_type<T>,
        detail::mp_defer<detail::nested_element_type_t, T>>;
#endif

    /// @copydoc iter_value
    template <class T>
    using iter_value_t = typename iter_value<T>::type;

    /** @} */
    /** @} */
} // namespace futures

#endif // FUTURES_ALGORITHM_TRAITS_ITER_VALUE_HPP
