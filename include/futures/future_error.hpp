//
// Copyright (c) 2021 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_FUTURE_ERROR_HPP
#define FUTURES_FUTURE_ERROR_HPP

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

    /// Class for errors in the futures library
    /// All errors in the futures library derive from this class
    class futures_error : public std::system_error {
    public:
        /// Construct underlying system error with a specified error code
        /// @param ec Error code
        explicit futures_error(std::error_code ec) : std::system_error{ ec } {}

        /// Construct underlying system error with a specified error code
        /// and literal string message @param ec Error code @param what_arg
        /// Error string
        futures_error(std::error_code ec, char const *what_arg)
            : std::system_error{ ec, what_arg } {}

        /// Construct underlying system error with a specified error code
        /// and std::string message @param ec Error code @param what_arg Error
        /// string
        futures_error(std::error_code ec, std::string const &what_arg)
            : std::system_error{ ec, what_arg } {}

        /// Destructor
        ~futures_error() override = default;
    };

    /// Error codes for futures
    enum class future_errc
    {
        /// The state owner got destroyed before the promise has been fulfilled
        broken_promise = 1,
        /// Attempted to retrieve a unique future twice
        future_already_retrieved = 2,
        /// Promise has already been fulfilled
        promise_already_satisfied = 3,
        /// There is no shared state we can access
        no_state = 4,
        /// Invalid operation on deferred future
        future_deferred = 5
    };

    FUTURES_DECLARE
    std::error_category const &
    future_category() noexcept;

    /// Class representing the common error category properties for
    /// future errors
    class future_error_category : public std::error_category {
    public:
        /// Name for future_error_category errors
        [[nodiscard]] char const *
        name() const noexcept override {
            return "future";
        }

        /// Generate error condition
        FUTURES_DECLARE
        [[nodiscard]] std::error_condition
        default_error_condition(int ev) const noexcept override;

        /// Check error condition
        FUTURES_DECLARE
        [[nodiscard]] bool
        equivalent(std::error_code const &code, int condition)
            const noexcept override;

        /// Generate message
        FUTURES_DECLARE
        [[nodiscard]] std::string
        message(int ev) const override;
    };

    /// Class for errors with specific future types or their
    /// dependencies, such as promises
    class future_error : public futures_error {
    public:
        /// Construct underlying futures error with a specified error
        /// code @param ec Error code
        explicit future_error(std::error_code ec) : futures_error{ ec } {}
    };

    FUTURES_DECLARE
    std::error_code
    make_error_code(future_errc code);

    /// Class for errors when a promise is not delivered properly
    class broken_promise : public future_error {
    public:
        /// Construct underlying future error with a specified error code
        broken_promise()
            : future_error{ make_error_code(future_errc::broken_promise) } {}
    };

    /// Class for errors when a promise is not delivered properly
    class promise_already_satisfied : public future_error {
    public:
        promise_already_satisfied()
            : future_error{ make_error_code(
                future_errc::promise_already_satisfied) } {}
    };

    /// Class for errors when the a unique future value has already been accessed
    class future_already_retrieved : public future_error {
    public:
        future_already_retrieved()
            : future_error{ make_error_code(
                future_errc::future_already_retrieved) } {}
    };

    /// Class for errors when the value of an uninitialized promise is accessed
    class promise_uninitialized : public future_error {
    public:
        promise_uninitialized()
            : future_error{ make_error_code(future_errc::no_state) } {}
    };

    /// Class for errors when the value of an uninitialized packaged task is
    /// accessed
    class packaged_task_uninitialized : public future_error {
    public:
        packaged_task_uninitialized()
            : future_error{ make_error_code(future_errc::no_state) } {}
    };

    /// Class for errors when the value of an uninitialized future is accessed
    class future_uninitialized : public future_error {
    public:
        future_uninitialized()
            : future_error{ make_error_code(future_errc::no_state) } {}
    };

    /// Class for errors when the operation is not available for deferred futures
    class future_deferred : public future_error {
    public:
        future_deferred()
            : future_error{ make_error_code(future_errc::future_deferred) } {}
    };

    /** @} */
    /** @} */
} // namespace futures

#ifdef FUTURES_HEADER_ONLY
#    include <futures/impl/future_error.ipp>
#endif

#endif // FUTURES_FUTURE_ERROR_HPP
