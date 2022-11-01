//
// Copyright (c) 2021 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ERROR_HPP
#define FUTURES_ERROR_HPP

#include <futures/config.hpp>
#include <system_error>

/**
 *  @file future_error.hpp
 *  @brief Future error types
 *
 *  This file defines error types used by futures types and algorithms.
 */

namespace futures {
    /** @addtogroup futures Futures
     *  @{
     */
    /** @addtogroup error Error
     *
     * \brief Basic future errors
     *
     *  @{
     */

    /// Error codes for futures
    enum class future_errc
    {
        /// The state owner got destroyed before the promise has been fulfilled
        broken_promise = 1,
        /// Attempted to retrieve a unique future twice
        future_already_retrieved,
        /// Promise has already been fulfilled
        promise_already_satisfied,
        /// There is no shared state we can access
        no_state,
        /// The promised hasn't been initialized yet
        promise_uninitialized,
        /// The packaged task hasn't been initialized yet
        packaged_task_uninitialized,
        /// The future hasn't been initialized yet
        future_uninitialized,
        /// Invalid operation on deferred future
        future_deferred,
    };

    /// Class for errors in the futures library
    /**
     * All errors in the futures library derive from this class.
     *
     * The type carries a @ref future_errc
     */
    class error : public std::system_error {
    public:
        /// Constructor
        template <class ErrorCodeEnum FUTURES_REQUIRE(
            (std::is_error_code_enum_v<ErrorCodeEnum>
             || std::is_same_v<ErrorCodeEnum, std::error_code>) )>
        error(ErrorCodeEnum ec) : std::system_error{ ec } {}

        /// Constructor
        template <class ErrorCodeEnum FUTURES_REQUIRE(
            (std::is_error_code_enum_v<ErrorCodeEnum>
             || std::is_same_v<ErrorCodeEnum, std::error_code>) )>
        error(ErrorCodeEnum ec, char const *what_arg)
            : std::system_error{ ec, what_arg } {}

        /// Constructor
        template <class ErrorCodeEnum FUTURES_REQUIRE(
            (std::is_error_code_enum_v<ErrorCodeEnum>
             || std::is_same_v<ErrorCodeEnum, std::error_code>) )>
        error(ErrorCodeEnum ec, std::string const &what_arg)
            : std::system_error{ ec, what_arg } {}

        /// Destructor
        ~error() override = default;
    };

#ifndef FUTURES_DOXYGEN
    FUTURES_DECLARE
    std::error_code
    make_error_code(future_errc code);
#endif

#define FUTURES_ERROR_TYPE(name)                                \
    class name : public error {                                 \
    public:                                                     \
        name() : error{ make_error_code(future_errc::name) } {} \
    }

    /// The state owner got destroyed before the promise has been fulfilled
    FUTURES_ERROR_TYPE(broken_promise);

    /// Attempted to retrieve a unique future twice
    FUTURES_ERROR_TYPE(future_already_retrieved);

    /// Promise has already been fulfilled
    FUTURES_ERROR_TYPE(promise_already_satisfied);

    /// There is no shared state we can access
    FUTURES_ERROR_TYPE(no_state);

    /// The promised hasn't been initialized yet
    FUTURES_ERROR_TYPE(promise_uninitialized);

    /// The packaged task hasn't been initialized yet
    FUTURES_ERROR_TYPE(packaged_task_uninitialized);

    /// The future hasn't been initialized yet
    FUTURES_ERROR_TYPE(future_uninitialized);

    /// Invalid operation on deferred future
    FUTURES_ERROR_TYPE(future_deferred);

#undef FUTURES_ERROR_TYPE

    /** @} */
    /** @} */
} // namespace futures

#include <futures/impl/error.hpp>
#ifdef FUTURES_HEADER_ONLY
#    include <futures/impl/error.ipp>
#endif

#endif // FUTURES_ERROR_HPP
