//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ALGORITHM_COMPARE_LESS_HPP
#define FUTURES_ALGORITHM_COMPARE_LESS_HPP

/**
 *  @file algorithm/compare/less.hpp
 *  @brief Less comparison functor
 *
 *  This file defines the less operator as a functor.
 */

#include <futures/config.hpp>
#include <futures/algorithm/traits/is_totally_ordered_with.hpp>
#include <utility>
#include <type_traits>

namespace futures {
    /// A C++17 functor equivalent to the C++20 std::ranges::less
    /**
     * @see https://en.cppreference.com/w/cpp/utility/functional/less
     */
    struct less {
        FUTURES_TEMPLATE(class T, class U)
        (requires is_totally_ordered_with_v<T, U>) constexpr bool
        operator()(T &&t, U &&u) const {
            return std::forward<T>(t) < std::forward<U>(u);
        }
        using is_transparent = void;
    };

} // namespace futures

#endif // FUTURES_ALGORITHM_COMPARE_LESS_HPP
