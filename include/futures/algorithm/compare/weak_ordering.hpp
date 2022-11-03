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

namespace futures {
#if FUTURES_DOXYGEN || __cpp_lib_three_way_comparison
    /// the result type of 3-way comparison that supports all 6 operators and is
    /// not substitutable
    using weak_ordering = std::weak_ordering;
#else
    class weak_ordering {
        using unspecified = partial_ordering::unspecified;

        signed char v_;

        constexpr explicit weak_ordering(signed char v) noexcept : v_(v) {}

        friend class strong_ordering;

    public:
        static const weak_ordering less;
        static const weak_ordering equivalent;
        static const weak_ordering greater;

        [[nodiscard]] constexpr operator partial_ordering() const noexcept {
            return partial_ordering(v_);
        }

        // comparisons
        [[nodiscard]] friend constexpr bool
        operator==(weak_ordering lhs, unspecified) noexcept {
            return lhs.v_ == 0;
        }

        [[nodiscard]] friend constexpr bool
        operator==(weak_ordering lhs, weak_ordering rhs) noexcept {
            return lhs.v_ == rhs.v_;
        }

        [[nodiscard]] friend constexpr bool
        operator<(weak_ordering lhs, unspecified) noexcept {
            return lhs.v_ < 0;
        }

        [[nodiscard]] friend constexpr bool
        operator>(weak_ordering lhs, unspecified) noexcept {
            return lhs.v_ > 0;
        }

        [[nodiscard]] friend constexpr bool
        operator<=(weak_ordering lhs, unspecified) noexcept {
            return lhs.v_ <= 0;
        }

        [[nodiscard]] friend constexpr bool
        operator>=(weak_ordering lhs, unspecified) noexcept {
            return lhs.v_ >= 0;
        }

        [[nodiscard]] friend constexpr bool
        operator<(unspecified, weak_ordering lhs) noexcept {
            return 0 < lhs.v_;
        }

        [[nodiscard]] friend constexpr bool
        operator>(unspecified, weak_ordering lhs) noexcept {
            return 0 > lhs.v_;
        }

        [[nodiscard]] friend constexpr bool
        operator<=(unspecified, weak_ordering lhs) noexcept {
            return 0 <= lhs.v_;
        }

        [[nodiscard]] friend constexpr bool
        operator>=(unspecified, weak_ordering lhs) noexcept {
            return 0 >= lhs.v_;
        }

#    if __cplusplus > 201703L && defined(__cpp_impl_three_way_comparison) \
        && __has_include(<compare>)
        [[nodiscard]] friend constexpr weak_ordering
        operator<=>(weak_ordering lhs, unspecified) noexcept {
            return lhs;
        }

        [[nodiscard]] friend constexpr weak_ordering
        operator<=>(unspecified, weak_ordering rhs) noexcept {
            return weak_ordering(-rhs.v_);
        }
#    endif
    };

    // valid values' definitions
    inline constexpr weak_ordering weak_ordering::less(-1);

    inline constexpr weak_ordering weak_ordering::equivalent(0);

    inline constexpr weak_ordering weak_ordering::greater(1);
#endif

} // namespace futures

#endif // FUTURES_ALGORITHM_COMPARE_WEAK_ORDERING_HPP
