//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ALGORITHM_COMPARISONS_NOT_EQUAL_TO_HPP
#define FUTURES_ALGORITHM_COMPARISONS_NOT_EQUAL_TO_HPP

#include <futures/algorithm/comparisons/equal_to.hpp>
#include <futures/algorithm/traits/is_equality_comparable_with.hpp>
#include <utility>
#include <type_traits>

namespace futures {
    /** A C++17 functor equivalent to the C++20 std::ranges::not_equal_to
     */
    struct not_equal_to {
        template <
            typename T,
            typename U
#ifndef FUTURES_DOXYGEN
            ,
            std::enable_if_t<is_equality_comparable_with_v<T, U>, int> = 0
#endif
            >
        constexpr bool
        operator()(T &&t, U &&u) const {
            return !equal_to{}(std::forward<T>(t), std::forward<U>(u));
        }
        using is_transparent = void;
    };

} // namespace futures

#endif // FUTURES_ALGORITHM_COMPARISONS_NOT_EQUAL_TO_HPP
