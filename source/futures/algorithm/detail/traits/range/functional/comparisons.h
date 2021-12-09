/// \file
// Range v3 library
//
//  Copyright Eric Niebler 2013-present
//  Copyright Casey Carter 2016
//
//  Use, modification and distribution is subject to the
//  Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
// Project home: https://github.com/ericniebler/range-v3
//
#ifndef FUTURES_RANGES_FUNCTIONAL_COMPARISONS_HPP
#define FUTURES_RANGES_FUNCTIONAL_COMPARISONS_HPP

#include <futures/algorithm/detail/traits/range/concepts/concepts.h>

#include <futures/algorithm/detail/traits/range/range_fwd.h>

#include <futures/algorithm/detail/traits/range/detail/prologue.h>

namespace futures::detail {
    /// \addtogroup group-functional
    /// @{
    struct equal_to {
        template(typename T, typename U)(
            /// \pre
            requires equality_comparable_with<T, U>) constexpr bool
        operator()(T &&t, U &&u) const {
            return (T &&) t == (U &&) u;
        }
        using is_transparent = void;
    };

    struct not_equal_to {
        template(typename T, typename U)(
            /// \pre
            requires equality_comparable_with<T, U>) constexpr bool
        operator()(T &&t, U &&u) const {
            return !equal_to{}((T &&) t, (U &&) u);
        }
        using is_transparent = void;
    };

    struct less {
        template(typename T, typename U)(
            /// \pre
            requires totally_ordered_with<T, U>) constexpr bool
        operator()(T &&t, U &&u) const {
            return (T &&) t < (U &&) u;
        }
        using is_transparent = void;
    };

    struct less_equal {
        template(typename T, typename U)(
            /// \pre
            requires totally_ordered_with<T, U>) constexpr bool
        operator()(T &&t, U &&u) const {
            return !less{}((U &&) u, (T &&) t);
        }
        using is_transparent = void;
    };

    struct greater_equal {
        template(typename T, typename U)(
            /// \pre
            requires totally_ordered_with<T, U>) constexpr bool
        operator()(T &&t, U &&u) const {
            return !less{}((T &&) t, (U &&) u);
        }
        using is_transparent = void;
    };

    struct greater {
        template(typename T, typename U)(
            /// \pre
            requires totally_ordered_with<T, U>) constexpr bool
        operator()(T &&t, U &&u) const {
            return less{}((U &&) u, (T &&) t);
        }
        using is_transparent = void;
    };

    using ordered_less RANGES_DEPRECATED("Repace uses of futures::detail::ordered_less with futures::detail::less") = less;

#if __cplusplus > 201703L && defined(__cpp_impl_three_way_comparison) && __has_include(<compare>)
    struct compare_three_way {
        template(typename T, typename U)(
            /// \pre
            requires three_way_comparable_with<T, U>) constexpr auto
        operator()(T &&t, U &&u) const -> decltype((T &&) t <=> (U &&) u) {
            return (T &&) t <=> (U &&) u;
        }

        using is_transparent = void;
    };
#endif // __cplusplus

    namespace cpp20 {
        using ::futures::detail::equal_to;
        using ::futures::detail::greater;
        using ::futures::detail::greater_equal;
        using ::futures::detail::less;
        using ::futures::detail::less_equal;
        using ::futures::detail::not_equal_to;
    } // namespace cpp20
    /// @}
} // namespace futures::detail

#include <futures/algorithm/detail/traits/range/detail/epilogue.h>

#endif
