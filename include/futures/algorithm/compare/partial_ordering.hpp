//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ALGORITHM_COMPARE_PARTIAL_ORDERING_HPP
#define FUTURES_ALGORITHM_COMPARE_PARTIAL_ORDERING_HPP

/**
 *  @file algorithm/compare/partial_ordering.hpp
 *  @brief Result of partial ordering comparison
 *
 *  This file defines the partial ordering type.
 */

#include <futures/config.hpp>

#ifdef __cpp_lib_three_way_comparison
#    include <compare>
#endif

namespace futures {
#if defined(FUTURES_DOXYGEN) || defined(__cpp_lib_three_way_comparison)
    /// the result type of 3-way comparison that supports all 6 operators,
    // does not imply substitutability, and admits incomparable values
    using partial_ordering = std::partial_ordering;

#    ifndef FUTURES_DOXYGEN
    FUTURES_NODISCARD constexpr bool
    is_eq(partial_ordering v) noexcept {
        return std::is_eq(v);
    }

    FUTURES_NODISCARD constexpr bool
    is_neq(partial_ordering v) noexcept {
        return std::is_neq(v);
    }

    FUTURES_NODISCARD constexpr bool
    is_lt(partial_ordering v) noexcept {
        return std::is_lt(v);
    }

    FUTURES_NODISCARD constexpr bool
    is_lteq(partial_ordering v) noexcept {
        return std::is_lteq(v);
    }

    FUTURES_NODISCARD constexpr bool
    is_gt(partial_ordering v) noexcept {
        return std::is_gt(v);
    }

    FUTURES_NODISCARD constexpr bool
    is_gteq(partial_ordering v) noexcept {
        return std::is_gteq(v);
    }
#    endif
#else
    class partial_ordering;
#endif
} // namespace futures

#include <futures/algorithm/compare/impl/partial_ordering.hpp>

#endif // FUTURES_ALGORITHM_COMPARE_PARTIAL_ORDERING_HPP
