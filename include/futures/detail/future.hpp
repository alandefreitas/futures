//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_DETAIL_FUTURE_HPP
#define FUTURES_DETAIL_FUTURE_HPP

namespace futures::detail {
    template <class T>
    struct is_executor_opt {
        static constexpr bool value = false;
    };

    template <class T>
    struct is_executor_opt<executor_opt<T>> {
        static constexpr bool value = true;
    };

    template <bool allow_copy>
    struct maybe_copyable {
    protected:
        maybe_copyable() = default;
        ~maybe_copyable() = default;
    };

    template <>
    struct maybe_copyable<false> {
        maybe_copyable() = default;
        maybe_copyable(maybe_copyable const&) = delete;
        maybe_copyable&
        operator=(maybe_copyable const&)
            = delete;

    protected:
        ~maybe_copyable() = default;
    };
}

#endif // FUTURES_DETAIL_FUTURE_HPP
