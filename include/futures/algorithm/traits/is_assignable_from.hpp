//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ALGORITHM_TRAITS_IS_ASSIGNABLE_FROM_HPP
#define FUTURES_ALGORITHM_TRAITS_IS_ASSIGNABLE_FROM_HPP

/**
 *  @file algorithm/traits/is_assignable_from.hpp
 *  @brief `is_assignable_from` trait
 *
 *  This file defines the `is_assignable_from` trait.
 */

#include <futures/algorithm/traits/is_constructible_from.hpp>
#include <futures/algorithm/traits/is_convertible_to.hpp>
#include <type_traits>

namespace futures {
    /** @addtogroup algorithms Algorithms
     *  @{
     */

    /** @addtogroup traits Traits
     *  @{
     */


    /// @brief A type trait equivalent to the `std::assignable_from` concept
    /**
     * @see [`std::assignable_from`](https://en.cppreference.com/w/cpp/concepts/assignable_from)
     */
#ifdef FUTURES_DOXYGEN
    template <class LHS, class RHS>
    using is_assignable_from = std::bool_constant<
        std::assignable_from<LHS, RHS>>;
#else
    template <class LHS, class RHS, class = void>
    struct is_assignable_from : std::false_type {};

    template <class LHS, class RHS>
    struct is_assignable_from<
        LHS,
        RHS,
        std::enable_if_t<
            // clang-format off
            detail::is_lvalue_reference_v<LHS> &&
            detail::is_same_v<decltype(std::declval<LHS>() = std::declval<RHS&&>()), LHS>
            // clang-format on
            >> : std::true_type {};
#endif

    /// @copydoc is_assignable_from
    template <class LHS, class RHS>
    constexpr bool is_assignable_from_v = is_assignable_from<LHS, RHS>::value;
    /** @} */
    /** @} */

} // namespace futures

#endif // FUTURES_ALGORITHM_TRAITS_IS_ASSIGNABLE_FROM_HPP
