//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ALGORITHM_COMPARE_GREATER_HPP
#define FUTURES_ALGORITHM_COMPARE_GREATER_HPP

/**
 *  @file algorithm/compare/greater.hpp
 *  @brief Greater comparison functor
 *
 *  This file defines the greater operator as a functor.
 */

#include <futures/algorithm/compare/less.hpp>
#include <utility>
#include <type_traits>

namespace futures {
    /// A C++17 functor equivalent to the C++20 std::ranges::greater
    /**
     * @see https://en.cppreference.com/w/cpp/utility/functional/greater
     */
    struct greater {
        FUTURES_TEMPLATE(class T, class U)
        (requires is_totally_ordered_with_v<T, U>) constexpr bool
        operator()(T &&t, U &&u) const {
            return less{}(std::forward<U>(u), std::forward<T>(t));
        }
        using is_transparent = void;
    };

} // namespace futures

#endif // FUTURES_ALGORITHM_COMPARE_GREATER_HPP
