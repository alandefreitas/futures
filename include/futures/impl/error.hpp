//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_IMPL_ERROR_HPP
#define FUTURES_IMPL_ERROR_HPP

namespace std {
    template <>
    struct is_error_code_enum<::futures::future_errc> {
        static bool const value = true;
    };
} // namespace std

#endif // FUTURES_IMPL_ERROR_HPP
