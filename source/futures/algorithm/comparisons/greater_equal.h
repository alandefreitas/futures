//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ALGORITHM_COMPARISONS_GREATER_EQUAL_H
#define FUTURES_ALGORITHM_COMPARISONS_GREATER_EQUAL_H

#include <futures/algorithm/comparisons/less.h>
#include <type_traits>
#include <utility>

namespace futures {
    /** A C++17 functor equivalent to the C++20 std::ranges::greater_equal
     */
    struct greater_equal
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
            return !less{}(std::forward<T>(t), std::forward<U>(u));
        }
        using is_transparent = void;
    };

} // namespace futures

#endif // FUTURES_ALGORITHM_COMPARISONS_GREATER_EQUAL_H
