//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ALGORITHM_TRAITS_IS_ASSIGNABLE_FROM_HPP
#define FUTURES_ALGORITHM_TRAITS_IS_ASSIGNABLE_FROM_HPP

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


    /** \brief A C++17 type trait equivalent to the C++20 assignable_from
     * concept
     */
#ifdef FUTURES_DOXYGEN
    template <class LHS, class RHS>
    using is_assignable_from = __see_below__;
#else
    template <class LHS, class RHS, class = void>
    struct is_assignable_from : std::false_type {};

    template <class LHS, class RHS>
    struct is_assignable_from<
        LHS,
        RHS,
        std::enable_if_t<
            // clang-format off
            std::is_lvalue_reference_v<LHS> &&
            std::is_same_v<decltype(std::declval<LHS>() = std::declval<RHS&&>()), LHS>
            // clang-format on
            >> : std::true_type {};
#endif
    template <class LHS, class RHS>
    constexpr bool is_assignable_from_v = is_assignable_from<LHS, RHS>::value;
    /** @}*/
    /** @}*/

} // namespace futures

#endif // FUTURES_ALGORITHM_TRAITS_IS_ASSIGNABLE_FROM_HPP
