//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ALGORITHM_COMPARE_LESS_EQUAL_HPP
#define FUTURES_ALGORITHM_COMPARE_LESS_EQUAL_HPP

/**
 *  @file algorithm/compare/less_equal.hpp
 *  @brief Less or equal comparison functor
 *
 *  This file defines the less or equal operator as a functor.
 */

#include <futures/algorithm/compare/less.hpp>
#include <utility>
#include <type_traits>

namespace futures {
    /// A C++17 functor equivalent to the C++20 std::ranges::less_equal
    /**
     * @see [`std::less`](https://en.cppreference.com/w/cpp/utility/functional/less)
     */
    struct less_equal {
        FUTURES_TEMPLATE(class T, class U)
        (requires is_totally_ordered_with_v<T, U>) constexpr bool
        operator()(T &&t, U &&u) const {
            return !less{}(std::forward<U>(u), std::forward<T>(t));
        }
        using is_transparent = void;
    };

} // namespace futures

#endif // FUTURES_ALGORITHM_COMPARE_LESS_EQUAL_HPP
