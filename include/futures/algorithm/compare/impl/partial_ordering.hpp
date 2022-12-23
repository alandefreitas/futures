//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ALGORITHM_COMPARE_IMPL_PARTIAL_ORDERING_HPP
#define FUTURES_ALGORITHM_COMPARE_IMPL_PARTIAL_ORDERING_HPP

/**
 *  @file algorithm/compare/partial_ordering.hpp
 *  @brief Result of partial ordering comparison
 *
 *  This file defines the partial ordering type.
 */

#if !defined(FUTURES_DOXYGEN) && !defined(__cpp_lib_three_way_comparison)
namespace futures {
#    ifndef __cpp_inline_variables
    namespace detail {
        // emulate inline variables in C++14 with templates that can be
        // initialized inline
        template <typename T>
        struct partial_ordering_base {
            FUTURES_CONST_INIT static const T less;
            FUTURES_CONST_INIT static const T equivalent;
            FUTURES_CONST_INIT static const T greater;
            FUTURES_CONST_INIT static const T unordered;
        };
    } // namespace detail
#    endif

    class partial_ordering
#    ifndef __cpp_inline_variables
        : public detail::partial_ordering_base<partial_ordering>
#    endif
    {
#    ifndef __cpp_inline_variables
        friend detail::partial_ordering_base<partial_ordering>;
#    endif

        struct unspecified {
            constexpr unspecified(unspecified*) noexcept {}
        };

        signed char v_;

        constexpr explicit partial_ordering(signed char v) noexcept : v_(v) {}

        friend class weak_ordering;
        friend class strong_ordering;

    public:
#    ifdef __cpp_inline_variables
        static const partial_ordering less;
        static const partial_ordering equivalent;
        static const partial_ordering greater;
        static const partial_ordering unordered;
#    endif

        FUTURES_NODISCARD friend constexpr bool
        operator==(partial_ordering lhs, unspecified) noexcept {
            return lhs.v_ == 0;
        }

        FUTURES_NODISCARD friend constexpr bool
        operator==(partial_ordering lhs, partial_ordering rhs) noexcept {
            return lhs.v_ == rhs.v_;
        };

        FUTURES_NODISCARD friend constexpr bool
        operator<(partial_ordering lhs, unspecified) noexcept {
            return lhs.v_ == -1;
        }

        FUTURES_NODISCARD friend constexpr bool
        operator>(partial_ordering lhs, unspecified) noexcept {
            return lhs.v_ == 1;
        }

        FUTURES_NODISCARD friend constexpr bool
        operator<=(partial_ordering lhs, unspecified) noexcept {
            return lhs.v_ <= 0;
        }

        FUTURES_NODISCARD friend constexpr bool
        operator>=(partial_ordering lhs, unspecified) noexcept {
            return (lhs.v_ & 1) == lhs.v_;
        }

        FUTURES_NODISCARD friend constexpr bool
        operator<(unspecified, partial_ordering rhs) noexcept {
            return rhs.v_ == 1;
        }

        FUTURES_NODISCARD friend constexpr bool
        operator>(unspecified, partial_ordering rhs) noexcept {
            return rhs.v_ == -1;
        }

        FUTURES_NODISCARD friend constexpr bool
        operator<=(unspecified, partial_ordering rhs) noexcept {
            return (rhs.v_ & 1) == rhs.v_;
        }

        FUTURES_NODISCARD friend constexpr bool
        operator>=(unspecified, partial_ordering rhs) noexcept {
            return 0 >= rhs.v_;
        }

#    if __cplusplus > 201703L && defined(__cpp_impl_three_way_comparison) \
        && __has_include(<compare>)
        FUTURES_NODISCARD friend constexpr partial_ordering
        operator<=>(partial_ordering lhs, unspecified) noexcept {
            return lhs;
        }

        FUTURES_NODISCARD friend constexpr partial_ordering
        operator<=>(unspecified, partial_ordering rhs) noexcept {
            if (rhs.v_ & 1) {
                return partial_ordering(-rhs.v_);
            } else {
                return rhs;
            }
        }
#    endif
    };

#    if defined(__cpp_inline_variables)
    inline constexpr partial_ordering partial_ordering::less(-1);
    inline constexpr partial_ordering partial_ordering::equivalent(0);
    inline constexpr partial_ordering partial_ordering::greater(1);
    inline constexpr partial_ordering partial_ordering::unordered(2);
#    else
    template <typename T>
    const T detail::partial_ordering_base<T>::less(-1);

    template <typename T>
    const T detail::partial_ordering_base<T>::equivalent(0);

    template <typename T>
    const T detail::partial_ordering_base<T>::greater(1);

    template <typename T>
    const T detail::partial_ordering_base<T>::unordered(1);
#    endif

    FUTURES_NODISCARD constexpr bool
    is_eq(partial_ordering v) noexcept {
        return v == 0;
    }

    FUTURES_NODISCARD constexpr bool
    is_neq(partial_ordering v) noexcept {
        return !(v == 0);
    }

    FUTURES_NODISCARD constexpr bool
    is_lt(partial_ordering v) noexcept {
        return v < 0;
    }

    FUTURES_NODISCARD constexpr bool
    is_lteq(partial_ordering v) noexcept {
        return v <= 0;
    }

    FUTURES_NODISCARD constexpr bool
    is_gt(partial_ordering v) noexcept {
        return v > 0;
    }

    FUTURES_NODISCARD constexpr bool
    is_gteq(partial_ordering v) noexcept {
        return v >= 0;
    }
} // namespace futures
#endif

#endif // FUTURES_ALGORITHM_COMPARE_IMPL_PARTIAL_ORDERING_HPP
