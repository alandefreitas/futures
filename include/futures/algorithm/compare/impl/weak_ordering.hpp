//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ALGORITHM_COMPARE_IMPL_WEAK_ORDERING_HPP
#define FUTURES_ALGORITHM_COMPARE_IMPL_WEAK_ORDERING_HPP

#if !defined(FUTURES_DOXYGEN) && !defined(__cpp_lib_three_way_comparison)
namespace futures {
#    ifndef __cpp_inline_variables
    namespace detail {
        // emulate inline variables in C++14 with templates that can be
        // initialized inline
        template <typename T>
        struct weak_ordering_base {
            FUTURES_CONST_INIT static const T less;
            FUTURES_CONST_INIT static const T equivalent;
            FUTURES_CONST_INIT static const T greater;
        };
    } // namespace detail
#    endif

    class weak_ordering
#    ifndef __cpp_inline_variables
        : public detail::weak_ordering_base<weak_ordering>
#    endif
    {
#    ifndef __cpp_inline_variables
        friend detail::weak_ordering_base<weak_ordering>;
#    endif

        using unspecified = partial_ordering::unspecified;

        signed char v_;

        constexpr explicit weak_ordering(signed char v) noexcept : v_(v) {}

        friend class strong_ordering;

    public:
#    ifdef __cpp_inline_variables
        static const weak_ordering less;
        static const weak_ordering equivalent;
        static const weak_ordering greater;
#    endif

        FUTURES_NODISCARD constexpr operator partial_ordering() const noexcept {
            return partial_ordering(v_);
        }

        // comparisons
        FUTURES_NODISCARD friend constexpr bool
        operator==(weak_ordering lhs, unspecified) noexcept {
            return lhs.v_ == 0;
        }

        FUTURES_NODISCARD friend constexpr bool
        operator==(weak_ordering lhs, weak_ordering rhs) noexcept {
            return lhs.v_ == rhs.v_;
        }

        FUTURES_NODISCARD friend constexpr bool
        operator<(weak_ordering lhs, unspecified) noexcept {
            return lhs.v_ < 0;
        }

        FUTURES_NODISCARD friend constexpr bool
        operator>(weak_ordering lhs, unspecified) noexcept {
            return lhs.v_ > 0;
        }

        FUTURES_NODISCARD friend constexpr bool
        operator<=(weak_ordering lhs, unspecified) noexcept {
            return lhs.v_ <= 0;
        }

        FUTURES_NODISCARD friend constexpr bool
        operator>=(weak_ordering lhs, unspecified) noexcept {
            return lhs.v_ >= 0;
        }

        FUTURES_NODISCARD friend constexpr bool
        operator<(unspecified, weak_ordering lhs) noexcept {
            return 0 < lhs.v_;
        }

        FUTURES_NODISCARD friend constexpr bool
        operator>(unspecified, weak_ordering lhs) noexcept {
            return 0 > lhs.v_;
        }

        FUTURES_NODISCARD friend constexpr bool
        operator<=(unspecified, weak_ordering lhs) noexcept {
            return 0 <= lhs.v_;
        }

        FUTURES_NODISCARD friend constexpr bool
        operator>=(unspecified, weak_ordering lhs) noexcept {
            return 0 >= lhs.v_;
        }

#    if __cplusplus > 201703L && defined(__cpp_impl_three_way_comparison) \
        && __has_include(<compare>)
        FUTURES_NODISCARD friend constexpr weak_ordering
        operator<=>(weak_ordering lhs, unspecified) noexcept {
            return lhs;
        }

        FUTURES_NODISCARD friend constexpr weak_ordering
        operator<=>(unspecified, weak_ordering rhs) noexcept {
            return weak_ordering(-rhs.v_);
        }
#    endif
    };

#    if defined(__cpp_inline_variables)
    // valid values' definitions
    FUTURES_INLINE_VAR constexpr weak_ordering weak_ordering::less(-1);

    FUTURES_INLINE_VAR constexpr weak_ordering weak_ordering::equivalent(0);

    FUTURES_INLINE_VAR constexpr weak_ordering weak_ordering::greater(1);
#    else
    template <typename T>
    const T detail::weak_ordering_base<T>::less(-1);

    template <typename T>
    const T detail::weak_ordering_base<T>::equivalent(0);

    template <typename T>
    const T detail::weak_ordering_base<T>::greater(1);
#    endif
} // namespace futures
#endif

#endif // FUTURES_ALGORITHM_COMPARE_IMPL_WEAK_ORDERING_HPP
