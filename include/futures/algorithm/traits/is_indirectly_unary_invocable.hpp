//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ALGORITHM_TRAITS_IS_INDIRECTLY_UNARY_INVOCABLE_HPP
#define FUTURES_ALGORITHM_TRAITS_IS_INDIRECTLY_UNARY_INVOCABLE_HPP

/**
 *  @file algorithm/traits/is_indirectly_unary_invocable.hpp
 *  @brief `is_indirectly_unary_invocable` trait
 *
 *  This file defines the `is_indirectly_unary_invocable` trait.
 */

#include <futures/algorithm/traits/is_convertible_to.hpp>
#include <futures/algorithm/traits/is_indirectly_readable.hpp>
#include <futures/algorithm/traits/iter_value.hpp>
#include <futures/detail/utility/invoke.hpp>
#include <type_traits>

namespace futures {
    /** @addtogroup algorithms Algorithms
     *  @{
     */

    /** @addtogroup traits Traits
     *  @{
     */


    /// @brief A type trait equivalent to the `std::indirectly_unary_invocable`
    /// concept
    /**
     * @see
     * https://en.cppreference.com/w/cpp/iterator/indirectly_unary_invocable
     */
#ifdef FUTURES_DOXYGEN
    template <class F, class I>
    using is_indirectly_unary_invocable = std::bool_constant<
        std::indirectly_unary_invocable<F, I>>;
#else
    template <class F, class I, class = void>
    struct is_indirectly_unary_invocable : std::false_type {};

    template <class F, class I>
    struct is_indirectly_unary_invocable<
        F,
        I,
        std::enable_if_t<
            // clang-format off
            is_indirectly_readable_v<I> &&
            detail::is_copy_constructible_v<F> &&
            detail::is_invocable_v<F&, iter_value_t<I>&>
            // clang-format on
            >> : std::true_type {};
#endif

    /// @copydoc is_indirectly_unary_invocable
    template <class F, class I>
    constexpr bool is_indirectly_unary_invocable_v
        = is_indirectly_unary_invocable<F, I>::value;

    /** @} */
    /** @} */
} // namespace futures

#endif // FUTURES_ALGORITHM_TRAITS_IS_INDIRECTLY_UNARY_INVOCABLE_HPP
