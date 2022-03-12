//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ALGORITHM_COMPARISONS_GREATER_HPP
#define FUTURES_ALGORITHM_COMPARISONS_GREATER_HPP

#include <futures/algorithm/comparisons/less.hpp>
#include <utility>
#include <type_traits>

namespace futures {
    /** A C++17 functor equivalent to the C++20 std::ranges::greater
     */
    struct greater
    {
        template <
            typename T,
            typename U
#ifndef FUTURES_DOXYGEN
            ,
            std::enable_if_t<is_totally_ordered_with_v<T, U>, int> = 0
#endif
            >
        constexpr bool
        operator()(T &&t, U &&u) const {
            return less{}(std::forward<U>(u), std::forward<T>(t));
        }
        using is_transparent = void;
    };

} // namespace futures

#endif // FUTURES_ALGORITHM_COMPARISONS_GREATER_HPP
