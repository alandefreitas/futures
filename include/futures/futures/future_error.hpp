//
// Copyright (c) 2021 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_FUTURES_FUTURE_ERROR_HPP
#define FUTURES_FUTURES_FUTURE_ERROR_HPP

#include <system_error>

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
    class futures_error : public std::system_error
    {
    public:
        /// Construct underlying system error with a specified error code
        /// @param ec Error code
        explicit futures_error(std::error_code ec) : std::system_error{ ec } {}

        /// Construct underlying system error with a specified error code
        /// and literal string message @param ec Error code @param what_arg
        /// Error string
        futures_error(std::error_code ec, const char *what_arg)
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

    // fwd-declare
    inline std::error_category const &
    future_category() noexcept;

    /// Class representing the common error category properties for
    /// future errors
    class future_error_category : public std::error_category
    {
    public:
        /// Name for future_error_category errors
        [[nodiscard]] const char *
        name() const noexcept override {
            return "future";
        }

        /// Generate error condition
        [[nodiscard]] std::error_condition
        default_error_condition(int ev) const noexcept override {
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

        /// Check error condition
        [[nodiscard]] bool
        equivalent(std::error_code const &code, int condition)
            const noexcept override {
            return *this == code.category()
                   && static_cast<int>(
                          default_error_condition(code.value()).value())
                          == condition;
        }

        /// Generate message
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

    /// Function to return a common reference to a global future error
    /// category
    inline std::error_category const &
    future_category() noexcept {
        static future_error_category cat;
        return cat;
    }

    /// Class for errors with specific future types or their
    /// dependencies, such as promises
    class future_error : public futures_error
    {
    public:
        /// Construct underlying futures error with a specified error
        /// code @param ec Error code
        explicit future_error(std::error_code ec) : futures_error{ ec } {}
    };

    inline std::error_code
    make_error_code(future_errc code) {
        return std::error_code{
            static_cast<int>(code),
            futures::future_category()
        };
    }

    /// Class for errors when a promise is not delivered properly
    class broken_promise : public future_error
    {
    public:
        /// Construct underlying future error with a specified error code
        broken_promise()
            : future_error{ make_error_code(future_errc::broken_promise) } {}
    };

    /// Class for errors when a promise is not delivered properly
    class promise_already_satisfied : public future_error
    {
    public:
        promise_already_satisfied()
            : future_error{ make_error_code(
                future_errc::promise_already_satisfied) } {}
    };

    class future_already_retrieved : public future_error
    {
    public:
        future_already_retrieved()
            : future_error{ make_error_code(
                future_errc::future_already_retrieved) } {}
    };

    class promise_uninitialized : public future_error
    {
    public:
        promise_uninitialized()
            : future_error{ make_error_code(future_errc::no_state) } {}
    };

    class packaged_task_uninitialized : public future_error
    {
    public:
        packaged_task_uninitialized()
            : future_error{ make_error_code(future_errc::no_state) } {}
    };

    class future_uninitialized : public future_error
    {
    public:
        future_uninitialized()
            : future_error{ make_error_code(future_errc::no_state) } {}
    };

    class future_deferred : public future_error
    {
    public:
        future_deferred()
            : future_error{ make_error_code(future_errc::future_deferred) } {}
    };

    /** @} */
    /** @} */
} // namespace futures

#endif // FUTURES_FUTURES_FUTURE_ERROR_HPP
