//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ALGORITHM_COMPARE_STRONG_ORDERING_HPP
#define FUTURES_ALGORITHM_COMPARE_STRONG_ORDERING_HPP

/**
 *  @file algorithm/compare/partial_ordering.hpp
 *  @brief Result of partial ordering comparison
 *
 *  This file defines the partial ordering type.
 */

#include <futures/config.hpp>
#include <futures/algorithm/compare/partial_ordering.hpp>
#include <futures/algorithm/compare/weak_ordering.hpp>

namespace futures {
#if FUTURES_DOXYGEN || __cpp_lib_three_way_comparison
    /// the result type of 3-way comparison that supports all 6 operators and is
    /// substitutable
    using strong_ordering = std::strong_ordering;
#else
    class strong_ordering {
        using unspecified = partial_ordering::unspecified;

        signed char v_;

        constexpr explicit strong_ordering(signed char v) noexcept
            : v_(v) {}

    public:
        // valid values
        static const strong_ordering less;
        static const strong_ordering equal;
        static const strong_ordering equivalent;
        static const strong_ordering greater;

        [[nodiscard]] constexpr operator partial_ordering() const noexcept {
            return partial_ordering(v_);
        }

        [[nodiscard]] constexpr operator weak_ordering() const noexcept {
            return weak_ordering(v_);
        }

        // comparisons
        [[nodiscard]] friend constexpr bool
        operator==(strong_ordering lhs, unspecified) noexcept {
            return lhs.v_ == 0;
        }

        [[nodiscard]] friend constexpr bool
        operator==(strong_ordering lhs, strong_ordering rhs) noexcept {
            return lhs.v_ == rhs.v_;
        }

        [[nodiscard]] friend constexpr bool
        operator<(strong_ordering lhs, unspecified) noexcept {
            return lhs.v_ < 0;
        }

        [[nodiscard]] friend constexpr bool
        operator>(strong_ordering lhs, unspecified) noexcept {
            return lhs.v_ > 0;
        }

        [[nodiscard]] friend constexpr bool
        operator<=(strong_ordering lhs, unspecified) noexcept {
            return lhs.v_ <= 0;
        }

        [[nodiscard]] friend constexpr bool
        operator>=(strong_ordering lhs, unspecified) noexcept {
            return lhs.v_ >= 0;
        }

        [[nodiscard]] friend constexpr bool
        operator<(unspecified, strong_ordering rhs) noexcept {
            return 0 < rhs.v_;
        }

        [[nodiscard]] friend constexpr bool
        operator>(unspecified, strong_ordering rhs) noexcept {
            return 0 > rhs.v_;
        }

        [[nodiscard]] friend constexpr bool
        operator<=(unspecified, strong_ordering rhs) noexcept {
            return 0 <= rhs.v_;
        }

        [[nodiscard]] friend constexpr bool
        operator>=(unspecified, strong_ordering rhs) noexcept {
            return 0 >= rhs.v_;
        }

#    if __cplusplus > 201703L && defined(__cpp_impl_three_way_comparison) \
        && __has_include(<compare>)
        [[nodiscard]] friend constexpr strong_ordering
        operator<=>(strong_ordering lhs, unspecified) noexcept {
            return lhs;
        }

        [[nodiscard]] friend constexpr strong_ordering
        operator<=>(unspecified, strong_ordering lhs) noexcept {
            return strong_ordering(-lhs.v_);
        }
#    endif
    };

    inline constexpr strong_ordering strong_ordering::less(-1);

    inline constexpr strong_ordering strong_ordering::equal(0);

    inline constexpr strong_ordering strong_ordering::equivalent(0);

    inline constexpr strong_ordering strong_ordering::greater(1);

#endif

} // namespace futures

#endif // FUTURES_ALGORITHM_COMPARE_STRONG_ORDERING_HPP
