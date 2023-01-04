//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ALGORITHM_COMPARE_NOT_EQUAL_TO_HPP
#define FUTURES_ALGORITHM_COMPARE_NOT_EQUAL_TO_HPP

/**
 *  @file algorithm/compare/not_equal_to.hpp
 *  @brief Not equal comparison functor
 *
 *  This file defines the not equal operator as a functor.
 */

#include <futures/algorithm/compare/equal_to.hpp>
#include <futures/algorithm/traits/is_equality_comparable_with.hpp>
#include <utility>
#include <type_traits>

namespace futures {
    /// A C++17 functor equivalent to the C++20 std::ranges::not_equal_to
    /**
     * @see [`std::not_equal_to`](https://en.cppreference.com/w/cpp/utility/functional/not_equal_to)
     */
    struct not_equal_to {
        FUTURES_TEMPLATE(class T, class U)
        (requires is_equality_comparable_with_v<T, U>) constexpr bool
        operator()(T &&t, U &&u) const {
            return !equal_to{}(std::forward<T>(t), std::forward<U>(u));
        }
        using is_transparent = void;
    };

} // namespace futures

#endif // FUTURES_ALGORITHM_COMPARE_NOT_EQUAL_TO_HPP
