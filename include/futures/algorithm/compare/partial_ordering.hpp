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
#if FUTURES_DOXYGEN || __cpp_lib_three_way_comparison
    /// the result type of 3-way comparison that supports all 6 operators, is
    /// not substitutable, and allows incomparable values
    using partial_ordering = std::partial_ordering;

#    ifndef FUTURES_DOXYGEN
    [[nodiscard]] constexpr bool
    is_eq(partial_ordering v) noexcept {
        return std::is_eq(v);
    }

    [[nodiscard]] constexpr bool
    is_neq(partial_ordering v) noexcept {
        return std::is_neq(v);
    }

    [[nodiscard]] constexpr bool
    is_lt(partial_ordering v) noexcept {
        return std::is_lt(v);
    }

    [[nodiscard]] constexpr bool
    is_lteq(partial_ordering v) noexcept {
        return std::is_lteq(v);
    }

    [[nodiscard]] constexpr bool
    is_gt(partial_ordering v) noexcept {
        return std::is_gt(v);
    }

    [[nodiscard]] constexpr bool
    is_gteq(partial_ordering v) noexcept {
        return std::is_gteq(v);
    }
#    endif
#else
    class partial_ordering {
        struct unspecified {
            constexpr unspecified(unspecified*) noexcept {}
        };

        signed char v_;

        constexpr explicit partial_ordering(signed char v) noexcept
            : v_(v) {}

        friend class weak_ordering;
        friend class strong_ordering;

    public:
        static const partial_ordering less;
        static const partial_ordering equivalent;
        static const partial_ordering greater;
        static const partial_ordering unordered;

        [[nodiscard]] friend constexpr bool
        operator==(partial_ordering lhs, unspecified) noexcept {
            return lhs.v_ == 0;
        }

        [[nodiscard]] friend constexpr bool
        operator==(partial_ordering lhs, partial_ordering rhs) noexcept {
            return lhs.v_ == rhs.v_;
        };

        [[nodiscard]] friend constexpr bool
        operator<(partial_ordering lhs, unspecified) noexcept {
            return lhs.v_ == -1;
        }

        [[nodiscard]] friend constexpr bool
        operator>(partial_ordering lhs, unspecified) noexcept {
            return lhs.v_ == 1;
        }

        [[nodiscard]] friend constexpr bool
        operator<=(partial_ordering lhs, unspecified) noexcept {
            return lhs.v_ <= 0;
        }

        [[nodiscard]] friend constexpr bool
        operator>=(partial_ordering lhs, unspecified) noexcept {
            return (lhs.v_ & 1) == lhs.v_;
        }

        [[nodiscard]] friend constexpr bool
        operator<(unspecified, partial_ordering rhs) noexcept {
            return rhs.v_ == 1;
        }

        [[nodiscard]] friend constexpr bool
        operator>(unspecified, partial_ordering rhs) noexcept {
            return rhs.v_ == -1;
        }

        [[nodiscard]] friend constexpr bool
        operator<=(unspecified, partial_ordering rhs) noexcept {
            return (rhs.v_ & 1) == rhs.v_;
        }

        [[nodiscard]] friend constexpr bool
        operator>=(unspecified, partial_ordering rhs) noexcept {
            return 0 >= rhs.v_;
        }

#    if __cplusplus > 201703L && defined(__cpp_impl_three_way_comparison) \
        && __has_include(<compare>)
        [[nodiscard]] friend constexpr partial_ordering
        operator<=>(partial_ordering lhs, unspecified) noexcept {
            return lhs;
        }

        [[nodiscard]] friend constexpr partial_ordering
        operator<=>(unspecified, partial_ordering rhs) noexcept {
            if (rhs.v_ & 1) {
                return partial_ordering(-rhs.v_);
            } else {
                return rhs;
            }
        }
#    endif
    };

    inline constexpr partial_ordering partial_ordering::less(-1);

    inline constexpr partial_ordering partial_ordering::equivalent(0);

    inline constexpr partial_ordering partial_ordering::greater(1);

    inline constexpr partial_ordering partial_ordering::unordered(2);

    [[nodiscard]] constexpr bool
    is_eq(partial_ordering v) noexcept {
        return v == 0;
    }

    [[nodiscard]] constexpr bool
    is_neq(partial_ordering v) noexcept {
        return !(v == 0);
    }

    [[nodiscard]] constexpr bool
    is_lt(partial_ordering v) noexcept {
        return v < 0;
    }

    [[nodiscard]] constexpr bool
    is_lteq(partial_ordering v) noexcept {
        return v <= 0;
    }

    [[nodiscard]] constexpr bool
    is_gt(partial_ordering v) noexcept {
        return v > 0;
    }

    [[nodiscard]] constexpr bool
    is_gteq(partial_ordering v) noexcept {
        return v >= 0;
    }
#endif

} // namespace futures

#endif // FUTURES_ALGORITHM_COMPARE_PARTIAL_ORDERING_HPP
