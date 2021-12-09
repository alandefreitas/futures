/// \file
// Range v3 library
//
//  Copyright Eric Niebler 2013-present
//
//  Use, modification and distribution is subject to the
//  Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
// Project home: https://github.com/ericniebler/range-v3
//
#ifndef FUTURES_RANGES_FUNCTIONAL_IDENTITY_HPP
#define FUTURES_RANGES_FUNCTIONAL_IDENTITY_HPP

#include <futures/algorithm/detail/traits/range/detail/config.h>

#include <futures/algorithm/detail/traits/range/detail/prologue.h>

namespace futures::detail {
    /// \addtogroup group-functional
    /// @{
    struct identity {
        template <typename T> constexpr T &&operator()(T &&t) const noexcept { return (T &&) t; }
        using is_transparent = void;
    };

    /// \cond
    using ident RANGES_DEPRECATED("Replace uses of futures::detail::ident with futures::detail::identity") = identity;
    /// \endcond

    namespace cpp20 {
        using ::futures::detail::identity;
    }
    /// @}
} // namespace futures::detail

#include <futures/algorithm/detail/traits/range/detail/epilogue.h>

#endif
