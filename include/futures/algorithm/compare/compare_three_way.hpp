//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ALGORITHM_COMPARE_COMPARE_THREE_WAY_HPP
#define FUTURES_ALGORITHM_COMPARE_COMPARE_THREE_WAY_HPP

/**
 *  @file algorithm/compare/compare_three_way.hpp
 *  @brief Spaceship comparison functor
 *
 *  This file defines the spaceship comparison as a functor.
 */

#include <futures/config.hpp>
#include <futures/algorithm/compare/less.hpp>
#include <futures/algorithm/compare/partial_ordering.hpp>
#include <futures/algorithm/traits/is_three_way_comparable_with.hpp>
#include <utility>
#include <type_traits>

#ifdef __cpp_lib_three_way_comparison
#    include <compare>
#endif


namespace futures {
    /// Function object for performing comparisons
    /**
     * This class defines functor equivalent to the C++20
     * `std::ranges::compare_three_way`. If C++20 is available, it represents
     * an alias to `std::ranges::compare_three_way`.
     *
     * @see
     * [`std::compare_three_way`](https://en.cppreference.com/w/cpp/utility/compare/compare_three_way)
     */
#if __cplusplus > 201703L && __cpp_lib_three_way_comparison
    using compare_three_way = std::compare_three_way;
#else
    struct compare_three_way {
        FUTURES_TEMPLATE(class T, class U)
        (requires is_three_way_comparable_with_v<T, U>) constexpr partial_ordering
        operator()(T &&t, U &&u) const {
            // Spaceship operator is not available, so result is always
            // partial_ordering, which is the what the synthesized <=>
            // operator would return.
            return less{}(std::forward<T>(t), std::forward<U>(u)) ?
                       partial_ordering::less :
                   less{}(std::forward<T>(u), std::forward<U>(t)) ?
                       partial_ordering::greater :
                       partial_ordering::equivalent;
        }
        using is_transparent = void;
    };
#endif

} // namespace futures

#endif // FUTURES_ALGORITHM_COMPARE_COMPARE_THREE_WAY_HPP
