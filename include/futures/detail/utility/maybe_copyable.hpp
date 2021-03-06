//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_DETAIL_UTILITY_MAYBE_COPYABLE_HPP
#define FUTURES_DETAIL_UTILITY_MAYBE_COPYABLE_HPP

namespace futures::detail {
    template <bool allow_copy>
    struct maybe_copyable
    {
    protected:
        maybe_copyable() = default;
        ~maybe_copyable() = default;
    };

    template <>
    struct maybe_copyable<false>
    {
        maybe_copyable() = default;
        maybe_copyable(maybe_copyable const&) = delete;
        maybe_copyable&
        operator=(maybe_copyable const&)
            = delete;

    protected:
        ~maybe_copyable() = default;
    };
} // namespace futures::detail

#endif // FUTURES_DETAIL_UTILITY_MAYBE_COPYABLE_HPP
