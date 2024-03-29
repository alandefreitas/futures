//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ALGORITHM_TRAITS_IS_INDIRECTLY_BINARY_INVOCABLE_HPP
#define FUTURES_ALGORITHM_TRAITS_IS_INDIRECTLY_BINARY_INVOCABLE_HPP

/**
 *  @file algorithm/traits/is_indirectly_binary_invocable.hpp
 *  @brief `is_indirectly_binary_invocable` trait
 *
 *  This file defines the `is_indirectly_binary_invocable` trait.
 */

#include <futures/algorithm/traits/is_convertible_to.hpp>
#include <futures/algorithm/traits/is_indirectly_readable.hpp>
#include <futures/algorithm/traits/iter_value.hpp>
#include <futures/detail/traits/std_type_traits.hpp>
#include <futures/detail/utility/invoke.hpp>
#include <type_traits>

namespace futures {
    /** @addtogroup algorithms Algorithms
     *  @{
     */

    /** @addtogroup traits Traits
     *  @{
     */


    /// @brief Determine if a function can be invoke with the value type of both
    /// iterators
#ifdef FUTURES_DOXYGEN
    template <class F, class I1, class I2>
    using is_indirectly_binary_invocable = __see_below__;
#else
    template <class F, class I1, class I2, class = void>
    struct is_indirectly_binary_invocable : std::false_type {};

    template <class F, class I1, class I2>
    struct is_indirectly_binary_invocable<
        F,
        I1,
        I2,
        std::enable_if_t<
            // clang-format off
            is_indirectly_readable_v<I1> &&
            is_indirectly_readable_v<I2> &&
            detail::is_copy_constructible_v<F> &&
            detail::is_invocable_v<F&, iter_value_t<I1>&, iter_value_t<I2>&>
            // clang-format on
            >> : std::true_type {};
#endif

    /// @copydoc is_indirectly_binary_invocable
    template <class F, class I1, class I2>
    constexpr bool is_indirectly_binary_invocable_v
        = is_indirectly_binary_invocable<F, I1, I2>::value;

    /** @} */
    /** @} */

} // namespace futures

#endif // FUTURES_ALGORITHM_TRAITS_IS_INDIRECTLY_BINARY_INVOCABLE_HPP
