//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ALGORITHM_COMPARE_WEAK_ORDERING_HPP
#define FUTURES_ALGORITHM_COMPARE_WEAK_ORDERING_HPP

/**
 *  @file algorithm/compare/partial_ordering.hpp
 *  @brief Result of partial ordering comparison
 *
 *  This file defines the partial ordering type.
 */

#include <futures/config.hpp>
#include <futures/algorithm/compare/partial_ordering.hpp>

#ifdef __cpp_lib_three_way_comparison
#    include <compare>
#endif

namespace futures {
#if defined(FUTURES_DOXYGEN) || defined(__cpp_lib_three_way_comparison)
    /// the result type of 3-way comparison that supports all 6 operators, does
    // not imply substitutability, and does not allow incomparable values
    using weak_ordering = std::weak_ordering;
#else
    class weak_ordering;
#endif
} // namespace futures

#include <futures/algorithm/compare/impl/weak_ordering.hpp>

#endif // FUTURES_ALGORITHM_COMPARE_WEAK_ORDERING_HPP
