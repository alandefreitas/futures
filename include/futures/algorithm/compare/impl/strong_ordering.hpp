//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ALGORITHM_COMPARE_IMPL_STRONG_ORDERING_HPP
#define FUTURES_ALGORITHM_COMPARE_IMPL_STRONG_ORDERING_HPP

#if !defined(FUTURES_DOXYGEN) && !defined(__cpp_lib_three_way_comparison)
namespace futures {
#    ifndef __cpp_inline_variables
    namespace detail {
        // emulate inline variables in C++14 with templates that can be
        // initialized inline
        template <typename T>
        struct strong_ordering_base {
            FUTURES_CONST_INIT static const T less;
            FUTURES_CONST_INIT static const T equal;
            FUTURES_CONST_INIT static const T equivalent;
            FUTURES_CONST_INIT static const T greater;
        };
    } // namespace detail
#    endif

    class strong_ordering
#    ifndef __cpp_inline_variables
        : public detail::strong_ordering_base<strong_ordering>
#    endif
    {
#    ifndef __cpp_inline_variables
        friend detail::strong_ordering_base<strong_ordering>;
#    endif

        using unspecified = partial_ordering::unspecified;
        signed char v_;
        constexpr explicit strong_ordering(signed char v) noexcept : v_(v) {}

    public:
#    ifdef __cpp_inline_variables
        static const strong_ordering less;
        static const strong_ordering equal;
        static const strong_ordering equivalent;
        static const strong_ordering greater;
#    endif

        FUTURES_NODISCARD constexpr operator partial_ordering() const noexcept {
            return partial_ordering(v_);
        }

        FUTURES_NODISCARD constexpr operator weak_ordering() const noexcept {
            return weak_ordering(v_);
        }

        // comparisons
        FUTURES_NODISCARD friend constexpr bool
        operator==(strong_ordering lhs, unspecified) noexcept {
            return lhs.v_ == 0;
        }

        FUTURES_NODISCARD friend constexpr bool
        operator==(strong_ordering lhs, strong_ordering rhs) noexcept {
            return lhs.v_ == rhs.v_;
        }

        FUTURES_NODISCARD friend constexpr bool
        operator<(strong_ordering lhs, unspecified) noexcept {
            return lhs.v_ < 0;
        }

        FUTURES_NODISCARD friend constexpr bool
        operator>(strong_ordering lhs, unspecified) noexcept {
            return lhs.v_ > 0;
        }

        FUTURES_NODISCARD friend constexpr bool
        operator<=(strong_ordering lhs, unspecified) noexcept {
            return lhs.v_ <= 0;
        }

        FUTURES_NODISCARD friend constexpr bool
        operator>=(strong_ordering lhs, unspecified) noexcept {
            return lhs.v_ >= 0;
        }

        FUTURES_NODISCARD friend constexpr bool
        operator<(unspecified, strong_ordering rhs) noexcept {
            return 0 < rhs.v_;
        }

        FUTURES_NODISCARD friend constexpr bool
        operator>(unspecified, strong_ordering rhs) noexcept {
            return 0 > rhs.v_;
        }

        FUTURES_NODISCARD friend constexpr bool
        operator<=(unspecified, strong_ordering rhs) noexcept {
            return 0 <= rhs.v_;
        }

        FUTURES_NODISCARD friend constexpr bool
        operator>=(unspecified, strong_ordering rhs) noexcept {
            return 0 >= rhs.v_;
        }

#    if __cplusplus > 201703L && defined(__cpp_impl_three_way_comparison) \
        && __has_include(<compare>)
        FUTURES_NODISCARD friend constexpr strong_ordering
        operator<=>(strong_ordering lhs, unspecified) noexcept {
            return lhs;
        }

        FUTURES_NODISCARD friend constexpr strong_ordering
        operator<=>(unspecified, strong_ordering lhs) noexcept {
            return strong_ordering(-lhs.v_);
        }
#    endif
    };

#    if defined(__cpp_inline_variables)
    inline constexpr strong_ordering strong_ordering::less(-1);
    inline constexpr strong_ordering strong_ordering::equal(0);
    inline constexpr strong_ordering strong_ordering::equivalent(0);
    inline constexpr strong_ordering strong_ordering::greater(1);
#    else
    template <typename T>
    const T detail::strong_ordering_base<T>::less(-1);

    template <typename T>
    const T detail::strong_ordering_base<T>::equal(0);

    template <typename T>
    const T detail::strong_ordering_base<T>::equivalent(0);

    template <typename T>
    const T detail::strong_ordering_base<T>::greater(1);
#    endif
} // namespace futures
#endif

#endif // FUTURES_ALGORITHM_COMPARE_IMPL_STRONG_ORDERING_HPP
