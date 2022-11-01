//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_IMPL_ERROR_IPP
#define FUTURES_IMPL_ERROR_IPP

#include <futures/error.hpp>

namespace futures {
    std::error_code
    make_error_code(future_errc code) {
        struct codes : public std::error_category {
        public:
            [[nodiscard]] char const*
            name() const noexcept override {
                return "futures";
            }

            [[nodiscard]] std::string
            message(int ev) const override {
                switch (static_cast<future_errc>(ev)) {
                case future_errc::broken_promise:
                    return std::string{
                        "The associated promise has been destructed prior "
                        "to the associated state becoming ready."
                    };
                case future_errc::future_already_retrieved:
                    return std::string{
                        "The future has already been retrieved from "
                        "the promise or packaged_task."
                    };
                case future_errc::promise_already_satisfied:
                    return std::string{
                        "The state of the promise has already been set."
                    };
                case future_errc::no_state:
                case future_errc::promise_uninitialized:
                case future_errc::packaged_task_uninitialized:
                case future_errc::future_uninitialized:
                    return std::string{
                        "Operation not permitted on an object without "
                        "an associated state."
                    };
                case future_errc::future_deferred:
                    return std::string{
                        "Operation not permitted on a deferred future."
                    };
                }
                return std::string{ "unspecified future_errc value\n" };
            }
        };

        static codes const cat{};
        return std::error_code{
            static_cast<std::underlying_type<future_errc>::type>(code),
            cat
        };
    }
} // namespace futures

#endif // FUTURES_IMPL_ERROR_IPP
