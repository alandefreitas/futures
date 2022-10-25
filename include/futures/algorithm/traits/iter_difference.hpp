//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ALGORITHM_TRAITS_ITER_DIFFERENCE_HPP
#define FUTURES_ALGORITHM_TRAITS_ITER_DIFFERENCE_HPP

/**
 *  @file algorithm/traits/iter_difference.hpp
 *  @brief `iter_difference` trait
 *
 *  This file defines the `iter_difference` trait.
 */

#include <futures/algorithm/traits/remove_cvref.hpp>
#include <futures/algorithm/traits/detail/has_difference_type.hpp>
#include <futures/algorithm/traits/detail/has_iterator_traits_difference_type.hpp>
#include <futures/algorithm/traits/detail/is_subtractable.hpp>
#include <futures/algorithm/traits/detail/nested_iterator_traits_difference_type.hpp>
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


    /// @brief A type trait equivalent to `std::iter_difference`
    /**
     * @see https://en.cppreference.com/w/cpp/iterator/iter_t
     */
#ifdef FUTURES_DOXYGEN
    template <class T>
    using iter_difference = __see_below__;
#else
    namespace detail {
        template <class T>
        using signed_subtract_invoke_result_t = std::make_signed_t<
            decltype(std::declval<T>() - std::declval<T>())>;
    } // namespace detail

    template <class T>
    using iter_difference = detail::mp_cond<
        detail::has_iterator_traits_difference_type<remove_cvref_t<T>>,
        detail::mp_defer<
            detail::nested_iterator_traits_difference_type_t,
            remove_cvref_t<T>>,
        std::is_pointer<T>,
        detail::mp_identity<std::ptrdiff_t>,
        std::is_pointer<std::remove_const_t<T>>,
        detail::mp_identity<std::ptrdiff_t>,
        detail::is_subtractable<remove_cvref_t<T>>,
        detail::mp_defer<detail::signed_subtract_invoke_result_t, T>>;
#endif

    /// @copydoc iter_difference
    template <class T>
    using iter_difference_t = typename iter_difference<T>::type;

    /** @} */
    /** @} */
} // namespace futures

#endif // FUTURES_ALGORITHM_TRAITS_ITER_DIFFERENCE_HPP
