//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ALGORITHM_COMPARISONS_COMPARE_THREE_WAY_HPP
#define FUTURES_ALGORITHM_COMPARISONS_COMPARE_THREE_WAY_HPP

/**
 *  @file algorithm/comparisons/compare_three_way.hpp
 *  @brief Spaceship comparison functor
 *
 *  This file defines the spaceship comparison as a functor.
 */

#include <futures/algorithm/comparisons/less.hpp>
#include <utility>
#include <type_traits>

namespace futures {
    /// A C++17 functor equivalent to the C++20 std::ranges::compare_three_way
    /**
     * @see https://en.cppreference.com/w/cpp/utility/compare/compare_three_way
     */
    struct compare_three_way {
        template <
            class T,
            class U
#ifndef FUTURES_DOXYGEN
            ,
            std::enable_if_t<is_totally_ordered_with_v<T, U>, int> = 0
#endif
            >
        constexpr auto
        operator()(T &&t, U &&u) const {
#if __cplusplus > 201703L && defined(__cpp_impl_three_way_comparison) \
    && __has_include(<compare>)
            return std::forward<T>(t) <=> std::forward<U>(u);
#else
            return less{}(std::forward<T>(t), std::forward<U>(u)) ?
                       -1 :
                   less{}(std::forward<T>(u), std::forward<U>(t)) ?
                       +1 :
                       0;
#endif
        }
        using is_transparent = void;
    };

} // namespace futures

#endif // FUTURES_ALGORITHM_COMPARISONS_COMPARE_THREE_WAY_HPP
