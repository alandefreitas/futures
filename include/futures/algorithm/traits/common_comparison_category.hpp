//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ALGORITHM_TRAITS_COMMON_COMPARISON_CATEGORY_HPP
#define FUTURES_ALGORITHM_TRAITS_COMMON_COMPARISON_CATEGORY_HPP

/**
 *  @file algorithm/traits/common_comparison_category.hpp
 *  @brief `common_comparison_category` trait
 *
 *  This file defines the `common_comparison_category` trait.
 */

#include <futures/config.hpp>
#include <futures/algorithm/compare/partial_ordering.hpp>
#include <futures/algorithm/compare/strong_ordering.hpp>
#include <futures/algorithm/compare/weak_ordering.hpp>
#include <futures/algorithm/traits/is_equality_comparable.hpp>
#include <futures/detail/deps/boost/mp11/algorithm.hpp>
#include <futures/detail/deps/boost/mp11/utility.hpp>
#include <type_traits>

#ifdef __cpp_lib_three_way_comparison
#    include <compare>
#endif

namespace futures {
    /** @addtogroup algorithms Algorithms
     *  @{
     */

    /** @addtogroup traits Traits
     *  @{
     */

    /// @brief A type trait equivalent to the `std::equality_comparable` concept
    /**
     * @see
     * https://en.cppreference.com/w/cpp/utility/compare/three_way_comparable
     */
#if defined(FUTURES_DOXYGEN) || defined(__cpp_lib_three_way_comparison)
    template <class... Ts>
    using common_comparison_category = std::common_comparison_category<Ts...>;
#else
    namespace detail {
        template <class T>
        using is_comparison_category = std::disjunction<
            std::is_same<T, strong_ordering>,
            std::is_same<T, weak_ordering>,
            std::is_same<T, partial_ordering>>;
    } // namespace detail

    template <class... Ts>
    using common_comparison_category = detail::mp_identity<detail::mp_cond<
        detail::mp_any_of_q<
            detail::mp_list<Ts...>,
            detail::mp_not_fn<detail::is_comparison_category>>,
        void,
        detail::mp_contains<detail::mp_list<Ts...>, partial_ordering>,
        partial_ordering,
        detail::mp_contains<detail::mp_list<Ts...>, weak_ordering>,
        weak_ordering,
        detail::mp_contains<detail::mp_list<Ts...>, strong_ordering>,
        strong_ordering,
        std::true_type,
        void>>;
#endif

    /// @copydoc common_comparison_category
    template <class... Ts>
    using common_comparison_category_t = typename common_comparison_category<
        Ts...>::type;

    /** @} */
    /** @} */
} // namespace futures

#endif // FUTURES_ALGORITHM_TRAITS_COMMON_COMPARISON_CATEGORY_HPP
