//
// Copyright (c) 2021 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_IMPL_FUTURE_ERROR_IPP
#define FUTURES_IMPL_FUTURE_ERROR_IPP

#include <futures/future_error.hpp>

namespace futures {
    std::error_category const &
    future_category() noexcept {
        static future_error_category cat;
        return cat;
    }

    [[nodiscard]] std::error_condition
    future_error_category::default_error_condition(int ev) const noexcept {
        switch (static_cast<future_errc>(ev)) {
        case future_errc::broken_promise:
            return std::error_condition{
                static_cast<int>(future_errc::broken_promise),
                future_category()
            };
        case future_errc::future_already_retrieved:
            return std::error_condition{
                static_cast<int>(future_errc::future_already_retrieved),
                future_category()
            };
        case future_errc::promise_already_satisfied:
            return std::error_condition{
                static_cast<int>(future_errc::promise_already_satisfied),
                future_category()
            };
        case future_errc::no_state:
            return std::error_condition{
                static_cast<int>(future_errc::no_state),
                future_category()
            };
        case future_errc::future_deferred:
            return std::error_condition{
                static_cast<int>(future_errc::future_deferred),
                future_category()
            };
        default:
            return std::error_condition{ ev, *this };
        }
    }

    [[nodiscard]] bool
    future_error_category::equivalent(
        std::error_code const &code,
        int condition) const noexcept {
        return *this == code.category()
               && static_cast<int>(
                      default_error_condition(code.value()).value())
                      == condition;
    }

    [[nodiscard]] std::string
    future_error_category::message(int ev) const {
        switch (static_cast<future_errc>(ev)) {
        case future_errc::broken_promise:
            return std::string{
                "The associated promise has been destructed prior "
                "to the associated state becoming ready."
            };
        case future_errc::future_already_retrieved:
            return std::string{ "The future has already been retrieved from "
                                "the promise or packaged_task." };
        case future_errc::promise_already_satisfied:
            return std::string{
                "The state of the promise has already been set."
            };
        case future_errc::no_state:
            return std::string{ "Operation not permitted on an object without "
                                "an associated state." };
        case future_errc::future_deferred:
            return std::string{
                "Operation not permitted on a deferred future."
            };
        }
        return std::string{ "unspecified future_errc value\n" };
    }

    std::error_code
    make_error_code(future_errc code) {
        return std::error_code{
            static_cast<int>(code),
            futures::future_category()
        };
    }
} // namespace futures

#endif // FUTURES_IMPL_FUTURE_ERROR_IPP
