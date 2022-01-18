//
// Created by Alan Freitas on 8/1/21.
//

#ifndef FUTURES_FUTURES_H
#define FUTURES_FUTURES_H

/// \file
/// Future types and functions to work with futures
///
/// Many of the ideas for these functions are based on:
/// - extensions for concurrency (ISO/IEC TS 19571:2016)
/// - async++
/// - continuable
/// - TBB
///
/// However, we use std::future and the ASIO proposed standard executors (P0443r13, P1348r0, and P1393r0) to allow
/// for better interoperability with the C++ standard.
/// - the async function can accept any standard executor
/// - the async function will use a reasonable default thread pool when no executor is provided
/// - future-concepts allows for new future classes to extend functionality while reusing algorithms
/// - a cancellable future class is provided for more sensitive use cases
/// - the API can be updated as the standard gets updated
/// - the standard algorithms are reimplemented with a preference for parallel operations
///
/// This interoperability comes at a price for continuations, as we might need to poll for when_all/when_any/then
/// events, because std::future does not have internal continuations.
///
/// Although we attempt to replicate these features without recreating the future class with internal continuations,
/// we use a number of heuristics to avoid polling for when_all/when_any/then:
/// - we allow for other future-like classes to be implemented through a future-concept and provide these
///   functionalities at a lower cost whenever we can
/// - `when_all` (or operator&&) returns a when_all_future class, which does not create a new std::future at all
///    and can check directly if futures are ready
/// - `when_any` (or operator||) returns a when_any_future class, which implements a number of heuristics to avoid
///    polling, limit polling time, increased pooling intervals, and only launching the necessary continuation futures
///    for long tasks. (although when_all always takes longer than when_any, when_any involves a number of heuristics
///    that influence its performance)
/// - `then` (or operator>>) returns a new future object that sleeps while the previous future isn't ready
/// - when the standard supports that, this approach based on concepts also serve as extension points to allow
///   for these proxy classes to change their behavior to some other algorithm that makes more sense for futures
///   that support continuations, cancellation, progress, queries, .... More interestingly, the concepts allow
///   for all these possible future types to interoperate.
///
/// \see https://en.cppreference.com/w/cpp/experimental/concurrency
/// \see https://think-async.com/Asio/asio-1.18.2/doc/asio/std_executors.html
/// \see https://github.com/Amanieu/asyncplusplus

// Future classes
// #include <futures/futures/basic_future.h>
//
// Created by Alan Freitas on 8/21/21.
//

#ifndef FUTURES_BASIC_FUTURE_H
#define FUTURES_BASIC_FUTURE_H

#include <functional>
#include <shared_mutex>
#include <utility>

// #include <futures/executor/default_executor.h>
//
// Created by Alan Freitas on 8/17/21.
//

#ifndef FUTURES_DEFAULT_EXECUTOR_H
#define FUTURES_DEFAULT_EXECUTOR_H

// #include <futures/config/asio_include.h>
//
// Copyright (c) Alan de Freitas 11/18/21.
//

#ifndef FUTURES_ASIO_INCLUDE_H
#define FUTURES_ASIO_INCLUDE_H

/// \file
/// Indirectly includes asio or boost.asio
///
/// Whenever including <asio.hpp>, we include this file instead.
/// This ensures the logic of including asio or boost::asio is consistent and that
/// we never include both.
///
/// Because this is not a networking library, at this point, we only depend on the
/// asio execution concepts (which we can forward-declare) and its thread-pool, which
/// is not very advanced. So this means we might be able to remove boost asio as a
/// dependency at some point and, because the small vector library is also not
/// mandatory, we can make this library free of dependencies.
///

#ifdef _WIN32
#include <SDKDDKVer.h>
#endif

/*
 * Check what versions of asio are available
 *
 * We use __has_include<...> because we are targeting C++17
 *
 */
#if __has_include(<asio.hpp>)
#define FUTURES_HAS_ASIO
#endif

#if __has_include(<boost/asio.hpp>)
#define FUTURES_HAS_BOOST_ASIO
#endif

#if !defined(FUTURES_HAS_BOOST_ASIO) && !defined(FUTURES_HAS_ASIO)
#error Asio headers not found
#endif

/*
 * Decide what version of asio to use
 */

// If the standalone is available, this is what we assume the user usually prefers, since it's more specific
#if defined(FUTURES_HAS_ASIO) && !(defined(FUTURES_HAS_BOOST_ASIO) && defined(FUTURES_PREFER_BOOST_DEPENDENCIES))
#define FUTURES_USE_ASIO
#define ASIO_HAS_MOVE
#include <asio.hpp>
#else
#define FUTURES_USE_BOOST_ASIO
#define BOOST_ASIO_HAS_MOVE
#include <boost/asio.hpp>
#endif

/*
 * Create the aliases
 */

namespace futures {
    /** \addtogroup executors Executors
     *  @{
     */

#if defined(FUTURES_USE_ASIO) || defined(FUTURES_DOXYGEN)
    /// \brief Alias to the asio namespace
    ///
    /// This futures::asio alias might point to ::asio or ::boost::asio.
    ///
    /// If you are referring to the asio namespace and need it
    /// to match the namespace used by futures, prefer `futures::asio`
    /// instead of using `::asio` or `::boost::asio` directly.
    namespace asio = ::asio;
#else
    namespace asio = ::boost::asio;
#endif
    /** @} */
}

#endif // FUTURES_ASIO_INCLUDE_H

// #include <futures/executor/is_executor.h>
//
// Copyright (c) alandefreitas 12/9/21.
// See accompanying file LICENSE
//

#ifndef FUTURES_IS_EXECUTOR_H
#define FUTURES_IS_EXECUTOR_H

// #include <futures/config/asio_include.h>



namespace futures {
    /** \addtogroup executors Executors
     *  @{
     */

    /// \brief Determine if type is an executor
    ///
    /// We only consider asio executors to be executors for now
    /// Future and previous executor models can be considered here, as long as their interface is the same
    /// as asio or we implement their respective traits to make @ref async work properly.
    template <typename T> using is_executor = asio::is_executor<T>;

    /// \brief Determine if type is an executor
    template <typename T> constexpr bool is_executor_v = is_executor<T>::value;

    /** @} */ // \addtogroup executors Executors
} // namespace futures

#endif // FUTURES_IS_EXECUTOR_H


namespace futures {
    /** \addtogroup executors Executors
     *  @{
     */

    /// \brief A version of hardware_concurrency that always returns at least 1
    ///
    /// This function is a safer version of hardware_concurrency that always returns at
    /// least 1 to represent the current context when the value is not computable.
    ///
    /// - It never returns 0, 1 is returned instead.
    /// - It is guaranteed to remain constant for the duration of the program.
    ///
    /// \see https://en.cppreference.com/w/cpp/thread/thread/hardware_concurrency
    ///
    /// \return Number of concurrent threads supported. If the value is not well-defined or not computable, returns 1.
    std::size_t hardware_concurrency() noexcept;
    inline std::size_t hardware_concurrency() noexcept {
        // Cache the value because calculating it may be expensive
        static std::size_t value = std::thread::hardware_concurrency();

        // Always return at least 1 core
        return std::max(static_cast<std::size_t>(1), value);
    }

    /// \brief The default execution context for async operations, unless otherwise stated
    ///
    /// Unless an executor is explicitly provided, this is the executor we use
    /// for async operations.
    ///
    /// This is the ASIO thread pool execution context with a default number of threads.
    /// However, the default execution context (and its type) might change in other
    /// versions of this library if something more general comes along. As the standard
    /// for executors gets adopted, libraries are likely to provide better implementations.
    ///
    /// Also note that executors might not allow work-stealing. This needs to be taken into
    /// account when implementing algorithms with recursive tasks. One common options is to
    /// use `try_async` for recursive tasks.
    ///
    /// Also note that, in the executors notation, the pool is an execution context but not
    /// an executor:
    /// - Execution context: a place where we can execute functions
    /// - A thread pool is an execution context, not an executor
    ///
    /// An execution context is:
    /// - Usually long lived
    /// - Non-copyable
    /// - May contain additional state, such as timers, and threads
    ///
#ifndef FUTURES_DOXYGEN
    using default_execution_context_type = asio::thread_pool;
#else
    using default_execution_context_type = __implementation_defined__;
#endif

    /// \brief Default executor type as a constant trait for future_base functions
    using default_executor_type = default_execution_context_type::executor_type;

    /// \brief Create an instance of the default execution context
    ///
    /// \return Reference to the default execution context for @ref async
    inline default_execution_context_type &default_execution_context() {
#ifdef FUTURES_DEFAULT_THREAD_POOL_SIZE
        const std::size_t default_thread_pool_size = FUTURES_DEFAULT_THREAD_POOL_SIZE;
#else
        const std::size_t default_thread_pool_size = hardware_concurrency();
#endif
        static asio::thread_pool pool(default_thread_pool_size);
        return pool;
    }

    /// \brief Create an Asio thread pool executor for the default thread pool
    ///
    /// In the executors notation:
    /// - Executor: set of rules governing where, when and how to run a function object
    ///   - A thread pool is an execution context for which we can create executors pointing to the pool.
    ///   - The executor rule for the default thread pool executor is to run function objects in the pool
    ///     and nowhere else.
    ///
    /// An executor is:
    /// - Lightweight and copyable (just references and pointers to the execution context).
    /// - May be long or short lived.
    /// - May be customized on a fine-grained basis, such as exception behavior, and order
    ///
    /// There might be many executor types associated with with the same execution context.
    ///
    /// \return Executor handle to the default execution context
    inline default_execution_context_type::executor_type make_default_executor() {
        asio::thread_pool &pool = default_execution_context();
        return pool.executor();
    }

    /** @} */ // \addtogroup executors Executors
} // namespace futures

#endif // FUTURES_DEFAULT_EXECUTOR_H


// #include <futures/futures/traits/is_executor_then_function.h>
//
// Created by Alan Freitas on 8/18/21.
//

#ifndef FUTURES_IS_EXECUTOR_THEN_FUNCTION_H
#define FUTURES_IS_EXECUTOR_THEN_FUNCTION_H

// #include <futures/config/asio_include.h>


namespace futures {
    /** \addtogroup futures Futures
     *  @{
     */
    /** \addtogroup future-traits Future Traits
     *  @{
     */

    class stop_token;
}

namespace futures::detail {
    /// \brief Check if types are an executor then a function
    /// The function should be invocable with the given args, the executor be an executor, and vice-versa
    template <class E, class F, typename... Args>
    using is_executor_then_function =
        std::conjunction<asio::is_executor<E>, std::negation<asio::is_executor<F>>,
                         std::negation<std::is_invocable<E, Args...>>, std::is_invocable<F, Args...>>;

    template <class E, class F, typename... Args>
    constexpr bool is_executor_then_function_v = is_executor_then_function<E, F, Args...>::value;

    template <class E, class F, typename... Args>
    using is_executor_then_stoppable_function =
        std::conjunction<asio::is_executor<E>, std::negation<asio::is_executor<F>>,
                         std::negation<std::is_invocable<E, stop_token, Args...>>,
                         std::is_invocable<F, stop_token, Args...>>;

    template <class E, class F, typename... Args>
    constexpr bool is_executor_then_stoppable_function_v = is_executor_then_stoppable_function<E, F, Args...>::value;

    template <class F, typename... Args>
    using is_invocable_non_executor =
        std::conjunction<std::negation<asio::is_executor<F>>, std::is_invocable<F, Args...>>;

    template <class F, typename... Args>
    constexpr bool is_invocable_non_executor_v = is_invocable_non_executor<F, Args...>::value;

    template <class F, typename... Args>
    using is_stoppable_invocable_non_executor =
        std::conjunction<std::negation<asio::is_executor<F>>, std::is_invocable<F, stop_token, Args...>>;

    template <class F, typename... Args>
    constexpr bool is_stoppable_invocable_non_executor_v = is_stoppable_invocable_non_executor<F, Args...>::value;

    template <class F, typename... Args>
    using is_async_input_non_executor =
        std::disjunction<is_invocable_non_executor<F, Args...>, is_stoppable_invocable_non_executor<F, Args...>>;

    template <class F, typename... Args>
    constexpr bool is_async_input_non_executor_v = is_async_input_non_executor<F, Args...>::value;
    /** @} */  // \addtogroup future-traits Future Traits
    /** @} */  // \addtogroup futures Futures
} // namespace futures::detail

#endif // FUTURES_IS_EXECUTOR_THEN_FUNCTION_H


// #include <futures/futures/detail/continuations_source.h>
//
// Copyright (c) alandefreitas 12/2/21.
// See accompanying file LICENSE
//

#ifndef FUTURES_CONTINUATIONS_SOURCE_H
#define FUTURES_CONTINUATIONS_SOURCE_H

#include <memory>

// #include <futures/config/small_vector_include.h>
//
// Copyright (c) Alan de Freitas 11/18/21.
//

#ifndef FUTURES_SMALL_VECTOR_INCLUDE_H
#define FUTURES_SMALL_VECTOR_INCLUDE_H

/// \file
/// Whenever including <small/vector.h>, we include this file instead
///
/// This ensures the logic of including small::vector or boost::container::small_vector is consistent

/*
 * Check what versions of small vectors are available
 *
 * We use __has_include<...> because we are targeting C++17
 *
 */
#if __has_include(<small/vector.h>)
#define FUTURES_HAS_SMALL_VECTOR
#endif

#if __has_include(<boost/container/small_vector.hpp>)
#define FUTURES_HAS_BOOST_SMALL_VECTOR
#endif

#if !defined(FUTURES_HAS_BOOST_SMALL_VECTOR) && !defined(FUTURES_HAS_SMALL_VECTOR)
#define FUTURES_HAS_NO_SMALL_VECTOR
#endif

/*
 * Decide what version of small vector to use
 */

#ifdef FUTURES_HAS_NO_SMALL_VECTOR
// If no small vectors are available, we recur to regular vectors
#include <vector>
#elif defined(FUTURES_HAS_SMALL_VECTOR) && !(defined(FUTURES_HAS_BOOST_SMALL_VECTOR) && defined(FUTURES_PREFER_BOOST_DEPENDENCIES))
// If the standalone is available, this is what we assume the user usually prefers, since it's more specific
#define FUTURES_USE_SMALL_VECTOR
#include <small/vector.h>
#else
// This is what boost users will often end up using
#define FUTURES_USE_BOOST_SMALL_VECTOR
#include <boost/container/small_vector.hpp>
#endif

/*
 * Create the aliases
 */

namespace futures {
#if defined(FUTURES_USE_SMALL_VECTOR) || defined(FUTURES_DOXYGEN)
    /// \brief Alias to the small vector class we use
    ///
    /// This futures::small_vector alias might refer to:
    /// - ::small::vector if small is available
    /// - ::boost::container::small_vector if boost is available
    /// - ::std::vector if no small vector is available
    ///
    /// If you are referring to this small vector class and need it
    /// to match whatever class is being used by futures, prefer `futures::small_vector`
    /// instead of using `::small::vector` or `::boost::container::small_vector` directly.
    template <typename T, size_t N = ::small::default_inline_storage_v<T>, typename A = std::allocator<T>>
    using small_vector = ::small::vector<T, N, A>;
#elif defined(FUTURES_USE_BOOST_SMALL_VECTOR)
    template <typename T, size_t N = 5, typename A = std::allocator<T>>
    using small_vector = ::boost::container::small_vector<T, N, A>;
#else
    template <typename T, size_t N = ::small::default_inline_storage_v<T>, typename A = std::allocator<T>>
    using small_vector = ::std::vector<T, A>;
#endif
}

#endif // FUTURES_SMALL_VECTOR_INCLUDE_H


namespace futures::detail {
    /** \addtogroup futures Futures
     *  @{
     */

    /// \brief The continuation state as a small thread safe container that holds continuation functions for a future
    ///
    /// The whole logic here is very similar to that of stop_tokens. There is a source, a state, and a token.
    ///
    /// This is very limited as a container because there are not many operations we need to do with the
    /// continuation state. We need to be able to attach continuations (then), and run all continuations
    /// with a single shared lock.
    ///
    /// Like the stop_state, a continuation state might be shared between shared futures.
    /// Once one of the futures has run the continuations, the state is considered done.
    ///
    /// The continuation state needs to be atomic because it's also a shared state.
    /// Especially when the future is shared, many threads might be trying to attach new continuations
    /// to this future type, and the main future callback needs to wait for it.
    class continuations_state {
      public:
        /// \name Public Types
        /// @{

        /// \brief Type of a continuation callback
        /// This is a callback function that posts the next task to an executor.
        /// We cannot ensure the tasks go to the same executor.
        /// This needs to be type erased because there are many types of callables
        /// that might become a continuation here.
        using continuation_type = std::function<void()>;

        /// \brief Continuation ptr
        /// The callbacks are stored pointers because their addresses cannot lose stability
        /// when the future is moved or shared
        using continuation_ptr = std::unique_ptr<continuation_type>;

        /// \brief The continuation vector
        /// We use a small vector because of the common case when there few continuations per task
        using continuation_vector = futures::small_vector<continuation_ptr>;

        /// @}

        /// \name Constructors
        /// @{

        /// \brief Default constructor
        continuations_state() = default;

        /// \brief Copy constructor
        continuations_state(const continuations_state &) = delete;

        /// \brief Destructor - Run continuations if they have not run yet
        ~continuations_state() { request_run(); }

        /// \brief Copy assignment
        continuations_state &operator=(const continuations_state &) = delete;

        /// @}

        /// \name Non-modifying
        /// @{

        /// \brief Get number of continuations
        [[nodiscard]] size_t size() const {
            std::shared_lock lock(continuations_mutex_);
            return continuations_.size();
        }

        /// \brief Get the i-th continuation
        /// The return reference is safe (in context) because the continuation vector has stability
        continuation_type &operator[](size_t index) const {
            std::shared_lock lock(continuations_mutex_);
            return continuations_.at(index).operator*();
        }
        /// @}

        /// \name Modifying
        /// @{

        /// \brief Emplace a new continuation
        /// Use executor ex if more continuations are not possible
        template <class Executor> bool emplace_back(const Executor &ex, continuation_type &&fn) {
            std::unique_lock lock(continuations_mutex_);
            if (is_run_possible()) {
                continuations_.emplace_back(std::make_unique<continuation_type>(std::move(fn)));
                return true;
            } else {
                // When the shared state currently associated with *this is ready, the continuation
                // is called on an unspecified thread of execution
                asio::post(ex, asio::use_future(std::move(fn)));
                return false;
            }
        }

        /// \brief Check if some source asked already asked for the continuations to run
        bool is_run_requested() const {
            std::shared_lock lock(run_requested_mutex_);
            return run_requested_;
        }

        /// \brief Check if some source asked already asked for the continuations to run
        bool is_run_possible() const { return !is_run_requested(); }

        /// \brief Run all continuations
        bool request_run() {
            {
                // Check or update in a single lock
                std::unique_lock lock(run_requested_mutex_);
                if (run_requested_) {
                    return false;
                } else {
                    run_requested_ = true;
                }
            }
            std::unique_lock lock(continuations_mutex_);
            for (auto &continuation : continuations_) {
                (*continuation)();
            }
            continuations_.clear();
            return true;
        }
        /// @}

      private:
        /// \brief The actual pointers to the continuation functions
        /// This is encapsulated so we can't break anything
        continuation_vector continuations_;
        bool run_requested_{false};
        mutable std::shared_mutex continuations_mutex_;
        mutable std::shared_mutex run_requested_mutex_;
    };

    /// Unit type intended for use as a placeholder in continuations_source non-default constructor
    struct nocontinuationsstate_t {
        explicit nocontinuationsstate_t() = default;
    };

    /// This is a constant object instance of stdnocontinuationsstate_t for use in constructing an empty
    /// continuations_source, as a placeholder value in the non-default constructor
    inline constexpr nocontinuationsstate_t nocontinuationsstate{};

    /// \brief Token the future object uses to emplace continuations
    class continuations_token {
      public:
        /// \brief Constructs an empty continuations_token with no associated continuations-state
        continuations_token() noexcept : state_(nullptr) {}

        /// \brief Constructs a continuations_token whose associated continuations-state is the same as that of
        /// other
        continuations_token(const continuations_token &other) noexcept = default;

        /// \brief Constructs a continuations_token whose associated continuations-state is the same as that of
        /// other; other is left empty
        continuations_token(continuations_token &&other) noexcept = default;

        /// \brief Copy-assigns the associated continuations-state of other to that of *this
        continuations_token &operator=(const continuations_token &other) noexcept = default;

        /// \brief Move-assigns the associated continuations-state of other to that of *this
        continuations_token &operator=(continuations_token &&other) noexcept = default;

        /// \brief Exchanges the associated continuations-state of *this and other.
        void swap(continuations_token &other) noexcept { std::swap(state_, other.state_); }

        /// \brief Checks if the continuations_token object has associated continuations-state and that state has
        /// received a run request
        [[nodiscard]] bool run_requested() const noexcept { return (state_ != nullptr) && state_->is_run_requested(); }

        /// \brief Checks if the continuations_token object has associated continuations-state, and that state
        /// either has already had a run requested or it has associated continuations_source object(s)
        [[nodiscard]] bool run_possible() const noexcept {
            return (state_ != nullptr) && (!state_->is_run_requested());
        }

        /// \brief compares two std::run_token objects
        [[nodiscard]] friend bool operator==(const continuations_token &lhs, const continuations_token &rhs) noexcept {
            return lhs.state_ == rhs.state_;
        }

        [[nodiscard]] friend bool operator!=(const continuations_token &lhs, const continuations_token &rhs) noexcept {
            return lhs.state_ != rhs.state_;
        }

      private:
        friend class continuations_source;

        /// \brief Create token from state
        explicit continuations_token(std::shared_ptr<continuations_state> state) noexcept : state_(std::move(state)) {}

        /// \brief The state
        std::shared_ptr<continuations_state> state_;
    };

    /// \brief The continuations_source class provides the means to issue a request to run the future continuations
    class continuations_source {
      public:
        /// \brief Constructs a continuations_source with new continuations-state
        continuations_source() : state_(std::make_shared<continuations_state>()){};

        /// \brief Constructs an empty continuations_source with no associated continuations-state.
        explicit continuations_source(nocontinuationsstate_t) noexcept : state_{nullptr} {}

        /// \brief Copy constructor.
        /// Constructs a continuations_source whose associated continuations-state is the same as that of other.
        continuations_source(const continuations_source &other) noexcept = default;

        /// \brief Move constructor.
        /// Constructs a continuations_source whose associated continuations-state is the same as that of other;
        /// other is left empty.
        continuations_source(continuations_source &&other) noexcept = default;

        /// \brief Copy-assigns the continuations-state of other to that of *this
        continuations_source &operator=(const continuations_source &other) noexcept = default;

        /// \brief Move-assigns the continuations-state of other to that of *this
        continuations_source &operator=(continuations_source &&other) noexcept = default;

        /// \brief Run all continuations
        /// The return reference is safe because the continuation vector has stability
        bool request_run() const {
            if (state_ != nullptr) {
                return state_->request_run();
            }
            return false;
        }

        /// \brief Run all continuations
        /// The return reference is safe because the continuation vector has stability
        template <class Executor>
        bool emplace_continuation(const Executor &ex, continuations_state::continuation_type &&fn) {
            if (state_ != nullptr) {
                return state_->emplace_back(ex, std::move(fn));
            }
            return false;
        }

        /// \brief Exchanges the continuations-state of *this and other.
        void swap(continuations_source &other) noexcept { std::swap(state_, other.state_); }

        /// \brief Get a token to this object
        /// Returns a continuations_token object associated with the continuations_source's continuations-state, if
        /// the continuations_source has continuations-state; otherwise returns a default-constructed (empty)
        /// continuations_token.
        [[nodiscard]] continuations_token get_token() const noexcept { return continuations_token(state_); }

        /// \brief Checks if the continuations_source object has a continuations-state and that state has received a
        /// run request.
        [[nodiscard]] bool run_requested() const noexcept { return state_ != nullptr && state_->is_run_requested(); }

        /// \brief Checks if the continuations_source object has a continuations-state.
        [[nodiscard]] bool run_possible() const noexcept { return state_ != nullptr; }

        /// \brief Compares two continuations_source values
        [[nodiscard]] friend bool operator==(const continuations_source &a, const continuations_source &b) noexcept {
            return a.state_ == b.state_;
        }

        /// \brief Compares two continuations_source values
        [[nodiscard]] friend bool operator!=(const continuations_source &a, const continuations_source &b) noexcept {
            return a.state_ != b.state_;
        }

      private:
        std::shared_ptr<continuations_state> state_;
    };

    /** @} */
} // namespace futures::detail

#endif // FUTURES_CONTINUATIONS_SOURCE_H

// #include <futures/futures/detail/shared_state.h>
//
// Copyright (c) alandefreitas 11/30/21.
// See accompanying file LICENSE
//

#ifndef FUTURES_SHARED_STATE_H
#define FUTURES_SHARED_STATE_H

#include <atomic>
#include <condition_variable>
#include <future>

// #include <futures/futures/future_error.h>
//
// Copyright (c) alandefreitas 11/30/21.
// See accompanying file LICENSE
//

#ifndef FUTURES_FUTURE_ERROR_H
#define FUTURES_FUTURE_ERROR_H

#include <system_error>

namespace futures {
    /** \addtogroup futures Futures
     *  @{
     */
    /** \addtogroup error Error
     *
     * \brief Basic future errors
     *
     *  @{
     */

    /// \brief Class for errors in the futures library
    /// All errors in the futures library derive from this class
    class futures_error : public std::system_error {
      public:
        /// \brief Construct underlying system error with a specified error code
        /// \param ec Error code
        explicit futures_error(std::error_code ec) : std::system_error{ec} {}

        /// \brief Construct underlying system error with a specified error code and literal string message
        /// \param ec Error code
        /// \param what_arg Error string
        futures_error(std::error_code ec, const char *what_arg) : std::system_error{ec, what_arg} {}

        /// \brief Construct underlying system error with a specified error code and std::string message
        /// \param ec Error code
        /// \param what_arg Error string
        futures_error(std::error_code ec, std::string const &what_arg) : std::system_error{ec, what_arg} {}

        /// \brief Destructor
        ~futures_error() override = default;
    };

    /// \brief Error codes for futures
    enum class future_errc {
        /// The state owner got destroyed before the promise has been fulfilled
        broken_promise = 1,
        /// Attempted to retrieve a unique future twice
        future_already_retrieved = 2,
        /// Promise has already been fulfilled
        promise_already_satisfied = 3,
        /// There is no shared state we can access
        no_state = 4
    };

    // fwd-declare
    inline std::error_category const &future_category() noexcept;

    /// \brief Class representing the common error category properties for future errors
    class future_error_category : public std::error_category {
      public:
        /// \brief Name for future_error_category errors
        [[nodiscard]] const char *name() const noexcept override { return "future"; }

        /// \brief Generate error condition
        [[nodiscard]] std::error_condition default_error_condition(int ev) const noexcept override {
            switch (static_cast<future_errc>(ev)) {
            case future_errc::broken_promise:
                return std::error_condition{static_cast<int>(future_errc::broken_promise), future_category()};
            case future_errc::future_already_retrieved:
                return std::error_condition{static_cast<int>(future_errc::future_already_retrieved), future_category()};
            case future_errc::promise_already_satisfied:
                return std::error_condition{static_cast<int>(future_errc::promise_already_satisfied),
                                            future_category()};
            case future_errc::no_state:
                return std::error_condition{static_cast<int>(future_errc::no_state), future_category()};
            default:
                return std::error_condition{ev, *this};
            }
        }

        /// \brief Check error condition
        [[nodiscard]] bool equivalent(std::error_code const &code, int condition) const noexcept override {
            return *this == code.category() &&
                   static_cast<int>(default_error_condition(code.value()).value()) == condition;
        }

        /// \brief Generate message
        [[nodiscard]] std::string message(int ev) const override {
            switch (static_cast<future_errc>(ev)) {
            case future_errc::broken_promise:
                return std::string{"The associated promise has been destructed prior "
                                   "to the associated state becoming ready."};
            case future_errc::future_already_retrieved:
                return std::string{"The future has already been retrieved from "
                                   "the promise or packaged_task."};
            case future_errc::promise_already_satisfied:
                return std::string{"The state of the promise has already been set."};
            case future_errc::no_state:
                return std::string{"Operation not permitted on an object without "
                                   "an associated state."};
            }
            return std::string{"unspecified future_errc value\n"};
        }
    };

    /// \brief Function to return a common reference to a global future error category
    inline std::error_category const &future_category() noexcept {
        static future_error_category cat;
        return cat;
    }

    /// \brief Class for errors with specific future types or their dependencies, such as promises
    class future_error : public futures_error {
      public:
        /// \brief Construct underlying futures error with a specified error code
        /// \param ec Error code
        explicit future_error(std::error_code ec) : futures_error{ec} {}
    };

    inline std::error_code make_error_code(future_errc code) {
        return std::error_code{static_cast<int>(code), futures::future_category()};
    }

    /// \brief Class for errors when a promise is not delivered properly
    class broken_promise : public future_error {
      public:
        /// \brief Construct underlying future error with a specified error code
        /// \param ec Error code
        broken_promise() : future_error{make_error_code(future_errc::broken_promise)} {}
    };

    /// \brief Class for errors when a promise is not delivered properly
    class promise_already_satisfied : public future_error {
      public:
        promise_already_satisfied() : future_error{make_error_code(future_errc::promise_already_satisfied)} {}
    };

    class future_already_retrieved : public future_error {
      public:
        future_already_retrieved() : future_error{make_error_code(future_errc::future_already_retrieved)} {}
    };

    class promise_uninitialized : public future_error {
      public:
        promise_uninitialized() : future_error{make_error_code(future_errc::no_state)} {}
    };

    class packaged_task_uninitialized : public future_error {
      public:
        packaged_task_uninitialized() : future_error{make_error_code(future_errc::no_state)} {}
    };

    class future_uninitialized : public future_error {
      public:
        future_uninitialized() : future_error{make_error_code(future_errc::no_state)} {}
    };

    /** @} */
    /** @} */
} // namespace futures

#endif // FUTURES_FUTURE_ERROR_H


namespace futures {
    /** \addtogroup futures Futures
     *  @{
     */

    namespace detail {

        /// \brief An object that temporarily unlocks a lock
        struct relocker {
            /// \brief The underlying lock
            std::unique_lock<std::mutex> &lock_;

            /// \brief Construct a relocker
            ///
            /// A relocker keeps a temporary reference to the lock and immediately unlocks it
            ///
            /// \param lk Reference to underlying lock
            explicit relocker(std::unique_lock<std::mutex> &lk) : lock_(lk) { lock_.unlock(); }

            /// \brief Copy constructor is deleted
            relocker(const relocker&) = delete;
            relocker(relocker&& other) noexcept = delete;

            /// \brief Copy assignment is deleted
            relocker& operator=(const relocker&) = delete;
            relocker& operator=(relocker&& other) noexcept = delete;

            /// \brief Destroy the relocker
            ///
            /// The relocker locks the underlying lock when it's done
            ~relocker() {
                if (!lock_.owns_lock()) {
                    lock_.lock();
                }
            }

            /// \brief Lock the underlying lock
            void lock() {
                if (!lock_.owns_lock()) {
                    lock_.lock();
                }
            }
        };

        /// \brief Members functions and objects common to all shared state object types
        ///
        /// Shared states for asynchronous operations contain an element of a given type and an exception.
        ///
        /// All objects such as futures and promises have shared states and inherit from this class to synchronize
        /// their access to their common shared state.
        class shared_state_base {
          public:
            /// \brief A list of waiters: condition variables to notify any object waiting for this shared state to be
            /// ready
            using waiter_list = futures::small_vector<std::condition_variable_any *>;

            /// \brief A handle to notify an external context about this state being ready
            using notify_when_ready_handle = waiter_list::iterator;

            /// \brief A default constructed shared state data
            shared_state_base() = default;

            /// \brief Cannot copy construct the shared state data
            ///
            /// We cannot copy construct the shared state data because its parent class
            /// holds synchronization primitives
            shared_state_base(const shared_state_base &) = delete;

            /// \brief Virtual shared state data destructor
            ///
            /// Virtual to make it inheritable
            virtual ~shared_state_base() = default;

            /// \brief Cannot copy assign the shared state data
            ///
            /// We cannot copy assign the shared state data because this parent class
            /// holds synchronization primitives
            shared_state_base &operator=(const shared_state_base &) = delete;

            /// \brief Indicate to the shared state its owner (promise or packaged task) has been destroyed
            ///
            /// If the owner of a shared state is destroyed before the shared state is ready, we need to set
            /// an exception indicating this error. This function checks if the shared state is at a correct
            /// state when the owner is destroyed.
            ///
            /// This overload uses the default global mutex for synchronization
            void signal_owner_destroyed() {
                auto lk = create_unique_lock();
                signal_owner_destroyed(lk);
            }

            /// \brief Set shared state to an exception
            ///
            /// This overload uses the default global mutex for synchronization
            void set_exception(std::exception_ptr except) { // NOLINT(performance-unnecessary-value-param)
                auto lk = create_unique_lock();
                set_exception(except, lk);
            }

            /// \brief Get the shared state when it's an exception
            ///
            /// This overload uses the default global mutex for synchronization
            std::exception_ptr get_exception_ptr() {
                auto lk = create_unique_lock();
                return get_exception_ptr(lk);
            }

            /// \brief Check if state is ready
            ///
            /// This overload uses the default global mutex for synchronization
            bool is_ready() const {
                auto lk = create_unique_lock();
                return is_ready(lk);
            }

            /// \brief Wait for shared state to become ready
            ///
            /// This overload uses the default global mutex for synchronization
            void wait() const {
                auto lk = create_unique_lock();
                wait(lk);
            }

            /// \brief Wait for a specified duration for the shared state to become ready
            ///
            /// This overload uses the default global mutex for synchronization
            template <typename Rep, typename Period>
            std::future_status wait_for(std::chrono::duration<Rep, Period> const &timeout_duration) const {
                auto lk = create_unique_lock();
                return wait_for(lk, timeout_duration);
            }

            /// \brief Wait until a specified time point for the shared state to become ready
            ///
            /// This overload uses the default global mutex for synchronization
            template <typename Clock, typename Duration>
            std::future_status wait_until(std::chrono::time_point<Clock, Duration> const &timeout_time) const {
                auto lk = create_unique_lock();
                return wait_until(lk, timeout_time);
            }

            /// \brief Include a condition variable in the list of waiters we need to notify when the state is ready
            notify_when_ready_handle notify_when_ready(std::condition_variable_any &cv) {
                auto lk = create_unique_lock();
                return notify_when_ready(lk, cv);
            }

            /// \brief Remove condition variable from list of condition variables we need to warn about this state
            void unnotify_when_ready(notify_when_ready_handle it) {
                auto lk = create_unique_lock();
                unnotify_when_ready(lk, it);
            }

            /// \brief Get a reference to the mutex in the shared state
            std::mutex &mutex() {
                return mutex_;
            }

            /// \brief Check if state is ready
            bool is_ready([[maybe_unused]] const std::unique_lock<std::mutex> &lk) const {
                assert(lk.owns_lock());
                return ready_;
            }

          protected:
            /// \brief Indicate to the shared state the state is ready in the derived class
            ///
            /// This operation marks the ready_ flags and warns any future waiting on it.
            /// This overload is meant to be used by derived classes that might need to use another mutex for this
            /// operation
            void mark_ready_and_notify(std::unique_lock<std::mutex> &lk) noexcept {
                assert(lk.owns_lock());
                ready_ = true;
                lk.unlock();
                waiters_.notify_all();
                for (auto &&external_waiter : external_waiters_) {
                    external_waiter->notify_all();
                }
            }

            /// \brief Indicate to the shared state its owner has been destroyed
            ///
            /// If owner has been destroyed before the shared state is ready, this means a promise has been broken
            /// and the shared state should store an exception.
            /// This overload is meant to be used by derived classes that might need to use another mutex for this
            /// operation
            void signal_owner_destroyed(std::unique_lock<std::mutex> &lk) {
                assert(lk.owns_lock());
                if (!ready_) {
                    set_exception(std::make_exception_ptr(broken_promise()), lk);
                }
            }

            /// \brief Set shared state to an exception
            ///
            /// This sets the exception value and marks the shared state as ready.
            /// If we try to set an exception on a shared state that's ready, we throw an exception.
            /// This overload is meant to be used by derived classes that might need to use another mutex for this
            /// operation
            void set_exception(std::exception_ptr except, // NOLINT(performance-unnecessary-value-param)
                               std::unique_lock<std::mutex> &lk) {
                assert(lk.owns_lock());
                if (ready_) {
                    throw promise_already_satisfied();
                }
                except_ = std::move(except);
                mark_ready_and_notify(lk);
            }

            /// \brief Get shared state when it's an exception
            /// This overload is meant to be used by derived classes that might need to use another mutex for this
            /// operation
            std::exception_ptr get_exception_ptr(std::unique_lock<std::mutex> &lk) {
                assert(lk.owns_lock());
                wait(lk);
                return except_;
            }

            /// \brief Wait for shared state to become ready
            /// This function uses the condition variable waiters to wait for this shared state to be marked as ready
            /// This overload is meant to be used by derived classes that might need to use another mutex for this
            /// operation
            void wait(std::unique_lock<std::mutex> &lk) const {
                assert(lk.owns_lock());
                waiters_.wait(lk, [this]() { return ready_; });
            }

            /// \brief Wait for a specified duration for the shared state to become ready
            /// This function uses the condition variable waiters to wait for this shared state to be marked as ready
            /// for a specified duration.
            /// This overload is meant to be used by derived classes that might need to use another mutex for this
            /// operation
            template <typename Rep, typename Period>
            std::future_status wait_for(std::unique_lock<std::mutex> &lk,
                                        std::chrono::duration<Rep, Period> const &timeout_duration) const {
                assert(lk.owns_lock());
                if (waiters_.wait_for(lk, timeout_duration, [this]() { return ready_; })) {
                    return std::future_status::ready;
                } else {
                    return std::future_status::timeout;
                }
            }

            /// \brief Wait until a specified time point for the shared state to become ready
            /// This function uses the condition variable waiters to wait for this shared state to be marked as ready
            /// until a specified time point.
            /// This overload is meant to be used by derived classes that might need to use another mutex for this
            /// operation
            template <typename Clock, typename Duration>
            std::future_status wait_until(std::unique_lock<std::mutex> &lk,
                                          std::chrono::time_point<Clock, Duration> const &timeout_time) const {
                assert(lk.owns_lock());
                if (waiters_.wait_until(lk, timeout_time, [this]() { return ready_; })) {
                    return std::future_status::ready;
                } else {
                    return std::future_status::timeout;
                }
            }

            /// \brief Call the internal callback function, if any
            void do_callback(std::unique_lock<std::mutex> &lk) const {
                if (callback_ && !ready_) {
                    std::function<void()> local_callback = callback_;
                    relocker relock(lk);
                    local_callback();
                }
            }

            /// \brief Include a condition variable in the list of waiters we need to notify when the state is ready
            notify_when_ready_handle notify_when_ready([[maybe_unused]] std::unique_lock<std::mutex> &lk,
                                                       std::condition_variable_any &cv) {
                assert(lk.owns_lock());
                do_callback(lk);
                return external_waiters_.insert(external_waiters_.end(), &cv);
            }

            /// \brief Remove condition variable from list of condition variables we need to warn about this state
            void unnotify_when_ready([[maybe_unused]] std::unique_lock<std::mutex> &lk, notify_when_ready_handle it) {
                assert(lk.owns_lock());
                external_waiters_.erase(it);
            }

            /// \brief Check if state is ready without locking
            constexpr bool is_ready_unsafe() const { return ready_; }

            /// \brief Check if state is ready without locking
            bool stores_exception_unsafe() const { return except_ != nullptr; }

            void throw_internal_exception() const { std::rethrow_exception(except_); }

            /// \brief Check if state is ready without locking
            ///
            /// Although the parent shared state class doesn't store the state, we know
            /// it's ready with state because it's the only way it's ready unless it has an exception.
            bool stores_state_unsafe() const { return is_ready_unsafe() && (!stores_exception_unsafe()); }

            /// \brief Generate unique lock for the shared state
            ///
            /// This lock can be used for any operations on the state that might need to be protected/
            std::unique_lock<std::mutex> create_unique_lock() const {
                return std::unique_lock{mutex_};
            }

          private:
            /// \brief Default global mutex for shared state operations
            mutable std::mutex mutex_{};

            /// \brief Indicates if the shared state is already set
            bool ready_{false};

            /// \brief Pointer to an exception, when the shared_state has been set to an exception
            std::exception_ptr except_{nullptr};

            /// \brief Condition variable to notify any object waiting for this shared state to be ready
            mutable std::condition_variable waiters_{};

            /// \brief List of external condition variables waiting for this shared state to be ready
            waiter_list external_waiters_;

            /// \brief Callback function we should call when waiting for the shared state
            std::function<void()> callback_;
        };

        /// \brief Determine the type we should use to store a shared state internally
        ///
        /// We usually need uninitialized storage for a given type, since the shared state needs to be in control
        /// of constructors and destructors.
        ///
        /// For trivial types, we can directly store the value.
        ///
        /// When the shared state is a reference, we store pointers internally.
        ///
        template <typename R, class Enable = void> struct shared_state_storage {
            using type = std::aligned_storage_t<sizeof(R), alignof(R)>;
        };

        template <typename R> struct shared_state_storage<R, std::enable_if_t<std::is_trivial_v<R>>> {
            using type = R;
        };

        template <typename R> using shared_state_storage_t = typename shared_state_storage<R>::type;
    } // namespace detail

    /// \brief Shared state specialization for regular variables
    ///
    /// This class stores the data for a shared state holding an element of type T
    ///
    /// The data is stored with uninitialized storage because this will only need to be initialized when the
    /// state becomes ready. This ensures the shared state works for all types and avoids wasting operations
    /// on a constructor we might not use.
    ///
    /// However, initialized storage might be more useful and easier to debug for trivial types.
    ///
    template <typename R> class shared_state : public detail::shared_state_base {
      public:
        /// \brief Default construct a shared state
        ///
        /// This will construct the state with uninitialized storage for R
        ///
        shared_state() = default;

        /// \brief Deleted copy constructor
        ///
        /// The copy constructor does not make sense for shared states as they should be shared.
        /// Besides, copying state that might be uninitialized is a bad idea.
        ///
        shared_state(shared_state const &) = delete;

        /// \brief Deleted copy assignment operator
        ///
        /// These functions do not make sense for shared states as they are shared
        shared_state &operator=(shared_state const &) = delete;

        /// \brief Destructor
        ///
        /// Destruct the shared object R manually if the state is ready with a value
        ///
        ~shared_state() override {
            if constexpr (!std::is_trivial_v<R>) {
                if (stores_state_unsafe()) {
                    reinterpret_cast<R *>(std::addressof(storage_))->~R();
                }
            }
        }

        /// \brief Set the value of the shared state to a copy of value
        ///
        /// This function locks the shared state and makes a copy of the value into the storage.
        void set_value(const R &value) {
            auto lk = create_unique_lock();
            set_value(value, lk);
        }

        /// \brief Set the value of the shared state by moving a value
        ///
        /// This function locks the shared state and moves the value to the state
        void set_value(R &&value) {
            auto lk = create_unique_lock();
            set_value(std::move(value), lk);
        }

        /// \brief Get the value of the shared state
        ///
        /// \return Reference to the state as a reference to R
        R &get() {
            auto lk = create_unique_lock();
            return get(lk);
        }

      private:
        /// \brief Set value of the shared state in the storage by copying the value
        ///
        /// \param value The value for the shared state
        /// \param lk Custom mutex lock
        void set_value(R const &value, std::unique_lock<std::mutex> &lk) {
            assert(lk.owns_lock());
            if (is_ready_unsafe()) {
                throw promise_already_satisfied{};
            }
            ::new (static_cast<void *>(std::addressof(storage_))) R(value);
            mark_ready_and_notify(lk);
        }

        /// \brief Set value of the shared state in the storage by moving the value
        ///
        /// \param value The rvalue for the shared state
        /// \param lk Custom mutex lock
        void set_value(R &&value, std::unique_lock<std::mutex> &lk) {
            assert(lk.owns_lock());
            if (is_ready_unsafe()) {
                throw promise_already_satisfied{};
            }
            ::new (static_cast<void *>(std::addressof(storage_))) R(std::move(value));
            mark_ready_and_notify(lk);
        }

        /// \brief Get value of the shared state
        /// This function wait for the shared state to become ready and returns its value
        /// \param lk Custom mutex lock
        /// \return Shared state as a reference to R
        R &get(std::unique_lock<std::mutex> &lk) {
            assert(lk.owns_lock());
            wait(lk);
            if (stores_exception_unsafe()) {
                throw_internal_exception();
            }
            return *reinterpret_cast<R *>(std::addressof(storage_));
        }

        /// \brief Aligned opaque storage for an element of type R
        ///
        /// This needs to be aligned_storage_t because the value might not be initialized yet
        detail::shared_state_storage_t<R> storage_{};
    };

    /// \brief Shared state specialization for references
    ///
    /// This class stores the data for a shared state holding a reference to an element of type T
    /// These shared states need to store a pointer to R internally
    template <typename R> class shared_state<R &> : public detail::shared_state_base {
      public:
        /// \brief Default construct a shared state
        ///
        /// This will construct the state with uninitialized storage for R
        shared_state() = default;

        /// \brief Deleted copy constructor
        ///
        /// These functions do not make sense for shared states as they are shared
        shared_state(shared_state const &) = delete;

        /// \brief Destructor
        ///
        /// Destruct the shared object R if the state is ready with a value
        ~shared_state() override = default;

        /// \brief Deleted copy assignment
        ///
        /// These functions do not make sense for shared states as they are shared
        shared_state &operator=(shared_state const &) = delete;

        /// \brief Set the value of the shared state by storing the address of value R in its internal state
        ///
        /// Shared states to references internally store a pointer to the value instead fo copying it.
        /// This function locks the shared state and moves the value to the state
        void set_value(R &value) {
            auto lk = create_unique_lock();
            set_value(value, lk);
        }

        /// \brief Get the value of the shared state as a reference to the internal pointer
        /// \return Reference to the state as a reference to R
        R &get() {
            auto lk = create_unique_lock();
            return get(lk);
        }

      private:
        /// \brief Set reference of the shared state to the address of a value R
        /// \param value The value for the shared state
        /// \param lk Custom mutex lock
        void set_value(R &value, std::unique_lock<std::mutex> &lk) {
            assert(lk.owns_lock());
            if (is_ready_unsafe()) {
                throw promise_already_satisfied();
            }
            value_ = std::addressof(value);
            mark_ready_and_notify(lk);
        }

        /// \brief Get reference to the shared value
        /// \return Reference to the state as a reference to R
        R &get(std::unique_lock<std::mutex> &lk) {
            assert(lk.owns_lock());
            wait(lk);
            if (stores_exception_unsafe()) {
                throw_internal_exception();
            }
            return *value_;
        }

        /// \brief Pointer to an element of type R representing the reference
        /// This needs to be aligned_storage_t because the value might not be initialized yet
        R *value_{nullptr};
    };

    /// \brief Shared state specialization for a void shared state
    ///
    /// A void shared state needs to synchronize waiting, but it does not need to store anything.
    template <> class shared_state<void> : public detail::shared_state_base {
      public:
        /// \brief Default construct a shared state
        ///
        /// This will construct the state with uninitialized storage for R
        shared_state() = default;

        /// \brief Deleted copy constructor
        ///
        /// These functions do not make sense for shared states as they are shared
        shared_state(shared_state const &) = delete;

        /// \brief Destructor
        ///
        /// Destruct the shared object R if the state is ready with a value
        ~shared_state() override = default;

        /// \brief Deleted copy assignment
        ///
        /// These functions do not make sense for shared states as they are shared
        shared_state &operator=(shared_state const &) = delete;

        /// \brief Set the value of the void shared state without any input as "not-error"
        ///
        /// This function locks the shared state and moves the value to the state
        void set_value() {
            auto lk = create_unique_lock();
            set_value(lk);
        }

        /// \brief Get the value of the shared state by waiting and checking for exceptions
        ///
        /// \return Reference to the state as a reference to R
        void get() {
            auto lk = create_unique_lock();
            get(lk);
        }

      private:
        /// \brief Set value of the shared state with no inputs as "no-error"
        ///
        /// \param lk Custom mutex lock
        void set_value(std::unique_lock<std::mutex> &lk) {
            assert(lk.owns_lock());
            if (is_ready_unsafe()) {
                throw promise_already_satisfied();
            }
            mark_ready_and_notify(lk);
        }

        /// \brief Get the value of the shared state by waiting and checking for exceptions
        void get(std::unique_lock<std::mutex> &lk) const {
            assert(lk.owns_lock());
            wait(lk);
            if (stores_exception_unsafe()) {
                throw_internal_exception();
            }
        }
    };

    /** @} */ // \addtogroup futures Futures
} // namespace futures

#endif // FUTURES_SHARED_STATE_H

// #include <futures/futures/detail/throw_exception.h>
//
// Copyright (c) alandefreitas 12/7/21.
// See accompanying file LICENSE
//

#ifndef FUTURES_THROW_EXCEPTION_H
#define FUTURES_THROW_EXCEPTION_H

namespace futures::detail {
    /** \addtogroup futures::detail Futures
     *  @{
     */

    /// \brief Throw an exception but terminate if we can't throw
    template <typename Ex> [[noreturn]] void throw_exception(Ex &&ex) {
#ifndef FUTURES_DISABLE_EXCEPTIONS
        throw static_cast<Ex &&>(ex);
#else
        (void)ex;
        std::terminate();
#endif
    }

    /// \brief Construct and throw an exception but terminate otherwise
    template <typename Ex, typename... Args> [[noreturn]] void throw_exception(Args &&...args) {
        throw_exception(Ex(std::forward<Args>(args)...));
    }

    /// \brief Throw an exception but terminate if we can't throw
    template <typename ThrowFn, typename CatchFn> void catch_exception(ThrowFn &&thrower, CatchFn &&catcher) {
#ifndef FUTURES_DISABLE_EXCEPTIONS
        try {
            return static_cast<ThrowFn &&>(thrower)();
        } catch (std::exception &) {
            return static_cast<CatchFn &&>(catcher)();
        }
#else
        return static_cast<ThrowFn &&>(thrower)();
#endif
    }

} // namespace futures::detail
#endif // FUTURES_THROW_EXCEPTION_H


// #include <futures/futures/stop_token.h>
//
// Created by Alan Freitas on 8/21/21.
//

#ifndef FUTURES_STOP_TOKEN_H
#define FUTURES_STOP_TOKEN_H

/// \file
///
/// This header contains is an adapted version of std::stop_token for futures rather than threads.
///
/// The main difference in this implementation is 1) the reference counter does not distinguish between
/// tokens and sources, and 2) there is no stop_callback.
///
/// The API was initially adapted from Baker Josuttis' reference implementation for C++20:
/// \see https://github.com/josuttis/jthread
///
/// Although the jfuture class is obviously different from std::jthread, this stop_token is not different
/// from std::stop_token. The main goal here is just to provide a stop source in C++17. In the future, we
/// might replace this with an alias to a C++20 std::stop_token.

// #include <atomic>

#include <thread>
#include <type_traits>
// #include <utility>


namespace futures {
    /** \addtogroup futures Futures
     *  @{
     */
    /** \addtogroup cancellation Cancellation
     *
     * \brief Future cancellation primitives
     *
     *  @{
     */

    namespace detail {
        using shared_stop_state = std::shared_ptr<std::atomic<bool>>;
    }

    class stop_source;

    /// \brief Empty struct to initialize a @ref stop_source without a shared stop state
    struct nostopstate_t {
        explicit nostopstate_t() = default;
    };

    /// \brief Empty struct to initialize a @ref stop_source without a shared stop state
    inline constexpr nostopstate_t nostopstate{};

    /// \brief Token to check if a stop request has been made
    ///
    /// The stop_token class provides the means to check if a stop request has been made or can be made, for its
    /// associated std::stop_source object. It is essentially a thread-safe "view" of the associated stop-state.
    class stop_token {
      public:
        /// \name Constructors
        /// @{

        /// \brief Constructs an empty stop_token with no associated stop-state
        ///
        /// \post stop_possible() and stop_requested() are both false
        ///
        /// \param other another stop_token object to construct this stop_token object
        stop_token() noexcept = default;

        /// \brief Copy constructor.
        ///
        /// Constructs a stop_token whose associated stop-state is the same as that of other.
        ///
        /// \post *this and other share the same associated stop-state and compare equal
        ///
        /// \param other another stop_token object to construct this stop_token object
        stop_token(const stop_token &other) noexcept = default;

        /// \brief Move constructor.
        ///
        /// Constructs a stop_token whose associated stop-state is the same as that of other; other is left empty
        ///
        /// \post *this has other's previously associated stop-state, and other.stop_possible() is false
        ///
        /// \param other another stop_token object to construct this stop_token object
        stop_token(stop_token &&other) noexcept : shared_state_(std::exchange(other.shared_state_, nullptr)) {}

        /// \brief Destroys the stop_token object.
        ///
        /// \post If *this has associated stop-state, releases ownership of it.
        ///
        ~stop_token() = default;

        /// \brief Copy-assigns the associated stop-state of other to that of *this
        ///
        /// Equivalent to stop_token(other).swap(*this)
        ///
        /// \param other Another stop_token object to share the stop-state with to or acquire the stop-state from
        stop_token &operator=(const stop_token &other) noexcept {
            if (shared_state_ != other.shared_state_) {
                stop_token tmp{other};
                swap(tmp);
            }
            return *this;
        }

        /// \brief Move-assigns the associated stop-state of other to that of *this
        ///
        /// After the assignment, *this contains the previous associated stop-state of other, and other has no
        /// associated stop-state
        ///
        /// Equivalent to stop_token(std::move(other)).swap(*this)
        ///
        /// \param other Another stop_token object to share the stop-state with to or acquire the stop-state from
        stop_token &operator=(stop_token &&other) noexcept {
            if (this != &other) {
                stop_token tmp{std::move(other)};
                swap(tmp);
            }
            return *this;
        }

        /// @}

        /// \name Modifiers
        /// @{

        /// \brief Exchanges the associated stop-state of *this and other
        ///
        /// \param other stop_token to exchange the contents with
        void swap(stop_token &other) noexcept { std::swap(shared_state_, other.shared_state_); }

        /// @}

        /// \name Observers
        /// @{

        /// \brief Checks whether the associated stop-state has been requested to stop
        ///
        /// Checks if the stop_token object has associated stop-state and that state has received a stop request. A
        /// default constructed stop_token has no associated stop-state, and thus has not had stop requested
        ///
        /// \return true if the stop_token object has associated stop-state and it received a stop request, false
        /// otherwise.
        [[nodiscard]] bool stop_requested() const noexcept {
            return (shared_state_ != nullptr) && shared_state_->load(std::memory_order_relaxed);
        }

        /// \brief Checks whether associated stop-state can be requested to stop
        ///
        /// Checks if the stop_token object has associated stop-state, and that state either has already had a stop
        /// requested or it has associated std::stop_source object(s).
        ///
        /// A default constructed stop_token has no associated `stop-state`, and thus cannot be stopped.
        /// the associated stop-state for which no std::stop_source object(s) exist can also not be stopped if
        /// such a request has not already been made.
        ///
        /// \note If the stop_token object has associated stop-state and a stop request has already been made, this
        /// function still returns true.
        ///
        /// \return false if the stop_token object has no associated stop-state, or it did not yet receive a stop
        /// request and there are no associated std::stop_source object(s); true otherwise
        [[nodiscard]] bool stop_possible() const noexcept {
            return (shared_state_ != nullptr) && (shared_state_->load(std::memory_order_relaxed) || (shared_state_.use_count() > 1));
        }

        /// @}

        /// \name Non-member functions
        /// @{

        /// \brief compares two std::stop_token objects
        ///
        /// This function is not visible to ordinary unqualified or qualified lookup, and can only be found by
        /// argument-dependent lookup when std::stop_token is an associated class of the arguments.
        ///
        /// \param a stop_tokens to compare
        /// \param b stop_tokens to compare
        ///
        /// \return true if lhs and rhs have the same associated stop-state, or both have no associated stop-state,
        /// otherwise false
        [[nodiscard]] friend bool operator==(const stop_token &a, const stop_token &b) noexcept {
            return a.shared_state_ == b.shared_state_;
        }

        /// \brief compares two std::stop_token objects for inequality
        ///
        /// The != operator is synthesized from operator==
        ///
        /// \param a stop_tokens to compare
        /// \param b stop_tokens to compare
        ///
        /// \return true if lhs and rhs have different associated stop-states
        [[nodiscard]] friend bool operator!=(const stop_token &a, const stop_token &b) noexcept {
            return a.shared_state_ != b.shared_state_;
        }

        /// @}

      private:
        friend class stop_source;

        /// \brief Constructor that allows the stop_source to construct the stop_token directly from the stop state
        ///
        /// \param state State for the new token
        explicit stop_token(detail::shared_stop_state state) noexcept : shared_state_(std::move(state)) {}

        /// \brief Shared pointer to an atomic bool indicating if an external procedure should stop
        detail::shared_stop_state shared_state_{nullptr};
    };

    /// \brief Object used to issue a stop request
    ///
    /// The stop_source class provides the means to issue a stop request, such as for std::jthread cancellation.
    /// A stop request made for one stop_source object is visible to all stop_sources and std::stop_tokens of
    /// the same associated stop-state; any std::stop_callback(s) registered for associated std::stop_token(s)
    /// will be invoked, and any std::condition_variable_any objects waiting on associated std::stop_token(s)
    /// will be awoken.
    class stop_source {
      public:
        /// \name Constructors
        /// @{

        /// \brief Constructs a stop_source with new stop-state
        ///
        /// \post stop_possible() is true and stop_requested() is false
        stop_source() : shared_state_(std::make_shared<std::atomic_bool>(false)) {}

        /// \brief Constructs an empty stop_source with no associated stop-state
        ///
        /// \post stop_possible() and stop_requested() are both false
        explicit stop_source(nostopstate_t) noexcept {};

        /// \brief Copy constructor
        ///
        /// Constructs a stop_source whose associated stop-state is the same as that of other.
        ///
        /// \post *this and other share the same associated stop-state and compare equal
        ///
        /// \param other another stop_source object to construct this stop_source object with
        stop_source(const stop_source &other) noexcept = default;

        /// \brief Move constructor
        ///
        /// Constructs a stop_source whose associated stop-state is the same as that of other; other is left empty
        ///
        /// \post *this has other's previously associated stop-state, and other.stop_possible() is false
        ///
        /// \param other another stop_source object to construct this stop_source object with
        stop_source(stop_source &&other) noexcept : shared_state_(std::exchange(other.shared_state_, nullptr)) {}

        /// \brief Destroys the stop_source object.
        ///
        /// If *this has associated stop-state, releases ownership of it.
        ~stop_source() = default;

        /// \brief Copy-assigns the stop-state of other
        ///
        /// Equivalent to stop_source(other).swap(*this)
        ///
        /// \param other another stop_source object acquire the stop-state from
        stop_source &operator=(stop_source &&other) noexcept {
            stop_source tmp{std::move(other)};
            swap(tmp);
            return *this;
        }

        /// \brief Move-assigns the stop-state of other
        ///
        /// Equivalent to stop_source(std::move(other)).swap(*this)
        ///
        /// \post After the assignment, *this contains the previous stop-state of other, and other has no stop-state
        ///
        /// \param other another stop_source object to share the stop-state with
        stop_source &operator=(const stop_source &other) noexcept {
            if (shared_state_ != other.shared_state_) {
                stop_source tmp{other};
                swap(tmp);
            }
            return *this;
        }

        /// @}

        /// \name Modifiers
        /// @{

        /// \brief Makes a stop request for the associated stop-state, if any
        ///
        /// Issues a stop request to the stop-state, if the stop_source object has a stop-state, and it has not yet
        /// already had stop requested.
        ///
        /// The determination is made atomically, and if stop was requested, the stop-state is atomically updated to
        /// avoid race conditions, such that:
        ///
        /// - stop_requested() and stop_possible() can be concurrently invoked on other stop_tokens and stop_sources of
        /// the same stop-state
        /// - request_stop() can be concurrently invoked on other stop_source objects, and only one will actually
        /// perform the stop request.
        ///
        /// \return true if the stop_source object has a stop-state and this invocation made a stop request (the
        /// underlying atomic value was successfully changed), otherwise false
        bool request_stop() noexcept {
            if (shared_state_ != nullptr) {
                bool expected = false;
                return shared_state_->compare_exchange_strong(expected, true, std::memory_order_relaxed);
            }
            return false;
        }

        /// \brief Swaps two stop_source objects
        /// \param other stop_source to exchange the contents with
        void swap(stop_source &other) noexcept { std::swap(shared_state_, other.shared_state_); }

        /// @}

        /// \name Non-member functions
        /// @{

        /// \brief Returns a stop_token for the associated stop-state
        ///
        /// Returns a stop_token object associated with the stop_source's stop-state, if the stop_source has stop-state,
        /// otherwise returns a default-constructed (empty) stop_token.
        ///
        /// \return A stop_token object, which will be empty if this->stop_possible() == false
        [[nodiscard]] stop_token get_token() const noexcept { return stop_token{shared_state_}; }

        /// \brief Checks whether the associated stop-state has been requested to stop
        ///
        /// Checks if the stop_source object has a stop-state and that state has received a stop request.
        ///
        /// \return true if the stop_token object has a stop-state, and it has received a stop request, false otherwise
        [[nodiscard]] bool stop_requested() const noexcept {
            return (shared_state_ != nullptr) && shared_state_->load(std::memory_order_relaxed);
        }

        /// \brief Checks whether associated stop-state can be requested to stop
        ///
        /// Checks if the stop_source object has a stop-state.
        ///
        /// \note If the stop_source object has a stop-state and a stop request has already been made, this function
        /// still returns true.
        ///
        /// \return true if the stop_source object has a stop-state, otherwise false
        [[nodiscard]] bool stop_possible() const noexcept { return shared_state_ != nullptr; }

        /// @}

        /// \name Non-member functions
        /// @{

        [[nodiscard]] friend bool operator==(const stop_source &a, const stop_source &b) noexcept {
            return a.shared_state_ == b.shared_state_;
        }
        [[nodiscard]] friend bool operator!=(const stop_source &a, const stop_source &b) noexcept {
            return a.shared_state_ != b.shared_state_;
        }

        /// @}

      private:
        /// \brief Shared pointer to an atomic bool indicating if an external procedure should stop
        detail::shared_stop_state shared_state_{nullptr};
    };

    /** @} */ // \addtogroup cancellation Cancellation
    /** @} */ // \addtogroup futures Futures
} // namespace futures

#endif // FUTURES_STOP_TOKEN_H

// #include <futures/futures/traits/is_future.h>
//
// Created by Alan Freitas on 8/18/21.
//

#ifndef FUTURES_IS_FUTURE_H
#define FUTURES_IS_FUTURE_H

// #include <future>

// #include <type_traits>


namespace futures {
    /** \addtogroup futures Futures
     *  @{
     */
    /** \addtogroup future-traits Future Traits
     *
     * \brief Determine properties of future types
     *
     *  @{
     */

    /// \brief Customization point to determine if a type is a future type
    template <typename> struct is_future : std::false_type {};

    /// \brief Customization point to determine if a type is a future type (specialization for std::future<T>)
    template <typename T> struct is_future<std::future<T>> : std::true_type {};

    /// \brief Customization point to determine if a type is a future type (specialization for std::shared_future<T>)
    template <typename T> struct is_future<std::shared_future<T>> : std::true_type {};

    /// \brief Customization point to determine if a type is a future type as a bool value
    template <class T> constexpr bool is_future_v = is_future<T>::value;

    /// \brief Customization point to determine if a type is a shared future type
    template <typename> struct has_ready_notifier : std::false_type {};

    /// \brief Customization point to determine if a type is a shared future type
    template <class T> constexpr bool has_ready_notifier_v = has_ready_notifier<T>::value;

    /// \brief Customization point to determine if a type is a shared future type
    template <typename> struct is_shared_future : std::false_type {};

    /// \brief Customization point to determine if a type is a shared future type (specialization for std::shared_future<T>)
    template <typename T> struct is_shared_future<std::shared_future<T>> : std::true_type {};

    /// \brief Customization point to determine if a type is a shared future type
    template <class T> constexpr bool is_shared_future_v = is_shared_future<T>::value;

    /// \brief Customization point to define future as supporting lazy continuations
    template <typename> struct is_lazy_continuable : std::false_type {};

    /// \brief Customization point to define future as supporting lazy continuations
    template <class T> constexpr bool is_lazy_continuable_v = is_lazy_continuable<T>::value;

    /// \brief Customization point to define future as stoppable
    template <typename> struct is_stoppable : std::false_type {};

    /// \brief Customization point to define future as stoppable
    template <class T> constexpr bool is_stoppable_v = is_stoppable<T>::value;

    /// \brief Customization point to define future having a common stop token
    template <typename> struct has_stop_token : std::false_type {};

    /// \brief Customization point to define future having a common stop token
    template <class T> constexpr bool has_stop_token_v = has_stop_token<T>::value;

    /** @} */ // \addtogroup future-traits Future Traits
    /** @} */ // \addtogroup futures Futures
} // namespace futures

#endif // FUTURES_IS_FUTURE_H


namespace futures {
    /** \addtogroup futures Futures
     *
     * \brief Basic future types and functions
     *
     * The futures library provides components to create and launch futures, objects representing data that might
     * not be available yet.
     *  @{
     */

    /** \addtogroup future-types Future types
     *
     * \brief Basic future types
     *
     * This module defines the @ref basic_future template class, which can be used to define futures
     * with a number of extensions.
     *
     *  @{
     */

    // Fwd-declaration
    template <class T, class Shared, class LazyContinuable, class Stoppable> class basic_future;

    /// \brief A simple future type similar to `std::future`
    template <class T> using future = basic_future<T, std::false_type, std::false_type, std::false_type>;

    /// \brief A future type with stop tokens
    ///
    /// This class is like a version of jthread for futures
    ///
    /// It's a quite common use case that we need a way to cancel futures and
    /// jfuture provides us with an even better way to do that.
    ///
    /// @ref async is adapted to return a jcfuture whenever:
    /// 1. the callable receives 1 more argument than the caller provides @ref async
    /// 2. the first callable argument is a stop token
    template <class T> using jfuture = basic_future<T, std::false_type, std::false_type, std::true_type>;

    /// \brief A future type with lazy continuations
    ///
    /// This is what a @ref futures::async returns when the first function parameter is not a @ref futures::stop_token
    ///
    template <class T> using cfuture = basic_future<T, std::false_type, std::true_type, std::false_type>;

    /// \brief A future type with lazy continuations and stop tokens
    ///
    /// This is what a @ref futures::async returns when the first function parameter is a @ref futures::stop_token
    template <class T> using jcfuture = basic_future<T, std::false_type, std::true_type, std::true_type>;

    /// \brief A future type with lazy continuations and stop tokens
    /// Same as @ref jcfuture
    template <class T> using cjfuture = jcfuture<T>;

    /// \brief A simple std::shared_future
    ///
    /// This is what a futures::future::share() returns
    template <class T> using shared_future = basic_future<T, std::true_type, std::false_type, std::false_type>;

    /// \brief A shared future type with stop tokens
    ///
    /// This is what a @ref futures::jfuture::share() returns
    template <class T> using shared_jfuture = basic_future<T, std::true_type, std::false_type, std::true_type>;

    /// \brief A shared future type with lazy continuations
    ///
    /// This is what a @ref futures::cfuture::share() returns
    template <class T> using shared_cfuture = basic_future<T, std::true_type, std::true_type, std::false_type>;

    /// \brief A shared future type with lazy continuations and stop tokens
    ///
    /// This is what a @ref futures::jcfuture::share() returns
    template <class T> using shared_jcfuture = basic_future<T, std::true_type, std::true_type, std::true_type>;

    /// \brief A shared future type with lazy continuations and stop tokens
    ///
    /// \note Same as @ref shared_jcfuture
    template <class T> using shared_cjfuture = shared_jcfuture<T>;

    namespace detail {
        /// \name Helpers to declare internal_async a friend
        /// These helpers also help us deduce what kind of types we will return from `async` and `then`
        /// @{

        /// @}

        // Fwd-declare
        template <typename R> class promise_base;
        struct async_future_scheduler;
        struct internal_then_functor;

        /// \class Enable a future class that automatically waits on destruction, and can be cancelled/stopped
        /// Besides a regular future, this class also carries a stop_source representing a should-stop
        /// state. The callable should receive a stop_token which represents a "view" of this stop
        /// state. The user can request the future to stop with request_stop, without directly
        /// interacting with the stop_source.
        /// jfuture, cfuture, and jcfuture share a lot of code.
        template <class Derived, class T> class enable_stop_token {
          public:
            enable_stop_token() = default;
            enable_stop_token(const enable_stop_token &c) = default;
            /// \brief Move construct/assign helper for the
            /// When the basic future is being moved, the stop source gets copied instead of moved,
            /// because we only want to move the future state, but we don't want to invalidate the
            /// stop token of a function that is already running. The user could copy the source
            /// before moving the future.
            enable_stop_token(enable_stop_token &&c) noexcept : stop_source_(c.stop_source_){};

            enable_stop_token &operator=(const enable_stop_token &c) = default;
            enable_stop_token &operator=(enable_stop_token &&c) noexcept {
                stop_source_ = c.stop_source_;
                return *this;
            };

            bool request_stop() noexcept { return get_stop_source().request_stop(); }

            [[nodiscard]] stop_source get_stop_source() const noexcept { return stop_source_; }

            [[nodiscard]] stop_token get_stop_token() const noexcept { return stop_source_.get_token(); }

          public:
            /// \brief Stop source for started future
            /// Unlike threads, futures need to be default constructed without nostopstate because
            /// the std::future might be set later in this object and it needs to be created with
            /// a reference to this already existing stop source.
            stop_source stop_source_{nostopstate};
        };

        template <class Derived, class T> class disable_stop_token {
          public:
            disable_stop_token() = default;
            disable_stop_token(const disable_stop_token &c) = default;
            disable_stop_token(disable_stop_token &&c) noexcept = default;
            disable_stop_token &operator=(const disable_stop_token &c) = default;
            disable_stop_token &operator=(disable_stop_token &&c) noexcept = default;
        };

        template <typename Enable, typename Derived, typename T>
        using stop_token_base =
            std::conditional_t<Enable::value, enable_stop_token<Derived, T>, disable_stop_token<Derived, T>>;

        /// \brief Enable lazy continuations for a future type
        /// When async starts this future type, its internal lambda function needs to be is programmed
        /// to continue and run all attached continuations in the same executor even after the original
        /// callable is over.
        template <class Derived, class T> class enable_lazy_continuations {
          public:
            enable_lazy_continuations() = default;
            enable_lazy_continuations(const enable_lazy_continuations &c) = default;
            enable_lazy_continuations &operator=(const enable_lazy_continuations &c) = default;
            enable_lazy_continuations(enable_lazy_continuations &&c) noexcept = default;
            enable_lazy_continuations &operator=(enable_lazy_continuations &&c) noexcept = default;

            /// Default Constructors: whenever the state is moved or copied, the default constructor
            /// will copy or move the internal shared ptr pointing to this common state of continuations.

            bool then(continuations_state::continuation_type &&fn) {
                return then(make_default_executor(), asio::use_future(std::move(fn)));
            }

            /// \brief Emplace a function to the shared vector of continuations
            /// If the function is ready, use the given executor instead of executing inline
            /// with the previous executor.
            template <class Executor> bool then(const Executor &ex, continuations_state::continuation_type &&fn) {
                if (not static_cast<Derived *>(this)->valid()) {
                    throw std::future_error(std::future_errc::no_state);
                }
                if (not static_cast<Derived *>(this)->is_ready() && continuations_source_.run_possible()) {
                    return continuations_source_.emplace_continuation(ex, std::move(fn));
                } else {
                    // When the shared state currently associated with *this is ready, the continuation
                    // is called on an unspecified thread of execution
                    asio::post(ex, asio::use_future(std::move(fn)));
                    return false;
                }
            }

          public:
            /// \brief Internal shared continuation state
            /// The continuation state needs to be in a shared pointer because of it's shared.
            /// Shared futures and internal functions accessing this state need be have access to the same
            /// state for continuations.
            /// The pointer also helps with stability issues when the futures are moved or shared.
            continuations_source continuations_source_;
        };

        template <class Derived, class T> class disable_lazy_continuations {
          public:
            disable_lazy_continuations() = default;
            disable_lazy_continuations(const disable_lazy_continuations &c) = default;
            disable_lazy_continuations &operator=(const disable_lazy_continuations &c) = default;
            disable_lazy_continuations(disable_lazy_continuations &&c) noexcept = default;
            disable_lazy_continuations &operator=(disable_lazy_continuations &&c) noexcept = default;
        };

        /// \subsection Convenience aliases to refer to base classes
        template <typename Enable, typename Derived, typename T>
        using lazy_continuations_base = std::conditional_t<Enable::value, enable_lazy_continuations<Derived, T>,
                                                           disable_lazy_continuations<Derived, T>>;
    } // namespace detail

#ifndef FUTURES_DOXYGEN
    // fwd-declare
    template <typename Signature> class packaged_task;
#endif

    /// \brief A basic future type with custom features
    ///
    /// Note that these classes only provide the capabilities of tracking these features, such
    /// as continuations.
    ///
    /// Setting up these capabilities (creating tokens or setting main future to run continuations)
    /// needs to be done when the future is created in a function such as @ref async by creating
    /// the appropriate state for each feature.
    ///
    /// All this behavior is already encapsulated in the @ref async function.
    ///
    /// \tparam T Type of element
    /// \tparam Shared `std::true_value` if this future is shared
    /// \tparam LazyContinuable `std::true_value` if this future supports continuations
    /// \tparam Stoppable `std::true_value` if this future contains a stop token
    ///
    template <class T, class Shared, class LazyContinuable, class Stoppable>
    class
#if defined(__clang__) && !defined(__apple_build_version__)
        [[clang::preferred_name(jfuture<T>), clang::preferred_name(cfuture<T>), clang::preferred_name(jcfuture<T>),
          clang::preferred_name(shared_jfuture<T>), clang::preferred_name(shared_cfuture<T>),
          clang::preferred_name(shared_jcfuture<T>)]]
#endif
        basic_future
#ifndef FUTURES_DOXYGEN
        : public detail::lazy_continuations_base<LazyContinuable, basic_future<T, Shared, LazyContinuable, Stoppable>,
                                                 T>,
          public detail::stop_token_base<Stoppable, basic_future<T, Shared, LazyContinuable, Stoppable>, T>
#endif
    {
      public:
        /// \name Public types
        /// @{

        using value_type = T;

        using is_shared = Shared;
        using is_lazy_continuable = LazyContinuable;
        using is_stoppable = Stoppable;
        static constexpr bool is_shared_v = Shared::value;
        static constexpr bool is_lazy_continuable_v = LazyContinuable::value;
        static constexpr bool is_stoppable_v = Stoppable::value;

#ifndef FUTURES_DOXYGEN
        using lazy_continuations_base =
            detail::lazy_continuations_base<LazyContinuable, basic_future<T, Shared, LazyContinuable, Stoppable>, T>;
        using stop_token_base =
            detail::stop_token_base<Stoppable, basic_future<T, Shared, LazyContinuable, Stoppable>, T>;

        friend lazy_continuations_base;
        friend stop_token_base;

        using notify_when_ready_handle = detail::shared_state_base::notify_when_ready_handle;
#endif

        /// @}

        /// \name Shared state counterparts
        /// @{

        // Other shared state types can access the shared state constructor directly
        friend class detail::promise_base<T>;

        template <typename Signature> friend class packaged_task;

#ifndef FUTURES_DOXYGEN
        // The shared variant is always a friend
        using basic_future_shared_version_t = basic_future<T, std::true_type, LazyContinuable, Stoppable>;
        friend basic_future_shared_version_t;

        using basic_future_unique_version_t = basic_future<T, std::false_type, LazyContinuable, Stoppable>;
        friend basic_future_unique_version_t;
#endif

        friend struct detail::async_future_scheduler;
        friend struct detail::internal_then_functor;

        /// @}

        /// \name Constructors
        /// @{

        /// \brief Constructs the basic_future
        ///
        /// The default constructor creates an invalid future with no shared state.
        ///
        /// Null shared state. Properties inherited from base classes.
        basic_future() noexcept
            : lazy_continuations_base(), // No continuations at constructions, but callbacks should be set
              stop_token_base(),         // Stop token false, but stop token parameter should be set
              state_{nullptr} {}

        /// \brief Construct from a pointer to the shared state
        ///
        /// This constructor is private because we need to ensure the launching
        /// function appropriately sets this std::future handling these traits
        /// This is a function for async.
        ///
        /// \param s Future shared state
        explicit basic_future(const std::shared_ptr<shared_state<T>> &s) noexcept
            : lazy_continuations_base(), // No continuations at constructions, but callbacks should be set
              stop_token_base(),         // Stop token false, but stop token parameter should be set
              state_{std::move(s)} {}

        /// \brief Copy constructor for shared futures only.
        ///
        /// Inherited from base classes.
        ///
        /// \note The copy constructor only participates in overload resolution if `other` is shared
        ///
        /// \param other Another future used as source to initialize the shared state
        basic_future(const basic_future &other)
            : lazy_continuations_base(other), // Copy reference to continuations
              stop_token_base(other),         // Copy reference to stop state
              state_{other.state_} {
            static_assert(is_shared_v, "Copy constructor is only available for shared futures");
        }

        /// \brief Move constructor.
        ///
        /// Inherited from base classes.
        basic_future(basic_future && other) noexcept
            : lazy_continuations_base(std::move(other)), // Get control of continuations
              stop_token_base(std::move(other)),         // Move stop state
              join_{other.join_}, state_{other.state_} {
            other.state_.reset();
        }

        /// \brief Destructor
        ///
        /// The shared pointer will take care of decrementing the reference counter of the shared state, but we
        /// still take care of the special options:
        /// - We let stoppable futures set the stop token and wait.
        /// - We run the continuations if possible
        ~basic_future() {
            if constexpr (is_stoppable_v && !is_shared_v) {
                if (valid() && !is_ready()) {
                    stop_token_base::request_stop();
                }
            }
            wait_if_last();
            if constexpr (is_lazy_continuable_v) {
                detail::continuations_source &cs = lazy_continuations_base::continuations_source_;
                if (cs.run_possible()) {
                    cs.request_run();
                }
            }
        }

        /// \brief Copy assignment for shared futures only.
        ///
        /// Inherited from base classes.
        basic_future &operator=(const basic_future &other) {
            static_assert(is_shared_v, "Copy assignment is only available for shared futures");
            if (&other == this) {
                return *this;
            }
            wait_if_last(); // If this is the last shared future waiting for previous result, we wait
            lazy_continuations_base::operator=(other); // Copy reference to continuations
            stop_token_base::operator=(other);         // Copy reference to stop state
            join_ = other.join_;
            state_ = other.state_; // Make it point to the same shared state
            other.detach();        // Detach other to ensure it won't block at destruction
            return *this;
        }

        /// \brief Move assignment.
        ///
        /// Inherited from base classes.
        basic_future &operator=(basic_future &&other) noexcept {
            if (&other == this) {
                return *this;
            }
            wait_if_last(); // If this is the last shared future waiting for previous result, we wait
            lazy_continuations_base::operator=(std::move(other)); // Get control of continuations
            stop_token_base::operator=(std::move(other));         // Move stop state
            join_ = other.join_;
            state_ = other.state_; // Make it point to the same shared state
            other.state_.reset();
            other.detach(); // Detach other to ensure it won't block at destruction
            return *this;
        }
        /// @}

#if FUTURES_DOXYGEN
        /// \brief Emplace a function to the shared vector of continuations
        ///
        /// If properly setup (by async), this future holds the result from a function that runs these
        /// continuations after the main promise is fulfilled.
        /// However, if this future is already ready, we can just run the continuation right away.
        ///
        /// \note This function only participates in overload resolution if the future supports continuations
        ///
        /// \return True if the contination was emplaced without the using the default executor
        bool then(continuations_state::continuation_type &&fn);

        /// \brief Emplace a function to the shared vector of continuations
        ///
        /// If the function is ready, use the given executor instead of executing inline
        /// with the previous executor.
        ///
        /// \note This function only participates in overload resolution if the future supports continuations
        ///
        /// \tparam Executor
        /// \param ex
        /// \param fn
        /// \return
        template <class Executor> bool then(const Executor &ex, continuations_state::continuation_type &&fn);

        /// \brief Request the future to stop whatever task it's running
        ///
        /// \note This function only participates in overload resolution if the future supports stop tokens
        ///
        /// \return Whether the request was made
        bool request_stop() noexcept;

        /// \brief Get this future's stop source
        ///
        /// \note This function only participates in overload resolution if the future supports stop tokens
        ///
        /// \return The stop source
        [[nodiscard]] stop_source get_stop_source() const noexcept;

        /// \brief Get this future's stop token
        ///
        /// \note This function only participates in overload resolution if the future supports stop tokens
        ///
        /// \return The stop token
        [[nodiscard]] stop_token get_stop_token() const noexcept;
#endif

        /// \brief Wait until all futures have a valid result and retrieves it
        ///
        /// The behaviour depends on shared_based.
        decltype(auto) get() {
            if (!valid()) {
                throw future_uninitialized{};
            }
            if constexpr (is_shared_v) {
                return state_->get();
            } else {
                std::shared_ptr<shared_state<T>> tmp{};
                tmp.swap(state_);
                if constexpr (std::is_reference_v<T> || std::is_void_v<T>) {
                    return tmp->get();
                } else {
                    return T(std::move(tmp->get()));
                }
            }
        }

        /// \brief Create another future whose state is shared
        ///
        /// Create a shared variant of the current future type.
        /// If the current type is not shared, the object becomes invalid.
        /// If the current type is shared, the new object is equivalent to a copy.
        /// \return A shared variant of this future
        basic_future_shared_version_t share() {
            if (!valid()) {
                throw future_uninitialized{};
            }
            basic_future_shared_version_t res{[this]() {
                if constexpr (is_shared_v) {
                    return state_;
                } else {
                    return std::move(state_);
                }
            }()};
            res.join_ = join_;
            // Shares the source of continuations
            if constexpr (is_lazy_continuable_v) {
                res.continuations_source_ = this->continuations_source_;
            }
            // Shares the source for the stop token
            if constexpr (is_stoppable_v) {
                res.stop_source_ = this->stop_source_;
            }
            return res;
        }

        /// \brief Get exception pointer without throwing exception
        ///
        /// This extends std::future so that we can always check if the future threw an exception
        std::exception_ptr get_exception_ptr() {
            if (!valid()) {
                throw future_uninitialized{};
            }
            return state_->get_exception_ptr();
        }

        /// \brief Checks if the future refers to a shared state
        [[nodiscard]] bool valid() const { return nullptr != state_.get(); }

        /// \brief Blocks until the result becomes available.
        void wait() const {
            if (!valid()) {
                throw future_uninitialized{};
            }
            state_->wait();
        }

        /// \brief Waits for the result to become available.
        template <class Rep, class Period>
        [[nodiscard]] std::future_status wait_for(const std::chrono::duration<Rep, Period> &timeout_duration) const {
            if (!valid()) {
                throw future_uninitialized{};
            }
            return state_->wait_for(timeout_duration);
        }

        /// \brief Waits for the result to become available.
        template <class Clock, class Duration>
        std::future_status wait_until(const std::chrono::time_point<Clock, Duration> &timeout_time) const {
            if (!valid()) {
                throw future_uninitialized{};
            }
            return state_->wait_until(timeout_time);
        }

        /// \brief Checks if the shared state is ready
        [[nodiscard]] bool is_ready() const {
            if (!valid()) {
                throw std::future_error(std::future_errc::no_state);
            }
            return state_->is_ready();
        }

        /// \brief Tell this future not to join at destruction
        ///
        /// For safety, all futures join at destruction
        void detach() { join_ = false; }

        /// \brief Notify this condition variable when the future is ready
        notify_when_ready_handle notify_when_ready(std::condition_variable_any & cv) {
            if (!state_) {
                detail::throw_exception<future_uninitialized>();
            }
            return state_->notify_when_ready(cv);
        }

        /// \brief Cancel request to notify this condition variable when the future is ready
        void unnotify_when_ready(notify_when_ready_handle h) {
            if (!state_) {
                detail::throw_exception<future_uninitialized>();
            }
            return state_->unnotify_when_ready(h);
        }

        /// \brief Get a reference to the mutex in the underlying shared state
        std::mutex &mutex() {
            if (!state_) {
                detail::throw_exception<future_uninitialized>();
            }
            return state_->mutex();
        }

        /// \brief Checks if the shared state is ready
        [[nodiscard]] bool is_ready(std::unique_lock<std::mutex> & lk) const {
            if (!valid()) {
                throw std::future_error(std::future_errc::no_state);
            }
            return state_->is_ready(lk);
        }

      private:
        /// \name Private Functions
        /// @{

        void wait_if_last() const {
            if (join_ && valid() && (!is_ready())) {
                if constexpr (!is_shared_v) {
                    wait();
                } else /* constexpr */ {
                    if (1 == state_.use_count()) {
                        wait();
                    }
                }
            }
        }

        /// \subsection Private getters and setters
        void set_stop_source(const stop_source &ss) noexcept { stop_token_base::stop_source_ = ss; }
        void set_continuations_source(const detail::continuations_source &cs) noexcept {
            lazy_continuations_base::continuations_source_ = cs;
        }
        detail::continuations_source get_continuations_source() const noexcept {
            return lazy_continuations_base::continuations_source_;
        }

        /// @}

      private:
        /// \name Members
        /// @{
        bool join_{true};

        /// \brief Pointer to shared state
        std::shared_ptr<shared_state<T>> state_{};
        /// @}
    };

#ifndef FUTURES_DOXYGEN
    /// \name Define basic_future as a kind of future
    /// @{
    template <typename... Args> struct is_future<basic_future<Args...>> : std::true_type {};
    template <typename... Args> struct is_future<basic_future<Args...> &> : std::true_type {};
    template <typename... Args> struct is_future<basic_future<Args...> &&> : std::true_type {};
    template <typename... Args> struct is_future<const basic_future<Args...>> : std::true_type {};
    template <typename... Args> struct is_future<const basic_future<Args...> &> : std::true_type {};
    /// @}

    /// \name Define basic_future as a kind of future
    /// @{
    template <typename... Args> struct has_ready_notifier<basic_future<Args...>> : std::true_type {};
    template <typename... Args> struct has_ready_notifier<basic_future<Args...> &> : std::true_type {};
    template <typename... Args> struct has_ready_notifier<basic_future<Args...> &&> : std::true_type {};
    template <typename... Args> struct has_ready_notifier<const basic_future<Args...>> : std::true_type {};
    template <typename... Args> struct has_ready_notifier<const basic_future<Args...> &> : std::true_type {};
    /// @}

    /// \name Define basic_futures as supporting lazy continuations
    /// @{
    template <class T, class SH, class L, class ST> struct is_shared_future<basic_future<T, SH, L, ST>> : SH {};
    template <class T, class SH, class L, class ST> struct is_shared_future<basic_future<T, SH, L, ST> &> : SH {};
    template <class T, class SH, class L, class ST> struct is_shared_future<basic_future<T, SH, L, ST> &&> : SH {};
    template <class T, class SH, class L, class ST> struct is_shared_future<const basic_future<T, SH, L, ST>> : SH {};
    template <class T, class SH, class L, class ST> struct is_shared_future<const basic_future<T, SH, L, ST> &> : SH {};
    /// @}

    /// \name Define basic_futures as supporting lazy continuations
    /// @{
    template <class T, class SH, class L, class ST> struct is_lazy_continuable<basic_future<T, SH, L, ST>> : L {};
    template <class T, class SH, class L, class ST> struct is_lazy_continuable<basic_future<T, SH, L, ST> &> : L {};
    template <class T, class SH, class L, class ST> struct is_lazy_continuable<basic_future<T, SH, L, ST> &&> : L {};
    template <class T, class SH, class L, class ST> struct is_lazy_continuable<const basic_future<T, SH, L, ST>> : L {};
    template <class T, class SH, class L, class ST>
    struct is_lazy_continuable<const basic_future<T, SH, L, ST> &> : L {};
    /// @}

    /// \name Define basic_futures as having a stop token
    /// @{
    template <class T, class S, class L, class Stoppable>
    struct has_stop_token<basic_future<T, S, L, Stoppable>> : Stoppable {};
    template <class T, class S, class L, class Stoppable>
    struct has_stop_token<basic_future<T, S, L, Stoppable> &> : Stoppable {};
    template <class T, class S, class L, class Stoppable>
    struct has_stop_token<basic_future<T, S, L, Stoppable> &&> : Stoppable {};
    template <class T, class S, class L, class Stoppable>
    struct has_stop_token<const basic_future<T, S, L, Stoppable>> : Stoppable {};
    template <class T, class S, class L, class Stoppable>
    struct has_stop_token<const basic_future<T, S, L, Stoppable> &> : Stoppable {};
    /// @}

    /// \name Define basic_futures as being stoppable (not the same as having a stop token for other future types)
    /// @{
    template <class T, class S, class L, class Stoppable>
    struct is_stoppable<basic_future<T, S, L, Stoppable>> : Stoppable {};
    template <class T, class S, class L, class Stoppable>
    struct is_stoppable<basic_future<T, S, L, Stoppable> &> : Stoppable {};
    template <class T, class S, class L, class Stoppable>
    struct is_stoppable<basic_future<T, S, L, Stoppable> &&> : Stoppable {};
    template <class T, class S, class L, class Stoppable>
    struct is_stoppable<const basic_future<T, S, L, Stoppable>> : Stoppable {};
    template <class T, class S, class L, class Stoppable>
    struct is_stoppable<const basic_future<T, S, L, Stoppable> &> : Stoppable {};
    /** @} */
#endif

    /** @} */
    /** @} */
} // namespace futures

#endif // FUTURES_BASIC_FUTURE_H

// #include <futures/futures/promise.h>
//
//
// Copyright (c) alandefreitas 11/30/21.
// See accompanying file LICENSE
//

#ifndef FUTURES_PROMISE_H
#define FUTURES_PROMISE_H

// #include <memory>


// #include <futures/futures/detail/empty_base.h>
//
// Copyright (c) alandefreitas 12/10/21.
// See accompanying file LICENSE
//

#ifndef FUTURES_EMPTY_BASE_H
#define FUTURES_EMPTY_BASE_H

namespace futures::detail {
    /** \addtogroup futures Futures
     *  @{
     */

    /// \brief A convenience struct to refer to an empty type whenever we need one
    struct empty_value_type {};

    /// \brief A convenience struct to refer to an empty value whenever we need one
    inline constexpr empty_value_type empty_value = empty_value_type();

    /// \brief Represents a potentially empty base class for empty base class optimization
    ///
    /// We use the name maybe_empty for the base class that might be empty and empty_value_type / empty_value
    /// for the values we know to be empty.
    ///
    /// \tparam T The type represented by the base class
    /// \tparam BaseIndex An index to differentiate base classes in the same derived class. This might be
    /// important to ensure the same base class isn't inherited twice.
    /// \tparam E Indicates whether we should really instantiate the class (true when the class is not empty)
    template <class T, unsigned BaseIndex = 0, bool E = std::is_empty_v<T>> class maybe_empty {
      public:
        /// \brief The type this base class is effectively represent
        using value_type = T;

        /// \brief Initialize this potentially empty base with the specified values
        /// This will initialize the vlalue with the default constructor T(args...)
        template <class... Args> explicit maybe_empty(Args &&...args) : value_(std::forward<Args>(args)...) {}

        /// \brief Get the effective value this class represents
        /// This returns the underlying value represented here
        const T &get() const noexcept { return value_; }

        /// \brief Get the effective value this class represents
        /// This returns the underlying value represented here
        T &get() noexcept { return value_; }

      private:
        /// \brief The effective value representation when the value is not empty
        T value_;
    };

    /// \brief Represents a potentially empty base class, when it's effectively not empty
    ///
    /// \tparam T The type represented by the base class
    /// \tparam BaseIndex An index to differentiate base classes in the same derived class
    template <class T, unsigned BaseIndex> class maybe_empty<T, BaseIndex, true> : public T {
      public:
        /// \brief The type this base class is effectively represent
        using value_type = T;

        /// \brief Initialize this potentially empty base with the specified values
        /// This won't initialize any values but it will call the appropriate constructor T() in case we need
        /// its behaviour
        template <class... Args> explicit maybe_empty(Args &&...args) : T(std::forward<Args>(args)...) {}

        /// \brief Get the effective value this class represents
        /// Although the element takes no space, we can return a reference to whatever it represents so
        /// we can access its underlying functions
        const T &get() const noexcept { return *this; }

        /// \brief Get the effective value this class represents
        /// Although the element takes no space, we can return a reference to whatever it represents so
        /// we can access its underlying functions
        T &get() noexcept { return *this; }
    };

    /** @} */
} // namespace futures::detail

#endif // FUTURES_EMPTY_BASE_H

// #include <futures/futures/detail/shared_state.h>

// #include <futures/futures/detail/to_address.h>
//
// Copyright (c) alandefreitas 12/1/21.
// See accompanying file LICENSE
//

#ifndef FUTURES_TO_ADDRESS_H
#define FUTURES_TO_ADDRESS_H

/// \file
/// Replicate the C++20 to_address functionality in C++17

// #include <memory>


namespace futures::detail {
    /// \brief Obtain the address represented by p without forming a reference to the object pointed to by p
    /// This is the "fancy pointer" overload: If the expression std::pointer_traits<Ptr>::to_address(p) is
    /// well-formed, returns the result of that expression. Otherwise, returns std::to_address(p.operator->()).
    /// \tparam T Element type
    /// \param v Element pointer
    /// \return Element address
    template <class T> constexpr T *to_address(T *v) noexcept { return v; }

    /// \brief Obtain the address represented by p without forming a reference to the object pointed to by p
    /// This is the "raw pointer overload": If T is a function type, the program is ill-formed. Otherwise,
    /// returns p unmodified.
    /// \tparam T Element type
    /// \param v Raw pointer
    /// \return Element address
    template <class T> inline typename std::pointer_traits<T>::element_type *to_address(const T &v) noexcept {
        return to_address(v.operator->());
    }
} // namespace futures::detail

#endif // FUTURES_TO_ADDRESS_H


// #include <futures/futures/basic_future.h>


namespace futures {
    /** \addtogroup futures Futures
     *  @{
     */
    /** \addtogroup shared_state Shared State
     *
     * \brief Shared state objects
     *
     *  @{
     */

    /// \brief Common members to promises of all types
    ///
    /// This includes a pointer to the corresponding shared_state for the future and the functions
    /// to manage the promise.
    ///
    /// The specific promise specialization will only differ by their set_value functions.
    ///
    template <typename R> class promise_base {
      public:
        /// \brief Create the base promise with std::allocator
        ///
        /// Use std::allocator_arg tag to dispatch and select allocator aware constructor
        promise_base() : promise_base{std::allocator_arg, std::allocator<promise_base>{}} {}

        /// \brief Create a base promise setting the shared state with the specified allocator
        ///
        /// This function allocates memory for and allocates an initial promise_shared_state (the future value)
        /// with the specified allocator. This object is stored in the internal intrusive pointer as the
        /// future shared state.
        template <typename Allocator>
        promise_base(std::allocator_arg_t, Allocator alloc)
            : shared_state_(std::allocate_shared<shared_state<R>>(alloc)) {}

        /// \brief No copy constructor
        promise_base(promise_base const &) = delete;

        /// \brief Move constructor
        promise_base(promise_base &&other) noexcept
            : obtained_{other.obtained_}, shared_state_{std::move(other.shared_state_)} {
            other.obtained_ = false;
        }

        /// \brief No copy assignment
        promise_base &operator=(promise_base const &) = delete;

        /// \brief Move assignment
        promise_base &operator=(promise_base &&other) noexcept {
            if (this != &other) {
                promise_base tmp{std::move(other)};
                swap(tmp);
            }
            return *this;
        }

        /// \brief Destructor
        ///
        /// This promise owns the shared state, so we need to warn the shared state when it's destroyed.
        virtual ~promise_base() {
            if (shared_state_ && obtained_) {
                shared_state_->signal_owner_destroyed();
            }
        }

        /// \brief Gets a future that shares its state with this promise
        ///
        /// This function constructs a future object that shares its state with this promise.
        /// Because this library handles more than a single future type, the future type we want is
        /// a template parameter.
        ///
        /// This function expects future type constructors to accept pointers to shared states.
        template <class Future = futures::cfuture<R>> Future get_future() {
            if (obtained_) {
                throw future_already_retrieved{};
            }
            if (!shared_state_) {
                throw promise_uninitialized{};
            }
            obtained_ = true;
            return Future{shared_state_};
        }

        /// \brief Set the promise result as an exception
        /// \note The set_value operation is only available at the concrete derived class,
        /// where we know the class type
        void set_exception(std::exception_ptr p) {
            if (!shared_state_) {
                throw promise_uninitialized{};
            }
            shared_state_->set_exception(p);
        }

        /// \brief Set the promise result as an exception
        template <typename E
#ifndef FUTURES_DOXYGEN
                  ,
                  std::enable_if_t<std::is_base_of_v<std::exception, E>, int> = 0
#endif
                  >
        void set_exception(E e) {
            set_exception(std::make_exception_ptr(e));
        }

      protected:
        /// \brief Swap the value of two promises
        void swap(promise_base &other) noexcept {
            std::swap(obtained_, other.obtained_);
            shared_state_.swap(other.shared_state_);
        }

        /// \brief Intrusive pointer to the future corresponding to this promise
        constexpr std::shared_ptr<shared_state<R>> &get_shared_state() { return shared_state_; };

      private:
        /// \brief True if the future has already obtained the shared state
        bool obtained_{false};

        /// \brief Intrusive pointer to the future corresponding to this promise
        std::shared_ptr<shared_state<R>> shared_state_{};
    };

    /// \brief A shared state that will later be acquired by a future type
    ///
    /// The difference between the promise specializations is only in how they handle
    /// their set_value functions.
    ///
    /// \tparam R The shared state type
    template <typename R> class promise : public promise_base<R> {
      public:
        /// \brief Create the promise for type R
        using promise_base<R>::promise_base;

        /// \brief Copy and set the promise value so it can be obtained by the future
        /// \param value lvalue reference to the shared state value
        void set_value(R const &value) {
            if (!promise_base<R>::get_shared_state()) {
                throw promise_uninitialized{};
            }
            promise_base<R>::get_shared_state()->set_value(value);
        }

        /// \brief Move and set the promise value so it can be obtained by the future
        /// \param value rvalue reference to the shared state value
        void set_value(R &&value) {
            if (!promise_base<R>::get_shared_state()) {
                throw promise_uninitialized{};
            }
            promise_base<R>::get_shared_state()->set_value(std::move(value));
        }

        /// \brief Swap the value of two promises
        void swap(promise &other) noexcept { promise_base<R>::swap(other); }
    };

    /// \brief A shared state that will later be acquired by a future type
    template <typename R> class promise<R &> : public promise_base<R &> {
      public:
        /// \brief Create the promise for type R&
        using promise_base<R &>::promise_base;

        /// \brief Set the promise value so it can be obtained by the future
        void set_value(R &value) {
            if (!promise_base<R &>::get_shared_state()) {
                throw promise_uninitialized{};
            }
            promise_base<R &>::get_shared_state()->set_value(value);
        }

        /// \brief Swap the value of two promises
        void swap(promise &other) noexcept { promise_base<R &>::swap(other); }
    };

    /// \brief A shared state that will later be acquired by a future type
    template <> class promise<void> : public promise_base<void> {
      public:
        /// \brief Create the promise for type void
        using promise_base<void>::promise_base;

        /// \brief Set the promise value, so it can be obtained by the future
        void set_value() { // NOLINT(readability-make-member-function-const)
            if (!promise_base<void>::get_shared_state()) {
                throw promise_uninitialized{};
            }
            promise_base<void>::get_shared_state()->set_value();
        }

        /// \brief Swap the value of two promises
        void swap(promise &other) noexcept { promise_base<void>::swap(other); }
    };

    /// \brief Swap the value of two promises
    template <typename R> void swap(promise<R> &l, promise<R> &r) noexcept { l.swap(r); }

    /** @} */
    /** @} */
} // namespace futures

#endif // FUTURES_PROMISE_H

// #include <futures/futures/packaged_task.h>
//
// Copyright (c) alandefreitas 11/30/21.
// See accompanying file LICENSE
//

#ifndef FUTURES_PACKAGED_TASK_H
#define FUTURES_PACKAGED_TASK_H

// #include <futures/futures/basic_future.h>

// #include <futures/futures/detail/shared_task.h>
//
// Copyright (c) alandefreitas 12/1/21.
// See accompanying file LICENSE
//

#ifndef FUTURES_SHARED_TASK_H
#define FUTURES_SHARED_TASK_H

// #include <futures/futures/detail/empty_base.h>

// #include <futures/futures/detail/shared_state.h>

// #include <futures/futures/detail/to_address.h>


namespace futures::detail {
    /** \addtogroup futures Futures
     *  @{
     */

    /// \brief Members common to shared tasks
    ///
    /// While the main purpose of shared_state_base is to differentiate the versions of `set_value`, the main purpose
    /// of this task base class is to nullify the function type and allocator from the concrete task implementation
    /// in the final packaged task.
    ///
    /// \tparam R Type returned by the task callable
    /// \tparam Args Argument types to run the task callable
    template <typename R, typename... Args> class shared_task_base : public shared_state<R> {
      public:
        /// \brief Virtual task destructor
        virtual ~shared_task_base() = default;

        /// \brief Virtual function to run the task with its Args
        /// \param args Arguments
        virtual void run(Args &&...args) = 0;

        /// \brief Reset the state
        ///
        /// This function returns a new pointer to this shared task where we reallocate everything
        ///
        /// \return New pointer to a shared_task
        virtual std::shared_ptr<shared_task_base> reset() = 0;
    };

    /// \brief A shared task object, that also stores the function to create the shared state
    ///
    /// A shared_task extends the shared state with a task. A task is an extension of and analogous with shared states.
    /// The main difference is that tasks also define a function that specify how to create the state, with the `run`
    /// function.
    ///
    /// In practice, a shared_task are to a packaged_task what a shared state is to a promise.
    ///
    /// \tparam R Type returned by the task callable
    /// \tparam Args Argument types to run the task callable
    template <typename Fn, typename Allocator, typename R, typename... Args>
    class shared_task : public shared_task_base<R, Args...>
#ifndef FUTURES_DOXYGEN
        ,
                        public maybe_empty<Fn>,
                        public maybe_empty<
                            /* allocator_type */ typename std::allocator_traits<Allocator>::template rebind_alloc<
                                shared_task<Fn, Allocator, R, Args...>>>
#endif
    {
      public:
        /// \brief Allocator used to allocate this task object type
        using allocator_type = typename std::allocator_traits<Allocator>::template rebind_alloc<shared_task>;

        /// \brief Construct a task object for the specified allocator and function, copying the function
        shared_task(const allocator_type &alloc, const Fn &fn)
            : shared_task_base<R, Args...>{}, maybe_empty<Fn>{fn}, maybe_empty<allocator_type>{alloc} {}

        /// \brief Construct a task object for the specified allocator and function, moving the function
        shared_task(const allocator_type &alloc, Fn &&fn)
            : shared_task_base<R, Args...>{}, maybe_empty<Fn>{std::move(fn)}, maybe_empty<allocator_type>{alloc} {}

        /// \brief No copy constructor
        shared_task(shared_task const &) = delete;

        /// \brief No copy assignment
        shared_task &operator=(shared_task const &) = delete;

        /// \brief Virtual shared task destructor
        virtual ~shared_task() = default;

        /// \brief Run the task function with the given arguments and use the result to set the shared state value
        /// \param args Arguments
        void run(Args &&...args) final {
            try {
                if constexpr (std::is_same_v<R, void>) {
                    std::apply(fn(), std::make_tuple(std::forward<Args>(args)...));
                    this->set_value();
                } else {
                    this->set_value(std::apply(fn(), std::make_tuple(std::forward<Args>(args)...)));
                }
            } catch (...) {
                this->set_exception(std::current_exception());
            }
        }

        /// \brief Reallocate and reconstruct a task object
        ///
        /// This constructs a task object of same type from scratch.
        typename std::shared_ptr<shared_task_base<R, Args...>> reset() final {
            return std::allocate_shared<shared_task>(alloc(), alloc(), std::move(fn()));
        }

      private:
        /// @name Maybe-empty internal members
        /// @{

        /// \brief Internal function object representing the task function
        const Fn &fn() const { return maybe_empty<Fn>::get(); }

        /// \brief Internal function object representing the task function
        Fn &fn() { return maybe_empty<Fn>::get(); }

        /// \brief Internal function object representing the task function
        const allocator_type &alloc() const { return maybe_empty<allocator_type>::get(); }

        /// \brief Internal function object representing the task function
        allocator_type &alloc() { return maybe_empty<allocator_type>::get(); }

        /// @}
    };

    /** @} */ // \addtogroup futures Futures
} // namespace futures::detail

#endif // FUTURES_SHARED_TASK_H


namespace futures {
    /** \addtogroup futures Futures
     *  @{
     */
    /** \addtogroup shared_state Shared State
     *  @{
     */

#ifndef FUTURES_DOXYGEN
    /// \brief Undefined packaged task class
    template <typename Signature> class packaged_task;
#endif

    /// \brief A packaged task that sets a shared state when done
    ///
    /// A packaged task holds a task to be executed and a shared state for its result.
    ///
    /// It's very similar to a promise where the shared state is replaced by a shared task.
    ///
    /// \tparam R Return type
    /// \tparam Args Task arguments
#ifndef FUTURES_DOXYGEN
    template <typename R, typename... Args>
#else
    template <typename Signature>
#endif
    class packaged_task<R(Args...)> {
      public:
        /// \brief Constructs a std::packaged_task object with no task and no shared state
        packaged_task() = default;

        /// \brief Construct a packaged task from a function with the default std allocator
        ///
        /// \par Constraints
        /// This constructor only participates in overload resolution if Fn is not a packaged task itself.
        ///
        /// \tparam Fn Function type
        /// \param fn The callable target to execute
        template <typename Fn
#ifndef FUTURES_DOXYGEN
                  ,
                  typename = std::enable_if_t<!std::is_base_of_v<packaged_task, typename std::decay_t<Fn>>>
#endif
                  >
        explicit packaged_task(Fn &&fn)
            : packaged_task{std::allocator_arg, std::allocator<packaged_task>{}, std::forward<Fn>(fn)} {
        }

        /// \brief Constructs a std::packaged_task object with a shared state and a copy of the task
        ///
        /// This function constructs a std::packaged_task object with a shared state and a copy of the task, initialized
        /// with std::forward<Fn>(fn). It uses the provided allocator to allocate memory necessary to store the task.
        ///
        /// \par Constraints
        /// This constructor does not participate in overload resolution if std::decay<Fn>::type is the same type as
        /// std::packaged_task<R(ArgTypes...)>.
        ///
        /// \tparam Fn Function type
        /// \tparam Allocator Allocator type
        /// \param alloc The allocator to use when storing the task
        /// \param fn The callable target to execute
        template <typename Fn, typename Allocator
#ifndef FUTURES_DOXYGEN
                  ,
                  typename = std::enable_if_t<!std::is_base_of_v<packaged_task, typename std::decay_t<Fn>>>
#endif
                  >
        explicit packaged_task(std::allocator_arg_t, const Allocator &alloc, Fn &&fn) {
            task_ = std::allocate_shared<detail::shared_task<std::decay_t<Fn>, Allocator, R, Args...>>(
                alloc, alloc, std::forward<Fn>(fn));
        }

        /// \brief The copy constructor is deleted, std::packaged_task is move-only.
        packaged_task(packaged_task const &) = delete;

        /// \brief Constructs a std::packaged_task with the shared state and task formerly owned by other
        packaged_task(packaged_task &&other) noexcept
            : future_retrieved_{other.future_retrieved_}, task_{std::move(other.task_)} {
            other.future_retrieved_ = false;
        }

        /// \brief The copy assignment is deleted, std::packaged_task is move-only.
        packaged_task &operator=(packaged_task const &) = delete;

        /// \brief Assigns a std::packaged_task with the shared state and task formerly owned by other
        packaged_task &operator=(packaged_task &&other) noexcept {
            if (this != &other) {
                packaged_task tmp{std::move(other)};
                swap(tmp);
            }
            return *this;
        }

        /// \brief Destructs the task object
        ~packaged_task() {
            if (task_ && future_retrieved_) {
                task_->signal_owner_destroyed();
            }
        }

        /// \brief Checks if the task object has a valid function
        ///
        /// \return true if *this has a shared state, false otherwise
        [[nodiscard]] bool valid() const noexcept { return task_ != nullptr; }

        /// \brief Swaps two task objects
        ///
        /// This function exchanges the shared states and stored tasks of *this and other
        ///
        /// \param other packaged task whose state to swap with
        void swap(packaged_task &other) noexcept {
            std::swap(future_retrieved_, other.future_retrieved_);
            task_.swap(other.task_);
        }

        /// \brief Returns a future object associated with the promised result
        ///
        /// This function constructs a future object that shares its state with this promise
        /// Because this library handles more than a single future type, the future type we want is
        /// a template parameter. This function expects future type constructors to accept pointers
        /// to shared states.
        template <class Future = cfuture<R>> Future get_future() {
            if (future_retrieved_) {
                throw future_already_retrieved{};
            }
            if (!valid()) {
                throw packaged_task_uninitialized{};
            }
            future_retrieved_ = true;
            return Future{std::static_pointer_cast<shared_state<R>>(task_)};
        }

        /// \brief Executes the function and set the shared state
        ///
        /// Calls the stored task with args as the arguments. The return value of the task or any exceptions thrown are
        /// stored in the shared state
        /// The shared state is made ready and any threads waiting for this are unblocked.
        ///
        /// \param args the parameters to pass on invocation of the stored task
        void operator()(Args... args) {
            if (!valid()) {
                throw packaged_task_uninitialized{};
            }
            task_->run(std::forward<Args>(args)...);
        }

        /// \brief Resets the shared state abandoning any stored results of previous executions
        ///
        /// Resets the state abandoning the results of previous executions. A new shared state is constructed.
        /// Equivalent to *this = packaged_task(std::move(f)), where f is the stored task.
        void reset() {
            if (!valid()) {
                throw packaged_task_uninitialized{};
            }
            task_ = task_->reset();
            future_retrieved_ = false;
        }

      private:
        /// \brief True if the corresponding future has already been retrieved
        bool future_retrieved_{false};

        /// \brief The function this task should execute
        std::shared_ptr<detail::shared_task_base<R, Args...>> task_{};
    };

    /// \brief Specializes the std::swap algorithm
    template <typename Signature> void swap(packaged_task<Signature> &l, packaged_task<Signature> &r) noexcept {
        l.swap(r);
    }

    /** @} */
    /** @} */
} // namespace futures

#endif // FUTURES_PACKAGED_TASK_H

// #include <futures/futures/async.h>
//
// Copyright (c) alandefreitas 11/30/21.
// See accompanying file LICENSE
//

#ifndef FUTURES_ASYNC_H
#define FUTURES_ASYNC_H

// #include <futures/executor/inline_executor.h>
//
// Created by Alan Freitas on 8/17/21.
//

#ifndef FUTURES_INLINE_EXECUTOR_H
#define FUTURES_INLINE_EXECUTOR_H

// #include <futures/config/asio_include.h>

// #include <futures/executor/is_executor.h>


namespace futures {
    /** \addtogroup executors Executors
     *  @{
     */

    /// \brief A minimal executor that runs anything in the local thread in the default context
    ///
    /// Although simple, it needs to meet the executor requirements:
    /// - Executor concept
    /// - Ability to query the execution context
    ///     - Result being derived from execution_context
    /// - The execute function
    /// \see https://think-async.com/Asio/asio-1.18.2/doc/asio/std_executors.html
    struct inline_executor {
        asio::execution_context *context_{nullptr};

        constexpr bool operator==(const inline_executor &other) const noexcept { return context_ == other.context_; }

        constexpr bool operator!=(const inline_executor &other) const noexcept { return !(*this == other); }

        [[nodiscard]] constexpr asio::execution_context &query(asio::execution::context_t) const noexcept {
            return *context_;
        }

        static constexpr asio::execution::blocking_t::never_t query(asio::execution::blocking_t) noexcept {
            return asio::execution::blocking_t::never;
        }

        template <class F> void execute(F f) const { f(); }
    };

    /// \brief Get the inline execution context
    asio::execution_context &inline_execution_context() {
        static asio::execution_context context;
        return context;
    }

    /// \brief Make an inline executor object
    inline_executor make_inline_executor() {
        asio::execution_context &ctx = inline_execution_context();
        return inline_executor{&ctx};
    }

    /** @} */  // \addtogroup executors Executors
} // namespace futures

#ifdef FUTURES_USE_BOOST_ASIO
namespace boost {
#endif
    namespace asio {
        /// \brief Ensure asio and our internal functions see inline_executor as an executor
        ///
        /// This traits ensures asio and our internal functions see inline_executor as an executor,
        /// as asio traits don't always work.
        ///
        /// This is quite a workaround until things don't improve with our executor traits.
        ///
        /// Ideally, we would have our own executor traits and let asio pick up from those.
        ///
        template <> class is_executor<futures::inline_executor> : public std::true_type {};

        namespace traits {
#if !defined(ASIO_HAS_DEDUCED_EXECUTE_MEMBER_TRAIT)
            template <typename F> struct execute_member<futures::inline_executor, F> {
                static constexpr bool is_valid = true;
                static constexpr bool is_noexcept = true;
                typedef void result_type;
            };

#endif // !defined(ASIO_HAS_DEDUCED_EXECUTE_MEMBER_TRAIT)
#if !defined(ASIO_HAS_DEDUCED_EQUALITY_COMPARABLE_TRAIT)
            template <> struct equality_comparable<futures::inline_executor> {
                static constexpr bool is_valid = true;
                static constexpr bool is_noexcept = true;
            };

#endif // !defined(ASIO_HAS_DEDUCED_EQUALITY_COMPARABLE_TRAIT)
#if !defined(ASIO_HAS_DEDUCED_QUERY_MEMBER_TRAIT)
            template <> struct query_member<futures::inline_executor, asio::execution::context_t> {
                static constexpr bool is_valid = true;
                static constexpr bool is_noexcept = true;
                typedef asio::execution_context &result_type;
            };

#endif // !defined(ASIO_HAS_DEDUCED_QUERY_MEMBER_TRAIT)
#if !defined(ASIO_HAS_DEDUCED_QUERY_STATIC_CONSTEXPR_MEMBER_TRAIT)
            template <typename Property>
            struct query_static_constexpr_member<
                futures::inline_executor, Property,
                typename enable_if<std::is_convertible<Property, asio::execution::blocking_t>::value>::type> {
                static constexpr bool is_valid = true;
                static constexpr bool is_noexcept = true;
                typedef asio::execution::blocking_t::never_t result_type;
                static constexpr result_type value() noexcept { return result_type(); }
            };

#endif // !defined(ASIO_HAS_DEDUCED_QUERY_STATIC_CONSTEXPR_MEMBER_TRAIT)

        } // namespace traits
    }     // namespace asio
#ifdef FUTURES_USE_BOOST_ASIO
}
#endif

#endif // FUTURES_INLINE_EXECUTOR_H


// #include <futures/futures/detail/empty_base.h>

// #include <futures/futures/detail/traits/async_result_of.h>
//
// Copyright (c) alandefreitas 12/3/21.
// See accompanying file LICENSE
//

#ifndef FUTURES_ASYNC_RESULT_OF_H
#define FUTURES_ASYNC_RESULT_OF_H

// #include <futures/futures/detail/traits/async_result_value_type.h>
//
// Copyright (c) alandefreitas 12/3/21.
// See accompanying file LICENSE
//

#ifndef FUTURES_ASYNC_RESULT_VALUE_TYPE_H
#define FUTURES_ASYNC_RESULT_VALUE_TYPE_H

// #include <futures/futures/detail/traits/type_member_or_void.h>
//
// Copyright (c) alandefreitas 12/3/21.
// See accompanying file LICENSE
//

#ifndef FUTURES_TYPE_MEMBER_OR_VOID_H
#define FUTURES_TYPE_MEMBER_OR_VOID_H

// #include <type_traits>


namespace futures::detail {
    /** \addtogroup futures Futures
     *  @{
     */

    /// \brief Return T::type or void as a placeholder if T::type doesn't exist
    /// This class is meant to avoid errors in std::conditional
    template <class, class = void> struct type_member_or_void { using type = void; };
    template <class T> struct type_member_or_void<T, std::void_t<typename T::type>> {
        using type = typename T::type;
    };
    template <class T> using type_member_or_void_t = typename type_member_or_void<T>::type;

    /** @} */
}

#endif // FUTURES_TYPE_MEMBER_OR_VOID_H

// #include <futures/futures/stop_token.h>


namespace futures::detail {
    /** \addtogroup futures Futures
     *  @{
     */

    /// \brief The return type of a callable given to futures::async with the Args...
    /// This is the value type of the future object returned by async.
    /// In typical implementations this is usually the same as result_of_t<Function, Args...>.
    /// However, our implementation is a little different as the stop_token is provided by the
    /// async function and is thus not a part of Args, so both paths need to be considered.
    template <typename Function, typename... Args>
    using async_result_value_type =
        std::conditional<std::is_invocable_v<std::decay_t<Function>, stop_token, Args...>,
                           type_member_or_void_t<std::invoke_result<std::decay_t<Function>, stop_token, Args...>>,
                           type_member_or_void_t<std::invoke_result<std::decay_t<Function>, Args...>>>;

    template <typename Function, typename... Args>
    using async_result_value_type_t = typename async_result_value_type<Function, Args...>::type;

    /** @} */
}


#endif // FUTURES_ASYNC_RESULT_VALUE_TYPE_H

// #include <futures/futures/basic_future.h>


namespace futures::detail {
    /** \addtogroup futures Futures
     *  @{
     */

    /// \brief The future type that results from calling async with a function <Function, Args...>
    /// This is the future type returned by async. In typical implementations this is usually
    /// the same as future<result_of_t<Function, Args...>>. However, our implementation is a
    /// little different as the stop_token is provided by the async function and can thus
    /// influence the resulting future type, so both paths need to be considered.
    /// Whenever we call async, we return a future with lazy continuations by default because
    /// we don't know if the user will need efficient continuations. Also, when the function
    /// expects a stop token, we return a jfuture.
    template <typename Function, typename... Args>
    using async_result_of = std::conditional<std::is_invocable_v<std::decay_t<Function>, stop_token, Args...>,
                                               jcfuture<async_result_value_type_t<Function, Args...>>,
                                               cfuture<async_result_value_type_t<Function, Args...>>>;

    template <typename Function, typename... Args>
    using async_result_of_t = typename async_result_of<Function, Args...>::type;

    /** @} */
} // namespace futures

#endif // FUTURES_ASYNC_RESULT_OF_H


// #include <futures/futures/await.h>
//
// Copyright (c) alandefreitas 12/3/21.
// See accompanying file LICENSE
//

#ifndef FUTURES_AWAIT_H
#define FUTURES_AWAIT_H

// #include <type_traits>


namespace futures {
    /** \addtogroup futures Futures
     *  @{
     */

    /** \addtogroup waiting Waiting
     *
     * \brief Basic function to wait for futures
     *
     * This module defines a variety of auxiliary functions to wait for futures.
     *
     *  @{
     */

    /// \brief Very simple syntax sugar for types that pass the @ref is_future concept
    ///
    /// This syntax is most useful for cases where we are immediately requesting the future result.
    ///
    /// The function also makes the syntax optionally a little closer to languages such as javascript.
    ///
    /// \tparam Future A future type
    ///
    /// \return The result of the future object
    template <typename Future
#ifndef FUTURES_DOXYGEN
              ,
              std::enable_if_t<is_future_v<std::decay_t<Future>>, int> = 0
#endif
              >
    decltype(auto) await(Future &&f) {
        return f.get();
    }

    /** @} */
    /** @} */
} // namespace futures

#endif // FUTURES_AWAIT_H

// #include <futures/futures/launch.h>
//
// Copyright (c) alandefreitas 12/3/21.
// See accompanying file LICENSE
//

#ifndef FUTURES_LAUNCH_H
#define FUTURES_LAUNCH_H

namespace futures {
    /** \addtogroup futures Futures
     *  @{
     */
    /** \addtogroup launch Launch
     *
     * \brief Functions and policies for launching asynchronous tasks
     *
     * This module defines functions for conveniently launching asynchronous tasks and
     * policies to determine how executors should handle these tasks.
     *
     *  @{
     */
    /** \addtogroup launch-policies Launch Policies
     *
     * \brief Launch policies for asynchronous tasks
     *
     * Launch policies determine how executors should launch and handle a task.
     *
     *  @{
     */

    /// \brief Specifies the launch policy for a task executed by the @ref futures::async function
    ///
    /// std::async creates a new thread for each asynchronous operation, which usually entails in only
    /// two execution policies: new thread or inline. Because futures::async use executors, there are
    /// many more policies and ways to use these executors beyond yes/no.
    ///
    /// Most of the time, we want the executor/post policy for executors. So as the @ref async function
    /// also accepts executors directly, this option can often be ignored, and is here mostly here for
    /// compatibility with the std::async.
    ///
    /// When only the policy is provided, async will try to generate the proper executor for that policy.
    /// When the executor and the policy is provided, we might only have some conflict for the deferred
    /// policy, which does not use an executor in std::launch. In the context of executors, the deferred
    /// policy means the function is only posted to the executor when its result is requested.
    enum class launch {
        /// no policy
        none = 0b0000'0000,
        /// execute on a new thread regardless of executors (same as std::async::async)
        new_thread = 0b0000'0001,
        /// execute on a new thread regardless of executors (same as std::async::async)
        async = 0b0000'0001,
        /// execute on the calling thread when result is requested (same as std::async::deferred)
        deferred = 0b0000'0010,
        /// execute on the calling thread when result is requested (same as std::async::deferred)
        lazy = 0b0000'0010,
        /// inherit from context
        inherit = 0b0000'0100,
        /// execute on the calling thread now (uses inline executor)
        inline_now = 0b0000'1000,
        /// execute on the calling thread now (uses inline executor)
        sync = 0b0000'1000,
        /// enqueue task in the executor
        post = 0b0001'0000,
        /// run immediately if inside the executor
        executor = 0b0001'0000,
        /// run immediately if inside the executor
        dispatch = 0b0010'0000,
        /// run immediately if inside the executor
        executor_now = 0b0010'0000,
        /// enqueue task for later in the executor
        executor_later = 0b0100'0000,
        /// enqueue task for later in the executor
        defer = 0b0100'0000,
        /// both async and deferred are OK
        any = async | deferred
    };

    /// \brief operator & for launch policies
    ///
    /// \param x left-hand side operand
    /// \param y right-hand side operand
    /// \return A launch policy that attempts to satisfy both policies
    constexpr launch operator&(launch x, launch y) {
        return static_cast<launch>(static_cast<int>(x) & static_cast<int>(y));
    }

    /// \brief operator | for launch policies
    ///
    /// \param x left-hand side operand
    /// \param y right-hand side operand
    /// \return A launch policy that attempts to satisfy any of the policies
    constexpr launch operator|(launch x, launch y) {
        return static_cast<launch>(static_cast<int>(x) | static_cast<int>(y));
    }

    /// \brief operator ^ for launch policies
    ///
    /// \param x left-hand side operand
    /// \param y right-hand side operand
    /// \return A launch policy that attempts to satisfy any policy set in only one of them
    constexpr launch operator^(launch x, launch y) {
        return static_cast<launch>(static_cast<int>(x) ^ static_cast<int>(y));
    }

    /// \brief operator ~ for launch policies
    ///
    /// \param x left-hand side operand
    /// \return A launch policy that attempts to satisfy the opposite of the policies set
    constexpr launch operator~(launch x) { return static_cast<launch>(~static_cast<int>(x)); }

    /// \brief operator &= for launch policies
    ///
    /// \param x left-hand side operand
    /// \param y right-hand side operand
    /// \return A reference to `x`
    constexpr launch &operator&=(launch &x, launch y) { return x = x & y; }

    /// \brief operator |= for launch policies
    ///
    /// \param x left-hand side operand
    /// \param y right-hand side operand
    /// \return A reference to `x`
    constexpr launch &operator|=(launch &x, launch y) { return x = x | y; }

    /// \brief operator ^= for launch policies
    ///
    /// \param x left-hand side operand
    /// \param y right-hand side operand
    /// \return A reference to `x`
    constexpr launch &operator^=(launch &x, launch y) { return x = x ^ y; }
    /** @} */
    /** @} */
    /** @} */
} // namespace futures

#endif // FUTURES_LAUNCH_H


namespace futures {
    /** \addtogroup futures Futures
     *  @{
     */
    /** \addtogroup launch Launch
     *  @{
     */
    /** \addtogroup launch-algorithms Launch Algorithms
     *
     * \brief Function to schedule and launch future tasks
     *
     * This module contains function we can use to launch and schedule tasks. If possible, tasks should be
     * scheduled lazily instead of launched eagerly to avoid a race between the task and its dependencies.
     *
     * When tasks are scheduled eagerly, the function @ref async provides an alternatives to launch tasks on specific
     * executors instead of creating a new thread for each asynchronous task.
     *
     *  @{
     */

    namespace detail {
        enum class schedule_future_policy {
            /// \brief Internal tag indicating the executor should post the execution
            post,
            /// \brief Internal tag indicating the executor should dispatch the execution
            dispatch,
            /// \brief Internal tag indicating the executor should defer the execution
            defer,
        };

        /// \brief A single trait to validate and constraint futures::async input types
        template <class Executor, class Function, typename... Args>
        using is_valid_async_input = std::disjunction<is_executor_then_function<Executor, Function, Args...>,
                                                      is_executor_then_stoppable_function<Executor, Function, Args...>>;

        /// \brief A single trait to validate and constraint futures::async input types
        template <class Executor, class Function, typename... Args>
        constexpr bool is_valid_async_input_v = is_valid_async_input<Executor, Function, Args...>::value;

        /// \brief Create a new stop source for the new shared state
        template <bool expects_stop_token> auto create_stop_source() {
            if constexpr (expects_stop_token) {
                stop_source ss;
                return std::make_pair(ss, ss.get_token());
            } else {
                return std::make_pair(empty_value, empty_value);
            }
        }

        /// This function is defined as a functor to facilitate friendship in basic_future
        struct async_future_scheduler {
            /// This is a functor to fulfill a promise in a packaged task
            /// Handle to fulfill promise. Asio requires us to create a handle because
            /// callables need to be *copy constructable*. Continuations also require us to create an
            /// extra handle because we need to run them after the function is over.
            template <class StopToken, class Function, class Task, class... Args>
            class promise_fulfill_handle
                : public maybe_empty<StopToken>, // stop token for stopping the process is represented in base class
                  public maybe_empty<std::tuple<Args...>> // arguments bound to the function to fulfill the promise also
                                                          // have empty base opt
            {
              public:
                promise_fulfill_handle(Task &&pt, std::tuple<Args...> &&args, continuations_source cs,
                                       const StopToken &st)
                    : maybe_empty<StopToken>(st), maybe_empty<std::tuple<Args...>>(std::move(args)),
                      pt_(std::forward<Task>(pt)), continuations_(std::move(cs)) {}

                void operator()() {
                    // Fulfill promise
                    if constexpr (std::is_invocable_v<Function, StopToken, Args...>) {
                        std::apply(pt_, std::tuple_cat(std::make_tuple(token()), std::move(args())));
                    } else {
                        std::apply(pt_, std::move(args()));
                    }
                    // Run future continuations
                    continuations_.request_run();
                }

                /// \brief Get stop token from the base class as function for convenience
                const StopToken &token() const { return maybe_empty<StopToken>::get(); }

                /// \brief Get stop token from the base class as function for convenience
                StopToken &token() { return maybe_empty<StopToken>::get(); }

                /// \brief Get args from the base class as function for convenience
                const std::tuple<Args...> &args() const { return maybe_empty<std::tuple<Args...>>::get(); }

                /// \brief Get args from the base class as function for convenience
                std::tuple<Args...> &args() { return maybe_empty<std::tuple<Args...>>::get(); }

              private:
                /// \brief Task we need to fulfill the promise and its shared state
                Task pt_;

                /// \brief Continuation source for next futures
                continuations_source continuations_;
            };

            /// \brief Schedule the function in the executor
            /// This is the internal function async uses to finally schedule the function after setting the
            /// default parameters and converting policies into scheduling strategies.
            template <typename Executor, typename Function, typename... Args
#ifndef FUTURES_DOXYGEN
                      ,
                      std::enable_if_t<is_valid_async_input_v<Executor, Function, Args...>, int> = 0
#endif
                      >
            async_result_of_t<Function, Args...> operator()(schedule_future_policy policy, const Executor &ex,
                                                            Function &&f, Args &&...args) const {
                using future_value_type = async_result_value_type_t<Function, Args...>;
                using future_type = async_result_of_t<Function, Args...>;

                // Shared sources
                constexpr bool expects_stop_token = std::is_invocable_v<Function, stop_token, Args...>;
                auto [s_source, s_token] = create_stop_source<expects_stop_token>();
                continuations_source cs;

                // Set up shared state
                using packaged_task_type =
                    std::conditional_t<expects_stop_token,
                                       packaged_task<future_value_type(stop_token, std::decay_t<Args>...)>,
                                       packaged_task<future_value_type(std::decay_t<Args>...)>>;
                packaged_task_type pt{std::forward<Function>(f)};
                future_type result{pt.template get_future<future_type>()};
                result.set_continuations_source(cs);
                if constexpr (expects_stop_token) {
                    result.set_stop_source(s_source);
                }
                promise_fulfill_handle<std::decay_t<decltype(s_token)>, Function, packaged_task_type, Args...>
                    fulfill_promise(std::move(pt), std::make_tuple(std::forward<Args>(args)...), cs, s_token);

                // Fire-and-forget: Post a handle running the complete function to the executor
                switch (policy) {
                case schedule_future_policy::dispatch:
                    asio::dispatch(ex, std::move(fulfill_promise));
                    break;
                case schedule_future_policy::defer:
                    asio::defer(ex, std::move(fulfill_promise));
                    break;
                default:
                    asio::post(ex, std::move(fulfill_promise));
                    break;
                }
                return result;
            }
        };
        constexpr async_future_scheduler schedule_future;
    } // namespace detail

    /// \brief Launch an asynchronous task with the specified executor and policy
    ///
    /// \par Example
    /// \code
    /// auto f = async([]() { return 2; });
    /// std::cout << f.get() << std::endl; // 2
    /// \endcode
    ///
    /// \see
    ///      \ref basic_future
    ///
    /// \tparam Executor Executor from an execution context
    /// \tparam Function A callable object
    /// \tparam Args Arguments for the Function
    ///
    /// \param policy Launch policy
    /// \param ex Executor
    /// \param f Function to execute
    /// \param args Function arguments
    ///
    /// \return A future object with the function results
    template <typename Executor, typename Function, typename... Args
#ifndef FUTURES_DOXYGEN
              ,
              std::enable_if_t<detail::is_valid_async_input_v<Executor, Function, Args...>, int> = 0
#endif
              >
#ifndef FUTURES_DOXYGEN
    decltype(auto)
#else
    __implementation_defined__
#endif
    async(launch policy, const Executor &ex, Function &&f, Args &&...args) {
        // Unwrap policies
        const bool new_thread_policy = (policy & launch::new_thread) == launch::new_thread;
        const bool deferred_policy = (policy & launch::deferred) == launch::deferred;
        const bool inline_now_policy = (policy & launch::inline_now) == launch::inline_now;
        const bool executor_policy = (policy & launch::executor) == launch::executor;
        const bool executor_now_policy = (policy & launch::executor_now) == launch::executor_now;
        const bool executor_later_policy = (policy & launch::executor_later) == launch::executor_later;

        // Define executor
        const bool use_default_executor = executor_policy && executor_now_policy && executor_later_policy;
        const bool use_new_thread_executor = (!use_default_executor) && new_thread_policy;
        const bool use_inline_later_executor = (!use_default_executor) && deferred_policy;
        const bool use_inline_executor = (!use_default_executor) && inline_now_policy;
        const bool no_executor_defined =
            !(use_default_executor || use_new_thread_executor || use_inline_later_executor || use_inline_executor);

        // Define schedule policy
        detail::schedule_future_policy schedule_policy;
        if (use_default_executor || no_executor_defined) {
            if (executor_now_policy || inline_now_policy) {
                schedule_policy = detail::schedule_future_policy::dispatch;
            } else if (executor_later_policy || deferred_policy) {
                schedule_policy = detail::schedule_future_policy::defer;
            } else {
                schedule_policy = detail::schedule_future_policy::post;
            }
        } else {
            schedule_policy = detail::schedule_future_policy::post;
        }

        return detail::schedule_future(schedule_policy, ex, std::forward<Function>(f), std::forward<Args>(args)...);
    }

    /// \brief Launch a task with a custom executor instead of policies.
    ///
    /// This version of the async function will always use the specified executor instead of
    /// creating a new thread.
    ///
    /// If no executor is provided, then the function is run in a default executor created from
    /// the default thread pool.
    ///
    /// \tparam Executor Executor from an execution context
    /// \tparam Function A callable object
    /// \tparam Args Arguments for the Function
    ///
    /// \param ex Executor
    /// \param f Function to execute
    /// \param args Function arguments
    ///
    /// \return A future object with the function results
    template <typename Executor, typename Function, typename... Args
#ifndef FUTURES_DOXYGEN
              ,
              std::enable_if_t<detail::is_valid_async_input_v<Executor, Function, Args...>, int> = 0
#endif
              >
#ifndef FUTURES_DOXYGEN
    detail::async_result_of_t<Function, Args...>
#else
    __implementation_defined__
#endif
    async(const Executor &ex, Function &&f, Args &&...args) {
        return async(launch::async, ex, std::forward<Function>(f), std::forward<Args>(args)...);
    }

    /// \brief Launch an async function according to the specified policy with the default executor
    ///
    /// \tparam Function A callable object
    /// \tparam Args Arguments for the Function
    ///
    /// \param policy Launch policy
    /// \param f Function to execute
    /// \param args Function arguments
    ///
    /// \return A future object with the function results
    template <typename Function, typename... Args
#ifndef FUTURES_DOXYGEN
              ,
              std::enable_if_t<detail::is_async_input_non_executor_v<Function, Args...>, int> = 0
#endif
              >
#ifndef FUTURES_DOXYGEN
    detail::async_result_of_t<Function, Args...>
#else
    __implementation_defined__
#endif
    async(launch policy, Function &&f, Args &&...args) {
        return async(policy, make_default_executor(), std::forward<Function>(f), std::forward<Args>(args)...);
    }

    /// \brief Launch an async function with the default executor of type @ref default_executor_type
    ///
    /// \tparam Executor Executor from an execution context
    /// \tparam Function A callable object
    /// \tparam Args Arguments for the Function
    ///
    /// \param f Function to execute
    /// \param args Function arguments
    ///
    /// \return A future object with the function results
    template <typename Function, typename... Args
#ifndef FUTURES_DOXYGEN
              ,
              std::enable_if_t<detail::is_async_input_non_executor_v<Function, Args...>, int> = 0
#endif
              >
#ifndef FUTURES_DOXYGEN
    detail::async_result_of_t<Function, Args...>
#else
    __implementation_defined__
#endif
    async(Function &&f, Args &&...args) {
        return async(launch::async, ::futures::make_default_executor(), std::forward<Function>(f),
                     std::forward<Args>(args)...);
    }

    /** @} */
    /** @} */
    /** @} */
} // namespace futures

#endif // FUTURES_ASYNC_H

// #include <futures/futures/wait_for_all.h>
//
// Copyright (c) alandefreitas 12/3/21.
// See accompanying file LICENSE
//

#ifndef FUTURES_WAIT_FOR_ALL_H
#define FUTURES_WAIT_FOR_ALL_H

// #include <futures/algorithm/detail/traits/range/range/concepts.h>
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

#ifndef FUTURES_RANGES_RANGE_CONCEPTS_HPP
#define FUTURES_RANGES_RANGE_CONCEPTS_HPP

#include <initializer_list>
// #include <type_traits>

// #include <utility>


#ifdef __has_include
#if __has_include(<span>)
namespace std {
    template <class T, std::size_t Extent> class span;
}
#endif
#if __has_include(<string_view>)
#include <string_view>
#endif
#endif

// #include <futures/algorithm/detail/traits/range/meta/meta.h>
/// \file meta.hpp Tiny meta-programming library.
//
// Meta library
//
//  Copyright Eric Niebler 2014-present
//
//  Use, modification and distribution is subject to the
//  Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
// Project home: https://github.com/ericniebler/meta
//

#ifndef META_HPP
#define META_HPP

#include <cstddef>
// #include <futures/algorithm/detail/traits/range/meta/meta_fwd.h>
/// \file meta_fwd.hpp Forward declarations
//
// Meta library
//
//  Copyright Eric Niebler 2014-present
//
//  Use, modification and distribution is subject to the
//  Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
// Project home: https://github.com/ericniebler/meta
//

#ifndef META_FWD_HPP
#define META_FWD_HPP

// #include <type_traits>

// #include <utility>


#ifdef __clang__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-variable-declarations"
#endif

#define META_CXX_STD_14 201402L
#define META_CXX_STD_17 201703L

#if defined(_MSVC_LANG) && _MSVC_LANG > __cplusplus // Older clangs define _MSVC_LANG < __cplusplus
#define META_CXX_VER _MSVC_LANG
#else
#define META_CXX_VER __cplusplus
#endif

#if defined(__apple_build_version__) || defined(__clang__)
#if defined(__apple_build_version__) || (defined(__clang__) && __clang_major__ < 6)
#define META_WORKAROUND_LLVM_28385 // https://llvm.org/bugs/show_bug.cgi?id=28385
#endif

#elif defined(_MSC_VER)
#define META_HAS_MAKE_INTEGER_SEQ 1
#if _MSC_VER < 1920
#define META_WORKAROUND_MSVC_702792 // Bogus C4018 comparing constant expressions with dependent type
#define META_WORKAROUND_MSVC_703656 // ICE with pack expansion inside decltype in alias template
#endif

#if _MSC_VER < 1921
#define META_WORKAROUND_MSVC_756112 // fold expression + alias templates in template argument
#endif

#elif defined(__GNUC__)
#define META_WORKAROUND_GCC_86356 // https://gcc.gnu.org/bugzilla/show_bug.cgi?id=86356
#if __GNUC__ < 8
#define META_WORKAROUND_GCC_UNKNOWN1 // Older GCCs don't like fold + debug + -march=native
#endif
#if __GNUC__ == 5 && __GNUC_MINOR__ == 1
#define META_WORKAROUND_GCC_66405 // https://gcc.gnu.org/bugzilla/show_bug.cgi?id=66405
#endif
#if __GNUC__ < 5
#define META_WORKAROUND_CWG_1558 // https://wg21.link/cwg1558
#endif
#endif

#ifndef META_CXX_VARIABLE_TEMPLATES
#ifdef __cpp_variable_templates
#define META_CXX_VARIABLE_TEMPLATES __cpp_variable_templates
#else
#define META_CXX_VARIABLE_TEMPLATES (META_CXX_VER >= META_CXX_STD_14)
#endif
#endif

#ifndef META_CXX_INLINE_VARIABLES
#ifdef __cpp_inline_variables
#define META_CXX_INLINE_VARIABLES __cpp_inline_variables
#else
#define META_CXX_INLINE_VARIABLES (META_CXX_VER >= META_CXX_STD_17)
#endif
#endif

#ifndef META_INLINE_VAR
#if META_CXX_INLINE_VARIABLES
#define META_INLINE_VAR inline
#else
#define META_INLINE_VAR
#endif
#endif

#ifndef META_CXX_INTEGER_SEQUENCE
#ifdef __cpp_lib_integer_sequence
#define META_CXX_INTEGER_SEQUENCE __cpp_lib_integer_sequence
#else
#define META_CXX_INTEGER_SEQUENCE (META_CXX_VER >= META_CXX_STD_14)
#endif
#endif

#ifndef META_HAS_MAKE_INTEGER_SEQ
#ifdef __has_builtin
#if __has_builtin(__make_integer_seq)
#define META_HAS_MAKE_INTEGER_SEQ 1
#endif
#endif
#endif
#ifndef META_HAS_MAKE_INTEGER_SEQ
#define META_HAS_MAKE_INTEGER_SEQ 0
#endif

#ifndef META_HAS_TYPE_PACK_ELEMENT
#ifdef __has_builtin
#if __has_builtin(__type_pack_element)
#define META_HAS_TYPE_PACK_ELEMENT 1
#endif
#endif
#endif
#ifndef META_HAS_TYPE_PACK_ELEMENT
#define META_HAS_TYPE_PACK_ELEMENT 0
#endif

#if !defined(META_DEPRECATED) && !defined(META_DISABLE_DEPRECATED_WARNINGS)
#if defined(__cpp_attribute_deprecated) || META_CXX_VER >= META_CXX_STD_14
#define META_DEPRECATED(...) [[deprecated(__VA_ARGS__)]]
#elif defined(__clang__) || defined(__GNUC__)
#define META_DEPRECATED(...) __attribute__((deprecated(__VA_ARGS__)))
#endif
#endif
#ifndef META_DEPRECATED
#define META_DEPRECATED(...)
#endif

#ifndef META_CXX_FOLD_EXPRESSIONS
#ifdef __cpp_fold_expressions
#define META_CXX_FOLD_EXPRESSIONS __cpp_fold_expressions
#else
#define META_CXX_FOLD_EXPRESSIONS (META_CXX_VER >= META_CXX_STD_17)
#endif
#endif

#if META_CXX_FOLD_EXPRESSIONS
#if !META_CXX_VARIABLE_TEMPLATES
#error Fold expressions, but no variable templates?
#endif
#endif

#if (defined(__cpp_concepts) && __cpp_concepts > 0) || defined(META_DOXYGEN_INVOKED)
#if !META_CXX_VARIABLE_TEMPLATES
#error Concepts, but no variable templates?
#endif
#if __cpp_concepts <= 201507L && !defined(META_DOXYGEN_INVOKED)
#define META_CONCEPT concept bool
// TS concepts subsumption barrier for atomic expressions
#define META_CONCEPT_BARRIER(...) ::futures::detail::meta::detail::barrier<__VA_ARGS__>
#else
#define META_CONCEPT concept
#define META_CONCEPT_BARRIER(...) __VA_ARGS__
#endif
#define META_TYPE_CONSTRAINT(...) __VA_ARGS__
#else
#define META_TYPE_CONSTRAINT(...) typename
#endif

#if (defined(__cpp_lib_type_trait_variable_templates) && __cpp_lib_type_trait_variable_templates > 0)
#define META_CXX_TRAIT_VARIABLE_TEMPLATES 1
#else
#define META_CXX_TRAIT_VARIABLE_TEMPLATES 0
#endif

#if defined(__clang__)
#define META_IS_SAME(...) __is_same(__VA_ARGS__)
#elif defined(__GNUC__) && __GNUC__ >= 6
#define META_IS_SAME(...) __is_same_as(__VA_ARGS__)
#elif META_CXX_TRAIT_VARIABLE_TEMPLATES
#define META_IS_SAME(...) std::is_same_v<__VA_ARGS__>
#else
#define META_IS_SAME(...) std::is_same<__VA_ARGS__>::value
#endif

#if defined(__GNUC__) || defined(_MSC_VER)
#define META_IS_BASE_OF(...) __is_base_of(__VA_ARGS__)
#elif META_CXX_TRAIT_VARIABLE_TEMPLATES
#define META_IS_BASE_OF(...) std::is_base_of_v<__VA_ARGS__>
#else
#define META_IS_BASE_OF(...) std::is_base_of<__VA_ARGS__>::value
#endif

#if defined(__clang__) || defined(_MSC_VER) || (defined(__GNUC__) && __GNUC__ >= 8)
#define META_IS_CONSTRUCTIBLE(...) __is_constructible(__VA_ARGS__)
#elif META_CXX_TRAIT_VARIABLE_TEMPLATES
#define META_IS_CONSTRUCTIBLE(...) std::is_constructible_v<__VA_ARGS__>
#else
#define META_IS_CONSTRUCTIBLE(...) std::is_constructible<__VA_ARGS__>::value
#endif

/// \cond
// Non-portable forward declarations of standard containers
#ifdef _LIBCPP_VERSION
#define META_BEGIN_NAMESPACE_STD _LIBCPP_BEGIN_NAMESPACE_STD
#define META_END_NAMESPACE_STD _LIBCPP_END_NAMESPACE_STD
#elif defined(_MSVC_STL_VERSION)
#define META_BEGIN_NAMESPACE_STD _STD_BEGIN
#define META_END_NAMESPACE_STD _STD_END
#else
#define META_BEGIN_NAMESPACE_STD namespace std {
#define META_END_NAMESPACE_STD }
#endif

#if defined(__GLIBCXX__)
#define META_BEGIN_NAMESPACE_VERSION _GLIBCXX_BEGIN_NAMESPACE_VERSION
#define META_END_NAMESPACE_VERSION _GLIBCXX_END_NAMESPACE_VERSION
#define META_BEGIN_NAMESPACE_CONTAINER _GLIBCXX_BEGIN_NAMESPACE_CONTAINER
#define META_END_NAMESPACE_CONTAINER _GLIBCXX_END_NAMESPACE_CONTAINER
#else
#define META_BEGIN_NAMESPACE_VERSION
#define META_END_NAMESPACE_VERSION
#define META_BEGIN_NAMESPACE_CONTAINER
#define META_END_NAMESPACE_CONTAINER
#endif

#if defined(_LIBCPP_VERSION) && _LIBCPP_VERSION >= 4000
#define META_TEMPLATE_VIS _LIBCPP_TEMPLATE_VIS
#elif defined(_LIBCPP_VERSION)
#define META_TEMPLATE_VIS _LIBCPP_TYPE_VIS_ONLY
#else
#define META_TEMPLATE_VIS
#endif
/// \endcond

namespace futures::detail::meta {
#if META_CXX_INTEGER_SEQUENCE
    using std::integer_sequence;
#else
    template <typename T, T...> struct integer_sequence;
#endif

    template <typename... Ts> struct list;

    template <typename T> struct id;

    template <template <typename...> class> struct quote;

    template <typename T, template <T...> class F> struct quote_i;

    template <template <typename...> class C, typename... Ts> struct defer;

    template <typename T, template <T...> class C, T... Is> struct defer_i;

#if META_CXX_VARIABLE_TEMPLATES || defined(META_DOXYGEN_INVOKED)
    /// is_v
    /// Test whether a type \p T is an instantiation of class
    /// template \p C.
    /// \ingroup trait
    template <typename, template <typename...> class> META_INLINE_VAR constexpr bool is_v = false;
    template <typename... Ts, template <typename...> class C> META_INLINE_VAR constexpr bool is_v<C<Ts...>, C> = true;
#endif

#ifdef META_CONCEPT
    namespace ranges_detail {
        template <bool B> META_INLINE_VAR constexpr bool barrier = B;

        template <class T, T> struct require_constant; // not defined
    }                                                  // namespace ranges_detail

    template <typename...> META_CONCEPT is_true = META_CONCEPT_BARRIER(true);

    template <typename T, typename U> META_CONCEPT same_as = META_CONCEPT_BARRIER(META_IS_SAME(T, U));

    template <template <typename...> class C, typename... Ts> META_CONCEPT valid = requires { typename C<Ts...>; };

    template <typename T, template <T...> class C, T... Is> META_CONCEPT valid_i = requires { typename C<Is...>; };

    template <typename T> META_CONCEPT trait = requires { typename T::type; };

    template <typename T> META_CONCEPT invocable = requires { typename quote<T::template invoke>; };

    template <typename T> META_CONCEPT list_like = is_v<T, list>;

    // clang-format off
    template <typename T>
    META_CONCEPT integral = requires
    {
        typename T::type;
        typename T::value_type;
        typename T::type::value_type;
    }
    && same_as<typename T::value_type, typename T::type::value_type>
#if META_CXX_TRAIT_VARIABLE_TEMPLATES
    && std::is_integral_v<typename T::value_type>
#else
    && std::is_integral<typename T::value_type>::value
#endif

    && requires
    {
        // { T::value } -> same_as<const typename T::value_type&>;
        T::value;
        requires same_as<decltype(T::value), const typename T::value_type>;
        typename ranges_detail::require_constant<decltype(T::value), T::value>;

        // { T::type::value } -> same_as<const typename T::value_type&>;
        T::type::value;
        requires same_as<decltype(T::type::value), const typename T::value_type>;
        typename ranges_detail::require_constant<decltype(T::type::value), T::type::value>;
        requires T::value == T::type::value;

        // { T{}() } -> same_as<typename T::value_type>;
        T{}();
        requires same_as<decltype(T{}()), typename T::value_type>;
        typename ranges_detail::require_constant<decltype(T{}()), T{}()>;
        requires T{}() == T::value;

        // { T{} } -> typename T::value_type;
    };
        // clang-format on
#endif // META_CONCEPT

    namespace extension {
        template <META_TYPE_CONSTRAINT(invocable) F, typename L> struct apply;
    }
} // namespace futures::detail::meta

#ifdef __clang__
#pragma GCC diagnostic pop
#endif

#endif

// #include <initializer_list>

// #include <type_traits>

// #include <utility>


#ifdef __clang__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunknown-pragmas"
#pragma GCC diagnostic ignored "-Wpragmas"
#pragma GCC diagnostic ignored "-Wdocumentation-deprecated-sync"
#pragma GCC diagnostic ignored "-Wmissing-variable-declarations"
#endif

/// \defgroup meta Meta
///
/// A tiny metaprogramming library

/// \defgroup trait Trait
/// Trait invocation/composition.
/// \ingroup meta

/// \defgroup invocation Invocation
/// Trait invocation
/// \ingroup trait

/// \defgroup composition Composition
/// Trait composition
/// \ingroup trait

/// \defgroup logical Logical
/// Logical operations
/// \ingroup meta

/// \defgroup algorithm Algorithms
/// Algorithms.
/// \ingroup meta

/// \defgroup query Query/Search
/// Query and search algorithms
/// \ingroup algorithm

/// \defgroup transformation Transformation
/// Transformation algorithms
/// \ingroup algorithm

/// \defgroup runtime Runtime
/// Runtime algorithms
/// \ingroup algorithm

/// \defgroup datatype Datatype
/// Datatypes.
/// \ingroup meta

/// \defgroup list list_like
/// \ingroup datatype

/// \defgroup integral Integer sequence
/// Equivalent to C++14's `std::integer_sequence`
/// \ingroup datatype

/// \defgroup extension Extension
/// Extend meta with your own datatypes.
/// \ingroup datatype

/// \defgroup math Math
/// Integral constant arithmetic.
/// \ingroup meta

/// \defgroup lazy_trait lazy
/// \ingroup trait

/// \defgroup lazy_invocation lazy
/// \ingroup invocation

/// \defgroup lazy_composition lazy
/// \ingroup composition

/// \defgroup lazy_logical lazy
/// \ingroup logical

/// \defgroup lazy_query lazy
/// \ingroup query

/// \defgroup lazy_transformation lazy
/// \ingroup transformation

/// \defgroup lazy_list lazy
/// \ingroup list

/// \defgroup lazy_datatype lazy
/// \ingroup datatype

/// \defgroup lazy_math lazy
/// \ingroup math

/// Tiny metaprogramming library
namespace futures::detail::meta {
    namespace ranges_detail {
        /// Returns a \p T nullptr
        template <typename T> constexpr T *_nullptr_v() { return nullptr; }

#if META_CXX_VARIABLE_TEMPLATES
        template <typename T> META_INLINE_VAR constexpr T *nullptr_v = nullptr;
#endif
    } // namespace ranges_detail

    /// An empty type.
    /// \ingroup datatype
    struct nil_ {};

    /// Type alias for \p T::type.
    /// \ingroup invocation
    template <META_TYPE_CONSTRAINT(trait) T> using _t = typename T::type;

#if META_CXX_VARIABLE_TEMPLATES || defined(META_DOXYGEN_INVOKED)
    /// Variable alias for \c T::type::value
    /// \note Requires C++14 or greater.
    /// \ingroup invocation
    template <META_TYPE_CONSTRAINT(integral) T> constexpr typename T::type::value_type _v = T::type::value;
#endif

    /// Lazy versions of meta actions
    namespace lazy {
        /// \sa `futures::detail::meta::_t`
        /// \ingroup lazy_invocation
        template <typename T> using _t = defer<_t, T>;
    } // namespace lazy

    /// An integral constant wrapper for \c std::size_t.
    /// \ingroup integral
    template <std::size_t N> using size_t = std::integral_constant<std::size_t, N>;

    /// An integral constant wrapper for \c bool.
    /// \ingroup integral
    template <bool B> using bool_ = std::integral_constant<bool, B>;

    /// An integral constant wrapper for \c int.
    /// \ingroup integral
    template <int I> using int_ = std::integral_constant<int, I>;

    /// An integral constant wrapper for \c char.
    /// \ingroup integral
    template <char Ch> using char_ = std::integral_constant<char, Ch>;

    ///////////////////////////////////////////////////////////////////////////////////////////
    // Math operations
    /// An integral constant wrapper around the result of incrementing the wrapped integer \c
    /// T::type::value.
    template <META_TYPE_CONSTRAINT(integral) T>
    using inc = std::integral_constant<decltype(T::type::value + 1), T::type::value + 1>;

    /// An integral constant wrapper around the result of decrementing the wrapped integer \c
    /// T::type::value.
    template <META_TYPE_CONSTRAINT(integral) T>
    using dec = std::integral_constant<decltype(T::type::value - 1), T::type::value - 1>;

    /// An integral constant wrapper around the result of adding the two wrapped integers
    /// \c T::type::value and \c U::type::value.
    /// \ingroup math
    template <META_TYPE_CONSTRAINT(integral) T, META_TYPE_CONSTRAINT(integral) U>
    using plus = std::integral_constant<decltype(T::type::value + U::type::value), T::type::value + U::type::value>;

    /// An integral constant wrapper around the result of subtracting the two wrapped integers
    /// \c T::type::value and \c U::type::value.
    /// \ingroup math
    template <META_TYPE_CONSTRAINT(integral) T, META_TYPE_CONSTRAINT(integral) U>
    using minus = std::integral_constant<decltype(T::type::value - U::type::value), T::type::value - U::type::value>;

    /// An integral constant wrapper around the result of multiplying the two wrapped integers
    /// \c T::type::value and \c U::type::value.
    /// \ingroup math
    template <META_TYPE_CONSTRAINT(integral) T, META_TYPE_CONSTRAINT(integral) U>
    using multiplies =
        std::integral_constant<decltype(T::type::value * U::type::value), T::type::value * U::type::value>;

    /// An integral constant wrapper around the result of dividing the two wrapped integers \c
    /// T::type::value and \c U::type::value.
    /// \ingroup math
    template <META_TYPE_CONSTRAINT(integral) T, META_TYPE_CONSTRAINT(integral) U>
    using divides = std::integral_constant<decltype(T::type::value / U::type::value), T::type::value / U::type::value>;

    /// An integral constant wrapper around the result of negating the wrapped integer
    /// \c T::type::value.
    /// \ingroup math
    template <META_TYPE_CONSTRAINT(integral) T>
    using negate = std::integral_constant<decltype(-T::type::value), -T::type::value>;

    /// An integral constant wrapper around the remainder of dividing the two wrapped integers
    /// \c T::type::value and \c U::type::value.
    /// \ingroup math
    template <META_TYPE_CONSTRAINT(integral) T, META_TYPE_CONSTRAINT(integral) U>
    using modulus = std::integral_constant<decltype(T::type::value % U::type::value), T::type::value % U::type::value>;

    /// A Boolean integral constant wrapper around the result of comparing \c T::type::value and
    /// \c U::type::value for equality.
    /// \ingroup math
    template <META_TYPE_CONSTRAINT(integral) T, META_TYPE_CONSTRAINT(integral) U>
    using equal_to = bool_<T::type::value == U::type::value>;

    /// A Boolean integral constant wrapper around the result of comparing \c T::type::value and
    /// \c U::type::value for inequality.
    /// \ingroup math
    template <META_TYPE_CONSTRAINT(integral) T, META_TYPE_CONSTRAINT(integral) U>
    using not_equal_to = bool_<T::type::value != U::type::value>;

    /// A Boolean integral constant wrapper around \c true if \c T::type::value is greater than
    /// \c U::type::value; \c false, otherwise.
    /// \ingroup math
    template <META_TYPE_CONSTRAINT(integral) T, META_TYPE_CONSTRAINT(integral) U>
    using greater = bool_<(T::type::value > U::type::value)>;

    /// A Boolean integral constant wrapper around \c true if \c T::type::value is less than \c
    /// U::type::value; \c false, otherwise.
    /// \ingroup math
    template <META_TYPE_CONSTRAINT(integral) T, META_TYPE_CONSTRAINT(integral) U>
    using less = bool_<(T::type::value < U::type::value)>;

    /// A Boolean integral constant wrapper around \c true if \c T::type::value is greater than
    /// or equal to \c U::type::value; \c false, otherwise.
    /// \ingroup math
    template <META_TYPE_CONSTRAINT(integral) T, META_TYPE_CONSTRAINT(integral) U>
    using greater_equal = bool_<(T::type::value >= U::type::value)>;

    /// A Boolean integral constant wrapper around \c true if \c T::type::value is less than or
    /// equal to \c U::type::value; \c false, otherwise.
    /// \ingroup math
    template <META_TYPE_CONSTRAINT(integral) T, META_TYPE_CONSTRAINT(integral) U>
    using less_equal = bool_<(T::type::value <= U::type::value)>;

    /// An integral constant wrapper around the result of bitwise-and'ing the two wrapped
    /// integers \c T::type::value and \c U::type::value.
    /// \ingroup math
    template <META_TYPE_CONSTRAINT(integral) T, META_TYPE_CONSTRAINT(integral) U>
    using bit_and = std::integral_constant<decltype(T::type::value & U::type::value), T::type::value & U::type::value>;

    /// An integral constant wrapper around the result of bitwise-or'ing the two wrapped
    /// integers \c T::type::value and \c U::type::value.
    /// \ingroup math
    template <META_TYPE_CONSTRAINT(integral) T, META_TYPE_CONSTRAINT(integral) U>
    using bit_or = std::integral_constant<decltype(T::type::value | U::type::value), T::type::value | U::type::value>;

    /// An integral constant wrapper around the result of bitwise-exclusive-or'ing the two
    /// wrapped integers \c T::type::value and \c U::type::value.
    /// \ingroup math
    template <META_TYPE_CONSTRAINT(integral) T, META_TYPE_CONSTRAINT(integral) U>
    using bit_xor = std::integral_constant<decltype(T::type::value ^ U::type::value), T::type::value ^ U::type::value>;

    /// An integral constant wrapper around the result of bitwise-complementing the wrapped
    /// integer \c T::type::value.
    /// \ingroup math
    template <META_TYPE_CONSTRAINT(integral) T>
    using bit_not = std::integral_constant<decltype(~T::type::value), ~T::type::value>;

    namespace lazy {
        /// \sa 'futures::detail::meta::int'
        /// \ingroup lazy_math
        template <typename T> using inc = defer<inc, T>;

        /// \sa 'futures::detail::meta::dec'
        /// \ingroup lazy_math
        template <typename T> using dec = defer<dec, T>;

        /// \sa 'futures::detail::meta::plus'
        /// \ingroup lazy_math
        template <typename T, typename U> using plus = defer<plus, T, U>;

        /// \sa 'futures::detail::meta::minus'
        /// \ingroup lazy_math
        template <typename T, typename U> using minus = defer<minus, T, U>;

        /// \sa 'futures::detail::meta::multiplies'
        /// \ingroup lazy_math
        template <typename T, typename U> using multiplies = defer<multiplies, T, U>;

        /// \sa 'futures::detail::meta::divides'
        /// \ingroup lazy_math
        template <typename T, typename U> using divides = defer<divides, T, U>;

        /// \sa 'futures::detail::meta::negate'
        /// \ingroup lazy_math
        template <typename T> using negate = defer<negate, T>;

        /// \sa 'futures::detail::meta::modulus'
        /// \ingroup lazy_math
        template <typename T, typename U> using modulus = defer<modulus, T, U>;

        /// \sa 'futures::detail::meta::equal_to'
        /// \ingroup lazy_math
        template <typename T, typename U> using equal_to = defer<equal_to, T, U>;

        /// \sa 'futures::detail::meta::not_equal_t'
        /// \ingroup lazy_math
        template <typename T, typename U> using not_equal_to = defer<not_equal_to, T, U>;

        /// \sa 'futures::detail::meta::greater'
        /// \ingroup lazy_math
        template <typename T, typename U> using greater = defer<greater, T, U>;

        /// \sa 'futures::detail::meta::less'
        /// \ingroup lazy_math
        template <typename T, typename U> using less = defer<less, T, U>;

        /// \sa 'futures::detail::meta::greater_equal'
        /// \ingroup lazy_math
        template <typename T, typename U> using greater_equal = defer<greater_equal, T, U>;

        /// \sa 'futures::detail::meta::less_equal'
        /// \ingroup lazy_math
        template <typename T, typename U> using less_equal = defer<less_equal, T, U>;

        /// \sa 'futures::detail::meta::bit_and'
        /// \ingroup lazy_math
        template <typename T, typename U> using bit_and = defer<bit_and, T, U>;

        /// \sa 'futures::detail::meta::bit_or'
        /// \ingroup lazy_math
        template <typename T, typename U> using bit_or = defer<bit_or, T, U>;

        /// \sa 'futures::detail::meta::bit_xor'
        /// \ingroup lazy_math
        template <typename T, typename U> using bit_xor = defer<bit_xor, T, U>;

        /// \sa 'futures::detail::meta::bit_not'
        /// \ingroup lazy_math
        template <typename T> using bit_not = defer<bit_not, T>;
    } // namespace lazy

    /// \cond
    namespace ranges_detail {
        enum class indices_strategy_ { done, repeat, recurse };

        constexpr indices_strategy_ strategy_(std::size_t cur, std::size_t end) {
            return cur >= end       ? indices_strategy_::done
                   : cur * 2 <= end ? indices_strategy_::repeat
                                    : indices_strategy_::recurse;
        }

        template <typename T> constexpr std::size_t range_distance_(T begin, T end) {
            return begin <= end ? static_cast<std::size_t>(end - begin)
                                : throw "The start of the integer_sequence must not be "
                                        "greater than the end";
        }

        template <std::size_t End, typename State, indices_strategy_ Status_> struct make_indices_ {
            using type = State;
        };

        template <typename T, T, typename> struct coerce_indices_ {};
    } // namespace ranges_detail
    /// \endcond

    ///////////////////////////////////////////////////////////////////////////////////////////
    // integer_sequence
#if !META_CXX_INTEGER_SEQUENCE
    /// A container for a sequence of compile-time integer constants.
    /// \ingroup integral
    template <typename T, T... Is> struct integer_sequence {
        using value_type = T;
        /// \return `sizeof...(Is)`
        static constexpr std::size_t size() noexcept { return sizeof...(Is); }
    };
#endif

    ///////////////////////////////////////////////////////////////////////////////////////////
    // index_sequence
    /// A container for a sequence of compile-time integer constants of type
    /// \c std::size_t
    /// \ingroup integral
    template <std::size_t... Is> using index_sequence = integer_sequence<std::size_t, Is...>;

#if META_HAS_MAKE_INTEGER_SEQ && !defined(META_DOXYGEN_INVOKED)
    // Implement make_integer_sequence and make_index_sequence with the
    // __make_integer_seq builtin on compilers that provide it. (Redirect
    // through decltype to workaround suspected clang bug.)
    /// \cond
    namespace ranges_detail {
        template <typename T, T N> __make_integer_seq<integer_sequence, T, N> make_integer_sequence_();
    }
    /// \endcond

    template <typename T, T N> using make_integer_sequence = decltype(ranges_detail::make_integer_sequence_<T, N>());

    template <std::size_t N> using make_index_sequence = make_integer_sequence<std::size_t, N>;
#else
    /// Generate \c index_sequence containing integer constants [0,1,2,...,N-1].
    /// \par Complexity
    /// \f$ O(log(N)) \f$.
    /// \ingroup integral
    template <std::size_t N>
    using make_index_sequence = _t<ranges_detail::make_indices_<N, index_sequence<0>, ranges_detail::strategy_(1, N)>>;

    /// Generate \c integer_sequence containing integer constants [0,1,2,...,N-1].
    /// \par Complexity
    /// \f$ O(log(N)) \f$.
    /// \ingroup integral
    template <typename T, T N>
    using make_integer_sequence = _t<ranges_detail::coerce_indices_<T, 0, make_index_sequence<static_cast<std::size_t>(N)>>>;
#endif

    ///////////////////////////////////////////////////////////////////////////////////////////
    // integer_range
    /// Makes the integer sequence <tt>[From, To)</tt>.
    /// \par Complexity
    /// \f$ O(log(To - From)) \f$.
    /// \ingroup integral
    template <typename T, T From, T To>
    using integer_range = _t<ranges_detail::coerce_indices_<T, From, make_index_sequence<ranges_detail::range_distance_(From, To)>>>;

    /// \cond
    namespace ranges_detail {
        template <typename, typename> struct concat_indices_ {};

        template <std::size_t... Is, std::size_t... Js>
        struct concat_indices_<index_sequence<Is...>, index_sequence<Js...>> {
            using type = index_sequence<Is..., (Js + sizeof...(Is))...>;
        };

        template <> struct make_indices_<0u, index_sequence<0>, indices_strategy_::done> {
            using type = index_sequence<>;
        };

        template <std::size_t End, std::size_t... Values>
        struct make_indices_<End, index_sequence<Values...>, indices_strategy_::repeat>
            : make_indices_<End, index_sequence<Values..., (Values + sizeof...(Values))...>,
                            ranges_detail::strategy_(sizeof...(Values) * 2, End)> {};

        template <std::size_t End, std::size_t... Values>
        struct make_indices_<End, index_sequence<Values...>, indices_strategy_::recurse>
            : concat_indices_<index_sequence<Values...>, make_index_sequence<End - sizeof...(Values)>> {};

        template <typename T, T Offset, std::size_t... Values>
        struct coerce_indices_<T, Offset, index_sequence<Values...>> {
            using type = integer_sequence<T, static_cast<T>(static_cast<T>(Values) + Offset)...>;
        };
    } // namespace ranges_detail
    /// \endcond

    /// Evaluate the invocable \p Fn with the arguments \p Args.
    /// \ingroup invocation
    template <META_TYPE_CONSTRAINT(invocable) Fn, typename... Args>
    using invoke = typename Fn::template invoke<Args...>;

    /// Lazy versions of meta actions
    namespace lazy {
        /// \sa `futures::detail::meta::invoke`
        /// \ingroup lazy_invocation
        template <typename Fn, typename... Args> using invoke = defer<invoke, Fn, Args...>;
    } // namespace lazy

    /// A trait that always returns its argument \p T. It is also an invocable
    /// that always returns \p T.
    /// \ingroup trait
    /// \ingroup invocation
    template <typename T> struct id {
#if defined(META_WORKAROUND_CWG_1558) && !defined(META_DOXYGEN_INVOKED)
        // Redirect through decltype for compilers that have not
        // yet implemented CWG 1558:
        static id impl(void *);

        template <typename... Ts> using invoke = _t<decltype(id::impl(static_cast<list<Ts...> *>(nullptr)))>;
#else
        template <typename...> using invoke = T;
#endif

        using type = T;
    };

    /// An alias for type \p T. Useful in non-deduced contexts.
    /// \ingroup trait
    template <typename T> using id_t = _t<id<T>>;

    namespace lazy {
        /// \sa `futures::detail::meta::id`
        /// \ingroup lazy_trait
        /// \ingroup lazy_invocation
        template <typename T> using id = defer<id, T>;
    } // namespace lazy

    /// An alias for `void`.
    /// \ingroup trait
#if defined(META_WORKAROUND_CWG_1558) && !defined(META_DOXYGEN_INVOKED)
    // Redirect through decltype for compilers that have not
    // yet implemented CWG 1558:
    template <typename... Ts> using void_ = invoke<id<void>, Ts...>;
#else
    template <typename...> using void_ = void;
#endif

#if META_CXX_VARIABLE_TEMPLATES
#ifdef META_CONCEPT
    /// `true` if `T::type` exists and names a type; `false` otherwise.
    /// \ingroup trait
    template <typename T> META_INLINE_VAR constexpr bool is_trait_v = trait<T>;

    /// `true` if `T::invoke` exists and names a class template; `false` otherwise.
    /// \ingroup trait
    template <typename T> META_INLINE_VAR constexpr bool is_callable_v = invocable<T>;
#else  // ^^^ Concepts / No concepts vvv
    /// \cond
    namespace ranges_detail {
        template <typename, typename = void> META_INLINE_VAR constexpr bool is_trait_ = false;

        template <typename T> META_INLINE_VAR constexpr bool is_trait_<T, void_<typename T::type>> = true;

        template <typename, typename = void> META_INLINE_VAR constexpr bool is_callable_ = false;

        template <typename T> META_INLINE_VAR constexpr bool is_callable_<T, void_<quote<T::template invoke>>> = true;
    } // namespace ranges_detail
    /// \endcond

    /// `true` if `T::type` exists and names a type; `false` otherwise.
    /// \ingroup trait
    template <typename T> META_INLINE_VAR constexpr bool is_trait_v = ranges_detail::is_trait_<T>;

    /// `true` if `T::invoke` exists and names a class template; `false` otherwise.
    /// \ingroup trait
    template <typename T> META_INLINE_VAR constexpr bool is_callable_v = ranges_detail::is_callable_<T>;
#endif // Concepts vs. variable templates

    /// An alias for `std::true_type` if `T::type` exists and names a type; otherwise, it's an
    /// alias for `std::false_type`.
    /// \ingroup trait
    template <typename T> using is_trait = bool_<is_trait_v<T>>;

    /// An alias for `std::true_type` if `T::invoke` exists and names a class template;
    /// otherwise, it's an alias for `std::false_type`.
    /// \ingroup trait
    template <typename T> using is_callable = bool_<is_callable_v<T>>;
#else // ^^^ META_CXX_VARIABLE_TEMPLATES / !META_CXX_VARIABLE_TEMPLATES vvv
    /// \cond
    namespace ranges_detail {
        template <typename, typename = void> struct is_trait_ { using type = std::false_type; };

        template <typename T> struct is_trait_<T, void_<typename T::type>> { using type = std::true_type; };

        template <typename, typename = void> struct is_callable_ { using type = std::false_type; };

        template <typename T> struct is_callable_<T, void_<quote<T::template invoke>>> { using type = std::true_type; };
    } // namespace ranges_detail
    /// \endcond

    template <typename T> using is_trait = _t<ranges_detail::is_trait_<T>>;

    /// An alias for `std::true_type` if `T::invoke` exists and names a class
    /// template or alias template; otherwise, it's an alias for
    /// `std::false_type`.
    /// \ingroup trait
    template <typename T> using is_callable = _t<ranges_detail::is_callable_<T>>;
#endif

    /// \cond
    namespace ranges_detail {
#ifdef META_CONCEPT
        template <template <typename...> class, typename...> struct defer_ {};

        template <template <typename...> class C, typename... Ts>
        requires valid<C, Ts...>
        struct defer_<C, Ts...> {
            using type = C<Ts...>;
        };

        template <typename T, template <T...> class, T...> struct defer_i_ {};

        template <typename T, template <T...> class C, T... Is>
        requires valid_i<T, C, Is...>
        struct defer_i_<T, C, Is...> {
            using type = C<Is...>;
        };
#elif defined(META_WORKAROUND_MSVC_703656) // ^^^ Concepts / MSVC workaround vvv
        template <typename, template <typename...> class, typename...> struct _defer_ {};

        template <template <typename...> class C, typename... Ts> struct _defer_<void_<C<Ts...>>, C, Ts...> {
            using type = C<Ts...>;
        };

        template <template <typename...> class C, typename... Ts> using defer_ = _defer_<void, C, Ts...>;

        template <typename, typename T, template <T...> class, T...> struct _defer_i_ {};

        template <typename T, template <T...> class C, T... Is> struct _defer_i_<void_<C<Is...>>, T, C, Is...> {
            using type = C<Is...>;
        };

        template <typename T, template <T...> class C, T... Is> using defer_i_ = _defer_i_<void, T, C, Is...>;
#else                                      // ^^^ workaround ^^^ / vvv no workaround vvv
        template <template <typename...> class C, typename... Ts, template <typename...> class D = C>
        id<D<Ts...>> try_defer_(int);
        template <template <typename...> class C, typename... Ts> nil_ try_defer_(long);

        template <template <typename...> class C, typename... Ts>
        using defer_ = decltype(ranges_detail::try_defer_<C, Ts...>(0));

        template <typename T, template <T...> class C, T... Is, template <T...> class D = C>
        id<D<Is...>> try_defer_i_(int);
        template <typename T, template <T...> class C, T... Is> nil_ try_defer_i_(long);

        template <typename T, template <T...> class C, T... Is>
        using defer_i_ = decltype(ranges_detail::try_defer_i_<T, C, Is...>(0));
#endif                                     // Concepts vs. MSVC vs. Other

        template <typename T> using _t_t = _t<_t<T>>;
    } // namespace ranges_detail
    /// \endcond

    ///////////////////////////////////////////////////////////////////////////////////////////
    // defer
    /// A wrapper that defers the instantiation of a template \p C with type parameters \p Ts in
    /// a \c lambda or \c let expression.
    ///
    /// In the code below, the lambda would ideally be written as
    /// `lambda<_a,_b,push_back<_a,_b>>`, however this fails since `push_back` expects its first
    /// argument to be a list, not a placeholder. Instead, we express it using \c defer as
    /// follows:
    ///
    /// \code
    /// template <typename L>
    /// using reverse = reverse_fold<L, list<>, lambda<_a, _b, defer<push_back, _a, _b>>>;
    /// \endcode
    ///
    /// \ingroup invocation
    template <template <typename...> class C, typename... Ts> struct defer : ranges_detail::defer_<C, Ts...> {};

    ///////////////////////////////////////////////////////////////////////////////////////////
    // defer_i
    /// A wrapper that defers the instantiation of a template \p C with integral constant
    /// parameters \p Is in a \c lambda or \c let expression.
    /// \sa `defer`
    /// \ingroup invocation
    template <typename T, template <T...> class C, T... Is> struct defer_i : ranges_detail::defer_i_<T, C, Is...> {};

    ///////////////////////////////////////////////////////////////////////////////////////////
    // defer_trait
    /// A wrapper that defers the instantiation of a trait \p C with type parameters \p Ts in a
    /// \c lambda or \c let expression.
    /// \sa `defer`
    /// \ingroup invocation
    template <template <typename...> class C, typename... Ts>
    using defer_trait = defer<ranges_detail::_t_t, ranges_detail::defer_<C, Ts...>>;

    ///////////////////////////////////////////////////////////////////////////////////////////
    // defer_trait_i
    /// A wrapper that defers the instantiation of a trait \p C with integral constant
    /// parameters \p Is in a \c lambda or \c let expression.
    /// \sa `defer_i`
    /// \ingroup invocation
    template <typename T, template <T...> class C, T... Is>
    using defer_trait_i = defer<ranges_detail::_t_t, ranges_detail::defer_i_<T, C, Is...>>;

    /// An alias that computes the size of the type \p T.
    /// \par Complexity
    /// \f$ O(1) \f$.
    /// \ingroup trait
    template <typename T> using sizeof_ = futures::detail::meta::size_t<sizeof(T)>;

    /// An alias that computes the alignment required for any instance of the type \p T.
    /// \par Complexity
    /// \f$ O(1) \f$.
    /// \ingroup trait
    template <typename T> using alignof_ = futures::detail::meta::size_t<alignof(T)>;

    namespace lazy {
        /// \sa `futures::detail::meta::sizeof_`
        /// \ingroup lazy_trait
        template <typename T> using sizeof_ = defer<sizeof_, T>;

        /// \sa `futures::detail::meta::alignof_`
        /// \ingroup lazy_trait
        template <typename T> using alignof_ = defer<alignof_, T>;
    } // namespace lazy

#if META_CXX_VARIABLE_TEMPLATES
    /// is
    /// Test whether a type \p T is an instantiation of class
    /// template \p C.
    /// \ingroup trait
    template <typename T, template <typename...> class C> using is = bool_<is_v<T, C>>;
#else
    /// is
    /// \cond
    namespace ranges_detail {
        template <typename, template <typename...> class> struct is_ : std::false_type {};

        template <typename... Ts, template <typename...> class C> struct is_<C<Ts...>, C> : std::true_type {};
    } // namespace ranges_detail
    /// \endcond

    /// Test whether a type \c T is an instantiation of class
    /// template \c C.
    /// \ingroup trait
    template <typename T, template <typename...> class C> using is = _t<ranges_detail::is_<T, C>>;
#endif

    /// Compose the Invocables \p Fns in the parameter pack \p Ts.
    /// \ingroup composition
    template <META_TYPE_CONSTRAINT(invocable)... Fns> struct compose_ {};

    template <META_TYPE_CONSTRAINT(invocable) Fn0> struct compose_<Fn0> {
        template <typename... Ts> using invoke = invoke<Fn0, Ts...>;
    };

    template <META_TYPE_CONSTRAINT(invocable) Fn0, META_TYPE_CONSTRAINT(invocable)... Fns>
    struct compose_<Fn0, Fns...> {
        template <typename... Ts> using invoke = invoke<Fn0, invoke<compose_<Fns...>, Ts...>>;
    };

    template <typename... Fns> using compose = compose_<Fns...>;

    namespace lazy {
        /// \sa 'futures::detail::meta::compose'
        /// \ingroup lazy_composition
        template <typename... Fns> using compose = defer<compose, Fns...>;
    } // namespace lazy

    /// Turn a template \p C into an invocable.
    /// \ingroup composition
    template <template <typename...> class C> struct quote {
        // Indirection through defer here needed to avoid Core issue 1430
        // https://wg21.link/cwg1430
        template <typename... Ts> using invoke = _t<defer<C, Ts...>>;
    };

    /// Turn a template \p C taking literals of type \p T into a
    /// invocable.
    /// \ingroup composition
    template <typename T, template <T...> class C> struct quote_i {
        // Indirection through defer_i here needed to avoid Core issue 1430
        // https://wg21.link/cwg1430
        template <META_TYPE_CONSTRAINT(integral)... Ts> using invoke = _t<defer_i<T, C, Ts::type::value...>>;
    };

#if defined(__GNUC__) && !defined(__clang__) && __GNUC__ == 4 && __GNUC_MINOR__ <= 8 && !defined(META_DOXYGEN_INVOKED)
    template <template <typename...> class C> struct quote_trait {
        template <typename... Ts> using invoke = _t<invoke<quote<C>, Ts...>>;
    };

    template <typename T, template <T...> class C> struct quote_trait_i {
        template <typename... Ts> using invoke = _t<invoke<quote_i<T, C>, Ts...>>;
    };
#else
    // clang-format off
    /// Turn a trait template \p C into an invocable.
    /// \code
    /// static_assert(std::is_same_v<invoke<quote_trait<std::add_const>, int>, int const>, "");
    /// \endcode
    /// \ingroup composition
    template <template <typename...> class C>
    using quote_trait = compose<quote<_t>, quote<C>>;

    /// Turn a trait template \p C taking literals of type \p T into an invocable.
    /// \ingroup composition
    template <typename T, template <T...> class C>
    using quote_trait_i = compose<quote<_t>, quote_i<T, C>>;
    // clang-format on
#endif

    /// An invocable that partially applies the invocable
    /// \p Fn by binding the arguments \p Ts to the \e front of \p Fn.
    /// \ingroup composition
    template <META_TYPE_CONSTRAINT(invocable) Fn, typename... Ts> struct bind_front {
        template <typename... Us> using invoke = invoke<Fn, Ts..., Us...>;
    };

    /// An invocable that partially applies the invocable \p Fn by binding the
    /// arguments \p Us to the \e back of \p Fn.
    /// \ingroup composition
    template <META_TYPE_CONSTRAINT(invocable) Fn, typename... Us> struct bind_back {
        template <typename... Ts> using invoke = invoke<Fn, Ts..., Us...>;
    };

    namespace lazy {
        /// \sa 'futures::detail::meta::bind_front'
        /// \ingroup lazy_composition
        template <typename Fn, typename... Ts> using bind_front = defer<bind_front, Fn, Ts...>;

        /// \sa 'futures::detail::meta::bind_back'
        /// \ingroup lazy_composition
        template <typename Fn, typename... Ts> using bind_back = defer<bind_back, Fn, Ts...>;
    } // namespace lazy

    /// Extend meta with your own datatypes.
    namespace extension {
        /// A trait that unpacks the types in the type list \p L into the invocable
        /// \p Fn.
        /// \ingroup extension
        template <META_TYPE_CONSTRAINT(invocable) Fn, typename L> struct apply {};

        template <META_TYPE_CONSTRAINT(invocable) Fn, typename Ret, typename... Args>
        struct apply<Fn, Ret(Args...)> : lazy::invoke<Fn, Ret, Args...> {};

        template <META_TYPE_CONSTRAINT(invocable) Fn, template <typename...> class T, typename... Ts>
        struct apply<Fn, T<Ts...>> : lazy::invoke<Fn, Ts...> {};

        template <META_TYPE_CONSTRAINT(invocable) Fn, typename T, T... Is>
        struct apply<Fn, integer_sequence<T, Is...>> : lazy::invoke<Fn, std::integral_constant<T, Is>...> {};
    } // namespace extension

    /// Applies the invocable \p Fn using the types in the type list \p L as
    /// arguments.
    /// \ingroup invocation
    template <META_TYPE_CONSTRAINT(invocable) Fn, typename L> using apply = _t<extension::apply<Fn, L>>;

    namespace lazy {
        template <typename Fn, typename L> using apply = defer<apply, Fn, L>;
    }

    /// An invocable that takes a bunch of arguments, bundles them into a type
    /// list, and then calls the invocable \p Fn with the type list \p Q.
    /// \ingroup composition
    template <META_TYPE_CONSTRAINT(invocable) Fn, META_TYPE_CONSTRAINT(invocable) Q = quote<list>>
    using curry = compose<Fn, Q>;

    /// An invocable that takes a type list, unpacks the types, and then
    /// calls the invocable \p Fn with the types.
    /// \ingroup composition
    template <META_TYPE_CONSTRAINT(invocable) Fn> using uncurry = bind_front<quote<apply>, Fn>;

    namespace lazy {
        /// \sa 'futures::detail::meta::curry'
        /// \ingroup lazy_composition
        template <typename Fn, typename Q = quote<list>> using curry = defer<curry, Fn, Q>;

        /// \sa 'futures::detail::meta::uncurry'
        /// \ingroup lazy_composition
        template <typename Fn> using uncurry = defer<uncurry, Fn>;
    } // namespace lazy

    /// An invocable that reverses the order of the first two arguments.
    /// \ingroup composition
    template <META_TYPE_CONSTRAINT(invocable) Fn> struct flip {
      private:
        template <typename... Ts> struct impl {};
        template <typename A, typename B, typename... Ts> struct impl<A, B, Ts...> : lazy::invoke<Fn, B, A, Ts...> {};

      public:
        template <typename... Ts> using invoke = _t<impl<Ts...>>;
    };

    namespace lazy {
        /// \sa 'futures::detail::meta::flip'
        /// \ingroup lazy_composition
        template <typename Fn> using flip = defer<flip, Fn>;
    } // namespace lazy

    /// \cond
    namespace ranges_detail {
        template <typename...> struct on_ {};
        template <typename Fn, typename... Gs> struct on_<Fn, Gs...> {
            template <typename... Ts> using invoke = invoke<Fn, invoke<compose<Gs...>, Ts>...>;
        };
    } // namespace ranges_detail
    /// \endcond

    /// Use as `on<Fn, Gs...>`. Creates an invocable that applies invocable \c Fn to the
    /// result of applying invocable `compose<Gs...>` to all the arguments.
    /// \ingroup composition
    template <META_TYPE_CONSTRAINT(invocable)... Fns> using on_ = ranges_detail::on_<Fns...>;

    template <typename... Fns> using on = on_<Fns...>;

    namespace lazy {
        /// \sa 'futures::detail::meta::on'
        /// \ingroup lazy_composition
        template <typename Fn, typename G> using on = defer<on, Fn, G>;
    } // namespace lazy

    ///////////////////////////////////////////////////////////////////////////////////////////
    // conditional_t
    /// \cond
    namespace ranges_detail {
        template <bool> struct _cond { template <typename Then, typename Else> using invoke = Then; };
        template <> struct _cond<false> { template <typename Then, typename Else> using invoke = Else; };
    } // namespace ranges_detail
    /// \endcond

    /// Select one type or another depending on a compile-time Boolean.
    /// \ingroup logical
    template <bool If, typename Then, typename Else = void>
    using conditional_t = typename ranges_detail::_cond<If>::template invoke<Then, Else>;

    ///////////////////////////////////////////////////////////////////////////////////////////
    // if_
    /// \cond
    namespace ranges_detail {
#ifdef META_CONCEPT
        template <typename...> struct _if_ {};

        template <integral If> struct _if_<If> : std::enable_if<_v<If>> {};

        template <integral If, typename Then> struct _if_<If, Then> : std::enable_if<_v<If>, Then> {};

        template <integral If, typename Then, typename Else>
        struct _if_<If, Then, Else> : std::conditional<_v<If>, Then, Else> {};
#elif defined(__clang__)
        // Clang is faster with this implementation
        template <typename, typename = bool> struct _if_ {};

        template <typename If>
        struct _if_<list<If>, decltype(bool(If::type::value))> : std::enable_if<If::type::value> {};

        template <typename If, typename Then>
        struct _if_<list<If, Then>, decltype(bool(If::type::value))> : std::enable_if<If::type::value, Then> {};

        template <typename If, typename Then, typename Else>
        struct _if_<list<If, Then, Else>, decltype(bool(If::type::value))>
            : std::conditional<If::type::value, Then, Else> {};
#else
        // GCC seems to prefer this implementation
        template <typename, typename = std::true_type> struct _if_ {};

        template <typename If> struct _if_<list<If>, bool_<If::type::value>> { using type = void; };

        template <typename If, typename Then> struct _if_<list<If, Then>, bool_<If::type::value>> {
            using type = Then;
        };

        template <typename If, typename Then, typename Else> struct _if_<list<If, Then, Else>, bool_<If::type::value>> {
            using type = Then;
        };

        template <typename If, typename Then, typename Else>
        struct _if_<list<If, Then, Else>, bool_<!If::type::value>> {
            using type = Else;
        };
#endif
    } // namespace ranges_detail
      /// \endcond

    /// Select one type or another depending on a compile-time Boolean.
    /// \ingroup logical
#ifdef META_CONCEPT
    template <typename... Args> using if_ = _t<ranges_detail::_if_<Args...>>;

    /// Select one type or another depending on a compile-time Boolean.
    /// \ingroup logical
    template <bool If, typename... Args> using if_c = _t<ranges_detail::_if_<bool_<If>, Args...>>;
#else
    template <typename... Args> using if_ = _t<ranges_detail::_if_<list<Args...>>>;

    template <bool If, typename... Args> using if_c = _t<ranges_detail::_if_<list<bool_<If>, Args...>>>;
#endif

    namespace lazy {
        /// \sa 'futures::detail::meta::if_'
        /// \ingroup lazy_logical
        template <typename... Args> using if_ = defer<if_, Args...>;

        /// \sa 'futures::detail::meta::if_c'
        /// \ingroup lazy_logical
        template <bool If, typename... Args> using if_c = if_<bool_<If>, Args...>;
    } // namespace lazy

    /// \cond
    namespace ranges_detail {
#ifdef META_CONCEPT
        template <typename...> struct _and_ {};

        template <> struct _and_<> : std::true_type {};

        template <integral B, typename... Bs>
        requires(bool(B::type::value)) struct _and_<B, Bs...> : _and_<Bs...> {
        };

        template <integral B, typename... Bs>
        requires(!bool(B::type::value)) struct _and_<B, Bs...> : std::false_type {
        };

        template <typename...> struct _or_ {};

        template <> struct _or_<> : std::false_type {};

        template <integral B, typename... Bs>
        requires(bool(B::type::value)) struct _or_<B, Bs...> : std::true_type {
        };

        template <integral B, typename... Bs>
        requires(!bool(B::type::value)) struct _or_<B, Bs...> : _or_<Bs...> {
        };
#else
        template <bool> struct _and_ { template <typename...> using invoke = std::true_type; };

        template <> struct _and_<false> {
            template <typename B, typename... Bs>
            using invoke = invoke<if_c<!B::type::value, id<std::false_type>, _and_<0 == sizeof...(Bs)>>, Bs...>;
        };

        template <bool> struct _or_ { template <typename = void> using invoke = std::false_type; };

        template <> struct _or_<false> {
            template <typename B, typename... Bs>
            using invoke = invoke<if_c<B::type::value, id<std::true_type>, _or_<0 == sizeof...(Bs)>>, Bs...>;
        };
#endif
    } // namespace ranges_detail
    /// \endcond

    /// Logically negate the Boolean parameter
    /// \ingroup logical
    template <bool B> using not_c = bool_<!B>;

    /// Logically negate the integral constant-wrapped Boolean parameter.
    /// \ingroup logical
    template <META_TYPE_CONSTRAINT(integral) B> using not_ = not_c<B::type::value>;

#if META_CXX_FOLD_EXPRESSIONS && !defined(META_WORKAROUND_GCC_UNKNOWN1)
    template <bool... Bs> META_INLINE_VAR constexpr bool and_v = (true && ... && Bs);

    /// Logically AND together all the Boolean parameters
    /// \ingroup logical
    template <bool... Bs>
#if defined(META_WORKAROUND_MSVC_756112) || defined(META_WORKAROUND_GCC_86356)
    using and_c = bool_<and_v<Bs...>>;
#else
    using and_c = bool_<(true && ... && Bs)>;
#endif
#else
#if defined(META_WORKAROUND_GCC_66405)
    template <bool... Bs>
    using and_c = futures::detail::meta::bool_<META_IS_SAME(integer_sequence<bool, true, Bs...>, integer_sequence<bool, Bs..., true>)>;
#else
    template <bool... Bs>
    struct and_c : futures::detail::meta::bool_<META_IS_SAME(integer_sequence<bool, Bs...>, integer_sequence<bool, (Bs || true)...>)> {};
#endif
#if META_CXX_VARIABLE_TEMPLATES
    template <bool... Bs>
    META_INLINE_VAR constexpr bool and_v = META_IS_SAME(integer_sequence<bool, Bs...>,
                                                        integer_sequence<bool, (Bs || true)...>);
#endif
#endif

    /// Logically AND together all the integral constant-wrapped Boolean
    /// parameters, \e without short-circuiting.
    /// \ingroup logical
    template <META_TYPE_CONSTRAINT(integral)... Bs> using strict_and_ = and_c<Bs::type::value...>;

    template <typename... Bs> using strict_and = strict_and_<Bs...>;

    /// Logically AND together all the integral constant-wrapped Boolean
    /// parameters, \e with short-circuiting.
    /// \ingroup logical
    template <typename... Bs>
#ifdef META_CONCEPT
    using and_ = _t<ranges_detail::_and_<Bs...>>;
#else
    // Make a trip through defer<> to avoid CWG1430
    // https://wg21.link/cwg1430
    using and_ = _t<defer<ranges_detail::_and_<0 == sizeof...(Bs)>::template invoke, Bs...>>;
#endif

    /// Logically OR together all the Boolean parameters
    /// \ingroup logical
#if META_CXX_FOLD_EXPRESSIONS && !defined(META_WORKAROUND_GCC_UNKNOWN1)
    template <bool... Bs> META_INLINE_VAR constexpr bool or_v = (false || ... || Bs);

    template <bool... Bs>
#if defined(META_WORKAROUND_MSVC_756112) || defined(META_WORKAROUND_GCC_86356)
    using or_c = bool_<or_v<Bs...>>;
#else
    using or_c = bool_<(false || ... || Bs)>;
#endif
#else
    template <bool... Bs>
    struct or_c : futures::detail::meta::bool_<!META_IS_SAME(integer_sequence<bool, Bs...>, integer_sequence<bool, (Bs && false)...>)> {
    };
#if META_CXX_VARIABLE_TEMPLATES
    template <bool... Bs>
    META_INLINE_VAR constexpr bool or_v =
        !META_IS_SAME(integer_sequence<bool, Bs...>, integer_sequence<bool, (Bs && false)...>);
#endif
#endif

    /// Logically OR together all the integral constant-wrapped Boolean
    /// parameters, \e without short-circuiting.
    /// \ingroup logical
    template <META_TYPE_CONSTRAINT(integral)... Bs> using strict_or_ = or_c<Bs::type::value...>;

    template <typename... Bs> using strict_or = strict_or_<Bs...>;

    /// Logically OR together all the integral constant-wrapped Boolean
    /// parameters, \e with short-circuiting.
    /// \ingroup logical
    template <typename... Bs>
#ifdef META_CONCEPT
    using or_ = _t<ranges_detail::_or_<Bs...>>;
#else
    // Make a trip through defer<> to avoid CWG1430
    // https://wg21.link/cwg1430
    using or_ = _t<defer<ranges_detail::_or_<0 == sizeof...(Bs)>::template invoke, Bs...>>;
#endif

    namespace lazy {
        /// \sa 'futures::detail::meta::and_'
        /// \ingroup lazy_logical
        template <typename... Bs> using and_ = defer<and_, Bs...>;

        /// \sa 'futures::detail::meta::or_'
        /// \ingroup lazy_logical
        template <typename... Bs> using or_ = defer<or_, Bs...>;

        /// \sa 'futures::detail::meta::not_'
        /// \ingroup lazy_logical
        template <typename B> using not_ = defer<not_, B>;

        /// \sa 'futures::detail::meta::strict_and'
        /// \ingroup lazy_logical
        template <typename... Bs> using strict_and = defer<strict_and, Bs...>;

        /// \sa 'futures::detail::meta::strict_or'
        /// \ingroup lazy_logical
        template <typename... Bs> using strict_or = defer<strict_or, Bs...>;
    } // namespace lazy

    ///////////////////////////////////////////////////////////////////////////////////////////
    // fold
    /// \cond
    namespace ranges_detail {
        template <typename, typename, typename> struct fold_ {};

        template <typename Fn, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5,
                  typename T6, typename T7, typename T8, typename T9>
        struct compose10_ {
            template <typename X, typename Y> using F = invoke<Fn, X, Y>;

            template <typename S>
            using invoke = F<F<F<F<F<F<F<F<F<F<_t<S>, T0>, T1>, T2>, T3>, T4>, T5>, T6>, T7>, T8>, T9>;
        };

#ifdef META_CONCEPT
        template <typename Fn> struct compose_ {
            template <typename X, typename Y> using F = invoke<Fn, X, Y>;

            template <typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6,
                      typename T7, typename T8, typename T9, typename State>
            using invoke = F<F<F<F<F<F<F<F<F<F<State, T0>, T1>, T2>, T3>, T4>, T5>, T6>, T7>, T8>, T9>;
        };

        template <typename State, typename Fn> struct fold_<list<>, State, Fn> { using type = State; };

        template <typename Head, typename... Tail, typename State, typename Fn>
        requires valid<invoke, Fn, State, Head>
        struct fold_<list<Head, Tail...>, State, Fn> : fold_<list<Tail...>, invoke<Fn, State, Head>, Fn> {
        };

        template <typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6,
                  typename T7, typename T8, typename T9, typename... Tail, typename State, typename Fn>
        requires valid<invoke, compose_<Fn>, T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, State>
        struct fold_<list<T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, Tail...>, State, Fn>
            : fold_<list<Tail...>, invoke<compose_<Fn>, T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, State>, Fn> {
        };
#else  // ^^^ Concepts / no Concepts vvv
        template <typename Fn, typename T0> struct compose1_ {
            template <typename X> using invoke = invoke<Fn, _t<X>, T0>;
        };

        template <typename State, typename Fn> struct fold_<list<>, State, Fn> : State {};

        template <typename Head, typename... Tail, typename State, typename Fn>
        struct fold_<list<Head, Tail...>, State, Fn>
            : fold_<list<Tail...>, lazy::invoke<compose1_<Fn, Head>, State>, Fn> {};

        template <typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6,
                  typename T7, typename T8, typename T9, typename... Tail, typename State, typename Fn>
        struct fold_<list<T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, Tail...>, State, Fn>
            : fold_<list<Tail...>, lazy::invoke<compose10_<Fn, T0, T1, T2, T3, T4, T5, T6, T7, T8, T9>, State>, Fn> {};
#endif // META_CONCEPT
    }  // namespace ranges_detail
    /// \endcond

    /// Return a new \c futures::detail::meta::list constructed by doing a left fold of the list \p L using
    /// binary invocable \p Fn and initial state \p State. That is, the \c State_N for
    /// the list element \c A_N is computed by `Fn(State_N-1, A_N) -> State_N`.
    /// \par Complexity
    /// \f$ O(N) \f$.
    /// \ingroup transformation
    template <META_TYPE_CONSTRAINT(list_like) L, typename State, META_TYPE_CONSTRAINT(invocable) Fn>
#ifdef META_CONCEPT
    using fold = _t<ranges_detail::fold_<L, State, Fn>>;
#else
    using fold = _t<ranges_detail::fold_<L, id<State>, Fn>>;
#endif

    /// An alias for `futures::detail::meta::fold`.
    /// \par Complexity
    /// \f$ O(N) \f$.
    /// \ingroup transformation
    template <META_TYPE_CONSTRAINT(list_like) L, typename State, META_TYPE_CONSTRAINT(invocable) Fn>
    using accumulate = fold<L, State, Fn>;

    namespace lazy {
        /// \sa 'futures::detail::meta::foldl'
        /// \ingroup lazy_transformation
        template <typename L, typename State, typename Fn> using fold = defer<fold, L, State, Fn>;

        /// \sa 'futures::detail::meta::accumulate'
        /// \ingroup lazy_transformation
        template <typename L, typename State, typename Fn> using accumulate = defer<accumulate, L, State, Fn>;
    } // namespace lazy

    ///////////////////////////////////////////////////////////////////////////////////////////
    // reverse_fold
    /// \cond
    namespace ranges_detail {
        template <typename, typename, typename> struct reverse_fold_ {};

        template <typename State, typename Fn> struct reverse_fold_<list<>, State, Fn> { using type = State; };

#ifdef META_CONCEPT
        template <typename Head, typename... L, typename State, typename Fn>
        requires trait<reverse_fold_<list<L...>, State, Fn>>
        struct reverse_fold_<list<Head, L...>, State, Fn>
            : lazy::invoke<Fn, _t<reverse_fold_<list<L...>, State, Fn>>, Head> {
        };
#else
        template <typename Head, typename... Tail, typename State, typename Fn>
        struct reverse_fold_<list<Head, Tail...>, State, Fn>
            : lazy::invoke<compose1_<Fn, Head>, reverse_fold_<list<Tail...>, State, Fn>> {};
#endif

        template <typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6,
                  typename T7, typename T8, typename T9, typename... Tail, typename State, typename Fn>
        struct reverse_fold_<list<T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, Tail...>, State, Fn>
            : lazy::invoke<compose10_<Fn, T9, T8, T7, T6, T5, T4, T3, T2, T1, T0>,
                           reverse_fold_<list<Tail...>, State, Fn>> {};
    } // namespace ranges_detail
    /// \endcond

    /// Return a new \c futures::detail::meta::list constructed by doing a right fold of the list \p L using
    /// binary invocable \p Fn and initial state \p State. That is, the \c State_N for the list
    /// element \c A_N is computed by `Fn(A_N, State_N+1) -> State_N`.
    /// \par Complexity
    /// \f$ O(N) \f$.
    /// \ingroup transformation
    template <META_TYPE_CONSTRAINT(list_like) L, typename State, META_TYPE_CONSTRAINT(invocable) Fn>
    using reverse_fold = _t<ranges_detail::reverse_fold_<L, State, Fn>>;

    namespace lazy {
        /// \sa 'futures::detail::meta::foldr'
        /// \ingroup lazy_transformation
        template <typename L, typename State, typename Fn> using reverse_fold = defer<reverse_fold, L, State, Fn>;
    } // namespace lazy

    ///////////////////////////////////////////////////////////////////////////////////////////
    // npos
    /// A special value used to indicate no matches. It equals the maximum
    /// value representable by std::size_t.
    /// \ingroup list
    using npos = futures::detail::meta::size_t<std::size_t(-1)>;

    ///////////////////////////////////////////////////////////////////////////////////////////
    // list
    /// A list of types.
    /// \ingroup list
    template <typename... Ts> struct list {
        using type = list;
        /// \return `sizeof...(Ts)`
        static constexpr std::size_t size() noexcept { return sizeof...(Ts); }
    };

    ///////////////////////////////////////////////////////////////////////////////////////////
    // size
    /// An integral constant wrapper that is the size of the \c futures::detail::meta::list
    /// \p L.
    /// \ingroup list
    template <META_TYPE_CONSTRAINT(list_like) L> using size = futures::detail::meta::size_t<L::size()>;

    namespace lazy {
        /// \sa 'futures::detail::meta::size'
        /// \ingroup lazy_list
        template <typename L> using size = defer<size, L>;
    } // namespace lazy

    ///////////////////////////////////////////////////////////////////////////////////////////
    // concat
    /// \cond
    namespace ranges_detail {
        template <typename... Lists> struct concat_ {};

        template <> struct concat_<> { using type = list<>; };

        template <typename... L1> struct concat_<list<L1...>> { using type = list<L1...>; };

        template <typename... L1, typename... L2> struct concat_<list<L1...>, list<L2...>> {
            using type = list<L1..., L2...>;
        };

        template <typename... L1, typename... L2, typename... L3>
        struct concat_<list<L1...>, list<L2...>, list<L3...>> {
            using type = list<L1..., L2..., L3...>;
        };

        template <typename... L1, typename... L2, typename... L3, typename... Rest>
        struct concat_<list<L1...>, list<L2...>, list<L3...>, Rest...> : concat_<list<L1..., L2..., L3...>, Rest...> {};

        template <typename... L1, typename... L2, typename... L3, typename... L4, typename... L5, typename... L6,
                  typename... L7, typename... L8, typename... L9, typename... L10, typename... Rest>
        struct concat_<list<L1...>, list<L2...>, list<L3...>, list<L4...>, list<L5...>, list<L6...>, list<L7...>,
                       list<L8...>, list<L9...>, list<L10...>, Rest...>
            : concat_<list<L1..., L2..., L3..., L4..., L5..., L6..., L7..., L8..., L9..., L10...>, Rest...> {};
    } // namespace ranges_detail
    /// \endcond

    /// Concatenates several lists into a single list.
    /// \pre The parameters must all be instantiations of \c futures::detail::meta::list.
    /// \par Complexity
    /// \f$ O(L) \f$ where \f$ L \f$ is the number of lists in the list of lists.
    /// \ingroup transformation
    template <META_TYPE_CONSTRAINT(list_like)... Ls> using concat_ = _t<ranges_detail::concat_<Ls...>>;

    template <typename... Lists> using concat = concat_<Lists...>;

    namespace lazy {
        /// \sa 'futures::detail::meta::concat'
        /// \ingroup lazy_transformation
        template <typename... Lists> using concat = defer<concat, Lists...>;
    } // namespace lazy

    /// Joins a list of lists into a single list.
    /// \pre The parameter must be an instantiation of \c futures::detail::meta::list\<T...\>
    /// where each \c T is itself an instantiation of \c futures::detail::meta::list.
    /// \par Complexity
    /// \f$ O(L) \f$ where \f$ L \f$ is the number of lists in the list of
    /// lists.
    /// \ingroup transformation
    template <META_TYPE_CONSTRAINT(list_like) ListOfLists> using join = apply<quote<concat>, ListOfLists>;

    namespace lazy {
        /// \sa 'futures::detail::meta::join'
        /// \ingroup lazy_transformation
        template <typename ListOfLists> using join = defer<join, ListOfLists>;
    } // namespace lazy

    ///////////////////////////////////////////////////////////////////////////////////////////
    // transform
    /// \cond
    namespace ranges_detail {
#ifdef META_CONCEPT
        template <typename... Args> struct transform_ {};

        template <typename... Ts, invocable Fn>
        requires and_v<valid<invoke, Fn, Ts>...>
        struct transform_<list<Ts...>, Fn> {
            using type = list<invoke<Fn, Ts>...>;
        };

        template <typename... Ts, typename... Us, invocable Fn>
        requires and_v<valid<invoke, Fn, Ts, Us>...>
        struct transform_<list<Ts...>, list<Us...>, Fn> {
            using type = list<invoke<Fn, Ts, Us>...>;
        };
#else
        template <typename, typename = void> struct transform_ {};

        template <typename... Ts, typename Fn> struct transform_<list<list<Ts...>, Fn>, void_<invoke<Fn, Ts>...>> {
            using type = list<invoke<Fn, Ts>...>;
        };

        template <typename... Ts0, typename... Ts1, typename Fn>
        struct transform_<list<list<Ts0...>, list<Ts1...>, Fn>, void_<invoke<Fn, Ts0, Ts1>...>> {
            using type = list<invoke<Fn, Ts0, Ts1>...>;
        };
#endif
    } // namespace ranges_detail
      /// \endcond

    /// Return a new \c futures::detail::meta::list constructed by transforming all the
    /// elements in \p L with the unary invocable \p Fn. \c transform can
    /// also be called with two lists of the same length and a binary
    /// invocable, in which case it returns a new list constructed with the
    /// results of calling \c Fn with each element in the lists, pairwise.
    /// \par Complexity
    /// \f$ O(N) \f$.
    /// \ingroup transformation
#ifdef META_CONCEPT
    template <typename... Args> using transform = _t<ranges_detail::transform_<Args...>>;
#else
    template <typename... Args> using transform = _t<ranges_detail::transform_<list<Args...>>>;
#endif

    namespace lazy {
        /// \sa 'futures::detail::meta::transform'
        /// \ingroup lazy_transformation
        template <typename... Args> using transform = defer<transform, Args...>;
    } // namespace lazy

    ///////////////////////////////////////////////////////////////////////////////////////////
    // repeat_n
    /// \cond
    namespace ranges_detail {
        template <typename T, std::size_t> using first_ = T;

        template <typename T, typename Ints> struct repeat_n_c_ {};

        template <typename T, std::size_t... Is> struct repeat_n_c_<T, index_sequence<Is...>> {
            using type = list<first_<T, Is>...>;
        };
    } // namespace ranges_detail
    /// \endcond

    /// Generate `list<T,T,T...T>` of size \p N arguments.
    /// \par Complexity
    /// \f$ O(log N) \f$.
    /// \ingroup list
    template <std::size_t N, typename T = void> using repeat_n_c = _t<ranges_detail::repeat_n_c_<T, make_index_sequence<N>>>;

    /// Generate `list<T,T,T...T>` of size \p N arguments.
    /// \par Complexity
    /// \f$ O(log N) \f$.
    /// \ingroup list
    template <META_TYPE_CONSTRAINT(integral) N, typename T = void> using repeat_n = repeat_n_c<N::type::value, T>;

    namespace lazy {
        /// \sa 'futures::detail::meta::repeat_n'
        /// \ingroup lazy_list
        template <typename N, typename T = void> using repeat_n = defer<repeat_n, N, T>;

        /// \sa 'futures::detail::meta::repeat_n_c'
        /// \ingroup lazy_list
        template <std::size_t N, typename T = void> using repeat_n_c = defer<repeat_n, futures::detail::meta::size_t<N>, T>;
    } // namespace lazy

    ///////////////////////////////////////////////////////////////////////////////////////////
    // at
    /// \cond
    namespace ranges_detail {
#if META_HAS_TYPE_PACK_ELEMENT && !defined(META_DOXYGEN_INVOKED)
        template <typename L, std::size_t N, typename = void> struct at_ {};

        template <typename... Ts, std::size_t N> struct at_<list<Ts...>, N, void_<__type_pack_element<N, Ts...>>> {
            using type = __type_pack_element<N, Ts...>;
        };
#else
        template <typename VoidPtrs> struct at_impl_;

        template <typename... VoidPtrs> struct at_impl_<list<VoidPtrs...>> {
            static nil_ eval(...);

            template <typename T, typename... Us> static T eval(VoidPtrs..., T *, Us *...);
        };

        template <typename L, std::size_t N> struct at_ {};

        template <typename... Ts, std::size_t N>
        struct at_<list<Ts...>, N>
            : decltype(at_impl_<repeat_n_c<N, void *>>::eval(static_cast<id<Ts> *>(nullptr)...)) {};
#endif // META_HAS_TYPE_PACK_ELEMENT
    }  // namespace ranges_detail
    /// \endcond

    /// Return the \p N th element in the \c futures::detail::meta::list \p L.
    /// \par Complexity
    /// Amortized \f$ O(1) \f$.
    /// \ingroup list
    template <META_TYPE_CONSTRAINT(list_like) L, std::size_t N> using at_c = _t<ranges_detail::at_<L, N>>;

    /// Return the \p N th element in the \c futures::detail::meta::list \p L.
    /// \par Complexity
    /// Amortized \f$ O(1) \f$.
    /// \ingroup list
    template <META_TYPE_CONSTRAINT(list_like) L, META_TYPE_CONSTRAINT(integral) N> using at = at_c<L, N::type::value>;

    namespace lazy {
        /// \sa 'futures::detail::meta::at'
        /// \ingroup lazy_list
        template <typename L, typename N> using at = defer<at, L, N>;
    } // namespace lazy

    ///////////////////////////////////////////////////////////////////////////////////////////
    // drop
    /// \cond
    namespace ranges_detail {
        ///////////////////////////////////////////////////////////////////////////////////////
        /// drop_impl_
        template <typename VoidPtrs> struct drop_impl_ { static nil_ eval(...); };

        template <typename... VoidPtrs> struct drop_impl_<list<VoidPtrs...>> {
            static nil_ eval(...);

            template <typename... Ts> static id<list<Ts...>> eval(VoidPtrs..., id<Ts> *...);
        };

        template <> struct drop_impl_<list<>> { template <typename... Ts> static id<list<Ts...>> eval(id<Ts> *...); };

        template <typename L, std::size_t N> struct drop_ {};

        template <typename... Ts, std::size_t N>
        struct drop_<list<Ts...>, N>
#if META_CXX_VARIABLE_TEMPLATES
            : decltype(drop_impl_<repeat_n_c<N, void *>>::eval(ranges_detail::nullptr_v<id<Ts>>...))
#else
            : decltype(drop_impl_<repeat_n_c<N, void *>>::eval(ranges_detail::_nullptr_v<id<Ts>>()...))
#endif
        {
        };
    } // namespace ranges_detail
    /// \endcond

    /// Return a new \c futures::detail::meta::list by removing the first \p N elements from \p L.
    /// \par Complexity
    /// \f$ O(1) \f$.
    /// \ingroup transformation
    template <META_TYPE_CONSTRAINT(list_like) L, std::size_t N> using drop_c = _t<ranges_detail::drop_<L, N>>;

    /// Return a new \c futures::detail::meta::list by removing the first \p N elements from \p L.
    /// \par Complexity
    /// \f$ O(1) \f$.
    /// \ingroup transformation
    template <META_TYPE_CONSTRAINT(list_like) L, META_TYPE_CONSTRAINT(integral) N>
    using drop = drop_c<L, N::type::value>;

    namespace lazy {
        /// \sa 'futures::detail::meta::drop'
        /// \ingroup lazy_transformation
        template <typename L, typename N> using drop = defer<drop, L, N>;
    } // namespace lazy

    ///////////////////////////////////////////////////////////////////////////////////////////
    // front
    /// \cond
    namespace ranges_detail {
        template <typename L> struct front_ {};

        template <typename Head, typename... Tail> struct front_<list<Head, Tail...>> { using type = Head; };
    } // namespace ranges_detail
    /// \endcond

    /// Return the first element in \c futures::detail::meta::list \p L.
    /// \par Complexity
    /// \f$ O(1) \f$.
    /// \ingroup list
    template <META_TYPE_CONSTRAINT(list_like) L> using front = _t<ranges_detail::front_<L>>;

    namespace lazy {
        /// \sa 'futures::detail::meta::front'
        /// \ingroup lazy_list
        template <typename L> using front = defer<front, L>;
    } // namespace lazy

    ///////////////////////////////////////////////////////////////////////////////////////////
    // back
    /// \cond
    namespace ranges_detail {
        template <typename L> struct back_ {};

        template <typename Head, typename... Tail> struct back_<list<Head, Tail...>> {
            using type = at_c<list<Head, Tail...>, sizeof...(Tail)>;
        };
    } // namespace ranges_detail
    /// \endcond

    /// Return the last element in \c futures::detail::meta::list \p L.
    /// \par Complexity
    /// Amortized \f$ O(1) \f$.
    /// \ingroup list
    template <META_TYPE_CONSTRAINT(list_like) L> using back = _t<ranges_detail::back_<L>>;

    namespace lazy {
        /// \sa 'futures::detail::meta::back'
        /// \ingroup lazy_list
        template <typename L> using back = defer<back, L>;
    } // namespace lazy

    ///////////////////////////////////////////////////////////////////////////////////////////
    // push_front
    /// Return a new \c futures::detail::meta::list by adding the element \c T to the front of \p L.
    /// \par Complexity
    /// \f$ O(1) \f$.
    /// \ingroup transformation
    template <META_TYPE_CONSTRAINT(list_like) L, typename... Ts>
    using push_front = apply<bind_front<quote<list>, Ts...>, L>;

    namespace lazy {
        /// \sa 'futures::detail::meta::push_front'
        /// \ingroup lazy_transformation
        template <typename... Ts> using push_front = defer<push_front, Ts...>;
    } // namespace lazy

    ///////////////////////////////////////////////////////////////////////////////////////////
    // pop_front
    /// \cond
    namespace ranges_detail {
        template <typename L> struct pop_front_ {};

        template <typename Head, typename... L> struct pop_front_<list<Head, L...>> { using type = list<L...>; };
    } // namespace ranges_detail
    /// \endcond

    /// Return a new \c futures::detail::meta::list by removing the first element from the
    /// front of \p L.
    /// \par Complexity
    /// \f$ O(1) \f$.
    /// \ingroup transformation
    template <META_TYPE_CONSTRAINT(list_like) L> using pop_front = _t<ranges_detail::pop_front_<L>>;

    namespace lazy {
        /// \sa 'futures::detail::meta::pop_front'
        /// \ingroup lazy_transformation
        template <typename L> using pop_front = defer<pop_front, L>;
    } // namespace lazy

    ///////////////////////////////////////////////////////////////////////////////////////////
    // push_back
    /// Return a new \c futures::detail::meta::list by adding the element \c T to the back of \p L.
    /// \par Complexity
    /// \f$ O(1) \f$.
    /// \note \c pop_back not provided because it cannot be made to meet the
    /// complexity guarantees one would expect.
    /// \ingroup transformation
    template <META_TYPE_CONSTRAINT(list_like) L, typename... Ts>
    using push_back = apply<bind_back<quote<list>, Ts...>, L>;

    namespace lazy {
        /// \sa 'futures::detail::meta::push_back'
        /// \ingroup lazy_transformation
        template <typename... Ts> using push_back = defer<push_back, Ts...>;
    } // namespace lazy

    /// \cond
    namespace ranges_detail {
        template <typename T, typename U> using min_ = if_<less<U, T>, U, T>;

        template <typename T, typename U> using max_ = if_<less<U, T>, T, U>;
    } // namespace ranges_detail
    /// \endcond

    /// An integral constant wrapper around the minimum of `Ts::type::value...`
    /// \ingroup math
    template <META_TYPE_CONSTRAINT(integral)... Ts>
    using min_ = fold<pop_front<list<Ts...>>, front<list<Ts...>>, quote<ranges_detail::min_>>;

    template <typename... Ts> using min = min_<Ts...>;

    /// An integral constant wrapper around the maximum of `Ts::type::value...`
    /// \ingroup math
    template <META_TYPE_CONSTRAINT(integral)... Ts>
    using max_ = fold<pop_front<list<Ts...>>, front<list<Ts...>>, quote<ranges_detail::max_>>;

    template <typename... Ts> using max = max_<Ts...>;

    namespace lazy {
        /// \sa 'futures::detail::meta::min'
        /// \ingroup lazy_math
        template <typename... Ts> using min = defer<min, Ts...>;

        /// \sa 'futures::detail::meta::max'
        /// \ingroup lazy_math
        template <typename... Ts> using max = defer<max, Ts...>;
    } // namespace lazy

    ///////////////////////////////////////////////////////////////////////////////////////////
    // empty
    /// An Boolean integral constant wrapper around \c true if \p L is an
    /// empty type list; \c false, otherwise.
    /// \par Complexity
    /// \f$ O(1) \f$.
    /// \ingroup list
    template <META_TYPE_CONSTRAINT(list_like) L> using empty = bool_<0 == size<L>::type::value>;

    namespace lazy {
        /// \sa 'futures::detail::meta::empty'
        /// \ingroup lazy_list
        template <typename L> using empty = defer<empty, L>;
    } // namespace lazy

    ///////////////////////////////////////////////////////////////////////////////////////////
    // pair
    /// A list with exactly two elements
    /// \ingroup list
    template <typename F, typename S> using pair = list<F, S>;

    /// Retrieve the first element of the \c pair \p Pair
    /// \ingroup list
    template <typename Pair> using first = front<Pair>;

    /// Retrieve the first element of the \c pair \p Pair
    /// \ingroup list
    template <typename Pair> using second = front<pop_front<Pair>>;

    namespace lazy {
        /// \sa 'futures::detail::meta::first'
        /// \ingroup lazy_list
        template <typename Pair> using first = defer<first, Pair>;

        /// \sa 'futures::detail::meta::second'
        /// \ingroup lazy_list
        template <typename Pair> using second = defer<second, Pair>;
    } // namespace lazy

    ///////////////////////////////////////////////////////////////////////////////////////////
    // find_index
    /// \cond
    namespace ranges_detail {
        // With thanks to Peter Dimov:
        constexpr std::size_t find_index_i_(bool const *const first, bool const *const last, std::size_t N = 0) {
            return first == last ? npos::value : *first ? N : find_index_i_(first + 1, last, N + 1);
        }

        template <typename L, typename T> struct find_index_ {};

        template <typename V> struct find_index_<list<>, V> { using type = npos; };

        template <typename... T, typename V> struct find_index_<list<T...>, V> {
#ifdef META_WORKAROUND_LLVM_28385
            static constexpr bool s_v[sizeof...(T)] = {META_IS_SAME(T, V)...};
#else
            static constexpr bool s_v[] = {META_IS_SAME(T, V)...};
#endif
            using type = size_t<find_index_i_(s_v, s_v + sizeof...(T))>;
        };
    } // namespace ranges_detail
    /// \endcond

    /// Finds the index of the first occurrence of the type \p T within the list \p L.
    /// Returns `#futures::detail::meta::npos` if the type \p T was not found.
    /// \par Complexity
    /// \f$ O(N) \f$.
    /// \ingroup query
    /// \sa `futures::detail::meta::npos`
    template <META_TYPE_CONSTRAINT(list_like) L, typename T> using find_index = _t<ranges_detail::find_index_<L, T>>;

    namespace lazy {
        /// \sa 'futures::detail::meta::find_index'
        /// \ingroup lazy_query
        template <typename L, typename T> using find_index = defer<find_index, L, T>;
    } // namespace lazy

    ///////////////////////////////////////////////////////////////////////////////////////////
    // reverse_find_index
    /// \cond
    namespace ranges_detail {
        // With thanks to Peter Dimov:
        constexpr std::size_t reverse_find_index_i_(bool const *const first, bool const *const last, std::size_t N) {
            return first == last ? npos::value : *(last - 1) ? N - 1 : reverse_find_index_i_(first, last - 1, N - 1);
        }

        template <typename L, typename T> struct reverse_find_index_ {};

        template <typename V> struct reverse_find_index_<list<>, V> { using type = npos; };

        template <typename... T, typename V> struct reverse_find_index_<list<T...>, V> {
#ifdef META_WORKAROUND_LLVM_28385
            static constexpr bool s_v[sizeof...(T)] = {META_IS_SAME(T, V)...};
#else
            static constexpr bool s_v[] = {META_IS_SAME(T, V)...};
#endif
            using type = size_t<reverse_find_index_i_(s_v, s_v + sizeof...(T), sizeof...(T))>;
        };
    } // namespace ranges_detail
    /// \endcond

    /// Finds the index of the last occurrence of the type \p T within the
    /// list \p L. Returns `#futures::detail::meta::npos` if the type \p T was not found.
    /// \par Complexity
    /// \f$ O(N) \f$.
    /// \ingroup query
    /// \sa `#futures::detail::meta::npos`
    template <META_TYPE_CONSTRAINT(list_like) L, typename T>
    using reverse_find_index = _t<ranges_detail::reverse_find_index_<L, T>>;

    namespace lazy {
        /// \sa 'futures::detail::meta::reverse_find_index'
        /// \ingroup lazy_query
        template <typename L, typename T> using reverse_find_index = defer<reverse_find_index, L, T>;
    } // namespace lazy

    ////////////////////////////////////////////////////////////////////////////////////
    // find
    /// Return the tail of the list \p L starting at the first occurrence of
    /// \p T, if any such element exists; the empty list, otherwise.
    /// \par Complexity
    /// \f$ O(N) \f$.
    /// \ingroup query
    template <META_TYPE_CONSTRAINT(list_like) L, typename T> using find = drop<L, min<find_index<L, T>, size<L>>>;

    namespace lazy {
        /// \sa 'futures::detail::meta::find'
        /// \ingroup lazy_query
        template <typename L, typename T> using find = defer<find, L, T>;
    } // namespace lazy

    ////////////////////////////////////////////////////////////////////////////////////
    // reverse_find
    /// \cond
    namespace ranges_detail {
        template <typename L, typename T, typename State = list<>> struct reverse_find_ {};

        template <typename T, typename State> struct reverse_find_<list<>, T, State> { using type = State; };

        template <typename Head, typename... L, typename T, typename State>
        struct reverse_find_<list<Head, L...>, T, State> : reverse_find_<list<L...>, T, State> {};

        template <typename... L, typename T, typename State>
        struct reverse_find_<list<T, L...>, T, State> : reverse_find_<list<L...>, T, list<T, L...>> {};
    } // namespace ranges_detail
    /// \endcond

    /// Return the tail of the list \p L starting at the last occurrence of \p T, if any such
    /// element exists; the empty list, otherwise.
    /// \par Complexity
    /// \f$ O(N) \f$.
    /// \ingroup query
    template <META_TYPE_CONSTRAINT(list_like) L, typename T>
    using reverse_find = drop<L, min<reverse_find_index<L, T>, size<L>>>;

    namespace lazy {
        /// \sa 'futures::detail::meta::rfind'
        /// \ingroup lazy_query
        template <typename L, typename T> using reverse_find = defer<reverse_find, L, T>;
    } // namespace lazy

    ///////////////////////////////////////////////////////////////////////////////////////////
    // find_if
    /// \cond
    namespace ranges_detail {
#ifdef META_CONCEPT
        template <typename L, typename Fn> struct find_if_ {};

        template <typename Fn> struct find_if_<list<>, Fn> { using type = list<>; };

        template <typename Head, typename... L, typename Fn>
        requires integral<invoke<Fn, Head>>
        struct find_if_<list<Head, L...>, Fn> : if_<invoke<Fn, Head>, id<list<Head, L...>>, find_if_<list<L...>, Fn>> {
        };
#else
        constexpr bool const *find_if_i_(bool const *const begin, bool const *const end) {
            return begin == end || *begin ? begin : find_if_i_(begin + 1, end);
        }

        template <typename L, typename Fn, typename = void> struct find_if_ {};

        template <typename Fn> struct find_if_<list<>, Fn> { using type = list<>; };

        template <typename... L, typename Fn>
        struct find_if_<list<L...>, Fn, void_<integer_sequence<bool, bool(invoke<Fn, L>::type::value)...>>> {
#ifdef META_WORKAROUND_LLVM_28385
            static constexpr bool s_v[sizeof...(L)] = {invoke<Fn, L>::type::value...};
#else
            static constexpr bool s_v[] = {invoke<Fn, L>::type::value...};
#endif
            using type = drop_c<list<L...>, ranges_detail::find_if_i_(s_v, s_v + sizeof...(L)) - s_v>;
        };
#endif
    } // namespace ranges_detail
    /// \endcond

    /// Return the tail of the list \p L starting at the first element `A`
    /// such that `invoke<Fn, A>::%value` is \c true, if any such element
    /// exists; the empty list, otherwise.
    /// \par Complexity
    /// \f$ O(N) \f$.
    /// \ingroup query
    template <META_TYPE_CONSTRAINT(list_like) L, META_TYPE_CONSTRAINT(invocable) Fn>
    using find_if = _t<ranges_detail::find_if_<L, Fn>>;

    namespace lazy {
        /// \sa 'futures::detail::meta::find_if'
        /// \ingroup lazy_query
        template <typename L, typename Fn> using find_if = defer<find_if, L, Fn>;
    } // namespace lazy

    ////////////////////////////////////////////////////////////////////////////////////
    // reverse_find_if
    /// \cond
    namespace ranges_detail {
#ifdef META_CONCEPT
        template <typename L, typename Fn, typename State = list<>> struct reverse_find_if_ {};

        template <typename Fn, typename State> struct reverse_find_if_<list<>, Fn, State> { using type = State; };

        template <typename Head, typename... L, typename Fn, typename State>
        requires integral<invoke<Fn, Head>>
        struct reverse_find_if_<list<Head, L...>, Fn, State>
            : reverse_find_if_<list<L...>, Fn, if_<invoke<Fn, Head>, list<Head, L...>, State>> {
        };
#else
        constexpr bool const *reverse_find_if_i_(bool const *const begin, bool const *const pos,
                                                 bool const *const end) {
            return begin == pos ? end : *(pos - 1) ? pos - 1 : reverse_find_if_i_(begin, pos - 1, end);
        }

        template <typename L, typename Fn, typename = void> struct reverse_find_if_ {};

        template <typename Fn> struct reverse_find_if_<list<>, Fn> { using type = list<>; };

        template <typename... L, typename Fn>
        struct reverse_find_if_<list<L...>, Fn, void_<integer_sequence<bool, bool(invoke<Fn, L>::type::value)...>>> {
#ifdef META_WORKAROUND_LLVM_28385
            static constexpr bool s_v[sizeof...(L)] = {invoke<Fn, L>::type::value...};
#else
            static constexpr bool s_v[] = {invoke<Fn, L>::type::value...};
#endif
            using type =
                drop_c<list<L...>, ranges_detail::reverse_find_if_i_(s_v, s_v + sizeof...(L), s_v + sizeof...(L)) - s_v>;
        };
#endif
    } // namespace ranges_detail
    /// \endcond

    /// Return the tail of the list \p L starting at the last element `A`
    /// such that `invoke<Fn, A>::%value` is \c true, if any such element
    /// exists; the empty list, otherwise.
    /// \par Complexity
    /// \f$ O(N) \f$.
    /// \ingroup query
    template <META_TYPE_CONSTRAINT(list_like) L, META_TYPE_CONSTRAINT(invocable) Fn>
    using reverse_find_if = _t<ranges_detail::reverse_find_if_<L, Fn>>;

    namespace lazy {
        /// \sa 'futures::detail::meta::rfind_if'
        /// \ingroup lazy_query
        template <typename L, typename Fn> using reverse_find_if = defer<reverse_find_if, L, Fn>;
    } // namespace lazy

    ///////////////////////////////////////////////////////////////////////////////////////////
    // replace
    /// \cond
    namespace ranges_detail {
        template <typename L, typename T, typename U> struct replace_ {};

        template <typename... L, typename T, typename U> struct replace_<list<L...>, T, U> {
            using type = list<if_c<META_IS_SAME(T, L), U, L>...>;
        };
    } // namespace ranges_detail
    /// \endcond

    /// Return a new \c futures::detail::meta::list where all instances of type \p T have
    /// been replaced with \p U.
    /// \par Complexity
    /// \f$ O(N) \f$.
    /// \ingroup transformation
    template <META_TYPE_CONSTRAINT(list_like) L, typename T, typename U> using replace = _t<ranges_detail::replace_<L, T, U>>;

    namespace lazy {
        /// \sa 'futures::detail::meta::replace'
        /// \ingroup lazy_transformation
        template <typename L, typename T, typename U> using replace = defer<replace, T, U>;
    } // namespace lazy

    ///////////////////////////////////////////////////////////////////////////////////////////
    // replace_if
    /// \cond
    namespace ranges_detail {
#ifdef META_CONCEPT
        template <typename L, typename C, typename U> struct replace_if_ {};

        template <typename... L, typename C, typename U>
        requires and_v<integral<invoke<C, L>>...>
        struct replace_if_<list<L...>, C, U> {
            using type = list<if_<invoke<C, L>, U, L>...>;
        };
#else
        template <typename L, typename C, typename U, typename = void> struct replace_if_ {};

        template <typename... L, typename C, typename U>
        struct replace_if_<list<L...>, C, U, void_<integer_sequence<bool, bool(invoke<C, L>::type::value)...>>> {
            using type = list<if_<invoke<C, L>, U, L>...>;
        };
#endif
    } // namespace ranges_detail
    /// \endcond

    /// Return a new \c futures::detail::meta::list where all elements \c A of the list \p L
    /// for which `invoke<C,A>::%value` is \c true have been replaced with
    /// \p U.
    /// \par Complexity
    /// \f$ O(N) \f$.
    /// \ingroup transformation
    template <META_TYPE_CONSTRAINT(list_like) L, typename C, typename U>
    using replace_if = _t<ranges_detail::replace_if_<L, C, U>>;

    namespace lazy {
        /// \sa 'futures::detail::meta::replace_if'
        /// \ingroup lazy_transformation
        template <typename L, typename C, typename U> using replace_if = defer<replace_if, C, U>;
    } // namespace lazy

    ///////////////////////////////////////////////////////////////////////////////////////
    // count
    namespace ranges_detail {
        template <typename, typename> struct count_ {};

#if (defined(META_CONCEPT) || META_CXX_VARIABLE_TEMPLATES) && META_CXX_FOLD_EXPRESSIONS
        template <typename... Ts, typename T> struct count_<list<Ts...>, T> {
            using type = futures::detail::meta::size_t<((std::size_t)META_IS_SAME(T, Ts) + ...)>;
        };
#else
        constexpr std::size_t count_i_(bool const *const begin, bool const *const end, std::size_t n) {
            return begin == end ? n : ranges_detail::count_i_(begin + 1, end, n + *begin);
        }

        template <typename T> struct count_<list<>, T> { using type = futures::detail::meta::size_t<0>; };

        template <typename... L, typename T> struct count_<list<L...>, T> {
#ifdef META_WORKAROUND_LLVM_28385
            static constexpr bool s_v[sizeof...(L)] = {META_IS_SAME(T, L)...};
#else
            static constexpr bool s_v[] = {META_IS_SAME(T, L)...};
#endif
            using type = futures::detail::meta::size_t<ranges_detail::count_i_(s_v, s_v + sizeof...(L), 0u)>;
        };
#endif
    } // namespace ranges_detail

    /// Count the number of times a type \p T appears in the list \p L.
    /// \par Complexity
    /// \f$ O(N) \f$.
    /// \ingroup query
    template <META_TYPE_CONSTRAINT(list_like) L, typename T> using count = _t<ranges_detail::count_<L, T>>;

    namespace lazy {
        /// \sa `futures::detail::meta::count`
        /// \ingroup lazy_query
        template <typename L, typename T> using count = defer<count, L, T>;
    } // namespace lazy

    ///////////////////////////////////////////////////////////////////////////////////////
    // count_if
    namespace ranges_detail {
#if defined(META_CONCEPT) && META_CXX_FOLD_EXPRESSIONS
        template <typename, typename> struct count_if_ {};

        template <typename... Ts, typename Fn>
        requires(integral<invoke<Fn, Ts>> &&...) struct count_if_<list<Ts...>, Fn> {
            using type = futures::detail::meta::size_t<((std::size_t)(bool)_v<invoke<Fn, Ts>> + ...)>;
        };
#else
        template <typename L, typename Fn, typename = void> struct count_if_ {};

        template <typename Fn> struct count_if_<list<>, Fn> { using type = futures::detail::meta::size_t<0>; };

        template <typename... L, typename Fn>
        struct count_if_<list<L...>, Fn, void_<integer_sequence<bool, bool(invoke<Fn, L>::type::value)...>>> {
#if META_CXX_FOLD_EXPRESSIONS
            using type = futures::detail::meta::size_t<((std::size_t)(bool)invoke<Fn, L>::type::value + ...)>;
#else
#ifdef META_WORKAROUND_LLVM_28385
            static constexpr bool s_v[sizeof...(L)] = {invoke<Fn, L>::type::value...};
#else
            static constexpr bool s_v[] = {invoke<Fn, L>::type::value...};
#endif
            using type = futures::detail::meta::size_t<ranges_detail::count_i_(s_v, s_v + sizeof...(L), 0u)>;
#endif // META_CXX_FOLD_EXPRESSIONS
        };
#endif // META_CONCEPT
    }  // namespace ranges_detail

    /// Count the number of times the predicate \p Fn evaluates to true for all the elements in
    /// the list \p L.
    /// \par Complexity
    /// \f$ O(N) \f$.
    /// \ingroup query
    template <META_TYPE_CONSTRAINT(list_like) L, META_TYPE_CONSTRAINT(invocable) Fn>
    using count_if = _t<ranges_detail::count_if_<L, Fn>>;

    namespace lazy {
        /// \sa `futures::detail::meta::count_if`
        /// \ingroup lazy_query
        template <typename L, typename Fn> using count_if = defer<count_if, L, Fn>;
    } // namespace lazy

    ///////////////////////////////////////////////////////////////////////////////////////////
    // filter
    /// \cond
    namespace ranges_detail {
        template <typename Pred> struct filter_ {
            template <typename A> using invoke = if_c<invoke<Pred, A>::type::value, list<A>, list<>>;
        };
    } // namespace ranges_detail
    /// \endcond

    /// Returns a new futures::detail::meta::list where only those elements of \p L that satisfy the
    /// Callable \p Pred such that `invoke<Pred,A>::%value` is \c true are present.
    /// That is, those elements that don't satisfy the \p Pred are "removed".
    /// \par Complexity
    /// \f$ O(N) \f$.
    /// \ingroup transformation
    template <typename L, typename Pred> using filter = join<transform<L, ranges_detail::filter_<Pred>>>;

    namespace lazy {
        /// \sa 'futures::detail::meta::filter'
        /// \ingroup lazy_transformation
        template <typename L, typename Fn> using filter = defer<filter, L, Fn>;
    } // namespace lazy

    ///////////////////////////////////////////////////////////////////////////////////////////
    // static_const
    ///\cond
    namespace ranges_detail {
        template <typename T> struct static_const { static constexpr T value{}; };

        // Avoid potential ODR violations with global objects:
        template <typename T> constexpr T static_const<T>::value;
    } // namespace ranges_detail

    ///\endcond

    ///////////////////////////////////////////////////////////////////////////////////////////
    // for_each
    /// \cond
    namespace ranges_detail {
        struct for_each_fn {
            template <class Fn, class... Args> constexpr auto operator()(list<Args...>, Fn f) const -> Fn {
                return (void)std::initializer_list<int>{((void)f(Args{}), 0)...}, f;
            }
        };
    } // namespace ranges_detail
    /// \endcond

#if META_CXX_INLINE_VARIABLES
    /// `for_each(L, Fn)` calls the \p Fn for each
    /// argument in the \p L.
    /// \ingroup runtime
    inline constexpr ranges_detail::for_each_fn for_each{};
#else
    ///\cond
    namespace {
        /// \endcond

        /// `for_each(List, UnaryFunction)` calls the \p UnaryFunction for each
        /// argument in the \p List.
        /// \ingroup runtime
        constexpr auto &&for_each = ranges_detail::static_const<ranges_detail::for_each_fn>::value;

        /// \cond
    } // namespace
      /// \endcond
#endif

    ///////////////////////////////////////////////////////////////////////////////////////////
    // transpose
    /// Given a list of lists of types \p ListOfLists, transpose the elements from the lists.
    /// \par Complexity
    /// \f$ O(N \times M) \f$, where \f$ N \f$ is the size of the outer list, and
    /// \f$ M \f$ is the size of the inner lists.
    /// \ingroup transformation
    template <META_TYPE_CONSTRAINT(list_like) ListOfLists>
    using transpose =
        fold<ListOfLists, repeat_n<size<front<ListOfLists>>, list<>>, bind_back<quote<transform>, quote<push_back>>>;

    namespace lazy {
        /// \sa 'futures::detail::meta::transpose'
        /// \ingroup lazy_transformation
        template <typename ListOfLists> using transpose = defer<transpose, ListOfLists>;
    } // namespace lazy

    ///////////////////////////////////////////////////////////////////////////////////////////
    // zip_with
    /// Given a list of lists of types \p ListOfLists and an invocable \p Fn, construct a new
    /// list by calling \p Fn with the elements from the lists pairwise.
    /// \par Complexity
    /// \f$ O(N \times M) \f$, where \f$ N \f$ is the size of the outer list, and
    /// \f$ M \f$ is the size of the inner lists.
    /// \ingroup transformation
    template <META_TYPE_CONSTRAINT(invocable) Fn, META_TYPE_CONSTRAINT(list_like) ListOfLists>
    using zip_with = transform<transpose<ListOfLists>, uncurry<Fn>>;

    namespace lazy {
        /// \sa 'futures::detail::meta::zip_with'
        /// \ingroup lazy_transformation
        template <typename Fn, typename ListOfLists> using zip_with = defer<zip_with, Fn, ListOfLists>;
    } // namespace lazy

    ///////////////////////////////////////////////////////////////////////////////////////////
    // zip
    /// Given a list of lists of types \p ListOfLists, construct a new list by grouping the
    /// elements from the lists pairwise into `futures::detail::meta::list`s.
    /// \par Complexity
    /// \f$ O(N \times M) \f$, where \f$ N \f$ is the size of the outer list, and \f$ M \f$
    /// is the size of the inner lists.
    /// \ingroup transformation
    template <META_TYPE_CONSTRAINT(list_like) ListOfLists> using zip = transpose<ListOfLists>;

    namespace lazy {
        /// \sa 'futures::detail::meta::zip'
        /// \ingroup lazy_transformation
        template <typename ListOfLists> using zip = defer<zip, ListOfLists>;
    } // namespace lazy

    ///////////////////////////////////////////////////////////////////////////////////////////
    // as_list
    /// \cond
    namespace ranges_detail {
        template <typename T> using uncvref_t = _t<std::remove_cv<_t<std::remove_reference<T>>>>;

        // Indirection here needed to avoid Core issue 1430
        // https://wg21.link/cwg1430
        template <typename Sequence> struct as_list_ : lazy::invoke<uncurry<quote<list>>, Sequence> {};
    } // namespace ranges_detail
    /// \endcond

    /// Turn a type into an instance of \c futures::detail::meta::list in a way determined by
    /// \c futures::detail::meta::apply.
    /// \ingroup list
    template <typename Sequence> using as_list = _t<ranges_detail::as_list_<ranges_detail::uncvref_t<Sequence>>>;

    namespace lazy {
        /// \sa 'futures::detail::meta::as_list'
        /// \ingroup lazy_list
        template <typename Sequence> using as_list = defer<as_list, Sequence>;
    } // namespace lazy

    ///////////////////////////////////////////////////////////////////////////////////////////
    // reverse
    /// \cond
    namespace ranges_detail {
        template <typename L, typename State = list<>> struct reverse_ : lazy::fold<L, State, quote<push_front>> {};

        template <typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6,
                  typename T7, typename T8, typename T9, typename... Ts, typename... Us>
        struct reverse_<list<T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, Ts...>, list<Us...>>
            : reverse_<list<Ts...>, list<T9, T8, T7, T6, T5, T4, T3, T2, T1, T0, Us...>> {};
    } // namespace ranges_detail
    /// \endcond

    /// Return a new \c futures::detail::meta::list by reversing the elements in the list \p L.
    /// \par Complexity
    /// \f$ O(N) \f$.
    /// \ingroup transformation
    template <META_TYPE_CONSTRAINT(list_like) L> using reverse = _t<ranges_detail::reverse_<L>>;

    namespace lazy {
        /// \sa 'futures::detail::meta::reverse'
        /// \ingroup lazy_transformation
        template <typename L> using reverse = defer<reverse, L>;
    } // namespace lazy

    /// Logically negate the result of invocable \p Fn.
    /// \ingroup trait
    template <META_TYPE_CONSTRAINT(invocable) Fn> using not_fn = compose<quote<not_>, Fn>;

    namespace lazy {
        /// \sa 'futures::detail::meta::not_fn'
        /// \ingroup lazy_trait
        template <typename Fn> using not_fn = defer<not_fn, Fn>;
    } // namespace lazy

    ///////////////////////////////////////////////////////////////////////////////////////////
    // all_of
    /// A Boolean integral constant wrapper around \c true if `invoke<Fn, A>::%value` is \c true
    /// for all elements \c A in \c futures::detail::meta::list \p L; \c false, otherwise.
    /// \par Complexity
    /// \f$ O(N) \f$.
    /// \ingroup query
    template <META_TYPE_CONSTRAINT(list_like) L, META_TYPE_CONSTRAINT(invocable) Fn>
    using all_of = empty<find_if<L, not_fn<Fn>>>;

    namespace lazy {
        /// \sa 'futures::detail::meta::all_of'
        /// \ingroup lazy_query
        template <typename L, typename Fn> using all_of = defer<all_of, L, Fn>;
    } // namespace lazy

    ///////////////////////////////////////////////////////////////////////////////////////////
    // any_of
    /// A Boolean integral constant wrapper around \c true if `invoke<Fn, A>::%value` is
    /// \c true for any element \c A in \c futures::detail::meta::list \p L; \c false, otherwise.
    /// \par Complexity
    /// \f$ O(N) \f$.
    /// \ingroup query
    template <META_TYPE_CONSTRAINT(list_like) L, META_TYPE_CONSTRAINT(invocable) Fn>
    using any_of = not_<empty<find_if<L, Fn>>>;

    namespace lazy {
        /// \sa 'futures::detail::meta::any_of'
        /// \ingroup lazy_query
        template <typename L, typename Fn> using any_of = defer<any_of, L, Fn>;
    } // namespace lazy

    ///////////////////////////////////////////////////////////////////////////////////////////
    // none_of
    /// A Boolean integral constant wrapper around \c true if `invoke<Fn, A>::%value` is
    /// \c false for all elements \c A in \c futures::detail::meta::list \p L; \c false, otherwise.
    /// \par Complexity
    /// \f$ O(N) \f$.
    /// \ingroup query
    template <META_TYPE_CONSTRAINT(list_like) L, META_TYPE_CONSTRAINT(invocable) Fn>
    using none_of = empty<find_if<L, Fn>>;

    namespace lazy {
        /// \sa 'futures::detail::meta::none_of'
        /// \ingroup lazy_query
        template <typename L, META_TYPE_CONSTRAINT(invocable) Fn> using none_of = defer<none_of, L, Fn>;
    } // namespace lazy

    ///////////////////////////////////////////////////////////////////////////////////////////
    // in
    /// A Boolean integral constant wrapper around \c true if there is at least one occurrence
    /// of \p T in \p L.
    /// \par Complexity
    /// \f$ O(N) \f$.
    /// \ingroup query
    template <META_TYPE_CONSTRAINT(list_like) L, typename T> using in = not_<empty<find<L, T>>>;

    namespace lazy {
        /// \sa 'futures::detail::meta::in'
        /// \ingroup lazy_query
        template <typename L, typename T> using in = defer<in, L, T>;
    } // namespace lazy

    ///////////////////////////////////////////////////////////////////////////////////////////
    // inherit
    /// \cond
    namespace ranges_detail {
        template <typename L> struct inherit_ {};

        template <typename... L> struct inherit_<list<L...>> : L... { using type = inherit_; };
    } // namespace ranges_detail
    /// \endcond

    /// A type that inherits from all the types in the list
    /// \pre The types in the list must be unique
    /// \pre All the types in the list must be non-final class types
    /// \ingroup datatype
    template <META_TYPE_CONSTRAINT(list_like) L> using inherit = futures::detail::meta::_t<ranges_detail::inherit_<L>>;

    namespace lazy {
        /// \sa 'futures::detail::meta::inherit'
        /// \ingroup lazy_datatype
        template <typename L> using inherit = defer<inherit, L>;
    } // namespace lazy

    ///////////////////////////////////////////////////////////////////////////////////////////
    // unique
    /// \cond
    namespace ranges_detail {
        template <typename Set, typename T> struct in_ {};

        template <typename... Set, typename T>
        struct in_<list<Set...>, T> : bool_<META_IS_BASE_OF(id<T>, inherit<list<id<Set>...>>)> {};

        template <typename Set, typename T> struct insert_back_ {};

        template <typename... Set, typename T> struct insert_back_<list<Set...>, T> {
            using type = if_<in_<list<Set...>, T>, list<Set...>, list<Set..., T>>;
        };
    } // namespace ranges_detail
    /// \endcond

    /// Return a new \c futures::detail::meta::list where all duplicate elements have been removed.
    /// \par Complexity
    /// \f$ O(N^2) \f$.
    /// \ingroup transformation
    template <META_TYPE_CONSTRAINT(list_like) L> using unique = fold<L, list<>, quote_trait<ranges_detail::insert_back_>>;

    namespace lazy {
        /// \sa 'futures::detail::meta::unique'
        /// \ingroup lazy_transformation
        template <typename L> using unique = defer<unique, L>;
    } // namespace lazy

    ///////////////////////////////////////////////////////////////////////////////////////////
    // partition
    /// \cond
    namespace ranges_detail {
        template <typename Fn> struct partition_ {
#ifdef META_CONCEPT
            template <typename, typename>
#else
            template <typename, typename, typename = void>
#endif
            struct impl {
            };
            template <typename... Yes, typename... No, typename A>
#ifdef META_CONCEPT
            requires integral<invoke<Fn, A>>
            struct impl<pair<list<Yes...>, list<No...>>, A>
#else
            struct impl<pair<list<Yes...>, list<No...>>, A, void_<bool_<invoke<Fn, A>::type::value>>>
#endif
            {
                using type = if_<invoke<Fn, A>, pair<list<Yes..., A>, list<No...>>, pair<list<Yes...>, list<No..., A>>>;
            };

            template <typename State, typename A> using invoke = _t<impl<State, A>>;
        };
    } // namespace ranges_detail
    /// \endcond

    /// Returns a pair of lists, where the elements of \p L that satisfy the
    /// invocable \p Fn such that `invoke<Fn,A>::%value` is \c true are present in the
    /// first list and the rest are in the second.
    /// \par Complexity
    /// \f$ O(N) \f$.
    /// \ingroup transformation
    template <META_TYPE_CONSTRAINT(list_like) L, META_TYPE_CONSTRAINT(invocable) Fn>
    using partition = fold<L, pair<list<>, list<>>, ranges_detail::partition_<Fn>>;

    namespace lazy {
        /// \sa 'futures::detail::meta::partition'
        /// \ingroup lazy_transformation
        template <typename L, typename Fn> using partition = defer<partition, L, Fn>;
    } // namespace lazy

    ///////////////////////////////////////////////////////////////////////////////////////////
    // sort
    /// \cond
    namespace ranges_detail {
        template <META_TYPE_CONSTRAINT(invocable) Fn, typename A, typename B, typename... Ts>
        using part_ = partition<list<B, Ts...>, bind_back<Fn, A>>;
#ifdef META_CONCEPT
        template <list_like L, invocable Fn>
#else
        template <typename, typename, typename = void>
#endif
        struct sort_ {
        };
        template <typename Fn> struct sort_<list<>, Fn> { using type = list<>; };

        template <typename A, typename Fn> struct sort_<list<A>, Fn> { using type = list<A>; };

        template <typename A, typename B, typename... Ts, typename Fn>
#ifdef META_CONCEPT
        requires trait<sort_<first<part_<Fn, A, B, Ts...>>, Fn>> && trait<sort_<second<part_<Fn, A, B, Ts...>>, Fn>>
        struct sort_<list<A, B, Ts...>, Fn>
#else
        struct sort_<list<A, B, Ts...>, Fn, void_<_t<sort_<first<part_<Fn, A, B, Ts...>>, Fn>>>>
#endif
        {
            using P = part_<Fn, A, B, Ts...>;
            using type = concat<_t<sort_<first<P>, Fn>>, list<A>, _t<sort_<second<P>, Fn>>>;
        };
    } // namespace ranges_detail
    /// \endcond

    // clang-format off
    /// Return a new \c futures::detail::meta::list that is sorted according to invocable predicate \p Fn.
    /// \par Complexity
    /// Expected: \f$ O(N log N) \f$
    /// Worst case: \f$ O(N^2) \f$.
    /// \code
    /// using L0 = list<char[5], char[3], char[2], char[6], char[1], char[5], char[10]>;
    /// using L1 = futures::detail::meta::sort<L0, lambda<_a, _b, lazy::less<lazy::sizeof_<_a>, lazy::sizeof_<_b>>>>;
    /// static_assert(std::is_same_v<L1, list<char[1], char[2], char[3], char[5], char[5], char[6], char[10]>>, "");
    /// \endcode
    /// \ingroup transformation
    // clang-format on
    template <META_TYPE_CONSTRAINT(list_like) L, META_TYPE_CONSTRAINT(invocable) Fn>
    using sort = _t<ranges_detail::sort_<L, Fn>>;

    namespace lazy {
        /// \sa 'futures::detail::meta::sort'
        /// \ingroup lazy_transformation
        template <typename L, typename Fn> using sort = defer<sort, L, Fn>;
    } // namespace lazy

    ////////////////////////////////////////////////////////////////////////////
    // lambda_
    /// \cond
    namespace ranges_detail {
        template <typename T, int = 0> struct protect_;

        template <typename, int = 0> struct vararg_;

        template <typename T, int = 0> struct is_valid_;

        // Returns which branch to evaluate
        template <typename If, typename... Ts>
#ifdef META_CONCEPT
        using lazy_if_ = lazy::_t<defer<_if_, If, protect_<Ts>...>>;
#else
        using lazy_if_ = lazy::_t<defer<_if_, list<If, protect_<Ts>...>>>;
#endif

        template <typename A, typename T, typename Fn, typename Ts> struct subst1_ { using type = list<list<T>>; };
        template <typename T, typename Fn, typename Ts> struct subst1_<Fn, T, Fn, Ts> { using type = list<>; };
        template <typename A, typename T, typename Fn, typename Ts> struct subst1_<vararg_<A>, T, Fn, Ts> {
            using type = list<Ts>;
        };

        template <typename As, typename Ts>
        using substitutions_ =
            push_back<join<transform<concat<As, repeat_n_c<size<Ts>{} + 2 - size<As>{}, back<As>>>,
                                     concat<Ts, repeat_n_c<2, back<As>>>,
                                     bind_back<quote_trait<subst1_>, back<As>, drop_c<Ts, size<As>{} - 2>>>>,
                      list<back<As>>>;

#if 0 // def META_CONCEPT
        template <list_like As, list_like Ts>
        requires (_v<size<Ts>> + 2 >= _v<size<As>>)
        using substitutions = substitutions_<As, Ts>;
#else // ^^^ concepts / no concepts vvv
        template <typename As, typename Ts>
        using substitutions =
#ifdef META_WORKAROUND_MSVC_702792
            invoke<if_c<(size<Ts>::value + 2 >= size<As>::value), quote<substitutions_>>, As, Ts>;
#else  // ^^^ workaround ^^^ / vvv no workaround vvv
            invoke<if_c<(size<Ts>{} + 2 >= size<As>{}), quote<substitutions_>>, As, Ts>;
#endif // META_WORKAROUND_MSVC_702792
#endif // META_CONCEPT

        template <typename T> struct is_vararg_ : std::false_type {};
        template <typename T> struct is_vararg_<vararg_<T>> : std::true_type {};

        template <META_TYPE_CONSTRAINT(list_like) Tags>
        using is_variadic_ = is_vararg_<at<push_front<Tags, void>, dec<size<Tags>>>>;

        template <META_TYPE_CONSTRAINT(list_like) Tags, bool IsVariadic = is_variadic_<Tags>::value> struct lambda_;

        // Non-variadic lambda implementation
        template <typename... As> struct lambda_<list<As...>, false> {
          private:
            static constexpr std::size_t arity = sizeof...(As) - 1;
            using Tags = list<As...>; // Includes the lambda body as the last arg!
            using Fn = back<Tags>;
            template <typename T, META_TYPE_CONSTRAINT(list_like) Args> struct impl;
            template <typename T, META_TYPE_CONSTRAINT(list_like) Args>
            using lazy_impl_ = lazy::_t<defer<impl, T, protect_<Args>>>;
#if 0 // def META_CONCEPT
            template <typename, list_like>
#else
            template <typename, typename, typename = void>
#endif
            struct subst_ {};
            template <template <typename...> class C, typename... Ts, typename Args>
#if 0 // def META_CONCEPT
            requires valid<C, _t<impl<Ts, Args>>...> struct subst_<defer<C, Ts...>, Args>
#else
            struct subst_<defer<C, Ts...>, Args, void_<C<_t<impl<Ts, Args>>...>>>
#endif
            {
                using type = C<_t<impl<Ts, Args>>...>;
            };
            template <typename T, template <T...> class C, T... Is, typename Args>
#if 0 // def META_CONCEPT
            requires valid_i<T, C, Is...> struct subst_<defer_i<T, C, Is...>, Args>
#else
            struct subst_<defer_i<T, C, Is...>, Args, void_<C<Is...>>>
#endif
            {
                using type = C<Is...>;
            };
            template <typename T, META_TYPE_CONSTRAINT(list_like) Args>
            struct impl
                : if_c<(reverse_find_index<Tags, T>() != npos()), lazy::at<Args, reverse_find_index<Tags, T>>, id<T>> {
            };
            template <typename T, typename Args> struct impl<protect_<T>, Args> { using type = T; };
            template <typename T, typename Args> struct impl<is_valid_<T>, Args> {
                using type = is_trait<impl<T, Args>>;
            };
            template <typename If, typename... Ts, typename Args>
            struct impl<defer<if_, If, Ts...>, Args> // Short-circuit if_
                : impl<lazy_impl_<lazy_if_<If, Ts...>, Args>, Args> {};
            template <typename B, typename... Bs, typename Args>
            struct impl<defer<and_, B, Bs...>, Args> // Short-circuit and_
                : impl<lazy_impl_<lazy_if_<B, lazy::and_<Bs...>, protect_<std::false_type>>, Args>, Args> {};
            template <typename B, typename... Bs, typename Args>
            struct impl<defer<or_, B, Bs...>, Args> // Short-circuit or_
                : impl<lazy_impl_<lazy_if_<B, protect_<std::true_type>, lazy::or_<Bs...>>, Args>, Args> {};
            template <template <typename...> class C, typename... Ts, typename Args>
            struct impl<defer<C, Ts...>, Args> : subst_<defer<C, Ts...>, Args> {};
            template <typename T, template <T...> class C, T... Is, typename Args>
            struct impl<defer_i<T, C, Is...>, Args> : subst_<defer_i<T, C, Is...>, Args> {};
            template <template <typename...> class C, typename... Ts, typename Args>
            struct impl<C<Ts...>, Args> : subst_<defer<C, Ts...>, Args> {};
            template <typename... Ts, typename Args> struct impl<lambda_<list<Ts...>, false>, Args> {
                using type =
                    compose<uncurry<lambda_<list<As..., Ts...>, false>>, curry<bind_front<quote<concat>, Args>>>;
            };
            template <typename... Bs, typename Args> struct impl<lambda_<list<Bs...>, true>, Args> {
                using type = compose<typename lambda_<list<As..., Bs...>, true>::thunk,
                                     bind_front<quote<concat>, transform<Args, quote<list>>>,
                                     curry<bind_front<quote<substitutions>, list<Bs...>>>>;
            };

          public:
            template <typename... Ts>
#ifdef META_CONCEPT
            requires(sizeof...(Ts) == arity) using invoke = _t<impl<Fn, list<Ts..., Fn>>>;
#else
            using invoke = _t<if_c<sizeof...(Ts) == arity, impl<Fn, list<Ts..., Fn>>>>;
#endif
        };

        // Lambda with variadic placeholder (broken out due to less efficient compile-time
        // resource usage)
        template <typename... As> struct lambda_<list<As...>, true> {
          private:
            template <META_TYPE_CONSTRAINT(list_like) T, bool IsVar> friend struct lambda_;
            using Tags = list<As...>; // Includes the lambda body as the last arg!
            template <typename T, META_TYPE_CONSTRAINT(list_like) Args> struct impl;
            template <META_TYPE_CONSTRAINT(list_like) Args> using eval_impl_ = bind_back<quote_trait<impl>, Args>;
            template <typename T, META_TYPE_CONSTRAINT(list_like) Args>
            using lazy_impl_ = lazy::_t<defer<impl, T, protect_<Args>>>;
            template <template <typename...> class C, META_TYPE_CONSTRAINT(list_like) Args,
                      META_TYPE_CONSTRAINT(list_like) Ts>
            using try_subst_ = apply<quote<C>, join<transform<Ts, eval_impl_<Args>>>>;
#if 0 // def META_CONCEPT
            template <typename, list_like>
#else
            template <typename, typename, typename = void>
#endif
            struct subst_ {};
            template <template <typename...> class C, typename... Ts, typename Args>
#if 0 // def META_CONCEPT
            requires is_true<try_subst_<C, Args, list<Ts...>>> struct subst_<defer<C, Ts...>, Args>
#else
            struct subst_<defer<C, Ts...>, Args, void_<try_subst_<C, Args, list<Ts...>>>>
#endif
            {
                using type = list<try_subst_<C, Args, list<Ts...>>>;
            };
            template <typename T, template <T...> class C, T... Is, typename Args>
#if 0 // def META_CONCEPT
            requires valid_i<T, C, Is...> struct subst_<defer_i<T, C, Is...>, Args>
#else
            struct subst_<defer_i<T, C, Is...>, Args, void_<C<Is...>>>
#endif
            {
                using type = list<C<Is...>>;
            };
            template <typename T, META_TYPE_CONSTRAINT(list_like) Args>
            struct impl : if_c<(reverse_find_index<Tags, T>() != npos()), lazy::at<Args, reverse_find_index<Tags, T>>,
                               id<list<T>>> {};
            template <typename T, typename Args> struct impl<protect_<T>, Args> { using type = list<T>; };
            template <typename T, typename Args> struct impl<is_valid_<T>, Args> {
                using type = list<is_trait<impl<T, Args>>>;
            };
            template <typename If, typename... Ts, typename Args>
            struct impl<defer<if_, If, Ts...>, Args> // Short-circuit if_
                : impl<lazy_impl_<lazy_if_<If, Ts...>, Args>, Args> {};
            template <typename B, typename... Bs, typename Args>
            struct impl<defer<and_, B, Bs...>, Args> // Short-circuit and_
                : impl<lazy_impl_<lazy_if_<B, lazy::and_<Bs...>, protect_<std::false_type>>, Args>, Args> {};
            template <typename B, typename... Bs, typename Args>
            struct impl<defer<or_, B, Bs...>, Args> // Short-circuit or_
                : impl<lazy_impl_<lazy_if_<B, protect_<std::true_type>, lazy::or_<Bs...>>, Args>, Args> {};
            template <template <typename...> class C, typename... Ts, typename Args>
            struct impl<defer<C, Ts...>, Args> : subst_<defer<C, Ts...>, Args> {};
            template <typename T, template <T...> class C, T... Is, typename Args>
            struct impl<defer_i<T, C, Is...>, Args> : subst_<defer_i<T, C, Is...>, Args> {};
            template <template <typename...> class C, typename... Ts, typename Args>
            struct impl<C<Ts...>, Args> : subst_<defer<C, Ts...>, Args> {};
            template <typename... Bs, bool IsVar, typename Args> struct impl<lambda_<list<Bs...>, IsVar>, Args> {
                using type =
                    list<compose<typename lambda_<list<As..., Bs...>, true>::thunk, bind_front<quote<concat>, Args>,
                                 curry<bind_front<quote<substitutions>, list<Bs...>>>>>;
            };
            struct thunk {
                template <typename S, typename R = _t<impl<back<Tags>, S>>>
#ifdef META_CONCEPT
                requires(_v<size<R>> == 1) using invoke = front<R>;
#else
                using invoke = if_c<size<R>{} == 1, front<R>>;
#endif
            };

          public:
            template <typename... Ts> using invoke = invoke<thunk, substitutions<Tags, list<Ts...>>>;
        };
    } // namespace ranges_detail
    /// \endcond

    ///////////////////////////////////////////////////////////////////////////////////////////
    // lambda
    /// For creating anonymous Invocables.
    /// \code
    /// using L = lambda<_a, _b, std::pair<_b, std::pair<_a, _a>>>;
    /// using P = invoke<L, int, short>;
    /// static_assert(std::is_same_v<P, std::pair<short, std::pair<int, int>>>, "");
    /// \endcode
    /// \ingroup trait
    template <typename... Ts>
#ifdef META_CONCEPT
    requires(sizeof...(Ts) > 0) using lambda = ranges_detail::lambda_<list<Ts...>>;
#else
    using lambda = if_c<(sizeof...(Ts) > 0), ranges_detail::lambda_<list<Ts...>>>;
#endif

    ///////////////////////////////////////////////////////////////////////////////////////////
    // is_valid
    /// For testing whether a deferred computation will succeed in a \c let or a \c lambda.
    /// \ingroup trait
    template <typename T> using is_valid = ranges_detail::is_valid_<T>;

    ///////////////////////////////////////////////////////////////////////////////////////////
    // vararg
    /// For defining variadic placeholders.
    template <typename T> using vararg = ranges_detail::vararg_<T>;

    ///////////////////////////////////////////////////////////////////////////////////////////
    // protect
    /// For preventing the evaluation of a nested `defer`ed computation in a \c let or
    /// \c lambda expression.
    template <typename T> using protect = ranges_detail::protect_<T>;

    ///////////////////////////////////////////////////////////////////////////////////////////
    // var
    /// For use when defining local variables in \c futures::detail::meta::let expressions
    /// \sa `futures::detail::meta::let`
    template <typename Tag, typename Value> struct var;

    /// \cond
    namespace ranges_detail {
        template <typename...> struct let_ {};
        template <typename Fn> struct let_<Fn> { using type = lazy::invoke<lambda<Fn>>; };
        template <typename Tag, typename Value, typename... Rest> struct let_<var<Tag, Value>, Rest...> {
            using type = lazy::invoke<lambda<Tag, _t<let_<Rest...>>>, Value>;
        };
    } // namespace ranges_detail
    /// \endcond

    /// A lexically scoped expression with local variables.
    ///
    /// \code
    /// template <typename T, typename L>
    /// using find_index_ = let<
    ///     var<_a, L>,
    ///     var<_b, lazy::find<_a, T>>,
    ///     lazy::if_<
    ///         std::is_same<_b, list<>>,
    ///         futures::detail::meta::npos,
    ///         lazy::minus<lazy::size<_a>, lazy::size<_b>>>>;
    /// static_assert(find_index_<int, list<short, int, float>>{} == 1, "");
    /// static_assert(find_index_<double, list<short, int, float>>{} == futures::detail::meta::npos{}, "");
    /// \endcode
    /// \ingroup trait
    template <typename... As> using let = _t<_t<ranges_detail::let_<As...>>>;

    namespace lazy {
        /// \sa `futures::detail::meta::let`
        /// \ingroup lazy_trait
        template <typename... As> using let = defer<let, As...>;
    } // namespace lazy

    // Some argument placeholders for use in \c lambda expressions.
    /// \ingroup trait
    inline namespace placeholders {
        // regular placeholders:
        struct _a;
        struct _b;
        struct _c;
        struct _d;
        struct _e;
        struct _f;
        struct _g;
        struct _h;
        struct _i;

        // variadic placeholders:
        using _args = vararg<void>;
        using _args_a = vararg<_a>;
        using _args_b = vararg<_b>;
        using _args_c = vararg<_c>;
    } // namespace placeholders

    ///////////////////////////////////////////////////////////////////////////////////////////
    // cartesian_product
    /// \cond
    namespace ranges_detail {
        template <typename M2, typename M> struct cartesian_product_fn {
            template <typename X> struct lambda0 {
                template <typename Xs> using lambda1 = list<push_front<Xs, X>>;
                using type = join<transform<M2, quote<lambda1>>>;
            };
            using type = join<transform<M, quote_trait<lambda0>>>;
        };
    } // namespace ranges_detail
    /// \endcond

    /// Given a list of lists \p ListOfLists, return a new list of lists that is the Cartesian
    /// Product. Like the `sequence` function from the Haskell Prelude.
    /// \par Complexity
    /// \f$ O(N \times M) \f$, where \f$ N \f$ is the size of the outer list, and
    /// \f$ M \f$ is the size of the inner lists.
    /// \ingroup transformation
    template <META_TYPE_CONSTRAINT(list_like) ListOfLists>
    using cartesian_product = reverse_fold<ListOfLists, list<list<>>, quote_trait<ranges_detail::cartesian_product_fn>>;

    namespace lazy {
        /// \sa 'futures::detail::meta::cartesian_product'
        /// \ingroup lazy_transformation
        template <typename ListOfLists> using cartesian_product = defer<cartesian_product, ListOfLists>;
    } // namespace lazy

    /// \cond
    ///////////////////////////////////////////////////////////////////////////////////////////
    // add_const_if
    namespace ranges_detail {
        template <bool> struct add_const_if { template <typename T> using invoke = T const; };
        template <> struct add_const_if<false> { template <typename T> using invoke = T; };
    } // namespace ranges_detail
    template <bool If> using add_const_if_c = ranges_detail::add_const_if<If>;
    template <META_TYPE_CONSTRAINT(integral) If> using add_const_if = add_const_if_c<If::type::value>;
    /// \endcond

    /// \cond
    ///////////////////////////////////////////////////////////////////////////////////////////
    // const_if
    template <bool If, typename T> using const_if_c = typename add_const_if_c<If>::template invoke<T>;
    template <typename If, typename T> using const_if = typename add_const_if<If>::template invoke<T>;
    /// \endcond

    /// \cond
    namespace ranges_detail {
        template <typename State, typename Ch>
        using atoi_ = if_c<(Ch::value >= '0' && Ch::value <= '9'),
                           std::integral_constant<typename State::value_type, State::value * 10 + (Ch::value - '0')>>;
    }
    /// \endcond

    inline namespace literals {
        /// A user-defined literal that generates objects of type \c futures::detail::meta::size_t.
        /// \ingroup integral
        template <char... Chs>
        constexpr fold<list<char_<Chs>...>, futures::detail::meta::size_t<0>, quote<ranges_detail::atoi_>> operator"" _z() {
            return {};
        }
    } // namespace literals
} // namespace futures::detail::meta

/// \cond
// Non-portable forward declarations of standard containers
#ifndef META_NO_STD_FORWARD_DECLARATIONS
#if defined(__apple_build_version__) || (defined(__clang__) && __clang_major__ < 6)
META_BEGIN_NAMESPACE_STD
META_BEGIN_NAMESPACE_VERSION
template <class> class META_TEMPLATE_VIS allocator;
template <class, class> struct META_TEMPLATE_VIS pair;
template <class> struct META_TEMPLATE_VIS hash;
template <class> struct META_TEMPLATE_VIS less;
template <class> struct META_TEMPLATE_VIS equal_to;
template <class> struct META_TEMPLATE_VIS char_traits;
#if defined(_GLIBCXX_USE_CXX11_ABI) && _GLIBCXX_USE_CXX11_ABI
inline namespace __cxx11 {
#endif
    template <class, class, class> class META_TEMPLATE_VIS basic_string;
#if defined(_GLIBCXX_USE_CXX11_ABI) && _GLIBCXX_USE_CXX11_ABI
}
#endif
META_END_NAMESPACE_VERSION
META_BEGIN_NAMESPACE_CONTAINER
#if defined(__GLIBCXX__)
inline namespace __cxx11 {
#endif
    template <class, class> class META_TEMPLATE_VIS list;
#if defined(__GLIBCXX__)
}
#endif
template <class, class> class META_TEMPLATE_VIS forward_list;
template <class, class> class META_TEMPLATE_VIS vector;
template <class, class> class META_TEMPLATE_VIS deque;
template <class, class, class, class> class META_TEMPLATE_VIS map;
template <class, class, class, class> class META_TEMPLATE_VIS multimap;
template <class, class, class> class META_TEMPLATE_VIS set;
template <class, class, class> class META_TEMPLATE_VIS multiset;
template <class, class, class, class, class> class META_TEMPLATE_VIS unordered_map;
template <class, class, class, class, class> class META_TEMPLATE_VIS unordered_multimap;
template <class, class, class, class> class META_TEMPLATE_VIS unordered_set;
template <class, class, class, class> class META_TEMPLATE_VIS unordered_multiset;
template <class, class> class META_TEMPLATE_VIS queue;
template <class, class, class> class META_TEMPLATE_VIS priority_queue;
template <class, class> class META_TEMPLATE_VIS stack;
META_END_NAMESPACE_CONTAINER
META_END_NAMESPACE_STD

namespace futures::detail::meta {
    namespace ranges_detail {
        template <typename T, typename A = std::allocator<T>> using std_list = std::list<T, A>;
        template <typename T, typename A = std::allocator<T>> using std_forward_list = std::forward_list<T, A>;
        template <typename T, typename A = std::allocator<T>> using std_vector = std::vector<T, A>;
        template <typename T, typename A = std::allocator<T>> using std_deque = std::deque<T, A>;
        template <typename T, typename C = std::char_traits<T>, typename A = std::allocator<T>>
        using std_basic_string = std::basic_string<T, C, A>;
        template <typename K, typename V, typename C = std::less<K>, typename A = std::allocator<std::pair<K const, V>>>
        using std_map = std::map<K, V, C, A>;
        template <typename K, typename V, typename C = std::less<K>, typename A = std::allocator<std::pair<K const, V>>>
        using std_multimap = std::multimap<K, V, C, A>;
        template <typename K, typename C = std::less<K>, typename A = std::allocator<K>>
        using std_set = std::set<K, C, A>;
        template <typename K, typename C = std::less<K>, typename A = std::allocator<K>>
        using std_multiset = std::multiset<K, C, A>;
        template <typename K, typename V, typename H = std::hash<K>, typename C = std::equal_to<K>,
                  typename A = std::allocator<std::pair<K const, V>>>
        using std_unordered_map = std::unordered_map<K, V, H, C, A>;
        template <typename K, typename V, typename H = std::hash<K>, typename C = std::equal_to<K>,
                  typename A = std::allocator<std::pair<K const, V>>>
        using std_unordered_multimap = std::unordered_multimap<K, V, H, C, A>;
        template <typename K, typename H = std::hash<K>, typename C = std::equal_to<K>, typename A = std::allocator<K>>
        using std_unordered_set = std::unordered_set<K, H, C, A>;
        template <typename K, typename H = std::hash<K>, typename C = std::equal_to<K>, typename A = std::allocator<K>>
        using std_unordered_multiset = std::unordered_multiset<K, H, C, A>;
        template <typename T, typename C = std_deque<T>> using std_queue = std::queue<T, C>;
        template <typename T, typename C = std_vector<T>, class D = std::less<typename C::value_type>>
        using std_priority_queue = std::priority_queue<T, C, D>;
        template <typename T, typename C = std_deque<T>> using std_stack = std::stack<T, C>;
    } // namespace ranges_detail

    template <> struct quote<::std::list> : quote<ranges_detail::std_list> {};
    template <> struct quote<::std::deque> : quote<ranges_detail::std_deque> {};
    template <> struct quote<::std::forward_list> : quote<ranges_detail::std_forward_list> {};
    template <> struct quote<::std::vector> : quote<ranges_detail::std_vector> {};
    template <> struct quote<::std::basic_string> : quote<ranges_detail::std_basic_string> {};
    template <> struct quote<::std::map> : quote<ranges_detail::std_map> {};
    template <> struct quote<::std::multimap> : quote<ranges_detail::std_multimap> {};
    template <> struct quote<::std::set> : quote<ranges_detail::std_set> {};
    template <> struct quote<::std::multiset> : quote<ranges_detail::std_multiset> {};
    template <> struct quote<::std::unordered_map> : quote<ranges_detail::std_unordered_map> {};
    template <> struct quote<::std::unordered_multimap> : quote<ranges_detail::std_unordered_multimap> {};
    template <> struct quote<::std::unordered_set> : quote<ranges_detail::std_unordered_set> {};
    template <> struct quote<::std::unordered_multiset> : quote<ranges_detail::std_unordered_multiset> {};
    template <> struct quote<::std::queue> : quote<ranges_detail::std_queue> {};
    template <> struct quote<::std::priority_queue> : quote<ranges_detail::std_priority_queue> {};
    template <> struct quote<::std::stack> : quote<ranges_detail::std_stack> {};
} // namespace futures::detail::meta

#endif
#endif
/// \endcond

#ifdef __clang__
#pragma GCC diagnostic pop
#endif
#endif


// #include <futures/algorithm/detail/traits/range/concepts/concepts.h>
/// \file
//  CPP, the Concepts PreProcessor library
//
//  Copyright Eric Niebler 2018-present
//  Copyright (c) 2018-present, Facebook, Inc.
//  Copyright (c) 2020-present, Google LLC.
//
//  Use, modification and distribution is subject to the
//  Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
// This source code is licensed under the MIT license found in the
// LICENSE file in the root directory of this source tree.
//
// Project home: https://github.com/ericniebler/range-v3
//

#ifndef CPP_CONCEPTS_HPP
#define CPP_CONCEPTS_HPP

// clang-format off

// #include <initializer_list>

// #include <utility>

// #include <type_traits>

// #include <futures/algorithm/detail/traits/range/concepts/swap.h>
/// \file
// Concepts library
//
//  Copyright Eric Niebler 2013-present
//
//  Use, modification and distribution is subject to the
//  Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
// Project home: https://github.com/ericniebler/range-v3

#ifndef CPP_SWAP_HPP
#define CPP_SWAP_HPP

// #include <futures/algorithm/detail/traits/range/meta/meta.h>

#include <tuple>
// #include <type_traits>

// #include <utility>


// Note: constexpr implies inline, to retain the same visibility
// C++14 constexpr functions are inline in C++11
#if (defined(__cpp_constexpr) && __cpp_constexpr >= 201304L) || (!defined(__cpp_constexpr) && __cplusplus >= 201402L)
#define CPP_CXX14_CONSTEXPR constexpr
#else
#define CPP_CXX14_CONSTEXPR inline
#endif

#ifndef CPP_CXX_INLINE_VARIABLES
#ifdef __cpp_inline_variables
#define CPP_CXX_INLINE_VARIABLES __cpp_inline_variables
#elif defined(__clang__) && (__clang_major__ > 3 || __clang_major__ == 3 && __clang_minor__ == 9) &&                   \
    __cplusplus > 201402L
#define CPP_CXX_INLINE_VARIABLES 201606L
#else
#define CPP_CXX_INLINE_VARIABLES __cplusplus
#endif // __cpp_inline_variables
#endif // CPP_CXX_INLINE_VARIABLES

#if defined(_MSC_VER) && !defined(__clang__)
#if _MSC_VER < 1926
#define CPP_WORKAROUND_MSVC_895622 // Error when phase 1 name binding finds only deleted function
#endif                             // _MSC_VER < 1926
#endif                             // MSVC

#if CPP_CXX_INLINE_VARIABLES < 201606L
#define CPP_INLINE_VAR
#define CPP_INLINE_VARIABLE(type, name)                                                                                \
    inline namespace {                                                                                                 \
        constexpr auto &name = ::futures::detail::concepts::ranges_detail::static_const<type>::value;                                          \
    }                                                                                                                  \
    /**/
#else // CPP_CXX_INLINE_VARIABLES >= 201606L
#define CPP_INLINE_VAR inline
#define CPP_INLINE_VARIABLE(type, name)                                                                                \
    inline constexpr type name{};                                                                                      \
    /**/
#endif // CPP_CXX_INLINE_VARIABLES

#if CPP_CXX_INLINE_VARIABLES < 201606L
#define CPP_DEFINE_CPO(type, name)                                                                                     \
    inline namespace {                                                                                                 \
        constexpr auto &name = ::futures::detail::concepts::ranges_detail::static_const<type>::value;                                          \
    }                                                                                                                  \
    /**/
#else // CPP_CXX_INLINE_VARIABLES >= 201606L
#define CPP_DEFINE_CPO(type, name)                                                                                     \
    inline namespace _ {                                                                                               \
        inline constexpr type name{};                                                                                  \
    }                                                                                                                  \
    /**/
#endif // CPP_CXX_INLINE_VARIABLES

#if defined(_MSC_VER) && !defined(__clang__)
#define CPP_DIAGNOSTIC_PUSH __pragma(warning(push))
#define CPP_DIAGNOSTIC_POP __pragma(warning(pop))
#define CPP_DIAGNOSTIC_IGNORE_INIT_LIST_LIFETIME
#define CPP_DIAGNOSTIC_IGNORE_FLOAT_EQUAL
#define CPP_DIAGNOSTIC_IGNORE_CPP2A_COMPAT
#else // ^^^ defined(_MSC_VER) ^^^ / vvv !defined(_MSC_VER) vvv
#if defined(__GNUC__) || defined(__clang__)
#define CPP_PRAGMA(X) _Pragma(#X)
#define CPP_DIAGNOSTIC_PUSH CPP_PRAGMA(GCC diagnostic push)
#define CPP_DIAGNOSTIC_POP CPP_PRAGMA(GCC diagnostic pop)
#define CPP_DIAGNOSTIC_IGNORE_PRAGMAS CPP_PRAGMA(GCC diagnostic ignored "-Wpragmas")
#define CPP_DIAGNOSTIC_IGNORE(X)                                                                                       \
    CPP_DIAGNOSTIC_IGNORE_PRAGMAS                                                                                      \
    CPP_PRAGMA(GCC diagnostic ignored "-Wunknown-pragmas")                                                             \
    CPP_PRAGMA(GCC diagnostic ignored X)
#define CPP_DIAGNOSTIC_IGNORE_INIT_LIST_LIFETIME                                                                       \
    CPP_DIAGNOSTIC_IGNORE("-Wunknown-warning-option")                                                                  \
    CPP_DIAGNOSTIC_IGNORE("-Winit-list-lifetime")
#define CPP_DIAGNOSTIC_IGNORE_FLOAT_EQUAL CPP_DIAGNOSTIC_IGNORE("-Wfloat-equal")
#define CPP_DIAGNOSTIC_IGNORE_CPP2A_COMPAT CPP_DIAGNOSTIC_IGNORE("-Wc++2a-compat")
#else
#define CPP_DIAGNOSTIC_PUSH
#define CPP_DIAGNOSTIC_POP
#define CPP_DIAGNOSTIC_IGNORE_INIT_LIST_LIFETIME
#define CPP_DIAGNOSTIC_IGNORE_FLOAT_EQUAL
#define CPP_DIAGNOSTIC_IGNORE_CPP2A_COMPAT
#endif
#endif // MSVC/Generic configuration switch

namespace futures::detail::concepts {
    /// \cond
    namespace ranges_detail {
        template <typename T>
        CPP_INLINE_VAR constexpr bool is_movable_v =
            std::is_object<T>::value &&std::is_move_constructible<T>::value &&std::is_move_assignable<T>::value;

        template <typename T> struct static_const { static constexpr T const value{}; };
        template <typename T> constexpr T const static_const<T>::value;
    } // namespace ranges_detail
    /// \endcond

    template <typename T> struct is_swappable;

    template <typename T> struct is_nothrow_swappable;

    template <typename T, typename U> struct is_swappable_with;

    template <typename T, typename U> struct is_nothrow_swappable_with;

    template <typename T, typename U = T>
    CPP_CXX14_CONSTEXPR futures::detail::meta::if_c<std::is_move_constructible<T>::value && std::is_assignable<T &, U>::value, T>
    exchange(T &t,
             U &&u) noexcept(std::is_nothrow_move_constructible<T>::value &&std::is_nothrow_assignable<T &, U>::value) {
        T tmp((T &&) t);
        t = (U &&) u;
        CPP_DIAGNOSTIC_IGNORE_INIT_LIST_LIFETIME
        return tmp;
    }

    /// \cond
    namespace adl_swap_detail {
        struct nope {};

        // Intentionally create an ambiguity with std::swap, which is
        // (possibly) unconstrained.
        template <typename T> nope swap(T &, T &) = delete;

        template <typename T, std::size_t N> nope swap(T (&)[N], T (&)[N]) = delete;

#ifdef CPP_WORKAROUND_MSVC_895622
        nope swap();
#endif

        template <typename T, typename U> decltype(swap(std::declval<T>(), std::declval<U>())) try_adl_swap_(int);

        template <typename T, typename U> nope try_adl_swap_(long);

        template <typename T, typename U = T>
        CPP_INLINE_VAR constexpr bool is_adl_swappable_v =
            !META_IS_SAME(decltype(adl_swap_detail::try_adl_swap_<T, U>(42)), nope);

        struct swap_fn {
            // Dispatch to user-defined swap found via ADL:
            template <typename T, typename U>
            CPP_CXX14_CONSTEXPR futures::detail::meta::if_c<is_adl_swappable_v<T, U>> operator()(T &&t, U &&u) const
                noexcept(noexcept(swap((T &&) t, (U &&) u))) {
                swap((T &&) t, (U &&) u);
            }

            // For intrinsically swappable (i.e., movable) types for which
            // a swap overload cannot be found via ADL, swap by moving.
            template <typename T>
            CPP_CXX14_CONSTEXPR futures::detail::meta::if_c<!is_adl_swappable_v<T &> && ranges_detail::is_movable_v<T>> operator()(T &a,
                                                                                                           T &b) const
                noexcept(noexcept(b = concepts::exchange(a, (T &&) b))) {
                b = concepts::exchange(a, (T &&) b);
            }

            // For arrays of intrinsically swappable (i.e., movable) types
            // for which a swap overload cannot be found via ADL, swap array
            // elements by moving.
            template <typename T, typename U, std::size_t N>
            CPP_CXX14_CONSTEXPR
                futures::detail::meta::if_c<!is_adl_swappable_v<T (&)[N], U (&)[N]> && is_swappable_with<T &, U &>::value>
                operator()(T (&t)[N], U (&u)[N]) const noexcept(is_nothrow_swappable_with<T &, U &>::value) {
                for (std::size_t i = 0; i < N; ++i)
                    (*this)(t[i], u[i]);
            }

            // For rvalue pairs and tuples of swappable types, swap the
            // members. This permits code like:
            //   futures::detail::swap(std::tie(a,b,c), std::tie(d,e,f));
            template <typename F0, typename S0, typename F1, typename S1>
            CPP_CXX14_CONSTEXPR futures::detail::meta::if_c<is_swappable_with<F0, F1>::value && is_swappable_with<S0, S1>::value>
            operator()(std::pair<F0, S0> &&left, std::pair<F1, S1> &&right) const
                noexcept(is_nothrow_swappable_with<F0, F1>::value &&is_nothrow_swappable_with<S0, S1>::value) {
                swap_fn()(static_cast<std::pair<F0, S0> &&>(left).first,
                          static_cast<std::pair<F1, S1> &&>(right).first);
                swap_fn()(static_cast<std::pair<F0, S0> &&>(left).second,
                          static_cast<std::pair<F1, S1> &&>(right).second);
            }

            template <typename... Ts, typename... Us>
            CPP_CXX14_CONSTEXPR futures::detail::meta::if_c<futures::detail::meta::and_c<is_swappable_with<Ts, Us>::value...>::value>
            operator()(std::tuple<Ts...> &&left, std::tuple<Us...> &&right) const
                noexcept(futures::detail::meta::and_c<is_nothrow_swappable_with<Ts, Us>::value...>::value) {
                swap_fn::impl(static_cast<std::tuple<Ts...> &&>(left), static_cast<std::tuple<Us...> &&>(right),
                              futures::detail::meta::make_index_sequence<sizeof...(Ts)>{});
            }

          private:
            template <typename... Ts> static constexpr int ignore_unused(Ts &&...) { return 0; }
            template <typename T, typename U, std::size_t... Is>
            CPP_CXX14_CONSTEXPR static void impl(T &&left, U &&right, futures::detail::meta::index_sequence<Is...>) {
                (void)swap_fn::ignore_unused(
                    (swap_fn()(std::get<Is>(static_cast<T &&>(left)), std::get<Is>(static_cast<U &&>(right))), 42)...);
            }
        };

        template <typename T, typename U, typename = void> struct is_swappable_with_ : std::false_type {};

        template <typename T, typename U>
        struct is_swappable_with_<T, U,
                                  futures::detail::meta::void_<decltype(swap_fn()(std::declval<T>(), std::declval<U>())),
                                              decltype(swap_fn()(std::declval<U>(), std::declval<T>()))>>
            : std::true_type {};

        template <typename T, typename U>
        struct is_nothrow_swappable_with_
            : futures::detail::meta::bool_<noexcept(swap_fn()(std::declval<T>(), std::declval<U>())) &&noexcept(
                  swap_fn()(std::declval<U>(), std::declval<T>()))> {};

        // Q: Should std::reference_wrapper be considered a proxy wrt swapping rvalues?
        // A: No. Its operator= is currently defined to reseat the references, so
        //    std::swap(ra, rb) already means something when ra and rb are (lvalue)
        //    reference_wrappers. That reseats the reference wrappers but leaves the
        //    referents unmodified. Treating rvalue reference_wrappers differently would
        //    be confusing.

        // Q: Then why is it OK to "re"-define swap for pairs and tuples of references?
        // A: Because as defined above, swapping an rvalue tuple of references has the same
        //    semantics as swapping an lvalue tuple of references. Rather than reseat the
        //    references, assignment happens *through* the references.

        // Q: But I have an iterator whose operator* returns an rvalue
        //    std::reference_wrapper<T>. How do I make it model indirectly_swappable?
        // A: With an overload of iter_swap.
    } // namespace adl_swap_detail
    /// \endcond

    /// \ingroup group-utility
    template <typename T, typename U> struct is_swappable_with : adl_swap_detail::is_swappable_with_<T, U> {};

    /// \ingroup group-utility
    template <typename T, typename U>
    struct is_nothrow_swappable_with
        : futures::detail::meta::and_<is_swappable_with<T, U>, adl_swap_detail::is_nothrow_swappable_with_<T, U>> {};

    /// \ingroup group-utility
    template <typename T> struct is_swappable : is_swappable_with<T &, T &> {};

    /// \ingroup group-utility
    template <typename T> struct is_nothrow_swappable : is_nothrow_swappable_with<T &, T &> {};

    /// \ingroup group-utility
    /// \relates adl_swap_detail::swap_fn
    CPP_DEFINE_CPO(adl_swap_detail::swap_fn, swap)
} // namespace futures::detail::concepts

#endif

// #include <futures/algorithm/detail/traits/range/concepts/type_traits.h>
/// \file
// Concepts library
//
//  Copyright Eric Niebler 2013-present
//
//  Use, modification and distribution is subject to the
//  Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
// Project home: https://github.com/ericniebler/range-v3

#ifndef CPP_TYPE_TRAITS_HPP
#define CPP_TYPE_TRAITS_HPP

// #include <futures/algorithm/detail/traits/range/meta/meta.h>

// #include <tuple>

// #include <type_traits>

// #include <utility>


namespace futures::detail::concepts {
    template <typename T> using remove_cvref_t = typename std::remove_cv<typename std::remove_reference<T>::type>::type;

    /// \cond
    namespace ranges_detail {
        template <typename From, typename To>
        using is_convertible = std::is_convertible<futures::detail::meta::_t<std::add_rvalue_reference<From>>, To>;

        template <bool> struct if_else_ { template <typename, typename U> using invoke = U; };
        template <> struct if_else_<true> { template <typename T, typename> using invoke = T; };
        template <bool B, typename T, typename U> using if_else_t = futures::detail::meta::invoke<if_else_<B>, T, U>;

        template <bool> struct if_ {};
        template <> struct if_<true> { template <typename T> using invoke = T; };
        template <bool B, typename T = void> using if_t = futures::detail::meta::invoke<if_<B>, T>;

        template <typename From, typename To> struct _copy_cv_ { using type = To; };
        template <typename From, typename To> struct _copy_cv_<From const, To> { using type = To const; };
        template <typename From, typename To> struct _copy_cv_<From volatile, To> { using type = To volatile; };
        template <typename From, typename To> struct _copy_cv_<From const volatile, To> {
            using type = To const volatile;
        };
        template <typename From, typename To> using _copy_cv = futures::detail::meta::_t<_copy_cv_<From, To>>;

        ////////////////////////////////////////////////////////////////////////////////////////
        template <typename T, typename U, typename = void> struct _builtin_common;

        template <typename T, typename U> using _builtin_common_t = futures::detail::meta::_t<_builtin_common<T, U>>;

        template <typename T, typename U> using _cond_res = decltype(true ? std::declval<T>() : std::declval<U>());

        template <typename T, typename U, typename R = _builtin_common_t<T &, U &>>
        using _rref_res = if_else_t<std::is_reference<R>::value, futures::detail::meta::_t<std::remove_reference<R>> &&, R>;

        template <typename T, typename U> using _lref_res = _cond_res<_copy_cv<T, U> &, _copy_cv<U, T> &>;

        template <typename T> struct as_cref_ { using type = T const &; };
        template <typename T> struct as_cref_<T &> { using type = T const &; };
        template <typename T> struct as_cref_<T &&> { using type = T const &; };
        template <> struct as_cref_<void> { using type = void; };
        template <> struct as_cref_<void const> { using type = void const; };

        template <typename T> using as_cref_t = typename as_cref_<T>::type;

        template <typename T> using decay_t = typename std::decay<T>::type;

#if !defined(__GNUC__) || defined(__clang__)
        template <typename T, typename U, typename = void> struct _builtin_common_3 {};
        template <typename T, typename U>
        struct _builtin_common_3<T, U, futures::detail::meta::void_<_cond_res<as_cref_t<T>, as_cref_t<U>>>>
            : std::decay<_cond_res<as_cref_t<T>, as_cref_t<U>>> {};
        template <typename T, typename U, typename = void> struct _builtin_common_2 : _builtin_common_3<T, U> {};
        template <typename T, typename U>
        struct _builtin_common_2<T, U, futures::detail::meta::void_<_cond_res<T, U>>> : std::decay<_cond_res<T, U>> {};
        template <typename T, typename U, typename /* = void */> struct _builtin_common : _builtin_common_2<T, U> {};
        template <typename T, typename U>
        struct _builtin_common<
            T &&, U &&,
            if_t<is_convertible<T &&, _rref_res<T, U>>::value && is_convertible<U &&, _rref_res<T, U>>::value>> {
            using type = _rref_res<T, U>;
        };
        template <typename T, typename U> struct _builtin_common<T &, U &> : futures::detail::meta::defer<_lref_res, T, U> {};
        template <typename T, typename U>
        struct _builtin_common<T &, U &&, if_t<is_convertible<U &&, _builtin_common_t<T &, U const &>>::value>>
            : _builtin_common<T &, U const &> {};
        template <typename T, typename U> struct _builtin_common<T &&, U &> : _builtin_common<U &, T &&> {};
#else
        template <typename T, typename U, typename = void> struct _builtin_common_3 {};
        template <typename T, typename U>
        struct _builtin_common_3<T, U, futures::detail::meta::void_<_cond_res<as_cref_t<T>, as_cref_t<U>>>>
            : std::decay<_cond_res<as_cref_t<T>, as_cref_t<U>>> {};
        template <typename T, typename U, typename = void> struct _builtin_common_2 : _builtin_common_3<T, U> {};
        template <typename T, typename U>
        struct _builtin_common_2<T, U, futures::detail::meta::void_<_cond_res<T, U>>> : std::decay<_cond_res<T, U>> {};
        template <typename T, typename U, typename /* = void */> struct _builtin_common : _builtin_common_2<T, U> {};
        template <typename T, typename U, typename = void> struct _builtin_common_rr : _builtin_common_2<T &&, U &&> {};
        template <typename T, typename U>
        struct _builtin_common_rr<
            T, U, if_t<is_convertible<T &&, _rref_res<T, U>>::value && is_convertible<U &&, _rref_res<T, U>>::value>> {
            using type = _rref_res<T, U>;
        };
        template <typename T, typename U> struct _builtin_common<T &&, U &&> : _builtin_common_rr<T, U> {};
        template <typename T, typename U> struct _builtin_common<T &, U &> : futures::detail::meta::defer<_lref_res, T, U> {};
        template <typename T, typename U, typename = void> struct _builtin_common_lr : _builtin_common_2<T &, T &&> {};
        template <typename T, typename U>
        struct _builtin_common_lr<T, U, if_t<is_convertible<U &&, _builtin_common_t<T &, U const &>>::value>>
            : _builtin_common<T &, U const &> {};
        template <typename T, typename U> struct _builtin_common<T &, U &&> : _builtin_common_lr<T, U> {};
        template <typename T, typename U> struct _builtin_common<T &&, U &> : _builtin_common<U &, T &&> {};
#endif
    } // namespace ranges_detail
    /// \endcond

    /// \addtogroup group-utility Utility
    /// @{
    ///

    /// Users should specialize this to hook the \c common_with concept
    /// until \c std gets a SFINAE-friendly \c std::common_type and there's
    /// some sane way to deal with cv and ref qualifiers.
    template <typename... Ts> struct common_type {};

    template <typename T> struct common_type<T> : std::decay<T> {};

    template <typename T, typename U>
    struct common_type<T, U>
        : ranges_detail::if_else_t<(META_IS_SAME(ranges_detail::decay_t<T>, T) && META_IS_SAME(ranges_detail::decay_t<U>, U)),
                            futures::detail::meta::defer<ranges_detail::_builtin_common_t, T, U>,
                            common_type<ranges_detail::decay_t<T>, ranges_detail::decay_t<U>>> {};

    template <typename... Ts> using common_type_t = typename common_type<Ts...>::type;

    template <typename T, typename U, typename... Vs>
    struct common_type<T, U, Vs...> : futures::detail::meta::lazy::fold<futures::detail::meta::list<U, Vs...>, T, futures::detail::meta::quote<common_type_t>> {};

    /// @}

    /// \addtogroup group-utility Utility
    /// @{
    ///

    /// Users can specialize this to hook the \c common_reference_with concept.
    /// \sa `common_reference`
    template <typename T, typename U, template <typename> class TQual, template <typename> class UQual>
    struct basic_common_reference {};

    /// \cond
    namespace ranges_detail {
        using _rref = futures::detail::meta::quote_trait<std::add_rvalue_reference>;
        using _lref = futures::detail::meta::quote_trait<std::add_lvalue_reference>;

        template <typename> struct _xref { template <typename T> using invoke = T; };
        template <typename T> struct _xref<T &&> {
            template <typename U> using invoke = futures::detail::meta::_t<std::add_rvalue_reference<futures::detail::meta::invoke<_xref<T>, U>>>;
        };
        template <typename T> struct _xref<T &> {
            template <typename U> using invoke = futures::detail::meta::_t<std::add_lvalue_reference<futures::detail::meta::invoke<_xref<T>, U>>>;
        };
        template <typename T> struct _xref<T const> { template <typename U> using invoke = U const; };
        template <typename T> struct _xref<T volatile> { template <typename U> using invoke = U volatile; };
        template <typename T> struct _xref<T const volatile> { template <typename U> using invoke = U const volatile; };

        template <typename T, typename U>
        using _basic_common_reference = basic_common_reference<remove_cvref_t<T>, remove_cvref_t<U>,
                                                               _xref<T>::template invoke, _xref<U>::template invoke>;

        template <typename T, typename U, typename = void>
        struct _common_reference2 : if_else_t<futures::detail::meta::is_trait<_basic_common_reference<T, U>>::value,
                                              _basic_common_reference<T, U>, common_type<T, U>> {};

        template <typename T, typename U>
        struct _common_reference2<T, U, if_t<std::is_reference<_builtin_common_t<T, U>>::value>>
            : _builtin_common<T, U> {};
    } // namespace ranges_detail
    /// \endcond

    /// Users can specialize this to hook the \c common_reference_with concept.
    /// \sa `basic_common_reference`
    template <typename... Ts> struct common_reference {};

    template <typename T> struct common_reference<T> { using type = T; };

    template <typename T, typename U> struct common_reference<T, U> : ranges_detail::_common_reference2<T, U> {};

    template <typename... Ts> using common_reference_t = typename common_reference<Ts...>::type;

    template <typename T, typename U, typename... Vs>
    struct common_reference<T, U, Vs...> : futures::detail::meta::lazy::fold<futures::detail::meta::list<U, Vs...>, T, futures::detail::meta::quote<common_reference_t>> {
    };
    /// @}
} // namespace futures::detail::concepts

#endif


// Disable buggy clang compatibility warning about "requires" and "concept" being
// C++20 keywords.
// https://bugs.llvm.org/show_bug.cgi?id=43708
#if defined(__clang__) && __cplusplus <= 201703L
#define CPP_PP_IGNORE_CXX2A_COMPAT_BEGIN                                                \
    CPP_DIAGNOSTIC_PUSH                                                                 \
    CPP_DIAGNOSTIC_IGNORE_CPP2A_COMPAT

#define CPP_PP_IGNORE_CXX2A_COMPAT_END                                                  \
    CPP_DIAGNOSTIC_POP

#else
#define CPP_PP_IGNORE_CXX2A_COMPAT_BEGIN
#define CPP_PP_IGNORE_CXX2A_COMPAT_END
#endif

#if defined(_MSC_VER) && !defined(__clang__)
#define CPP_WORKAROUND_MSVC_779763 // FATAL_UNREACHABLE calling constexpr function via template parameter
#define CPP_WORKAROUND_MSVC_784772 // Failure to invoke *explicit* bool conversion in a constant expression
#endif

#if !defined(CPP_CXX_CONCEPTS)
#ifdef CPP_DOXYGEN_INVOKED
#define CPP_CXX_CONCEPTS 201800L
#elif defined(__cpp_concepts) && __cpp_concepts > 0
// gcc-6 concepts are too buggy to use
#if !defined(__GNUC__) || defined(__clang__) || __GNUC__ >= 7
#define CPP_CXX_CONCEPTS __cpp_concepts
#else
#define CPP_CXX_CONCEPTS 0L
#endif
#else
#define CPP_CXX_CONCEPTS 0L
#endif
#endif

#define CPP_PP_CAT_(X, ...)  X ## __VA_ARGS__
#define CPP_PP_CAT(X, ...)   CPP_PP_CAT_(X, __VA_ARGS__)

#define CPP_PP_EVAL_(X, ARGS) X ARGS
#define CPP_PP_EVAL(X, ...) CPP_PP_EVAL_(X, (__VA_ARGS__))

#define CPP_PP_EVAL2_(X, ARGS) X ARGS
#define CPP_PP_EVAL2(X, ...) CPP_PP_EVAL2_(X, (__VA_ARGS__))

#define CPP_PP_EXPAND(...) __VA_ARGS__
#define CPP_PP_EAT(...)

#define CPP_PP_FIRST(LIST) CPP_PP_FIRST_ LIST
#define CPP_PP_FIRST_(...) __VA_ARGS__ CPP_PP_EAT

#define CPP_PP_SECOND(LIST) CPP_PP_SECOND_ LIST
#define CPP_PP_SECOND_(...) CPP_PP_EXPAND

#define CPP_PP_CHECK(...) CPP_PP_EXPAND(CPP_PP_CHECK_N(__VA_ARGS__, 0,))
#define CPP_PP_CHECK_N(x, n, ...) n
#define CPP_PP_PROBE(x) x, 1,
#define CPP_PP_PROBE_N(x, n) x, n,

#define CPP_PP_IS_PAREN(x) CPP_PP_CHECK(CPP_PP_IS_PAREN_PROBE x)
#define CPP_PP_IS_PAREN_PROBE(...) CPP_PP_PROBE(~)

// CPP_CXX_VA_OPT
#ifndef CPP_CXX_VA_OPT
#if __cplusplus > 201703L
#define CPP_CXX_VA_OPT_(...) CPP_PP_CHECK(__VA_OPT__(,) 1)
#define CPP_CXX_VA_OPT CPP_CXX_VA_OPT_(~)
#else
#define CPP_CXX_VA_OPT 0
#endif
#endif // CPP_CXX_VA_OPT

// The final CPP_PP_EXPAND here is to avoid
// https://stackoverflow.com/questions/5134523/msvc-doesnt-expand-va-args-correctly
#define CPP_PP_COUNT(...)                                                       \
    CPP_PP_EXPAND(CPP_PP_COUNT_(__VA_ARGS__,                                    \
        50, 49, 48, 47, 46, 45, 44, 43, 42, 41,                                 \
        40, 39, 38, 37, 36, 35, 34, 33, 32, 31,                                 \
        30, 29, 28, 27, 26, 25, 24, 23, 22, 21,                                 \
        20, 19, 18, 17, 16, 15, 14, 13, 12, 11,                                 \
        10, 9, 8, 7, 6, 5, 4, 3, 2, 1,))

#define CPP_PP_COUNT_(                                                          \
    _01, _02, _03, _04, _05, _06, _07, _08, _09, _10,                           \
    _11, _12, _13, _14, _15, _16, _17, _18, _19, _20,                           \
    _21, _22, _23, _24, _25, _26, _27, _28, _29, _30,                           \
    _31, _32, _33, _34, _35, _36, _37, _38, _39, _40,                           \
    _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, N, ...)                   \
    N

#define CPP_PP_IIF(BIT) CPP_PP_CAT_(CPP_PP_IIF_, BIT)
#define CPP_PP_IIF_0(TRUE, ...) __VA_ARGS__
#define CPP_PP_IIF_1(TRUE, ...) TRUE

#define CPP_PP_LPAREN (
#define CPP_PP_RPAREN )

#define CPP_PP_NOT(BIT) CPP_PP_CAT_(CPP_PP_NOT_, BIT)
#define CPP_PP_NOT_0 1
#define CPP_PP_NOT_1 0

#define CPP_PP_EMPTY()
#define CPP_PP_COMMA() ,
#define CPP_PP_LBRACE() {
#define CPP_PP_RBRACE() }
#define CPP_PP_COMMA_IIF(X)                                                     \
    CPP_PP_IIF(X)(CPP_PP_EMPTY, CPP_PP_COMMA)()

#define CPP_PP_FOR_EACH(M, ...)                                                 \
    CPP_PP_FOR_EACH_N(CPP_PP_COUNT(__VA_ARGS__), M, __VA_ARGS__)
#define CPP_PP_FOR_EACH_N(N, M, ...)                                            \
    CPP_PP_CAT(CPP_PP_FOR_EACH_, N)(M, __VA_ARGS__)
#define CPP_PP_FOR_EACH_1(M, _1)                                                \
    M(_1)
#define CPP_PP_FOR_EACH_2(M, _1, _2)                                            \
    M(_1), M(_2)
#define CPP_PP_FOR_EACH_3(M, _1, _2, _3)                                        \
    M(_1), M(_2), M(_3)
#define CPP_PP_FOR_EACH_4(M, _1, _2, _3, _4)                                    \
    M(_1), M(_2), M(_3), M(_4)
#define CPP_PP_FOR_EACH_5(M, _1, _2, _3, _4, _5)                                \
    M(_1), M(_2), M(_3), M(_4), M(_5)
#define CPP_PP_FOR_EACH_6(M, _1, _2, _3, _4, _5, _6)                            \
    M(_1), M(_2), M(_3), M(_4), M(_5), M(_6)
#define CPP_PP_FOR_EACH_7(M, _1, _2, _3, _4, _5, _6, _7)                        \
    M(_1), M(_2), M(_3), M(_4), M(_5), M(_6), M(_7)
#define CPP_PP_FOR_EACH_8(M, _1, _2, _3, _4, _5, _6, _7, _8)                    \
    M(_1), M(_2), M(_3), M(_4), M(_5), M(_6), M(_7), M(_8)

#define CPP_PP_PROBE_EMPTY_PROBE_CPP_PP_PROBE_EMPTY                             \
    CPP_PP_PROBE(~)

#define CPP_PP_PROBE_EMPTY()
#define CPP_PP_IS_NOT_EMPTY(...)                                                \
    CPP_PP_EVAL(                                                                \
        CPP_PP_CHECK,                                                           \
        CPP_PP_CAT(                                                             \
            CPP_PP_PROBE_EMPTY_PROBE_,                                          \
            CPP_PP_PROBE_EMPTY __VA_ARGS__ ()))

#if defined(_MSC_VER) && !defined(__clang__) && (__cplusplus <= 201703L)
#define CPP_BOOL(...) ::futures::detail::meta::bool_<__VA_ARGS__>::value
#define CPP_TRUE_FN                                                             \
    !::futures::detail::concepts::ranges_detail::instance_<                                             \
        decltype(CPP_true_fn(::futures::detail::concepts::ranges_detail::xNil{}))>

#define CPP_NOT(...) (!CPP_BOOL(__VA_ARGS__))
#else
#define CPP_BOOL(...) __VA_ARGS__
#define CPP_TRUE_FN CPP_true_fn(::futures::detail::concepts::ranges_detail::xNil{})
#define CPP_NOT(...) (!(__VA_ARGS__))
#endif

#define CPP_assert(...)                                                         \
    static_assert(static_cast<bool>(__VA_ARGS__),                               \
        "Concept assertion failed : " #__VA_ARGS__)

#define CPP_assert_msg static_assert

#if CPP_CXX_CONCEPTS || defined(CPP_DOXYGEN_INVOKED)
#define CPP_concept META_CONCEPT
#define CPP_and &&

#else
#define CPP_concept CPP_INLINE_VAR constexpr bool
#define CPP_and CPP_and_sfinae

#endif

////////////////////////////////////////////////////////////////////////////////
// CPP_template
// Usage:
//   CPP_template(typename A, typename B)
//     (requires Concept1<A> CPP_and Concept2<B>)
//   void foo(A a, B b)
//   {}
#if CPP_CXX_CONCEPTS
#define CPP_template(...) template<__VA_ARGS__ CPP_TEMPLATE_AUX_
#define CPP_template_def CPP_template
#define CPP_member
#define CPP_ctor(TYPE) TYPE CPP_CTOR_IMPL_1_

#if defined(CPP_DOXYGEN_INVOKED) && CPP_DOXYGEN_INVOKED
/// INTERNAL ONLY
#define CPP_CTOR_IMPL_1_(...) (__VA_ARGS__) CPP_CTOR_IMPL_2_
#define CPP_CTOR_IMPL_2_(...) __VA_ARGS__ `
#else
/// INTERNAL ONLY
#define CPP_CTOR_IMPL_1_(...) (__VA_ARGS__) CPP_PP_EXPAND
#endif

/// INTERNAL ONLY
#define CPP_TEMPLATE_AUX_(...)                                                  \
    > CPP_PP_CAT(                                                               \
        CPP_TEMPLATE_AUX_,                                                      \
        CPP_TEMPLATE_AUX_WHICH_(__VA_ARGS__,))(__VA_ARGS__)

/// INTERNAL ONLY
#define CPP_TEMPLATE_AUX_WHICH_(FIRST, ...)                                     \
    CPP_PP_EVAL(                                                                \
        CPP_PP_CHECK,                                                           \
        CPP_PP_CAT(CPP_TEMPLATE_PROBE_CONCEPT_, FIRST))

/// INTERNAL ONLY
#define CPP_TEMPLATE_PROBE_CONCEPT_concept                                      \
    CPP_PP_PROBE(~)

#if defined(CPP_DOXYGEN_INVOKED) && CPP_DOXYGEN_INVOKED
// A template with a requires clause. Turn the requires clause into
// a Doxygen precondition block.
/// INTERNAL ONLY
#define CPP_TEMPLATE_AUX_0(...) __VA_ARGS__`
#define requires requires `

#else
// A template with a requires clause
/// INTERNAL ONLY
#define CPP_TEMPLATE_AUX_0(...) __VA_ARGS__
#endif

// A concept definition
/// INTERNAL ONLY
#define CPP_TEMPLATE_AUX_1(DECL, ...)                                           \
    CPP_concept CPP_CONCEPT_NAME_(DECL) = __VA_ARGS__

#define CPP_concept_ref(NAME, ...)                                              \
    CPP_PP_CAT(NAME, _concept_)<__VA_ARGS__>

#else // ^^^^ with concepts / without concepts vvvv

#define CPP_template CPP_template_sfinae
#define CPP_template_def CPP_template_def_sfinae
#define CPP_member CPP_member_sfinae
#define CPP_ctor CPP_ctor_sfinae
#define CPP_concept_ref(NAME, ...)                                              \
    (1u == sizeof(CPP_PP_CAT(NAME, _concept_)(                                  \
        (::futures::detail::concepts::ranges_detail::tag<__VA_ARGS__>*)nullptr)))

/// INTERNAL ONLY
#define CPP_TEMPLATE_AUX_ CPP_TEMPLATE_SFINAE_AUX_

#endif

#define CPP_template_sfinae(...)                                                \
    CPP_PP_IGNORE_CXX2A_COMPAT_BEGIN                                            \
    template<__VA_ARGS__ CPP_TEMPLATE_SFINAE_AUX_

/// INTERNAL ONLY
#define CPP_TEMPLATE_SFINAE_PROBE_CONCEPT_concept                               \
    CPP_PP_PROBE(~)

/// INTERNAL ONLY
#define CPP_TEMPLATE_SFINAE_AUX_WHICH_(FIRST, ...)                              \
    CPP_PP_EVAL(                                                                \
        CPP_PP_CHECK,                                                           \
        CPP_PP_CAT(CPP_TEMPLATE_SFINAE_PROBE_CONCEPT_, FIRST))

/// INTERNAL ONLY
#define CPP_TEMPLATE_SFINAE_AUX_(...)                                           \
    CPP_PP_CAT(                                                                 \
        CPP_TEMPLATE_SFINAE_AUX_,                                               \
        CPP_TEMPLATE_SFINAE_AUX_WHICH_(__VA_ARGS__,))(__VA_ARGS__)

// A template with a requires clause
/// INTERNAL ONLY
#define CPP_TEMPLATE_SFINAE_AUX_0(...) ,                                        \
    bool CPP_true = true,                                                       \
    std::enable_if_t<                                                           \
        CPP_PP_CAT(CPP_TEMPLATE_SFINAE_AUX_3_, __VA_ARGS__) &&                  \
        CPP_BOOL(CPP_true),                                                     \
        int> = 0>                                                               \
    CPP_PP_IGNORE_CXX2A_COMPAT_END

// A concept definition
/// INTERNAL ONLY
#define CPP_TEMPLATE_SFINAE_AUX_1(DECL, ...) ,                                  \
        bool CPP_true = true,                                                   \
        std::enable_if_t<__VA_ARGS__ && CPP_BOOL(CPP_true), int> = 0>           \
    auto CPP_CONCEPT_NAME_(DECL)(                                               \
        ::futures::detail::concepts::ranges_detail::tag<CPP_CONCEPT_PARAMS_(DECL)>*)                    \
        -> char(&)[1];                                                          \
    auto CPP_CONCEPT_NAME_(DECL)(...) -> char(&)[2]                             \
    CPP_PP_IGNORE_CXX2A_COMPAT_END

/// INTERNAL ONLY
#define CPP_CONCEPT_NAME_(DECL)                                                 \
    CPP_PP_EVAL(                                                                \
        CPP_PP_CAT,                                                             \
        CPP_PP_EVAL(CPP_PP_FIRST, CPP_EAT_CONCEPT_(DECL)), _concept_)

/// INTERNAL ONLY
#define CPP_CONCEPT_PARAMS_(DECL)                                               \
    CPP_PP_EVAL(CPP_PP_SECOND, CPP_EAT_CONCEPT_(DECL))

/// INTERNAL ONLY
#define CPP_EAT_CONCEPT_(DECL)                                                  \
    CPP_PP_CAT(CPP_EAT_CONCEPT_, DECL)

/// INTERNAL ONLY
#define CPP_EAT_CONCEPT_concept

#define CPP_and_sfinae                                                          \
    && CPP_BOOL(CPP_true), int> = 0, std::enable_if_t<

#define CPP_template_def_sfinae(...)                                            \
    template<__VA_ARGS__ CPP_TEMPLATE_DEF_SFINAE_AUX_

/// INTERNAL ONLY
#define CPP_TEMPLATE_DEF_SFINAE_AUX_(...) ,                                     \
    bool CPP_true,                                                              \
    std::enable_if_t<                                                           \
        CPP_PP_CAT(CPP_TEMPLATE_SFINAE_AUX_3_, __VA_ARGS__) &&                  \
        CPP_BOOL(CPP_true),                                                     \
        int>>

#define CPP_and_sfinae_def                                                      \
    && CPP_BOOL(CPP_true), int>, std::enable_if_t<

/// INTERNAL ONLY
#define CPP_TEMPLATE_SFINAE_AUX_3_requires

#define CPP_member_sfinae                                                       \
    CPP_broken_friend_member

#define CPP_ctor_sfinae(TYPE)                                                   \
    CPP_PP_IGNORE_CXX2A_COMPAT_BEGIN                                            \
    TYPE CPP_CTOR_SFINAE_IMPL_1_

/// INTERNAL ONLY
#define CPP_CTOR_SFINAE_IMPL_1_(...)                                            \
    (__VA_ARGS__                                                                \
        CPP_PP_COMMA_IIF(                                                       \
            CPP_PP_NOT(CPP_PP_IS_NOT_EMPTY(__VA_ARGS__)))                       \
    CPP_CTOR_SFINAE_REQUIRES

/// INTERNAL ONLY
#define CPP_CTOR_SFINAE_PROBE_NOEXCEPT_noexcept                                 \
    CPP_PP_PROBE(~)

/// INTERNAL ONLY
#define CPP_CTOR_SFINAE_MAKE_PROBE(FIRST,...)                                   \
    CPP_PP_CAT(CPP_CTOR_SFINAE_PROBE_NOEXCEPT_, FIRST)

/// INTERNAL ONLY
#define CPP_CTOR_SFINAE_REQUIRES(...)                                           \
    CPP_PP_CAT(                                                                 \
        CPP_CTOR_SFINAE_REQUIRES_,                                              \
        CPP_PP_EVAL(                                                            \
            CPP_PP_CHECK,                                                       \
            CPP_CTOR_SFINAE_MAKE_PROBE(__VA_ARGS__,)))(__VA_ARGS__)

// No noexcept-clause:
/// INTERNAL ONLY
#define CPP_CTOR_SFINAE_REQUIRES_0(...)                                         \
    std::enable_if_t<                                                           \
        CPP_PP_CAT(CPP_TEMPLATE_SFINAE_AUX_3_, __VA_ARGS__) && CPP_TRUE_FN,     \
        ::futures::detail::concepts::ranges_detail::Nil                                                 \
    > = {})                                                                     \
    CPP_PP_IGNORE_CXX2A_COMPAT_END

// Yes noexcept-clause:
/// INTERNAL ONLY
#define CPP_CTOR_SFINAE_REQUIRES_1(...)                                         \
    std::enable_if_t<                                                           \
        CPP_PP_EVAL(CPP_PP_CAT,                                                 \
            CPP_TEMPLATE_SFINAE_AUX_3_,                                         \
            CPP_PP_CAT(CPP_CTOR_SFINAE_EAT_NOEXCEPT_, __VA_ARGS__)) && CPP_TRUE_FN,\
        ::futures::detail::concepts::ranges_detail::Nil                                                 \
    > = {})                                                                     \
    CPP_PP_EXPAND(CPP_PP_CAT(CPP_CTOR_SFINAE_SHOW_NOEXCEPT_, __VA_ARGS__)))

/// INTERNAL ONLY
#define CPP_CTOR_SFINAE_EAT_NOEXCEPT_noexcept(...)

/// INTERNAL ONLY
#define CPP_CTOR_SFINAE_SHOW_NOEXCEPT_noexcept(...)                             \
    noexcept(__VA_ARGS__)                                                       \
    CPP_PP_IGNORE_CXX2A_COMPAT_END                                              \
    CPP_PP_EAT CPP_PP_LPAREN

#ifdef CPP_DOXYGEN_INVOKED
#define CPP_broken_friend_ret(...)                                              \
    __VA_ARGS__ CPP_PP_EXPAND

#else // ^^^ CPP_DOXYGEN_INVOKED / not CPP_DOXYGEN_INVOKED vvv
#define CPP_broken_friend_ret(...)                                              \
    ::futures::detail::concepts::return_t<                                                       \
        __VA_ARGS__,                                                            \
        std::enable_if_t<CPP_BROKEN_FRIEND_RETURN_TYPE_AUX_

/// INTERNAL ONLY
#define CPP_BROKEN_FRIEND_RETURN_TYPE_AUX_(...)                                 \
    CPP_BROKEN_FRIEND_RETURN_TYPE_AUX_3_(CPP_PP_CAT(                            \
        CPP_TEMPLATE_AUX_2_, __VA_ARGS__))

/// INTERNAL ONLY
#define CPP_TEMPLATE_AUX_2_requires

/// INTERNAL ONLY
#define CPP_BROKEN_FRIEND_RETURN_TYPE_AUX_3_(...)                               \
    __VA_ARGS__ && CPP_TRUE_FN>>

#ifdef CPP_WORKAROUND_MSVC_779763
#define CPP_broken_friend_member                                                \
    template<::futures::detail::concepts::ranges_detail::CPP_true_t const &CPP_true_fn =                \
        ::futures::detail::concepts::ranges_detail::CPP_true_fn_>

#else // ^^^ workaround / no workaround vvv
#define CPP_broken_friend_member                                                \
    template<bool (&CPP_true_fn)(::futures::detail::concepts::ranges_detail::xNil) =                    \
        ::futures::detail::concepts::ranges_detail::CPP_true_fn>

#endif // CPP_WORKAROUND_MSVC_779763
#endif

#if CPP_CXX_CONCEPTS
#define CPP_requires(NAME, REQS)                                                \
CPP_concept CPP_PP_CAT(NAME, _requires_) =                                      \
    CPP_PP_CAT(CPP_REQUIRES_, REQS)

#define CPP_requires_ref(NAME, ...)                                             \
    CPP_PP_CAT(NAME, _requires_)<__VA_ARGS__>

/// INTERNAL ONLY
#define CPP_REQUIRES_requires(...)                                              \
    requires(__VA_ARGS__) CPP_REQUIRES_AUX_

/// INTERNAL ONLY
#define CPP_REQUIRES_AUX_(...)                                                  \
    { __VA_ARGS__; }

#else
#define CPP_requires(NAME, REQS)                                                \
    auto CPP_PP_CAT(NAME, _requires_test_)                                      \
    CPP_REQUIRES_AUX_(NAME, CPP_REQUIRES_ ## REQS)

#define CPP_requires_ref(NAME, ...)                                             \
    (1u == sizeof(CPP_PP_CAT(NAME, _requires_)(                                 \
        (::futures::detail::concepts::ranges_detail::tag<__VA_ARGS__>*)nullptr)))

/// INTERNAL ONLY
#define CPP_REQUIRES_requires(...)                                              \
    (__VA_ARGS__) -> decltype CPP_REQUIRES_RETURN_

/// INTERNAL ONLY
#define CPP_REQUIRES_RETURN_(...) (__VA_ARGS__, void()) {}

/// INTERNAL ONLY
#define CPP_REQUIRES_AUX_(NAME, ...)                                            \
    __VA_ARGS__                                                                 \
    template<typename... As>                                                    \
    auto CPP_PP_CAT(NAME, _requires_)(                                          \
        ::futures::detail::concepts::ranges_detail::tag<As...> *,                                       \
        decltype(&CPP_PP_CAT(NAME, _requires_test_)<As...>) = nullptr)          \
        -> char(&)[1];                                                          \
    auto CPP_PP_CAT(NAME, _requires_)(...) -> char(&)[2]

#endif

#if CPP_CXX_CONCEPTS

#if defined(CPP_DOXYGEN_INVOKED) && CPP_DOXYGEN_INVOKED
#define CPP_ret(...)                                                            \
    __VA_ARGS__ CPP_RET_AUX_
#define CPP_RET_AUX_(...) __VA_ARGS__ `
#else
#define CPP_ret(...)                                                            \
    __VA_ARGS__ CPP_PP_EXPAND
#endif

#else
#define CPP_ret                                                                 \
    CPP_broken_friend_ret

#endif

////////////////////////////////////////////////////////////////////////////////
// CPP_fun
#if CPP_CXX_CONCEPTS

#if defined(CPP_DOXYGEN_INVOKED) && CPP_DOXYGEN_INVOKED
/// INTERNAL ONLY
#define CPP_FUN_IMPL_1_(...)                                                    \
    (__VA_ARGS__)                                                               \
    CPP_FUN_IMPL_2_
#define CPP_FUN_IMPL_2_(...)                                                    \
    __VA_ARGS__ `
#else
/// INTERNAL ONLY
#define CPP_FUN_IMPL_1_(...)                                                    \
    (__VA_ARGS__)                                                               \
    CPP_PP_EXPAND
#endif

#define CPP_fun(X) X CPP_FUN_IMPL_1_
#else
/// INTERNAL ONLY
#define CPP_FUN_IMPL_1_(...)                                                    \
    (__VA_ARGS__                                                                \
        CPP_PP_COMMA_IIF(                                                       \
            CPP_PP_NOT(CPP_PP_IS_NOT_EMPTY(__VA_ARGS__)))                       \
    CPP_FUN_IMPL_REQUIRES

/// INTERNAL ONLY
#define CPP_FUN_IMPL_REQUIRES(...)                                              \
    CPP_PP_EVAL2_(                                                              \
        CPP_FUN_IMPL_SELECT_CONST_,                                             \
        (__VA_ARGS__,)                                                          \
    )(__VA_ARGS__)

/// INTERNAL ONLY
#define CPP_FUN_IMPL_SELECT_CONST_(MAYBE_CONST, ...)                            \
    CPP_PP_CAT(CPP_FUN_IMPL_SELECT_CONST_,                                      \
        CPP_PP_EVAL(CPP_PP_CHECK, CPP_PP_CAT(                                   \
            CPP_PP_PROBE_CONST_PROBE_, MAYBE_CONST)))

/// INTERNAL ONLY
#define CPP_PP_PROBE_CONST_PROBE_const CPP_PP_PROBE(~)

/// INTERNAL ONLY
#define CPP_FUN_IMPL_SELECT_CONST_1(...)                                        \
    CPP_PP_EVAL(                                                                \
        CPP_FUN_IMPL_SELECT_CONST_NOEXCEPT_,                                    \
        CPP_PP_CAT(CPP_FUN_IMPL_EAT_CONST_, __VA_ARGS__),)(                     \
        CPP_PP_CAT(CPP_FUN_IMPL_EAT_CONST_, __VA_ARGS__))

/// INTERNAL ONLY
#define CPP_FUN_IMPL_SELECT_CONST_NOEXCEPT_(MAYBE_NOEXCEPT, ...)                \
    CPP_PP_CAT(CPP_FUN_IMPL_SELECT_CONST_NOEXCEPT_,                             \
        CPP_PP_EVAL2(CPP_PP_CHECK, CPP_PP_CAT(                                  \
            CPP_PP_PROBE_NOEXCEPT_PROBE_, MAYBE_NOEXCEPT)))

/// INTERNAL ONLY
#define CPP_PP_PROBE_NOEXCEPT_PROBE_noexcept CPP_PP_PROBE(~)

/// INTERNAL ONLY
#define CPP_FUN_IMPL_SELECT_CONST_NOEXCEPT_0(...)                               \
    std::enable_if_t<                                                           \
        CPP_PP_EVAL(                                                            \
            CPP_PP_CAT,                                                         \
            CPP_FUN_IMPL_EAT_REQUIRES_,                                         \
            __VA_ARGS__) && CPP_TRUE_FN,                                           \
        ::futures::detail::concepts::ranges_detail::Nil                                                 \
    > = {}) const                                                               \
    CPP_PP_IGNORE_CXX2A_COMPAT_END

/// INTERNAL ONLY
#define CPP_FUN_IMPL_SELECT_CONST_NOEXCEPT_1(...)                               \
    std::enable_if_t<                                                           \
        CPP_PP_EVAL(                                                            \
            CPP_PP_CAT,                                                         \
            CPP_FUN_IMPL_EAT_REQUIRES_,                                         \
            CPP_PP_CAT(CPP_FUN_IMPL_EAT_NOEXCEPT_, __VA_ARGS__)) && CPP_TRUE_FN,   \
        ::futures::detail::concepts::ranges_detail::Nil                                                 \
    > = {}) const                                                               \
    CPP_PP_EXPAND(CPP_PP_CAT(CPP_FUN_IMPL_SHOW_NOEXCEPT_, __VA_ARGS__)))

/// INTERNAL ONLY
#define CPP_FUN_IMPL_EAT_NOEXCEPT_noexcept(...)

/// INTERNAL ONLY
#define CPP_FUN_IMPL_SHOW_NOEXCEPT_noexcept(...)                                \
    noexcept(__VA_ARGS__) CPP_PP_IGNORE_CXX2A_COMPAT_END                        \
    CPP_PP_EAT CPP_PP_LPAREN

/// INTERNAL ONLY
#define CPP_FUN_IMPL_SELECT_CONST_0(...)                                        \
    CPP_PP_EVAL_(                                                               \
        CPP_FUN_IMPL_SELECT_NONCONST_NOEXCEPT_,                                 \
        (__VA_ARGS__,)                                                          \
    )(__VA_ARGS__)

/// INTERNAL ONLY
#define CPP_FUN_IMPL_SELECT_NONCONST_NOEXCEPT_(MAYBE_NOEXCEPT, ...)             \
    CPP_PP_CAT(CPP_FUN_IMPL_SELECT_NONCONST_NOEXCEPT_,                          \
          CPP_PP_EVAL2(CPP_PP_CHECK, CPP_PP_CAT(                                \
            CPP_PP_PROBE_NOEXCEPT_PROBE_, MAYBE_NOEXCEPT)))

/// INTERNAL ONLY
#define CPP_FUN_IMPL_SELECT_NONCONST_NOEXCEPT_0(...)                            \
    std::enable_if_t<                                                           \
        CPP_PP_CAT(CPP_FUN_IMPL_EAT_REQUIRES_, __VA_ARGS__) && CPP_TRUE_FN,        \
        ::futures::detail::concepts::ranges_detail::Nil                                                 \
    > = {})                                                                     \
    CPP_PP_IGNORE_CXX2A_COMPAT_END

/// INTERNAL ONLY
#define CPP_FUN_IMPL_SELECT_NONCONST_NOEXCEPT_1(...)                            \
    std::enable_if_t<                                                           \
        CPP_PP_EVAL(                                                            \
            CPP_PP_CAT,                                                         \
            CPP_FUN_IMPL_EAT_REQUIRES_,                                         \
            CPP_PP_CAT(CPP_FUN_IMPL_EAT_NOEXCEPT_, __VA_ARGS__)                 \
        ) && CPP_TRUE_FN,                                                          \
        ::futures::detail::concepts::ranges_detail::Nil                                                 \
    > = {})                                                                     \
    CPP_PP_EXPAND(CPP_PP_CAT(CPP_FUN_IMPL_SHOW_NOEXCEPT_, __VA_ARGS__)))

/// INTERNAL ONLY
#define CPP_FUN_IMPL_EAT_CONST_const

/// INTERNAL ONLY
#define CPP_FUN_IMPL_EAT_REQUIRES_requires

////////////////////////////////////////////////////////////////////////////////
// CPP_fun
// Usage:
//   template <typename A, typename B>
//   void CPP_fun(foo)(A a, B b)([const]opt [noexcept(true)]opt
//       requires Concept1<A> && Concept2<B>)
//   {}
//
// Note: This macro cannot be used when the last function argument is a
//       parameter pack.
#define CPP_fun(X) CPP_PP_IGNORE_CXX2A_COMPAT_BEGIN X CPP_FUN_IMPL_1_
#endif

////////////////////////////////////////////////////////////////////////////////
// CPP_auto_fun
// Usage:
//   template <typename A, typename B>
//   auto CPP_auto_fun(foo)(A a, B b)([const]opt [noexcept(cond)]opt)opt
//   (
//       return a + b
//   )
#define CPP_auto_fun(X) X CPP_AUTO_FUN_IMPL_

/// INTERNAL ONLY
#define CPP_AUTO_FUN_IMPL_(...) (__VA_ARGS__) CPP_AUTO_FUN_RETURNS_

/// INTERNAL ONLY
#define CPP_AUTO_FUN_RETURNS_(...)                                              \
    CPP_PP_EVAL2_(                                                              \
        CPP_AUTO_FUN_SELECT_RETURNS_,                                           \
        (__VA_ARGS__,)                                                          \
    )(__VA_ARGS__)

/// INTERNAL ONLY
#define CPP_AUTO_FUN_SELECT_RETURNS_(MAYBE_CONST, ...)                          \
    CPP_PP_CAT(CPP_AUTO_FUN_RETURNS_CONST_,                                     \
        CPP_PP_EVAL(CPP_PP_CHECK, CPP_PP_CAT(                                   \
            CPP_PP_PROBE_CONST_MUTABLE_PROBE_, MAYBE_CONST)))

/// INTERNAL ONLY
#define CPP_PP_PROBE_CONST_MUTABLE_PROBE_const CPP_PP_PROBE_N(~, 1)

/// INTERNAL ONLY
#define CPP_PP_PROBE_CONST_MUTABLE_PROBE_mutable CPP_PP_PROBE_N(~, 2)

/// INTERNAL ONLY
#define CPP_PP_EAT_MUTABLE_mutable

/// INTERNAL ONLY
#define CPP_AUTO_FUN_RETURNS_CONST_2(...)                                       \
    CPP_PP_CAT(CPP_PP_EAT_MUTABLE_, __VA_ARGS__) CPP_AUTO_FUN_RETURNS_CONST_0

/// INTERNAL ONLY
#define CPP_AUTO_FUN_RETURNS_CONST_1(...)                                       \
    __VA_ARGS__ CPP_AUTO_FUN_RETURNS_CONST_0

/// INTERNAL ONLY
#define CPP_AUTO_FUN_RETURNS_CONST_0(...)                                       \
    CPP_PP_EVAL(CPP_AUTO_FUN_DECLTYPE_NOEXCEPT_,                                \
        CPP_PP_CAT(CPP_AUTO_FUN_RETURNS_, __VA_ARGS__))

/// INTERNAL ONLY
#define CPP_AUTO_FUN_RETURNS_return

#ifdef __cpp_guaranteed_copy_elision
/// INTERNAL ONLY
#define CPP_AUTO_FUN_DECLTYPE_NOEXCEPT_(...)                                    \
    noexcept(noexcept(__VA_ARGS__)) -> decltype(__VA_ARGS__)                    \
    { return (__VA_ARGS__); }

#else
/// INTERNAL ONLY
#define CPP_AUTO_FUN_DECLTYPE_NOEXCEPT_(...)                                    \
    noexcept(noexcept(decltype(__VA_ARGS__)(__VA_ARGS__))) ->                   \
    decltype(__VA_ARGS__)                                                       \
    { return (__VA_ARGS__); }

#endif

namespace futures::detail::concepts
{
    template<bool B>
    using bool_ = std::integral_constant<bool, B>;

#if defined(__cpp_fold_expressions) && __cpp_fold_expressions >= 201603
    template<bool...Bs>
    CPP_INLINE_VAR constexpr bool and_v = (Bs &&...);

    template<bool...Bs>
    CPP_INLINE_VAR constexpr bool or_v = (Bs ||...);
#else
    namespace ranges_detail
    {
        template<bool...>
        struct bools;
    } // namespace ranges_detail

    template<bool...Bs>
    CPP_INLINE_VAR constexpr bool and_v =
        META_IS_SAME(ranges_detail::bools<Bs..., true>, ranges_detail::bools<true, Bs...>);

    template<bool...Bs>
    CPP_INLINE_VAR constexpr bool or_v =
        !META_IS_SAME(ranges_detail::bools<Bs..., false>, ranges_detail::bools<false, Bs...>);
#endif

    template<typename>
    struct return_t_
    {
        template<typename T>
        using invoke = T;
    };

    template<typename T, typename EnableIf>
    using return_t = futures::detail::meta::invoke<return_t_<EnableIf>, T>;

    namespace ranges_detail
    {
        struct ignore
        {
            template<class... Args>
            constexpr ignore(Args&&...) noexcept {}
        };

        template<class>
        constexpr bool true_()
        {
            return true;
        }

        template<typename...>
        struct tag;

        template<typename T>
        CPP_INLINE_VAR constexpr T instance_ = T{};

        template<typename>
        constexpr bool requires_()
        {
            return true;
        }

        struct Nil
        {};

#ifdef CPP_WORKAROUND_MSVC_779763
        enum class xNil {};

        struct CPP_true_t
        {
            constexpr bool operator()(Nil) const noexcept
            {
                return true;
            }
            constexpr bool operator()(xNil) const noexcept
            {
                return true;
            }
        };

        CPP_INLINE_VAR constexpr CPP_true_t CPP_true_fn_ {};

        constexpr bool CPP_true_fn(xNil)
        {
            return true;
        }
#else
        using xNil = Nil;
#endif

        constexpr bool CPP_true_fn(Nil)
        {
            return true;
        }
    } // namespace ranges_detail

#if defined(__clang__) || defined(_MSC_VER)
    template<bool B>
    std::enable_if_t<B> requires_()
    {}
#else
    template<bool B>
    CPP_INLINE_VAR constexpr std::enable_if_t<B, int> requires_ = 0;
#endif

    inline namespace defs
    {
        ////////////////////////////////////////////////////////////////////////
        // Utility concepts
        ////////////////////////////////////////////////////////////////////////

        template<bool B>
        CPP_concept is_true = B;

        template<typename... Args>
        CPP_concept type = true;

        template<class T, template<typename...> class Trait, typename... Args>
        CPP_concept satisfies =
            static_cast<bool>(Trait<T, Args...>::type::value);

        ////////////////////////////////////////////////////////////////////////
        // Core language concepts
        ////////////////////////////////////////////////////////////////////////

        template<typename A, typename B>
        CPP_concept same_as =
            META_IS_SAME(A, B) && META_IS_SAME(B, A);

        /// \cond
        template<typename A, typename B>
        CPP_concept not_same_as_ =
            (!same_as<remove_cvref_t<A>, remove_cvref_t<B>>);

        // Workaround bug in the Standard Library:
        // From cannot be an incomplete class type despite that
        // is_convertible<X, Y> should be equivalent to is_convertible<X&&, Y>
        // in such a case.
        template<typename From, typename To>
        CPP_concept implicitly_convertible_to =
            std::is_convertible<std::add_rvalue_reference_t<From>, To>::value;

        template<typename From, typename To>
        CPP_requires(explicitly_convertible_to_,
            requires(From(*from)()) //
            (
                static_cast<To>(from())
            ));
        template<typename From, typename To>
        CPP_concept explicitly_convertible_to =
            CPP_requires_ref(concepts::explicitly_convertible_to_, From, To);
        /// \endcond

        template<typename From, typename To>
        CPP_concept convertible_to =
            implicitly_convertible_to<From, To> &&
            explicitly_convertible_to<From, To>;

        CPP_template(typename T, typename U)(
        concept (derived_from_)(T, U),
            convertible_to<T const volatile *, U const volatile *>
        );
        template<typename T, typename U>
        CPP_concept derived_from =
            META_IS_BASE_OF(U, T) &&
            CPP_concept_ref(concepts::derived_from_, T, U);

        CPP_template(typename T, typename U)(
        concept (common_reference_with_)(T, U),
            same_as<common_reference_t<T, U>, common_reference_t<U, T>> CPP_and
            convertible_to<T, common_reference_t<T, U>> CPP_and
            convertible_to<U, common_reference_t<T, U>>
        );
        template<typename T, typename U>
        CPP_concept common_reference_with =
            CPP_concept_ref(concepts::common_reference_with_, T, U);

        CPP_template(typename T, typename U)(
        concept (common_with_)(T, U),
            same_as<common_type_t<T, U>, common_type_t<U, T>> CPP_and
            convertible_to<T, common_type_t<T, U>> CPP_and
            convertible_to<U, common_type_t<T, U>> CPP_and
            common_reference_with<
                std::add_lvalue_reference_t<T const>,
                std::add_lvalue_reference_t<U const>> CPP_and
            common_reference_with<
                std::add_lvalue_reference_t<common_type_t<T, U>>,
                common_reference_t<
                    std::add_lvalue_reference_t<T const>,
                    std::add_lvalue_reference_t<U const>>>
        );
        template<typename T, typename U>
        CPP_concept common_with =
            CPP_concept_ref(concepts::common_with_, T, U);

        template<typename T>
        CPP_concept integral =
            std::is_integral<T>::value;

        template<typename T>
        CPP_concept signed_integral =
            integral<T> &&
            std::is_signed<T>::value;

        template<typename T>
        CPP_concept unsigned_integral =
            integral<T> &&
            !signed_integral<T>;

        template<typename T, typename U>
        CPP_requires(assignable_from_,
            requires(T t, U && u) //
            (
                t = (U &&) u,
                requires_<same_as<T, decltype(t = (U &&) u)>>
            ));
        template<typename T, typename U>
        CPP_concept assignable_from =
            std::is_lvalue_reference<T>::value &&
            common_reference_with<ranges_detail::as_cref_t<T>, ranges_detail::as_cref_t<U>> &&
            CPP_requires_ref(defs::assignable_from_, T, U);

        template<typename T>
        CPP_requires(swappable_,
            requires(T & t, T & u) //
            (
                concepts::swap(t, u)
            ));
        template<typename T>
        CPP_concept swappable =
            CPP_requires_ref(defs::swappable_, T);

        template<typename T, typename U>
        CPP_requires(swappable_with_,
            requires(T && t, U && u) //
            (
                concepts::swap((T &&) t, (T &&) t),
                concepts::swap((U &&) u, (U &&) u),
                concepts::swap((U &&) u, (T &&) t),
                concepts::swap((T &&) t, (U &&) u)
            ));
        template<typename T, typename U>
        CPP_concept swappable_with =
            common_reference_with<ranges_detail::as_cref_t<T>, ranges_detail::as_cref_t<U>> &&
            CPP_requires_ref(defs::swappable_with_, T, U);

    }  // inline namespace defs

    namespace ranges_detail
    {
        template<typename T>
        CPP_concept boolean_testable_impl_ = convertible_to<T, bool>;

        template<typename T>
        CPP_requires(boolean_testable_frag_,
            requires(T && t) //
            (
                !(T&&) t,
                concepts::requires_<boolean_testable_impl_<decltype(!(T&&) t)>>
            ));

        template<typename T>
        CPP_concept boolean_testable_ =
            CPP_requires_ref(boolean_testable_frag_, T) &&
            boolean_testable_impl_<T>;

        CPP_DIAGNOSTIC_PUSH
        CPP_DIAGNOSTIC_IGNORE_FLOAT_EQUAL

        template<typename T, typename U>
        CPP_requires(weakly_equality_comparable_with_frag_,
            requires(ranges_detail::as_cref_t<T> t, ranges_detail::as_cref_t<U> u) //
            (
                concepts::requires_<boolean_testable_<decltype(t == u)>>,
                concepts::requires_<boolean_testable_<decltype(t != u)>>,
                concepts::requires_<boolean_testable_<decltype(u == t)>>,
                concepts::requires_<boolean_testable_<decltype(u != t)>>
            ));
        template<typename T, typename U>
        CPP_concept weakly_equality_comparable_with_ =
            CPP_requires_ref(weakly_equality_comparable_with_frag_, T, U);

        template<typename T, typename U>
        CPP_requires(partially_ordered_with_frag_,
            requires(ranges_detail::as_cref_t<T>& t, ranges_detail::as_cref_t<U>& u) //
            (
                concepts::requires_<boolean_testable_<decltype(t < u)>>,
                concepts::requires_<boolean_testable_<decltype(t > u)>>,
                concepts::requires_<boolean_testable_<decltype(t <= u)>>,
                concepts::requires_<boolean_testable_<decltype(t >= u)>>,
                concepts::requires_<boolean_testable_<decltype(u < t)>>,
                concepts::requires_<boolean_testable_<decltype(u > t)>>,
                concepts::requires_<boolean_testable_<decltype(u <= t)>>,
                concepts::requires_<boolean_testable_<decltype(u >= t)>>
            ));
        template<typename T, typename U>
        CPP_concept partially_ordered_with_ =
            CPP_requires_ref(partially_ordered_with_frag_, T, U);

        CPP_DIAGNOSTIC_POP
    } // namespace ranges_detail

    inline namespace defs
    {
        ////////////////////////////////////////////////////////////////////////
        // Comparison concepts
        ////////////////////////////////////////////////////////////////////////

        template<typename T>
        CPP_concept equality_comparable =
            ranges_detail::weakly_equality_comparable_with_<T, T>;

        CPP_template(typename T, typename U)(
        concept (equality_comparable_with_)(T, U),
            equality_comparable<
                common_reference_t<ranges_detail::as_cref_t<T>, ranges_detail::as_cref_t<U>>>
        );
        template<typename T, typename U>
        CPP_concept equality_comparable_with =
            equality_comparable<T> &&
            equality_comparable<U> &&
            ranges_detail::weakly_equality_comparable_with_<T, U> &&
            common_reference_with<ranges_detail::as_cref_t<T>, ranges_detail::as_cref_t<U>> &&
            CPP_concept_ref(concepts::equality_comparable_with_, T, U);

        template<typename T>
        CPP_concept totally_ordered =
            equality_comparable<T> &&
            ranges_detail::partially_ordered_with_<T, T>;

        CPP_template(typename T, typename U)(
        concept (totally_ordered_with_)(T, U),
            totally_ordered<
                common_reference_t<
                    ranges_detail::as_cref_t<T>,
                    ranges_detail::as_cref_t<U>>> CPP_and
            ranges_detail::partially_ordered_with_<T, U>);

        template<typename T, typename U>
        CPP_concept totally_ordered_with =
            totally_ordered<T> &&
            totally_ordered<U> &&
            equality_comparable_with<T, U> &&
            CPP_concept_ref(concepts::totally_ordered_with_, T, U);

        ////////////////////////////////////////////////////////////////////////
        // Object concepts
        ////////////////////////////////////////////////////////////////////////

        template<typename T>
        CPP_concept destructible =
            std::is_nothrow_destructible<T>::value;

        template<typename T, typename... Args>
        CPP_concept constructible_from =
            destructible<T> &&
            META_IS_CONSTRUCTIBLE(T, Args...);

        template<typename T>
        CPP_concept default_constructible =
            constructible_from<T>;

        template<typename T>
        CPP_concept move_constructible =
            constructible_from<T, T> &&
            convertible_to<T, T>;

        CPP_template(typename T)(
        concept (copy_constructible_)(T),
            constructible_from<T, T &> &&
            constructible_from<T, T const &> &&
            constructible_from<T, T const> &&
            convertible_to<T &, T> &&
            convertible_to<T const &, T> &&
            convertible_to<T const, T>);
        template<typename T>
        CPP_concept copy_constructible =
            move_constructible<T> &&
            CPP_concept_ref(concepts::copy_constructible_, T);

        CPP_template(typename T)(
        concept (move_assignable_)(T),
            assignable_from<T &, T>
        );
        template<typename T>
        CPP_concept movable =
            std::is_object<T>::value &&
            move_constructible<T> &&
            CPP_concept_ref(concepts::move_assignable_, T) &&
            swappable<T>;

        CPP_template(typename T)(
        concept (copy_assignable_)(T),
            assignable_from<T &, T const &>
        );
        template<typename T>
        CPP_concept copyable =
            copy_constructible<T> &&
            movable<T> &&
            CPP_concept_ref(concepts::copy_assignable_, T);

        template<typename T>
        CPP_concept semiregular =
            copyable<T> &&
            default_constructible<T>;
            // Axiom: copies are independent. See Fundamentals of Generic
            // Programming http://www.stepanovpapers.com/DeSt98.pdf

        template<typename T>
        CPP_concept regular =
            semiregular<T> &&
            equality_comparable<T>;

    } // inline namespace defs
} // namespace futures::detail::concepts

#endif // FUTURES_RANGES_UTILITY_CONCEPTS_HPP


// #include <futures/algorithm/detail/traits/range/range_fwd.h>
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

#ifndef FUTURES_RANGES_RANGE_FWD_HPP
#define FUTURES_RANGES_RANGE_FWD_HPP

// #include <type_traits>

// #include <utility>


// #include <futures/algorithm/detail/traits/range/meta/meta.h>


// #include <futures/algorithm/detail/traits/range/concepts/compare.h>
/// \file
//  CPP, the Concepts PreProcessor library
//
//  Copyright Eric Niebler 2018-present
//  Copyright (c) 2020-present, Google LLC.
//
//  Use, modification and distribution is subject to the
//  Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
// This source code is licensed under the MIT license found in the
// LICENSE file in the root directory of this source tree.
//
// Project home: https://github.com/ericniebler/range-v3
//
#ifndef CPP_COMPARE_HPP
#define CPP_COMPARE_HPP

#if __cplusplus > 201703L && defined(__cpp_impl_three_way_comparison) && __has_include(<compare>)

#include <compare>
// #include <futures/algorithm/detail/traits/range/compare.h>
/// \file
//  CPP, the Concepts PreProcessor library
//
//  Copyright Eric Niebler 2018-present
//  Copyright (c) 2020-present, Google LLC.
//
//  Use, modification and distribution is subject to the
//  Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
// This source code is licensed under the MIT license found in the
// LICENSE file in the root directory of this source tree.
//
// Project home: https://github.com/ericniebler/range-v3
//
#ifndef FUTURES_RANGES_COMPARE_HPP
#define FUTURES_RANGES_COMPARE_HPP

#if __cplusplus > 201703L && defined(__cpp_impl_three_way_comparison) && __has_include(<compare>)

// #include <compare>

// #include <type_traits>


namespace futures::detail {
    template <typename... Ts> struct common_comparison_category { using type = void; };

    template <typename... Ts>
    requires((std::is_same_v<Ts, std::partial_ordering> || std::is_same_v<Ts, std::weak_ordering> ||
              std::is_same_v<Ts, std::strong_ordering>)&&...) struct common_comparison_category<Ts...>
        : std::common_type<Ts...> {
    };

    template <typename... Ts> using common_comparison_category_t = typename common_comparison_category<Ts...>::type;
} // namespace futures::detail

#endif // __cplusplus
#endif // FUTURES_RANGES_COMPARE_HPP

// #include <futures/algorithm/detail/traits/range/concepts/concepts.h>


// clang-format off

namespace futures::detail::concepts
{
    // Note: concepts in this file can use C++20 concepts, since operator<=> isn't available in
    // compilers that don't support core concepts.
    namespace ranges_detail
    {
        template<typename T, typename Cat>
        concept compares_as = same_as<futures::detail::common_comparison_category_t<T, Cat>, Cat>;
    } // namespace ranges_detail

    inline namespace defs
    {
        template<typename T, typename Cat = std::partial_ordering>
        concept three_way_comparable =
            ranges_detail::weakly_equality_comparable_with_<T, T> &&
            ranges_detail::partially_ordered_with_<T ,T> &&
            requires(ranges_detail::as_cref_t<T>& a, ranges_detail::as_cref_t<T>& b) {
                { a <=> b } -> ranges_detail::compares_as<Cat>;
            };

        template<typename T, typename U, typename Cat = std::partial_ordering>
        concept three_way_comparable_with =
            three_way_comparable<T, Cat> &&
            three_way_comparable<U, Cat> &&
            common_reference_with<ranges_detail::as_cref_t<T>&, ranges_detail::as_cref_t<U>&> &&
            three_way_comparable<common_reference_t<ranges_detail::as_cref_t<T>&, ranges_detail::as_cref_t<U>&>> &&
            ranges_detail::partially_ordered_with_<T, U> &&
            requires(ranges_detail::as_cref_t<T>& t, ranges_detail::as_cref_t<U>& u) {
                { t <=> u } -> ranges_detail::compares_as<Cat>;
                { u <=> t } -> ranges_detail::compares_as<Cat>;
            };
    } // inline namespace defs
} // namespace futures::detail::concepts

// clang-format on

#endif // __cplusplus
#endif // CPP_COMPARE_HPP

// #include <futures/algorithm/detail/traits/range/concepts/concepts.h>


// #include <futures/algorithm/detail/traits/range/detail/config.h>
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

#ifndef FUTURES_RANGES_DETAIL_CONFIG_HPP
#define FUTURES_RANGES_DETAIL_CONFIG_HPP

// Grab some version information.
#ifndef __has_include
#include <iosfwd>
#elif __has_include(<version>)
#include <version>
#else
// #include <iosfwd>

#endif

#if (defined(NDEBUG) && !defined(RANGES_ENSURE_MSG)) ||                                                                \
    (!defined(NDEBUG) && !defined(RANGES_ASSERT) &&                                                                    \
     ((defined(__GNUC__) && !defined(__clang__) && (__GNUC__ < 5 || defined(__MINGW32__))) ||                          \
      defined(_MSVC_STL_VERSION)))
#include <cstdio>
#include <cstdlib>

namespace futures::detail {
    namespace ranges_detail {
        template <typename = void> [[noreturn]] void assert_failure(char const *file, int line, char const *msg) {
            std::fprintf(stderr, "%s(%d): %s\n", file, line, msg);
            std::abort();
        }
    } // namespace ranges_detail
} // namespace futures::detail

#endif

#ifndef RANGES_ASSERT
// Always use our hand-rolled assert implementation on older GCCs, which do
// not allow assert to be used in a constant expression, and on MSVC whose
// assert is not marked [[noreturn]].
#if !defined(NDEBUG) && ((defined(__GNUC__) && !defined(__clang__) && (__GNUC__ < 5 || defined(__MINGW32__))) ||       \
                         defined(_MSVC_STL_VERSION))
#define RANGES_ASSERT(...)                                                                                             \
    static_cast<void>((__VA_ARGS__)                                                                                    \
                          ? void(0)                                                                                    \
                          : ::futures::detail::ranges_detail::assert_failure(__FILE__, __LINE__, "assertion failed: " #__VA_ARGS__))
#else
#include <cassert>
#define RANGES_ASSERT assert
#endif
#endif

// #include <futures/algorithm/detail/traits/range/meta/meta_fwd.h>


#ifndef RANGES_ASSUME
#if defined(__clang__) || defined(__GNUC__)
#define RANGES_ASSUME(COND) static_cast<void>((COND) ? void(0) : __builtin_unreachable())
#elif defined(_MSC_VER)
#define RANGES_ASSUME(COND) static_cast<void>(__assume(COND))
#else
#define RANGES_ASSUME(COND) static_cast<void>(COND)
#endif
#endif // RANGES_ASSUME

#ifndef RANGES_EXPECT
#ifdef NDEBUG
#define RANGES_EXPECT(COND) RANGES_ASSUME(COND)
#else // NDEBUG
#define RANGES_EXPECT(COND) RANGES_ASSERT(COND)
#endif // NDEBUG
#endif // RANGES_EXPECT

#ifndef RANGES_ENSURE_MSG
#if defined(NDEBUG)
#define RANGES_ENSURE_MSG(COND, MSG)                                                                                   \
    static_cast<void>((COND) ? void(0) : ::futures::detail::ranges_detail::assert_failure(__FILE__, __LINE__, "ensure failed: " MSG))
#else
#define RANGES_ENSURE_MSG(COND, MSG) RANGES_ASSERT((COND) && MSG)
#endif
#endif

#ifndef RANGES_ENSURE
#define RANGES_ENSURE(...) RANGES_ENSURE_MSG((__VA_ARGS__), #__VA_ARGS__)
#endif

#define RANGES_DECLTYPE_AUTO_RETURN(...)                                                                               \
    ->decltype(__VA_ARGS__) { return (__VA_ARGS__); }                                                                  \
    /**/

#define RANGES_DECLTYPE_AUTO_RETURN_NOEXCEPT(...)                                                                      \
    noexcept(noexcept(decltype(__VA_ARGS__)(__VA_ARGS__)))->decltype(__VA_ARGS__) { return (__VA_ARGS__); }            \
    /**/

#define RANGES_AUTO_RETURN_NOEXCEPT(...)                                                                               \
    noexcept(noexcept(decltype(__VA_ARGS__)(__VA_ARGS__))) { return (__VA_ARGS__); }                                   \
    /**/

#define RANGES_DECLTYPE_NOEXCEPT(...) noexcept(noexcept(decltype(__VA_ARGS__)(__VA_ARGS__)))->decltype(__VA_ARGS__) /**/

// Non-portable forward declarations of standard containers
#define RANGES_BEGIN_NAMESPACE_STD META_BEGIN_NAMESPACE_STD
#define RANGES_END_NAMESPACE_STD META_END_NAMESPACE_STD
#define RANGES_BEGIN_NAMESPACE_VERSION META_BEGIN_NAMESPACE_VERSION
#define RANGES_END_NAMESPACE_VERSION META_END_NAMESPACE_VERSION
#define RANGES_BEGIN_NAMESPACE_CONTAINER META_BEGIN_NAMESPACE_CONTAINER
#define RANGES_END_NAMESPACE_CONTAINER META_END_NAMESPACE_CONTAINER

// Database of feature versions
#define RANGES_CXX_STATIC_ASSERT_11 200410L
#define RANGES_CXX_STATIC_ASSERT_14 RANGES_CXX_STATIC_ASSERT_11
#define RANGES_CXX_STATIC_ASSERT_17 201411L
#define RANGES_CXX_VARIABLE_TEMPLATES_11 0L
#define RANGES_CXX_VARIABLE_TEMPLATES_14 201304L
#define RANGES_CXX_VARIABLE_TEMPLATES_17 RANGES_CXX_VARIABLE_TEMPLATES_14
#define RANGES_CXX_ATTRIBUTE_DEPRECATED_11 0L
#define RANGES_CXX_ATTRIBUTE_DEPRECATED_14 201309L
#define RANGES_CXX_ATTRIBUTE_DEPRECATED_17 RANGES_CXX_ATTRIBUTE_DEPRECATED_14
#define RANGES_CXX_CONSTEXPR_11 200704L
#define RANGES_CXX_CONSTEXPR_14 201304L
#define RANGES_CXX_CONSTEXPR_17 201603L
#define RANGES_CXX_CONSTEXPR_LAMBDAS 201603L
#define RANGES_CXX_RANGE_BASED_FOR_11 200907L
#define RANGES_CXX_RANGE_BASED_FOR_14 RANGES_CXX_RANGE_BASED_FOR_11
#define RANGES_CXX_RANGE_BASED_FOR_17 201603L
#define RANGES_CXX_LIB_IS_FINAL_11 0L
#define RANGES_CXX_LIB_IS_FINAL_14 201402L
#define RANGES_CXX_LIB_IS_FINAL_17 RANGES_CXX_LIB_IS_FINAL_14
#define RANGES_CXX_RETURN_TYPE_DEDUCTION_11 0L
#define RANGES_CXX_RETURN_TYPE_DEDUCTION_14 201304L
#define RANGES_CXX_RETURN_TYPE_DEDUCTION_17 RANGES_CXX_RETURN_TYPE_DEDUCTION_14
#define RANGES_CXX_GENERIC_LAMBDAS_11 0L
#define RANGES_CXX_GENERIC_LAMBDAS_14 201304L
#define RANGES_CXX_GENERIC_LAMBDAS_17 RANGES_CXX_GENERIC_LAMBDAS_14
#define RANGES_CXX_STD_11 201103L
#define RANGES_CXX_STD_14 201402L
#define RANGES_CXX_STD_17 201703L
#define RANGES_CXX_THREAD_LOCAL_PRE_STANDARD 200000L // Arbitrary number between 0 and C++11
#define RANGES_CXX_THREAD_LOCAL_11 RANGES_CXX_STD_11
#define RANGES_CXX_THREAD_LOCAL_14 RANGES_CXX_THREAD_LOCAL_11
#define RANGES_CXX_THREAD_LOCAL_17 RANGES_CXX_THREAD_LOCAL_14
#define RANGES_CXX_INLINE_VARIABLES_11 0L
#define RANGES_CXX_INLINE_VARIABLES_14 0L
#define RANGES_CXX_INLINE_VARIABLES_17 201606L
#define RANGES_CXX_COROUTINES_11 0L
#define RANGES_CXX_COROUTINES_14 0L
#define RANGES_CXX_COROUTINES_17 0L
#define RANGES_CXX_COROUTINES_TS1 201703L
#define RANGES_CXX_DEDUCTION_GUIDES_11 0L
#define RANGES_CXX_DEDUCTION_GUIDES_14 0L
#define RANGES_CXX_DEDUCTION_GUIDES_17 201606L
#define RANGES_CXX_IF_CONSTEXPR_11 0L
#define RANGES_CXX_IF_CONSTEXPR_14 0L
#define RANGES_CXX_IF_CONSTEXPR_17 201606L
#define RANGES_CXX_ALIGNED_NEW_11 0L
#define RANGES_CXX_ALIGNED_NEW_14 0L
#define RANGES_CXX_ALIGNED_NEW_17 201606L

// Implementation-specific diagnostic control
#if defined(_MSC_VER) && !defined(__clang__)
#define RANGES_DIAGNOSTIC_PUSH __pragma(warning(push))
#define RANGES_DIAGNOSTIC_POP __pragma(warning(pop))
#define RANGES_DIAGNOSTIC_IGNORE_PRAGMAS __pragma(warning(disable : 4068))
#define RANGES_DIAGNOSTIC_IGNORE(X) RANGES_DIAGNOSTIC_IGNORE_PRAGMAS __pragma(warning(disable : X))
#define RANGES_DIAGNOSTIC_IGNORE_SHADOWING RANGES_DIAGNOSTIC_IGNORE(4456)
#define RANGES_DIAGNOSTIC_IGNORE_INDENTATION
#define RANGES_DIAGNOSTIC_IGNORE_UNDEFINED_INTERNAL
#define RANGES_DIAGNOSTIC_IGNORE_MISMATCHED_TAGS RANGES_DIAGNOSTIC_IGNORE(4099)
#define RANGES_DIAGNOSTIC_IGNORE_GLOBAL_CONSTRUCTORS
#define RANGES_DIAGNOSTIC_IGNORE_SIGN_CONVERSION
#define RANGES_DIAGNOSTIC_IGNORE_UNNEEDED_INTERNAL
#define RANGES_DIAGNOSTIC_IGNORE_UNNEEDED_MEMBER
#define RANGES_DIAGNOSTIC_IGNORE_ZERO_LENGTH_ARRAY
#define RANGES_DIAGNOSTIC_IGNORE_CXX17_COMPAT
#define RANGES_DIAGNOSTIC_IGNORE_CXX2A_COMPAT
#define RANGES_DIAGNOSTIC_IGNORE_FLOAT_EQUAL
#define RANGES_DIAGNOSTIC_IGNORE_MISSING_BRACES
#define RANGES_DIAGNOSTIC_IGNORE_UNDEFINED_FUNC_TEMPLATE
#define RANGES_DIAGNOSTIC_IGNORE_INCONSISTENT_OVERRIDE
#define RANGES_DIAGNOSTIC_IGNORE_RANGE_LOOP_ANALYSIS
#define RANGES_DIAGNOSTIC_IGNORE_DEPRECATED_DECLARATIONS RANGES_DIAGNOSTIC_IGNORE(4996)
#define RANGES_DIAGNOSTIC_IGNORE_DEPRECATED_THIS_CAPTURE
#define RANGES_DIAGNOSTIC_IGNORE_INIT_LIST_LIFETIME
// Ignores both "divide by zero" and "mod by zero":
#define RANGES_DIAGNOSTIC_IGNORE_DIVIDE_BY_ZERO RANGES_DIAGNOSTIC_IGNORE(4723 4724)
#define RANGES_DIAGNOSTIC_IGNORE_UNSIGNED_MATH RANGES_DIAGNOSTIC_IGNORE(4146)
#define RANGES_DIAGNOSTIC_IGNORE_TRUNCATION RANGES_DIAGNOSTIC_IGNORE(4244)
#define RANGES_DIAGNOSTIC_IGNORE_MULTIPLE_ASSIGNMENT_OPERATORS RANGES_DIAGNOSTIC_IGNORE(4522)
#define RANGES_DIAGNOSTIC_IGNORE_VOID_PTR_DEREFERENCE
#define RANGES_DIAGNOSTIC_KEYWORD_MACRO

#define RANGES_CXX_VER _MSVC_LANG

#if _MSC_VER < 1920 || _MSVC_LANG < 201703L
#error range-v3 requires Visual Studio 2019 with the /std:c++17 (or /std:c++latest) and /permissive- options.
#endif

#if _MSC_VER < 1927
#define RANGES_WORKAROUND_MSVC_895622 // Error when phase 1 name binding finds only
                                      // deleted function

#if _MSC_VER < 1925
#define RANGES_WORKAROUND_MSVC_779708 // ADL for operands of function type [No workaround]

#if _MSC_VER < 1923
#define RANGES_WORKAROUND_MSVC_573728 // rvalues of array types bind to lvalue references
                                      // [no workaround]
#define RANGES_WORKAROUND_MSVC_934330 // Deduction guide not correctly preferred to copy
                                      // deduction candidate [No workaround]

#if _MSC_VER < 1922
#define RANGES_WORKAROUND_MSVC_756601 // constexpr friend non-template erroneously
                                      // rejected with C3615
#define RANGES_WORKAROUND_MSVC_793042 // T[0] sometimes accepted as a valid type in SFINAE
                                      // context

#if _MSC_VER < 1921
#define RANGES_WORKAROUND_MSVC_785522 // SFINAE failure for error in immediate context
#define RANGES_WORKAROUND_MSVC_786376 // Assertion casting anonymous union member in
                                      // trailing-return-type
#define RANGES_WORKAROUND_MSVC_787074 // Over-eager substitution of dependent type in
                                      // non-instantiated nested class template
#define RANGES_WORKAROUND_MSVC_790554 // Assert for return type that uses dependent
                                      // default non-type template argument
#endif                                // _MSC_VER < 1921
#endif                                // _MSC_VER < 1922
#endif                                // _MSC_VER < 1923
#endif                                // _MSC_VER < 1925
#endif                                // _MSC_VER < 1926

#if 1                                 // Fixed in 1920, but more bugs hiding behind workaround
#define RANGES_WORKAROUND_MSVC_701385 // Yet another alias expansion error
#endif

#define RANGES_WORKAROUND_MSVC_249830 // constexpr and arguments that aren't subject to
                                      // lvalue-to-rvalue conversion
#define RANGES_WORKAROUND_MSVC_677925 // Bogus C2676 "binary '++': '_Ty' does not define
                                      // this operator"
#define RANGES_WORKAROUND_MSVC_683388 // decltype(*i) is incorrectly an rvalue reference
                                      // for pointer-to-array i
#define RANGES_WORKAROUND_MSVC_688606 // SFINAE failing to account for access control
                                      // during specialization matching
#define RANGES_WORKAROUND_MSVC_786312 // Yet another mixed-pack-expansion failure
#define RANGES_WORKAROUND_MSVC_792338 // Failure to match specialization enabled via call
                                      // to constexpr function
#define RANGES_WORKAROUND_MSVC_835948 // Silent bad codegen destroying sized_generator [No
                                      // workaround]
#define RANGES_WORKAROUND_MSVC_934264 // Explicitly-defaulted inherited default
                                      // constructor is not correctly implicitly constexpr
#if _MSVC_LANG <= 201703L
#define RANGES_WORKAROUND_MSVC_OLD_LAMBDA
#endif

#elif defined(__GNUC__) || defined(__clang__)
#define RANGES_PRAGMA(X) _Pragma(#X)
#define RANGES_DIAGNOSTIC_PUSH RANGES_PRAGMA(GCC diagnostic push)
#define RANGES_DIAGNOSTIC_POP RANGES_PRAGMA(GCC diagnostic pop)
#define RANGES_DIAGNOSTIC_IGNORE_PRAGMAS RANGES_PRAGMA(GCC diagnostic ignored "-Wpragmas")
#define RANGES_DIAGNOSTIC_IGNORE(X)                                                                                    \
    RANGES_DIAGNOSTIC_IGNORE_PRAGMAS                                                                                   \
    RANGES_PRAGMA(GCC diagnostic ignored "-Wunknown-pragmas")                                                          \
    RANGES_PRAGMA(GCC diagnostic ignored "-Wunknown-warning-option")                                                   \
    RANGES_PRAGMA(GCC diagnostic ignored X)
#define RANGES_DIAGNOSTIC_IGNORE_SHADOWING RANGES_DIAGNOSTIC_IGNORE("-Wshadow")
#define RANGES_DIAGNOSTIC_IGNORE_INDENTATION RANGES_DIAGNOSTIC_IGNORE("-Wmisleading-indentation")
#define RANGES_DIAGNOSTIC_IGNORE_UNDEFINED_INTERNAL RANGES_DIAGNOSTIC_IGNORE("-Wundefined-internal")
#define RANGES_DIAGNOSTIC_IGNORE_MISMATCHED_TAGS RANGES_DIAGNOSTIC_IGNORE("-Wmismatched-tags")
#define RANGES_DIAGNOSTIC_IGNORE_SIGN_CONVERSION RANGES_DIAGNOSTIC_IGNORE("-Wsign-conversion")
#define RANGES_DIAGNOSTIC_IGNORE_FLOAT_EQUAL RANGES_DIAGNOSTIC_IGNORE("-Wfloat-equal")
#define RANGES_DIAGNOSTIC_IGNORE_MISSING_BRACES RANGES_DIAGNOSTIC_IGNORE("-Wmissing-braces")
#define RANGES_DIAGNOSTIC_IGNORE_GLOBAL_CONSTRUCTORS RANGES_DIAGNOSTIC_IGNORE("-Wglobal-constructors")
#define RANGES_DIAGNOSTIC_IGNORE_UNNEEDED_INTERNAL RANGES_DIAGNOSTIC_IGNORE("-Wunneeded-internal-declaration")
#define RANGES_DIAGNOSTIC_IGNORE_UNNEEDED_MEMBER RANGES_DIAGNOSTIC_IGNORE("-Wunneeded-member-function")
#define RANGES_DIAGNOSTIC_IGNORE_ZERO_LENGTH_ARRAY RANGES_DIAGNOSTIC_IGNORE("-Wzero-length-array")
#define RANGES_DIAGNOSTIC_IGNORE_CXX17_COMPAT RANGES_DIAGNOSTIC_IGNORE("-Wc++1z-compat")
#define RANGES_DIAGNOSTIC_IGNORE_CXX2A_COMPAT RANGES_DIAGNOSTIC_IGNORE("-Wc++2a-compat")
#define RANGES_DIAGNOSTIC_IGNORE_UNDEFINED_FUNC_TEMPLATE RANGES_DIAGNOSTIC_IGNORE("-Wundefined-func-template")
#define RANGES_DIAGNOSTIC_IGNORE_INCONSISTENT_OVERRIDE RANGES_DIAGNOSTIC_IGNORE("-Winconsistent-missing-override")
#define RANGES_DIAGNOSTIC_IGNORE_RANGE_LOOP_ANALYSIS RANGES_DIAGNOSTIC_IGNORE("-Wrange-loop-analysis")
#define RANGES_DIAGNOSTIC_IGNORE_DEPRECATED_DECLARATIONS RANGES_DIAGNOSTIC_IGNORE("-Wdeprecated-declarations")
#define RANGES_DIAGNOSTIC_IGNORE_DEPRECATED_THIS_CAPTURE RANGES_DIAGNOSTIC_IGNORE("-Wdeprecated-this-capture")
#define RANGES_DIAGNOSTIC_IGNORE_INIT_LIST_LIFETIME RANGES_DIAGNOSTIC_IGNORE("-Winit-list-lifetime")
#define RANGES_DIAGNOSTIC_IGNORE_DIVIDE_BY_ZERO
#define RANGES_DIAGNOSTIC_IGNORE_UNSIGNED_MATH
#define RANGES_DIAGNOSTIC_IGNORE_TRUNCATION
#define RANGES_DIAGNOSTIC_IGNORE_MULTIPLE_ASSIGNMENT_OPERATORS
#define RANGES_DIAGNOSTIC_IGNORE_VOID_PTR_DEREFERENCE RANGES_DIAGNOSTIC_IGNORE("-Wvoid-ptr-dereference")
#define RANGES_DIAGNOSTIC_KEYWORD_MACRO RANGES_DIAGNOSTIC_IGNORE("-Wkeyword-macro")

#define RANGES_WORKAROUND_CWG_1554
#ifdef __clang__
#if __clang_major__ < 4
#define RANGES_WORKAROUND_CLANG_23135 // constexpr leads to premature instantiation on
                                      // clang-3.x
#endif
#define RANGES_WORKAROUND_CLANG_43400 // template friend is redefinition of itself
#else                                 // __GNUC__
#if __GNUC__ < 6
#define RANGES_WORKAROUND_GCC_UNFILED0 /* Workaround old GCC name lookup bug */
#endif
#if __GNUC__ == 7 || __GNUC__ == 8
#define RANGES_WORKAROUND_GCC_91525 /* Workaround strange GCC ICE */
#endif
#if __GNUC__ >= 9
#if __GNUC__ == 9 && __GNUC_MINOR__ < 3 && __cplusplus == RANGES_CXX_STD_17
#define RANGES_WORKAROUND_GCC_91923 // Failure-to-SFINAE with class type NTTP in C++17
#endif
#endif
#endif

#else
#define RANGES_DIAGNOSTIC_PUSH
#define RANGES_DIAGNOSTIC_POP
#define RANGES_DIAGNOSTIC_IGNORE_PRAGMAS
#define RANGES_DIAGNOSTIC_IGNORE_SHADOWING
#define RANGES_DIAGNOSTIC_IGNORE_INDENTATION
#define RANGES_DIAGNOSTIC_IGNORE_UNDEFINED_INTERNAL
#define RANGES_DIAGNOSTIC_IGNORE_MISMATCHED_TAGS
#define RANGES_DIAGNOSTIC_IGNORE_GLOBAL_CONSTRUCTORS
#define RANGES_DIAGNOSTIC_IGNORE_SIGN_CONVERSION
#define RANGES_DIAGNOSTIC_IGNORE_UNNEEDED_INTERNAL
#define RANGES_DIAGNOSTIC_IGNORE_UNNEEDED_MEMBER
#define RANGES_DIAGNOSTIC_IGNORE_ZERO_LENGTH_ARRAY
#define RANGES_DIAGNOSTIC_IGNORE_CXX17_COMPAT
#define RANGES_DIAGNOSTIC_IGNORE_CXX2A_COMPAT
#define RANGES_DIAGNOSTIC_IGNORE_FLOAT_EQUAL
#define RANGES_DIAGNOSTIC_IGNORE_MISSING_BRACES
#define RANGES_DIAGNOSTIC_IGNORE_UNDEFINED_FUNC_TEMPLATE
#define RANGES_DIAGNOSTIC_IGNORE_INCONSISTENT_OVERRIDE
#define RANGES_DIAGNOSTIC_IGNORE_DEPRECATED_DECLARATIONS
#define RANGES_DIAGNOSTIC_IGNORE_DEPRECATED_THIS_CAPTURE
#define RANGES_DIAGNOSTIC_IGNORE_INIT_LIST_LIFETIME
#define RANGES_DIAGNOSTIC_IGNORE_DIVIDE_BY_ZERO
#define RANGES_DIAGNOSTIC_IGNORE_UNSIGNED_MATH
#define RANGES_DIAGNOSTIC_IGNORE_TRUNCATION
#define RANGES_DIAGNOSTIC_IGNORE_MULTIPLE_ASSIGNMENT_OPERATORS
#define RANGES_DIAGNOSTIC_IGNORE_VOID_PTR_DEREFERENCE
#define RANGES_DIAGNOSTIC_KEYWORD_MACRO
#endif

// Configuration via feature-test macros, with fallback to __cplusplus
#ifndef RANGES_CXX_VER
#define RANGES_CXX_VER __cplusplus
#endif

#define RANGES_CXX_FEATURE_CONCAT2(y, z) RANGES_CXX_##y##_##z
#define RANGES_CXX_FEATURE_CONCAT(y, z) RANGES_CXX_FEATURE_CONCAT2(y, z)

#if RANGES_CXX_VER >= RANGES_CXX_STD_17
#define RANGES_CXX_STD RANGES_CXX_STD_17
#define RANGES_CXX_FEATURE(x) RANGES_CXX_FEATURE_CONCAT(x, 17)
#elif RANGES_CXX_VER >= RANGES_CXX_STD_14
#define RANGES_CXX_STD RANGES_CXX_STD_14
#define RANGES_CXX_FEATURE(x) RANGES_CXX_FEATURE_CONCAT(x, 14)
#else
#define RANGES_CXX_STD RANGES_CXX_STD_11
#define RANGES_CXX_FEATURE(x) RANGES_CXX_FEATURE_CONCAT(x, 11)
#endif

#ifndef RANGES_CXX_STATIC_ASSERT
#ifdef __cpp_static_assert
#define RANGES_CXX_STATIC_ASSERT __cpp_static_assert
#else
#define RANGES_CXX_STATIC_ASSERT RANGES_CXX_FEATURE(STATIC_ASSERT)
#endif
#endif

#ifndef RANGES_CXX_VARIABLE_TEMPLATES
#ifdef __cpp_variable_templates
#define RANGES_CXX_VARIABLE_TEMPLATES __cpp_variable_templates
#else
#define RANGES_CXX_VARIABLE_TEMPLATES RANGES_CXX_FEATURE(VARIABLE_TEMPLATES)
#endif
#endif

#if (defined(__cpp_lib_type_trait_variable_templates) && __cpp_lib_type_trait_variable_templates > 0) ||               \
    RANGES_CXX_VER >= RANGES_CXX_STD_17
#define RANGES_CXX_TRAIT_VARIABLE_TEMPLATES 1
#else
#define RANGES_CXX_TRAIT_VARIABLE_TEMPLATES 0
#endif

#ifndef RANGES_CXX_ATTRIBUTE_DEPRECATED
#ifdef __has_cpp_attribute
#define RANGES_CXX_ATTRIBUTE_DEPRECATED __has_cpp_attribute(deprecated)
#elif defined(__cpp_attribute_deprecated)
#define RANGES_CXX_ATTRIBUTE_DEPRECATED __cpp_attribute_deprecated
#else
#define RANGES_CXX_ATTRIBUTE_DEPRECATED RANGES_CXX_FEATURE(ATTRIBUTE_DEPRECATED)
#endif
#endif

#ifndef RANGES_CXX_CONSTEXPR
#ifdef __cpp_constexpr
#define RANGES_CXX_CONSTEXPR __cpp_constexpr
#else
#define RANGES_CXX_CONSTEXPR RANGES_CXX_FEATURE(CONSTEXPR)
#endif
#endif

#ifndef RANGES_CXX_RANGE_BASED_FOR
#ifdef __cpp_range_based_for
#define RANGES_CXX_RANGE_BASED_FOR __cpp_range_based_for
#else
#define RANGES_CXX_RANGE_BASED_FOR RANGES_CXX_FEATURE(RANGE_BASED_FOR)
#endif
#endif

#ifndef RANGES_CXX_LIB_IS_FINAL
// #include <type_traits>

#ifdef __cpp_lib_is_final
#define RANGES_CXX_LIB_IS_FINAL __cpp_lib_is_final
#else
#define RANGES_CXX_LIB_IS_FINAL RANGES_CXX_FEATURE(LIB_IS_FINAL)
#endif
#endif

#ifndef RANGES_CXX_RETURN_TYPE_DEDUCTION
#ifdef __cpp_return_type_deduction
#define RANGES_CXX_RETURN_TYPE_DEDUCTION __cpp_return_type_deduction
#else
#define RANGES_CXX_RETURN_TYPE_DEDUCTION RANGES_CXX_FEATURE(RETURN_TYPE_DEDUCTION)
#endif
#endif

#ifndef RANGES_CXX_GENERIC_LAMBDAS
#ifdef __cpp_generic_lambdas
#define RANGES_CXX_GENERIC_LAMBDAS __cpp_generic_lambdas
#else
#define RANGES_CXX_GENERIC_LAMBDAS RANGES_CXX_FEATURE(GENERIC_LAMBDAS)
#endif
#endif

#ifndef RANGES_CXX_THREAD_LOCAL
#if defined(__IPHONE_OS_VERSION_MIN_REQUIRED) && __IPHONE_OS_VERSION_MIN_REQUIRED <= 70100
#define RANGES_CXX_THREAD_LOCAL 0
#elif defined(__IPHONE_OS_VERSION_MIN_REQUIRED) ||                                                                     \
    (defined(__clang__) && (defined(__CYGWIN__) || defined(__apple_build_version__)))
// BUGBUG avoid unresolved __cxa_thread_atexit
#define RANGES_CXX_THREAD_LOCAL RANGES_CXX_THREAD_LOCAL_PRE_STANDARD
#else
#define RANGES_CXX_THREAD_LOCAL RANGES_CXX_FEATURE(THREAD_LOCAL)
#endif
#endif

#if !defined(RANGES_DEPRECATED) && !defined(RANGES_DISABLE_DEPRECATED_WARNINGS)
#if defined(__GNUC__) && !defined(__clang__)
// GCC's support for [[deprecated("message")]] is unusably buggy.
#define RANGES_DEPRECATED(MSG) __attribute__((deprecated(MSG)))
#elif RANGES_CXX_ATTRIBUTE_DEPRECATED &&                                                                               \
    !((defined(__clang__) || defined(__GNUC__)) && RANGES_CXX_STD < RANGES_CXX_STD_14)
#define RANGES_DEPRECATED(MSG) [[deprecated(MSG)]]
#elif defined(__clang__) || defined(__GNUC__)
#define RANGES_DEPRECATED(MSG) __attribute__((deprecated(MSG)))
#endif
#endif
#ifndef RANGES_DEPRECATED
#define RANGES_DEPRECATED(MSG)
#endif

#if !defined(RANGES_DEPRECATED_HEADER) && !defined(RANGES_DISABLE_DEPRECATED_WARNINGS)
#ifdef __GNUC__
#define RANGES_DEPRECATED_HEADER(MSG) RANGES_PRAGMA(GCC warning MSG)
#elif defined(_MSC_VER)
#define RANGES_STRINGIZE_(MSG) #MSG
#define RANGES_STRINGIZE(MSG) RANGES_STRINGIZE_(MSG)
#define RANGES_DEPRECATED_HEADER(MSG) __pragma(message(__FILE__ "(" RANGES_STRINGIZE(__LINE__) ") : Warning: " MSG))
#endif
#else
#define RANGES_DEPRECATED_HEADER(MSG) /**/
#endif
// #ifndef RANGES_DEPRECATED_HEADER
// #define RANGES_DEPRECATED_HEADER(MSG)
// #endif

#ifndef RANGES_CXX_COROUTINES
#if defined(__cpp_coroutines) && defined(__has_include)
#if __has_include(<coroutine>)
#define RANGES_CXX_COROUTINES __cpp_coroutines
#define RANGES_COROUTINES_HEADER <coroutine>
#define RANGES_COROUTINES_NS std
#elif __has_include(<experimental/coroutine>)
#define RANGES_CXX_COROUTINES __cpp_coroutines
#define RANGES_COROUTINES_HEADER <experimental/coroutine>
#define RANGES_COROUTINES_NS std::experimental
#endif
#endif
#ifndef RANGES_CXX_COROUTINES
#define RANGES_CXX_COROUTINES RANGES_CXX_FEATURE(COROUTINES)
#endif
#endif

// RANGES_CXX14_CONSTEXPR macro (see also BOOST_CXX14_CONSTEXPR)
// Note: constexpr implies inline, to retain the same visibility
// C++14 constexpr functions are inline in C++11
#if RANGES_CXX_CONSTEXPR >= RANGES_CXX_CONSTEXPR_14
#define RANGES_CXX14_CONSTEXPR constexpr
#else
#define RANGES_CXX14_CONSTEXPR inline
#endif

#ifdef NDEBUG
#define RANGES_NDEBUG_CONSTEXPR constexpr
#else
#define RANGES_NDEBUG_CONSTEXPR inline
#endif

#ifndef RANGES_CXX_INLINE_VARIABLES
#ifdef __cpp_inline_variables
#define RANGES_CXX_INLINE_VARIABLES __cpp_inline_variables
#elif defined(__clang__) && (__clang_major__ == 3 && __clang_minor__ == 9) && RANGES_CXX_VER > RANGES_CXX_STD_14
// Clang 3.9 supports inline variables in C++17 mode, but doesn't define
// __cpp_inline_variables
#define RANGES_CXX_INLINE_VARIABLES RANGES_CXX_INLINE_VARIABLES_17
#else
#define RANGES_CXX_INLINE_VARIABLES RANGES_CXX_FEATURE(INLINE_VARIABLES)
#endif // __cpp_inline_variables
#endif // RANGES_CXX_INLINE_VARIABLES

#if RANGES_CXX_INLINE_VARIABLES < RANGES_CXX_INLINE_VARIABLES_17 && !defined(RANGES_DOXYGEN_INVOKED)
#define RANGES_INLINE_VAR
#define RANGES_INLINE_VARIABLE(type, name)                                                                             \
    namespace {                                                                                                        \
        constexpr auto &name = ::futures::detail::static_const<type>::value;                                                    \
    }
#else // RANGES_CXX_INLINE_VARIABLES >= RANGES_CXX_INLINE_VARIABLES_17
#define RANGES_INLINE_VAR inline
#define RANGES_INLINE_VARIABLE(type, name)                                                                             \
    inline constexpr type name{};                                                                                      \
    /**/
#endif // RANGES_CXX_INLINE_VARIABLES

#if defined(RANGES_DOXYGEN_INVOKED)
#define RANGES_DEFINE_CPO(type, name)                                                                                  \
    inline constexpr type name{};                                                                                      \
    /**/
#elif RANGES_CXX_INLINE_VARIABLES < RANGES_CXX_INLINE_VARIABLES_17
#define RANGES_DEFINE_CPO(type, name)                                                                                  \
    namespace {                                                                                                        \
        constexpr auto &name = ::futures::detail::static_const<type>::value;                                                    \
    }                                                                                                                  \
    /**/
#else // RANGES_CXX_INLINE_VARIABLES >= RANGES_CXX_INLINE_VARIABLES_17
#define RANGES_DEFINE_CPO(type, name)                                                                                  \
    namespace _ {                                                                                                      \
        inline constexpr type name{};                                                                                  \
    }                                                                                                                  \
    using namespace _;                                                                                                 \
    /**/
#endif // RANGES_CXX_INLINE_VARIABLES

#ifndef RANGES_DOXYGEN_INVOKED
#define RANGES_HIDDEN_DETAIL(...) __VA_ARGS__
#else
#define RANGES_HIDDEN_DETAIL(...)
#endif

#ifndef RANGES_DOXYGEN_INVOKED
#define RANGES_ADL_BARRIER_FOR(S) S##_ns
#define RANGES_STRUCT_WITH_ADL_BARRIER(S)                                                                              \
    _ranges_adl_barrier_noop_;                                                                                         \
    namespace RANGES_ADL_BARRIER_FOR(S) {                                                                              \
        struct S;                                                                                                      \
    }                                                                                                                  \
    using RANGES_ADL_BARRIER_FOR(S)::S;                                                                                \
    struct RANGES_ADL_BARRIER_FOR(S)::S /**/
#else
#define RANGES_ADL_BARRIER_FOR(S)
#define RANGES_STRUCT_WITH_ADL_BARRIER(S) S
#endif

#ifndef RANGES_DOXYGEN_INVOKED
#define RANGES_FUNC_BEGIN(NAME) struct NAME##_fn {
#define RANGES_FUNC_END(NAME)                                                                                          \
    }                                                                                                                  \
    ;                                                                                                                  \
    RANGES_INLINE_VARIABLE(NAME##_fn, NAME)
#define RANGES_FUNC(NAME) operator() RANGES_FUNC_CONST_ /**/
#define RANGES_FUNC_CONST_(...) (__VA_ARGS__) const
#else
#define RANGES_FUNC_BEGIN(NAME)
#define RANGES_FUNC_END(NAME)
#define RANGES_FUNC(NAME) NAME
#endif

#ifndef RANGES_CXX_DEDUCTION_GUIDES
#if defined(__clang__) && defined(__apple_build_version__)
// Apple's clang version doesn't do deduction guides very well.
#define RANGES_CXX_DEDUCTION_GUIDES 0
#elif defined(__cpp_deduction_guides)
#define RANGES_CXX_DEDUCTION_GUIDES __cpp_deduction_guides
#else
#define RANGES_CXX_DEDUCTION_GUIDES RANGES_CXX_FEATURE(DEDUCTION_GUIDES)
#endif // __cpp_deduction_guides
#endif // RANGES_CXX_DEDUCTION_GUIDES

// __VA_OPT__
#ifndef RANGES_CXX_VA_OPT
#if __cplusplus > 201703L
#define RANGES_CXX_THIRD_ARG_(A, B, C, ...) C
#define RANGES_CXX_VA_OPT_I_(...) RANGES_CXX_THIRD_ARG_(__VA_OPT__(, ), 1, 0, ?)
#define RANGES_CXX_VA_OPT RANGES_CXX_VA_OPT_I_(?)
#else
#define RANGES_CXX_VA_OPT 0
#endif
#endif // RANGES_CXX_VA_OPT

#ifndef RANGES_CXX_IF_CONSTEXPR
#ifdef __cpp_if_constexpr
#define RANGES_CXX_IF_CONSTEXPR __cpp_if_constexpr
#else
#define RANGES_CXX_IF_CONSTEXPR RANGES_CXX_FEATURE(IF_CONSTEXPR)
#endif
#endif // RANGES_CXX_IF_CONSTEXPR

// Its not enough for the compiler to support this; the stdlib must support it too.
#ifndef RANGES_CXX_ALIGNED_NEW
#if (!defined(_LIBCPP_VERSION) || (_LIBCPP_VERSION >= 4000 && !defined(_LIBCPP_HAS_NO_ALIGNED_ALLOCATION))) &&         \
    (!defined(__GLIBCXX__) || (defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE >= 7))
#if defined(__cpp_aligned_new)
#define RANGES_CXX_ALIGNED_NEW __cpp_aligned_new
#else
#define RANGES_CXX_ALIGNED_NEW RANGES_CXX_FEATURE(ALIGNED_NEW)
#endif
#else // _LIBCPP_VERSION < 4000 || __GLIBCXX__ < 20170502
#define RANGES_CXX_ALIGNED_NEW 0L
#endif
#endif // RANGES_CXX_ALIGNED_NEW

#if defined(__clang__)
#define RANGES_IS_SAME(...) __is_same(__VA_ARGS__)
#elif defined(__GNUC__) && __GNUC__ >= 6
#define RANGES_IS_SAME(...) __is_same_as(__VA_ARGS__)
#elif RANGES_CXX_TRAIT_VARIABLE_TEMPLATES
#define RANGES_IS_SAME(...) std::is_same_v<__VA_ARGS__>
#else
#define RANGES_IS_SAME(...) std::is_same<__VA_ARGS__>::value
#endif

// https://gcc.gnu.org/bugzilla/show_bug.cgi?id=93667
#if defined(__has_cpp_attribute) && __has_cpp_attribute(no_unique_address) &&                                          \
    !(defined(__GNUC__) && !defined(__clang__) && __GNUC__ >= 10)
#define RANGES_NO_UNIQUE_ADDRESS [[no_unique_address]]
#else
#define RANGES_NO_UNIQUE_ADDRESS
#endif

#if defined(__clang__)
#if __has_attribute(no_sanitize)
#define RANGES_INTENDED_MODULAR_ARITHMETIC __attribute__((__no_sanitize__("unsigned-integer-overflow")))
#else
#define RANGES_INTENDED_MODULAR_ARITHMETIC
#endif
#else
#define RANGES_INTENDED_MODULAR_ARITHMETIC
#endif

#ifndef RANGES_CONSTEXPR_IF
#if RANGES_CXX_IF_CONSTEXPR >= RANGES_CXX_IF_CONSTEXPR_17
#define RANGES_CONSTEXPR_IF(...) false) \
    {} else if constexpr(__VA_ARGS__
#else
#define RANGES_CONSTEXPR_IF(...) __VA_ARGS__
#endif
#endif // RANGES_CONSTEXPR_IF

#if !defined(RANGES_BROKEN_CPO_LOOKUP) && !defined(RANGES_DOXYGEN_INVOKED) &&                                          \
    (defined(RANGES_WORKAROUND_GCC_UNFILED0) || defined(RANGES_WORKAROUND_MSVC_895622))
#define RANGES_BROKEN_CPO_LOOKUP 1
#endif
#ifndef RANGES_BROKEN_CPO_LOOKUP
#define RANGES_BROKEN_CPO_LOOKUP 0
#endif

#ifndef RANGES_NODISCARD
#if defined(__has_cpp_attribute) && __has_cpp_attribute(nodiscard)
#if defined(__clang__) && __cplusplus < 201703L
// clang complains about using nodiscard in C++14 mode.
#define RANGES_NODISCARD                                                                                               \
    RANGES_DIAGNOSTIC_PUSH                                                                                             \
    RANGES_DIAGNOSTIC_IGNORE("-Wc++1z-extensions")                                                                     \
    [[nodiscard]] RANGES_DIAGNOSTIC_POP /**/
#else
#define RANGES_NODISCARD [[nodiscard]]
#endif
#else
#define RANGES_NODISCARD
#endif
#endif

#ifndef RANGES_EMPTY_BASES
#ifdef _MSC_VER
#define RANGES_EMPTY_BASES __declspec(empty_bases)
#else
#define RANGES_EMPTY_BASES
#endif
#endif

#endif

// #include <futures/algorithm/detail/traits/range/utility/static_const.h>
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

#ifndef FUTURES_RANGES_UTILITY_STATIC_CONST_HPP
#define FUTURES_RANGES_UTILITY_STATIC_CONST_HPP

namespace futures::detail {
    /// \ingroup group-utility

    template <typename T> struct static_const { static constexpr T value{}; };

    /// \ingroup group-utility
    /// \sa `static_const`
    template <typename T> constexpr T static_const<T>::value;
} // namespace futures::detail

#endif

// #include <futures/algorithm/detail/traits/range/version.h>
/// \file
// Range v3 library
//
//  Copyright Eric Niebler 2017-present
//
//  Use, modification and distribution is subject to the
//  Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
// Project home: https://github.com/ericniebler/range-v3
//

#ifndef FUTURES_RANGES_VERSION_HPP
#define FUTURES_RANGES_VERSION_HPP

#define RANGE_V3_MAJOR 0
#define RANGE_V3_MINOR 11
#define RANGE_V3_PATCHLEVEL 0

#define RANGE_V3_VERSION (RANGE_V3_MAJOR * 10000 + RANGE_V3_MINOR * 100 + RANGE_V3_PATCHLEVEL)

#endif


/// \defgroup group-iterator Iterator
/// Iterator functionality

/// \defgroup group-iterator-concepts Iterator Concepts
/// \ingroup group-iterator
/// Iterator concepts

/// \defgroup group-range Range
/// Core range functionality

/// \defgroup group-range-concepts Range Concepts
/// \ingroup group-range
/// Range concepts

/// \defgroup group-algorithms Algorithms
/// Iterator- and range-based algorithms, like the standard algorithms

/// \defgroup group-views Views
/// Lazy, non-owning, non-mutating, composable range views

/// \defgroup group-actions Actions
/// Eager, mutating, composable algorithms

/// \defgroup group-utility Utility
/// Utility classes

/// \defgroup group-functional Functional
/// Function and function object utilities

/// \defgroup group-numerics Numerics
/// Numeric utilities

// #include <futures/algorithm/detail/traits/range/detail/prologue.h>
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

#ifndef FUTURES_RANGES_DETAIL_PROLOGUE_HPP
#define FUTURES_RANGES_DETAIL_PROLOGUE_HPP
// #include <futures/algorithm/detail/traits/range/detail/config.h>

#endif

#ifdef RANGES_PROLOGUE_INCLUDED
#error "Prologue already included!"
#endif
#define RANGES_PROLOGUE_INCLUDED

RANGES_DIAGNOSTIC_PUSH

#ifdef RANGES_FEWER_WARNINGS
RANGES_DIAGNOSTIC_IGNORE_UNDEFINED_INTERNAL
RANGES_DIAGNOSTIC_IGNORE_INDENTATION
RANGES_DIAGNOSTIC_IGNORE_CXX17_COMPAT
#endif

RANGES_DIAGNOSTIC_KEYWORD_MACRO

#define template(...)                                                                                                  \
    CPP_PP_IGNORE_CXX2A_COMPAT_BEGIN                                                                                   \
    template <__VA_ARGS__ CPP_TEMPLATE_AUX_

#define AND CPP_and


RANGES_DIAGNOSTIC_PUSH
RANGES_DIAGNOSTIC_IGNORE_CXX17_COMPAT

namespace futures::detail {
    /// \cond
    namespace views {}

    namespace actions {}

// GCC either fails to accept an attribute on a namespace, or else
// it ignores the deprecation attribute. Frustrating.
#if (RANGES_CXX_VER < RANGES_CXX_STD_17 || defined(__GNUC__) && !defined(__clang__))
    inline namespace v3 {
        using namespace futures::detail;
    }

    namespace view = views;
    namespace action = actions;
#else
    inline namespace RANGES_DEPRECATED("The name futures::detail::v3 namespace is deprecated. "
                                       "Please discontinue using it.") v3 {
        using namespace futures::detail;
    }

    namespace RANGES_DEPRECATED("The futures::detail::view namespace has been renamed to futures::detail::views. "
                                "(Sorry!)") view {
        using namespace views;
    }

    namespace RANGES_DEPRECATED("The futures::detail::action namespace has been renamed to futures::detail::actions. "
                                "(Sorry!)") action {
        using namespace actions;
    }
#endif

    namespace _end_ {
        struct fn;
    }
    using end_fn = _end_::fn;

    namespace _size_ {
        struct fn;
    }

    template <typename> struct result_of;

    template <typename Sig>
    using result_of_t RANGES_DEPRECATED("futures::detail::result_of_t is deprecated. "
                                        "Please use futures::detail::invoke_result_t") = futures::detail::meta::_t<result_of<Sig>>;
    /// \endcond

    template <typename...> struct variant;

    struct dangling;

    struct make_pipeable_fn;

    struct pipeable_base;

    template <typename First, typename Second> struct composed;

    template <typename... Fns> struct overloaded;

    namespace actions {
        template <typename ActionFn> struct action_closure;
    }

    namespace views {
        template <typename ViewFn> struct view_closure;
    }

    struct advance_fn;

    struct advance_to_fn;

    struct advance_bounded_fn;

    struct next_fn;

    struct prev_fn;

    struct distance_fn;

    struct iter_size_fn;

    template <typename T> struct indirectly_readable_traits;

    template <typename T>
    using readable_traits
        RANGES_DEPRECATED("Please use futures::detail::indirectly_readable_traits") = indirectly_readable_traits<T>;

    template <typename T> struct incrementable_traits;

    struct view_base {};

    /// \cond
    namespace ranges_detail {
        template <typename T> struct difference_type_;

        template <typename T> struct value_type_;
    } // namespace ranges_detail

    template <typename T>
    using difference_type
        RANGES_DEPRECATED("futures::detail::difference_type<T>::type is deprecated. Use "
                          "futures::detail::incrementable_traits<T>::difference_type instead.") = ranges_detail::difference_type_<T>;

    template <typename T>
    using value_type
        RANGES_DEPRECATED("futures::detail::value_type<T>::type is deprecated. Use "
                          "futures::detail::indirectly_readable_traits<T>::value_type instead.") = ranges_detail::value_type_<T>;

    template <typename T> struct size_type;
    /// \endcond

    /// \cond
    namespace ranges_detail {
        struct ignore_t {
            ignore_t() = default;
            template <typename T> constexpr ignore_t(T &&) noexcept {}
            template <typename T> constexpr ignore_t const &operator=(T &&) const noexcept { return *this; }
        };

        struct value_init {
            template <typename T> operator T() const { return T{}; }
        };

        struct make_compressed_pair_fn;

        template <typename T> constexpr futures::detail::meta::_t<std::remove_reference<T>> &&move(T &&t) noexcept {
            return static_cast<futures::detail::meta::_t<std::remove_reference<T>> &&>(t);
        }

        struct as_const_fn {
            template <typename T> constexpr T const &operator()(T &t) const noexcept { return t; }
            template <typename T> constexpr T const &&operator()(T &&t) const noexcept { return (T &&) t; }
        };

        RANGES_INLINE_VARIABLE(as_const_fn, as_const)

        template <typename T> using as_const_t = decltype(as_const(std::declval<T>()));

        template <typename T> using decay_t = futures::detail::meta::_t<std::decay<T>>;

        template <typename T, typename R = futures::detail::meta::_t<std::remove_reference<T>>>
        using as_ref_t = futures::detail::meta::_t<std::add_lvalue_reference<futures::detail::meta::_t<std::remove_const<R>>>>;

        template <typename T, typename R = futures::detail::meta::_t<std::remove_reference<T>>>
        using as_cref_t = futures::detail::meta::_t<std::add_lvalue_reference<R const>>;

        struct get_first;
        struct get_second;

        template <typename Val1, typename Val2> struct replacer_fn;

        template <typename Pred, typename Val> struct replacer_if_fn;

        template <typename I> struct move_into_cursor;

        template <typename Int> struct from_end_;

        template <typename... Ts> constexpr int ignore_unused(Ts &&...) { return 42; }

        template <int I> struct priority_tag : priority_tag<I - 1> {};

        template <> struct priority_tag<0> {};

#if defined(__clang__) && !defined(_LIBCPP_VERSION)
        template <typename T, typename... Args>
        RANGES_INLINE_VAR constexpr bool is_trivially_constructible_v = __is_trivially_constructible(T, Args...);
        template <typename T>
        RANGES_INLINE_VAR constexpr bool is_trivially_default_constructible_v = is_trivially_constructible_v<T>;
        template <typename T>
        RANGES_INLINE_VAR constexpr bool is_trivially_copy_constructible_v = is_trivially_constructible_v<T, T const &>;
        template <typename T>
        RANGES_INLINE_VAR constexpr bool is_trivially_move_constructible_v = is_trivially_constructible_v<T, T>;
        template <typename T> RANGES_INLINE_VAR constexpr bool is_trivially_copyable_v = __is_trivially_copyable(T);
        template <typename T, typename U>
        RANGES_INLINE_VAR constexpr bool is_trivially_assignable_v = __is_trivially_assignable(T, U);
        template <typename T>
        RANGES_INLINE_VAR constexpr bool is_trivially_copy_assignable_v = is_trivially_assignable_v<T &, T const &>;
        template <typename T>
        RANGES_INLINE_VAR constexpr bool is_trivially_move_assignable_v = is_trivially_assignable_v<T &, T>;

        template <typename T, typename... Args>
        struct is_trivially_constructible : futures::detail::meta::bool_<is_trivially_constructible_v<T, Args...>> {};
        template <typename T>
        struct is_trivially_default_constructible : futures::detail::meta::bool_<is_trivially_default_constructible_v<T>> {};
        template <typename T>
        struct is_trivially_copy_constructible : futures::detail::meta::bool_<is_trivially_copy_constructible_v<T>> {};
        template <typename T>
        struct is_trivially_move_constructible : futures::detail::meta::bool_<is_trivially_move_constructible_v<T>> {};
        template <typename T> struct is_trivially_copyable : futures::detail::meta::bool_<is_trivially_copyable_v<T>> {};
        template <typename T, typename U>
        struct is_trivially_assignable : futures::detail::meta::bool_<is_trivially_assignable_v<T, U>> {};
        template <typename T> struct is_trivially_copy_assignable : futures::detail::meta::bool_<is_trivially_copy_assignable_v<T>> {};
        template <typename T> struct is_trivially_move_assignable : futures::detail::meta::bool_<is_trivially_move_assignable_v<T>> {};
#else
        using std::is_trivially_assignable;
        using std::is_trivially_constructible;
        using std::is_trivially_copy_assignable;
        using std::is_trivially_copy_constructible;
        using std::is_trivially_copyable;
        using std::is_trivially_default_constructible;
        using std::is_trivially_move_assignable;
        using std::is_trivially_move_constructible;
#if META_CXX_TRAIT_VARIABLE_TEMPLATES
        using std::is_trivially_assignable_v;
        using std::is_trivially_constructible_v;
        using std::is_trivially_copy_assignable_v;
        using std::is_trivially_copy_constructible_v;
        using std::is_trivially_copyable_v;
        using std::is_trivially_default_constructible_v;
        using std::is_trivially_move_assignable_v;
        using std::is_trivially_move_constructible_v;
#else
        template <typename T, typename... Args>
        RANGES_INLINE_VAR constexpr bool is_trivially_constructible_v = is_trivially_constructible<T, Args...>::value;
        template <typename T>
        RANGES_INLINE_VAR constexpr bool is_trivially_default_constructible_v =
            is_trivially_default_constructible<T>::value;
        template <typename T>
        RANGES_INLINE_VAR constexpr bool is_trivially_copy_constructible_v = is_trivially_copy_constructible<T>::value;
        template <typename T>
        RANGES_INLINE_VAR constexpr bool is_trivially_move_constructible_v = is_trivially_move_constructible<T>::value;
        template <typename T>
        RANGES_INLINE_VAR constexpr bool is_trivially_copyable_v = is_trivially_copyable<T>::value;
        template <typename T, typename U>
        RANGES_INLINE_VAR constexpr bool is_trivially_assignable_v = is_trivially_assignable<T, U>::value;
        template <typename T>
        RANGES_INLINE_VAR constexpr bool is_trivially_copy_assignable_v = is_trivially_copy_assignable<T>::value;
        template <typename T>
        RANGES_INLINE_VAR constexpr bool is_trivially_move_assignable_v = is_trivially_move_assignable<T>::value;
#endif
#endif

        template <typename T>
        RANGES_INLINE_VAR constexpr bool is_trivial_v =
            is_trivially_copyable_v<T> &&is_trivially_default_constructible_v<T>;

        template <typename T> struct is_trivial : futures::detail::meta::bool_<is_trivial_v<T>> {};

#if RANGES_CXX_LIB_IS_FINAL > 0
#if defined(__clang__) && !defined(_LIBCPP_VERSION)
        template <typename T> RANGES_INLINE_VAR constexpr bool is_final_v = __is_final(T);

        template <typename T> struct is_final : futures::detail::meta::bool_<is_final_v<T>> {};
#else
        using std::is_final;
#if META_CXX_TRAIT_VARIABLE_TEMPLATES
        using std::is_final_v;
#else
        template <typename T> RANGES_INLINE_VAR constexpr bool is_final_v = is_final<T>::value;
#endif
#endif
#else
        template <typename T> RANGES_INLINE_VAR constexpr bool is_final_v = false;

        template <typename T> using is_final = std::false_type;
#endif

        // Work around libc++'s buggy std::is_function
        // Function types here:
        template <typename T> char (&is_function_impl_(priority_tag<0>))[1];

        // Array types here:
        template <typename T, typename = decltype((*(T *)0)[0])> char (&is_function_impl_(priority_tag<1>))[2];

        // Anything that can be returned from a function here (including
        // void and reference types):
        template <typename T, typename = T (*)()> char (&is_function_impl_(priority_tag<2>))[3];

        // Classes and unions (including abstract types) here:
        template <typename T, typename = int T::*> char (&is_function_impl_(priority_tag<3>))[4];

        template <typename T>
        RANGES_INLINE_VAR constexpr bool is_function_v = sizeof(ranges_detail::is_function_impl_<T>(priority_tag<3>{})) == 1;

        template <typename T> struct remove_rvalue_reference { using type = T; };

        template <typename T> struct remove_rvalue_reference<T &&> { using type = T; };

        template <typename T> using remove_rvalue_reference_t = futures::detail::meta::_t<remove_rvalue_reference<T>>;

        // Workaround bug in the Standard Library:
        // From cannot be an incomplete class type despite that
        // is_convertible<X, Y> should be equivalent to is_convertible<X&&, Y>
        // in such a case.
        template <typename From, typename To>
        using is_convertible = std::is_convertible<futures::detail::meta::_t<std::add_rvalue_reference<From>>, To>;
    } // namespace ranges_detail
    /// \endcond

    struct begin_tag {};
    struct end_tag {};
    struct copy_tag {};
    struct move_tag {};

    template <typename T> using uncvref_t = futures::detail::meta::_t<std::remove_cv<futures::detail::meta::_t<std::remove_reference<T>>>>;

    struct not_equal_to;
    struct equal_to;
    struct less;
#if __cplusplus > 201703L && defined(__cpp_impl_three_way_comparison) && __has_include(<compare>)
    struct compare_three_way;
#endif // __cplusplus
    struct identity;
    template <typename Pred> struct logical_negate;

    enum cardinality : std::ptrdiff_t { infinite = -3, unknown = -2, finite = -1 };

    template <typename Rng, typename Void = void> struct range_cardinality;

    template <typename Rng> using is_finite = futures::detail::meta::bool_<range_cardinality<Rng>::value >= finite>;

    template <typename Rng> using is_infinite = futures::detail::meta::bool_<range_cardinality<Rng>::value == infinite>;

    template <typename S, typename I> RANGES_INLINE_VAR constexpr bool disable_sized_sentinel = false;

    template <typename R> RANGES_INLINE_VAR constexpr bool enable_borrowed_range = false;

    namespace ranges_detail {
        template <typename R>
        RANGES_DEPRECATED("Please use futures::detail::enable_borrowed_range instead.")
        RANGES_INLINE_VAR constexpr bool enable_safe_range = enable_borrowed_range<R>;
    } // namespace ranges_detail

    using ranges_detail::enable_safe_range;

    template <typename Cur> struct basic_mixin;

    template <typename Cur> struct RANGES_EMPTY_BASES basic_iterator;

    template <cardinality> struct basic_view : view_base {};

    template <typename Derived, cardinality C = finite> struct view_facade;

    template <typename Derived, typename BaseRng, cardinality C = range_cardinality<BaseRng>::value>
    struct view_adaptor;

    template <typename I, typename S> struct common_iterator;

    /// \cond
    namespace ranges_detail {
        template <typename I> struct cpp17_iterator_cursor;

        template <typename I> using cpp17_iterator = basic_iterator<cpp17_iterator_cursor<I>>;
    } // namespace ranges_detail
    /// \endcond

    template <typename First, typename Second> struct compressed_pair;

    template <typename T> struct bind_element;

    template <typename T> using bind_element_t = futures::detail::meta::_t<bind_element<T>>;

    template <typename Derived, cardinality = finite> struct view_interface;

    template <typename T> struct istream_view;

    template <typename I, typename S = I> struct RANGES_EMPTY_BASES iterator_range;

    template <typename I, typename S = I> struct sized_iterator_range;

    template <typename T> struct reference_wrapper;

    // Views
    //
    template <typename Rng, typename Pred> struct RANGES_EMPTY_BASES adjacent_filter_view;

    namespace views {
        struct adjacent_filter_fn;
    }

    template <typename Rng, typename Pred> struct RANGES_EMPTY_BASES adjacent_remove_if_view;

    namespace views {
        struct adjacent_remove_if_fn;
    }

    namespace views {
        struct all_fn;
    }

    template <typename Rng> struct const_view;

    namespace views {
        struct const_fn;
    }

    template <typename I> struct counted_view;

    namespace views {
        struct counted_fn;
    }

    struct default_sentinel_t;

    template <typename I> struct move_iterator;

    template <typename I> using move_into_iterator = basic_iterator<ranges_detail::move_into_cursor<I>>;

    template <typename Rng, bool = (bool)is_infinite<Rng>()> struct RANGES_EMPTY_BASES cycled_view;

    namespace views {
        struct cycle_fn;
    }

    /// \cond
    namespace ranges_detail {
        template <typename I> struct reverse_cursor;
    }
    /// \endcond

    template <typename I> using reverse_iterator = basic_iterator<ranges_detail::reverse_cursor<I>>;

    template <typename T> struct empty_view;

    namespace views {
        struct empty_fn;
    }

    template <typename Rng, typename Fun> struct group_by_view;

    namespace views {
        struct group_by_fn;
    }

    template <typename Rng> struct indirect_view;

    namespace views {
        struct indirect_fn;
    }

    struct unreachable_sentinel_t;

    template <typename From, typename To = unreachable_sentinel_t> struct iota_view;

    template <typename From, typename To = From> struct closed_iota_view;

    namespace views {
        struct iota_fn;
        struct closed_iota_fn;
    } // namespace views

    template <typename Rng> struct join_view;

    template <typename Rng, typename ValRng> struct join_with_view;

    namespace views {
        struct join_fn;
    }

    template <typename... Rngs> struct concat_view;

    namespace views {
        struct concat_fn;
    }

    template <typename Rng, typename Fun> struct partial_sum_view;

    namespace views {
        struct partial_sum_fn;
    }

    template <typename Rng> struct move_view;

    namespace views {
        struct move_fn;
    }

    template <typename Rng> struct ref_view;

    namespace views {
        struct ref_fn;
    }

    template <typename Val> struct repeat_view;

    namespace views {
        struct repeat_fn;
    }

    template <typename Rng> struct RANGES_EMPTY_BASES reverse_view;

    namespace views {
        struct reverse_fn;
    }

    template <typename Rng> struct slice_view;

    namespace views {
        struct slice_fn;
    }

    // template<typename Rng, typename Fun>
    // struct split_view;

    // namespace views
    // {
    //     struct split_fn;
    // }

    template <typename Rng> struct single_view;

    namespace views {
        struct single_fn;
    }

    template <typename Rng> struct stride_view;

    namespace views {
        struct stride_fn;
    }

    template <typename Rng> struct take_view;

    namespace views {
        struct take_fn;
    }

    /// \cond
    namespace ranges_detail {
        template <typename Rng> struct is_random_access_common_;

        template <typename Rng, bool IsRandomAccessCommon = is_random_access_common_<Rng>::value>
        struct take_exactly_view_;
    } // namespace ranges_detail
    /// \endcond

    template <typename Rng> using take_exactly_view = ranges_detail::take_exactly_view_<Rng>;

    namespace views {
        struct take_exactly_fn;
    }

    template <typename Rng, typename Pred> struct iter_take_while_view;

    template <typename Rng, typename Pred> struct take_while_view;

    namespace views {
        struct iter_take_while_fn;
        struct take_while_fn;
    } // namespace views

    template <typename Rng, typename Regex, typename SubMatchRange> struct tokenize_view;

    namespace views {
        struct tokenize_fn;
    }

    template <typename Rng, typename Fun> struct iter_transform_view;

    template <typename Rng, typename Fun> struct transform_view;

    namespace views {
        struct transform_fn;
    }

    template <typename Rng, typename Val1, typename Val2>
    using replace_view = iter_transform_view<Rng, ranges_detail::replacer_fn<Val1, Val2>>;

    template <typename Rng, typename Pred, typename Val>
    using replace_if_view = iter_transform_view<Rng, ranges_detail::replacer_if_fn<Pred, Val>>;

    namespace views {
        struct replace_fn;

        struct replace_if_fn;
    } // namespace views

    template <typename Rng, typename Pred> struct trim_view;

    namespace views {
        struct trim_fn;
    }

    template <typename I> struct unbounded_view;

    namespace views {
        struct unbounded_fn;
    }

    template <typename Rng> using unique_view = adjacent_filter_view<Rng, logical_negate<equal_to>>;

    namespace views {
        struct unique_fn;
    }

    template <typename Rng> using keys_range_view = transform_view<Rng, ranges_detail::get_first>;

    template <typename Rng> using values_view = transform_view<Rng, ranges_detail::get_second>;

    namespace views {
        struct keys_fn;

        struct values_fn;
    } // namespace views

    template <typename Fun, typename... Rngs> struct iter_zip_with_view;

    template <typename Fun, typename... Rngs> struct zip_with_view;

    template <typename... Rngs> struct zip_view;

    namespace views {
        struct iter_zip_with_fn;

        struct zip_with_fn;

        struct zip_fn;
    } // namespace views
} // namespace futures::detail

/// \cond
namespace futures::detail {
    // namespace futures::detail::concepts = ::futures::detail::concepts;
    using namespace ::futures::detail::concepts::defs;
    using ::futures::detail::concepts::and_v;
} // namespace futures::detail
/// \endcond

RANGES_DIAGNOSTIC_POP

// #include <futures/algorithm/detail/traits/range/detail/epilogue.h>
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

#ifndef RANGES_PROLOGUE_INCLUDED
#error "Including epilogue, but prologue not included!"
#endif
#undef RANGES_PROLOGUE_INCLUDED

#undef template
#undef AND

RANGES_DIAGNOSTIC_POP


#endif


// #include <futures/algorithm/detail/traits/range/functional/comparisons.h>
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

// #include <futures/algorithm/detail/traits/range/concepts/concepts.h>


// #include <futures/algorithm/detail/traits/range/range_fwd.h>


// #include <futures/algorithm/detail/traits/range/detail/prologue.h>
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

#ifndef FUTURES_RANGES_DETAIL_PROLOGUE_HPP
#define FUTURES_RANGES_DETAIL_PROLOGUE_HPP
// #include <futures/algorithm/detail/traits/range/detail/config.h>

#endif

#ifdef RANGES_PROLOGUE_INCLUDED
#error "Prologue already included!"
#endif
#define RANGES_PROLOGUE_INCLUDED

RANGES_DIAGNOSTIC_PUSH

#ifdef RANGES_FEWER_WARNINGS
RANGES_DIAGNOSTIC_IGNORE_UNDEFINED_INTERNAL
RANGES_DIAGNOSTIC_IGNORE_INDENTATION
RANGES_DIAGNOSTIC_IGNORE_CXX17_COMPAT
#endif

RANGES_DIAGNOSTIC_KEYWORD_MACRO

#define template(...)                                                                                                  \
    CPP_PP_IGNORE_CXX2A_COMPAT_BEGIN                                                                                   \
    template <__VA_ARGS__ CPP_TEMPLATE_AUX_

#define AND CPP_and


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

// #include <futures/algorithm/detail/traits/range/detail/epilogue.h>
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

#ifndef RANGES_PROLOGUE_INCLUDED
#error "Including epilogue, but prologue not included!"
#endif
#undef RANGES_PROLOGUE_INCLUDED

#undef template
#undef AND

RANGES_DIAGNOSTIC_POP


#endif

// #include <futures/algorithm/detail/traits/range/iterator/concepts.h>
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

#ifndef FUTURES_RANGES_ITERATOR_CONCEPTS_HPP
#define FUTURES_RANGES_ITERATOR_CONCEPTS_HPP

#include <iterator>
// #include <type_traits>


// #include <futures/algorithm/detail/traits/range/meta/meta.h>


// #include <futures/algorithm/detail/traits/range/concepts/concepts.h>


// #include <futures/algorithm/detail/traits/range/range_fwd.h>


// #include <futures/algorithm/detail/traits/range/functional/comparisons.h>

// #include <futures/algorithm/detail/traits/range/functional/concepts.h>

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
#ifndef FUTURES_RANGES_FUNCTIONAL_CONCEPTS_HPP
#define FUTURES_RANGES_FUNCTIONAL_CONCEPTS_HPP

// #include <futures/algorithm/detail/traits/range/concepts/concepts.h>


// #include <futures/algorithm/detail/traits/range/functional/invoke.h>
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
#ifndef FUTURES_RANGES_FUNCTIONAL_INVOKE_HPP
#define FUTURES_RANGES_FUNCTIONAL_INVOKE_HPP

// #include <functional>

// #include <type_traits>


// #include <futures/algorithm/detail/traits/range/meta/meta.h>


// #include <futures/algorithm/detail/traits/range/concepts/concepts.h>


// #include <futures/algorithm/detail/traits/range/range_fwd.h>


// #include <futures/algorithm/detail/traits/range/utility/static_const.h>


// #include <futures/algorithm/detail/traits/range/detail/prologue.h>
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

#ifndef FUTURES_RANGES_DETAIL_PROLOGUE_HPP
#define FUTURES_RANGES_DETAIL_PROLOGUE_HPP
// #include <futures/algorithm/detail/traits/range/detail/config.h>

#endif

#ifdef RANGES_PROLOGUE_INCLUDED
#error "Prologue already included!"
#endif
#define RANGES_PROLOGUE_INCLUDED

RANGES_DIAGNOSTIC_PUSH

#ifdef RANGES_FEWER_WARNINGS
RANGES_DIAGNOSTIC_IGNORE_UNDEFINED_INTERNAL
RANGES_DIAGNOSTIC_IGNORE_INDENTATION
RANGES_DIAGNOSTIC_IGNORE_CXX17_COMPAT
#endif

RANGES_DIAGNOSTIC_KEYWORD_MACRO

#define template(...)                                                                                                  \
    CPP_PP_IGNORE_CXX2A_COMPAT_BEGIN                                                                                   \
    template <__VA_ARGS__ CPP_TEMPLATE_AUX_

#define AND CPP_and


RANGES_DIAGNOSTIC_PUSH
RANGES_DIAGNOSTIC_IGNORE_CXX17_COMPAT
RANGES_DIAGNOSTIC_IGNORE_DEPRECATED_DECLARATIONS

#ifndef RANGES_CONSTEXPR_INVOKE
#ifdef RANGES_WORKAROUND_CLANG_23135
#define RANGES_CONSTEXPR_INVOKE 0
#else
#define RANGES_CONSTEXPR_INVOKE 1
#endif
#endif

namespace futures::detail {
    /// \addtogroup group-functional
    /// @{

    /// \cond
    namespace ranges_detail {
        RANGES_DIAGNOSTIC_PUSH
        RANGES_DIAGNOSTIC_IGNORE_VOID_PTR_DEREFERENCE

        template <typename U> U &can_reference_(U &&);

        // clang-format off
        template<typename T>
        CPP_requires(dereferenceable_part_,
            requires(T && t) //
            (
                ranges_detail::can_reference_(*(T &&) t)
            ));
        template<typename T>
        CPP_concept dereferenceable_ = //
            CPP_requires_ref(ranges_detail::dereferenceable_part_, T);
        // clang-format on

        RANGES_DIAGNOSTIC_POP

        template <typename T>
        RANGES_INLINE_VAR constexpr bool is_reference_wrapper_v =
            futures::detail::meta::is<T, reference_wrapper>::value || futures::detail::meta::is<T, std::reference_wrapper>::value;
    } // namespace ranges_detail
    /// \endcond

    template <typename T>
    RANGES_INLINE_VAR constexpr bool is_reference_wrapper_v = ranges_detail::is_reference_wrapper_v<ranges_detail::decay_t<T>>;

    template <typename T> using is_reference_wrapper = futures::detail::meta::bool_<is_reference_wrapper_v<T>>;

    /// \cond
    template <typename T>
    using is_reference_wrapper_t
        RANGES_DEPRECATED("is_reference_wrapper_t is deprecated.") = futures::detail::meta::_t<is_reference_wrapper<T>>;
    /// \endcond

    struct invoke_fn {
      private:
        template(typename, typename T1)(
            /// \pre
            requires ranges_detail::dereferenceable_<T1>) static constexpr decltype(auto)
            coerce(T1 &&t1, long) noexcept(noexcept(*static_cast<T1 &&>(t1))) {
            return *static_cast<T1 &&>(t1);
        }

        template(typename T, typename T1)(
            /// \pre
            requires derived_from<ranges_detail::decay_t<T1>, T>) static constexpr T1 &&coerce(T1 &&t1, int) noexcept {
            return static_cast<T1 &&>(t1);
        }

        template(typename, typename T1)(
            /// \pre
            requires ranges_detail::is_reference_wrapper_v<ranges_detail::decay_t<T1>>) static constexpr decltype(auto)
            coerce(T1 &&t1, int) noexcept {
            return static_cast<T1 &&>(t1).get();
        }

      public:
        template <typename F, typename T, typename T1, typename... Args>
        constexpr auto operator()(F T::*f, T1 &&t1, Args &&...args) const
            noexcept(noexcept((invoke_fn::coerce<T>((T1 &&) t1, 0).*f)((Args &&) args...)))
                -> decltype((invoke_fn::coerce<T>((T1 &&) t1, 0).*f)((Args &&) args...)) {
            return (invoke_fn::coerce<T>((T1 &&) t1, 0).*f)((Args &&) args...);
        }

        template <typename D, typename T, typename T1>
        constexpr auto operator()(D T::*f, T1 &&t1) const noexcept(noexcept(invoke_fn::coerce<T>((T1 &&) t1, 0).*f))
            -> decltype(invoke_fn::coerce<T>((T1 &&) t1, 0).*f) {
            return invoke_fn::coerce<T>((T1 &&) t1, 0).*f;
        }

        template <typename F, typename... Args>
        CPP_PP_IIF(RANGES_CONSTEXPR_INVOKE)
        (CPP_PP_EXPAND, CPP_PP_EAT)(constexpr) auto operator()(F &&f, Args &&...args) const
            noexcept(noexcept(((F &&) f)((Args &&) args...))) -> decltype(((F &&) f)((Args &&) args...)) {
            return ((F &&) f)((Args &&) args...);
        }
    };

    RANGES_INLINE_VARIABLE(invoke_fn, invoke)

#ifdef RANGES_WORKAROUND_MSVC_701385
    /// \cond
    namespace ranges_detail {
        template <typename Void, typename Fun, typename... Args> struct _invoke_result_ {};

        template <typename Fun, typename... Args>
        struct _invoke_result_<futures::detail::meta::void_<decltype(invoke(std::declval<Fun>(), std::declval<Args>()...))>, Fun,
                               Args...> {
            using type = decltype(invoke(std::declval<Fun>(), std::declval<Args>()...));
        };
    } // namespace ranges_detail
    /// \endcond

    template <typename Fun, typename... Args> using invoke_result = ranges_detail::_invoke_result_<void, Fun, Args...>;

    template <typename Fun, typename... Args> using invoke_result_t = futures::detail::meta::_t<invoke_result<Fun, Args...>>;

#else  // RANGES_WORKAROUND_MSVC_701385
    template <typename Fun, typename... Args>
    using invoke_result_t = decltype(invoke(std::declval<Fun>(), std::declval<Args>()...));

    template <typename Fun, typename... Args> struct invoke_result : futures::detail::meta::defer<invoke_result_t, Fun, Args...> {};
#endif // RANGES_WORKAROUND_MSVC_701385

    /// \cond
    namespace ranges_detail {
        template <bool IsInvocable> struct is_nothrow_invocable_impl_ {
            template <typename Fn, typename... Args> static constexpr bool apply() noexcept { return false; }
        };
        template <> struct is_nothrow_invocable_impl_<true> {
            template <typename Fn, typename... Args> static constexpr bool apply() noexcept {
                return noexcept(invoke(std::declval<Fn>(), std::declval<Args>()...));
            }
        };
    } // namespace ranges_detail
    /// \endcond

    template <typename Fn, typename... Args>
    RANGES_INLINE_VAR constexpr bool is_invocable_v = futures::detail::meta::is_trait<invoke_result<Fn, Args...>>::value;

    template <typename Fn, typename... Args>
    RANGES_INLINE_VAR constexpr bool is_nothrow_invocable_v =
        ranges_detail::is_nothrow_invocable_impl_<is_invocable_v<Fn, Args...>>::template apply<Fn, Args...>();

    /// \cond
    template <typename Sig>
    struct RANGES_DEPRECATED("futures::detail::result_of is deprecated. "
                             "Please use futures::detail::invoke_result") result_of {};

    template <typename Fun, typename... Args>
    struct RANGES_DEPRECATED("futures::detail::result_of is deprecated. "
                             "Please use futures::detail::invoke_result") result_of<Fun(Args...)>
        : futures::detail::meta::defer<invoke_result_t, Fun, Args...> {};
    /// \endcond

    namespace cpp20 {
        using ::futures::detail::invoke;
        using ::futures::detail::invoke_result;
        using ::futures::detail::invoke_result_t;
        using ::futures::detail::is_invocable_v;
        using ::futures::detail::is_nothrow_invocable_v;
    } // namespace cpp20

    /// @}
} // namespace futures::detail

RANGES_DIAGNOSTIC_POP

// #include <futures/algorithm/detail/traits/range/detail/epilogue.h>
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

#ifndef RANGES_PROLOGUE_INCLUDED
#error "Including epilogue, but prologue not included!"
#endif
#undef RANGES_PROLOGUE_INCLUDED

#undef template
#undef AND

RANGES_DIAGNOSTIC_POP


#endif // FUTURES_RANGES_FUNCTIONAL_INVOKE_HPP


// #include <futures/algorithm/detail/traits/range/detail/prologue.h>
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

#ifndef FUTURES_RANGES_DETAIL_PROLOGUE_HPP
#define FUTURES_RANGES_DETAIL_PROLOGUE_HPP
// #include <futures/algorithm/detail/traits/range/detail/config.h>

#endif

#ifdef RANGES_PROLOGUE_INCLUDED
#error "Prologue already included!"
#endif
#define RANGES_PROLOGUE_INCLUDED

RANGES_DIAGNOSTIC_PUSH

#ifdef RANGES_FEWER_WARNINGS
RANGES_DIAGNOSTIC_IGNORE_UNDEFINED_INTERNAL
RANGES_DIAGNOSTIC_IGNORE_INDENTATION
RANGES_DIAGNOSTIC_IGNORE_CXX17_COMPAT
#endif

RANGES_DIAGNOSTIC_KEYWORD_MACRO

#define template(...)                                                                                                  \
    CPP_PP_IGNORE_CXX2A_COMPAT_BEGIN                                                                                   \
    template <__VA_ARGS__ CPP_TEMPLATE_AUX_

#define AND CPP_and


namespace futures::detail {
    /// \addtogroup group-functional
    /// @{

    // clang-format off
    // WORKAROUND mysterious msvc bug
#if defined(_MSC_VER)
    template<typename Fun, typename... Args>
    CPP_concept invocable =
        std::is_invocable_v<Fun, Args...>;
#else
    template<typename Fun, typename... Args>
    CPP_requires(invocable_,
        requires(Fun && fn) //
        (
            invoke((Fun &&) fn, std::declval<Args>()...)
        ));
    template<typename Fun, typename... Args>
    CPP_concept invocable =
        CPP_requires_ref(::futures::detail::invocable_, Fun, Args...);
#endif

    template<typename Fun, typename... Args>
    CPP_concept regular_invocable =
        invocable<Fun, Args...>;
        // Axiom: equality_preserving(invoke(f, args...))

    template<typename Fun, typename... Args>
    CPP_requires(predicate_,
        requires(Fun && fn) //
        (
            concepts::requires_<
                convertible_to<
                    decltype(invoke((Fun &&) fn, std::declval<Args>()...)),
                    bool>>
        ));
    template<typename Fun, typename... Args>
    CPP_concept predicate =
        regular_invocable<Fun, Args...> &&
        CPP_requires_ref(::futures::detail::predicate_, Fun, Args...);

    template<typename R, typename T, typename U>
    CPP_concept relation =
        predicate<R, T, T> &&
        predicate<R, U, U> &&
        predicate<R, T, U> &&
        predicate<R, U, T>;

    template<typename R, typename T, typename U>
    CPP_concept strict_weak_order =
        relation<R, T, U>;
    // clang-format on

    namespace cpp20 {
        using ::futures::detail::invocable;
        using ::futures::detail::predicate;
        using ::futures::detail::regular_invocable;
        using ::futures::detail::relation;
        using ::futures::detail::strict_weak_order;
    } // namespace cpp20
    /// @}
} // namespace futures::detail

// #include <futures/algorithm/detail/traits/range/detail/epilogue.h>
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

#ifndef RANGES_PROLOGUE_INCLUDED
#error "Including epilogue, but prologue not included!"
#endif
#undef RANGES_PROLOGUE_INCLUDED

#undef template
#undef AND

RANGES_DIAGNOSTIC_POP


#endif

// #include <futures/algorithm/detail/traits/range/functional/identity.h>
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

// #include <futures/algorithm/detail/traits/range/detail/config.h>


// #include <futures/algorithm/detail/traits/range/detail/prologue.h>
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

#ifndef FUTURES_RANGES_DETAIL_PROLOGUE_HPP
#define FUTURES_RANGES_DETAIL_PROLOGUE_HPP
// #include <futures/algorithm/detail/traits/range/detail/config.h>

#endif

#ifdef RANGES_PROLOGUE_INCLUDED
#error "Prologue already included!"
#endif
#define RANGES_PROLOGUE_INCLUDED

RANGES_DIAGNOSTIC_PUSH

#ifdef RANGES_FEWER_WARNINGS
RANGES_DIAGNOSTIC_IGNORE_UNDEFINED_INTERNAL
RANGES_DIAGNOSTIC_IGNORE_INDENTATION
RANGES_DIAGNOSTIC_IGNORE_CXX17_COMPAT
#endif

RANGES_DIAGNOSTIC_KEYWORD_MACRO

#define template(...)                                                                                                  \
    CPP_PP_IGNORE_CXX2A_COMPAT_BEGIN                                                                                   \
    template <__VA_ARGS__ CPP_TEMPLATE_AUX_

#define AND CPP_and


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

// #include <futures/algorithm/detail/traits/range/detail/epilogue.h>
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

#ifndef RANGES_PROLOGUE_INCLUDED
#error "Including epilogue, but prologue not included!"
#endif
#undef RANGES_PROLOGUE_INCLUDED

#undef template
#undef AND

RANGES_DIAGNOSTIC_POP


#endif

// #include <futures/algorithm/detail/traits/range/functional/invoke.h>

// #include <futures/algorithm/detail/traits/range/iterator/access.h>
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

#ifndef FUTURES_RANGES_ITERATOR_ACCESS_HPP
#define FUTURES_RANGES_ITERATOR_ACCESS_HPP

// #include <iterator>

// #include <type_traits>

// #include <utility>


// #include <futures/algorithm/detail/traits/range/std/detail/associated_types.h>
// Range v3 library
//
//  Copyright Eric Niebler 2013-2014, 2016-present
//  Copyright Casey Carter 2016
//
//  Use, modification and distribution is subject to the
//  Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
// Project home: https://github.com/ericniebler/range-v3
//

#ifndef FUTURES_RANGES_STD_DETAIL_ASSOCIATED_TYPES_HPP
#define FUTURES_RANGES_STD_DETAIL_ASSOCIATED_TYPES_HPP

#include <climits>
#include <cstdint>

// #include <futures/algorithm/detail/traits/range/detail/config.h>


// #include <futures/algorithm/detail/traits/range/detail/prologue.h>
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

#ifndef FUTURES_RANGES_DETAIL_PROLOGUE_HPP
#define FUTURES_RANGES_DETAIL_PROLOGUE_HPP
// #include <futures/algorithm/detail/traits/range/detail/config.h>

#endif

#ifdef RANGES_PROLOGUE_INCLUDED
#error "Prologue already included!"
#endif
#define RANGES_PROLOGUE_INCLUDED

RANGES_DIAGNOSTIC_PUSH

#ifdef RANGES_FEWER_WARNINGS
RANGES_DIAGNOSTIC_IGNORE_UNDEFINED_INTERNAL
RANGES_DIAGNOSTIC_IGNORE_INDENTATION
RANGES_DIAGNOSTIC_IGNORE_CXX17_COMPAT
#endif

RANGES_DIAGNOSTIC_KEYWORD_MACRO

#define template(...)                                                                                                  \
    CPP_PP_IGNORE_CXX2A_COMPAT_BEGIN                                                                                   \
    template <__VA_ARGS__ CPP_TEMPLATE_AUX_

#define AND CPP_and


namespace futures::detail {
    /// \addtogroup group-iterator
    /// @{

    ////////////////////////////////////////////////////////////////////////////////////////
    /// \cond
    namespace ranges_detail {
        struct nil_ {};

        template <typename T, typename...> using always_ = T;

#if defined(_MSC_VER) && !defined(__clang__) && !defined(__EDG__)
        // MSVC laughs at your silly micro-optimizations and implements
        // conditional_t, enable_if_t, is_object_v, and is_integral_v in the
        // compiler.
        using std::conditional_t;
        using std::enable_if;
        using std::enable_if_t;
#else // ^^^ MSVC / not MSVC vvv
        template <bool> struct _cond { template <typename, typename U> using invoke = U; };
        template <> struct _cond<true> { template <typename T, typename> using invoke = T; };
        template <bool B, typename T, typename U> using conditional_t = typename _cond<B>::template invoke<T, U>;

        template <bool> struct enable_if {};
        template <> struct enable_if<true> { template <typename T> using invoke = T; };
        template <bool B, typename T = void> using enable_if_t = typename enable_if<B>::template invoke<T>;

#ifndef __clang__
        // NB: insufficient for MSVC, which (non-conformingly) allows function
        // pointers to implicitly convert to void*.
        void is_objptr_(void const volatile *);

        // std::is_object, optimized for compile time.
        template <typename T> constexpr bool is_object_(long) { return false; }
        template <typename T>
        constexpr bool is_object_(int, T *(*q)(T &) = nullptr, T *p = nullptr,
                                  decltype(ranges_detail::is_objptr_(q(*p))) * = nullptr) {
            return (void)p, (void)q, true;
        }
#endif // !__clang__

        template <typename T> constexpr bool is_integral_(...) { return false; }
        template <typename T, T = 1> constexpr bool is_integral_(long) { return true; }
#if defined(__cpp_nontype_template_parameter_class) && __cpp_nontype_template_parameter_class > 0
        template <typename T> constexpr bool is_integral_(int, int T::* = nullptr) { return false; }
#endif
#endif // detect MSVC

        template <typename T> struct with_difference_type_ { using difference_type = T; };

        template <typename T>
        using difference_result_t = decltype(std::declval<T const &>() - std::declval<T const &>());

        template <typename, typename = void> struct incrementable_traits_2_ {};

        template <typename T>
        struct incrementable_traits_2_<T,
#if defined(_MSC_VER) && !defined(__clang__) && !defined(__EDG__)
                                       std::enable_if_t<std::is_integral_v<difference_result_t<T>>>>
#elif defined(RANGES_WORKAROUND_GCC_91923)
                                       std::enable_if_t<std::is_integral<difference_result_t<T>>::value>>
#else
                                       always_<void, int[is_integral_<difference_result_t<T>>(0)]>>
#endif // detect MSVC
        {
            using difference_type = std::make_signed_t<difference_result_t<T>>;
        };

        template <typename T, typename = void> struct incrementable_traits_1_ : incrementable_traits_2_<T> {};

        template <typename T>
        struct incrementable_traits_1_<T *>
#ifdef __clang__
            : conditional_t<__is_object(T), with_difference_type_<std::ptrdiff_t>, nil_>
#elif defined(_MSC_VER) && !defined(__EDG__)
            : conditional_t<std::is_object_v<T>, with_difference_type_<std::ptrdiff_t>, nil_>
#else  // ^^^ MSVC / not MSVC vvv
            : conditional_t<is_object_<T>(0), with_difference_type_<std::ptrdiff_t>, nil_>
#endif // detect MSVC
        {
        };

        template <typename T> struct incrementable_traits_1_<T, always_<void, typename T::difference_type>> {
            using difference_type = typename T::difference_type;
        };
    } // namespace ranges_detail
    /// \endcond

    template <typename T> struct incrementable_traits : ranges_detail::incrementable_traits_1_<T> {};

    template <typename T> struct incrementable_traits<T const> : incrementable_traits<T> {};

    /// \cond
    namespace ranges_detail {
#ifdef __clang__
        template <typename T, bool = __is_object(T)>
#elif defined(_MSC_VER) && !defined(__EDG__)
        template <typename T, bool = std::is_object_v<T>>
#else  // ^^^ MSVC / not MSVC vvv
        template <typename T, bool = is_object_<T>(0)>
#endif // detect MSVC
        struct with_value_type_ {
        };
        template <typename T> struct with_value_type_<T, true> { using value_type = T; };
        template <typename T> struct with_value_type_<T const, true> { using value_type = T; };
        template <typename T> struct with_value_type_<T volatile, true> { using value_type = T; };
        template <typename T> struct with_value_type_<T const volatile, true> { using value_type = T; };
        template <typename, typename = void> struct readable_traits_2_ {};
        template <typename T>
        struct readable_traits_2_<T, always_<void, typename T::element_type>>
            : with_value_type_<typename T::element_type> {};
        template <typename T, typename = void> struct readable_traits_1_ : readable_traits_2_<T> {};
        template <typename T> struct readable_traits_1_<T[]> : with_value_type_<T> {};
        template <typename T, std::size_t N> struct readable_traits_1_<T[N]> : with_value_type_<T> {};
        template <typename T> struct readable_traits_1_<T *> : ranges_detail::with_value_type_<T> {};
        template <typename T>
        struct readable_traits_1_<T, always_<void, typename T::value_type>> : with_value_type_<typename T::value_type> {
        };
    } // namespace ranges_detail
    /// \endcond

    ////////////////////////////////////////////////////////////////////////////////////////
    // Not to spec:
    // * For class types with both member value_type and element_type, value_type is
    //   preferred (see ericniebler/stl2#299).
    template <typename T> struct indirectly_readable_traits : ranges_detail::readable_traits_1_<T> {};

    template <typename T> struct indirectly_readable_traits<T const> : indirectly_readable_traits<T> {};

    /// \cond
    namespace ranges_detail {
        template <typename D = std::ptrdiff_t> struct std_output_iterator_traits {
            using iterator_category = std::output_iterator_tag;
            using difference_type = D;
            using value_type = void;
            using reference = void;
            using pointer = void;
        };

        // For testing whether a particular instantiation of std::iterator_traits
        // is user-specified or not.
#if defined(_MSVC_STL_UPDATE) && defined(__cpp_lib_concepts) && _MSVC_STL_UPDATE >= 201908L
        template <typename I>
        inline constexpr bool is_std_iterator_traits_specialized_v = !std::_Is_from_primary<std::iterator_traits<I>>;
#else
#if defined(__GLIBCXX__)
        template <typename I> char (&is_std_iterator_traits_specialized_impl_(std::__iterator_traits<I> *))[2];
        template <typename I> char is_std_iterator_traits_specialized_impl_(void *);
#elif defined(_LIBCPP_VERSION)
        template <typename I, bool B>
        char (&is_std_iterator_traits_specialized_impl_(std::__iterator_traits<I, B> *))[2];
        template <typename I> char is_std_iterator_traits_specialized_impl_(void *);
#elif defined(_MSVC_STL_VERSION)
        template <typename I> char (&is_std_iterator_traits_specialized_impl_(std::_Iterator_traits_base<I> *))[2];
        template <typename I> char is_std_iterator_traits_specialized_impl_(void *);
#else
        template <typename I> char (&is_std_iterator_traits_specialized_impl_(void *))[2];
#endif
        template <typename, typename T>
        char (&is_std_iterator_traits_specialized_impl_(std::iterator_traits<T *> *))[2];

        template <typename I>
        RANGES_INLINE_VAR constexpr bool is_std_iterator_traits_specialized_v =
            1 == sizeof(is_std_iterator_traits_specialized_impl_<I>(static_cast<std::iterator_traits<I> *>(nullptr)));

        // The standard iterator_traits<T *> specialization(s) do not count
        // as user-specialized. This will no longer be necessary in C++20.
        // This helps with `T volatile*` and `void *`.
        template <typename T> RANGES_INLINE_VAR constexpr bool is_std_iterator_traits_specialized_v<T *> = false;
#endif
    } // namespace ranges_detail
    /// \endcond
} // namespace futures::detail

// #include <futures/algorithm/detail/traits/range/detail/epilogue.h>
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

#ifndef RANGES_PROLOGUE_INCLUDED
#error "Including epilogue, but prologue not included!"
#endif
#undef RANGES_PROLOGUE_INCLUDED

#undef template
#undef AND

RANGES_DIAGNOSTIC_POP


#endif


// #include <futures/algorithm/detail/traits/range/meta/meta.h>


// #include <futures/algorithm/detail/traits/range/concepts/concepts.h>


// #include <futures/algorithm/detail/traits/range/range_fwd.h>


// #include <futures/algorithm/detail/traits/range/utility/move.h>
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

#ifndef FUTURES_RANGES_UTILITY_MOVE_HPP
#define FUTURES_RANGES_UTILITY_MOVE_HPP

// #include <type_traits>


// #include <futures/algorithm/detail/traits/range/meta/meta.h>


// #include <futures/algorithm/detail/traits/range/range_fwd.h>


// #include <futures/algorithm/detail/traits/range/utility/static_const.h>


// #include <futures/algorithm/detail/traits/range/detail/prologue.h>
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

#ifndef FUTURES_RANGES_DETAIL_PROLOGUE_HPP
#define FUTURES_RANGES_DETAIL_PROLOGUE_HPP
// #include <futures/algorithm/detail/traits/range/detail/config.h>

#endif

#ifdef RANGES_PROLOGUE_INCLUDED
#error "Prologue already included!"
#endif
#define RANGES_PROLOGUE_INCLUDED

RANGES_DIAGNOSTIC_PUSH

#ifdef RANGES_FEWER_WARNINGS
RANGES_DIAGNOSTIC_IGNORE_UNDEFINED_INTERNAL
RANGES_DIAGNOSTIC_IGNORE_INDENTATION
RANGES_DIAGNOSTIC_IGNORE_CXX17_COMPAT
#endif

RANGES_DIAGNOSTIC_KEYWORD_MACRO

#define template(...)                                                                                                  \
    CPP_PP_IGNORE_CXX2A_COMPAT_BEGIN                                                                                   \
    template <__VA_ARGS__ CPP_TEMPLATE_AUX_

#define AND CPP_and


namespace futures::detail {
    namespace aux {
        /// \ingroup group-utility
        struct move_fn : move_tag {
            template <typename T>
            constexpr futures::detail::meta::_t<std::remove_reference<T>> &&operator()(T &&t) const //
                noexcept {
                return static_cast<futures::detail::meta::_t<std::remove_reference<T>> &&>(t);
            }

            /// \ingroup group-utility
            /// \sa `move_fn`
            template <typename T> friend constexpr decltype(auto) operator|(T &&t, move_fn move) noexcept {
                return move(t);
            }
        };

        /// \ingroup group-utility
        /// \sa `move_fn`
        RANGES_INLINE_VARIABLE(move_fn, move)

        /// \ingroup group-utility
        /// \sa `move_fn`
        template <typename R>
        using move_t =
            futures::detail::meta::if_c<std::is_reference<R>::value, futures::detail::meta::_t<std::remove_reference<R>> &&, ranges_detail::decay_t<R>>;
    } // namespace aux
} // namespace futures::detail

// #include <futures/algorithm/detail/traits/range/detail/epilogue.h>
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

#ifndef RANGES_PROLOGUE_INCLUDED
#error "Including epilogue, but prologue not included!"
#endif
#undef RANGES_PROLOGUE_INCLUDED

#undef template
#undef AND

RANGES_DIAGNOSTIC_POP


#endif

// #include <futures/algorithm/detail/traits/range/utility/static_const.h>

// #include <futures/algorithm/detail/traits/range/utility/swap.h>
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
// The implementation of swap (see below) has been adapted from libc++
// (http://libcxx.llvm.org).

#ifndef FUTURES_RANGES_UTILITY_SWAP_HPP
#define FUTURES_RANGES_UTILITY_SWAP_HPP

// #include <futures/algorithm/detail/traits/range/concepts/swap.h>


// #include <futures/algorithm/detail/traits/range/range_fwd.h>


// #include <futures/algorithm/detail/traits/range/utility/static_const.h>


// #include <futures/algorithm/detail/traits/range/detail/prologue.h>
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

#ifndef FUTURES_RANGES_DETAIL_PROLOGUE_HPP
#define FUTURES_RANGES_DETAIL_PROLOGUE_HPP
// #include <futures/algorithm/detail/traits/range/detail/config.h>

#endif

#ifdef RANGES_PROLOGUE_INCLUDED
#error "Prologue already included!"
#endif
#define RANGES_PROLOGUE_INCLUDED

RANGES_DIAGNOSTIC_PUSH

#ifdef RANGES_FEWER_WARNINGS
RANGES_DIAGNOSTIC_IGNORE_UNDEFINED_INTERNAL
RANGES_DIAGNOSTIC_IGNORE_INDENTATION
RANGES_DIAGNOSTIC_IGNORE_CXX17_COMPAT
#endif

RANGES_DIAGNOSTIC_KEYWORD_MACRO

#define template(...)                                                                                                  \
    CPP_PP_IGNORE_CXX2A_COMPAT_BEGIN                                                                                   \
    template <__VA_ARGS__ CPP_TEMPLATE_AUX_

#define AND CPP_and


namespace futures::detail {
    template <typename T> using is_swappable = concepts::is_swappable<T>;

    template <typename T> using is_nothrow_swappable = concepts::is_nothrow_swappable<T>;

    template <typename T, typename U> using is_swappable_with = concepts::is_swappable_with<T, U>;

    template <typename T, typename U> using is_nothrow_swappable_with = concepts::is_nothrow_swappable_with<T, U>;

    using concepts::exchange;

    /// \ingroup group-utility
    /// \relates concepts::adl_swap_ranges_detail::swap_fn
    RANGES_DEFINE_CPO(uncvref_t<decltype(concepts::swap)>, swap)

    namespace cpp20 {
        using ::futures::detail::swap;
    }
} // namespace futures::detail

// #include <futures/algorithm/detail/traits/range/detail/epilogue.h>
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

#ifndef RANGES_PROLOGUE_INCLUDED
#error "Including epilogue, but prologue not included!"
#endif
#undef RANGES_PROLOGUE_INCLUDED

#undef template
#undef AND

RANGES_DIAGNOSTIC_POP


#endif


// #include <futures/algorithm/detail/traits/range/detail/prologue.h>
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

#ifndef FUTURES_RANGES_DETAIL_PROLOGUE_HPP
#define FUTURES_RANGES_DETAIL_PROLOGUE_HPP
// #include <futures/algorithm/detail/traits/range/detail/config.h>

#endif

#ifdef RANGES_PROLOGUE_INCLUDED
#error "Prologue already included!"
#endif
#define RANGES_PROLOGUE_INCLUDED

RANGES_DIAGNOSTIC_PUSH

#ifdef RANGES_FEWER_WARNINGS
RANGES_DIAGNOSTIC_IGNORE_UNDEFINED_INTERNAL
RANGES_DIAGNOSTIC_IGNORE_INDENTATION
RANGES_DIAGNOSTIC_IGNORE_CXX17_COMPAT
#endif

RANGES_DIAGNOSTIC_KEYWORD_MACRO

#define template(...)                                                                                                  \
    CPP_PP_IGNORE_CXX2A_COMPAT_BEGIN                                                                                   \
    template <__VA_ARGS__ CPP_TEMPLATE_AUX_

#define AND CPP_and


namespace futures::detail {
    /// \addtogroup group-iterator
    /// @{

    /// \cond
    namespace ranges_detail {
        template <typename I,
#ifdef RANGES_WORKAROUND_MSVC_683388
                  typename R = futures::detail::meta::conditional_t<
                      std::is_pointer<uncvref_t<I>>::value && std::is_array<std::remove_pointer_t<uncvref_t<I>>>::value,
                      std::add_lvalue_reference_t<std::remove_pointer_t<uncvref_t<I>>>, decltype(*std::declval<I &>())>,
#else
                  typename R = decltype(*std::declval<I &>()),
#endif
                  typename = R &>
        using iter_reference_t_ = R;

#if defined(RANGES_DEEP_STL_INTEGRATION) && RANGES_DEEP_STL_INTEGRATION && !defined(RANGES_DOXYGEN_INVOKED)
        template <typename T>
        using iter_value_t_ =
            typename futures::detail::meta::conditional_t<is_std_iterator_traits_specialized_v<T>, std::iterator_traits<T>,
                                         indirectly_readable_traits<T>>::value_type;
#else
        template <typename T> using iter_value_t_ = typename indirectly_readable_traits<T>::value_type;
#endif
    } // namespace ranges_detail
    /// \endcond

    template <typename R> using iter_reference_t = ranges_detail::iter_reference_t_<R>;

    template <typename R> using iter_value_t = ranges_detail::iter_value_t_<uncvref_t<R>>;

    /// \cond
    namespace _iter_move_ {
#if RANGES_BROKEN_CPO_LOOKUP
        void iter_move(); // unqualified name lookup block
#endif

        template <typename T> decltype(iter_move(std::declval<T>())) try_adl_iter_move_(int);

        template <typename T> void try_adl_iter_move_(long);

        template <typename T>
        RANGES_INLINE_VAR constexpr bool is_adl_indirectly_movable_v =
            !RANGES_IS_SAME(void, decltype(_iter_move_::try_adl_iter_move_<T>(42)));

        struct fn {
            // clang-format off
            template<typename I,
                     typename = ranges_detail::enable_if_t<is_adl_indirectly_movable_v<I &>>>
#ifndef RANGES_WORKAROUND_CLANG_23135
            constexpr
#endif // RANGES_WORKAROUND_CLANG_23135
            auto CPP_auto_fun(operator())(I &&i)(const)
            (
                return iter_move(i)
            )

            template<
                typename I,
                typename = ranges_detail::enable_if_t<!is_adl_indirectly_movable_v<I &>>,
                typename R = iter_reference_t<I>>
#ifndef RANGES_WORKAROUND_CLANG_23135
            constexpr
#endif // RANGES_WORKAROUND_CLANG_23135
            auto CPP_auto_fun(operator())(I &&i)(const)
            (
                return static_cast<aux::move_t<R>>(aux::move(*i))
            )
            // clang-format on
        };
    } // namespace _iter_move_
    /// \endcond

    RANGES_DEFINE_CPO(_iter_move_::fn, iter_move)

    /// \cond
    namespace ranges_detail {
        template <typename I, typename O>
        auto is_indirectly_movable_(I &(*i)(), O &(*o)(), iter_value_t<I> *v = nullptr)
            -> always_<std::true_type, decltype(iter_value_t<I>(iter_move(i()))), decltype(*v = iter_move(i())),
                       decltype(*o() = (iter_value_t<I> &&) * v), decltype(*o() = iter_move(i()))>;
        template <typename I, typename O> auto is_indirectly_movable_(...) -> std::false_type;

        template <typename I, typename O>
        auto is_nothrow_indirectly_movable_(iter_value_t<I> *v)
            -> futures::detail::meta::bool_<noexcept(iter_value_t<I>(iter_move(std::declval<I &>()))) &&noexcept(
                *v = iter_move(std::declval<I &>())) &&noexcept(*std::declval<O &>() = (iter_value_t<I> &&) * v)
                               &&noexcept(*std::declval<O &>() = iter_move(std::declval<I &>()))>;
        template <typename I, typename O> auto is_nothrow_indirectly_movable_(...) -> std::false_type;
    } // namespace ranges_detail
    /// \endcond

    template <typename I, typename O>
    RANGES_INLINE_VAR constexpr bool is_indirectly_movable_v =
        decltype(ranges_detail::is_indirectly_movable_<I, O>(nullptr, nullptr))::value;

    template <typename I, typename O>
    RANGES_INLINE_VAR constexpr bool is_nothrow_indirectly_movable_v =
        decltype(ranges_detail::is_nothrow_indirectly_movable_<I, O>(nullptr))::value;

    template <typename I, typename O> struct is_indirectly_movable : futures::detail::meta::bool_<is_indirectly_movable_v<I, O>> {};

    template <typename I, typename O>
    struct is_nothrow_indirectly_movable : futures::detail::meta::bool_<is_nothrow_indirectly_movable_v<I, O>> {};

    /// \cond
    namespace _iter_swap_ {
        struct nope {};

        // Q: Should std::reference_wrapper be considered a proxy wrt swapping rvalues?
        // A: No. Its operator= is currently defined to reseat the references, so
        //    std::swap(ra, rb) already means something when ra and rb are (lvalue)
        //    reference_wrappers. That reseats the reference wrappers but leaves the
        //    referents unmodified. Treating rvalue reference_wrappers differently would
        //    be confusing.

        // Q: Then why is it OK to "re"-define swap for pairs and tuples of references?
        // A: Because as defined above, swapping an rvalue tuple of references has the
        //    same semantics as swapping an lvalue tuple of references. Rather than
        //    reseat the references, assignment happens *through* the references.

        // Q: But I have an iterator whose operator* returns an rvalue
        //    std::reference_wrapper<T>. How do I make it model indirectly_swappable?
        // A: With an overload of iter_swap.

        // Intentionally create an ambiguity with std::iter_swap, which is
        // unconstrained.
        template <typename T, typename U> nope iter_swap(T, U) = delete;

#ifdef RANGES_WORKAROUND_MSVC_895622
        nope iter_swap();
#endif

        template <typename T, typename U>
        decltype(iter_swap(std::declval<T>(), std::declval<U>())) try_adl_iter_swap_(int);

        template <typename T, typename U> nope try_adl_iter_swap_(long);

        // Test whether an overload of iter_swap for a T and a U can be found
        // via ADL with the iter_swap overload above participating in the
        // overload set. This depends on user-defined iter_swap overloads
        // being a better match than the overload in namespace std.
        template <typename T, typename U>
        RANGES_INLINE_VAR constexpr bool is_adl_indirectly_swappable_v =
            !RANGES_IS_SAME(nope, decltype(_iter_swap_::try_adl_iter_swap_<T, U>(42)));

        struct fn {
            // *If* a user-defined iter_swap is found via ADL, call that:
            template <typename T, typename U>
            constexpr ranges_detail::enable_if_t<is_adl_indirectly_swappable_v<T, U>> operator()(T &&t, U &&u) const
                noexcept(noexcept(iter_swap((T &&) t, (U &&) u))) {
                (void)iter_swap((T &&) t, (U &&) u);
            }

            // *Otherwise*, for readable types with swappable reference
            // types, call futures::detail::swap(*a, *b)
            template <typename I0, typename I1>
            constexpr ranges_detail::enable_if_t<!is_adl_indirectly_swappable_v<I0, I1> &&
                                          is_swappable_with<iter_reference_t<I0>, iter_reference_t<I1>>::value>
            operator()(I0 &&a, I1 &&b) const noexcept(noexcept(::futures::detail::swap(*a, *b))) {
                ::futures::detail::swap(*a, *b);
            }

            // *Otherwise*, for readable types that are mutually
            // indirectly_movable_storable, implement as:
            //      iter_value_t<T0> tmp = iter_move(a);
            //      *a = iter_move(b);
            //      *b = std::move(tmp);
            template <typename I0, typename I1>
            constexpr ranges_detail::enable_if_t<!is_adl_indirectly_swappable_v<I0, I1> &&
                                          !is_swappable_with<iter_reference_t<I0>, iter_reference_t<I1>>::value &&
                                          is_indirectly_movable_v<I0, I1> && is_indirectly_movable_v<I1, I0>>
            operator()(I0 &&a, I1 &&b) const
                noexcept(is_nothrow_indirectly_movable_v<I0, I1> &&is_nothrow_indirectly_movable_v<I1, I0>) {
                iter_value_t<I0> v0 = iter_move(a);
                *a = iter_move(b);
                *b = ranges_detail::move(v0);
            }
        };
    } // namespace _iter_swap_
    /// \endcond

    /// \relates _iter_swap_::fn
    RANGES_DEFINE_CPO(_iter_swap_::fn, iter_swap)

    /// \cond
    namespace ranges_detail {
        template <typename T, typename U>
        auto is_indirectly_swappable_(T &(*t)(), U &(*u)())
            -> ranges_detail::always_<std::true_type, decltype(iter_swap(t(), u()))>;
        template <typename T, typename U> auto is_indirectly_swappable_(...) -> std::false_type;

        template <typename T, typename U>
        auto is_nothrow_indirectly_swappable_(int)
            -> futures::detail::meta::bool_<noexcept(iter_swap(std::declval<T &>(), std::declval<U &>()))>;
        template <typename T, typename U> auto is_nothrow_indirectly_swappable_(long) -> std::false_type;
    } // namespace ranges_detail
    /// \endcond

    template <typename T, typename U>
    RANGES_INLINE_VAR constexpr bool is_indirectly_swappable_v =
        decltype(ranges_detail::is_indirectly_swappable_<T, U>(nullptr, nullptr))::value;

    template <typename T, typename U>
    RANGES_INLINE_VAR constexpr bool is_nothrow_indirectly_swappable_v =
        decltype(ranges_detail::is_nothrow_indirectly_swappable_<T, U>(0))::value;

    template <typename T, typename U> struct is_indirectly_swappable : futures::detail::meta::bool_<is_indirectly_swappable_v<T, U>> {};

    template <typename T, typename U>
    struct is_nothrow_indirectly_swappable : futures::detail::meta::bool_<is_nothrow_indirectly_swappable_v<T, U>> {};

    namespace cpp20 {
        using ::futures::detail::iter_move;
        using ::futures::detail::iter_reference_t;
        using ::futures::detail::iter_swap;
        using ::futures::detail::iter_value_t;
    } // namespace cpp20
    /// @}
} // namespace futures::detail

// #include <futures/algorithm/detail/traits/range/detail/epilogue.h>
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

#ifndef RANGES_PROLOGUE_INCLUDED
#error "Including epilogue, but prologue not included!"
#endif
#undef RANGES_PROLOGUE_INCLUDED

#undef template
#undef AND

RANGES_DIAGNOSTIC_POP


#endif // FUTURES_RANGES_ITERATOR_ACCESS_HPP

// #include <futures/algorithm/detail/traits/range/iterator/traits.h>
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

#ifndef FUTURES_RANGES_ITERATOR_TRAITS_HPP
#define FUTURES_RANGES_ITERATOR_TRAITS_HPP

// #include <iterator>

// #include <type_traits>


// #include <futures/algorithm/detail/traits/range/meta/meta.h>


// #include <futures/algorithm/detail/traits/range/concepts/concepts.h>


// #include <futures/algorithm/detail/traits/range/range_fwd.h>


// #include <futures/algorithm/detail/traits/range/iterator/access.h>
 // for iter_move, iter_swap
// #include <futures/algorithm/detail/traits/range/utility/common_type.h>
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

#ifndef FUTURES_RANGES_UTILITY_COMMON_TYPE_HPP
#define FUTURES_RANGES_UTILITY_COMMON_TYPE_HPP

// #include <tuple>

// #include <utility>


// #include <futures/algorithm/detail/traits/range/meta/meta.h>


// #include <futures/algorithm/detail/traits/range/concepts/type_traits.h>


// #include <futures/algorithm/detail/traits/range/range_fwd.h>


// #include <futures/algorithm/detail/traits/range/detail/prologue.h>
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

#ifndef FUTURES_RANGES_DETAIL_PROLOGUE_HPP
#define FUTURES_RANGES_DETAIL_PROLOGUE_HPP
// #include <futures/algorithm/detail/traits/range/detail/config.h>

#endif

#ifdef RANGES_PROLOGUE_INCLUDED
#error "Prologue already included!"
#endif
#define RANGES_PROLOGUE_INCLUDED

RANGES_DIAGNOSTIC_PUSH

#ifdef RANGES_FEWER_WARNINGS
RANGES_DIAGNOSTIC_IGNORE_UNDEFINED_INTERNAL
RANGES_DIAGNOSTIC_IGNORE_INDENTATION
RANGES_DIAGNOSTIC_IGNORE_CXX17_COMPAT
#endif

RANGES_DIAGNOSTIC_KEYWORD_MACRO

#define template(...)                                                                                                  \
    CPP_PP_IGNORE_CXX2A_COMPAT_BEGIN                                                                                   \
    template <__VA_ARGS__ CPP_TEMPLATE_AUX_

#define AND CPP_and


// Sadly, this is necessary because of:
//  - std::common_type is !SFINAE-friendly, and
//  - The specification of std::common_type makes it impossibly
//    difficult to specialize on user-defined types without spamming
//    out a bajillion copies to handle all combinations of cv and ref
//    qualifiers.

namespace futures::detail {
    template <typename... Ts> using common_type = concepts::common_type<Ts...>;

    template <typename... Ts> using common_type_t = concepts::common_type_t<Ts...>;

    template <typename... Ts> using common_reference = concepts::common_reference<Ts...>;

    template <typename... Ts> using common_reference_t = concepts::common_reference_t<Ts...>;

    /// \cond
    template <typename F, typename S> struct common_pair;

    template <typename... Ts> struct common_tuple;
    /// \endcond
} // namespace futures::detail

/// \cond
// Specializations for pair and tuple
namespace futures::detail::concepts {
    // common_type for std::pairs
    template <typename F1, typename S1, typename F2, typename S2>
    struct common_type<std::pair<F1, S1>, ::futures::detail::common_pair<F2, S2>>;

    template <typename F1, typename S1, typename F2, typename S2>
    struct common_type<::futures::detail::common_pair<F1, S1>, std::pair<F2, S2>>;

    template <typename F1, typename S1, typename F2, typename S2>
    struct common_type<::futures::detail::common_pair<F1, S1>, ::futures::detail::common_pair<F2, S2>>;

    // common_type for std::tuples
    template <typename... Ts, typename... Us> struct common_type<::futures::detail::common_tuple<Ts...>, std::tuple<Us...>>;

    template <typename... Ts, typename... Us> struct common_type<std::tuple<Ts...>, ::futures::detail::common_tuple<Us...>>;

    template <typename... Ts, typename... Us>
    struct common_type<::futures::detail::common_tuple<Ts...>, ::futures::detail::common_tuple<Us...>>;

    // A common reference for std::pairs
    template <typename F1, typename S1, typename F2, typename S2, template <typename> class Qual1,
              template <typename> class Qual2>
    struct basic_common_reference<::futures::detail::common_pair<F1, S1>, std::pair<F2, S2>, Qual1, Qual2>;

    template <typename F1, typename S1, typename F2, typename S2, template <typename> class Qual1,
              template <typename> class Qual2>
    struct basic_common_reference<std::pair<F1, S1>, ::futures::detail::common_pair<F2, S2>, Qual1, Qual2>;

    template <typename F1, typename S1, typename F2, typename S2, template <typename> class Qual1,
              template <typename> class Qual2>
    struct basic_common_reference<::futures::detail::common_pair<F1, S1>, ::futures::detail::common_pair<F2, S2>, Qual1, Qual2>;

    // A common reference for std::tuples
    template <typename... Ts, typename... Us, template <typename> class Qual1, template <typename> class Qual2>
    struct basic_common_reference<::futures::detail::common_tuple<Ts...>, std::tuple<Us...>, Qual1, Qual2>;

    template <typename... Ts, typename... Us, template <typename> class Qual1, template <typename> class Qual2>
    struct basic_common_reference<std::tuple<Ts...>, ::futures::detail::common_tuple<Us...>, Qual1, Qual2>;

    template <typename... Ts, typename... Us, template <typename> class Qual1, template <typename> class Qual2>
    struct basic_common_reference<::futures::detail::common_tuple<Ts...>, ::futures::detail::common_tuple<Us...>, Qual1, Qual2>;
} // namespace futures::detail::concepts

#if RANGES_CXX_VER > RANGES_CXX_STD_17
RANGES_DIAGNOSTIC_PUSH
RANGES_DIAGNOSTIC_IGNORE_MISMATCHED_TAGS
RANGES_BEGIN_NAMESPACE_STD
RANGES_BEGIN_NAMESPACE_VERSION

template <typename...> struct common_type;

// common_type for std::pairs
template <typename F1, typename S1, typename F2, typename S2>
struct common_type<std::pair<F1, S1>, ::futures::detail::common_pair<F2, S2>>;

template <typename F1, typename S1, typename F2, typename S2>
struct common_type<::futures::detail::common_pair<F1, S1>, std::pair<F2, S2>>;

template <typename F1, typename S1, typename F2, typename S2>
struct common_type<::futures::detail::common_pair<F1, S1>, ::futures::detail::common_pair<F2, S2>>;

// common_type for std::tuples
template <typename... Ts, typename... Us> struct common_type<::futures::detail::common_tuple<Ts...>, std::tuple<Us...>>;

template <typename... Ts, typename... Us> struct common_type<std::tuple<Ts...>, ::futures::detail::common_tuple<Us...>>;

template <typename... Ts, typename... Us>
struct common_type<::futures::detail::common_tuple<Ts...>, ::futures::detail::common_tuple<Us...>>;

template <typename, typename, template <typename> class, template <typename> class> struct basic_common_reference;

// A common reference for std::pairs
template <typename F1, typename S1, typename F2, typename S2, template <typename> class Qual1,
          template <typename> class Qual2>
struct basic_common_reference<::futures::detail::common_pair<F1, S1>, std::pair<F2, S2>, Qual1, Qual2>;

template <typename F1, typename S1, typename F2, typename S2, template <typename> class Qual1,
          template <typename> class Qual2>
struct basic_common_reference<std::pair<F1, S1>, ::futures::detail::common_pair<F2, S2>, Qual1, Qual2>;

template <typename F1, typename S1, typename F2, typename S2, template <typename> class Qual1,
          template <typename> class Qual2>
struct basic_common_reference<::futures::detail::common_pair<F1, S1>, ::futures::detail::common_pair<F2, S2>, Qual1, Qual2>;

// A common reference for std::tuples
template <typename... Ts, typename... Us, template <typename> class Qual1, template <typename> class Qual2>
struct basic_common_reference<::futures::detail::common_tuple<Ts...>, std::tuple<Us...>, Qual1, Qual2>;

template <typename... Ts, typename... Us, template <typename> class Qual1, template <typename> class Qual2>
struct basic_common_reference<std::tuple<Ts...>, ::futures::detail::common_tuple<Us...>, Qual1, Qual2>;

template <typename... Ts, typename... Us, template <typename> class Qual1, template <typename> class Qual2>
struct basic_common_reference<::futures::detail::common_tuple<Ts...>, ::futures::detail::common_tuple<Us...>, Qual1, Qual2>;

RANGES_END_NAMESPACE_VERSION
RANGES_END_NAMESPACE_STD
RANGES_DIAGNOSTIC_POP
#endif // RANGES_CXX_VER > RANGES_CXX_STD_17
/// \endcond

// #include <futures/algorithm/detail/traits/range/detail/epilogue.h>
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

#ifndef RANGES_PROLOGUE_INCLUDED
#error "Including epilogue, but prologue not included!"
#endif
#undef RANGES_PROLOGUE_INCLUDED

#undef template
#undef AND

RANGES_DIAGNOSTIC_POP


#endif


// #include <futures/algorithm/detail/traits/range/detail/prologue.h>
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

#ifndef FUTURES_RANGES_DETAIL_PROLOGUE_HPP
#define FUTURES_RANGES_DETAIL_PROLOGUE_HPP
// #include <futures/algorithm/detail/traits/range/detail/config.h>

#endif

#ifdef RANGES_PROLOGUE_INCLUDED
#error "Prologue already included!"
#endif
#define RANGES_PROLOGUE_INCLUDED

RANGES_DIAGNOSTIC_PUSH

#ifdef RANGES_FEWER_WARNINGS
RANGES_DIAGNOSTIC_IGNORE_UNDEFINED_INTERNAL
RANGES_DIAGNOSTIC_IGNORE_INDENTATION
RANGES_DIAGNOSTIC_IGNORE_CXX17_COMPAT
#endif

RANGES_DIAGNOSTIC_KEYWORD_MACRO

#define template(...)                                                                                                  \
    CPP_PP_IGNORE_CXX2A_COMPAT_BEGIN                                                                                   \
    template <__VA_ARGS__ CPP_TEMPLATE_AUX_

#define AND CPP_and


namespace futures::detail {
    /// \addtogroup group-iterator
    /// @{

    /// \cond
    using input_iterator_tag RANGES_DEPRECATED("Please switch to the standard iterator tags") = std::input_iterator_tag;
    using forward_iterator_tag
        RANGES_DEPRECATED("Please switch to the standard iterator tags") = std::forward_iterator_tag;
    using bidirectional_iterator_tag
        RANGES_DEPRECATED("Please switch to the standard iterator tags") = std::bidirectional_iterator_tag;
    using random_access_iterator_tag
        RANGES_DEPRECATED("Please switch to the standard iterator tags") = std::random_access_iterator_tag;
    /// \endcond

    struct contiguous_iterator_tag : std::random_access_iterator_tag {};

    /// \cond
    namespace ranges_detail {
        template <typename I, typename = iter_reference_t<I>, typename R = decltype(iter_move(std::declval<I &>())),
                  typename = R &>
        using iter_rvalue_reference_t = R;

        template <typename I>
        RANGES_INLINE_VAR constexpr bool
            has_nothrow_iter_move_v = noexcept(iter_rvalue_reference_t<I>(::futures::detail::iter_move(std::declval<I &>())));
    } // namespace ranges_detail
    /// \endcond

    template <typename I> using iter_rvalue_reference_t = ranges_detail::iter_rvalue_reference_t<I>;

    template <typename I> using iter_common_reference_t = common_reference_t<iter_reference_t<I>, iter_value_t<I> &>;

#if defined(RANGES_DEEP_STL_INTEGRATION) && RANGES_DEEP_STL_INTEGRATION && !defined(RANGES_DOXYGEN_INVOKED)
    template <typename T>
    using iter_difference_t = typename futures::detail::meta::conditional_t<ranges_detail::is_std_iterator_traits_specialized_v<T>,
                                                           std::iterator_traits<uncvref_t<T>>,
                                                           incrementable_traits<uncvref_t<T>>>::difference_type;
#else
    template <typename T> using iter_difference_t = typename incrementable_traits<uncvref_t<T>>::difference_type;
#endif

    // Defined in <range/v3/iterator/access.hpp>
    // template<typename T>
    // using iter_value_t = ...

    // Defined in <range/v3/iterator/access.hpp>
    // template<typename R>
    // using iter_reference_t = ranges_detail::iter_reference_t_<R>;

    // Defined in <range/v3/range_fwd.hpp>:
    // template<typename S, typename I>
    // inline constexpr bool disable_sized_sentinel = false;

    /// \cond
    namespace ranges_detail {
        template <typename I>
        using iter_size_t =
            futures::detail::meta::_t<futures::detail::meta::conditional_t<std::is_integral<iter_difference_t<I>>::value,
                                         std::make_unsigned<iter_difference_t<I>>, futures::detail::meta::id<iter_difference_t<I>>>>;

        template <typename I> using iter_arrow_t = decltype(std::declval<I &>().operator->());

        template <typename I>
        using iter_pointer_t =
            futures::detail::meta::_t<futures::detail::meta::conditional_t<futures::detail::meta::is_trait<futures::detail::meta::defer<iter_arrow_t, I>>::value,
                                         futures::detail::meta::defer<iter_arrow_t, I>, std::add_pointer<iter_reference_t<I>>>>;

        template <typename T> struct difference_type_ : futures::detail::meta::defer<iter_difference_t, T> {};

        template <typename T> struct value_type_ : futures::detail::meta::defer<iter_value_t, T> {};

        template <typename T> struct size_type_ : futures::detail::meta::defer<iter_size_t, T> {};
    } // namespace ranges_detail

    template <typename T>
    using difference_type_t RANGES_DEPRECATED("futures::detail::difference_type_t is deprecated. Please use "
                                              "futures::detail::iter_difference_t instead.") = iter_difference_t<T>;

    template <typename T>
    using value_type_t RANGES_DEPRECATED("futures::detail::value_type_t is deprecated. Please use "
                                         "futures::detail::iter_value_t instead.") = iter_value_t<T>;

    template <typename R>
    using reference_t RANGES_DEPRECATED("futures::detail::reference_t is deprecated. Use futures::detail::iter_reference_t "
                                        "instead.") = iter_reference_t<R>;

    template <typename I>
    using rvalue_reference_t RANGES_DEPRECATED("rvalue_reference_t is deprecated; "
                                               "use iter_rvalue_reference_t instead") = iter_rvalue_reference_t<I>;

    template <typename T>
    struct RANGES_DEPRECATED("futures::detail::size_type is deprecated. Iterators do not have an associated "
                             "size_type.") size_type : ranges_detail::size_type_<T> {};

    template <typename I> using size_type_t RANGES_DEPRECATED("size_type_t is deprecated.") = ranges_detail::iter_size_t<I>;
    /// \endcond

    namespace cpp20 {
        using ::futures::detail::iter_common_reference_t;
        using ::futures::detail::iter_difference_t;
        using ::futures::detail::iter_reference_t;
        using ::futures::detail::iter_rvalue_reference_t;
        using ::futures::detail::iter_value_t;

        // Specialize these in the ::futures::detail:: namespace
        using ::futures::detail::disable_sized_sentinel;
        template <typename T> using incrementable_traits = ::futures::detail::incrementable_traits<T>;
        template <typename T> using indirectly_readable_traits = ::futures::detail::indirectly_readable_traits<T>;
    } // namespace cpp20
    /// @}
} // namespace futures::detail

// #include <futures/algorithm/detail/traits/range/detail/epilogue.h>
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

#ifndef RANGES_PROLOGUE_INCLUDED
#error "Including epilogue, but prologue not included!"
#endif
#undef RANGES_PROLOGUE_INCLUDED

#undef template
#undef AND

RANGES_DIAGNOSTIC_POP


#endif // FUTURES_RANGES_ITERATOR_TRAITS_HPP


#ifdef _GLIBCXX_DEBUG
#include <debug/safe_iterator.h>
#endif

// #include <futures/algorithm/detail/traits/range/detail/prologue.h>
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

#ifndef FUTURES_RANGES_DETAIL_PROLOGUE_HPP
#define FUTURES_RANGES_DETAIL_PROLOGUE_HPP
// #include <futures/algorithm/detail/traits/range/detail/config.h>

#endif

#ifdef RANGES_PROLOGUE_INCLUDED
#error "Prologue already included!"
#endif
#define RANGES_PROLOGUE_INCLUDED

RANGES_DIAGNOSTIC_PUSH

#ifdef RANGES_FEWER_WARNINGS
RANGES_DIAGNOSTIC_IGNORE_UNDEFINED_INTERNAL
RANGES_DIAGNOSTIC_IGNORE_INDENTATION
RANGES_DIAGNOSTIC_IGNORE_CXX17_COMPAT
#endif

RANGES_DIAGNOSTIC_KEYWORD_MACRO

#define template(...)                                                                                                  \
    CPP_PP_IGNORE_CXX2A_COMPAT_BEGIN                                                                                   \
    template <__VA_ARGS__ CPP_TEMPLATE_AUX_

#define AND CPP_and


namespace futures::detail {
    /// \addtogroup group-iterator-concepts
    /// @{

    /// \cond
    namespace ranges_detail {
        template <typename I>
        using iter_traits_t = futures::detail::meta::conditional_t<is_std_iterator_traits_specialized_v<I>, std::iterator_traits<I>, I>;

#if defined(_GLIBCXX_DEBUG)
        template(typename I, typename T, typename Seq)(
            /// \pre
            requires same_as<
                I, __gnu_debug::_Safe_iterator<T *, Seq>>) auto iter_concept_(__gnu_debug::_Safe_iterator<T *, Seq>,
                                                                              priority_tag<3>)
            -> ::futures::detail::contiguous_iterator_tag
#endif
#if defined(__GLIBCXX__)
            template(typename I, typename T, typename Seq)(
                /// \pre
                requires same_as<I, __gnu_cxx::__normal_iterator<
                                        T *, Seq>>) auto iter_concept_(__gnu_cxx::__normal_iterator<T *, Seq>,
                                                                       priority_tag<3>)
                -> ::futures::detail::contiguous_iterator_tag;
#endif
#if defined(_LIBCPP_VERSION)
        template(typename I, typename T)(
            /// \pre
            requires same_as<I, std::__wrap_iter<T *>>) auto iter_concept_(std::__wrap_iter<T *>, priority_tag<3>)
            -> ::futures::detail::contiguous_iterator_tag;
#endif
#if defined(_MSVC_STL_VERSION) || defined(_IS_WRS)
        template(typename I)(
            /// \pre
            requires same_as<I, class I::_Array_iterator>) auto iter_concept_(I, priority_tag<3>)
            -> ::futures::detail::contiguous_iterator_tag;
        template(typename I)(
            /// \pre
            requires same_as<I, class I::_Array_const_iterator>) auto iter_concept_(I, priority_tag<3>)
            -> ::futures::detail::contiguous_iterator_tag;
        template(typename I)(
            /// \pre
            requires same_as<I, class I::_Vector_iterator>) auto iter_concept_(I, priority_tag<3>)
            -> ::futures::detail::contiguous_iterator_tag;
        template(typename I)(
            /// \pre
            requires same_as<I, class I::_Vector_const_iterator>) auto iter_concept_(I, priority_tag<3>)
            -> ::futures::detail::contiguous_iterator_tag;
        template(typename I)(
            /// \pre
            requires same_as<I, class I::_String_iterator>) auto iter_concept_(I, priority_tag<3>)
            -> ::futures::detail::contiguous_iterator_tag;
        template(typename I)(
            /// \pre
            requires same_as<I, class I::_String_const_iterator>) auto iter_concept_(I, priority_tag<3>)
            -> ::futures::detail::contiguous_iterator_tag;
        template(typename I)(
            /// \pre
            requires same_as<I, class I::_String_view_iterator>) auto iter_concept_(I, priority_tag<3>)
            -> ::futures::detail::contiguous_iterator_tag;
#endif
        template(typename I, typename T)(
            /// \pre
            requires same_as<I, T *>) auto iter_concept_(T *, priority_tag<3>) -> ::futures::detail::contiguous_iterator_tag;
        template <typename I> auto iter_concept_(I, priority_tag<2>) -> typename iter_traits_t<I>::iterator_concept;
        template <typename I> auto iter_concept_(I, priority_tag<1>) -> typename iter_traits_t<I>::iterator_category;
        template <typename I>
        auto iter_concept_(I, priority_tag<0>)
            -> enable_if_t<!is_std_iterator_traits_specialized_v<I>, std::random_access_iterator_tag>;

        template <typename I> using iter_concept_t = decltype(iter_concept_<I>(std::declval<I>(), priority_tag<3>{}));

        using ::futures::detail::concepts::ranges_detail::weakly_equality_comparable_with_;

        template <typename I>
        using readable_types_t = futures::detail::meta::list<iter_value_t<I>, iter_reference_t<I>, iter_rvalue_reference_t<I>>;
    } // namespace ranges_detail
      /// \endcond

    // clang-format off
    template(typename I)(
    concept (readable_)(I),
        // requires (I const i)
        // (
        //     { *i } -> same_as<iter_reference_t<I>>;
        //     { iter_move(i) } -> same_as<iter_rvalue_reference_t<I>>;
        // ) &&
        same_as<iter_reference_t<I const>, iter_reference_t<I>> AND
        same_as<iter_rvalue_reference_t<I const>, iter_rvalue_reference_t<I>> AND
        common_reference_with<iter_reference_t<I> &&, iter_value_t<I> &> AND
        common_reference_with<iter_reference_t<I> &&,
                              iter_rvalue_reference_t<I> &&> AND
        common_reference_with<iter_rvalue_reference_t<I> &&, iter_value_t<I> const &>
    );

    template<typename I>
    CPP_concept indirectly_readable = //
        CPP_concept_ref(::futures::detail::readable_, uncvref_t<I>);

    template<typename I>
    RANGES_DEPRECATED("Please use ::futures::detail::indirectly_readable instead")
    RANGES_INLINE_VAR constexpr bool readable = //
        indirectly_readable<I>;

    template<typename O, typename T>
    CPP_requires(writable_,
        requires(O && o, T && t) //
        (
            *o = (T &&) t,
            *(O &&) o = (T &&) t,
            const_cast<iter_reference_t<O> const &&>(*o) = (T &&) t,
            const_cast<iter_reference_t<O> const &&>(*(O &&) o) = (T &&) t
        ));
    template<typename O, typename T>
    CPP_concept indirectly_writable = //
        CPP_requires_ref(::futures::detail::writable_, O, T);

    template<typename O, typename T>
    RANGES_DEPRECATED("Please use ::futures::detail::indirectly_writable instead")
    RANGES_INLINE_VAR constexpr bool writable = //
        indirectly_writable<O, T>;
    // clang-format on

    /// \cond
    namespace ranges_detail {
#if RANGES_CXX_INLINE_VARIABLES >= RANGES_CXX_INLINE_VARIABLES_17
        template <typename D> inline constexpr bool _is_integer_like_ = std::is_integral<D>::value;
#else
        template <typename D, typename = void> constexpr bool _is_integer_like_ = std::is_integral<D>::value;
#endif

        // gcc10 uses for std::futures::detail::range_difference_t<
        // std::futures::detail::iota_view<size_t, size_t>> == __int128
#if __SIZEOF_INT128__
        __extension__ typedef __int128 int128_t;
#if RANGES_CXX_INLINE_VARIABLES >= RANGES_CXX_INLINE_VARIABLES_17
        template <> inline constexpr bool _is_integer_like_<int128_t> = true;
#else
        template <typename Enable> constexpr bool _is_integer_like_<int128_t, Enable> = true;
#endif
#endif // __SIZEOF_INT128__

        // clang-format off
        template<typename D>
        CPP_concept integer_like_ = _is_integer_like_<D>;

#ifdef RANGES_WORKAROUND_MSVC_792338
        template<typename D, bool Signed = (D(-1) < D(0))>
        constexpr bool _is_signed_(D *)
        {
            return Signed;
        }
        constexpr bool _is_signed_(void *)
        {
            return false;
        }

        template<typename D>
        CPP_concept signed_integer_like_ =
            integer_like_<D> && ranges_detail::_is_signed_((D*) nullptr);
#else // ^^^ workaround / no workaround vvv
        template(typename D)(
        concept (signed_integer_like_impl_)(D),
            integer_like_<D> AND
            concepts::type<std::integral_constant<bool, (D(-1) < D(0))>> AND
            std::integral_constant<bool, (D(-1) < D(0))>::value
        );

        template<typename D>
        CPP_concept signed_integer_like_ =
            integer_like_<D> &&
            CPP_concept_ref(ranges_detail::signed_integer_like_impl_, D);
#endif // RANGES_WORKAROUND_MSVC_792338
       // clang-format on
    } // namespace ranges_detail
      /// \endcond

    // clang-format off
    template<typename I>
    CPP_requires(weakly_incrementable_,
        requires(I i) //
        (
            ++i,
            i++,
            concepts::requires_<same_as<I&, decltype(++i)>>
        ));

    template(typename I)(
    concept (weakly_incrementable_)(I),
        concepts::type<iter_difference_t<I>> AND
        ranges_detail::signed_integer_like_<iter_difference_t<I>>);

    template<typename I>
    CPP_concept weakly_incrementable =
        semiregular<I> &&
        CPP_requires_ref(::futures::detail::weakly_incrementable_, I) &&
        CPP_concept_ref(::futures::detail::weakly_incrementable_, I);

    template<typename I>
    CPP_requires(incrementable_,
        requires(I i) //
        (
            concepts::requires_<same_as<I, decltype(i++)>>
        ));
    template<typename I>
    CPP_concept incrementable =
        regular<I> &&
        weakly_incrementable<I> &&
        CPP_requires_ref(::futures::detail::incrementable_, I);

    template(typename I)(
    concept (input_or_output_iterator_)(I),
        ranges_detail::dereferenceable_<I&>
    );

    template<typename I>
    CPP_concept input_or_output_iterator =
        weakly_incrementable<I> &&
        CPP_concept_ref(::futures::detail::input_or_output_iterator_, I);

    template<typename S, typename I>
    CPP_concept sentinel_for =
        semiregular<S> &&
        input_or_output_iterator<I> &&
        ranges_detail::weakly_equality_comparable_with_<S, I>;

    template<typename S, typename I>
    CPP_requires(sized_sentinel_for_,
        requires(S const & s, I const & i) //
        (
            s - i,
            i - s,
            concepts::requires_<same_as<iter_difference_t<I>, decltype(s - i)>>,
            concepts::requires_<same_as<iter_difference_t<I>, decltype(i - s)>>
        ));
    template(typename S, typename I)(
    concept (sized_sentinel_for_)(S, I),
        (!disable_sized_sentinel<std::remove_cv_t<S>, std::remove_cv_t<I>>) AND
        sentinel_for<S, I>);

    template<typename S, typename I>
    CPP_concept sized_sentinel_for =
        CPP_concept_ref(sized_sentinel_for_, S, I) &&
        CPP_requires_ref(::futures::detail::sized_sentinel_for_, S, I);

    template<typename Out, typename T>
    CPP_requires(output_iterator_,
        requires(Out o, T && t) //
        (
            *o++ = (T &&) t
        ));
    template<typename Out, typename T>
    CPP_concept output_iterator =
        input_or_output_iterator<Out> &&
        indirectly_writable<Out, T> &&
        CPP_requires_ref(::futures::detail::output_iterator_, Out, T);

    template(typename I, typename Tag)(
    concept (with_category_)(I, Tag),
        derived_from<ranges_detail::iter_concept_t<I>, Tag>
    );

    template<typename I>
    CPP_concept input_iterator =
        input_or_output_iterator<I> &&
        indirectly_readable<I> &&
        CPP_concept_ref(::futures::detail::with_category_, I, std::input_iterator_tag);

    template<typename I>
    CPP_concept forward_iterator =
        input_iterator<I> &&
        incrementable<I> &&
        sentinel_for<I, I> &&
        CPP_concept_ref(::futures::detail::with_category_, I, std::forward_iterator_tag);

    template<typename I>
    CPP_requires(bidirectional_iterator_,
        requires(I i) //
        (
            --i,
            i--,
            concepts::requires_<same_as<I&, decltype(--i)>>,
            concepts::requires_<same_as<I, decltype(i--)>>
        ));
    template<typename I>
    CPP_concept bidirectional_iterator =
        forward_iterator<I> &&
        CPP_requires_ref(::futures::detail::bidirectional_iterator_, I) &&
        CPP_concept_ref(::futures::detail::with_category_, I, std::bidirectional_iterator_tag);

    template<typename I>
    CPP_requires(random_access_iterator_,
        requires(I i, iter_difference_t<I> n)
        (
            i + n,
            n + i,
            i - n,
            i += n,
            i -= n,
            concepts::requires_<same_as<decltype(i + n), I>>,
            concepts::requires_<same_as<decltype(n + i), I>>,
            concepts::requires_<same_as<decltype(i - n), I>>,
            concepts::requires_<same_as<decltype(i += n), I&>>,
            concepts::requires_<same_as<decltype(i -= n), I&>>,
            concepts::requires_<same_as<decltype(i[n]), iter_reference_t<I>>>
        ));
    template<typename I>
    CPP_concept random_access_iterator =
        bidirectional_iterator<I> &&
        totally_ordered<I> &&
        sized_sentinel_for<I, I> &&
        CPP_requires_ref(::futures::detail::random_access_iterator_, I) &&
        CPP_concept_ref(::futures::detail::with_category_, I, std::random_access_iterator_tag);

    template(typename I)(
    concept (contiguous_iterator_)(I),
        std::is_lvalue_reference<iter_reference_t<I>>::value AND
        same_as<iter_value_t<I>, uncvref_t<iter_reference_t<I>>> AND
        derived_from<ranges_detail::iter_concept_t<I>, ::futures::detail::contiguous_iterator_tag>
    );

    template<typename I>
    CPP_concept contiguous_iterator =
        random_access_iterator<I> &&
        CPP_concept_ref(::futures::detail::contiguous_iterator_, I);
    // clang-format on

    /////////////////////////////////////////////////////////////////////////////////////
    // iterator_tag_of
    template <typename Rng>
    using iterator_tag_of =                              //
        std::enable_if_t<                                //
            input_iterator<Rng>,                         //
            futures::detail::meta::conditional_t<                         //
                contiguous_iterator<Rng>,                //
                ::futures::detail::contiguous_iterator_tag,         //
                futures::detail::meta::conditional_t<                     //
                    random_access_iterator<Rng>,         //
                    std::random_access_iterator_tag,     //
                    futures::detail::meta::conditional_t<                 //
                        bidirectional_iterator<Rng>,     //
                        std::bidirectional_iterator_tag, //
                        futures::detail::meta::conditional_t<             //
                            forward_iterator<Rng>,       //
                            std::forward_iterator_tag,   //
                            std::input_iterator_tag>>>>>;

    /// \cond
    namespace ranges_detail {
        template <typename, bool> struct iterator_category_ {};

        template <typename I> struct iterator_category_<I, true> { using type = iterator_tag_of<I>; };

        template <typename T, typename U = futures::detail::meta::_t<std::remove_const<T>>>
        using iterator_category = iterator_category_<U, (bool)input_iterator<U>>;
    } // namespace ranges_detail
    /// \endcond

    /// \cond
    // Generally useful to know if an iterator is single-pass or not:
    // clang-format off
    template<typename I>
    CPP_concept single_pass_iterator_ =
        input_or_output_iterator<I> && !forward_iterator<I>;
    // clang-format on
    /// \endcond

    //////////////////////////////////////////////////////////////////////////////////////
    // indirect_result_t
    template <typename Fun, typename... Is>
    using indirect_result_t = ranges_detail::enable_if_t<(bool)and_v<(bool)indirectly_readable<Is>...>,
                                                  invoke_result_t<Fun, iter_reference_t<Is>...>>;

    /// \cond
    namespace ranges_detail {
        // clang-format off
        template(typename T1, typename T2, typename T3, typename T4)(
        concept (common_reference_with_4_impl_)(T1, T2, T3, T4),
            concepts::type<common_reference_t<T1, T2, T3, T4>>     AND
            convertible_to<T1, common_reference_t<T1, T2, T3, T4>> AND
            convertible_to<T2, common_reference_t<T1, T2, T3, T4>> AND
            convertible_to<T3, common_reference_t<T1, T2, T3, T4>> AND
            convertible_to<T4, common_reference_t<T1, T2, T3, T4>>
        );

        template<typename T1, typename T2, typename T3, typename T4>
        CPP_concept common_reference_with_4_ =
            CPP_concept_ref(ranges_detail::common_reference_with_4_impl_, T1, T2, T3, T4);
        // axiom: all permutations of T1,T2,T3,T4 have the same
        // common reference type.

        template(typename F, typename I)(
        concept (indirectly_unary_invocable_impl_)(F, I),
            invocable<F &, iter_value_t<I> &> AND
            invocable<F &, iter_reference_t<I>> AND
            invocable<F &, iter_common_reference_t<I>> AND
            common_reference_with<
                invoke_result_t<F &, iter_value_t<I> &>,
                invoke_result_t<F &, iter_reference_t<I>>>
        );

        template<typename F, typename I>
        CPP_concept indirectly_unary_invocable_ =
            indirectly_readable<I> &&
            CPP_concept_ref(ranges_detail::indirectly_unary_invocable_impl_, F, I);
        // clang-format on
    } // namespace ranges_detail
      /// \endcond

    // clang-format off
    template<typename F, typename I>
    CPP_concept indirectly_unary_invocable =
        ranges_detail::indirectly_unary_invocable_<F, I> &&
        copy_constructible<F>;

    template(typename F, typename I)(
    concept (indirectly_regular_unary_invocable_)(F, I),
        regular_invocable<F &, iter_value_t<I> &> AND
        regular_invocable<F &, iter_reference_t<I>> AND
        regular_invocable<F &, iter_common_reference_t<I>> AND
        common_reference_with<
            invoke_result_t<F &, iter_value_t<I> &>,
            invoke_result_t<F &, iter_reference_t<I>>>
    );

    template<typename F, typename I>
    CPP_concept indirectly_regular_unary_invocable =
        indirectly_readable<I> &&
        copy_constructible<F> &&
        CPP_concept_ref(::futures::detail::indirectly_regular_unary_invocable_, F, I);

    /// \cond
    // Non-standard indirect invocable concepts
    template(typename F, typename I1, typename I2)(
    concept (indirectly_binary_invocable_impl_)(F, I1, I2),
        invocable<F &, iter_value_t<I1> &, iter_value_t<I2> &> AND
        invocable<F &, iter_value_t<I1> &, iter_reference_t<I2>> AND
        invocable<F &, iter_reference_t<I1>, iter_value_t<I2> &> AND
        invocable<F &, iter_reference_t<I1>, iter_reference_t<I2>> AND
        invocable<F &, iter_common_reference_t<I1>, iter_common_reference_t<I2>> AND
        ranges_detail::common_reference_with_4_<
            invoke_result_t<F &, iter_value_t<I1> &, iter_value_t<I2> &>,
            invoke_result_t<F &, iter_value_t<I1> &, iter_reference_t<I2>>,
            invoke_result_t<F &, iter_reference_t<I1>, iter_value_t<I2> &>,
            invoke_result_t<F &, iter_reference_t<I1>, iter_reference_t<I2>>>
    );

    template<typename F, typename I1, typename I2>
    CPP_concept indirectly_binary_invocable_ =
        indirectly_readable<I1> && indirectly_readable<I2> &&
        copy_constructible<F> &&
        CPP_concept_ref(::futures::detail::indirectly_binary_invocable_impl_, F, I1, I2);

    template(typename F, typename I1, typename I2)(
    concept (indirectly_regular_binary_invocable_impl_)(F, I1, I2),
        regular_invocable<F &, iter_value_t<I1> &, iter_value_t<I2> &> AND
        regular_invocable<F &, iter_value_t<I1> &, iter_reference_t<I2>> AND
        regular_invocable<F &, iter_reference_t<I1>, iter_value_t<I2> &> AND
        regular_invocable<F &, iter_reference_t<I1>, iter_reference_t<I2>> AND
        regular_invocable<F &, iter_common_reference_t<I1>, iter_common_reference_t<I2>> AND
        ranges_detail::common_reference_with_4_<
            invoke_result_t<F &, iter_value_t<I1> &, iter_value_t<I2> &>,
            invoke_result_t<F &, iter_value_t<I1> &, iter_reference_t<I2>>,
            invoke_result_t<F &, iter_reference_t<I1>, iter_value_t<I2> &>,
            invoke_result_t<F &, iter_reference_t<I1>, iter_reference_t<I2>>>
    );

    template<typename F, typename I1, typename I2>
    CPP_concept indirectly_regular_binary_invocable_ =
        indirectly_readable<I1> && indirectly_readable<I2> &&
        copy_constructible<F> &&
        CPP_concept_ref(::futures::detail::indirectly_regular_binary_invocable_impl_, F, I1, I2);
    /// \endcond

    template(typename F, typename I)(
    concept (indirect_unary_predicate_)(F, I),
        predicate<F &, iter_value_t<I> &> AND
        predicate<F &, iter_reference_t<I>> AND
        predicate<F &, iter_common_reference_t<I>>
    );

    template<typename F, typename I>
    CPP_concept indirect_unary_predicate =
        indirectly_readable<I> &&
        copy_constructible<F> &&
        CPP_concept_ref(::futures::detail::indirect_unary_predicate_, F, I);

    template(typename F, typename I1, typename I2)(
    concept (indirect_binary_predicate_impl_)(F, I1, I2),
        predicate<F &, iter_value_t<I1> &, iter_value_t<I2> &> AND
        predicate<F &, iter_value_t<I1> &, iter_reference_t<I2>> AND
        predicate<F &, iter_reference_t<I1>, iter_value_t<I2> &> AND
        predicate<F &, iter_reference_t<I1>, iter_reference_t<I2>> AND
        predicate<F &, iter_common_reference_t<I1>, iter_common_reference_t<I2>>
    );

    template<typename F, typename I1, typename I2>
    CPP_concept indirect_binary_predicate_ =
        indirectly_readable<I1> && indirectly_readable<I2> &&
        copy_constructible<F> &&
        CPP_concept_ref(::futures::detail::indirect_binary_predicate_impl_, F, I1, I2);

    template(typename F, typename I1, typename I2)(
    concept (indirect_relation_)(F, I1, I2),
        relation<F &, iter_value_t<I1> &, iter_value_t<I2> &> AND
        relation<F &, iter_value_t<I1> &, iter_reference_t<I2>> AND
        relation<F &, iter_reference_t<I1>, iter_value_t<I2> &> AND
        relation<F &, iter_reference_t<I1>, iter_reference_t<I2>> AND
        relation<F &, iter_common_reference_t<I1>, iter_common_reference_t<I2>>
    );

    template<typename F, typename I1, typename I2 = I1>
    CPP_concept indirect_relation =
        indirectly_readable<I1> && indirectly_readable<I2> &&
        copy_constructible<F> &&
        CPP_concept_ref(::futures::detail::indirect_relation_, F, I1, I2);

    template(typename F, typename I1, typename I2)(
    concept (indirect_strict_weak_order_)(F, I1, I2),
        strict_weak_order<F &, iter_value_t<I1> &, iter_value_t<I2> &> AND
        strict_weak_order<F &, iter_value_t<I1> &, iter_reference_t<I2>> AND
        strict_weak_order<F &, iter_reference_t<I1>, iter_value_t<I2> &> AND
        strict_weak_order<F &, iter_reference_t<I1>, iter_reference_t<I2>> AND
        strict_weak_order<F &, iter_common_reference_t<I1>, iter_common_reference_t<I2>>
    );

    template<typename F, typename I1, typename I2 = I1>
    CPP_concept indirect_strict_weak_order =
        indirectly_readable<I1> && indirectly_readable<I2> &&
        copy_constructible<F> &&
        CPP_concept_ref(::futures::detail::indirect_strict_weak_order_, F, I1, I2);
    // clang-format on

    //////////////////////////////////////////////////////////////////////////////////////
    // projected struct, for "projecting" a readable with a unary callable
    /// \cond
    namespace ranges_detail {
        RANGES_DIAGNOSTIC_PUSH
        RANGES_DIAGNOSTIC_IGNORE_UNDEFINED_INTERNAL
        template <typename I, typename Proj> struct projected_ {
            struct type {
                using reference = indirect_result_t<Proj &, I>;
                using value_type = uncvref_t<reference>;
                reference operator*() const;
            };
        };
        RANGES_DIAGNOSTIC_POP

        template <typename Proj> struct select_projected_ {
            template <typename I>
            using apply = futures::detail::meta::_t<
                ranges_detail::enable_if_t<(bool)indirectly_regular_unary_invocable<Proj, I>, ranges_detail::projected_<I, Proj>>>;
        };

        template <> struct select_projected_<identity> {
            template <typename I> using apply = ranges_detail::enable_if_t<(bool)indirectly_readable<I>, I>;
        };
    } // namespace ranges_detail
    /// \endcond

    template <typename I, typename Proj> using projected = typename ranges_detail::select_projected_<Proj>::template apply<I>;

    template <typename I, typename Proj>
    struct incrementable_traits<ranges_detail::projected_<I, Proj>> : incrementable_traits<I> {};

    // clang-format off
    template(typename I, typename O)(
    concept (indirectly_movable_)(I, O),
        indirectly_writable<O, iter_rvalue_reference_t<I>>
    );

    template<typename I, typename O>
    CPP_concept indirectly_movable =
        indirectly_readable<I> && CPP_concept_ref(::futures::detail::indirectly_movable_, I, O);

    template(typename I, typename O)(
    concept (indirectly_movable_storable_)(I, O),
        indirectly_writable<O, iter_value_t<I>> AND
        movable<iter_value_t<I>> AND
        constructible_from<iter_value_t<I>, iter_rvalue_reference_t<I>> AND
        assignable_from<iter_value_t<I> &, iter_rvalue_reference_t<I>>
    );

    template<typename I, typename O>
    CPP_concept indirectly_movable_storable =
        indirectly_movable<I, O> &&
        CPP_concept_ref(::futures::detail::indirectly_movable_storable_, I, O);

    template(typename I, typename O)(
    concept (indirectly_copyable_)(I, O),
        indirectly_writable<O, iter_reference_t<I>>
    );

    template<typename I, typename O>
    CPP_concept indirectly_copyable =
        indirectly_readable<I> && CPP_concept_ref(::futures::detail::indirectly_copyable_, I, O);

    template(typename I, typename O)(
    concept (indirectly_copyable_storable_)(I, O),
        indirectly_writable<O, iter_value_t<I> const &> AND
        copyable<iter_value_t<I>> AND
        constructible_from<iter_value_t<I>, iter_reference_t<I>> AND
        assignable_from<iter_value_t<I> &, iter_reference_t<I>>
    );

    template<typename I, typename O>
    CPP_concept indirectly_copyable_storable =
        indirectly_copyable<I, O> &&
        CPP_concept_ref(::futures::detail::indirectly_copyable_storable_, I, O);

    template<typename I1, typename I2>
    CPP_requires(indirectly_swappable_,
        requires(I1 const i1, I2 const i2) //
        (
            ::futures::detail::iter_swap(i1, i2),
            ::futures::detail::iter_swap(i1, i1),
            ::futures::detail::iter_swap(i2, i2),
            ::futures::detail::iter_swap(i2, i1)
        ));
    template<typename I1, typename I2 = I1>
    CPP_concept indirectly_swappable =
        indirectly_readable<I1> && //
        indirectly_readable<I2> && //
        CPP_requires_ref(::futures::detail::indirectly_swappable_, I1, I2);

    template(typename C, typename I1, typename P1, typename I2, typename P2)(
    concept (projected_indirect_relation_)(C, I1, P1, I2, P2),
        indirect_relation<C, projected<I1, P1>, projected<I2, P2>>
    );

    template<typename I1, typename I2, typename C, typename P1 = identity,
        typename P2 = identity>
    CPP_concept indirectly_comparable =
        CPP_concept_ref(::futures::detail::projected_indirect_relation_, C, I1, P1, I2, P2);

    //////////////////////////////////////////////////////////////////////////////////////
    // Composite concepts for use defining algorithms:
    template<typename I>
    CPP_concept permutable =
        forward_iterator<I> &&
        indirectly_swappable<I, I> &&
        indirectly_movable_storable<I, I>;

    template(typename C, typename I1, typename P1, typename I2, typename P2)(
    concept (projected_indirect_strict_weak_order_)(C, I1, P1, I2, P2),
        indirect_strict_weak_order<C, projected<I1, P1>, projected<I2, P2>>
    );

    template<typename I1, typename I2, typename Out, typename C = less,
        typename P1 = identity, typename P2 = identity>
    CPP_concept mergeable =
        input_iterator<I1> &&
        input_iterator<I2> &&
        weakly_incrementable<Out> &&
        indirectly_copyable<I1, Out> &&
        indirectly_copyable<I2, Out> &&
        CPP_concept_ref(::futures::detail::projected_indirect_strict_weak_order_, C, I1, P1, I2, P2);

    template<typename I, typename C = less, typename P = identity>
    CPP_concept sortable =
        permutable<I> &&
        CPP_concept_ref(::futures::detail::projected_indirect_strict_weak_order_, C, I, P, I, P);
    // clang-format on

    struct sentinel_tag {};
    struct sized_sentinel_tag : sentinel_tag {};

    template <typename S, typename I>
    using sentinel_tag_of =               //
        std::enable_if_t<                 //
            sentinel_for<S, I>,           //
            futures::detail::meta::conditional_t<          //
                sized_sentinel_for<S, I>, //
                sized_sentinel_tag,       //
                sentinel_tag>>;

    // Deprecated things:
    /// \cond
    template <typename I>
    using iterator_category RANGES_DEPRECATED("iterator_category is deprecated. Use the iterator concepts instead") =
        ranges_detail::iterator_category<I>;

    template <typename I>
    using iterator_category_t
        RANGES_DEPRECATED("iterator_category_t is deprecated. Use the iterator concepts instead") =
            futures::detail::meta::_t<ranges_detail::iterator_category<I>>;

    template <typename Fun, typename... Is>
    using indirect_invoke_result_t
        RANGES_DEPRECATED("Please switch to indirect_result_t") = indirect_result_t<Fun, Is...>;

    template <typename Fun, typename... Is>
    struct RANGES_DEPRECATED("Please switch to indirect_result_t") indirect_invoke_result
        : futures::detail::meta::defer<indirect_result_t, Fun, Is...> {};

    template <typename Sig> struct indirect_result_of {};

    template <typename Fun, typename... Is>
    struct RANGES_DEPRECATED("Please switch to indirect_result_t") indirect_result_of<Fun(Is...)>
        : futures::detail::meta::defer<indirect_result_t, Fun, Is...> {};

    template <typename Sig>
    using indirect_result_of_t
        RANGES_DEPRECATED("Please switch to indirect_result_t") = futures::detail::meta::_t<indirect_result_of<Sig>>;
    /// \endcond

    namespace cpp20 {
        using ::futures::detail::bidirectional_iterator;
        using ::futures::detail::contiguous_iterator;
        using ::futures::detail::forward_iterator;
        using ::futures::detail::incrementable;
        using ::futures::detail::indirect_relation;
        using ::futures::detail::indirect_result_t;
        using ::futures::detail::indirect_strict_weak_order;
        using ::futures::detail::indirect_unary_predicate;
        using ::futures::detail::indirectly_comparable;
        using ::futures::detail::indirectly_copyable;
        using ::futures::detail::indirectly_copyable_storable;
        using ::futures::detail::indirectly_movable;
        using ::futures::detail::indirectly_movable_storable;
        using ::futures::detail::indirectly_readable;
        using ::futures::detail::indirectly_regular_unary_invocable;
        using ::futures::detail::indirectly_swappable;
        using ::futures::detail::indirectly_unary_invocable;
        using ::futures::detail::indirectly_writable;
        using ::futures::detail::input_iterator;
        using ::futures::detail::input_or_output_iterator;
        using ::futures::detail::mergeable;
        using ::futures::detail::output_iterator;
        using ::futures::detail::permutable;
        using ::futures::detail::projected;
        using ::futures::detail::random_access_iterator;
        using ::futures::detail::sentinel_for;
        using ::futures::detail::sized_sentinel_for;
        using ::futures::detail::sortable;
        using ::futures::detail::weakly_incrementable;
    } // namespace cpp20
    /// @}
} // namespace futures::detail

#ifdef _GLIBCXX_DEBUG
// HACKHACK: workaround underconstrained operator- for libstdc++ debug iterator wrapper
// by intentionally creating an ambiguity when the wrapped types don't support the
// necessary operation.
namespace __gnu_debug {
    template(typename I1, typename I2, typename Seq)(
        /// \pre
        requires(!::futures::detail::sized_sentinel_for<I1, I2>)) //
        void
        operator-(_Safe_iterator<I1, Seq> const &, _Safe_iterator<I2, Seq> const &) = delete;

    template(typename I1, typename Seq)(
        /// \pre
        requires(!::futures::detail::sized_sentinel_for<I1, I1>)) //
        void
        operator-(_Safe_iterator<I1, Seq> const &, _Safe_iterator<I1, Seq> const &) = delete;
} // namespace __gnu_debug
#endif

#if defined(__GLIBCXX__) || (defined(_LIBCPP_VERSION) && _LIBCPP_VERSION <= 3900)
// HACKHACK: workaround libc++ (https://llvm.org/bugs/show_bug.cgi?id=28421)
// and libstdc++ (https://gcc.gnu.org/bugzilla/show_bug.cgi?id=71771)
// underconstrained operator- for reverse_iterator by disabling sized_sentinel_for
// when the base iterators do not model sized_sentinel_for.

namespace futures::detail {
    template <typename S, typename I>
    /*inline*/ constexpr bool disable_sized_sentinel<std::reverse_iterator<S>, std::reverse_iterator<I>> =
        !static_cast<bool>(sized_sentinel_for<I, S>);
} // namespace futures::detail

#endif // defined(__GLIBCXX__) || (defined(_LIBCPP_VERSION) && _LIBCPP_VERSION <= 3900)

// #include <futures/algorithm/detail/traits/range/detail/epilogue.h>
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

#ifndef RANGES_PROLOGUE_INCLUDED
#error "Including epilogue, but prologue not included!"
#endif
#undef RANGES_PROLOGUE_INCLUDED

#undef template
#undef AND

RANGES_DIAGNOSTIC_POP


#endif // FUTURES_RANGES_ITERATOR_CONCEPTS_HPP

// #include <futures/algorithm/detail/traits/range/iterator/traits.h>

// #include <futures/algorithm/detail/traits/range/range/access.h>
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

#ifndef FUTURES_RANGES_RANGE_ACCESS_HPP
#define FUTURES_RANGES_RANGE_ACCESS_HPP

// #include <functional>
 // for reference_wrapper (whose use with begin/end is deprecated)
// #include <initializer_list>

// #include <iterator>

#include <limits>
// #include <utility>


#ifdef __has_include
#if __has_include(<span>)
namespace std {
    template <class T, std::size_t Extent> class span;
}
#endif
#if __has_include(<string_view>)
// #include <string_view>

#endif
#endif

// #include <futures/algorithm/detail/traits/range/range_fwd.h>


// #include <futures/algorithm/detail/traits/range/iterator/concepts.h>

// #include <futures/algorithm/detail/traits/range/iterator/reverse_iterator.h>
/// \file
// Range v3 library
//
//  Copyright Eric Niebler 2014-present
//
//  Use, modification and distribution is subject to the
//  Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
// Project home: https://github.com/ericniebler/range-v3
//
#ifndef FUTURES_RANGES_ITERATOR_REVERSE_ITERATOR_HPP
#define FUTURES_RANGES_ITERATOR_REVERSE_ITERATOR_HPP

// #include <utility>


// #include <futures/algorithm/detail/traits/range/range_fwd.h>


// #include <futures/algorithm/detail/traits/range/iterator/basic_iterator.h>
/// \file
// Range v3 library
//
//  Copyright Eric Niebler 2014-present
//  Copyright Casey Carter 2016
//
//  Use, modification and distribution is subject to the
//  Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
// Project home: https://github.com/ericniebler/range-v3
//
#ifndef FUTURES_RANGES_ITERATOR_BASIC_ITERATOR_HPP
#define FUTURES_RANGES_ITERATOR_BASIC_ITERATOR_HPP

// #include <type_traits>

// #include <utility>


// #include <futures/algorithm/detail/traits/range/meta/meta.h>


// #include <futures/algorithm/detail/traits/range/concepts/concepts.h>


// #include <futures/algorithm/detail/traits/range/range_fwd.h>


// #include <futures/algorithm/detail/traits/range/detail/range_access.h>
/// \file
// Range v3 library
//
//  Copyright Eric Niebler 2014-present
//  Copyright Casey Carter 2016
//
//  Use, modification and distribution is subject to the
//  Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
// Project home: https://github.com/ericniebler/range-v3
//

#ifndef FUTURES_RANGES_DETAIL_RANGE_ACCESS_HPP
#define FUTURES_RANGES_DETAIL_RANGE_ACCESS_HPP

// #include <cstddef>

// #include <utility>


// #include <futures/algorithm/detail/traits/range/meta/meta.h>


// #include <futures/algorithm/detail/traits/range/concepts/concepts.h>


// #include <futures/algorithm/detail/traits/range/range_fwd.h>


// #include <futures/algorithm/detail/traits/range/iterator/concepts.h>


// #include <futures/algorithm/detail/traits/range/detail/prologue.h>
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

#ifndef FUTURES_RANGES_DETAIL_PROLOGUE_HPP
#define FUTURES_RANGES_DETAIL_PROLOGUE_HPP
// #include <futures/algorithm/detail/traits/range/detail/config.h>

#endif

#ifdef RANGES_PROLOGUE_INCLUDED
#error "Prologue already included!"
#endif
#define RANGES_PROLOGUE_INCLUDED

RANGES_DIAGNOSTIC_PUSH

#ifdef RANGES_FEWER_WARNINGS
RANGES_DIAGNOSTIC_IGNORE_UNDEFINED_INTERNAL
RANGES_DIAGNOSTIC_IGNORE_INDENTATION
RANGES_DIAGNOSTIC_IGNORE_CXX17_COMPAT
#endif

RANGES_DIAGNOSTIC_KEYWORD_MACRO

#define template(...)                                                                                                  \
    CPP_PP_IGNORE_CXX2A_COMPAT_BEGIN                                                                                   \
    template <__VA_ARGS__ CPP_TEMPLATE_AUX_

#define AND CPP_and


namespace futures::detail {
    /// \addtogroup group-views
    /// @{
    struct range_access {
        /// \cond
      private:
        template <typename T> static std::false_type single_pass_2_(long);
        template <typename T> static typename T::single_pass single_pass_2_(int);

        template <typename T> struct single_pass_ { using type = decltype(range_access::single_pass_2_<T>(42)); };

        template <typename T> static std::false_type contiguous_2_(long);
        template <typename T> static typename T::contiguous contiguous_2_(int);

        template <typename T> struct contiguous_ { using type = decltype(range_access::contiguous_2_<T>(42)); };

        template <typename T> static basic_mixin<T> mixin_base_2_(long);
        template <typename T> static typename T::mixin mixin_base_2_(int);

        template <typename Cur> struct mixin_base_ { using type = decltype(range_access::mixin_base_2_<Cur>(42)); };

      public:
        template <typename Cur> using single_pass_t = futures::detail::meta::_t<single_pass_<Cur>>;

        template <typename Cur> using contiguous_t = futures::detail::meta::_t<contiguous_<Cur>>;

        template <typename Cur> using mixin_base_t = futures::detail::meta::_t<mixin_base_<Cur>>;

        // clang-format off
        template<typename Rng>
        static constexpr auto CPP_auto_fun(begin_cursor)(Rng &rng)
        (
            return rng.begin_cursor()
        )
        template<typename Rng>
        static constexpr auto CPP_auto_fun(end_cursor)(Rng &rng)
        (
            return rng.end_cursor()
        )

        template<typename Rng>
        static constexpr auto CPP_auto_fun(begin_adaptor)(Rng &rng)
        (
            return rng.begin_adaptor()
        )
        template<typename Rng>
        static constexpr auto CPP_auto_fun(end_adaptor)(Rng &rng)
        (
            return rng.end_adaptor()
        )

        template<typename Cur>
        static constexpr auto CPP_auto_fun(read)(Cur const &pos)
        (
            return pos.read()
        )
        template<typename Cur>
        static constexpr auto CPP_auto_fun(arrow)(Cur const &pos)
        (
            return pos.arrow()
        )
        template<typename Cur>
        static constexpr auto CPP_auto_fun(move)(Cur const &pos)
        (
            return pos.move()
        )
        template<typename Cur, typename T>
        static constexpr auto CPP_auto_fun(write)(Cur &pos, T &&t)
        (
            return pos.write((T &&) t)
        )
        template<typename Cur>
        static constexpr auto CPP_auto_fun(next)(Cur & pos)
        (
            return pos.next()
        )
        template<typename Cur, typename O>
        static constexpr auto CPP_auto_fun(equal)(Cur const &pos, O const &other)
        (
            return pos.equal(other)
        )
        template<typename Cur>
        static constexpr auto CPP_auto_fun(prev)(Cur & pos)
        (
            return pos.prev()
        )
        template<typename Cur, typename D>
        static constexpr auto CPP_auto_fun(advance)(Cur & pos, D n)
        (
            return pos.advance(n)
        )
        template<typename Cur, typename O>
        static constexpr auto CPP_auto_fun(distance_to)(Cur const &pos, O const &other)
        (
            return pos.distance_to(other)
        )

    private:
        template<typename Cur>
        using sized_cursor_difference_t = decltype(
            range_access::distance_to(std::declval<Cur>(), std::declval<Cur>()));
        // clang-format on

        template <typename T> static std::ptrdiff_t cursor_difference_2_(ranges_detail::ignore_t);
        template <typename T> static sized_cursor_difference_t<T> cursor_difference_2_(long);
        template <typename T> static typename T::difference_type cursor_difference_2_(int);

        template <typename T> using cursor_reference_t = decltype(std::declval<T const &>().read());

        template <typename T> static futures::detail::meta::id<uncvref_t<cursor_reference_t<T>>> cursor_value_2_(long);
        template <typename T> static futures::detail::meta::id<typename T::value_type> cursor_value_2_(int);

#ifdef RANGES_WORKAROUND_CWG_1554
        template <typename Cur> struct cursor_difference {
            using type = decltype(range_access::cursor_difference_2_<Cur>(42));
        };

        template <typename Cur> struct cursor_value : decltype(range_access::cursor_value_2_<Cur>(42)) {};
#endif // RANGES_WORKAROUND_CWG_1554
      public:
#ifdef RANGES_WORKAROUND_CWG_1554
        template <typename Cur> using cursor_difference_t = futures::detail::meta::_t<cursor_difference<Cur>>;

        template <typename Cur> using cursor_value_t = futures::detail::meta::_t<cursor_value<Cur>>;
#else  // ^^^ workaround ^^^ / vvv no workaround vvv
        template <typename Cur> using cursor_difference_t = decltype(range_access::cursor_difference_2_<Cur>(42));

        template <typename Cur> using cursor_value_t = futures::detail::meta::_t<decltype(range_access::cursor_value_2_<Cur>(42))>;
#endif // RANGES_WORKAROUND_CWG_1554

        template <typename Cur> static constexpr Cur &pos(basic_iterator<Cur> &it) noexcept { return it.pos(); }
        template <typename Cur> static constexpr Cur const &pos(basic_iterator<Cur> const &it) noexcept {
            return it.pos();
        }
        template <typename Cur> static constexpr Cur &&pos(basic_iterator<Cur> &&it) noexcept {
            return ranges_detail::move(it.pos());
        }

        template <typename Cur> static constexpr Cur cursor(basic_iterator<Cur> it) { return std::move(it.pos()); }
        /// endcond
    };
    /// @}

    /// \cond
    namespace ranges_detail {
        //
        // Concepts that the range cursor must model
        // clang-format off
        //
        template<typename T>
        CPP_concept cursor =
            semiregular<T> && semiregular<range_access::mixin_base_t<T>> &&
            constructible_from<range_access::mixin_base_t<T>, T> &&
            constructible_from<range_access::mixin_base_t<T>, T const &>;
            // Axiom: mixin_base_t<T> has a member get(), accessible to derived classes,
            //   which perfectly-returns the contained cursor object and does not throw
            //   exceptions.

        template<typename T>
        CPP_requires(has_cursor_next_,
            requires(T & t)
            (
                range_access::next(t)
            ));
        template<typename T>
        CPP_concept has_cursor_next = CPP_requires_ref(ranges_detail::has_cursor_next_, T);

        template<typename S, typename C>
        CPP_requires(sentinel_for_cursor_,
            requires(S & s, C & c) //
            (
                range_access::equal(c, s),
                concepts::requires_<convertible_to<decltype(
                    range_access::equal(c, s)), bool>>
            ));
        template<typename S, typename C>
        CPP_concept sentinel_for_cursor =
            semiregular<S> &&
            cursor<C> &&
            CPP_requires_ref(ranges_detail::sentinel_for_cursor_, S, C);

        template<typename T>
        CPP_requires(readable_cursor_,
            requires(T & t) //
            (
                range_access::read(t)
            ));
        template<typename T>
        CPP_concept readable_cursor = CPP_requires_ref(ranges_detail::readable_cursor_, T);

        template<typename T>
        CPP_requires(has_cursor_arrow_,
            requires(T const & t) //
            (
                range_access::arrow(t)
            ));
        template<typename T>
        CPP_concept has_cursor_arrow = CPP_requires_ref(ranges_detail::has_cursor_arrow_, T);

        template<typename T, typename U>
        CPP_requires(writable_cursor_,
            requires(T & t, U && u) //
            (
                range_access::write(t, (U &&) u)
            ));
        template<typename T, typename U>
        CPP_concept writable_cursor =
            CPP_requires_ref(ranges_detail::writable_cursor_, T, U);

        template<typename S, typename C>
        CPP_requires(sized_sentinel_for_cursor_,
            requires(S & s, C & c) //
            (
                range_access::distance_to(c, s),
                concepts::requires_<signed_integer_like_<decltype(
                    range_access::distance_to(c, s))>>
            )
        );
        template<typename S, typename C>
        CPP_concept sized_sentinel_for_cursor =
            sentinel_for_cursor<S, C> &&
            CPP_requires_ref(ranges_detail::sized_sentinel_for_cursor_, S, C);

        template<typename T, typename U>
        CPP_concept output_cursor =
            writable_cursor<T, U> && cursor<T>;

        template<typename T>
        CPP_concept input_cursor =
            readable_cursor<T> && cursor<T> && has_cursor_next<T>;

        template<typename T>
        CPP_concept forward_cursor =
            input_cursor<T> && sentinel_for_cursor<T, T> &&
            !range_access::single_pass_t<uncvref_t<T>>::value;

        template<typename T>
        CPP_requires(bidirectional_cursor_,
            requires(T & t) //
            (
                range_access::prev(t)
            ));
        template<typename T>
        CPP_concept bidirectional_cursor =
            forward_cursor<T> &&
            CPP_requires_ref(ranges_detail::bidirectional_cursor_, T);

        template<typename T>
        CPP_requires(random_access_cursor_,
            requires(T & t) //
            (
                range_access::advance(t, range_access::distance_to(t, t))
            ));
        template<typename T>
        CPP_concept random_access_cursor =
            bidirectional_cursor<T> && //
            sized_sentinel_for_cursor<T, T> && //
            CPP_requires_ref(ranges_detail::random_access_cursor_, T);

        template(class T)(
            /// \pre
            requires std::is_lvalue_reference<T>::value)
        void is_lvalue_reference(T&&);

        template<typename T>
        CPP_requires(contiguous_cursor_,
            requires(T & t) //
            (
                ranges_detail::is_lvalue_reference(range_access::read(t))
            ));
        template<typename T>
        CPP_concept contiguous_cursor =
            random_access_cursor<T> && //
            range_access::contiguous_t<uncvref_t<T>>::value && //
            CPP_requires_ref(ranges_detail::contiguous_cursor_, T);
        // clang-format on

        template <typename Cur, bool IsReadable> RANGES_INLINE_VAR constexpr bool is_writable_cursor_ = true;

        template <typename Cur>
        RANGES_INLINE_VAR constexpr bool
            is_writable_cursor_<Cur, true> = (bool)writable_cursor<Cur, range_access::cursor_value_t<Cur>>;

        template <typename Cur>
        RANGES_INLINE_VAR constexpr bool is_writable_cursor_v = is_writable_cursor_<Cur, (bool)readable_cursor<Cur>>;
    } // namespace ranges_detail
    /// \endcond
} // namespace futures::detail

// #include <futures/algorithm/detail/traits/range/detail/epilogue.h>
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

#ifndef RANGES_PROLOGUE_INCLUDED
#error "Including epilogue, but prologue not included!"
#endif
#undef RANGES_PROLOGUE_INCLUDED

#undef template
#undef AND

RANGES_DIAGNOSTIC_POP


#endif

// #include <futures/algorithm/detail/traits/range/iterator/concepts.h>

// #include <futures/algorithm/detail/traits/range/iterator/traits.h>

// #include <futures/algorithm/detail/traits/range/utility/addressof.h>
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
#ifndef FUTURES_RANGES_UTILITY_ADDRESSOF_HPP
#define FUTURES_RANGES_UTILITY_ADDRESSOF_HPP

// #include <memory>

// #include <type_traits>


// #include <futures/algorithm/detail/traits/range/concepts/concepts.h>


// #include <futures/algorithm/detail/traits/range/range_fwd.h>


// #include <futures/algorithm/detail/traits/range/detail/config.h>


// #include <futures/algorithm/detail/traits/range/detail/prologue.h>
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

#ifndef FUTURES_RANGES_DETAIL_PROLOGUE_HPP
#define FUTURES_RANGES_DETAIL_PROLOGUE_HPP
// #include <futures/algorithm/detail/traits/range/detail/config.h>

#endif

#ifdef RANGES_PROLOGUE_INCLUDED
#error "Prologue already included!"
#endif
#define RANGES_PROLOGUE_INCLUDED

RANGES_DIAGNOSTIC_PUSH

#ifdef RANGES_FEWER_WARNINGS
RANGES_DIAGNOSTIC_IGNORE_UNDEFINED_INTERNAL
RANGES_DIAGNOSTIC_IGNORE_INDENTATION
RANGES_DIAGNOSTIC_IGNORE_CXX17_COMPAT
#endif

RANGES_DIAGNOSTIC_KEYWORD_MACRO

#define template(...)                                                                                                  \
    CPP_PP_IGNORE_CXX2A_COMPAT_BEGIN                                                                                   \
    template <__VA_ARGS__ CPP_TEMPLATE_AUX_

#define AND CPP_and


namespace futures::detail {
    /// \cond
    namespace ranges_detail {
#ifdef __cpp_lib_addressof_constexpr
        using std::addressof;
#else
        namespace check_addressof {
            inline ignore_t operator&(ignore_t) { return {}; }
            template <typename T> auto addressof(T &t) { return &t; }
        } // namespace check_addressof

        template <typename T> constexpr bool has_bad_addressof() {
            return !std::is_scalar<T>::value &&
                   !RANGES_IS_SAME(decltype(check_addressof::addressof(*(T *)nullptr)), ignore_t);
        }

        template(typename T)(
            /// \pre
            requires(has_bad_addressof<T>())) T *addressof(T &arg) noexcept {
            return std::addressof(arg);
        }

        template(typename T)(
            /// \pre
            requires(!has_bad_addressof<T>())) constexpr T *addressof(T &arg) noexcept {
            return &arg;
        }

        template <typename T> T const *addressof(T const &&) = delete;
#endif
    } // namespace ranges_detail
    /// \endcond
} // namespace futures::detail

// #include <futures/algorithm/detail/traits/range/detail/epilogue.h>
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

#ifndef RANGES_PROLOGUE_INCLUDED
#error "Including epilogue, but prologue not included!"
#endif
#undef RANGES_PROLOGUE_INCLUDED

#undef template
#undef AND

RANGES_DIAGNOSTIC_POP


#endif

// #include <futures/algorithm/detail/traits/range/utility/box.h>
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

#ifndef FUTURES_RANGES_UTILITY_BOX_HPP
#define FUTURES_RANGES_UTILITY_BOX_HPP

// #include <cstdlib>

// #include <type_traits>

// #include <utility>


// #include <futures/algorithm/detail/traits/range/meta/meta.h>


// #include <futures/algorithm/detail/traits/range/concepts/concepts.h>


// #include <futures/algorithm/detail/traits/range/range_fwd.h>


// #include <futures/algorithm/detail/traits/range/utility/get.h>
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

#ifndef FUTURES_RANGES_UTILITY_GET_HPP
#define FUTURES_RANGES_UTILITY_GET_HPP

// #include <futures/algorithm/detail/traits/range/meta/meta.h>


// #include <futures/algorithm/detail/traits/range/concepts/concepts.h>


// #include <futures/algorithm/detail/traits/range/detail/adl_get.h>
// Range v3 library
//
//  Copyright Casey Carter 2018
//
//  Use, modification and distribution is subject to the
//  Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
// Project home: https://github.com/ericniebler/range-v3

#ifndef FUTURES_RANGES_DETAIL_ADL_GET_HPP
#define FUTURES_RANGES_DETAIL_ADL_GET_HPP

// #include <cstddef>


// #include <futures/algorithm/detail/traits/range/concepts/concepts.h>


// #include <futures/algorithm/detail/traits/range/range_fwd.h>


// #include <futures/algorithm/detail/traits/range/detail/prologue.h>
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

#ifndef FUTURES_RANGES_DETAIL_PROLOGUE_HPP
#define FUTURES_RANGES_DETAIL_PROLOGUE_HPP
// #include <futures/algorithm/detail/traits/range/detail/config.h>

#endif

#ifdef RANGES_PROLOGUE_INCLUDED
#error "Prologue already included!"
#endif
#define RANGES_PROLOGUE_INCLUDED

RANGES_DIAGNOSTIC_PUSH

#ifdef RANGES_FEWER_WARNINGS
RANGES_DIAGNOSTIC_IGNORE_UNDEFINED_INTERNAL
RANGES_DIAGNOSTIC_IGNORE_INDENTATION
RANGES_DIAGNOSTIC_IGNORE_CXX17_COMPAT
#endif

RANGES_DIAGNOSTIC_KEYWORD_MACRO

#define template(...)                                                                                                  \
    CPP_PP_IGNORE_CXX2A_COMPAT_BEGIN                                                                                   \
    template <__VA_ARGS__ CPP_TEMPLATE_AUX_

#define AND CPP_and


namespace futures::detail {
    /// \cond
    namespace ranges_detail {
        namespace _adl_get_ {
            template <typename> void get();

            template <std::size_t I, typename TupleLike>
            constexpr auto adl_get(TupleLike &&t) noexcept -> decltype(get<I>(static_cast<TupleLike &&>(t))) {
                return get<I>(static_cast<TupleLike &&>(t));
            }
            template <typename T, typename TupleLike>
            constexpr auto adl_get(TupleLike &&t) noexcept -> decltype(get<T>(static_cast<TupleLike &&>(t))) {
                return get<T>(static_cast<TupleLike &&>(t));
            }
        } // namespace _adl_get_
        using _adl_get_::adl_get;
    } // namespace ranges_detail

    namespace _tuple_wrapper_ {
        template <typename TupleLike> struct forward_tuple_interface : TupleLike {
            forward_tuple_interface() = default;
            using TupleLike::TupleLike;
#if !defined(__clang__) || __clang_major__ > 3
            CPP_member constexpr CPP_ctor(forward_tuple_interface)(TupleLike &&base)( //
                noexcept(std::is_nothrow_move_constructible<TupleLike>::value)        //
                requires move_constructible<TupleLike>)
                : TupleLike(static_cast<TupleLike &&>(base)) {}
            CPP_member constexpr CPP_ctor(forward_tuple_interface)(TupleLike const &base)( //
                noexcept(std::is_nothrow_copy_constructible<TupleLike>::value)             //
                requires copy_constructible<TupleLike>)
                : TupleLike(base) {}
#else
            // Clang 3.x have a problem with inheriting constructors
            // that causes the declarations in the preceeding PP block to get
            // instantiated too early.
            template(typename B = TupleLike)(
                /// \pre
                requires move_constructible<
                    B>) constexpr forward_tuple_interface(TupleLike &&base) noexcept(std::
                                                                                         is_nothrow_move_constructible<
                                                                                             TupleLike>::value)
                : TupleLike(static_cast<TupleLike &&>(base)) {}
            template(typename B = TupleLike)(
                /// \pre
                requires copy_constructible<
                    B>) constexpr forward_tuple_interface(TupleLike const
                                                              &base) noexcept(std::
                                                                                  is_nothrow_copy_constructible<
                                                                                      TupleLike>::value)
                : TupleLike(base) {}
#endif

            // clang-format off
            template<std::size_t I, typename U = TupleLike>
            friend constexpr auto CPP_auto_fun(get)(
                forward_tuple_interface<TupleLike> &wb)
            (
                return ranges_detail::adl_get<I>(static_cast<U &>(wb))
            )
            template<std::size_t I, typename U = TupleLike>
            friend constexpr auto CPP_auto_fun(get)(
                forward_tuple_interface<TupleLike> const &wb)
            (
                return ranges_detail::adl_get<I>(static_cast<U const &>(wb))
            )
            template<std::size_t I, typename U = TupleLike>
            friend constexpr auto CPP_auto_fun(get)(
                forward_tuple_interface<TupleLike> &&wb)
            (
                return ranges_detail::adl_get<I>(static_cast<U &&>(wb))
            )
            template<std::size_t I, typename U = TupleLike>
            friend constexpr auto CPP_auto_fun(get)(
                forward_tuple_interface<TupleLike> const &&wb)
            (
                return ranges_detail::adl_get<I>(static_cast<U const &&>(wb))
            )
            template<typename T, typename U = TupleLike>
            friend constexpr auto CPP_auto_fun(get)(
                forward_tuple_interface<TupleLike> &wb)
            (
                return ranges_detail::adl_get<T>(static_cast<U &>(wb))
            )
            template<typename T, typename U = TupleLike>
            friend constexpr auto CPP_auto_fun(get)(
                forward_tuple_interface<TupleLike> const &wb)
            (
                return ranges_detail::adl_get<T>(static_cast<U const &>(wb))
            )
            template<typename T, typename U = TupleLike>
            friend constexpr auto CPP_auto_fun(get)(
                forward_tuple_interface<TupleLike> &&wb)
            (
                return ranges_detail::adl_get<T>(static_cast<U &&>(wb))
            )
            template<typename T, typename U = TupleLike>
            friend constexpr auto CPP_auto_fun(get)(
                forward_tuple_interface<TupleLike> const &&wb)
            (
                return ranges_detail::adl_get<T>(static_cast<U const &&>(wb))
            )
            // clang-format on
        };
    } // namespace _tuple_wrapper_
    /// \endcond
} // namespace futures::detail

// #include <futures/algorithm/detail/traits/range/detail/epilogue.h>
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

#ifndef RANGES_PROLOGUE_INCLUDED
#error "Including epilogue, but prologue not included!"
#endif
#undef RANGES_PROLOGUE_INCLUDED

#undef template
#undef AND

RANGES_DIAGNOSTIC_POP


#endif // FUTURES_RANGES_DETAIL_ADL_GET_HPP


// #include <futures/algorithm/detail/traits/range/detail/prologue.h>
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

#ifndef FUTURES_RANGES_DETAIL_PROLOGUE_HPP
#define FUTURES_RANGES_DETAIL_PROLOGUE_HPP
// #include <futures/algorithm/detail/traits/range/detail/config.h>

#endif

#ifdef RANGES_PROLOGUE_INCLUDED
#error "Prologue already included!"
#endif
#define RANGES_PROLOGUE_INCLUDED

RANGES_DIAGNOSTIC_PUSH

#ifdef RANGES_FEWER_WARNINGS
RANGES_DIAGNOSTIC_IGNORE_UNDEFINED_INTERNAL
RANGES_DIAGNOSTIC_IGNORE_INDENTATION
RANGES_DIAGNOSTIC_IGNORE_CXX17_COMPAT
#endif

RANGES_DIAGNOSTIC_KEYWORD_MACRO

#define template(...)                                                                                                  \
    CPP_PP_IGNORE_CXX2A_COMPAT_BEGIN                                                                                   \
    template <__VA_ARGS__ CPP_TEMPLATE_AUX_

#define AND CPP_and


namespace futures::detail {
    /// \addtogroup group-utility Utility
    /// @{
    ///

    /// \cond
    namespace _get_ {
        /// \endcond
        // clang-format off
        template<std::size_t I, typename TupleLike>
        constexpr auto CPP_auto_fun(get)(TupleLike &&t)
        (
            return ranges_detail::adl_get<I>(static_cast<TupleLike &&>(t))
        )
        template<typename T, typename TupleLike>
        constexpr auto CPP_auto_fun(get)(TupleLike &&t)
        (
            return ranges_detail::adl_get<T>(static_cast<TupleLike &&>(t))
        )
            // clang-format on

            template <typename T>
            T &get(futures::detail::meta::id_t<T> &value) noexcept {
            return value;
        }
        template <typename T> T const &get(futures::detail::meta::id_t<T> const &value) noexcept { return value; }
        template <typename T> T &&get(futures::detail::meta::id_t<T> &&value) noexcept { return static_cast<T &&>(value); }
        /// \cond
    } // namespace _get_
    using namespace _get_;
    /// \endcond

    /// @}
} // namespace futures::detail

// #include <futures/algorithm/detail/traits/range/detail/epilogue.h>
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

#ifndef RANGES_PROLOGUE_INCLUDED
#error "Including epilogue, but prologue not included!"
#endif
#undef RANGES_PROLOGUE_INCLUDED

#undef template
#undef AND

RANGES_DIAGNOSTIC_POP


#endif


// #include <futures/algorithm/detail/traits/range/detail/prologue.h>
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

#ifndef FUTURES_RANGES_DETAIL_PROLOGUE_HPP
#define FUTURES_RANGES_DETAIL_PROLOGUE_HPP
// #include <futures/algorithm/detail/traits/range/detail/config.h>

#endif

#ifdef RANGES_PROLOGUE_INCLUDED
#error "Prologue already included!"
#endif
#define RANGES_PROLOGUE_INCLUDED

RANGES_DIAGNOSTIC_PUSH

#ifdef RANGES_FEWER_WARNINGS
RANGES_DIAGNOSTIC_IGNORE_UNDEFINED_INTERNAL
RANGES_DIAGNOSTIC_IGNORE_INDENTATION
RANGES_DIAGNOSTIC_IGNORE_CXX17_COMPAT
#endif

RANGES_DIAGNOSTIC_KEYWORD_MACRO

#define template(...)                                                                                                  \
    CPP_PP_IGNORE_CXX2A_COMPAT_BEGIN                                                                                   \
    template <__VA_ARGS__ CPP_TEMPLATE_AUX_

#define AND CPP_and


RANGES_DIAGNOSTIC_PUSH
RANGES_DIAGNOSTIC_IGNORE_DEPRECATED_DECLARATIONS

namespace futures::detail {
    /// \addtogroup group-utility Utility
    /// @{
    ///

    /// \cond
    template <typename T> struct RANGES_DEPRECATED("The futures::detail::mutable_ class template is deprecated") mutable_ {
        mutable T value;

        CPP_member constexpr CPP_ctor(mutable_)()(
            /// \pre
            requires std::is_default_constructible<T>::value)
            : value{} {}
        constexpr explicit mutable_(T const &t) : value(t) {}
        constexpr explicit mutable_(T &&t) : value(ranges_detail::move(t)) {}
        mutable_ const &operator=(T const &t) const {
            value = t;
            return *this;
        }
        mutable_ const &operator=(T &&t) const {
            value = ranges_detail::move(t);
            return *this;
        }
        constexpr operator T &() const & { return value; }
    };

    template <typename T, T v> struct RANGES_DEPRECATED("The futures::detail::constant class template is deprecated") constant {
        constant() = default;
        constexpr explicit constant(T const &) {}
        constant &operator=(T const &) { return *this; }
        constant const &operator=(T const &) const { return *this; }
        constexpr operator T() const { return v; }
        constexpr T exchange(T const &) const { return v; }
    };
    /// \endcond

    /// \cond
    namespace ranges_detail {
        // "box" has three different implementations that store a T differently:
        enum class box_compress {
            none,    // Nothing special: get() returns a reference to a T member subobject
            ebo,     // Apply Empty Base Optimization: get() returns a reference to a T base
                     // subobject
            coalesce // Coalesce all Ts into one T: get() returns a reference to a static
                     // T singleton
        };

        // Per N4582, lambda closures are *not*:
        // - aggregates             ([expr.prim.lambda]/4)
        // - default constructible_from  ([expr.prim.lambda]/p21)
        // - copy assignable        ([expr.prim.lambda]/p21)
        template <typename Fn>
        using could_be_lambda =
            futures::detail::meta::bool_<!std::is_default_constructible<Fn>::value && !std::is_copy_assignable<Fn>::value>;

        template <typename> constexpr box_compress box_compression_(...) { return box_compress::none; }
        template <typename T,
                  typename = futures::detail::meta::if_<futures::detail::meta::strict_and<std::is_empty<T>,
                                                        futures::detail::meta::bool_<!ranges_detail::is_final_v<T>>
#if defined(__GNUC__) && !defined(__clang__) && __GNUC__ == 6 && __GNUC_MINOR__ < 2
                                                        // GCC 6.0 & 6.1 find empty lambdas' implicit conversion
                                                        // to function pointer when doing overload resolution
                                                        // for function calls. That causes hard errors.
                                                        // https://gcc.gnu.org/bugzilla/show_bug.cgi?id=71117
                                                        ,
                                                        futures::detail::meta::not_<could_be_lambda<T>>
#endif
                                                        >>>
        constexpr box_compress box_compression_(long) {
            return box_compress::ebo;
        }
#ifndef RANGES_WORKAROUND_MSVC_249830
        // MSVC pukes passing non-constant-expression objects to constexpr
        // functions, so do not coalesce.
        template <typename T, typename = futures::detail::meta::if_<futures::detail::meta::strict_and<std::is_empty<T>, ranges_detail::is_trivial<T>>>>
        constexpr box_compress box_compression_(int) {
            return box_compress::coalesce;
        }
#endif
        template <typename T> constexpr box_compress box_compression() { return box_compression_<T>(0); }
    } // namespace ranges_detail
    /// \endcond

    template <typename Element, typename Tag = void, ranges_detail::box_compress = ranges_detail::box_compression<Element>()>
    class box {
        Element value;

      public:
        CPP_member constexpr CPP_ctor(box)()(                               //
            noexcept(std::is_nothrow_default_constructible<Element>::value) //
            requires std::is_default_constructible<Element>::value)
            : value{} {}
#if defined(__cpp_conditional_explicit) && __cpp_conditional_explicit > 0
        template(typename E)(
            /// \pre
            requires(!same_as<box, ranges_detail::decay_t<E>>)
                AND constructible_from<Element, E>) constexpr explicit(!convertible_to<E, Element>)
            box(E &&e) noexcept(std::is_nothrow_constructible<Element, E>::value) //
            : value(static_cast<E &&>(e)) {}
#else
        template(typename E)(
            /// \pre
            requires(!same_as<box, ranges_detail::decay_t<E>>) AND constructible_from<Element, E> AND convertible_to<
                E, Element>) constexpr box(E &&e) noexcept(std::is_nothrow_constructible<Element, E>::value)
            : value(static_cast<E &&>(e)) {}
        template(typename E)(
            /// \pre
            requires(!same_as<box, ranges_detail::decay_t<E>>) AND constructible_from<Element, E> AND(
                !convertible_to<
                    E, Element>)) constexpr explicit box(E &&e) noexcept(std::is_nothrow_constructible<Element,
                                                                                                       E>::value) //
            : value(static_cast<E &&>(e)) {}
#endif

        constexpr Element &get() &noexcept { return value; }
        constexpr Element const &get() const &noexcept { return value; }
        constexpr Element &&get() &&noexcept { return ranges_detail::move(value); }
        constexpr Element const &&get() const &&noexcept { return ranges_detail::move(value); }
    };

    template <typename Element, typename Tag> class box<Element, Tag, ranges_detail::box_compress::ebo> : Element {
      public:
        CPP_member constexpr CPP_ctor(box)()(                               //
            noexcept(std::is_nothrow_default_constructible<Element>::value) //
            requires std::is_default_constructible<Element>::value)
            : Element{} {}
#if defined(__cpp_conditional_explicit) && __cpp_conditional_explicit > 0
        template(typename E)(
            /// \pre
            requires(!same_as<box, ranges_detail::decay_t<E>>)
                AND constructible_from<Element, E>) constexpr explicit(!convertible_to<E, Element>)
            box(E &&e) noexcept(std::is_nothrow_constructible<Element, E>::value) //
            : Element(static_cast<E &&>(e)) {}
#else
        template(typename E)(
            /// \pre
            requires(!same_as<box, ranges_detail::decay_t<E>>) AND constructible_from<Element, E> AND convertible_to<
                E, Element>) constexpr box(E &&e) noexcept(std::is_nothrow_constructible<Element, E>::value) //
            : Element(static_cast<E &&>(e)) {}
        template(typename E)(
            /// \pre
            requires(!same_as<box, ranges_detail::decay_t<E>>) AND constructible_from<Element, E> AND(
                !convertible_to<
                    E, Element>)) constexpr explicit box(E &&e) noexcept(std::is_nothrow_constructible<Element,
                                                                                                       E>::value) //
            : Element(static_cast<E &&>(e)) {}
#endif

        constexpr Element &get() &noexcept { return *this; }
        constexpr Element const &get() const &noexcept { return *this; }
        constexpr Element &&get() &&noexcept { return ranges_detail::move(*this); }
        constexpr Element const &&get() const &&noexcept { return ranges_detail::move(*this); }
    };

    template <typename Element, typename Tag> class box<Element, Tag, ranges_detail::box_compress::coalesce> {
        static Element value;

      public:
        constexpr box() noexcept = default;

#if defined(__cpp_conditional_explicit) && __cpp_conditional_explicit > 0
        template(typename E)(
            /// \pre
            requires(!same_as<box, ranges_detail::decay_t<E>>)
                AND constructible_from<Element, E>) constexpr explicit(!convertible_to<E, Element>) box(E &&) noexcept {
        }
#else
        template(typename E)(
            /// \pre
            requires(!same_as<box, ranges_detail::decay_t<E>>) AND constructible_from<Element, E> AND
                convertible_to<E, Element>) constexpr box(E &&) noexcept {}
        template(typename E)(
            /// \pre
            requires(!same_as<box, ranges_detail::decay_t<E>>) AND constructible_from<Element, E> AND(
                !convertible_to<E, Element>)) constexpr explicit box(E &&) noexcept {}
#endif

        constexpr Element &get() &noexcept { return value; }
        constexpr Element const &get() const &noexcept { return value; }
        constexpr Element &&get() &&noexcept { return ranges_detail::move(value); }
        constexpr Element const &&get() const &&noexcept { return ranges_detail::move(value); }
    };

    template <typename Element, typename Tag> Element box<Element, Tag, ranges_detail::box_compress::coalesce>::value{};

    /// \cond
    namespace _get_ {
        /// \endcond
        // Get by tag type
        template <typename Tag, typename Element, ranges_detail::box_compress BC>
        constexpr Element &get(box<Element, Tag, BC> &b) noexcept {
            return b.get();
        }
        template <typename Tag, typename Element, ranges_detail::box_compress BC>
        constexpr Element const &get(box<Element, Tag, BC> const &b) noexcept {
            return b.get();
        }
        template <typename Tag, typename Element, ranges_detail::box_compress BC>
        constexpr Element &&get(box<Element, Tag, BC> &&b) noexcept {
            return ranges_detail::move(b).get();
        }
        // Get by index
        template <std::size_t I, typename Element, ranges_detail::box_compress BC>
        constexpr Element &get(box<Element, futures::detail::meta::size_t<I>, BC> &b) noexcept {
            return b.get();
        }
        template <std::size_t I, typename Element, ranges_detail::box_compress BC>
        constexpr Element const &get(box<Element, futures::detail::meta::size_t<I>, BC> const &b) noexcept {
            return b.get();
        }
        template <std::size_t I, typename Element, ranges_detail::box_compress BC>
        constexpr Element &&get(box<Element, futures::detail::meta::size_t<I>, BC> &&b) noexcept {
            return ranges_detail::move(b).get();
        }
        /// \cond
    } // namespace _get_
    /// \endcond
    /// @}
} // namespace futures::detail

RANGES_DIAGNOSTIC_POP

// #include <futures/algorithm/detail/traits/range/detail/epilogue.h>
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

#ifndef RANGES_PROLOGUE_INCLUDED
#error "Including epilogue, but prologue not included!"
#endif
#undef RANGES_PROLOGUE_INCLUDED

#undef template
#undef AND

RANGES_DIAGNOSTIC_POP


#endif

// #include <futures/algorithm/detail/traits/range/utility/move.h>

// #include <futures/algorithm/detail/traits/range/utility/semiregular_box.h>
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

#ifndef FUTURES_RANGES_UTILITY_SEMIREGULAR_BOX_HPP
#define FUTURES_RANGES_UTILITY_SEMIREGULAR_BOX_HPP

// #include <type_traits>

// #include <utility>


// #include <futures/algorithm/detail/traits/range/meta/meta.h>


// #include <futures/algorithm/detail/traits/range/concepts/concepts.h>


// #include <futures/algorithm/detail/traits/range/range_fwd.h>


// #include <futures/algorithm/detail/traits/range/functional/concepts.h>

// #include <futures/algorithm/detail/traits/range/functional/invoke.h>

// #include <futures/algorithm/detail/traits/range/functional/reference_wrapper.h>
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
#ifndef FUTURES_RANGES_FUNCTIONAL_REFERENCE_WRAPPER_HPP
#define FUTURES_RANGES_FUNCTIONAL_REFERENCE_WRAPPER_HPP

// #include <type_traits>

// #include <utility>


// #include <futures/algorithm/detail/traits/range/meta/meta.h>


// #include <futures/algorithm/detail/traits/range/concepts/concepts.h>


// #include <futures/algorithm/detail/traits/range/functional/invoke.h>

// #include <futures/algorithm/detail/traits/range/utility/addressof.h>

// #include <futures/algorithm/detail/traits/range/utility/static_const.h>


// #include <futures/algorithm/detail/traits/range/detail/prologue.h>
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

#ifndef FUTURES_RANGES_DETAIL_PROLOGUE_HPP
#define FUTURES_RANGES_DETAIL_PROLOGUE_HPP
// #include <futures/algorithm/detail/traits/range/detail/config.h>

#endif

#ifdef RANGES_PROLOGUE_INCLUDED
#error "Prologue already included!"
#endif
#define RANGES_PROLOGUE_INCLUDED

RANGES_DIAGNOSTIC_PUSH

#ifdef RANGES_FEWER_WARNINGS
RANGES_DIAGNOSTIC_IGNORE_UNDEFINED_INTERNAL
RANGES_DIAGNOSTIC_IGNORE_INDENTATION
RANGES_DIAGNOSTIC_IGNORE_CXX17_COMPAT
#endif

RANGES_DIAGNOSTIC_KEYWORD_MACRO

#define template(...)                                                                                                  \
    CPP_PP_IGNORE_CXX2A_COMPAT_BEGIN                                                                                   \
    template <__VA_ARGS__ CPP_TEMPLATE_AUX_

#define AND CPP_and


namespace futures::detail {
    /// \addtogroup group-functional
    /// @{

    /// \cond
    namespace ranges_detail {
        template <typename T> struct reference_wrapper_ {
            T *t_ = nullptr;
            constexpr reference_wrapper_() = default;
            constexpr reference_wrapper_(T &t) noexcept : t_(ranges_detail::addressof(t)) {}
            constexpr reference_wrapper_(T &&) = delete;
            constexpr T &get() const noexcept { return *t_; }
        };
        template <typename T> struct reference_wrapper_<T &> : reference_wrapper_<T> {
            using reference_wrapper_<T>::reference_wrapper_;
        };
        template <typename T> struct reference_wrapper_<T &&> {
            T *t_ = nullptr;
            constexpr reference_wrapper_() = default;
            constexpr reference_wrapper_(T &&t) noexcept : t_(ranges_detail::addressof(t)) {}
            constexpr T &&get() const noexcept { return static_cast<T &&>(*t_); }
        };
    } // namespace ranges_detail
    /// \endcond

    // Can be used to store rvalue references in addition to lvalue references.
    // Also, see: https://wg21.link/lwg2993
    template <typename T> struct reference_wrapper : private ranges_detail::reference_wrapper_<T> {
      private:
        using base_ = ranges_detail::reference_wrapper_<T>;
        using base_::t_;

      public:
        using type = futures::detail::meta::_t<std::remove_reference<T>>;
        using reference = futures::detail::meta::if_<std::is_reference<T>, T, T &>;

        constexpr reference_wrapper() = default;
        template(typename U)(
            /// \pre
            requires(!same_as<uncvref_t<U>, reference_wrapper>) AND constructible_from<
                base_, U>) constexpr reference_wrapper(U &&u) noexcept(std::is_nothrow_constructible<base_, U>::value)
            : ranges_detail::reference_wrapper_<T>{static_cast<U &&>(u)} {}
        constexpr reference get() const noexcept { return this->base_::get(); }
        constexpr operator reference() const noexcept { return get(); }
        template(typename...)(
            /// \pre
            requires(!std::is_rvalue_reference<T>::value)) //
        operator std::reference_wrapper<type>() const noexcept {
            return {get()};
        }
        // clang-format off
        template<typename ...Args>
        constexpr auto CPP_auto_fun(operator())(Args &&...args) (const)
        (
            return invoke(static_cast<reference>(*t_), static_cast<Args &&>(args)...)
        )
        // clang-format on
    };

    struct ref_fn {
        template(typename T)(
            /// \pre
            requires(!is_reference_wrapper_v<T>)) //
            reference_wrapper<T>
            operator()(T &t) const {
            return {t};
        }
        /// \overload
        template <typename T> reference_wrapper<T> operator()(reference_wrapper<T> t) const { return t; }
        /// \overload
        template <typename T> reference_wrapper<T> operator()(std::reference_wrapper<T> t) const { return {t.get()}; }
    };

    /// \ingroup group-functional
    /// \sa `ref_fn`
    RANGES_INLINE_VARIABLE(ref_fn, ref)

    template <typename T> using ref_t = decltype(ref(std::declval<T>()));

    struct unwrap_reference_fn {
        template <typename T> T &&operator()(T &&t) const noexcept { return static_cast<T &&>(t); }
        /// \overload
        template <typename T>
        typename reference_wrapper<T>::reference operator()(reference_wrapper<T> t) const noexcept {
            return t.get();
        }
        /// \overload
        template <typename T> T &operator()(std::reference_wrapper<T> t) const noexcept { return t.get(); }
        /// \overload
        template <typename T> T &operator()(ref_view<T> t) const noexcept { return t.base(); }
    };

    /// \ingroup group-functional
    /// \sa `unwrap_reference_fn`
    RANGES_INLINE_VARIABLE(unwrap_reference_fn, unwrap_reference)

    template <typename T> using unwrap_reference_t = decltype(unwrap_reference(std::declval<T>()));
    /// @}
} // namespace futures::detail

// #include <futures/algorithm/detail/traits/range/detail/epilogue.h>
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

#ifndef RANGES_PROLOGUE_INCLUDED
#error "Including epilogue, but prologue not included!"
#endif
#undef RANGES_PROLOGUE_INCLUDED

#undef template
#undef AND

RANGES_DIAGNOSTIC_POP


#endif

// #include <futures/algorithm/detail/traits/range/utility/get.h>

// #include <futures/algorithm/detail/traits/range/utility/in_place.h>
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

#ifndef FUTURES_RANGES_UTILITY_IN_PLACE_HPP
#define FUTURES_RANGES_UTILITY_IN_PLACE_HPP

// #include <futures/algorithm/detail/traits/range/range_fwd.h>


// #include <futures/algorithm/detail/traits/range/utility/static_const.h>


// #include <futures/algorithm/detail/traits/range/detail/prologue.h>
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

#ifndef FUTURES_RANGES_DETAIL_PROLOGUE_HPP
#define FUTURES_RANGES_DETAIL_PROLOGUE_HPP
// #include <futures/algorithm/detail/traits/range/detail/config.h>

#endif

#ifdef RANGES_PROLOGUE_INCLUDED
#error "Prologue already included!"
#endif
#define RANGES_PROLOGUE_INCLUDED

RANGES_DIAGNOSTIC_PUSH

#ifdef RANGES_FEWER_WARNINGS
RANGES_DIAGNOSTIC_IGNORE_UNDEFINED_INTERNAL
RANGES_DIAGNOSTIC_IGNORE_INDENTATION
RANGES_DIAGNOSTIC_IGNORE_CXX17_COMPAT
#endif

RANGES_DIAGNOSTIC_KEYWORD_MACRO

#define template(...)                                                                                                  \
    CPP_PP_IGNORE_CXX2A_COMPAT_BEGIN                                                                                   \
    template <__VA_ARGS__ CPP_TEMPLATE_AUX_

#define AND CPP_and


namespace futures::detail {
    /// \ingroup group-utility
    struct in_place_t {};
    RANGES_INLINE_VARIABLE(in_place_t, in_place)
} // namespace futures::detail

// #include <futures/algorithm/detail/traits/range/detail/epilogue.h>
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

#ifndef RANGES_PROLOGUE_INCLUDED
#error "Including epilogue, but prologue not included!"
#endif
#undef RANGES_PROLOGUE_INCLUDED

#undef template
#undef AND

RANGES_DIAGNOSTIC_POP


#endif


// #include <futures/algorithm/detail/traits/range/detail/prologue.h>
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

#ifndef FUTURES_RANGES_DETAIL_PROLOGUE_HPP
#define FUTURES_RANGES_DETAIL_PROLOGUE_HPP
// #include <futures/algorithm/detail/traits/range/detail/config.h>

#endif

#ifdef RANGES_PROLOGUE_INCLUDED
#error "Prologue already included!"
#endif
#define RANGES_PROLOGUE_INCLUDED

RANGES_DIAGNOSTIC_PUSH

#ifdef RANGES_FEWER_WARNINGS
RANGES_DIAGNOSTIC_IGNORE_UNDEFINED_INTERNAL
RANGES_DIAGNOSTIC_IGNORE_INDENTATION
RANGES_DIAGNOSTIC_IGNORE_CXX17_COMPAT
#endif

RANGES_DIAGNOSTIC_KEYWORD_MACRO

#define template(...)                                                                                                  \
    CPP_PP_IGNORE_CXX2A_COMPAT_BEGIN                                                                                   \
    template <__VA_ARGS__ CPP_TEMPLATE_AUX_

#define AND CPP_and


namespace futures::detail {
    /// \cond
    template <typename T> struct semiregular_box;

    namespace ranges_detail {
        struct semiregular_get {
            // clang-format off
            template<typename T>
            friend auto CPP_auto_fun(get)(futures::detail::meta::id_t<semiregular_box<T>> &t)
            (
                return t.get()
            )
            template<typename T>
            friend auto CPP_auto_fun(get)(futures::detail::meta::id_t<semiregular_box<T>> const &t)
            (
                return t.get()
            )
            template<typename T>
            friend auto CPP_auto_fun(get)(futures::detail::meta::id_t<semiregular_box<T>> &&t)
            (
                return ranges_detail::move(t).get()
            )
            // clang-format on
        };
    } // namespace ranges_detail
    /// \endcond

    /// \addtogroup group-utility
    /// @{
    template <typename T> struct semiregular_box : private ranges_detail::semiregular_get {
      private:
        struct tag {};
        template <typename... Args> void construct_from(Args &&...args) {
            new ((void *)std::addressof(data_)) T(static_cast<Args &&>(args)...);
            engaged_ = true;
        }
        void move_assign(T &&t, std::true_type) { data_ = ranges_detail::move(t); }
        void move_assign(T &&t, std::false_type) {
            reset();
            construct_from(ranges_detail::move(t));
        }
        void copy_assign(T const &t, std::true_type) { data_ = t; }
        void copy_assign(T &&t, std::false_type) {
            reset();
            construct_from(t);
        }
        constexpr semiregular_box(tag, std::false_type) noexcept {}
        constexpr semiregular_box(tag, std::true_type) noexcept(std::is_nothrow_default_constructible<T>::value)
            : data_{}, engaged_(true) {}
        void reset() {
            if (engaged_) {
                data_.~T();
                engaged_ = false;
            }
        }
        union {
            char ch_{};
            T data_;
        };
        bool engaged_{false};

      public:
        constexpr semiregular_box() noexcept(std::is_nothrow_default_constructible<T>::value ||
                                             !std::is_default_constructible<T>::value)
            : semiregular_box(tag{}, std::is_default_constructible<T>{}) {}
        semiregular_box(semiregular_box &&that) noexcept(std::is_nothrow_move_constructible<T>::value) {
            if (that.engaged_)
                this->construct_from(ranges_detail::move(that.data_));
        }
        semiregular_box(semiregular_box const &that) noexcept(std::is_nothrow_copy_constructible<T>::value) {
            if (that.engaged_)
                this->construct_from(that.data_);
        }
#if defined(__cpp_conditional_explicit) && 0 < __cpp_conditional_explicit
        template(typename U)(
            /// \pre
            requires(!same_as<uncvref_t<U>, semiregular_box>) AND constructible_from<
                T,
                U>) explicit(!convertible_to<U,
                                             T>) constexpr semiregular_box(U &&u) noexcept(std::
                                                                                               is_nothrow_constructible<
                                                                                                   T, U>::value)
            : semiregular_box(in_place, static_cast<U &&>(u)) {}
#else
        template(typename U)(
            /// \pre
            requires(!same_as<uncvref_t<U>, semiregular_box>) AND constructible_from<T, U> AND(
                !convertible_to<U, T>)) //
            constexpr explicit semiregular_box(U &&u) noexcept(std::is_nothrow_constructible<T, U>::value)
            : semiregular_box(in_place, static_cast<U &&>(u)) {}
        template(typename U)(
            /// \pre
            requires(!same_as<uncvref_t<U>, semiregular_box>) AND constructible_from<T, U> AND convertible_to<
                U, T>) constexpr semiregular_box(U &&u) noexcept(std::is_nothrow_constructible<T, U>::value)
            : semiregular_box(in_place, static_cast<U &&>(u)) {}
#endif
        template(typename... Args)(
            /// \pre
            requires constructible_from<T, Args...>) constexpr semiregular_box(in_place_t, Args &&...args) //
            noexcept(std::is_nothrow_constructible<T, Args...>::value)
            : data_(static_cast<Args &&>(args)...), engaged_(true) {}
        ~semiregular_box() { reset(); }
        semiregular_box &operator=(semiregular_box &&that) noexcept(std::is_nothrow_move_constructible<T>::value &&
                                                                    (!std::is_move_assignable<T>::value ||
                                                                     std::is_nothrow_move_assignable<T>::value)) {
            if (engaged_ && that.engaged_)
                this->move_assign(ranges_detail::move(that.data_), std::is_move_assignable<T>());
            else if (that.engaged_)
                this->construct_from(ranges_detail::move(that.data_));
            else if (engaged_)
                this->reset();
            return *this;
        }
        semiregular_box &operator=(semiregular_box const &that) noexcept(std::is_nothrow_copy_constructible<T>::value &&
                                                                         (!std::is_copy_assignable<T>::value ||
                                                                          std::is_nothrow_copy_assignable<T>::value)) {
            if (engaged_ && that.engaged_)
                this->copy_assign(that.data_, std::is_copy_assignable<T>());
            else if (that.engaged_)
                this->construct_from(that.data_);
            else if (engaged_)
                this->reset();
            return *this;
        }
        constexpr T &get() &noexcept { return RANGES_ENSURE(engaged_), data_; }
        constexpr T const &get() const &noexcept { return RANGES_ENSURE(engaged_), data_; }
        constexpr T &&get() &&noexcept { return RANGES_ENSURE(engaged_), ranges_detail::move(data_); }
        T const &&get() const && = delete;
        constexpr operator T &() &noexcept { return get(); }
        constexpr operator T const &() const &noexcept { return get(); }
        constexpr operator T &&() &&noexcept { return ranges_detail::move(get()); }
        operator T const &&() const && = delete;
        // clang-format off
        template(typename... Args)(
            /// \pre
            requires invocable<T &, Args...>)
        constexpr decltype(auto) operator()(Args &&... args) &
            noexcept(is_nothrow_invocable_v<T &, Args...>)
        {
            return invoke(data_, static_cast<Args &&>(args)...);
        }
        template(typename... Args)(
            /// \pre
            requires invocable<T const &, Args...>)
        constexpr decltype(auto) operator()(Args &&... args) const &
            noexcept(is_nothrow_invocable_v<T const &, Args...>)
        {
            return invoke(data_, static_cast<Args &&>(args)...);
        }
        template(typename... Args)(
            /// \pre
            requires invocable<T, Args...>)
        constexpr decltype(auto) operator()(Args &&... args) &&
            noexcept(is_nothrow_invocable_v<T, Args...>)
        {
            return invoke(static_cast<T &&>(data_), static_cast<Args &&>(args)...);
        }
        template<typename... Args>
        void operator()(Args &&...) const && = delete;
        // clang-format on
    };

    template <typename T>
    struct semiregular_box<T &> : private futures::detail::reference_wrapper<T &>, private ranges_detail::semiregular_get {
        semiregular_box() = default;
        template(typename Arg)(
            /// \pre
            requires constructible_from<futures::detail::reference_wrapper<T &>, Arg &>)
            semiregular_box(in_place_t, Arg &arg) noexcept //
            : futures::detail::reference_wrapper<T &>(arg) {}
        using futures::detail::reference_wrapper<T &>::get;
        using futures::detail::reference_wrapper<T &>::operator T &;
        using futures::detail::reference_wrapper<T &>::operator();

#if defined(_MSC_VER)
        template(typename U)(
            /// \pre
            requires(!same_as<uncvref_t<U>, semiregular_box>) AND constructible_from<
                futures::detail::reference_wrapper<T &>,
                U>) constexpr semiregular_box(U &&u) noexcept(std::
                                                                  is_nothrow_constructible<
                                                                      futures::detail::reference_wrapper<T &>, U>::value)
            : futures::detail::reference_wrapper<T &>{static_cast<U &&>(u)} {}
#else
        using futures::detail::reference_wrapper<T &>::reference_wrapper;
#endif
    };

    template <typename T>
    struct semiregular_box<T &&> : private futures::detail::reference_wrapper<T &&>, private ranges_detail::semiregular_get {
        semiregular_box() = default;
        template(typename Arg)(
            /// \pre
            requires constructible_from<futures::detail::reference_wrapper<T &&>, Arg>)
            semiregular_box(in_place_t, Arg &&arg) noexcept //
            : futures::detail::reference_wrapper<T &&>(static_cast<Arg &&>(arg)) {}
        using futures::detail::reference_wrapper<T &&>::get;
        using futures::detail::reference_wrapper<T &&>::operator T &&;
        using futures::detail::reference_wrapper<T &&>::operator();

#if defined(_MSC_VER)
        template(typename U)(
            /// \pre
            requires(!same_as<uncvref_t<U>, semiregular_box>) AND constructible_from<
                futures::detail::reference_wrapper<T &&>,
                U>) constexpr semiregular_box(U &&u) noexcept(std::
                                                                  is_nothrow_constructible<
                                                                      futures::detail::reference_wrapper<T &&>, U>::value)
            : futures::detail::reference_wrapper<T &&>{static_cast<U &&>(u)} {}
#else
        using futures::detail::reference_wrapper<T &&>::reference_wrapper;
#endif
    };

    template <typename T> using semiregular_box_t = futures::detail::meta::if_c<(bool)semiregular<T>, T, semiregular_box<T>>;

    template <typename T, bool IsConst = false>
    using semiregular_box_ref_or_val_t =
        futures::detail::meta::if_c<(bool)semiregular<T>, futures::detail::meta::if_c<IsConst || std::is_empty<T>::value, T, reference_wrapper<T>>,
                   reference_wrapper<futures::detail::meta::if_c<IsConst, semiregular_box<T> const, semiregular_box<T>>>>;
    /// @}

    /// \cond
    template <typename T>
    using semiregular_t RANGES_DEPRECATED("Please use semiregular_box_t instead.") = semiregular_box_t<T>;

    template <typename T, bool IsConst = false>
    using semiregular_ref_or_val_t RANGES_DEPRECATED("Please use semiregular_box_ref_or_val_t instead.") =
        semiregular_box_ref_or_val_t<T, IsConst>;
    /// \endcond

} // namespace futures::detail

// #include <futures/algorithm/detail/traits/range/detail/epilogue.h>
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

#ifndef RANGES_PROLOGUE_INCLUDED
#error "Including epilogue, but prologue not included!"
#endif
#undef RANGES_PROLOGUE_INCLUDED

#undef template
#undef AND

RANGES_DIAGNOSTIC_POP


#endif

// #include <futures/algorithm/detail/traits/range/utility/static_const.h>


// #include <futures/algorithm/detail/traits/range/detail/prologue.h>
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

#ifndef FUTURES_RANGES_DETAIL_PROLOGUE_HPP
#define FUTURES_RANGES_DETAIL_PROLOGUE_HPP
// #include <futures/algorithm/detail/traits/range/detail/config.h>

#endif

#ifdef RANGES_PROLOGUE_INCLUDED
#error "Prologue already included!"
#endif
#define RANGES_PROLOGUE_INCLUDED

RANGES_DIAGNOSTIC_PUSH

#ifdef RANGES_FEWER_WARNINGS
RANGES_DIAGNOSTIC_IGNORE_UNDEFINED_INTERNAL
RANGES_DIAGNOSTIC_IGNORE_INDENTATION
RANGES_DIAGNOSTIC_IGNORE_CXX17_COMPAT
#endif

RANGES_DIAGNOSTIC_KEYWORD_MACRO

#define template(...)                                                                                                  \
    CPP_PP_IGNORE_CXX2A_COMPAT_BEGIN                                                                                   \
    template <__VA_ARGS__ CPP_TEMPLATE_AUX_

#define AND CPP_and


RANGES_DIAGNOSTIC_PUSH
RANGES_DIAGNOSTIC_IGNORE_MULTIPLE_ASSIGNMENT_OPERATORS

namespace futures::detail {
    /// \addtogroup group-iterator Iterator
    /// @{
    ///
    template <typename T> struct basic_mixin : private box<T> {
        CPP_member constexpr CPP_ctor(basic_mixin)()(                 //
            noexcept(std::is_nothrow_default_constructible<T>::value) //
            requires default_constructible<T>)
            : box<T>{} {}
        CPP_member constexpr explicit CPP_ctor(basic_mixin)(T &&t)( //
            noexcept(std::is_nothrow_move_constructible<T>::value)  //
            requires move_constructible<T>)
            : box<T>(ranges_detail::move(t)) {}
        CPP_member constexpr explicit CPP_ctor(basic_mixin)(T const &t)( //
            noexcept(std::is_nothrow_copy_constructible<T>::value)       //
            requires copy_constructible<T>)
            : box<T>(t) {}

      protected:
        using box<T>::get;
    };

    /// \cond
    namespace ranges_detail {
        template <typename Cur> using cursor_reference_t = decltype(range_access::read(std::declval<Cur const &>()));

        // Compute the rvalue reference type of a cursor
        template <typename Cur> auto cursor_move(Cur const &cur, int) -> decltype(range_access::move(cur));
        template <typename Cur> auto cursor_move(Cur const &cur, long) -> aux::move_t<cursor_reference_t<Cur>>;

        template <typename Cur>
        using cursor_rvalue_reference_t = decltype(ranges_detail::cursor_move(std::declval<Cur const &>(), 42));

        // Define conversion operators from the proxy reference type
        // to the common reference types, so that basic_iterator can model readable
        // even with getters/setters.
        template <typename Derived, typename Head> struct proxy_reference_conversion {
            operator Head() const noexcept(noexcept(Head(Head(std::declval<Derived const &>().read_())))) {
                return Head(static_cast<Derived const *>(this)->read_());
            }
        };

        // Collect the reference types associated with cursors
        template <typename Cur, bool IsReadable> struct cursor_traits_ {
          private:
            struct private_ {};

          public:
            using value_t_ = private_;
            using reference_t_ = private_;
            using rvalue_reference_t_ = private_;
            using common_refs = futures::detail::meta::list<>;
        };

        template <typename Cur> struct cursor_traits_<Cur, true> {
            using value_t_ = range_access::cursor_value_t<Cur>;
            using reference_t_ = cursor_reference_t<Cur>;
            using rvalue_reference_t_ = cursor_rvalue_reference_t<Cur>;

          private:
            using R1 = reference_t_;
            using R2 = common_reference_t<reference_t_, value_t_ &>;
            using R3 = common_reference_t<reference_t_, rvalue_reference_t_>;
            using tmp1 = futures::detail::meta::list<value_t_, R1>;
            using tmp2 = futures::detail::meta::if_<futures::detail::meta::in<tmp1, uncvref_t<R2>>, tmp1, futures::detail::meta::push_back<tmp1, R2>>;
            using tmp3 = futures::detail::meta::if_<futures::detail::meta::in<tmp2, uncvref_t<R3>>, tmp2, futures::detail::meta::push_back<tmp2, R3>>;

          public:
            using common_refs = futures::detail::meta::unique<futures::detail::meta::pop_front<tmp3>>;
        };

        template <typename Cur> using cursor_traits = cursor_traits_<Cur, (bool)readable_cursor<Cur>>;

        template <typename Cur> using cursor_value_t = typename cursor_traits<Cur>::value_t_;

        template <typename Cur, bool IsReadable> struct basic_proxy_reference_;
        template <typename Cur> using basic_proxy_reference = basic_proxy_reference_<Cur, (bool)readable_cursor<Cur>>;

        // The One Proxy Reference type to rule them all. basic_iterator uses this
        // as the return type of operator* when the cursor type has a set() member
        // function of the correct signature (i.e., if it can accept a value_type &&).
        template <typename Cur, bool IsReadable /*= (bool) readable_cursor<Cur>*/>
        struct RANGES_EMPTY_BASES basic_proxy_reference_
            : cursor_traits<Cur>
            // The following adds conversion operators to the common reference
            // types, so that basic_proxy_reference can model readable
            ,
              futures::detail::meta::inherit<futures::detail::meta::transform<
                  typename cursor_traits<Cur>::common_refs,
                  futures::detail::meta::bind_front<futures::detail::meta::quote<proxy_reference_conversion>, basic_proxy_reference_<Cur, IsReadable>>>> {
          private:
            Cur *cur_;
            template <typename, bool> friend struct basic_proxy_reference_;
            template <typename, typename> friend struct proxy_reference_conversion;
            using typename cursor_traits<Cur>::value_t_;
            using typename cursor_traits<Cur>::reference_t_;
            using typename cursor_traits<Cur>::rvalue_reference_t_;
            static_assert((bool)common_reference_with<value_t_ &, reference_t_>,
                          "Your readable and writable cursor must have a value type and "
                          "a reference type that share a common reference type. See the "
                          "futures::detail::common_reference type trait.");
            // BUGBUG make these private:
          public:
            constexpr reference_t_ read_() const
                noexcept(noexcept(reference_t_(range_access::read(std::declval<Cur const &>())))) {
                return range_access::read(*cur_);
            }
            template <typename T> constexpr void write_(T &&t) const { range_access::write(*cur_, (T &&) t); }
            // public:
            basic_proxy_reference_() = default;
            basic_proxy_reference_(basic_proxy_reference_ const &) = default;
            template(typename OtherCur)(
                /// \pre
                requires convertible_to<OtherCur *,
                                        Cur *>) constexpr basic_proxy_reference_(basic_proxy_reference<OtherCur> const
                                                                                     &that) noexcept
                : cur_(that.cur_) {}
            constexpr explicit basic_proxy_reference_(Cur &cur) noexcept : cur_(&cur) {}
            CPP_member constexpr auto operator=(basic_proxy_reference_ &&that) -> CPP_ret(basic_proxy_reference_ &)(
                /// \pre
                requires readable_cursor<Cur>) {
                return *this = that;
            }
            CPP_member constexpr auto operator=(basic_proxy_reference_ const &that)
                -> CPP_ret(basic_proxy_reference_ &)(
                    /// \pre
                    requires readable_cursor<Cur>) {
                this->write_(that.read_());
                return *this;
            }
            CPP_member constexpr auto operator=(basic_proxy_reference_ &&that) const
                -> CPP_ret(basic_proxy_reference_ const &)(
                    /// \pre
                    requires readable_cursor<Cur>) {
                return *this = that;
            }
            CPP_member constexpr auto operator=(basic_proxy_reference_ const &that) const
                -> CPP_ret(basic_proxy_reference_ const &)(
                    /// \pre
                    requires readable_cursor<Cur>) {
                this->write_(that.read_());
                return *this;
            }
            template(typename OtherCur)(
                /// \pre
                requires readable_cursor<OtherCur> AND
                    writable_cursor<Cur, cursor_reference_t<OtherCur>>) constexpr basic_proxy_reference_ & //
            operator=(basic_proxy_reference<OtherCur> &&that) {
                return *this = that;
            }
            template(typename OtherCur)(
                /// \pre
                requires readable_cursor<OtherCur> AND
                    writable_cursor<Cur, cursor_reference_t<OtherCur>>) constexpr basic_proxy_reference_ & //
            operator=(basic_proxy_reference<OtherCur> const &that) {
                this->write_(that.read_());
                return *this;
            }
            template(typename OtherCur)(
                /// \pre
                requires readable_cursor<OtherCur> AND
                    writable_cursor<Cur, cursor_reference_t<OtherCur>>) constexpr basic_proxy_reference_ const & //
            operator=(basic_proxy_reference<OtherCur> &&that) const {
                return *this = that;
            }
            template(typename OtherCur)(
                /// \pre
                requires readable_cursor<OtherCur> AND
                    writable_cursor<Cur, cursor_reference_t<OtherCur>>) constexpr basic_proxy_reference_ const & //
            operator=(basic_proxy_reference<OtherCur> const &that) const {
                this->write_(that.read_());
                return *this;
            }
            template(typename T)(
                /// \pre
                requires writable_cursor<Cur, T>) constexpr basic_proxy_reference_ &
            operator=(T &&t) //
            {
                this->write_((T &&) t);
                return *this;
            }
            template(typename T)(
                /// \pre
                requires writable_cursor<Cur, T>) constexpr basic_proxy_reference_ const &
            operator=(T &&t) const {
                this->write_((T &&) t);
                return *this;
            }
        };

        template(typename Cur, bool IsReadable)(
            /// \pre
            requires readable_cursor<Cur> AND equality_comparable<cursor_value_t<Cur>>) constexpr bool
        operator==(basic_proxy_reference_<Cur, IsReadable> const &x, cursor_value_t<Cur> const &y) {
            return x.read_() == y;
        }
        template(typename Cur, bool IsReadable)(
            /// \pre
            requires readable_cursor<Cur> AND equality_comparable<cursor_value_t<Cur>>) constexpr bool
        operator!=(basic_proxy_reference_<Cur, IsReadable> const &x, cursor_value_t<Cur> const &y) {
            return !(x == y);
        }
        template(typename Cur, bool IsReadable)(
            /// \pre
            requires readable_cursor<Cur> AND equality_comparable<cursor_value_t<Cur>>) constexpr bool
        operator==(cursor_value_t<Cur> const &x, basic_proxy_reference_<Cur, IsReadable> const &y) {
            return x == y.read_();
        }
        template(typename Cur, bool IsReadable)(
            /// \pre
            requires readable_cursor<Cur> AND equality_comparable<cursor_value_t<Cur>>) constexpr bool
        operator!=(cursor_value_t<Cur> const &x, basic_proxy_reference_<Cur, IsReadable> const &y) {
            return !(x == y);
        }
        template(typename Cur, bool IsReadable)(
            /// \pre
            requires readable_cursor<Cur> AND equality_comparable<cursor_value_t<Cur>>) constexpr bool
        operator==(basic_proxy_reference_<Cur, IsReadable> const &x, basic_proxy_reference_<Cur, IsReadable> const &y) {
            return x.read_() == y.read_();
        }
        template(typename Cur, bool IsReadable)(
            /// \pre
            requires readable_cursor<Cur> AND equality_comparable<cursor_value_t<Cur>>) constexpr bool
        operator!=(basic_proxy_reference_<Cur, IsReadable> const &x, basic_proxy_reference_<Cur, IsReadable> const &y) {
            return !(x == y);
        }

        template <typename Cur>
        using cpp20_iter_cat_of_t =                          //
            std::enable_if_t<                                //
                input_cursor<Cur>,                           //
                futures::detail::meta::conditional_t<                         //
                    contiguous_cursor<Cur>,                  //
                    futures::detail::contiguous_iterator_tag,         //
                    futures::detail::meta::conditional_t<                     //
                        random_access_cursor<Cur>,           //
                        std::random_access_iterator_tag,     //
                        futures::detail::meta::conditional_t<                 //
                            bidirectional_cursor<Cur>,       //
                            std::bidirectional_iterator_tag, //
                            futures::detail::meta::conditional_t<             //
                                forward_cursor<Cur>,         //
                                std::forward_iterator_tag,   //
                                std::input_iterator_tag>>>>>;

        // clang-format off
        template(typename C)(
        concept (cpp17_input_cursor_)(C),
            // Either it is not single-pass, or else we can create a
            // proxy for postfix increment.
            !range_access::single_pass_t<uncvref_t<C>>::value ||
            (move_constructible<range_access::cursor_value_t<C>> &&
             constructible_from<range_access::cursor_value_t<C>, cursor_reference_t<C>>)
        );

        template<typename C>
        CPP_concept cpp17_input_cursor =
            input_cursor<C> &&
            sentinel_for_cursor<C, C> &&
            CPP_concept_ref(cpp17_input_cursor_, C);

        template(typename C)(
        concept (cpp17_forward_cursor_)(C),
            std::is_reference<cursor_reference_t<C>>::value
        );

        template<typename C>
        CPP_concept cpp17_forward_cursor =
            forward_cursor<C> &&
            CPP_concept_ref(cpp17_forward_cursor_, C);
        // clang-format on

        template <typename Category, typename Base = void> struct with_iterator_category : Base {
            using iterator_category = Category;
        };

        template <typename Category> struct with_iterator_category<Category> { using iterator_category = Category; };

        template <typename Cur>
        using cpp17_iter_cat_of_t =                      //
            std::enable_if_t<                            //
                cpp17_input_cursor<Cur>,                 //
                futures::detail::meta::conditional_t<                     //
                    random_access_cursor<Cur>,           //
                    std::random_access_iterator_tag,     //
                    futures::detail::meta::conditional_t<                 //
                        bidirectional_cursor<Cur>,       //
                        std::bidirectional_iterator_tag, //
                        futures::detail::meta::conditional_t<             //
                            cpp17_forward_cursor<Cur>,   //
                            std::forward_iterator_tag,   //
                            std::input_iterator_tag>>>>;

        template <typename Cur, typename = void>
        struct readable_iterator_associated_types_base : range_access::mixin_base_t<Cur> {
            readable_iterator_associated_types_base() = default;
            using range_access::mixin_base_t<Cur>::mixin_base_t;
            readable_iterator_associated_types_base(Cur &&cur)
                : range_access::mixin_base_t<Cur>(static_cast<Cur &&>(cur)) {}
            readable_iterator_associated_types_base(Cur const &cur) : range_access::mixin_base_t<Cur>(cur) {}
        };

        template <typename Cur>
        struct readable_iterator_associated_types_base<Cur, always_<void, cpp17_iter_cat_of_t<Cur>>>
            : range_access::mixin_base_t<Cur> {
            using iterator_category = cpp17_iter_cat_of_t<Cur>;
            readable_iterator_associated_types_base() = default;
            using range_access::mixin_base_t<Cur>::mixin_base_t;
            readable_iterator_associated_types_base(Cur &&cur)
                : range_access::mixin_base_t<Cur>(static_cast<Cur &&>(cur)) {}
            readable_iterator_associated_types_base(Cur const &cur) : range_access::mixin_base_t<Cur>(cur) {}
        };

        template <typename Cur, bool IsReadable /*= (bool) readable_cursor<Cur>*/>
        struct iterator_associated_types_base_ : range_access::mixin_base_t<Cur> {
            // BUGBUG
            // protected:
            using iter_reference_t = basic_proxy_reference<Cur>;
            using const_reference_t = basic_proxy_reference<Cur const>;

          public:
            using reference = void;
            using difference_type = range_access::cursor_difference_t<Cur>;

            iterator_associated_types_base_() = default;
            using range_access::mixin_base_t<Cur>::mixin_base_t;
            iterator_associated_types_base_(Cur &&cur) : range_access::mixin_base_t<Cur>(static_cast<Cur &&>(cur)) {}
            iterator_associated_types_base_(Cur const &cur) : range_access::mixin_base_t<Cur>(cur) {}
        };

        template <typename Cur> using cursor_arrow_t = decltype(range_access::arrow(std::declval<Cur const &>()));

        template <typename Cur>
        struct iterator_associated_types_base_<Cur, true> : readable_iterator_associated_types_base<Cur> {
            // BUGBUG
            // protected:
            using iter_reference_t = futures::detail::meta::conditional_t<
                is_writable_cursor_v<Cur const>, basic_proxy_reference<Cur const>,
                futures::detail::meta::conditional_t<is_writable_cursor_v<Cur>, basic_proxy_reference<Cur>, cursor_reference_t<Cur>>>;
            using const_reference_t = futures::detail::meta::conditional_t<is_writable_cursor_v<Cur const>,
                                                          basic_proxy_reference<Cur const>, cursor_reference_t<Cur>>;

          public:
            using difference_type = range_access::cursor_difference_t<Cur>;
            using value_type = range_access::cursor_value_t<Cur>;
            using reference = iter_reference_t;
            using iterator_concept = cpp20_iter_cat_of_t<Cur>;
            using pointer = futures::detail::meta::_t<futures::detail::meta::conditional_t<(bool)has_cursor_arrow<Cur>, futures::detail::meta::defer<cursor_arrow_t, Cur>,
                                                         std::add_pointer<reference>>>;
            using common_reference = common_reference_t<reference, value_type &>;

            iterator_associated_types_base_() = default;
            using readable_iterator_associated_types_base<Cur>::readable_iterator_associated_types_base;
            iterator_associated_types_base_(Cur &&cur)
                : readable_iterator_associated_types_base<Cur>(static_cast<Cur &&>(cur)) {}
            iterator_associated_types_base_(Cur const &cur) : readable_iterator_associated_types_base<Cur>(cur) {}
        };

        template <typename Cur>
        using iterator_associated_types_base = iterator_associated_types_base_<Cur, (bool)readable_cursor<Cur>>;

        template <typename Value> struct postfix_increment_proxy {
          private:
            Value cache_;

          public:
            template <typename T> constexpr postfix_increment_proxy(T &&t) : cache_(static_cast<T &&>(t)) {}
            constexpr Value const &operator*() const noexcept { return cache_; }
        };
    } // namespace ranges_detail
    /// \endcond

#if RANGES_BROKEN_CPO_LOOKUP
    namespace _basic_iterator_ {
        template <typename> struct adl_hook {};
    } // namespace _basic_iterator_
#endif

    template <typename Cur>
    struct RANGES_EMPTY_BASES basic_iterator : ranges_detail::iterator_associated_types_base<Cur>
#if RANGES_BROKEN_CPO_LOOKUP
        ,
                                               private _basic_iterator_::adl_hook<basic_iterator<Cur>>
#endif
    {
      private:
        template <typename> friend struct basic_iterator;
        friend range_access;
        using base_t = ranges_detail::iterator_associated_types_base<Cur>;
        using mixin_t = range_access::mixin_base_t<Cur>;
        static_assert((bool)ranges_detail::cursor<Cur>, "");
        using assoc_types_ = ranges_detail::iterator_associated_types_base<Cur>;
        using typename assoc_types_::const_reference_t;
        using typename assoc_types_::iter_reference_t;
        constexpr Cur &pos() noexcept { return this->mixin_t::basic_mixin::get(); }
        constexpr Cur const &pos() const noexcept { return this->mixin_t::basic_mixin::get(); }

      public:
        using typename assoc_types_::difference_type;
        constexpr basic_iterator() = default;
        template(typename OtherCur)(
            /// \pre
            requires(!same_as<OtherCur, Cur>) AND convertible_to<OtherCur, Cur> AND
                constructible_from<mixin_t, OtherCur>) constexpr basic_iterator(basic_iterator<OtherCur> that)
            : base_t{std::move(that.pos())} {}
        // Mix in any additional constructors provided by the mixin
        using base_t::base_t;

        explicit basic_iterator(Cur &&cur) : base_t(static_cast<Cur &&>(cur)) {}

        explicit basic_iterator(Cur const &cur) : base_t(cur) {}

        template(typename OtherCur)(
            /// \pre
            requires(!same_as<OtherCur, Cur>) AND convertible_to<OtherCur, Cur>) constexpr basic_iterator &
        operator=(basic_iterator<OtherCur> that) {
            pos() = std::move(that.pos());
            return *this;
        }

        CPP_member constexpr auto operator*() const noexcept(noexcept(range_access::read(std::declval<Cur const &>())))
            -> CPP_ret(const_reference_t)(
                /// \pre
                requires ranges_detail::readable_cursor<Cur> && (!ranges_detail::is_writable_cursor_v<Cur>)) {
            return range_access::read(pos());
        }
        CPP_member constexpr auto operator*()                           //
            noexcept(noexcept(iter_reference_t{std::declval<Cur &>()})) //
            -> CPP_ret(iter_reference_t)(
                /// \pre
                requires ranges_detail::has_cursor_next<Cur> &&ranges_detail::is_writable_cursor_v<Cur>) {
            return iter_reference_t{pos()};
        }
        CPP_member constexpr auto operator*() const noexcept(noexcept(const_reference_t{std::declval<Cur const &>()}))
            -> CPP_ret(const_reference_t)(
                /// \pre
                requires ranges_detail::has_cursor_next<Cur> &&ranges_detail::is_writable_cursor_v<Cur const>) {
            return const_reference_t{pos()};
        }
        CPP_member constexpr auto operator*() noexcept //
            -> CPP_ret(basic_iterator &)(
                /// \pre
                requires(!ranges_detail::has_cursor_next<Cur>)) {
            return *this;
        }

        // Use cursor's arrow() member, if any.
        template(typename C = Cur)(
            /// \pre
            requires ranges_detail::has_cursor_arrow<C>) constexpr ranges_detail::cursor_arrow_t<C>
        operator->() const noexcept(noexcept(range_access::arrow(std::declval<C const &>()))) {
            return range_access::arrow(pos());
        }
        // Otherwise, if iter_reference_t is an lvalue reference to cv-qualified
        // iter_value_t, return the address of **this.
        template(typename C = Cur)(
            /// \pre
            requires(!ranges_detail::has_cursor_arrow<C>) AND ranges_detail::readable_cursor<C> AND
                std::is_lvalue_reference<const_reference_t>::value AND
                    same_as<typename ranges_detail::iterator_associated_types_base<C>::value_type,
                            uncvref_t<const_reference_t>>) constexpr std::add_pointer_t<const_reference_t>
        operator->() const noexcept(noexcept(*std::declval<basic_iterator const &>())) {
            return ranges_detail::addressof(**this);
        }

        CPP_member constexpr auto operator++() //
            -> CPP_ret(basic_iterator &)(
                /// \pre
                requires ranges_detail::has_cursor_next<Cur>) {
            range_access::next(pos());
            return *this;
        }
        CPP_member constexpr auto operator++() noexcept //
            -> CPP_ret(basic_iterator &)(
                /// \pre
                requires(!ranges_detail::has_cursor_next<Cur>)) {
            return *this;
        }

      private:
        constexpr basic_iterator post_increment_(std::false_type, int) {
            basic_iterator tmp{*this};
            ++*this;
            return tmp;
        }
        // Attempt to satisfy the C++17 iterator requirements by returning a
        // proxy from postfix increment:
        template(typename A = assoc_types_, typename V = typename A::value_type)(
            /// \pre
            requires constructible_from<V, typename A::reference> AND
                move_constructible<V>) constexpr auto post_increment_(std::true_type, int) //
            -> ranges_detail::postfix_increment_proxy<V> {
            ranges_detail::postfix_increment_proxy<V> p{**this};
            ++*this;
            return p;
        }
        constexpr void post_increment_(std::true_type, long) { ++*this; }

      public:
        CPP_member constexpr auto operator++(int) {
            return this->post_increment_(futures::detail::meta::bool_ < ranges_detail::input_cursor<Cur> && !ranges_detail::forward_cursor < Cur >> {},
                                         0);
        }

        CPP_member constexpr auto operator--() -> CPP_ret(basic_iterator &)(
            /// \pre
            requires ranges_detail::bidirectional_cursor<Cur>) {
            range_access::prev(pos());
            return *this;
        }
        CPP_member constexpr auto operator--(int) //
            -> CPP_ret(basic_iterator)(
                /// \pre
                requires ranges_detail::bidirectional_cursor<Cur>) {
            basic_iterator tmp(*this);
            --*this;
            return tmp;
        }
        CPP_member constexpr auto operator+=(difference_type n) //
            -> CPP_ret(basic_iterator &)(
                /// \pre
                requires ranges_detail::random_access_cursor<Cur>) {
            range_access::advance(pos(), n);
            return *this;
        }
        CPP_member constexpr auto operator-=(difference_type n) //
            -> CPP_ret(basic_iterator &)(
                /// \pre
                requires ranges_detail::random_access_cursor<Cur>) {
            range_access::advance(pos(), (difference_type)-n);
            return *this;
        }
        CPP_member constexpr auto operator[](difference_type n) const //
            -> CPP_ret(const_reference_t)(
                /// \pre
                requires ranges_detail::random_access_cursor<Cur>) {
            return *(*this + n);
        }

#if !RANGES_BROKEN_CPO_LOOKUP
        // Optionally support hooking iter_move when the cursor sports a
        // move() member function.
        template <typename C = Cur>
        friend constexpr auto
        iter_move(basic_iterator const &it) noexcept(noexcept(range_access::move(std::declval<C const &>())))
            -> CPP_broken_friend_ret(decltype(range_access::move(std::declval<C const &>())))(
                /// \pre
                requires same_as<C, Cur> &&ranges_detail::input_cursor<Cur>) {
            return range_access::move(it.pos());
        }
#endif
    };

    template(typename Cur, typename Cur2)(
        /// \pre
        requires ranges_detail::sentinel_for_cursor<Cur2, Cur>) constexpr bool
    operator==(basic_iterator<Cur> const &left, basic_iterator<Cur2> const &right) {
        return range_access::equal(range_access::pos(left), range_access::pos(right));
    }
    template(typename Cur, typename Cur2)(
        /// \pre
        requires ranges_detail::sentinel_for_cursor<Cur2, Cur>) constexpr bool
    operator!=(basic_iterator<Cur> const &left, basic_iterator<Cur2> const &right) {
        return !(left == right);
    }
    template(typename Cur, typename S)(
        /// \pre
        requires ranges_detail::sentinel_for_cursor<S, Cur>) constexpr bool
    operator==(basic_iterator<Cur> const &left, S const &right) {
        return range_access::equal(range_access::pos(left), right);
    }
    template(typename Cur, typename S)(
        /// \pre
        requires ranges_detail::sentinel_for_cursor<S, Cur>) constexpr bool
    operator!=(basic_iterator<Cur> const &left, S const &right) {
        return !(left == right);
    }
    template(typename S, typename Cur)(
        /// \pre
        requires ranges_detail::sentinel_for_cursor<S, Cur>) constexpr bool
    operator==(S const &left, basic_iterator<Cur> const &right) {
        return right == left;
    }
    template(typename S, typename Cur)(
        /// \pre
        requires ranges_detail::sentinel_for_cursor<S, Cur>) constexpr bool
    operator!=(S const &left, basic_iterator<Cur> const &right) {
        return right != left;
    }

    template(typename Cur)(
        /// \pre
        requires ranges_detail::random_access_cursor<Cur>) constexpr basic_iterator<Cur> //
    operator+(basic_iterator<Cur> left, typename basic_iterator<Cur>::difference_type n) {
        left += n;
        return left;
    }
    template(typename Cur)(
        /// \pre
        requires ranges_detail::random_access_cursor<Cur>) constexpr basic_iterator<Cur> //
    operator+(typename basic_iterator<Cur>::difference_type n, basic_iterator<Cur> right) {
        right += n;
        return right;
    }
    template(typename Cur)(
        /// \pre
        requires ranges_detail::random_access_cursor<Cur>) constexpr basic_iterator<Cur> //
    operator-(basic_iterator<Cur> left, typename basic_iterator<Cur>::difference_type n) {
        left -= n;
        return left;
    }
    template(typename Cur2, typename Cur)(
        /// \pre
        requires ranges_detail::sized_sentinel_for_cursor<Cur2, Cur>) constexpr
        typename basic_iterator<Cur>::difference_type //
        operator-(basic_iterator<Cur2> const &left, basic_iterator<Cur> const &right) {
        return range_access::distance_to(range_access::pos(right), range_access::pos(left));
    }
    template(typename S, typename Cur)(
        /// \pre
        requires ranges_detail::sized_sentinel_for_cursor<S, Cur>) constexpr typename basic_iterator<Cur>::difference_type //
    operator-(S const &left, basic_iterator<Cur> const &right) {
        return range_access::distance_to(range_access::pos(right), left);
    }
    template(typename Cur, typename S)(
        /// \pre
        requires ranges_detail::sized_sentinel_for_cursor<S, Cur>) constexpr typename basic_iterator<Cur>::difference_type //
    operator-(basic_iterator<Cur> const &left, S const &right) {
        return -(right - left);
    }
    // Asymmetric comparisons
    template(typename Left, typename Right)(
        /// \pre
        requires ranges_detail::sized_sentinel_for_cursor<Right, Left>) constexpr bool
    operator<(basic_iterator<Left> const &left, basic_iterator<Right> const &right) {
        return 0 < (right - left);
    }
    template(typename Left, typename Right)(
        /// \pre
        requires ranges_detail::sized_sentinel_for_cursor<Right, Left>) constexpr bool
    operator<=(basic_iterator<Left> const &left, basic_iterator<Right> const &right) {
        return 0 <= (right - left);
    }
    template(typename Left, typename Right)(
        /// \pre
        requires ranges_detail::sized_sentinel_for_cursor<Right, Left>) constexpr bool
    operator>(basic_iterator<Left> const &left, basic_iterator<Right> const &right) {
        return (right - left) < 0;
    }
    template(typename Left, typename Right)(
        /// \pre
        requires ranges_detail::sized_sentinel_for_cursor<Right, Left>) constexpr bool
    operator>=(basic_iterator<Left> const &left, basic_iterator<Right> const &right) {
        return (right - left) <= 0;
    }

#if RANGES_BROKEN_CPO_LOOKUP
    namespace _basic_iterator_ {
        // Optionally support hooking iter_move when the cursor sports a
        // move() member function.
        template <typename Cur>
        constexpr auto
        iter_move(basic_iterator<Cur> const &it) noexcept(noexcept(range_access::move(std::declval<Cur const &>())))
            -> CPP_broken_friend_ret(decltype(range_access::move(std::declval<Cur const &>())))(
                /// \pre
                requires ranges_detail::input_cursor<Cur>) {
            return range_access::move(range_access::pos(it));
        }
    } // namespace _basic_iterator_
#endif

    /// Get a cursor from a basic_iterator
    struct get_cursor_fn {
        template <typename Cur> constexpr Cur &operator()(basic_iterator<Cur> &it) const noexcept {
            return range_access::pos(it);
        }
        template <typename Cur> constexpr Cur const &operator()(basic_iterator<Cur> const &it) const noexcept {
            return range_access::pos(it);
        }
        template <typename Cur>
        constexpr Cur operator()(basic_iterator<Cur> &&it) const
            noexcept(std::is_nothrow_move_constructible<Cur>::value) {
            return range_access::pos(std::move(it));
        }
    };

    /// \sa `get_cursor_fn`
    RANGES_INLINE_VARIABLE(get_cursor_fn, get_cursor)
    /// @}
} // namespace futures::detail

/// \cond
namespace futures::detail::concepts {
    // common_reference specializations for basic_proxy_reference
    template <typename Cur, typename U, template <typename> class TQual, template <typename> class UQual>
    struct basic_common_reference<::futures::detail::ranges_detail::basic_proxy_reference_<Cur, true>, U, TQual, UQual>
        : basic_common_reference<::futures::detail::ranges_detail::cursor_reference_t<Cur>, U, TQual, UQual> {};
    template <typename T, typename Cur, template <typename> class TQual, template <typename> class UQual>
    struct basic_common_reference<T, ::futures::detail::ranges_detail::basic_proxy_reference_<Cur, true>, TQual, UQual>
        : basic_common_reference<T, ::futures::detail::ranges_detail::cursor_reference_t<Cur>, TQual, UQual> {};
    template <typename Cur1, typename Cur2, template <typename> class TQual, template <typename> class UQual>
    struct basic_common_reference<::futures::detail::ranges_detail::basic_proxy_reference_<Cur1, true>,
                                  ::futures::detail::ranges_detail::basic_proxy_reference_<Cur2, true>, TQual, UQual>
        : basic_common_reference<::futures::detail::ranges_detail::cursor_reference_t<Cur1>, ::futures::detail::ranges_detail::cursor_reference_t<Cur2>,
                                 TQual, UQual> {};

    // common_type specializations for basic_proxy_reference
    template <typename Cur, typename U>
    struct common_type<::futures::detail::ranges_detail::basic_proxy_reference_<Cur, true>, U>
        : common_type<::futures::detail::range_access::cursor_value_t<Cur>, U> {};
    template <typename T, typename Cur>
    struct common_type<T, ::futures::detail::ranges_detail::basic_proxy_reference_<Cur, true>>
        : common_type<T, ::futures::detail::range_access::cursor_value_t<Cur>> {};
    template <typename Cur1, typename Cur2>
    struct common_type<::futures::detail::ranges_detail::basic_proxy_reference_<Cur1, true>,
                       ::futures::detail::ranges_detail::basic_proxy_reference_<Cur2, true>>
        : common_type<::futures::detail::range_access::cursor_value_t<Cur1>, ::futures::detail::range_access::cursor_value_t<Cur2>> {};
} // namespace futures::detail::concepts

#if RANGES_CXX_VER > RANGES_CXX_STD_17
RANGES_DIAGNOSTIC_PUSH
RANGES_DIAGNOSTIC_IGNORE_MISMATCHED_TAGS
RANGES_BEGIN_NAMESPACE_STD
RANGES_BEGIN_NAMESPACE_VERSION
template <typename, typename, template <typename> class, template <typename> class> struct basic_common_reference;

// common_reference specializations for basic_proxy_reference
template <typename Cur, typename U, template <typename> class TQual, template <typename> class UQual>
struct basic_common_reference<::futures::detail::ranges_detail::basic_proxy_reference_<Cur, true>, U, TQual, UQual>
    : basic_common_reference<::futures::detail::ranges_detail::cursor_reference_t<Cur>, U, TQual, UQual> {};
template <typename T, typename Cur, template <typename> class TQual, template <typename> class UQual>
struct basic_common_reference<T, ::futures::detail::ranges_detail::basic_proxy_reference_<Cur, true>, TQual, UQual>
    : basic_common_reference<T, ::futures::detail::ranges_detail::cursor_reference_t<Cur>, TQual, UQual> {};
template <typename Cur1, typename Cur2, template <typename> class TQual, template <typename> class UQual>
struct basic_common_reference<::futures::detail::ranges_detail::basic_proxy_reference_<Cur1, true>,
                              ::futures::detail::ranges_detail::basic_proxy_reference_<Cur2, true>, TQual, UQual>
    : basic_common_reference<::futures::detail::ranges_detail::cursor_reference_t<Cur1>, ::futures::detail::ranges_detail::cursor_reference_t<Cur2>,
                             TQual, UQual> {};

template <typename...> struct common_type;

// common_type specializations for basic_proxy_reference
template <typename Cur, typename U>
struct common_type<::futures::detail::ranges_detail::basic_proxy_reference_<Cur, true>, U>
    : common_type<::futures::detail::range_access::cursor_value_t<Cur>, U> {};
template <typename T, typename Cur>
struct common_type<T, ::futures::detail::ranges_detail::basic_proxy_reference_<Cur, true>>
    : common_type<T, ::futures::detail::range_access::cursor_value_t<Cur>> {};
template <typename Cur1, typename Cur2>
struct common_type<::futures::detail::ranges_detail::basic_proxy_reference_<Cur1, true>,
                   ::futures::detail::ranges_detail::basic_proxy_reference_<Cur2, true>>
    : common_type<::futures::detail::range_access::cursor_value_t<Cur1>, ::futures::detail::range_access::cursor_value_t<Cur2>> {};
RANGES_END_NAMESPACE_VERSION
RANGES_END_NAMESPACE_STD
RANGES_DIAGNOSTIC_POP
#endif // RANGES_CXX_VER > RANGES_CXX_STD_17

namespace futures::detail {
    /// \cond
    namespace ranges_detail {
        template <typename Cur, bool IsReadable> struct std_iterator_traits_ {
            using difference_type = typename iterator_associated_types_base<Cur>::difference_type;
            using value_type = void;
            using reference = void;
            using pointer = void;
            using iterator_category = std::output_iterator_tag;
            using iterator_concept = std::output_iterator_tag;
        };

        template <typename Cur> struct std_iterator_traits_<Cur, true> : iterator_associated_types_base<Cur> {};

        template <typename Cur> using std_iterator_traits = std_iterator_traits_<Cur, (bool)readable_cursor<Cur>>;
    } // namespace ranges_detail
    /// \endcond
} // namespace futures::detail

namespace std {
    template <typename Cur>
    struct iterator_traits<::futures::detail::basic_iterator<Cur>> : ::futures::detail::ranges_detail::std_iterator_traits<Cur> {};
} // namespace std
/// \endcond

RANGES_DIAGNOSTIC_POP

// #include <futures/algorithm/detail/traits/range/detail/epilogue.h>
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

#ifndef RANGES_PROLOGUE_INCLUDED
#error "Including epilogue, but prologue not included!"
#endif
#undef RANGES_PROLOGUE_INCLUDED

#undef template
#undef AND

RANGES_DIAGNOSTIC_POP


#endif

// #include <futures/algorithm/detail/traits/range/iterator/concepts.h>


// #include <futures/algorithm/detail/traits/range/detail/prologue.h>
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

#ifndef FUTURES_RANGES_DETAIL_PROLOGUE_HPP
#define FUTURES_RANGES_DETAIL_PROLOGUE_HPP
// #include <futures/algorithm/detail/traits/range/detail/config.h>

#endif

#ifdef RANGES_PROLOGUE_INCLUDED
#error "Prologue already included!"
#endif
#define RANGES_PROLOGUE_INCLUDED

RANGES_DIAGNOSTIC_PUSH

#ifdef RANGES_FEWER_WARNINGS
RANGES_DIAGNOSTIC_IGNORE_UNDEFINED_INTERNAL
RANGES_DIAGNOSTIC_IGNORE_INDENTATION
RANGES_DIAGNOSTIC_IGNORE_CXX17_COMPAT
#endif

RANGES_DIAGNOSTIC_KEYWORD_MACRO

#define template(...)                                                                                                  \
    CPP_PP_IGNORE_CXX2A_COMPAT_BEGIN                                                                                   \
    template <__VA_ARGS__ CPP_TEMPLATE_AUX_

#define AND CPP_and


namespace futures::detail {
    /// \addtogroup group-iterator
    /// @{

    /// \cond
    namespace ranges_detail {
        template <typename I> struct reverse_cursor {
          private:
            CPP_assert(bidirectional_iterator<I>);
            friend range_access;
            template <typename OtherI> friend struct reverse_cursor;
            struct mixin : basic_mixin<reverse_cursor> {
                mixin() = default;
#ifndef _MSC_VER
                using basic_mixin<reverse_cursor>::basic_mixin;
#else
                constexpr explicit mixin(reverse_cursor &&cur)
                    : basic_mixin<reverse_cursor>(static_cast<reverse_cursor &&>(cur)) {}
                constexpr explicit mixin(reverse_cursor const &cur) : basic_mixin<reverse_cursor>(cur) {}
#endif
                constexpr mixin(I it) : mixin{reverse_cursor{it}} {}
                constexpr I base() const { return this->get().base(); }
            };

            I it_;

            constexpr reverse_cursor(I it) : it_(std::move(it)) {}
            constexpr iter_reference_t<I> read() const { return *arrow(); }
            constexpr I arrow() const {
                auto tmp = it_;
                --tmp;
                return tmp;
            }
            constexpr I base() const { return it_; }
            template(typename J)(
                /// \pre
                requires sentinel_for<J, I>) constexpr bool equal(reverse_cursor<J> const &that) const {
                return it_ == that.it_;
            }
            constexpr void next() { --it_; }
            constexpr void prev() { ++it_; }
            CPP_member constexpr auto advance(iter_difference_t<I> n) //
                -> CPP_ret(void)(
                    /// \pre
                    requires random_access_iterator<I>) {
                it_ -= n;
            }
            template(typename J)(
                /// \pre
                requires sized_sentinel_for<J, I>) constexpr iter_difference_t<I> distance_to(reverse_cursor<J> const
                                                                                                  &that) //
                const {
                return it_ - that.base();
            }
            constexpr iter_rvalue_reference_t<I> move() const
                noexcept(noexcept((void)I(I(it_)), (void)--const_cast<I &>(it_), iter_move(it_))) {
                auto tmp = it_;
                --tmp;
                return iter_move(tmp);
            }

          public:
            reverse_cursor() = default;
            template(typename U)(
                /// \pre
                requires convertible_to<U, I>) constexpr reverse_cursor(reverse_cursor<U> const &u)
                : it_(u.base()) {}
        };
    } // namespace ranges_detail
    /// \endcond

    struct make_reverse_iterator_fn {
        template(typename I)(
            /// \pre
            requires bidirectional_iterator<I>) constexpr reverse_iterator<I>
        operator()(I i) const {
            return reverse_iterator<I>(i);
        }
    };

    RANGES_INLINE_VARIABLE(make_reverse_iterator_fn, make_reverse_iterator)

    namespace cpp20 {
        using futures::detail::make_reverse_iterator;
        using futures::detail::reverse_iterator;
    } // namespace cpp20
    /// @}
} // namespace futures::detail

// #include <futures/algorithm/detail/traits/range/detail/epilogue.h>
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

#ifndef RANGES_PROLOGUE_INCLUDED
#error "Including epilogue, but prologue not included!"
#endif
#undef RANGES_PROLOGUE_INCLUDED

#undef template
#undef AND

RANGES_DIAGNOSTIC_POP


#endif // FUTURES_RANGES_ITERATOR_REVERSE_ITERATOR_HPP

// #include <futures/algorithm/detail/traits/range/iterator/traits.h>

// #include <futures/algorithm/detail/traits/range/utility/static_const.h>


// #include <futures/algorithm/detail/traits/range/detail/prologue.h>
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

#ifndef FUTURES_RANGES_DETAIL_PROLOGUE_HPP
#define FUTURES_RANGES_DETAIL_PROLOGUE_HPP
// #include <futures/algorithm/detail/traits/range/detail/config.h>

#endif

#ifdef RANGES_PROLOGUE_INCLUDED
#error "Prologue already included!"
#endif
#define RANGES_PROLOGUE_INCLUDED

RANGES_DIAGNOSTIC_PUSH

#ifdef RANGES_FEWER_WARNINGS
RANGES_DIAGNOSTIC_IGNORE_UNDEFINED_INTERNAL
RANGES_DIAGNOSTIC_IGNORE_INDENTATION
RANGES_DIAGNOSTIC_IGNORE_CXX17_COMPAT
#endif

RANGES_DIAGNOSTIC_KEYWORD_MACRO

#define template(...)                                                                                                  \
    CPP_PP_IGNORE_CXX2A_COMPAT_BEGIN                                                                                   \
    template <__VA_ARGS__ CPP_TEMPLATE_AUX_

#define AND CPP_and


namespace futures::detail {
#if defined(__cpp_lib_string_view) && __cpp_lib_string_view > 0
    template <class CharT, class Traits>
    RANGES_INLINE_VAR constexpr bool enable_borrowed_range<std::basic_string_view<CharT, Traits>> = true;
#endif

#if defined(__cpp_lib_span) && __cpp_lib_span > 0
    template <class T, std::size_t N> RANGES_INLINE_VAR constexpr bool enable_borrowed_range<std::span<T, N>> = true;
#endif

    namespace ranges_detail {
        template <typename T> RANGES_INLINE_VAR constexpr bool _borrowed_range = enable_borrowed_range<uncvref_t<T>>;

        template <typename T> RANGES_INLINE_VAR constexpr bool _borrowed_range<T &> = true;
    } // namespace ranges_detail

    /// \cond
    namespace _begin_ {
        // Poison pill for std::begin. (See the detailed discussion at
        // https://github.com/ericniebler/stl2/issues/139)
        template <typename T> void begin(T &&) = delete;

#ifdef RANGES_WORKAROUND_MSVC_895622
        void begin();
#endif

        template <typename T> void begin(std::initializer_list<T>) = delete;

        template(typename I)(
            /// \pre
            requires input_or_output_iterator<I>) void is_iterator(I);

        // clang-format off
        template<typename T>
        CPP_requires(has_member_begin_,
            requires(T & t) //
            (
                _begin_::is_iterator(t.begin())
            ));
        template<typename T>
        CPP_concept has_member_begin =
            CPP_requires_ref(_begin_::has_member_begin_, T);

        template<typename T>
        CPP_requires(has_non_member_begin_,
            requires(T & t) //
            (
                _begin_::is_iterator(begin(t))
            ));
        template<typename T>
        CPP_concept has_non_member_begin =
            CPP_requires_ref(_begin_::has_non_member_begin_, T);
        // clang-format on

        struct fn {
          private:
            template <bool> struct impl_ {
                // has_member_begin == true
                template <typename R> static constexpr auto invoke(R &&r) noexcept(noexcept(r.begin())) {
                    return r.begin();
                }
            };

            template <typename R> using impl = impl_<has_member_begin<R>>;

          public:
            template <typename R, std::size_t N> void operator()(R(&&)[N]) const = delete;

            template <typename R, std::size_t N> constexpr R *operator()(R (&array)[N]) const noexcept { return array; }

            template(typename R)(
                /// \pre
                requires ranges_detail::_borrowed_range<R> AND(has_member_begin<R> || has_non_member_begin<R>)) constexpr auto
            operator()(R &&r) const //
                noexcept(noexcept(impl<R>::invoke(r))) {
                return impl<R>::invoke(r);
            }

            template <typename T, typename Fn = fn>
            RANGES_DEPRECATED("Using a reference_wrapper as a range is deprecated. Use views::ref "
                              "instead.")
            constexpr auto
            operator()(std::reference_wrapper<T> ref) const noexcept(noexcept(Fn{}(ref.get())))
                -> decltype(Fn{}(ref.get())) {
                return Fn{}(ref.get());
            }

            template <typename T, typename Fn = fn>
            RANGES_DEPRECATED("Using a reference_wrapper as a range is deprecated. Use views::ref "
                              "instead.")
            constexpr auto
            operator()(futures::detail::reference_wrapper<T> ref) const noexcept(noexcept(Fn{}(ref.get())))
                -> decltype(Fn{}(ref.get())) {
                return Fn{}(ref.get());
            }
        };

        template <> struct fn::impl_<false> {
            // has_member_begin == false
            template <typename R> static constexpr auto invoke(R &&r) noexcept(noexcept(begin(r))) { return begin(r); }
        };

        template <typename R> using _t = decltype(fn{}(std::declval<R>()));
    } // namespace _begin_
    /// \endcond

    /// \ingroup group-range
    /// \param r
    /// \return \c r, if \c r is an array. Otherwise, `r.begin()` if that expression is
    ///   well-formed and returns an input_or_output_iterator. Otherwise, `begin(r)` if
    ///   that expression returns an input_or_output_iterator.
    RANGES_DEFINE_CPO(_begin_::fn, begin)

    /// \cond
    namespace _end_ {
        // Poison pill for std::end. (See the detailed discussion at
        // https://github.com/ericniebler/stl2/issues/139)
        template <typename T> void end(T &&) = delete;

#ifdef RANGES_WORKAROUND_MSVC_895622
        void end();
#endif

        template <typename T> void end(std::initializer_list<T>) = delete;

        template(typename I, typename S)(
            /// \pre
            requires sentinel_for<S, I>) void _is_sentinel(S, I);

        // clang-format off
        template<typename T>
        CPP_requires(has_member_end_,
            requires(T & t) //
            (
                _end_::_is_sentinel(t.end(), futures::detail::begin(t))
            ));
        template<typename T>
        CPP_concept has_member_end =
            CPP_requires_ref(_end_::has_member_end_, T);

        template<typename T>
        CPP_requires(has_non_member_end_,
            requires(T & t) //
            (
                _end_::_is_sentinel(end(t), futures::detail::begin(t))
            ));
        template<typename T>
        CPP_concept has_non_member_end =
            CPP_requires_ref(_end_::has_non_member_end_, T);
        // clang-format on

        struct fn {
          private:
            template <bool> struct impl_ {
                // has_member_end == true
                template <typename R> static constexpr auto invoke(R &&r) noexcept(noexcept(r.end())) {
                    return r.end();
                }
            };

            template <typename Int>
            using iter_diff_t = futures::detail::meta::_t<futures::detail::meta::conditional_t<std::is_integral<Int>::value,
                                                             std::make_signed<Int>, //
                                                             futures::detail::meta::id<Int>>>;

            template <typename R> using impl = impl_<has_member_end<R>>;

          public:
            template <typename R, std::size_t N> void operator()(R(&&)[N]) const = delete;

            template <typename R, std::size_t N> constexpr R *operator()(R (&array)[N]) const noexcept {
                return array + N;
            }

            template(typename R)(
                /// \pre
                requires ranges_detail::_borrowed_range<R> AND(has_member_end<R> || has_non_member_end<R>)) constexpr auto
            operator()(R &&r) const                    //
                noexcept(noexcept(impl<R>::invoke(r))) //
            {
                return impl<R>::invoke(r);
            }

            template <typename T, typename Fn = fn>
            RANGES_DEPRECATED("Using a reference_wrapper as a range is deprecated. Use views::ref "
                              "instead.")
            constexpr auto
            operator()(std::reference_wrapper<T> ref) const noexcept(noexcept(Fn{}(ref.get())))
                -> decltype(Fn{}(ref.get())) {
                return Fn{}(ref.get());
            }

            template <typename T, typename Fn = fn>
            RANGES_DEPRECATED("Using a reference_wrapper as a range is deprecated. Use views::ref "
                              "instead.")
            constexpr auto
            operator()(futures::detail::reference_wrapper<T> ref) const noexcept(noexcept(Fn{}(ref.get())))
                -> decltype(Fn{}(ref.get())) {
                return Fn{}(ref.get());
            }

            template(typename Int)(
                /// \pre
                requires ranges_detail::integer_like_<Int>) auto
            operator-(Int dist) const -> ranges_detail::from_end_<iter_diff_t<Int>> {
                using SInt = iter_diff_t<Int>;
                RANGES_EXPECT(0 <= dist);
                RANGES_EXPECT(dist <= static_cast<Int>((std::numeric_limits<SInt>::max)()));
                return ranges_detail::from_end_<SInt>{-static_cast<SInt>(dist)};
            }
        };

        // has_member_end == false
        template <> struct fn::impl_<false> {
            template <typename R> static constexpr auto invoke(R &&r) noexcept(noexcept(end(r))) { return end(r); }
        };

        template <typename R> using _t = decltype(fn{}(std::declval<R>()));
    } // namespace _end_
    /// \endcond

    /// \ingroup group-range
    /// \param r
    /// \return \c r+size(r), if \c r is an array. Otherwise, `r.end()` if that expression
    /// is
    ///   well-formed and returns an input_or_output_iterator. Otherwise, `end(r)` if that
    ///   expression returns an input_or_output_iterator.
    RANGES_DEFINE_CPO(_end_::fn, end)

    /// \cond
    namespace _cbegin_ {
        struct fn {
            template <typename R>
            constexpr _begin_::_t<ranges_detail::as_const_t<R>> operator()(R &&r) const
                noexcept(noexcept(futures::detail::begin(ranges_detail::as_const(r)))) {
                return futures::detail::begin(ranges_detail::as_const(r));
            }
        };
    } // namespace _cbegin_
    /// \endcond

    /// \ingroup group-range
    /// \param r
    /// \return The result of calling `futures::detail::begin` with a const-qualified
    ///    reference to r.
    RANGES_INLINE_VARIABLE(_cbegin_::fn, cbegin)

    /// \cond
    namespace _cend_ {
        struct fn {
            template <typename R>
            constexpr _end_::_t<ranges_detail::as_const_t<R>> operator()(R &&r) const
                noexcept(noexcept(futures::detail::end(ranges_detail::as_const(r)))) {
                return futures::detail::end(ranges_detail::as_const(r));
            }
        };
    } // namespace _cend_
    /// \endcond

    /// \ingroup group-range
    /// \param r
    /// \return The result of calling `futures::detail::end` with a const-qualified
    ///    reference to r.
    RANGES_INLINE_VARIABLE(_cend_::fn, cend)

    /// \cond
    namespace _rbegin_ {
        template <typename R> void rbegin(R &&) = delete;
        // Non-standard, to keep unqualified rbegin(r) from finding std::rbegin
        // and returning a std::reverse_iterator.
        template <typename T> void rbegin(std::initializer_list<T>) = delete;
        template <typename T, std::size_t N> void rbegin(T (&)[N]) = delete;

        // clang-format off
        template<typename T>
        CPP_requires(has_member_rbegin_,
            requires(T & t) //
            (
                _begin_::is_iterator(t.rbegin())
            ));
        template<typename T>
        CPP_concept has_member_rbegin =
            CPP_requires_ref(_rbegin_::has_member_rbegin_, T);

        template<typename T>
        CPP_requires(has_non_member_rbegin_,
            requires(T & t) //
            (
                _begin_::is_iterator(rbegin(t))
            ));
        template<typename T>
        CPP_concept has_non_member_rbegin =
            CPP_requires_ref(_rbegin_::has_non_member_rbegin_, T);

        template<typename I>
        void _same_type(I, I);

        template<typename T>
        CPP_requires(can_reverse_end_,
            requires(T & t) //
            (
                // make_reverse_iterator is constrained with
                // bidirectional_iterator.
                futures::detail::make_reverse_iterator(futures::detail::end(t)),
                _rbegin_::_same_type(futures::detail::begin(t), futures::detail::end(t))
            ));
        template<typename T>
        CPP_concept can_reverse_end =
            CPP_requires_ref(_rbegin_::can_reverse_end_, T);
        // clang-format on

        struct fn {
          private:
            // has_member_rbegin == true
            template <int> struct impl_ {
                template <typename R> static constexpr auto invoke(R &&r) noexcept(noexcept(r.rbegin())) {
                    return r.rbegin();
                }
            };

            template <typename R> using impl = impl_<has_member_rbegin<R> ? 0 : has_non_member_rbegin<R> ? 1 : 2>;

          public:
            template(typename R)(
                /// \pre
                requires ranges_detail::_borrowed_range<R> AND(has_member_rbegin<R> || has_non_member_rbegin<R> ||
                                                        can_reverse_end<R>)) //
                constexpr auto
                operator()(R &&r) const                //
                noexcept(noexcept(impl<R>::invoke(r))) //
            {
                return impl<R>::invoke(r);
            }

            template <typename T, typename Fn = fn>
            RANGES_DEPRECATED("Using a reference_wrapper as a range is deprecated. Use views::ref "
                              "instead.")
            constexpr auto
            operator()(std::reference_wrapper<T> ref) const //
                noexcept(noexcept(Fn{}(ref.get())))         //
                -> decltype(Fn{}(ref.get())) {
                return Fn{}(ref.get());
            }

            template <typename T, typename Fn = fn>
            RANGES_DEPRECATED("Using a reference_wrapper as a range is deprecated. Use views::ref "
                              "instead.")
            constexpr auto
            operator()(futures::detail::reference_wrapper<T> ref) const noexcept(noexcept(Fn{}(ref.get()))) //
                -> decltype(Fn{}(ref.get())) {
                return Fn{}(ref.get());
            }
        };

        // has_non_member_rbegin == true
        template <> struct fn::impl_<1> {
            template <typename R> static constexpr auto invoke(R &&r) noexcept(noexcept(rbegin(r))) {
                return rbegin(r);
            }
        };

        // can_reverse_end
        template <> struct fn::impl_<2> {
            template <typename R>
            static constexpr auto invoke(R &&r) noexcept(noexcept(futures::detail::make_reverse_iterator(futures::detail::end(r)))) {
                return futures::detail::make_reverse_iterator(futures::detail::end(r));
            }
        };

        template <typename R> using _t = decltype(fn{}(std::declval<R>()));
    } // namespace _rbegin_
    /// \endcond

    /// \ingroup group-range
    /// \param r
    /// \return `make_reverse_iterator(r + futures::detail::size(r))` if r is an array. Otherwise,
    ///   `r.rbegin()` if that expression is well-formed and returns an
    ///   input_or_output_iterator. Otherwise, `make_reverse_iterator(futures::detail::end(r))` if
    ///   `futures::detail::begin(r)` and `futures::detail::end(r)` are both well-formed and have the same
    ///   type that satisfies `bidirectional_iterator`.
    RANGES_DEFINE_CPO(_rbegin_::fn, rbegin)

    /// \cond
    namespace _rend_ {
        template <typename R> void rend(R &&) = delete;
        // Non-standard, to keep unqualified rend(r) from finding std::rend
        // and returning a std::reverse_iterator.
        template <typename T> void rend(std::initializer_list<T>) = delete;
        template <typename T, std::size_t N> void rend(T (&)[N]) = delete;

        // clang-format off
        template<typename T>
        CPP_requires(has_member_rend_,
            requires(T & t) //
            (
                _end_::_is_sentinel(t.rend(), futures::detail::rbegin(t))
            ));
        template<typename T>
        CPP_concept has_member_rend =
            CPP_requires_ref(_rend_::has_member_rend_, T);

        template<typename T>
        CPP_requires(has_non_member_rend_,
            requires(T & t) //
            (
                _end_::_is_sentinel(rend(t), futures::detail::rbegin(t))
            ));
        template<typename T>
        CPP_concept has_non_member_rend =
            CPP_requires_ref(_rend_::has_non_member_rend_, T);

        template<typename T>
        CPP_requires(can_reverse_begin_,
            requires(T & t) //
            (
                // make_reverse_iterator is constrained with
                // bidirectional_iterator.
                futures::detail::make_reverse_iterator(futures::detail::begin(t)),
                _rbegin_::_same_type(futures::detail::begin(t), futures::detail::end(t))
            ));
        template<typename T>
        CPP_concept can_reverse_begin =
            CPP_requires_ref(_rend_::can_reverse_begin_, T);
        // clang-format on

        struct fn {
          private:
            // has_member_rbegin == true
            template <int> struct impl_ {
                template <typename R> static constexpr auto invoke(R &&r) noexcept(noexcept(r.rend())) {
                    return r.rend();
                }
            };

            template <typename R> using impl = impl_<has_member_rend<R> ? 0 : has_non_member_rend<R> ? 1 : 2>;

          public:
            template(typename R)(
                /// \pre
                requires ranges_detail::_borrowed_range<R> AND(has_member_rend<R> ||     //
                                                        has_non_member_rend<R> || //
                                                        can_reverse_begin<R>))    //
                constexpr auto
                operator()(R &&r) const noexcept(noexcept(impl<R>::invoke(r))) //
            {
                return impl<R>::invoke(r);
            }

            template <typename T, typename Fn = fn>
            RANGES_DEPRECATED("Using a reference_wrapper as a range is deprecated. Use views::ref "
                              "instead.")
            constexpr auto
            operator()(std::reference_wrapper<T> ref) const noexcept(noexcept(Fn{}(ref.get()))) //
                -> decltype(Fn{}(ref.get())) {
                return Fn{}(ref.get());
            }

            template <typename T, typename Fn = fn>
            RANGES_DEPRECATED("Using a reference_wrapper as a range is deprecated. Use views::ref "
                              "instead.")
            constexpr auto
            operator()(futures::detail::reference_wrapper<T> ref) const noexcept(noexcept(Fn{}(ref.get()))) //
                -> decltype(Fn{}(ref.get())) {
                return Fn{}(ref.get());
            }
        };

        // has_non_member_rend == true
        template <> struct fn::impl_<1> {
            template <typename R> static constexpr auto invoke(R &&r) noexcept(noexcept(rend(r))) { return rend(r); }
        };

        // can_reverse_begin
        template <> struct fn::impl_<2> {
            template <typename R>
            static constexpr auto invoke(R &&r) noexcept(noexcept(futures::detail::make_reverse_iterator(futures::detail::begin(r)))) {
                return futures::detail::make_reverse_iterator(futures::detail::begin(r));
            }
        };

        template <typename R> using _t = decltype(fn{}(std::declval<R>()));
    } // namespace _rend_
    /// \endcond

    /// \ingroup group-range
    /// \param r
    /// \return `make_reverse_iterator(r)` if `r` is an array. Otherwise,
    ///   `r.rend()` if that expression is well-formed and returns a type that
    ///   satisfies `sentinel_for<S, I>` where `I` is the type of `futures::detail::rbegin(r)`.
    ///   Otherwise, `make_reverse_iterator(futures::detail::begin(r))` if `futures::detail::begin(r)`
    ///   and `futures::detail::end(r)` are both well-formed and have the same type that
    ///   satisfies `bidirectional_iterator`.
    RANGES_DEFINE_CPO(_rend_::fn, rend)

    /// \cond
    namespace _crbegin_ {
        struct fn {
            template <typename R>
            constexpr _rbegin_::_t<ranges_detail::as_const_t<R>> operator()(R &&r) const
                noexcept(noexcept(futures::detail::rbegin(ranges_detail::as_const(r)))) {
                return futures::detail::rbegin(ranges_detail::as_const(r));
            }
        };
    } // namespace _crbegin_
    /// \endcond

    /// \ingroup group-range
    /// \param r
    /// \return The result of calling `futures::detail::rbegin` with a const-qualified
    ///    reference to r.
    RANGES_INLINE_VARIABLE(_crbegin_::fn, crbegin)

    /// \cond
    namespace _crend_ {
        struct fn {
            template <typename R>
            constexpr _rend_::_t<ranges_detail::as_const_t<R>> operator()(R &&r) const
                noexcept(noexcept(futures::detail::rend(ranges_detail::as_const(r)))) {
                return futures::detail::rend(ranges_detail::as_const(r));
            }
        };
    } // namespace _crend_
    /// \endcond

    /// \ingroup group-range
    /// \param r
    /// \return The result of calling `futures::detail::rend` with a const-qualified
    ///    reference to r.
    RANGES_INLINE_VARIABLE(_crend_::fn, crend)

    template <typename Rng> using iterator_t = decltype(begin(std::declval<Rng &>()));

    template <typename Rng> using sentinel_t = decltype(end(std::declval<Rng &>()));

    namespace cpp20 {
        using futures::detail::begin;
        using futures::detail::cbegin;
        using futures::detail::cend;
        using futures::detail::crbegin;
        using futures::detail::crend;
        using futures::detail::end;
        using futures::detail::rbegin;
        using futures::detail::rend;

        using futures::detail::iterator_t;
        using futures::detail::sentinel_t;

        using futures::detail::enable_borrowed_range;
    } // namespace cpp20
} // namespace futures::detail

// #include <futures/algorithm/detail/traits/range/detail/epilogue.h>
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

#ifndef RANGES_PROLOGUE_INCLUDED
#error "Including epilogue, but prologue not included!"
#endif
#undef RANGES_PROLOGUE_INCLUDED

#undef template
#undef AND

RANGES_DIAGNOSTIC_POP


#endif

// #include <futures/algorithm/detail/traits/range/range/primitives.h>
/// \file
// Range v3 library
//
//  Copyright Eric Niebler 2014-present
//
//  Use, modification and distribution is subject to the
//  Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
// Project home: https://github.com/ericniebler/range-v3
//

#ifndef FUTURES_RANGES_RANGE_PRIMITIVES_HPP
#define FUTURES_RANGES_RANGE_PRIMITIVES_HPP

// #include <futures/algorithm/detail/traits/range/concepts/concepts.h>


// #include <futures/algorithm/detail/traits/range/range_fwd.h>


// #include <futures/algorithm/detail/traits/range/iterator/concepts.h>

// #include <futures/algorithm/detail/traits/range/range/access.h>

// #include <futures/algorithm/detail/traits/range/utility/addressof.h>

// #include <futures/algorithm/detail/traits/range/utility/static_const.h>


// #include <futures/algorithm/detail/traits/range/detail/prologue.h>
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

#ifndef FUTURES_RANGES_DETAIL_PROLOGUE_HPP
#define FUTURES_RANGES_DETAIL_PROLOGUE_HPP
// #include <futures/algorithm/detail/traits/range/detail/config.h>

#endif

#ifdef RANGES_PROLOGUE_INCLUDED
#error "Prologue already included!"
#endif
#define RANGES_PROLOGUE_INCLUDED

RANGES_DIAGNOSTIC_PUSH

#ifdef RANGES_FEWER_WARNINGS
RANGES_DIAGNOSTIC_IGNORE_UNDEFINED_INTERNAL
RANGES_DIAGNOSTIC_IGNORE_INDENTATION
RANGES_DIAGNOSTIC_IGNORE_CXX17_COMPAT
#endif

RANGES_DIAGNOSTIC_KEYWORD_MACRO

#define template(...)                                                                                                  \
    CPP_PP_IGNORE_CXX2A_COMPAT_BEGIN                                                                                   \
    template <__VA_ARGS__ CPP_TEMPLATE_AUX_

#define AND CPP_and


namespace futures::detail {
    /// \addtogroup group-range
    // Specialize this if the default is wrong.
    template <typename T> RANGES_INLINE_VAR constexpr bool disable_sized_range = false;

    /// \cond
    namespace _size_ {
        template <typename T> void size(T &&) = delete;

#ifdef RANGES_WORKAROUND_MSVC_895622
        void size();
#endif

        struct fn {
          private:
            template <typename R> using member_size_t = decltype(+(std::declval<R>()).size());
            template <typename R> using non_member_size_t = decltype(+size(std::declval<R>()));

            template <typename R, std::size_t N> static constexpr std::size_t impl_(R (&)[N], int) noexcept {
                return N;
            }

            template <typename R, std::size_t N> static constexpr std::size_t impl_(R(&&)[N], int) noexcept {
                return N;
            }

            // Prefer member if it returns integral.
            template(typename R)(
                /// \pre
                requires integral<member_size_t<R>> AND(!disable_sized_range<uncvref_t<R>>)) //
                static constexpr member_size_t<R> impl_(R &&r, int)                          //
                noexcept(noexcept(((R &&) r).size())) {
                return ((R &&) r).size();
            }

            // Use ADL if it returns integral.
            template(typename R)(
                /// \pre
                requires integral<non_member_size_t<R>> AND(!disable_sized_range<uncvref_t<R>>)) //
                static constexpr non_member_size_t<R> impl_(R &&r, long)                         //
                noexcept(noexcept(size((R &&) r))) {
                return size((R &&) r);
            }

            template(typename R)(
                /// \pre
                requires forward_iterator<_begin_::_t<R>> AND
                    sized_sentinel_for<_end_::_t<R>, _begin_::_t<R>>) static constexpr auto impl_(R &&r, ...)
                -> ranges_detail::iter_size_t<_begin_::_t<R>> {
                using size_type = ranges_detail::iter_size_t<_begin_::_t<R>>;
                return static_cast<size_type>(futures::detail::end((R &&) r) - futures::detail::begin((R &&) r));
            }

          public:
            template <typename R>
            constexpr auto operator()(R &&r) const noexcept(noexcept(fn::impl_((R &&) r, 0)))
                -> decltype(fn::impl_((R &&) r, 0)) {
                return fn::impl_((R &&) r, 0);
            }

            template <typename T, typename Fn = fn>
            RANGES_DEPRECATED("Using a reference_wrapper as a Range is deprecated. Use views::ref "
                              "instead.")
            constexpr auto
            operator()(std::reference_wrapper<T> ref) const noexcept(noexcept(Fn{}(ref.get())))
                -> decltype(Fn{}(ref.get())) {
                return Fn{}(ref.get());
            }

            template <typename T, typename Fn = fn>
            RANGES_DEPRECATED("Using a reference_wrapper as a Range is deprecated. Use views::ref "
                              "instead.")
            constexpr auto
            operator()(futures::detail::reference_wrapper<T> ref) const noexcept(noexcept(Fn{}(ref.get())))
                -> decltype(Fn{}(ref.get())) {
                return Fn{}(ref.get());
            }
        };
    } // namespace _size_
    /// \endcond

    /// \ingroup group-range
    /// \return For a given expression `E` of type `T`, `futures::detail::size(E)` is equivalent
    /// to:
    ///   * `+extent_v<T>` if `T` is an array type.
    ///   * Otherwise, `+E.size()` if it is a valid expression and its type `I` models
    ///     `integral` and `disable_sized_range<std::remove_cvref_t<T>>` is false.
    ///   * Otherwise, `+size(E)` if it is a valid expression and its type `I` models
    ///     `integral` with overload resolution performed in a context that includes the
    ///     declaration:
    ///     \code
    ///     template<class T> void size(T&&) = delete;
    ///     \endcode
    ///     and does not include a declaration of `futures::detail::size`, and
    ///     `disable_sized_range<std::remove_cvref_t<T>>` is false.
    ///   * Otherwise, `static_cast<U>(futures::detail::end(E) - futures::detail::begin(E))` where `U` is
    ///     `std::make_unsigned_t<iter_difference_t<iterator_t<T>>>` if
    ///     `iter_difference_t<iterator_t<T>>` satisfies `integral` and
    ///     `iter_difference_t<iterator_t<T>>` otherwise; except that `E` is
    ///     evaluated once, if it is a valid expression and the types `I` and `S` of
    ///     `futures::detail::begin(E)` and `futures::detail::end(E)` model `sized_sentinel_for<S, I>` and
    ///     `forward_iterator<I>`.
    ///   * Otherwise, `futures::detail::size(E)` is ill-formed.
    RANGES_DEFINE_CPO(_size_::fn, size)

    // Customization point data
    /// \cond
    namespace _data_ {
        struct fn {
          private:
            template <typename R> using member_data_t = ranges_detail::decay_t<decltype(std::declval<R>().data())>;

            template(typename R)(
                /// \pre
                requires std::is_pointer<member_data_t<R &>>::value) //
                static constexpr member_data_t<R &> impl_(R &r, ranges_detail::priority_tag<2>) noexcept(noexcept(r.data())) {
                return r.data();
            }
            template(typename R)(
                /// \pre
                requires std::is_pointer<_begin_::_t<R>>::value) //
                static constexpr _begin_::_t<R> impl_(R &&r, ranges_detail::priority_tag<1>) noexcept(
                    noexcept(futures::detail::begin((R &&) r))) {
                return futures::detail::begin((R &&) r);
            }
            template(typename R)(
                /// \pre
                requires contiguous_iterator<
                    _begin_::_t<R>>) static constexpr auto impl_(R &&r,
                                                                 ranges_detail::priority_tag<
                                                                     0>) noexcept(noexcept(futures::detail::begin((R &&) r) ==
                                                                                                   futures::detail::end((R &&) r)
                                                                                               ? nullptr
                                                                                               : ranges_detail::addressof(
                                                                                                     *futures::detail::begin(
                                                                                                         (R &&) r))))
                -> decltype(ranges_detail::addressof(*futures::detail::begin((R &&) r))) {
                return futures::detail::begin((R &&) r) == futures::detail::end((R &&) r) ? nullptr
                                                                        : ranges_detail::addressof(*futures::detail::begin((R &&) r));
            }

          public:
            template <typename charT, typename Traits, typename Alloc>
            constexpr charT *operator()(std::basic_string<charT, Traits, Alloc> &s) const noexcept {
                // string doesn't have non-const data before C++17
                return const_cast<charT *>(ranges_detail::as_const(s).data());
            }

            template <typename R>
            constexpr auto operator()(R &&r) const noexcept(noexcept(fn::impl_((R &&) r, ranges_detail::priority_tag<2>{})))
                -> decltype(fn::impl_((R &&) r, ranges_detail::priority_tag<2>{})) {
                return fn::impl_((R &&) r, ranges_detail::priority_tag<2>{});
            }
        };

        template <typename R> using _t = decltype(fn{}(std::declval<R>()));
    } // namespace _data_
    /// \endcond

    RANGES_INLINE_VARIABLE(_data_::fn, data)

    /// \cond
    namespace _cdata_ {
        struct fn {
            template <typename R>
            constexpr _data_::_t<R const &> operator()(R const &r) const noexcept(noexcept(futures::detail::data(r))) {
                return futures::detail::data(r);
            }
            template <typename R>
            constexpr _data_::_t<R const> operator()(R const &&r) const
                noexcept(noexcept(futures::detail::data((R const &&)r))) {
                return futures::detail::data((R const &&)r);
            }
        };
    } // namespace _cdata_
    /// \endcond

    /// \ingroup group-range
    /// \param r
    /// \return The result of calling `futures::detail::data` with a const-qualified
    ///    (lvalue or rvalue) reference to `r`.
    RANGES_INLINE_VARIABLE(_cdata_::fn, cdata)

    /// \cond
    namespace _empty_ {
        struct fn {
          private:
            // Prefer member if it is valid.
            template <typename R>
            static constexpr auto impl_(R &&r, ranges_detail::priority_tag<2>) noexcept(noexcept(bool(((R &&) r).empty())))
                -> decltype(bool(((R &&) r).empty())) {
                return bool(((R &&) r).empty());
            }

            // Fall back to size == 0.
            template <typename R>
            static constexpr auto impl_(R &&r,
                                        ranges_detail::priority_tag<1>) noexcept(noexcept(bool(futures::detail::size((R &&) r) == 0)))
                -> decltype(bool(futures::detail::size((R &&) r) == 0)) {
                return bool(futures::detail::size((R &&) r) == 0);
            }

            // Fall further back to begin == end.
            template(typename R)(
                /// \pre
                requires forward_iterator<
                    _begin_::_t<R>>) static constexpr auto impl_(R &&r,
                                                                 ranges_detail::priority_tag<
                                                                     0>) noexcept(noexcept(bool(futures::detail::begin((R &&)
                                                                                                                  r) ==
                                                                                                futures::detail::end((R &&) r))))
                -> decltype(bool(futures::detail::begin((R &&) r) == futures::detail::end((R &&) r))) {
                return bool(futures::detail::begin((R &&) r) == futures::detail::end((R &&) r));
            }

          public:
            template <typename R>
            constexpr auto operator()(R &&r) const noexcept(noexcept(fn::impl_((R &&) r, ranges_detail::priority_tag<2>{})))
                -> decltype(fn::impl_((R &&) r, ranges_detail::priority_tag<2>{})) {
                return fn::impl_((R &&) r, ranges_detail::priority_tag<2>{});
            }

            template <typename T, typename Fn = fn>
            RANGES_DEPRECATED("Using a reference_wrapper as a Range is deprecated. Use views::ref "
                              "instead.")
            constexpr auto
            operator()(std::reference_wrapper<T> ref) const noexcept(noexcept(Fn{}(ref.get())))
                -> decltype(Fn{}(ref.get())) {
                return Fn{}(ref.get());
            }

            template <typename T, typename Fn = fn>
            RANGES_DEPRECATED("Using a reference_wrapper as a Range is deprecated. Use views::ref "
                              "instead.")
            constexpr auto
            operator()(futures::detail::reference_wrapper<T> ref) const noexcept(noexcept(Fn{}(ref.get())))
                -> decltype(Fn{}(ref.get())) {
                return Fn{}(ref.get());
            }
        };
    } // namespace _empty_
    /// \endcond

    /// \ingroup group-range
    /// \return true if and only if range contains no elements.
    RANGES_INLINE_VARIABLE(_empty_::fn, empty)

    namespace cpp20 {
        // Specialize this is namespace futures::detail::
        using futures::detail::cdata;
        using futures::detail::data;
        using futures::detail::disable_sized_range;
        using futures::detail::empty;
        using futures::detail::size;
    } // namespace cpp20
} // namespace futures::detail

// #include <futures/algorithm/detail/traits/range/detail/epilogue.h>
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

#ifndef RANGES_PROLOGUE_INCLUDED
#error "Including epilogue, but prologue not included!"
#endif
#undef RANGES_PROLOGUE_INCLUDED

#undef template
#undef AND

RANGES_DIAGNOSTIC_POP


#endif

// #include <futures/algorithm/detail/traits/range/range/traits.h>
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

#ifndef FUTURES_RANGES_RANGE_TRAITS_HPP
#define FUTURES_RANGES_RANGE_TRAITS_HPP

#include <array>
// #include <iterator>

// #include <type_traits>

// #include <utility>


// #include <futures/algorithm/detail/traits/range/meta/meta.h>


// #include <futures/algorithm/detail/traits/range/range_fwd.h>


// #include <futures/algorithm/detail/traits/range/range/access.h>

// #include <futures/algorithm/detail/traits/range/range/primitives.h>


// #include <futures/algorithm/detail/traits/range/detail/prologue.h>
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

#ifndef FUTURES_RANGES_DETAIL_PROLOGUE_HPP
#define FUTURES_RANGES_DETAIL_PROLOGUE_HPP
// #include <futures/algorithm/detail/traits/range/detail/config.h>

#endif

#ifdef RANGES_PROLOGUE_INCLUDED
#error "Prologue already included!"
#endif
#define RANGES_PROLOGUE_INCLUDED

RANGES_DIAGNOSTIC_PUSH

#ifdef RANGES_FEWER_WARNINGS
RANGES_DIAGNOSTIC_IGNORE_UNDEFINED_INTERNAL
RANGES_DIAGNOSTIC_IGNORE_INDENTATION
RANGES_DIAGNOSTIC_IGNORE_CXX17_COMPAT
#endif

RANGES_DIAGNOSTIC_KEYWORD_MACRO

#define template(...)                                                                                                  \
    CPP_PP_IGNORE_CXX2A_COMPAT_BEGIN                                                                                   \
    template <__VA_ARGS__ CPP_TEMPLATE_AUX_

#define AND CPP_and


namespace futures::detail {
    /// \cond
    namespace ranges_detail {
        template <typename I, typename S>
        using common_iterator_impl_t =
            enable_if_t<(bool)(input_or_output_iterator<I> &&sentinel_for<S, I>), common_iterator<I, S>>;
    }
    /// \endcond

    /// \addtogroup group-range
    /// @{
    template <typename I, typename S>
    using common_iterator_t = futures::detail::meta::conditional_t<std::is_same<I, S>::value, I, ranges_detail::common_iterator_impl_t<I, S>>;

    /// \cond
    namespace ranges_detail {
        template <typename I, typename S>
        using cpp17_iterator_t = futures::detail::meta::conditional_t<std::is_integral<iter_difference_t<I>>::value,
                                                     common_iterator_t<I, S>, cpp17_iterator<common_iterator_t<I, S>>>;
    }
    /// \endcond

    // Aliases (SFINAE-able)
    template <typename Rng> using range_difference_t = iter_difference_t<iterator_t<Rng>>;

    template <typename Rng> using range_value_t = iter_value_t<iterator_t<Rng>>;

    template <typename Rng> using range_reference_t = iter_reference_t<iterator_t<Rng>>;

    template <typename Rng> using range_rvalue_reference_t = iter_rvalue_reference_t<iterator_t<Rng>>;

    template <typename Rng> using range_common_reference_t = iter_common_reference_t<iterator_t<Rng>>;

    template <typename Rng> using range_size_t = decltype(futures::detail::size(std::declval<Rng &>()));

    /// \cond
    template <typename Rng>
    using range_difference_type_t
        RANGES_DEPRECATED("range_difference_type_t is deprecated. Use the range_difference_t instead.") =
            iter_difference_t<iterator_t<Rng>>;

    template <typename Rng>
    using range_value_type_t RANGES_DEPRECATED("range_value_type_t is deprecated. Use the range_value_t instead.") =
        iter_value_t<iterator_t<Rng>>;

    template <typename Rng>
    using range_category_t RANGES_DEPRECATED("range_category_t is deprecated. Use the range concepts instead.") =
        futures::detail::meta::_t<ranges_detail::iterator_category<iterator_t<Rng>>>;

    template <typename Rng>
    using range_size_type_t RANGES_DEPRECATED("range_size_type_t is deprecated. Use range_size_t instead.") =
        ranges_detail::iter_size_t<iterator_t<Rng>>;
    /// \endcond

    template <typename Rng> using range_common_iterator_t = common_iterator_t<iterator_t<Rng>, sentinel_t<Rng>>;

    /// \cond
    namespace ranges_detail {
        template <typename Rng> using range_cpp17_iterator_t = cpp17_iterator_t<iterator_t<Rng>, sentinel_t<Rng>>;

        std::integral_constant<cardinality, finite> test_cardinality(void *);
        template <cardinality Card> std::integral_constant<cardinality, Card> test_cardinality(basic_view<Card> *);
        template <typename T, std::size_t N>
        std::integral_constant<cardinality, static_cast<cardinality>(N)> test_cardinality(T (*)[N]);
        template <typename T, std::size_t N>
        std::integral_constant<cardinality, static_cast<cardinality>(N)> test_cardinality(std::array<T, N> *);
    } // namespace ranges_detail
    /// \endcond

    // User customization point for specifying the cardinality of ranges:
    template <typename Rng, typename Void /*= void*/>
    struct range_cardinality
        : futures::detail::meta::conditional_t<RANGES_IS_SAME(Rng, uncvref_t<Rng>),
                              decltype(ranges_detail::test_cardinality(static_cast<uncvref_t<Rng> *>(nullptr))),
                              range_cardinality<uncvref_t<Rng>>> {};

    /// @}
    namespace cpp20 {
        using futures::detail::range_difference_t;
        using futures::detail::range_reference_t;
        using futures::detail::range_rvalue_reference_t;
        using futures::detail::range_value_t;
    } // namespace cpp20
} // namespace futures::detail

// #include <futures/algorithm/detail/traits/range/detail/epilogue.h>
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

#ifndef RANGES_PROLOGUE_INCLUDED
#error "Including epilogue, but prologue not included!"
#endif
#undef RANGES_PROLOGUE_INCLUDED

#undef template
#undef AND

RANGES_DIAGNOSTIC_POP


#endif


// #include <futures/algorithm/detail/traits/range/detail/prologue.h>
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

#ifndef FUTURES_RANGES_DETAIL_PROLOGUE_HPP
#define FUTURES_RANGES_DETAIL_PROLOGUE_HPP
// #include <futures/algorithm/detail/traits/range/detail/config.h>

#endif

#ifdef RANGES_PROLOGUE_INCLUDED
#error "Prologue already included!"
#endif
#define RANGES_PROLOGUE_INCLUDED

RANGES_DIAGNOSTIC_PUSH

#ifdef RANGES_FEWER_WARNINGS
RANGES_DIAGNOSTIC_IGNORE_UNDEFINED_INTERNAL
RANGES_DIAGNOSTIC_IGNORE_INDENTATION
RANGES_DIAGNOSTIC_IGNORE_CXX17_COMPAT
#endif

RANGES_DIAGNOSTIC_KEYWORD_MACRO

#define template(...)                                                                                                  \
    CPP_PP_IGNORE_CXX2A_COMPAT_BEGIN                                                                                   \
    template <__VA_ARGS__ CPP_TEMPLATE_AUX_

#define AND CPP_and


namespace futures::detail {
    /// \addtogroup group-range
    /// @{

    ///
    /// Range concepts below
    ///

    // clang-format off
    template<typename T>
    CPP_requires(_range_,
        requires(T & t) //
        (
            futures::detail::begin(t), // not necessarily equality-preserving
            futures::detail::end(t)
        ));
    template<typename T>
    CPP_concept range =
        CPP_requires_ref(futures::detail::_range_, T);

    template<typename T>
    CPP_concept borrowed_range =
        range<T> && ranges_detail::_borrowed_range<T>;

    template <typename R>
    RANGES_DEPRECATED("Please use futures::detail::borrowed_range instead.")
    RANGES_INLINE_VAR constexpr bool safe_range = borrowed_range<R>;

    template(typename T, typename V)(
    concept (output_range_)(T, V),
        output_iterator<iterator_t<T>, V>
    );
    template<typename T, typename V>
    CPP_concept output_range =
        range<T> && CPP_concept_ref(futures::detail::output_range_, T, V);

    template(typename T)(
    concept (input_range_)(T),
        input_iterator<iterator_t<T>>
    );
    template<typename T>
    CPP_concept input_range =
        range<T> && CPP_concept_ref(futures::detail::input_range_, T);

    template(typename T)(
    concept (forward_range_)(T),
        forward_iterator<iterator_t<T>>
    );
    template<typename T>
    CPP_concept forward_range =
        input_range<T> && CPP_concept_ref(futures::detail::forward_range_, T);

    template(typename T)(
    concept (bidirectional_range_)(T),
        bidirectional_iterator<iterator_t<T>>
    );
    template<typename T>
    CPP_concept bidirectional_range =
        forward_range<T> && CPP_concept_ref(futures::detail::bidirectional_range_, T);

    template(typename T)(
    concept (random_access_range_)(T),
        random_access_iterator<iterator_t<T>>
    );

    template<typename T>
    CPP_concept random_access_range =
        bidirectional_range<T> && CPP_concept_ref(futures::detail::random_access_range_, T);
    // clang-format on

    /// \cond
    namespace ranges_detail {
        template <typename Rng> using data_t = decltype(futures::detail::data(std::declval<Rng &>()));

        template <typename Rng> using element_t = futures::detail::meta::_t<std::remove_pointer<data_t<Rng>>>;
    } // namespace ranges_detail
      /// \endcond

    // clang-format off
    template(typename T)(
    concept (contiguous_range_)(T),
        contiguous_iterator<iterator_t<T>> AND
        same_as<ranges_detail::data_t<T>, std::add_pointer_t<iter_reference_t<iterator_t<T>>>>
    );

    template<typename T>
    CPP_concept contiguous_range =
        random_access_range<T> && CPP_concept_ref(futures::detail::contiguous_range_, T);

    template(typename T)(
    concept (common_range_)(T),
        same_as<iterator_t<T>, sentinel_t<T>>
    );

    template<typename T>
    CPP_concept common_range =
        range<T> && CPP_concept_ref(futures::detail::common_range_, T);

    /// \cond
    template<typename T>
    CPP_concept bounded_range =
        common_range<T>;
    /// \endcond

    template<typename T>
    CPP_requires(sized_range_,
        requires(T & t) //
        (
            futures::detail::size(t)
        ));
    template(typename T)(
    concept (sized_range_)(T),
        ranges_detail::integer_like_<range_size_t<T>>);

    template<typename T>
    CPP_concept sized_range =
        range<T> &&
        !disable_sized_range<uncvref_t<T>> &&
        CPP_requires_ref(futures::detail::sized_range_, T) &&
        CPP_concept_ref(futures::detail::sized_range_, T);
    // clang-format on

    /// \cond
    namespace ext {
        template <typename T> struct enable_view : std::is_base_of<view_base, T> {};
    } // namespace ext
    /// \endcond

    // Specialize this if the default is wrong.
    template <typename T> RANGES_INLINE_VAR constexpr bool enable_view = ext::enable_view<T>::value;

#if defined(__cpp_lib_string_view) && __cpp_lib_string_view > 0
    template <typename Char, typename Traits>
    RANGES_INLINE_VAR constexpr bool enable_view<std::basic_string_view<Char, Traits>> = true;
#endif

#if defined(__cpp_lib_span) && __cpp_lib_span > 0
    template <typename T, std::size_t N> RANGES_INLINE_VAR constexpr bool enable_view<std::span<T, N>> = N + 1 < 2;
#endif

    ///
    /// View concepts below
    ///

    // clang-format off
    template<typename T>
    CPP_concept view_ =
        range<T> &&
        semiregular<T> &&
        enable_view<T>;

    template<typename T>
    CPP_concept viewable_range =
        range<T> &&
        (borrowed_range<T> || view_<uncvref_t<T>>);
    // clang-format on

    //////////////////////////////////////////////////////////////////////////////////////
    // range_tag
    struct range_tag {};

    struct input_range_tag : range_tag {};
    struct forward_range_tag : input_range_tag {};
    struct bidirectional_range_tag : forward_range_tag {};
    struct random_access_range_tag : bidirectional_range_tag {};
    struct contiguous_range_tag : random_access_range_tag {};

    template <typename Rng>
    using range_tag_of =                          //
        std::enable_if_t<                         //
            range<Rng>,                           //
            futures::detail::meta::conditional_t<                  //
                contiguous_range<Rng>,            //
                contiguous_range_tag,             //
                futures::detail::meta::conditional_t<              //
                    random_access_range<Rng>,     //
                    random_access_range_tag,      //
                    futures::detail::meta::conditional_t<          //
                        bidirectional_range<Rng>, //
                        bidirectional_range_tag,  //
                        futures::detail::meta::conditional_t<      //
                            forward_range<Rng>,   //
                            forward_range_tag,    //
                            futures::detail::meta::conditional_t<  //
                                input_range<Rng>, //
                                input_range_tag,  //
                                range_tag>>>>>>;

    //////////////////////////////////////////////////////////////////////////////////////
    // common_range_tag_of
    struct common_range_tag : range_tag {};

    template <typename Rng>
    using common_range_tag_of = //
        std::enable_if_t<       //
            range<Rng>,         //
            futures::detail::meta::conditional_t<common_range<Rng>, common_range_tag, range_tag>>;

    //////////////////////////////////////////////////////////////////////////////////////
    // sized_range_concept
    struct sized_range_tag : range_tag {};

    template <typename Rng>
    using sized_range_tag_of = //
        std::enable_if_t<      //
            range<Rng>,        //
            futures::detail::meta::conditional_t<sized_range<Rng>, sized_range_tag, range_tag>>;

    /// \cond
    namespace view_detail_ {
        // clang-format off
        template<typename T>
        CPP_concept view =
            futures::detail::view_<T>;
        // clang-format on
    } // namespace view_detail_
    /// \endcond

    namespace cpp20 {
        using futures::detail::bidirectional_range;
        using futures::detail::borrowed_range;
        using futures::detail::common_range;
        using futures::detail::contiguous_range;
        using futures::detail::enable_view;
        using futures::detail::forward_range;
        using futures::detail::input_range;
        using futures::detail::output_range;
        using futures::detail::random_access_range;
        using futures::detail::range;
        using futures::detail::sized_range;
        using futures::detail::view_base;
        using futures::detail::viewable_range;
        using futures::detail::view_detail_::view;
    } // namespace cpp20
    /// @}
} // namespace futures::detail

// #include <futures/algorithm/detail/traits/range/detail/epilogue.h>
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

#ifndef RANGES_PROLOGUE_INCLUDED
#error "Including epilogue, but prologue not included!"
#endif
#undef RANGES_PROLOGUE_INCLUDED

#undef template
#undef AND

RANGES_DIAGNOSTIC_POP


#endif

// #include <futures/futures/traits/is_future.h>


// #include <type_traits>


namespace futures {
    /** \addtogroup futures Futures
     *  @{
     */
    /** \addtogroup waiting Waiting
     *  @{
     */

    /// \brief Wait for a sequence of futures to be ready
    ///
    /// This function waits for all futures in the range [`first`, `last`) to be ready.
    /// It simply waits iteratively for each of the futures to be ready.
    ///
    /// \note This function is adapted from boost::wait_for_all
    ///
    /// \see
    /// [boost.thread wait_for_all](https://www.boost.org/doc/libs/1_78_0/doc/html/thread/synchronization.html#thread.synchronization.futures.reference.wait_for_all)
    ///
    /// \tparam Iterator Iterator type in a range of futures
    /// \param first Iterator to the first element in the range
    /// \param last Iterator to one past the last element in the range
    template <typename Iterator
#ifndef FUTURES_DOXYGEN
              ,
              typename std::enable_if_t<is_future_v<detail::iter_value_t<Iterator>>, int> = 0
#endif
              >
    void wait_for_all(Iterator first, Iterator last) {
        for (Iterator it = first; it != last; ++it) {
            it->wait();
        }
    }

    /// \brief Wait for a sequence of futures to be ready
    ///
    /// This function waits for all futures in the range `r` to be ready.
    /// It simply waits iteratively for each of the futures to be ready.
    ///
    /// \note This function is adapted from boost::wait_for_all
    ///
    /// \see
    /// [boost.thread wait_for_all](https://www.boost.org/doc/libs/1_78_0/doc/html/thread/synchronization.html#thread.synchronization.futures.reference.wait_for_all)
    ///
    /// \tparam Range A range of futures type
    /// \param r Range of futures
    template <typename Range
#ifndef FUTURES_DOXYGEN
              ,
              typename std::enable_if_t<detail::range<Range> && is_future_v<detail::range_value_t<Range>>, int> = 0
#endif
              >
    void wait_for_all(Range &&r) {
        wait_for_all(std::begin(r), std::end(r));
    }

    /// \brief Wait for a sequence of futures to be ready
    ///
    /// This function waits for all specified futures `fs`... to be ready.
    ///
    /// It creates a compile-time fixed-size data structure to store references to all of the futures and then
    /// waits for each of the futures to be ready.
    ///
    /// \note This function is adapted from boost::wait_for_all
    ///
    /// \see
    /// [boost.thread wait_for_all](https://www.boost.org/doc/libs/1_78_0/doc/html/thread/synchronization.html#thread.synchronization.futures.reference.wait_for_all)
    ///
    /// \tparam Fs A list of future types
    /// \param fs A list of future objects
    template <typename... Fs
#ifndef FUTURES_DOXYGEN
              ,
              typename std::enable_if_t<std::conjunction_v<is_future<std::decay_t<Fs>>...>, int> = 0
#endif
              >
    void wait_for_all(Fs &&...fs) {
        (fs.wait(), ...);
    }

    /** @} */
    /** @} */
} // namespace futures

#endif // FUTURES_WAIT_FOR_ALL_H

// #include <futures/futures/wait_for_any.h>
//
// Copyright (c) alandefreitas 12/3/21.
// See accompanying file LICENSE
//

#ifndef FUTURES_WAIT_FOR_ANY_H
#define FUTURES_WAIT_FOR_ANY_H

// #include <futures/algorithm/detail/traits/range/range/concepts.h>

// #include <futures/futures/detail/waiter_for_any.h>
//
// Copyright (c) alandefreitas 12/15/21.
// See accompanying file LICENSE
//

#ifndef FUTURES_WAITER_FOR_ANY_H
#define FUTURES_WAITER_FOR_ANY_H

// #include <futures/futures/detail/lock.h>
//
// Copyright (c) alandefreitas 12/15/21.
// See accompanying file LICENSE
//

#ifndef FUTURES_LOCK_H
#define FUTURES_LOCK_H

// #include <futures/algorithm/detail/traits/range/iterator/concepts.h>


namespace futures::detail {
    /** \addtogroup futures Futures
     *  @{
     */

    /// \brief Try to lock range of mutexes in a way that all of them should work
    ///
    /// Calls try_lock() on each of the Lockable objects in the supplied range. If any of the calls to try_lock()
    /// returns false then all locks acquired are released and an iterator referencing the failed lock is returned.
    ///
    /// If any of the try_lock() operations on the supplied Lockable objects throws an exception any locks acquired by
    /// the function will be released before the function exits.
    ///
    /// \throws exception Any exceptions thrown by calling try_lock() on the supplied Lockable objects
    ///
    /// \post All the supplied Lockable objects are locked by the calling thread.
    ///
    /// \see
    /// https://www.boost.org/doc/libs/1_78_0/doc/html/thread/synchronization.html#thread.synchronization.lock_functions.try_lock_range
    ///
    /// \tparam Iterator Range iterator type
    /// \param first Iterator to first mutex in the range
    /// \param last Iterator to one past the last mutex in the range
    /// \return Iterator to first element that could *not* be locked, or `end` if all the supplied Lockable objects are
    /// now locked
    template <typename Iterator, std::enable_if_t<detail::input_iterator<Iterator>, int> = 0>
    Iterator try_lock(Iterator first, Iterator last) {
        using lock_type = typename std::iterator_traits<Iterator>::value_type;

        // Handle trivial cases
        if (const bool empty_range = first == last; empty_range) {
            return last;
        } else if (const bool single_element = std::next(first) == last; single_element) {
            if (first->try_lock()) {
                return last;
            } else {
                return first;
            }
        }

        // General cases: Try to lock first and already return if fails
        std::unique_lock<lock_type> guard_first(*first, std::try_to_lock);
        if (const bool locking_failed = !guard_first.owns_lock(); locking_failed) {
            return first;
        }

        // While first is locked by guard_first, try to lock the other elements in the range
        const Iterator failed_mutex_it = try_lock(std::next(first), last);
        if (const bool none_failed = failed_mutex_it == last; none_failed) {
            // Break the association of the associated mutex (i.e. don't unlock at destruction)
            guard_first.release();
        }
        return failed_mutex_it;
    }

    /// \brief Lock range of mutexes in a way that avoids deadlock
    ///
    /// Locks the Lockable objects in the range [`first`, `last`) supplied as arguments in an unspecified and
    /// indeterminate order in a way that avoids deadlock. It is safe to call this function concurrently from multiple
    /// threads for any set of mutexes (or other lockable objects) in any order without risk of deadlock. If any of the
    /// lock() or try_lock() operations on the supplied Lockable objects throws an exception any locks acquired by the
    /// function will be released before the function exits.
    ///
    /// \throws exception Any exceptions thrown by calling lock() or try_lock() on the supplied Lockable objects
    ///
    /// \post All the supplied Lockable objects are locked by the calling thread.
    ///
    /// \see
    /// https://www.boost.org/doc/libs/1_78_0/doc/html/thread/synchronization.html#thread.synchronization.lock_functions
    ///
    /// \tparam Iterator Range iterator type
    /// \param first Iterator to first mutex in the range
    /// \param last Iterator to one past the last mutex in the range
    template <typename Iterator, std::enable_if_t<detail::input_iterator<Iterator>, int> = 0>
    void lock(Iterator first, Iterator last) {
        using lock_type = typename std::iterator_traits<Iterator>::value_type;

        /// \brief Auxiliary lock guard for a range of mutexes recursively using this lock function
        struct range_lock_guard {
            /// Iterator to first locked mutex in the range
            Iterator begin;

            /// Iterator to one past last locked mutex in the range
            Iterator end;

            /// \brief Construct a lock guard for a range of mutexes
            range_lock_guard(Iterator first, Iterator last) : begin(first), end(last) {
                // The range lock guard recursively calls the same lock function we use here
                futures::detail::lock(begin, end);
            }

            range_lock_guard(const range_lock_guard &) = delete;
            range_lock_guard &operator=(const range_lock_guard &) = delete;

            range_lock_guard(range_lock_guard &&other) noexcept
                : begin(std::exchange(other.begin, Iterator{})), end(std::exchange(other.end, Iterator{})){}

            range_lock_guard &operator=(range_lock_guard && other) noexcept {
                if (this == &other) {
                    return *this;
                }
                begin = std::exchange(other.begin, Iterator{});
                end = std::exchange(other.end, Iterator{});
                return *this;
            };

            /// \brief Unlock each mutex in the range
            ~range_lock_guard() {
                while (begin != end) {
                    begin->unlock();
                    ++begin;
                }
            }

            /// \brief Make the range empty so nothing is unlocked at destruction
            void release() { begin = end; }
        };

        // Handle trivial cases
        if (const bool empty_range = first == last; empty_range) {
            return;
        } else if (const bool single_element = std::next(first) == last; single_element) {
            first->lock();
            return;
        }

        // Handle general case
        // One of the locking strategies we are trying
        bool currently_using_first_strategy = true;

        // Define the two ranges first < second <= next < last
        Iterator second = std::next(first);
        Iterator next = second;

        // Alternate between two locking strategies
        for (;;) {
            // A deferred lock assumes the algorithm might lock the first lock later
            std::unique_lock<lock_type> first_lock(*first, std::defer_lock);
            if (currently_using_first_strategy) {
                // First strategy: Lock first, then _try_ to lock the others
                first_lock.lock();
                const Iterator failed_lock_it = try_lock(next, last);
                if (const bool no_lock_failed = failed_lock_it == last; no_lock_failed) {
                    // !SUCCESS!
                    // Breaks the association of the associated mutex (i.e. don't unlock first_lock)
                    first_lock.release();
                    return;
                } else {
                    // !FAIL!
                    // Try another strategy in the next iteration
                    currently_using_first_strategy = false;
                    next = failed_lock_it;
                }
            } else {
                // Second strategy: Lock others, then try to lock first
                // Create range lock guard for the range [next, last)
                range_lock_guard range_guard(next, last);
                // If we can lock first
                if (first_lock.try_lock()) {
                    // Try to lock [second, next)
                    const Iterator failed_lock = try_lock(second, next);
                    if (const bool all_locked = failed_lock == next; all_locked) {
                        // !SUCCESS!
                        // Don't let it unlock
                        first_lock.release();
                        range_guard.release();
                        return;
                    } else {
                        // Try this strategy again with a new "next"
                        currently_using_first_strategy = false;
                        next = failed_lock;
                    }
                } else {
                    // We couldn't lock the first mutex, move to first strategy
                    currently_using_first_strategy = true;
                    next = second;
                }
            }
        }
    }

    /** @} */ // \addtogroup futures Futures
} // namespace futures::detail

#endif // FUTURES_LOCK_H

// #include <futures/futures/detail/shared_state.h>


namespace futures::detail {
    /** \addtogroup futures Futures
     *  @{
     */

    /// \brief Helper class to set signals and wait for any future in a sequence of futures to become ready
    class waiter_for_any {
      public:
        /// \brief Construct a waiter_for_any watching zero futures
        waiter_for_any() = default;

        /// \brief Construct a waiter_for_any that waits for one of the futures in a range of futures
        template <typename Iterator> waiter_for_any(Iterator first, Iterator last) {
            for (Iterator current = first; current != last; ++current) {
                add(*current);
            }
        }

        waiter_for_any(const waiter_for_any&) = delete;
        waiter_for_any(waiter_for_any&&) = delete;
        waiter_for_any& operator=(const waiter_for_any&) = delete;
        waiter_for_any& operator=(waiter_for_any&&) = delete;

        /// \brief Destruct the waiter
        ///
        /// If the waiter is destroyed before we wait for a result, we disable the future notifications
        ///
        ~waiter_for_any() {
            for (auto const & waiter : waiters_) {
                waiter.disable_notification();
            }
        }

        /// \brief Watch the specified future
        template <typename F> void add(F &f) {
            if constexpr (has_ready_notifier_v<std::decay_t<F>>) {
                if (f.valid()) {
                    registered_waiter waiter(f, f.notify_when_ready(cv), future_count);
                    try {
                        waiters_.push_back(waiter);
                    } catch (...) {
                        f.unnotify_when_ready(waiter.handle);
                        throw;
                    }
                    ++future_count;
                }
            } else {
                // The future has no ready-notifier, so we create a future to poll until it can notify us
                // This is the future we wait for instead
                poller_futures_.emplace_back(futures::async([f = &f]() { f->wait(); }));
                add(poller_futures_.back());
            }
        }

        /// \brief Watch the specified futures in the parameter pack
        template <typename F1, typename... Fs> void add(F1 &&f1, Fs &&...fs) {
            add(std::forward<F1>(f1));
            add(std::forward<Fs>(fs)...);
        }

        /// \brief Wait for one of the futures to notify it got ready
        std::size_t wait() {
            // Lock the mutex associated with all futures in the range
            registered_waiter_range_lock lk(waiters_.begin(), waiters_.end());
            while (true) {
                // Check if any of the futures is ready
                for (std::size_t i = 0; i < waiters_.size(); ++i) {
                    if (waiters_[i].is_ready((*lk.locks)[i])) {
                        return waiters_[i].index;
                    }
                }
                // Else, wait for a signal from one of the futures
                cv.wait(lk);
                // While this happen, shared_task::run -> shared_state::set_value -> shared_state::create_unique_lock is happening
                // how can we lock these mutexes and still allow the future to set value?
                // boost::thread obviously does something different here to avoid this deadlock
            }
        }

      private:
        /// \brief Type of handle in the future object used to notify completion
        using notify_when_ready_handle = typename detail::shared_state_base::notify_when_ready_handle;

        /// \brief Helper class to store information about each of the futures we are waiting for
        ///
        /// Because the waiter can be associated with futures of different types, this class also
        /// nullifies the operations necessary to check the state of the future object.
        ///
        struct registered_waiter {
            /// \brief Mutex associated with a future we are watching
            std::mutex *future_mutex_;

            /// \brief Callback to disable notifications
            std::function<void(notify_when_ready_handle)> disable_notification_callback;

            /// \brief Callback to disable notifications
            std::function<bool(std::unique_lock<std::mutex> &)> is_ready_callback;

            /// \brief Handler to the resource that will notify us when the future is ready
            ///
            /// In the shared state, this usually represents a pointer to the condition variable
            ///
            notify_when_ready_handle handle;

            /// \brief Index to this future in the underlying range
            std::size_t index;

            /// \brief Construct a registered waiter to be enqueued in the main waiter
            template <class Future>
            registered_waiter(Future &a_future, const notify_when_ready_handle &handle_, std::size_t index_)
                : future_mutex_(&a_future.mutex()),
                  disable_notification_callback(
                      [future_ = &a_future](notify_when_ready_handle h) -> void { future_->unnotify_when_ready(h); }),
                  is_ready_callback([future_ = &a_future](std::unique_lock<std::mutex> &lk) -> bool {
                      return future_->is_ready(lk);
                  }),
                  handle(handle_), index(index_) {}

            /// \brief Get the mutex associated with the future we are watching
            [[nodiscard]] std::mutex &mutex() const { return *future_mutex_; }

            /// \brief Disable notification when the future is ready
            void disable_notification() const { disable_notification_callback(handle); }

            /// \brief Check if underlying future is ready
            bool is_ready(std::unique_lock<std::mutex> &lk) const { return is_ready_callback(lk); }
        };

        /// \brief Helper class to lock all futures
        struct registered_waiter_range_lock {
            /// \brief Type for a vector of locks
            using lock_vector = std::vector<std::unique_lock<std::mutex>>;

            /// \brief Type for a shared vector of locks
            using shared_lock_vector = std::shared_ptr<lock_vector>;

            /// \brief Number of futures locked
            std::size_t count;

            /// \brief Locks for each future in the range
            shared_lock_vector locks;

            /// \brief Create a lock for each future in the specified vector of registered waiters
            template <typename WaiterIterator>
            explicit registered_waiter_range_lock(WaiterIterator first_waiter, WaiterIterator last_waiter)
                : count(std::distance(first_waiter, last_waiter)), locks(std::make_shared<lock_vector>(count)) {
                WaiterIterator waiter_it = first_waiter;
                std::size_t lock_idx = 0;
                while (waiter_it != last_waiter) {
                    (*locks)[lock_idx] = (std::unique_lock<std::mutex>(waiter_it->mutex()));
                    ++waiter_it;
                    ++lock_idx;
                }
            }

            /// \brief Lock all future mutexes in the range
            void lock() const { futures::detail::lock(locks->begin(), locks->end()); }

            /// \brief Unlock all future mutexes in the range
            void unlock() const {
                for (size_t i = 0; i < count; ++i) {
                    (*locks)[i].unlock();
                }
            }
        };

        /// \brief Condition variable to warn about any ready future
        std::condition_variable_any cv;

        /// \brief Waiters with information about each future and notification handlers
        std::vector<registered_waiter> waiters_;

        /// \brief Number of futures in this range
        std::size_t future_count{0};

        /// \brief Poller futures
        ///
        /// Futures that support notifications wrapping future types that don't
        ///
        small_vector<cfuture<void>> poller_futures_{};
    };

    /** @} */
} // namespace futures::detail

#endif // FUTURES_WAITER_FOR_ANY_H

// #include <futures/futures/traits/is_future.h>


// #include <type_traits>


namespace futures {
    /** \addtogroup futures Futures
     *  @{
     */
    /** \addtogroup waiting Waiting
     *  @{
     */

    /// \brief Wait for any future in a sequence to be ready
    ///
    /// This function waits for any future in the range [`first`, `last`) to be ready.
    ///
    /// Unlike @ref wait_for_all, this function requires special data structures to allow that
    /// to happen without blocking.
    ///
    /// \note This function is adapted from `boost::wait_for_any`
    ///
    /// \see
    /// [boost.thread wait_for_any](https://www.boost.org/doc/libs/1_78_0/doc/html/thread/synchronization.html#thread.synchronization.futures.reference.wait_for_any)
    ///
    /// \tparam Iterator Iterator type in a range of futures
    /// \param first Iterator to the first element in the range
    /// \param last Iterator to one past the last element in the range
    /// \return Iterator to the first future that got ready
    template <typename Iterator
#ifndef FUTURES_DOXYGEN
              ,
              typename std::enable_if_t<is_future_v<detail::iter_value_t<Iterator>>, int> = 0
#endif
              >
    Iterator wait_for_any(Iterator first, Iterator last) {
        if (const bool is_empty = first == last; is_empty) {
            return last;
        } else if (const bool is_single = std::next(first) == last; is_single) {
            first->wait();
            return first;
        } else {
            detail::waiter_for_any waiter(first, last);
            auto ready_future_index = waiter.wait();
            return std::next(first, ready_future_index);
        }
    }

    /// \brief Wait for any future in a sequence to be ready
    ///
    /// This function waits for any future in the range `r` to be ready.
    /// This function requires special data structures to allow that to happen without blocking.
    ///
    /// \note This function is adapted from `boost::wait_for_any`
    ///
    /// \see
    /// [boost.thread wait_for_any](https://www.boost.org/doc/libs/1_78_0/doc/html/thread/synchronization.html#thread.synchronization.futures.reference.wait_for_any)
    ///
    /// \tparam Iterator A range of futures type
    /// \param r Range of futures
    /// \return Iterator to the first future that got ready
    template <typename Range
#ifndef FUTURES_DOXYGEN
              ,
              typename std::enable_if_t<detail::range<Range> && is_future_v<detail::range_value_t<Range>>, int> = 0
#endif
              >
    detail::iterator_t<Range> wait_for_any(Range &&r) {
        return wait_for_any(std::begin(r), std::end(r));
    }

    /// \brief Wait for any future in a sequence to be ready
    ///
    /// This function waits for all specified futures `fs`... to be ready.
    ///
    /// \note This function is adapted from `boost::wait_for_any`
    ///
    /// \see
    /// [boost.thread wait_for_any](https://www.boost.org/doc/libs/1_78_0/doc/html/thread/synchronization.html#thread.synchronization.futures.reference.wait_for_any)
    ///
    /// \tparam Fs A list of future types
    /// \param fs A list of future objects
    /// \return Index of the first future that got ready
    template <typename... Fs
#ifndef FUTURES_DOXYGEN
              ,
              typename std::enable_if_t<std::conjunction_v<is_future<std::decay_t<Fs>>...>, int> = 0
#endif
              >
    std::size_t wait_for_any(Fs &&...fs) {
        constexpr std::size_t size = sizeof...(Fs);
        if constexpr (const bool is_empty = size == 0; is_empty) {
            return 0;
        } else if constexpr (const bool is_single = size == 1; is_single) {
            wait_for_all(std::forward<Fs>(fs)...);
            return 0;
        } else {
            detail::waiter_for_any waiter;
            waiter.add(std::forward<Fs>(fs)...);
            return waiter.wait();
        }
    }

    /** @} */
    /** @} */
} // namespace futures

#endif // FUTURES_WAIT_FOR_ANY_H

// #include <futures/futures/await.h>


// Adaptors
// #include <futures/adaptor/ready_future.h>
//
// Created by Alan Freitas on 8/18/21.
//

#ifndef FUTURES_READY_FUTURE_H
#define FUTURES_READY_FUTURE_H

// #include <future>


// #include <futures/futures/detail/traits/has_is_ready.h>
//
// Created by alandefreitas on 10/15/21.
//

#ifndef FUTURES_HAS_IS_READY_H
#define FUTURES_HAS_IS_READY_H

// #include <type_traits>


namespace futures::detail {
    /** \addtogroup futures Futures
     *  @{
     */
    /** \addtogroup future-traits Future Traits
     *  @{
     */

    /// Check if a type implements the is_ready function and it returns bool
    /// This is what we use to identify the return type of a future type candidate
    /// However, this doesn't mean the type is a future in the terms of the is_future concept
    template <typename T, typename = void> struct has_is_ready : std::false_type {};

    template <typename T>
    struct has_is_ready<T, std::void_t<decltype(std::declval<T>().is_ready())>>
        : std::is_same<bool, decltype(std::declval<T>().is_ready())> {};

    template <typename T> constexpr bool has_is_ready_v = has_is_ready<T>::value;

    /** @} */
    /** @} */
} // namespace futures::detail

#endif // FUTURES_HAS_IS_READY_H


// #include <futures/futures/traits/future_return.h>
//
// Created by Alan Freitas on 8/16/21.
//

#ifndef FUTURES_FUTURE_RETURN_H
#define FUTURES_FUTURE_RETURN_H

// #include <futures/adaptor/detail/traits/is_reference_wrapper.h>
//
// Created by Alan Freitas on 8/16/21.
//

#ifndef FUTURES_IS_REFERENCE_WRAPPER_H
#define FUTURES_IS_REFERENCE_WRAPPER_H

// #include <type_traits>


namespace futures {
    /** \addtogroup futures Futures
     *  @{
     */
    /** \addtogroup future-traits Future Traits
     *  @{
     */

    /// Check if type is a reference_wrapper
    template <typename> struct is_reference_wrapper : std::false_type {};

    template <class T> struct is_reference_wrapper<std::reference_wrapper<T>> : std::true_type {};

    template <class T> constexpr bool is_reference_wrapper_v = is_reference_wrapper<T>::value;
    /** @} */  // \addtogroup future-traits Future Traits
    /** @} */  // \addtogroup futures Futures
} // namespace futures

#endif // FUTURES_IS_REFERENCE_WRAPPER_H

// #include <type_traits>


namespace futures {
    /** \addtogroup futures Futures
     *  @{
     */
    /** \addtogroup future-traits Future Traits
     *  @{
     */

    /// Determine the type to be stored and returned by a future object
    template <class T>
    using future_return = std::conditional<is_reference_wrapper_v<std::decay_t<T>>, T &, std::decay_t<T>>;

    /// Determine the type to be stored and returned by a future object
    template <class T> using future_return_t = typename future_return<T>::type;

    /** @} */  // \addtogroup future-traits Future Traits
    /** @} */  // \addtogroup futures Futures
} // namespace futures

#endif // FUTURES_FUTURE_RETURN_H

// #include <futures/futures/traits/is_future.h>


// #include <futures/futures/basic_future.h>

// #include <futures/futures/promise.h>


namespace futures {
    /** \addtogroup adaptors Adaptors
     *  @{
     */

    /// \brief Check if a future is ready
    ///
    /// Although basic_future has its more efficient is_ready function, this free function
    /// allows us to query other futures that don't implement is_ready, such as std::future.
    template <typename Future
#ifndef FUTURES_DOXYGEN
              ,
              std::enable_if_t<is_future_v<std::decay_t<Future>>, int> = 0
#endif
              >
    bool is_ready(Future &&f) {
        assert(f.valid() && "Undefined behaviour. Checking if an invalid future is ready.");
        if constexpr (detail::has_is_ready_v<Future>) {
            return f.is_ready();
        } else {
            return f.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
        }
    }

    /// \brief Make a placeholder future object that is ready
    ///
    /// \see https://en.cppreference.com/w/cpp/experimental/make_ready_future
    ///
    /// \return A future associated with the shared state that is created.
    template <typename T, typename Future = future<typename std::decay_t<T>>> Future make_ready_future(T &&value) {
        using decay_type = typename std::decay_t<T>;
        promise<decay_type> p;
        Future result = p.template get_future<Future>();
        p.set_value(value);
        return result;
    }

    /// \brief Make a placeholder future object that is ready from a reference
    ///
    /// \see https://en.cppreference.com/w/cpp/experimental/make_ready_future
    ///
    /// \return A future associated with the shared state that is created.
    template <typename T, typename Future = future<T &>> Future make_ready_future(std::reference_wrapper<T> value) {
        promise<T &> p;
        Future result = p.template get_future<Future>();
        p.set_value(value);
        return result;
    }

    /// \brief Make a placeholder void future object that is ready
    ///
    /// \see https://en.cppreference.com/w/cpp/experimental/make_ready_future
    ///
    /// \return A future associated with the shared state that is created.
    template <typename Future = future<void>> Future make_ready_future() {
        promise<void> p;
        auto result = p.get_future<Future>();
        p.set_value();
        return result;
    }

    /// \brief Make a placeholder @ref cfuture object that is ready
    ///
    /// \see https://en.cppreference.com/w/cpp/experimental/make_ready_future
    ///
    /// \return A cfuture associated with the shared state that is created.
    template <typename T> cfuture<typename std::decay<T>> make_ready_cfuture(T &&value) {
        return make_ready_future<T, cfuture<typename std::decay<T>>>(std::forward<T>(value));
    }

    /// \brief Make a placeholder @ref cfuture object that is ready
    template <typename T> cfuture<T &> make_ready_cfuture(std::reference_wrapper<T> value) {
        return make_ready_future<T, cfuture<T &>>(value);
    }

    /// \brief Make a placeholder void @ref cfuture object that is ready
    ///
    /// \see https://en.cppreference.com/w/cpp/experimental/make_ready_future
    ///
    /// \return A cfuture associated with the shared state that is created.
    inline cfuture<void> make_ready_cfuture() { return make_ready_future<cfuture<void>>(); }

    /// \brief Make a placeholder @ref jcfuture object that is ready
    ///
    /// \see https://en.cppreference.com/w/cpp/experimental/make_ready_future
    ///
    /// \return A cfuture associated with the shared state that is created.
    template <typename T> jcfuture<typename std::decay<T>> make_ready_jcfuture(T &&value) {
        return make_ready_future<T, jcfuture<typename std::decay<T>>>(std::forward<T>(value));
    }

    /// \brief Make a placeholder @ref cfuture object that is ready
    template <typename T> jcfuture<T &> make_ready_jcfuture(std::reference_wrapper<T> value) {
        return make_ready_future<T, jcfuture<T &>>(value);
    }

    /// \brief Make a placeholder void @ref jcfuture object that is ready
    ///
    /// \see https://en.cppreference.com/w/cpp/experimental/make_ready_future
    ///
    /// \return A cfuture associated with the shared state that is created.
    inline jcfuture<void> make_ready_jcfuture() { return make_ready_future<jcfuture<void>>(); }

    /// \brief Make a placeholder future object that is ready with an exception from an exception ptr
    ///
    /// \see https://en.cppreference.com/w/cpp/experimental/make_exceptional_future
    ///
    /// \return A future associated with the shared state that is created.
    template <typename T, typename Future = future<T>> future<T> make_exceptional_future(std::exception_ptr ex) {
        promise<T> p;
        p.set_exception(ex);
        return p.template get_future<Future>();
    }

    /// \brief Make a placeholder future object that is ready with from any exception
    ///
    /// \see https://en.cppreference.com/w/cpp/experimental/make_exceptional_future
    ///
    /// \return A future associated with the shared state that is created.
    template <class T, typename Future = future<T>, class E> future<T> make_exceptional_future(E ex) {
        promise<T> p;
        p.set_exception(std::make_exception_ptr(ex));
        return p.template get_future<Future>();
    }
    /** @} */
} // namespace futures

#endif // FUTURES_READY_FUTURE_H

// #include <futures/adaptor/then.h>
//
// Created by Alan Freitas on 8/18/21.
//

#ifndef FUTURES_THEN_H
#define FUTURES_THEN_H

// #include <future>


// #include <futures/adaptor/detail/continuation_unwrap.h>
//
// Copyright (c) alandefreitas 12/5/21.
// See accompanying file LICENSE
//

#ifndef FUTURES_CONTINUATION_UNWRAP_H
#define FUTURES_CONTINUATION_UNWRAP_H

// #include <futures/adaptor/detail/move_or_copy.h>
//
// Copyright (c) alandefreitas 12/14/21.
// See accompanying file LICENSE
//

#ifndef FUTURES_MOVE_OR_COPY_H
#define FUTURES_MOVE_OR_COPY_H

// #include <futures/futures/traits/is_future.h>


namespace futures::detail {
    /** \addtogroup futures Futures
     *  @{
     */
    /** \addtogroup future-traits Future Traits
     *  @{
     */

    /// \brief Move or share a future, depending on the type of future input
    ///
    /// Create another future with the state of the before future (usually for a continuation function).
    /// This state should be copied to the new callback function.
    /// Shared futures can be copied. Normal futures should be moved.
    /// \return The moved future or the shared future
    template <class Future> constexpr decltype(auto) move_or_copy(Future &&before) {
        if constexpr (is_shared_future_v<Future>) {
            return std::forward<Future>(before);
        } else {
            return std::move(std::forward<Future>(before));
        }
    }

    /** @} */ // \addtogroup future-traits Future Traits
    /** @} */ // \addtogroup futures Futures
}
#endif // FUTURES_MOVE_OR_COPY_H

// #include <futures/adaptor/detail/traits/is_callable.h>
//
// Created by Alan Freitas on 8/18/21.
//

#ifndef FUTURES_IS_CALLABLE_H
#define FUTURES_IS_CALLABLE_H

namespace futures {
    /** \addtogroup futures Futures
     *  @{
     */
    /** \addtogroup future-traits Future Traits
     *  @{
     */

    /// \brief Check if something is callable, regardless of the arguments
    template <typename T> struct is_callable {
      private:
        typedef char (&yes)[1];
        typedef char (&no)[2];

        struct Fallback {
            void operator()();
        };
        struct Derived : T, Fallback {};

        template <typename U, U> struct Check;

        template <typename> static yes test(...);

        template <typename C> static no test(Check<void (Fallback::*)(), &C::operator()> *);

      public:
        static const bool value = sizeof(test<Derived>(0)) == sizeof(yes);
    };

    template <typename T>
    constexpr bool is_callable_v = is_callable<T>::value;

    /** @} */  // \addtogroup future-traits Future Traits
    /** @} */  // \addtogroup futures Futures
} // namespace futures

#endif // FUTURES_IS_CALLABLE_H

// #include <futures/adaptor/detail/traits/is_single_type_tuple.h>
//
// Copyright (c) alandefreitas 12/4/21.
// See accompanying file LICENSE
//

#ifndef FUTURES_TUPLE_TYPE_IS_SINGLE_TYPE_TUPLE_H
#define FUTURES_TUPLE_TYPE_IS_SINGLE_TYPE_TUPLE_H

// #include <futures/adaptor/detail/traits/is_tuple.h>
//
// Created by Alan Freitas on 8/19/21.
//

#ifndef FUTURES_IS_TUPLE_H
#define FUTURES_IS_TUPLE_H

// #include <type_traits>

// #include <tuple>


namespace futures {
    /** \addtogroup futures Futures
     *  @{
     */
    /** \addtogroup future-traits Future Traits
     *  @{
     */

    /// Check if type is a tuple
    template <typename> struct is_tuple : std::false_type {};

    template <typename... Args> struct is_tuple<std::tuple<Args...>> : std::true_type {};
    template <typename... Args> struct is_tuple<const std::tuple<Args...>> : std::true_type {};
    template <typename... Args> struct is_tuple<std::tuple<Args...> &> : std::true_type {};
    template <typename... Args> struct is_tuple<std::tuple<Args...> &&> : std::true_type {};
    template <typename... Args> struct is_tuple<const std::tuple<Args...> &> : std::true_type {};

    template <class T> constexpr bool is_tuple_v = is_tuple<T>::value;
    /** @} */  // \addtogroup future-traits Future Traits
    /** @} */  // \addtogroup futures Futures
} // namespace futures

#endif // FUTURES_IS_TUPLE_H

// #include <tuple>

// #include <type_traits>


namespace futures::detail {
    /// \brief Check if all types in a tuple match a predicate
    template <class L> struct is_single_type_tuple : is_tuple<L> {};

    template <class T1> struct is_single_type_tuple<std::tuple<T1>> : std::true_type {};

    template <class T1, class T2> struct is_single_type_tuple<std::tuple<T1, T2>> : std::is_same<T1, T2> {};

    template <class T1, class T2, class... Tn>
    struct is_single_type_tuple<std::tuple<T1, T2, Tn...>>
        : std::bool_constant<std::is_same_v<T1, T2> && is_single_type_tuple<std::tuple<T2, Tn...>>::value> {};

    template <class L> constexpr bool is_single_type_tuple_v = is_single_type_tuple<L>::value;

} // namespace futures::detail

#endif // FUTURES_TUPLE_TYPE_IS_SINGLE_TYPE_TUPLE_H

// #include <futures/adaptor/detail/traits/is_tuple.h>

// #include <futures/adaptor/detail/traits/is_tuple_invocable.h>
//
// Copyright (c) alandefreitas 12/4/21.
// See accompanying file LICENSE
//

#ifndef FUTURES_IS_TUPLE_INVOCABLE_H
#define FUTURES_IS_TUPLE_INVOCABLE_H

// #include <tuple>

// #include <type_traits>


namespace futures::detail {
    /** \addtogroup futures Futures
     *  @{
     */

    /// \brief Check if a function can be invoked with the elements of a tuple as arguments, as in std::apply
    template <typename Function, typename Tuple>
    struct is_tuple_invocable : std::false_type {};

    template <typename Function, class... Args>
    struct is_tuple_invocable<Function, std::tuple<Args...>>
        : std::is_invocable<Function, Args...> {};

    template <typename Function, typename Tuple>
    constexpr bool is_tuple_invocable_v = is_tuple_invocable<Function, Tuple>::value;

    /** @} */
} // namespace futures::detail

#endif // FUTURES_IS_TUPLE_INVOCABLE_H

// #include <futures/adaptor/detail/traits/is_when_any_result.h>
//
// Copyright (c) alandefreitas 12/4/21.
// See accompanying file LICENSE
//

#ifndef FUTURES_IS_WHEN_ANY_RESULT_H
#define FUTURES_IS_WHEN_ANY_RESULT_H

// #include <futures/adaptor/when_any_result.h>
//
// Created by Alan Freitas on 8/20/21.
//

#ifndef FUTURES_WHEN_ANY_RESULT_H
#define FUTURES_WHEN_ANY_RESULT_H

namespace futures {
    /** \addtogroup adaptors Adaptors
     *  @{
     */

    /// \brief Result type for when_any_future objects
    ///
    /// This is defined in a separate file because many other concepts depend on this definition,
    /// especially the inferences for unwrapping `then` continuations, regardless of the when_any algorithm.
    template <typename Sequence> struct when_any_result {
        using size_type = std::size_t;
        using sequence_type = Sequence;

        size_type index{static_cast<size_type>(-1)};
        sequence_type tasks;
    };



    /** @} */
}

#endif // FUTURES_WHEN_ANY_RESULT_H

// #include <type_traits>


namespace futures::detail {
    /** \addtogroup futures Futures
     *  @{
     */

    /// Check if type is a when_any_result
    template <typename> struct is_when_any_result : std::false_type {};
    template <typename Sequence> struct is_when_any_result<when_any_result<Sequence>> : std::true_type {};
    template <typename Sequence> struct is_when_any_result<const when_any_result<Sequence>> : std::true_type {};
    template <typename Sequence> struct is_when_any_result<when_any_result<Sequence> &> : std::true_type {};
    template <typename Sequence> struct is_when_any_result<when_any_result<Sequence> &&> : std::true_type {};
    template <typename Sequence> struct is_when_any_result<const when_any_result<Sequence> &> : std::true_type {};
    template <class T> constexpr bool is_when_any_result_v = is_when_any_result<T>::value;

    /** @} */
}


#endif // FUTURES_IS_WHEN_ANY_RESULT_H

// #include <futures/adaptor/detail/traits/tuple_type_all_of.h>
//
// Copyright (c) alandefreitas 12/4/21.
// See accompanying file LICENSE
//

#ifndef FUTURES_TUPLE_TYPE_ALL_OF_H
#define FUTURES_TUPLE_TYPE_ALL_OF_H

// #include <tuple>

// #include <type_traits>


// #include <futures/adaptor/detail/traits/is_tuple.h>


namespace futures::detail {
    /// \brief Check if all types in a tuple match a predicate
    template <class T, template <class...> class P> struct tuple_type_all_of : is_tuple<T> {};

    template <class T1, template <class...> class P> struct tuple_type_all_of<std::tuple<T1>, P> : P<T1> {};

    template <class T1, class... Tn, template <class...> class P>
    struct tuple_type_all_of<std::tuple<T1, Tn...>, P>
        : std::bool_constant<P<T1>::value && tuple_type_all_of<std::tuple<Tn...>, P>::value> {};

    template <class L, template <class...> class P>
    constexpr bool tuple_type_all_of_v = tuple_type_all_of<L, P>::value;

} // namespace futures::detail

#endif // FUTURES_TUPLE_TYPE_ALL_OF_H

// #include <futures/adaptor/detail/traits/tuple_type_concat.h>
//
// Copyright (c) alandefreitas 12/4/21.
// See accompanying file LICENSE
//

#ifndef FUTURES_TUPLE_TYPE_CONCAT_H
#define FUTURES_TUPLE_TYPE_CONCAT_H

// #include <tuple>


namespace futures::detail {
    /// \brief Concatenate type lists
    /// The detail functions related to type lists assume we use std::tuple for all type lists
    template<class...> struct tuple_type_concat {
        using type = std::tuple<>;
    };

    template<class T1> struct tuple_type_concat<T1> {
        using type = T1;
    };

    template <class... First, class... Second>
    struct tuple_type_concat<std::tuple<First...>, std::tuple<Second...>> {
        using type = std::tuple<First..., Second...>;
    };

    template<class T1, class... Tn>
    struct tuple_type_concat<T1, Tn...> {
        using type = typename tuple_type_concat<T1, typename tuple_type_concat<Tn...>::type>::type;
    };

    template<class... Tn>
    using tuple_type_concat_t = typename tuple_type_concat<Tn...>::type;
}

#endif // FUTURES_TUPLE_TYPE_CONCAT_H

// #include <futures/adaptor/detail/traits/tuple_type_transform.h>
//
// Copyright (c) alandefreitas 12/4/21.
// See accompanying file LICENSE
//

#ifndef FUTURES_TUPLE_TYPE_TRANSFORM_H
#define FUTURES_TUPLE_TYPE_TRANSFORM_H

// #include <tuple>

// #include <type_traits>


namespace futures::detail {
    /// \brief Transform all types in a tuple
    template <class L, template <class...> class P> struct tuple_type_transform {
        using type = std::tuple<>;
    };

    template <class... Tn, template <class...> class P>
    struct tuple_type_transform<std::tuple<Tn...>, P> {
        using type = std::tuple<typename P<Tn>::type...>;
    };

    template <class L, template <class...> class P>
    using tuple_type_transform_t = typename tuple_type_transform<L, P>::type;

} // namespace futures::detail

#endif // FUTURES_TUPLE_TYPE_TRANSFORM_H

// #include <futures/adaptor/detail/traits/type_member_or.h>
//
// Copyright (c) alandefreitas 12/5/21.
// See accompanying file LICENSE
//

#ifndef FUTURES_TYPE_MEMBER_OR_H
#define FUTURES_TYPE_MEMBER_OR_H

// #include <type_traits>


namespace futures::detail {
    /** \addtogroup futures Futures
     *  @{
     */

    /// \brief Return T::type or a second type as a placeholder if T::type doesn't exist
    /// This class is meant to avoid errors in std::conditional
    template <class, class Placeholder = void, class = void> struct type_member_or { using type = Placeholder; };

    template <class T, class Placeholder> struct type_member_or<T, Placeholder, std::void_t<typename T::type>> {
        using type = typename T::type;
    };

    template <class T, class Placeholder> using type_member_or_t = typename type_member_or<T>::type;

    /** @} */
} // namespace futures::detail

#endif // FUTURES_TYPE_MEMBER_OR_H

// #include <futures/adaptor/detail/tuple_algorithm.h>
//
// Created by Alan Freitas on 8/18/21.
//

#ifndef FUTURES_TUPLE_ALGORITHM_H
#define FUTURES_TUPLE_ALGORITHM_H

// #include <futures/futures/detail/throw_exception.h>

// #include <tuple>


namespace futures {
    /** \addtogroup futures Futures
     *  @{
     */

    namespace detail {
        template<class Function, class... Args, std::size_t... Is>
        static void for_each_impl(const std::tuple<Args...> &t, Function &&fn, std::index_sequence<Is...>) {
            (fn(std::get<Is>(t)), ...);
        }
    } // namespace detail

    /// \brief tuple_for_each for tuples
    template<class Function, class... Args>
    static void tuple_for_each(const std::tuple<Args...> &t, Function &&fn) {
        detail::for_each_impl(t, std::forward<Function>(fn), std::index_sequence_for<Args...>{});
    }

    namespace detail {
        template<class Function, class... Args1, class... Args2, std::size_t... Is>
        static void for_each_paired_impl(std::tuple<Args1...> &t1, std::tuple<Args2...> &t2, Function &&fn,
                                         std::index_sequence<Is...>) {
            (fn(std::get<Is>(t1), std::get<Is>(t2)), ...);
        }
    } // namespace detail

    /// \brief for_each_paired for paired tuples of same size
    template<class Function, class... Args1, class... Args2>
    static void for_each_paired(std::tuple<Args1...> &t1, std::tuple<Args2...> &t2, Function &&fn) {
        static_assert(std::tuple_size_v<std::tuple<Args1...>> == std::tuple_size_v<std::tuple<Args2...>>);
        detail::for_each_paired_impl(t1, t2, std::forward<Function>(fn), std::index_sequence_for<Args1...>{});
    }

    namespace detail {
        template<class Function, class... Args1, class T, size_t N, std::size_t... Is>
        static void for_each_paired_impl(std::tuple<Args1...> &t, std::array<T, N> &a, Function &&fn,
                                         std::index_sequence<Is...>) {
            (fn(std::get<Is>(t), a[Is]), ...);
        }
    } // namespace detail

    /// \brief for_each_paired for paired tuples and arrays of same size
    template<class Function, class... Args1, class T, size_t N>
    static void for_each_paired(std::tuple<Args1...> &t, std::array<T, N> &a, Function &&fn) {
        static_assert(std::tuple_size_v<std::tuple<Args1...>> == N);
        detail::for_each_paired_impl(t, a, std::forward<Function>(fn), std::index_sequence_for<Args1...>{});
    }

    /// \brief find_if for tuples
    template<class Function, size_t t_idx = 0, class... Args>
    static size_t tuple_find_if(const std::tuple<Args...> &t, Function &&fn) {
        if constexpr (t_idx == std::tuple_size_v<std::decay_t<decltype(t)>>) {
            return t_idx;
        } else {
            if (fn(std::get<t_idx>(t))) {
                return t_idx;
            }
            return tuple_find_if<Function, t_idx + 1, Args...>(t, std::forward<Function>(fn));
        }
    }

    namespace detail {
        template<class Function, class... Args, std::size_t... Is>
        static bool all_of_impl(const std::tuple<Args...> &t, Function &&fn, std::index_sequence<Is...>) {
            return (fn(std::get<Is>(t)) && ...);
        }
    } // namespace detail

    /// \brief all_of for tuples
    template<class Function, class... Args>
    static bool tuple_all_of(const std::tuple<Args...> &t, Function &&fn) {
        return detail::all_of_impl(t, std::forward<Function>(fn), std::index_sequence_for<Args...>{});
    }

    namespace detail {
        template<class Function, class... Args, std::size_t... Is>
        static bool any_of_impl(const std::tuple<Args...> &t, Function &&fn, std::index_sequence<Is...>) {
            return (fn(std::get<Is>(t)) || ...);
        }
    } // namespace detail

    /// \brief any_of for tuples
    template<class Function, class... Args>
    static bool tuple_any_of(const std::tuple<Args...> &t, Function &&fn) {
        return detail::any_of_impl(t, std::forward<Function>(fn), std::index_sequence_for<Args...>{});
    }

    /// \brief Apply a function to a single tuple element at runtime
    /// The function must, of course, be valid for all tuple elements
    template<class Function, class Tuple, size_t current_tuple_idx = 0,
            std::enable_if_t<is_callable_v < Function> &&is_tuple_v<Tuple> &&
            (current_tuple_idx < std::tuple_size_v<std::decay_t<Tuple>>),
    int> = 0>

    constexpr static auto apply(Function &&fn, Tuple &&t, std::size_t idx) {
        assert(idx < std::tuple_size_v<std::decay_t<Tuple>>);
        if (current_tuple_idx == idx) {
            return fn(std::get<current_tuple_idx>(t));
        } else if constexpr (current_tuple_idx + 1 < std::tuple_size_v<std::decay_t<Tuple>>) {
            return apply < Function, Tuple, current_tuple_idx + 1 > (std::forward<Function>(fn), std::forward<Tuple>(t),
                    idx);
        } else {
            detail::throw_exception<std::out_of_range>("apply:: tuple idx out of range");
        }
    }

    /// \brief Return the i-th element from a tuple whose types are the same
    /// The return expression function must, of course, be valid for all tuple elements
    template<
            class Tuple, size_t current_tuple_idx = 0,
            std::enable_if_t<
                    is_tuple_v < Tuple> &&(current_tuple_idx < std::tuple_size_v<std::decay_t<Tuple>>), int> = 0>

    constexpr static decltype(auto) get(Tuple &&t, std::size_t idx) {
        assert(idx < std::tuple_size_v<std::decay_t<Tuple>>);
        if (current_tuple_idx == idx) {
            return std::get<current_tuple_idx>(t);
        } else if constexpr (current_tuple_idx + 1 < std::tuple_size_v<std::decay_t<Tuple>>) {
            return get < Tuple, current_tuple_idx + 1 > (std::forward<Tuple>(t), idx);
        } else {
            detail::throw_exception<std::out_of_range>("get:: tuple idx out of range");
        }
    }

    /// \brief Return the i-th element from a tuple with a transformation function whose return is always the same
    /// The return expression function must, of course, be valid for all tuple elements
    template<
            class Tuple, size_t current_tuple_idx = 0, class TransformFn,
            std::enable_if_t<
                    is_tuple_v < Tuple> &&(current_tuple_idx < std::tuple_size_v<std::decay_t<Tuple>>), int> = 0>

    constexpr static decltype(auto) get(Tuple &&t, std::size_t idx, TransformFn &&transform) {
        assert(idx < std::tuple_size_v<std::decay_t<Tuple>>);
        if (current_tuple_idx == idx) {
            return transform(std::get<current_tuple_idx>(t));
        } else if constexpr (current_tuple_idx + 1 < std::tuple_size_v<std::decay_t<Tuple>>) {
            return get < Tuple, current_tuple_idx + 1 > (std::forward<Tuple>(t), idx, transform);
        } else {
            detail::throw_exception<std::out_of_range>("get:: tuple idx out of range");
        }
    }

    namespace detail {
        template<class F, class FT, class Tuple, std::size_t... I>
        constexpr decltype(auto) transform_and_apply_impl(F &&f, FT &&ft, Tuple &&t, std::index_sequence<I...>) {
            return std::invoke(std::forward<F>(f), ft(std::get<I>(std::forward<Tuple>(t)))...);
        }
    } // namespace detail

    template<class F, class FT, class Tuple>
    constexpr decltype(auto) transform_and_apply(F &&f, FT &&ft, Tuple &&t) {
        return detail::transform_and_apply_impl(
                std::forward<F>(f), std::forward<FT>(ft), std::forward<Tuple>(t),
                std::make_index_sequence<std::tuple_size_v<std::remove_reference_t<Tuple>>>{});
    }

    namespace detail {
        /// The tuple type after we filtered it with a template template predicate
        template<template<typename> typename UnaryPredicate, typename Tuple>
        struct filtered_tuple_type;

        /// The tuple type after we filtered it with a template template predicate
        template<template<typename> typename UnaryPredicate, typename... Ts>
        struct filtered_tuple_type<UnaryPredicate, std::tuple<Ts...>> {
            /// If this element has to be kept, returns `std::tuple<Ts>`
            /// Otherwise returns `std::tuple<>`
            template<class E>
            using t_filtered_tuple_type_impl =
            std::conditional_t<UnaryPredicate<E>::value, std::tuple<E>, std::tuple<>>;

            /// Determines the type that would be returned by `std::tuple_cat`
            ///  if it were called with instances of the types reported by
            ///  t_filtered_tuple_type_impl for each element
            using type = decltype(std::tuple_cat(std::declval<t_filtered_tuple_type_impl<Ts>>()...));
        };

        /// The tuple type after we filtered it with a template template predicate
        template<template<typename> typename UnaryPredicate, typename Tuple>
        struct transformed_tuple;

        /// The tuple type after we filtered it with a template template predicate
        template<template<typename> typename UnaryPredicate, typename... Ts>
        struct transformed_tuple<UnaryPredicate, std::tuple<Ts...>> {
            /// If this element has to be kept, returns `std::tuple<Ts>`
            /// Otherwise returns `std::tuple<>`
            template<class E> using transformed_tuple_element_type = typename UnaryPredicate<E>::type;

            /// Determines the type that would be returned by `std::tuple_cat`
            ///  if it were called with instances of the types reported by
            ///  transformed_tuple_element_type for each element
            using type = decltype(std::tuple_cat(std::declval<transformed_tuple_element_type<Ts>>()...));
        };
    } // namespace detail

    /// \brief Filter tuple elements based on their types
    template<template<typename> typename UnaryPredicate, typename... Ts>
    constexpr typename detail::filtered_tuple_type<UnaryPredicate, std::tuple<Ts...>>::type
    filter_if(const std::tuple<Ts...> &tup) {
        return std::apply(
                [](auto... tuple_value) {
                    return std::tuple_cat(std::conditional_t<UnaryPredicate<decltype(tuple_value)>::value,
                            std::tuple<decltype(tuple_value)>, std::tuple<>>{}...);
                },
                tup);
    }

    /// \brief Remove tuple elements based on their types
    template<template<typename> typename UnaryPredicate, typename... Ts>
    constexpr typename detail::filtered_tuple_type<UnaryPredicate, std::tuple<Ts...>>::type
    remove_if(const std::tuple<Ts...> &tup) {
        return std::apply(
                [](auto... tuple_value) {
                    return std::tuple_cat(std::conditional_t<not UnaryPredicate<decltype(tuple_value)>::value,
                            std::tuple<decltype(tuple_value)>, std::tuple<>>{}...);
                },
                tup);
    }

    /// \brief Transform tuple elements based on their types
    template<template<typename> typename UnaryPredicate, typename... Ts>
    constexpr typename detail::transformed_tuple<UnaryPredicate, std::tuple<Ts...>>::type
    transform(const std::tuple<Ts...> &tup) {
        return std::apply(
                [](auto... tuple_value) {
                    return std::tuple_cat(
                            std::tuple<typename UnaryPredicate<decltype(tuple_value)>::type>{tuple_value}...);
                },
                tup);
    }

    /** @} */  // \addtogroup futures Futures
} // namespace futures

#endif // FUTURES_TUPLE_ALGORITHM_H

// #include <futures/algorithm/detail/traits/range/range/concepts.h>

// #include <futures/futures/detail/traits/type_member_or_void.h>


// #include <futures/futures/basic_future.h>

// #include <futures/futures/traits/is_future.h>

// #include <futures/futures/traits/unwrap_future.h>
//
// Created by Alan Freitas on 8/18/21.
//

#ifndef FUTURES_UNWRAP_FUTURE_H
#define FUTURES_UNWRAP_FUTURE_H

// #include <futures/adaptor/detail/traits/has_get.h>
//
// Created by alandefreitas on 10/15/21.
//

#ifndef FUTURES_HAS_GET_H
#define FUTURES_HAS_GET_H

namespace futures {
    /** \addtogroup futures Futures
     *  @{
     */
    /** \addtogroup future-traits Future Traits
     *  @{
     */

    namespace detail {
        /// Check if a type implements the get function
        /// This is what we use to identify the return type of a future type candidate
        /// However, this doesn't mean the type is a future in the terms of the is_future concept
        template <typename T, typename = void> struct has_get : std::false_type {};

        template <typename T> struct has_get<T, std::void_t<decltype(std::declval<T>().get())>> : std::true_type {};
    } // namespace detail
    /** @} */  // \addtogroup future-traits Future Traits
    /** @} */  // \addtogroup futures Futures
} // namespace futures

#endif // FUTURES_HAS_GET_H

// #include <futures/futures/traits/is_future.h>


namespace futures {
    /** \addtogroup futures Futures
     *  @{
     */
    /** \addtogroup future-traits Future Traits
     *  @{
     */


    /// \brief Determine type the future object holds
    ///
    /// Primary template handles non-future types
    ///
    /// \note Not to be confused with continuation unwrapping
    template <typename T, class Enable = void> struct unwrap_future { using type = void; };

    /// \brief Determine type a future object holds (specialization for types that implement `get()`)
    ///
    /// Template for types that implement ::get()
    ///
    /// \note Not to be confused with continuation unwrapping
    template <typename Future>
    struct unwrap_future<Future, std::enable_if_t<detail::has_get<std::decay_t<Future>>::value>> {
        using type = std::invoke_result_t<decltype(&std::decay_t<Future>::get), Future>;
    };

    /// \brief Determine type a future object holds
    ///
    /// \note Not to be confused with continuation unwrapping
    template <class T> using unwrap_future_t = typename unwrap_future<T>::type;

    /** @} */  // \addtogroup future-traits Future Traits
    /** @} */  // \addtogroup futures Futures
} // namespace futures

#endif // FUTURES_UNWRAP_FUTURE_H


namespace futures::detail {
    /** \addtogroup futures Futures
     *  @{
     */

    struct unwrapping_failure_t {};

    /// \brief Get the element type of a when any result object
    /// This is a very specific helper trait we need
    template <typename T, class Enable = void> struct range_or_tuple_element_type {};

    template <typename Sequence> struct range_or_tuple_element_type<Sequence, std::enable_if_t<range<Sequence>>> {
        using type = range_value_t<Sequence>;
    };

    template <typename Sequence> struct range_or_tuple_element_type<Sequence, std::enable_if_t<is_tuple_v<Sequence>>> {
        using type = std::tuple_element_t<0, Sequence>;
    };

    template <class T> using range_or_tuple_element_type_t = typename range_or_tuple_element_type<T>::type;

    /// \brief Unwrap the results from `before` future object and give them to `continuation`
    ///
    /// This function unfortunately has very high cyclomatic complexity because it's the only way
    /// we can concatenate so many `if constexpr` without negating all previous conditions.
    ///
    /// \param before_future The antecedent future to be unwrapped
    /// \param continuation The continuation function
    /// \param args Arguments we send to the function before the unwrapped result (stop_token or <empty>)
    /// \return The continuation result
    template <class Future, typename Function, typename... PrefixArgs, std::enable_if_t<is_future_v<std::decay_t<Future>>, int> = 0>
    decltype(auto) unwrap_and_continue(Future &&before_future, Function &&continuation, PrefixArgs &&...prefix_args) {
        // Types we might use in continuation
        using value_type = unwrap_future_t<Future>;
        using lvalue_type = std::add_lvalue_reference_t<value_type>;
        using rvalue_type = std::add_rvalue_reference_t<value_type>;

        // What kind of unwrapping is the continuation invocable with
        constexpr bool no_unwrap = std::is_invocable_v<Function, PrefixArgs..., Future>;
        constexpr bool no_input = std::is_invocable_v<Function, PrefixArgs...>;
        constexpr bool value_unwrap = std::is_invocable_v<Function, PrefixArgs..., value_type>;
        constexpr bool lvalue_unwrap = std::is_invocable_v<Function, PrefixArgs..., lvalue_type>;
        constexpr bool rvalue_unwrap = std::is_invocable_v<Function, PrefixArgs..., rvalue_type>;
        constexpr bool double_unwrap =
            is_future_v<std::decay_t<value_type>> && std::is_invocable_v<Function, PrefixArgs..., unwrap_future_t<value_type>>;
        constexpr bool is_tuple = is_tuple_v<value_type>;
        constexpr bool is_range = range<value_type>;

        // 5 main unwrapping paths: (no unwrap, no input, single future, when_all, when_any)
        constexpr bool direct_unwrap = value_unwrap || lvalue_unwrap || rvalue_unwrap || double_unwrap;
        constexpr bool sequence_unwrap = is_tuple || range<value_type>;
        constexpr bool when_any_unwrap = is_when_any_result_v<value_type>;

        constexpr auto fail = []() {
            // Could not unwrap, return unwrapping_failure_t to indicate we couldn't unwrap the continuation
            // The function still needs to be well-formed because other templates depend on it
            detail::throw_exception<std::logic_error>("Continuation unwrapping not possible");
            return unwrapping_failure_t{};
        };

        // Common continuations for basic_future
        if constexpr (no_unwrap) {
            return continuation(std::forward<PrefixArgs>(prefix_args)..., detail::move_or_copy(before_future));
        } else if constexpr (no_input) {
            before_future.get();
            return continuation(std::forward<PrefixArgs>(prefix_args)...);
        } else if constexpr (direct_unwrap) {
            value_type prev_state = before_future.get();
            if constexpr (value_unwrap) {
                return continuation(std::forward<PrefixArgs>(prefix_args)..., std::move(prev_state));
            } else if constexpr (lvalue_unwrap) {
                return continuation(std::forward<PrefixArgs>(prefix_args)..., prev_state);
            } else if constexpr (rvalue_unwrap) {
                return continuation(std::forward<PrefixArgs>(prefix_args)..., std::move(prev_state));
            } else if constexpr (double_unwrap) {
                return continuation(std::forward<PrefixArgs>(prefix_args)..., prev_state.get());
            } else {
                return fail();
            }
        } else if constexpr (sequence_unwrap || when_any_unwrap) {
            using prefix_as_tuple = std::tuple<PrefixArgs...>;
            if constexpr (sequence_unwrap && is_tuple) {
                constexpr bool tuple_explode =
                    is_tuple_invocable_v<Function, tuple_type_concat_t<prefix_as_tuple, value_type>>;
                constexpr bool is_future_tuple = tuple_type_all_of_v<std::decay_t<value_type>, is_future>;
                if constexpr (tuple_explode) {
                    // future<tuple<future<T1>, future<T2>, ...>> -> function(future<T1>, future<T2>, ...)
                    return std::apply(
                        continuation,
                        std::tuple_cat(std::make_tuple(std::forward<PrefixArgs>(prefix_args)...), before_future.get()));
                } else if constexpr (is_future_tuple) {
                    // future<tuple<future<T1>, future<T2>, ...>> -> function(T1, T2, ...)
                    using unwrapped_elements = tuple_type_transform_t<value_type, unwrap_future>;
                    constexpr bool tuple_explode_unwrap =
                        is_tuple_invocable_v<Function, tuple_type_concat_t<prefix_as_tuple, unwrapped_elements>>;
                    if constexpr (tuple_explode_unwrap) {
                        return transform_and_apply(
                            continuation,
                            [](auto &&el) {
                                if constexpr (!is_future_v<std::decay_t<decltype(el)>>) {
                                    return el;
                                } else {
                                    return el.get();
                                }
                            },
                            std::tuple_cat(std::make_tuple(std::forward<PrefixArgs>(prefix_args)...),
                                           before_future.get()));
                    } else {
                        return fail();
                    }
                } else {
                    return fail();
                }
            } else if constexpr (sequence_unwrap && is_range) {
                // when_all vector<future<T>> -> function(futures::small_vector<T>)
                using range_value_t = detail::range_value_t<value_type>;
                constexpr bool is_range_of_futures = is_future_v<std::decay_t<range_value_t>>;
                using continuation_vector = futures::small_vector<unwrap_future_t<range_value_t>>;
                using lvalue_continuation_vector = std::add_lvalue_reference_t<continuation_vector>;
                constexpr bool vector_unwrap =
                    is_range_of_futures && (std::is_invocable_v<Function, PrefixArgs..., continuation_vector> ||
                                            std::is_invocable_v<Function, PrefixArgs..., lvalue_continuation_vector>);
                if constexpr (vector_unwrap) {
                    value_type futures_vector = before_future.get();
                    using future_vector_value_type = typename value_type::value_type;
                    using unwrap_vector_value_type = unwrap_future_t<future_vector_value_type>;
                    using unwrap_vector_type = ::futures::small_vector<unwrap_vector_value_type>;
                    unwrap_vector_type continuation_values;
                    std::transform(futures_vector.begin(), futures_vector.end(),
                                   std::back_inserter(continuation_values),
                                   [](future_vector_value_type &f) { return f.get(); });
                    return continuation(std::forward<PrefixArgs>(prefix_args)..., continuation_values);
                } else {
                    return fail();
                }
            } else if constexpr (when_any_unwrap) {
                // Common continuations for when_any futures
                // when_any<tuple<future<T1>, future<T2>, ...>> -> function(size_t, tuple<future<T1>, future<T2>, ...>)
                using when_any_index = typename value_type::size_type;
                using when_any_sequence = typename value_type::sequence_type;
                using when_any_members_as_tuple = std::tuple<when_any_index, when_any_sequence>;
                constexpr bool when_any_split =
                    is_tuple_invocable_v<Function, tuple_type_concat_t<prefix_as_tuple, when_any_members_as_tuple>>;

                // when_any<tuple<future<>,...>> -> function(size_t, future<T1>, future<T2>, ...)
                constexpr bool when_any_explode = []() {
                    if constexpr (is_tuple_v<when_any_sequence>) {
                        return is_tuple_invocable_v<
                            Function,
                            tuple_type_concat_t<prefix_as_tuple, std::tuple<when_any_index>, when_any_sequence>>;
                    } else {
                        return false;
                    }
                }();

                // when_any_result<tuple<future<T>, future<T>, ...>> -> continuation(future<T>)
                constexpr bool when_any_same_type =
                    range<when_any_sequence> || is_single_type_tuple_v<when_any_sequence>;
                using when_any_element_type = range_or_tuple_element_type_t<when_any_sequence>;
                constexpr bool when_any_element =
                    when_any_same_type &&
                    is_tuple_invocable_v<Function,
                                         tuple_type_concat_t<prefix_as_tuple, std::tuple<when_any_element_type>>>;

                // when_any_result<tuple<future<T>, future<T>, ...>> -> continuation(T)
                constexpr bool when_any_unwrap_element =
                    when_any_same_type &&
                    is_tuple_invocable_v<
                        Function,
                        tuple_type_concat_t<prefix_as_tuple, std::tuple<unwrap_future_t<when_any_element_type>>>>;

                auto w = before_future.get();
                if constexpr (when_any_split) {
                    return std::apply(continuation, std::make_tuple(std::forward<PrefixArgs>(prefix_args)..., w.index,
                                                                    std::move(w.tasks)));
                } else if constexpr (when_any_explode) {
                    return std::apply(continuation,
                                      std::tuple_cat(std::make_tuple(std::forward<PrefixArgs>(prefix_args)..., w.index),
                                                     std::move(w.tasks)));
                } else if constexpr (when_any_element || when_any_unwrap_element) {
                    constexpr auto get_nth_future = [](auto &when_any_f) {
                        if constexpr (is_tuple_v<when_any_sequence>) {
                            return std::move(futures::get(std::move(when_any_f.tasks), when_any_f.index));
                        } else {
                            return std::move(when_any_f.tasks[when_any_f.index]);
                        }
                    };
                    auto nth_future = get_nth_future(w);
                    if constexpr (when_any_element) {
                        return continuation(std::forward<PrefixArgs>(prefix_args)..., std::move(nth_future));
                    } else if constexpr (when_any_unwrap_element) {
                        return continuation(std::forward<PrefixArgs>(prefix_args)..., std::move(nth_future.get()));
                    } else {
                        return fail();
                    }
                } else {
                    return fail();
                }
            } else {
                return fail();
            }
        } else {
            return fail();
        }
    }

    /// \brief Find the result of unwrap and continue or return unwrapping_failure_t if expression is not well-formed
    template <class Future, class Function, class = void> struct result_of_unwrap {
        using type = unwrapping_failure_t;
    };

    template <class Future, class Function>
    struct result_of_unwrap<
        Future, Function,
        std::void_t<decltype(unwrap_and_continue(std::declval<Future>(), std::declval<Function>()))>> {
        using type = decltype(unwrap_and_continue(std::declval<Future>(), std::declval<Function>()));
    };

    template <class Future, class Function>
    using result_of_unwrap_t = typename result_of_unwrap<Future, Function>::type;

    /// \brief Find the result of unwrap and continue with token or return unwrapping_failure_t otherwise
    template <class Future, class Function, class = void> struct result_of_unwrap_with_token {
        using type = unwrapping_failure_t;
    };

    template <class Future, class Function>
    struct result_of_unwrap_with_token<
        Future, Function,
        std::void_t<decltype(unwrap_and_continue(std::declval<Future>(), std::declval<Function>(),
                                                 std::declval<stop_token>()))>> {
        using type =
            decltype(unwrap_and_continue(std::declval<Future>(), std::declval<Function>(), std::declval<stop_token>()));
    };

    template <class Future, class Function>
    using result_of_unwrap_with_token_t = typename result_of_unwrap_with_token<Future, Function>::type;

    template <typename Function, typename Future> struct unwrap_traits {
        // The return type of unwrap and continue function
        using unwrap_result_no_token_type = result_of_unwrap_t<Future, Function>;
        using unwrap_result_with_token_type = result_of_unwrap_with_token_t<Future, Function>;

        // Whether the continuation expects a token
        static constexpr bool is_valid_without_stop_token =
            !std::is_same_v<unwrap_result_no_token_type, unwrapping_failure_t>;
        static constexpr bool is_valid_with_stop_token =
            !std::is_same_v<unwrap_result_with_token_type, unwrapping_failure_t>;

        // Whether the continuation is valid
        static constexpr bool is_valid = is_valid_without_stop_token || is_valid_with_stop_token;

        // The result type of unwrap and continue for the valid version, with or without token
        using result_value_type =
            std::conditional_t<is_valid_with_stop_token, unwrap_result_with_token_type, unwrap_result_no_token_type>;

        // Stop token for the continuation function
        constexpr static bool continuation_expects_stop_token = is_valid_with_stop_token;

        // Check if the stop token should be inherited from previous future
        constexpr static bool previous_future_has_stop_token = has_stop_token_v<Future>;
        constexpr static bool previous_future_is_shared = is_shared_future_v<Future>;
        constexpr static bool inherit_stop_token = previous_future_has_stop_token && (!previous_future_is_shared);

        // Continuation future should have stop token
        constexpr static bool after_has_stop_token = is_valid_with_stop_token || inherit_stop_token;

        // The result type of unwrap and continue for the valid version, with or without token
        using result_future_type =
            std::conditional_t<after_has_stop_token, jcfuture<result_value_type>, cfuture<result_value_type>>;
    };

    /// \brief The result we get from the `then` function
    /// - If after function expects a stop token:
    ///   - If previous future is stoppable and not-shared: return jcfuture with shared stop source
    ///   - Otherwise:                                      return jcfuture with new stop source
    /// - If after function does not expect a stop token:
    ///   - If previous future is stoppable and not-shared: return jcfuture with shared stop source
    ///   - Otherwise:                                      return cfuture with no stop source
    template <typename Function, typename Future> struct result_of_then {
        using type = typename unwrap_traits<Function, Future>::result_future_type;
    };

    template <typename Function, typename Future>
    using result_of_then_t = typename result_of_then<Function, Future>::type;

    /// \brief A trait to validate whether a Function can be continuation to a future
    template <class Function, class Future>
    using is_valid_continuation = std::bool_constant<unwrap_traits<Function, Future>::is_valid>;

    template <class Function, class Future>
    constexpr bool is_valid_continuation_v = is_valid_continuation<Function, Future>::value;

    // Wrap implementation in empty struct to facilitate friends
    struct internal_then_functor {

        /// \brief Make an appropriate stop source for the continuation
        template <typename Function, class Future>
        static stop_source make_continuation_stop_source(const Future &before, const Function &) {
            using traits = unwrap_traits<Function, Future>;
            if constexpr (traits::after_has_stop_token) {
                if constexpr (traits::inherit_stop_token && (!traits::continuation_expects_stop_token)) {
                    // condition 1: continuation shares token
                    return before.get_stop_source();
                } else {
                    // condition 2: continuation has new token
                    return {};
                }
            } else {
                // condition 3: continuation has no token
                return stop_source(nostopstate);
            }
        }

        /// \brief Maybe copy the previous continuations source
        template <class Future> static continuations_source copy_continuations_source(const Future &before) {
            constexpr bool before_is_lazy_continuable = is_lazy_continuable_v<Future>;
            if constexpr (before_is_lazy_continuable) {
                return before.get_continuations_source();
            } else {
                return continuations_source(nocontinuationsstate);
            }
        }

        /// \brief Create a tuple with the arguments for unwrap and continue
        template <typename Function, class Future>
        static decltype(auto) make_unwrap_args_tuple(Future &&before_future, Function &&continuation_function,
                                                     stop_token st) {
            using traits = unwrap_traits<Function, Future>;
            if constexpr (!traits::is_valid_with_stop_token) {
                return std::make_tuple(std::forward<Future>(before_future),
                                       std::forward<Function>(continuation_function));
            } else {
                return std::make_tuple(std::forward<Future>(before_future),
                                       std::forward<Function>(continuation_function), st);
            }
        }

        template <typename Executor, typename Function, class Future
#ifndef FUTURES_DOXYGEN
                  ,
                  std::enable_if_t<is_executor_v<std::decay_t<Executor>> && !is_executor_v<std::decay_t<Function>> &&
                                       !is_executor_v<std::decay_t<Future>> && is_future_v<std::decay_t<Future>> &&
                                       is_valid_continuation_v<std::decay_t<Function>, std::decay_t<Future>>,
                                   int> = 0
#endif
                  >
        result_of_then_t<Function, Future> operator()(const Executor &ex, Future &&before, Function &&after) const {
            using traits = unwrap_traits<Function, Future>;

            // Shared sources
            stop_source ss = make_continuation_stop_source(before, after);
            detail::continuations_source after_continuations;
            continuations_source before_cs = copy_continuations_source(before);

            // Set up shared state (packaged task contains unwrap and continue instead of after)
            promise<typename traits::result_value_type> p;
            typename traits::result_future_type result{p.template get_future<typename traits::result_future_type>()};
            result.set_continuations_source(after_continuations);
            if constexpr (traits::after_has_stop_token) {
                result.set_stop_source(ss);
            }
            // Set the complete executor task, using result to fulfill the promise, and running continuations
            auto fulfill_promise = [p = std::move(p), // task and shared state
                                    before_future =
                                        detail::move_or_copy(std::forward<Future>(before)), // the previous future
                                    continuation = std::forward<Function>(after),           // the continuation function
                                    after_continuations,   // continuation source for after
                                    token = ss.get_token() // maybe shared stop token
            ]() mutable {
                try {
                    if constexpr (std::is_same_v<typename traits::result_value_type, void>) {
                        if constexpr (traits::is_valid_with_stop_token) {
                            detail::unwrap_and_continue(before_future, continuation, token);
                            p.set_value();
                        } else {
                            detail::unwrap_and_continue(before_future, continuation);
                            p.set_value();
                        }
                    } else {
                        if constexpr (traits::is_valid_with_stop_token) {
                            p.set_value(detail::unwrap_and_continue(before_future, continuation, token));
                        } else {
                            p.set_value(detail::unwrap_and_continue(before_future, continuation));
                        }
                    }
                } catch (...) {
                    p.set_exception(std::current_exception());
                }
                after_continuations.request_run();
            };

            // Move function to shared location because executors require handles to be copy constructable
            auto fulfill_promise_ptr = std::make_shared<decltype(fulfill_promise)>(std::move(fulfill_promise));
            auto copyable_handle = [fulfill_promise_ptr]() { (*fulfill_promise_ptr)(); };

            // Fire-and-forget: Post a handle running the complete continuation function to the executor
            if constexpr (is_lazy_continuable_v<Future>) {
                // Attach continuation to previous future.
                // - Continuation is posted when previous is done
                // - Continuation is posted immediately if previous is already done
                before_cs.emplace_continuation(
                    ex, [h = std::move(copyable_handle), ex]() { asio::post(ex, std::move(h)); });
            } else {
                // We defer the task in the executor because the input doesn't have lazy continuations.
                // The executor will take care of this running later, so we don't need polling.
                asio::defer(ex, std::move(copyable_handle));
            }

            return result;
        }
    };
    constexpr internal_then_functor internal_then;

    /** @} */
} // namespace futures::detail

#endif // FUTURES_CONTINUATION_UNWRAP_H


// #include <version>


namespace futures {
    /** \addtogroup adaptors Adaptors
     *
     * \brief Functions to create new futures from existing functions.
     *
     * This module defines functions we can use to create new futures from existing futures. Future adaptors
     * are future types of whose values are dependant on the condition of other future objects.
     *
     *  @{
     */

    /// \brief Schedule a continuation function to a future
    ///
    /// This creates a continuation that gets executed when the before future is over.
    /// The continuation needs to be invocable with the return type of the previous future.
    ///
    /// This function works for all kinds of futures but behavior depends on the input:
    /// - If previous future is continuable, attach the function to the continuation list
    /// - If previous future is not continuable (such as std::future), post to execution with deferred policy
    /// In both cases, the result becomes a cfuture or jcfuture.
    ///
    /// Stop tokens are also propagated:
    /// - If after function expects a stop token:
    ///   - If previous future is stoppable and not-shared: return jcfuture with shared stop source
    ///   - Otherwise:                                      return jcfuture with new stop source
    /// - If after function does not expect a stop token:
    ///   - If previous future is stoppable and not-shared: return jcfuture with shared stop source
    ///   - Otherwise:                                      return cfuture with no stop source
    ///
    /// \return A continuation to the before future
    template <typename Executor, typename Function, class Future
#ifndef FUTURES_DOXYGEN
              ,
              std::enable_if_t<is_executor_v<std::decay_t<Executor>> && !is_executor_v<std::decay_t<Function>> &&
                                   !is_executor_v<std::decay_t<Future>> && is_future_v<std::decay_t<Future>> &&
                                   detail::is_valid_continuation_v<std::decay_t<Function>, std::decay_t<Future>>,
                               int> = 0
#endif
              >
    decltype(auto) then(const Executor &ex, Future &&before, Function &&after) {
        return detail::internal_then(ex, std::forward<Future>(before), std::forward<Function>(after));
    }

    /// \brief Schedule a continuation function to a future, allow an executor as second parameter
    ///
    /// \see @ref then
    template <class Future, typename Executor, typename Function
#ifndef FUTURES_DOXYGEN
              ,
              std::enable_if_t<is_executor_v<std::decay_t<Executor>> && !is_executor_v<std::decay_t<Function>> &&
                                   !is_executor_v<std::decay_t<Future>> && is_future_v<std::decay_t<Future>> &&
                                   detail::is_valid_continuation_v<std::decay_t<Function>, std::decay_t<Future>>,
                               int> = 0
#endif
              >
    decltype(auto) then(Future &&before, const Executor &ex, Function &&after) {
        return then(ex, std::forward<Future>(before), std::forward<Function>(after));
    }

    /// \brief Schedule a continuation function to a future with the default executor
    ///
    /// \return A continuation to the before future
    ///
    /// \see @ref then
    template <class Future, typename Function
#ifndef FUTURES_DOXYGEN
              ,
              std::enable_if_t<!is_executor_v<std::decay_t<Function>> && !is_executor_v<std::decay_t<Future>> &&
                                   is_future_v<std::decay_t<Future>> &&
                                   detail::is_valid_continuation_v<std::decay_t<Function>, std::decay_t<Future>>,
                               int> = 0
#endif
              >
    decltype(auto) then(Future &&before, Function &&after) {
        return then(::futures::make_default_executor(), std::forward<Future>(before), std::forward<Function>(after));
    }

    /// \brief Operator to schedule a continuation function to a future
    ///
    /// \return A continuation to the before future
    template <class Future, typename Function
#ifndef FUTURES_DOXYGEN
              ,
              std::enable_if_t<!is_executor_v<std::decay_t<Function>> && !is_executor_v<std::decay_t<Future>> &&
                                   is_future_v<std::decay_t<Future>> &&
                                   detail::is_valid_continuation_v<std::decay_t<Function>, std::decay_t<Future>>,
                               int> = 0
#endif
              >
    auto operator>>(Future &&before, Function &&after) {
        return then(std::forward<Future>(before), std::forward<Function>(after));
    }

    /// \brief Schedule a continuation function to a future with a custom executor
    ///
    /// \return A continuation to the before future
    template <class Executor, class Future, typename Function
#ifndef FUTURES_DOXYGEN
              ,
              std::enable_if_t<is_executor_v<std::decay_t<Executor>> && !is_executor_v<std::decay_t<Function>> &&
                                   !is_executor_v<std::decay_t<Future>> && is_future_v<std::decay_t<Future>> &&
                                   detail::is_valid_continuation_v<std::decay_t<Function>, std::decay_t<Future>>,
                               int> = 0
#endif
              >
    auto operator>>(Future &&before, std::pair<const Executor &, Function &> &&after) {
        return then(after.first, std::forward<Future>(before), std::forward<Function>(after.second));
    }

    /// \brief Create a proxy pair to schedule a continuation function to a future with a custom executor
    ///
    /// For this operation, we needed an operator with higher precedence than operator>>
    /// Our options are: +, -, *, /, %, &, !, ~.
    /// Although + seems like an obvious choice, % is the one that leads to less conflict with other functions.
    ///
    /// \return A proxy pair to schedule execution
    template <class Executor, typename Function, typename... Args
#ifndef FUTURES_DOXYGEN
              ,
              std::enable_if_t<is_executor_v<std::decay_t<Executor>> && !is_executor_v<std::decay_t<Function>> &&
                                   !is_callable_v<std::decay_t<Executor>> && is_callable_v<std::decay_t<Function>>,
                               int> = 0
#endif
              >
    auto operator%(const Executor &ex, Function &&after) {
        return std::make_pair(std::cref(ex), std::ref(after));
    }

    /** @} */
} // namespace futures

#endif // FUTURES_THEN_H

// #include <futures/adaptor/when_all.h>
//
// Created by Alan Freitas on 8/18/21.
//

#ifndef FUTURES_WHEN_ALL_H
#define FUTURES_WHEN_ALL_H

/// \file Implement the when_all functionality for futures and executors
///
/// Because all tasks need to be done to achieve the result, the algorithm doesn't depend much
/// on the properties of the underlying futures. The thread that is awaiting just needs sleep
/// and await for each of the internal futures.
///
/// The usual approach, without our future concepts, like in returning another std::future, is
/// to start another polling thread, which sets a promise when all other futures are ready.
/// If the futures support lazy continuations, these promises can be set from the previous
/// objects. However, this has an obvious cost for such a trivial operation, given that the
/// solutions is already available in the underlying futures.
///
/// Instead, we implement one more future type `when_all_future` that can query if the
/// futures are ready and waits for them to be ready whenever get() is called.
/// This proxy object can then be converted to a regular future if the user needs to.
///
/// This has a disadvantage over futures with lazy continuations because we might need
/// to schedule another task if we need notifications from this future. However,
/// we avoid scheduling another task right now, so this is, at worst, as good as
/// the common approach of wrapping it into another existing future type.
///
/// If the input futures are not shared, they are moved into `when_all_future` and are invalidated,
/// as usual. The `when_all_future` cannot be shared.

// #include <futures/config/small_vector_include.h>


// #include <futures/algorithm/detail/traits/range/range/concepts.h>


// #include <futures/adaptor/detail/traits/is_tuple.h>

// #include <futures/adaptor/detail/tuple_algorithm.h>

// #include <futures/futures/traits/to_future.h>
//
// Created by Alan Freitas on 8/19/21.
//

#ifndef FUTURES_TO_FUTURE_H
#define FUTURES_TO_FUTURE_H

namespace futures {
    /// \brief Trait to convert input type to its proper future type
    ///
    /// - Futures become their decayed versions
    /// - Lambdas become futures
    ///
    /// The primary template handles non-future types
    template <typename T, class Enable = void> struct to_future { using type = void; };

    /// \brief Trait to convert input type to its proper future type (specialization for future types)
    ///
    /// - Futures become their decayed versions
    /// - Lambdas become futures
    ///
    /// The primary template handles non-future types
    template <typename Future> struct to_future<Future, std::enable_if_t<is_future_v<std::decay_t<Future>>>> {
        using type = std::decay_t<Future>;
    };

    /// \brief Trait to convert input type to its proper future type (specialization for functions)
    ///
    /// - Futures become their decayed versions
    /// - Lambdas become futures
    ///
    /// The primary template handles non-future types
    template <typename Lambda> struct to_future<Lambda, std::enable_if_t<std::is_invocable_v<std::decay_t<Lambda>>>> {
        using type = futures::cfuture<std::invoke_result_t<std::decay_t<Lambda>>>;
    };

    template <class T> using to_future_t = typename to_future<T>::type;
} // namespace futures

#endif // FUTURES_TO_FUTURE_H


namespace futures {
    /** \addtogroup adaptors Adaptors
     *  @{
     */

    /// \brief Proxy future class referring to the result of a conjunction of futures from @ref when_all
    ///
    /// This class implements the behavior of the `when_all` operation as another future type,
    /// which can handle heterogeneous future objects.
    ///
    /// This future type logically checks the results of other futures in place to avoid creating a
    /// real conjunction of futures that would need to be polling (or be a lazy continuation)
    /// on another thread.
    ///
    /// If the user does want to poll on another thread, then this can be converted into a cfuture
    /// as usual with async. If the other future holds the when_all_state as part of its state,
    /// then it can become another future.
    template <class Sequence> class when_all_future {
      private:
        using sequence_type = Sequence;
        using corresponding_future_type = std::future<sequence_type>;
        static constexpr bool sequence_is_range = futures::detail::range<sequence_type>;
        static constexpr bool sequence_is_tuple = is_tuple_v<sequence_type>;
        static_assert(sequence_is_range || sequence_is_tuple);

      public:
        /// \brief Default constructor.
        /// Constructs a when_all_future with no shared state. After construction, valid() == false
        when_all_future() noexcept = default;

        /// \brief Move a sequence of futures into the when_all_future constructor.
        /// The sequence is moved into this future object and the objects from which the
        /// sequence was created get invalidated
        explicit when_all_future(sequence_type &&v) noexcept : v(std::move(v)) {}

        /// \brief Move constructor.
        /// Constructs a when_all_future with the shared state of other using move semantics.
        /// After construction, other.valid() == false
        when_all_future(when_all_future &&other) noexcept : v(std::move(other.v)) {}

        /// \brief when_all_future is not CopyConstructible
        when_all_future(const when_all_future &other) = delete;

        /// \brief Releases any shared state.
        /// - If the return object or provider holds the last reference to its shared state, the shared state is
        /// destroyed
        /// - the return object or provider gives up its reference to its shared state
        /// This means we just need to let the internal futures destroy themselves
        ~when_all_future() = default;

        /// \brief Assigns the contents of another future object.
        /// Releases any shared state and move-assigns the contents of other to *this.
        /// After the assignment, other.valid() == false and this->valid() will yield the same value as
        /// other.valid() before the assignment.
        when_all_future &operator=(when_all_future &&other) noexcept { v = std::move(other.v); }

        /// \brief Assigns the contents of another future object.
        /// when_all_future is not CopyAssignable.
        when_all_future &operator=(const when_all_future &other) = delete;

        /// \brief Wait until all futures have a valid result and retrieves it
        /// It effectively calls wait() in order to wait for the result.
        /// The behavior is undefined if valid() is false before the call to this function.
        /// Any shared state is released. valid() is false after a call to this method.
        /// The value v stored in the shared state, as std::move(v)
        sequence_type get() {
            // Check if the sequence is valid
            if (not valid()) {
                throw std::future_error(std::future_errc::no_state);
            }
            // Wait for the complete sequence to be ready
            wait();
            // Move results
            return std::move(v);
        }

        /// \brief Checks if the future refers to a shared state
        [[nodiscard]] bool valid() const noexcept {
            if constexpr (sequence_is_range) {
                return std::all_of(v.begin(), v.end(), [](auto &&f) { return f.valid(); });
            } else {
                return tuple_all_of(v, [](auto &&f) { return f.valid(); });
            }
        }

        /// \brief Blocks until the result becomes available.
        /// valid() == true after the call.
        /// The behavior is undefined if valid() == false before the call to this function
        void wait() const {
            // Check if the sequence is valid
            if (not valid()) {
                throw std::future_error(std::future_errc::no_state);
            }
            if constexpr (sequence_is_range) {
                std::for_each(v.begin(), v.end(), [](auto &&f) { f.wait(); });
            } else {
                tuple_for_each(v, [](auto &&f) { f.wait(); });
            }
        }

        /// \brief Waits for the result to become available.
        /// Blocks until specified timeout_duration has elapsed or the result becomes available, whichever comes
        /// first.
        template <class Rep, class Period>
        [[nodiscard]] std::future_status wait_for(const std::chrono::duration<Rep, Period> &timeout_duration) const {
            constexpr bool is_compile_time_empty = []() {
                if constexpr (sequence_is_tuple) {
                    return std::tuple_size_v<sequence_type> == 0;
                } else {
                    return false;
                }
            }();
            if constexpr (sequence_is_tuple && is_compile_time_empty) {
                return std::future_status::ready;
            } else {
                if constexpr (sequence_is_range) {
                    if (v.empty()) {
                        return std::future_status::ready;
                    }
                }

                // Check if the sequence is valid
                if (not valid()) {
                    throw std::future_error(std::future_errc::no_state);
                }
                using duration_type = std::chrono::duration<Rep, Period>;
                using namespace std::chrono;
                auto start_time = system_clock::now();
                duration_type total_elapsed = duration_cast<duration_type>(nanoseconds(0));
                auto equal_fn = [&](auto &&f) {
                    std::future_status s = f.wait_for(timeout_duration - total_elapsed);
                    total_elapsed = duration_cast<duration_type>(system_clock::now() - start_time);
                    const bool when_all_impossible = s != std::future_status::ready;
                    return when_all_impossible || total_elapsed > timeout_duration;
                };
                if constexpr (sequence_is_range) {
                    // Use a hack to "break" for_each loops with find_if
                    auto it = std::find_if(v.begin(), v.end(), equal_fn);
                    return (it == v.end()) ? std::future_status::ready : it->wait_for(seconds(0));
                } else {
                    auto idx = tuple_find_if(v, equal_fn);
                    if (idx == std::tuple_size<sequence_type>()) {
                        return std::future_status::ready;
                    } else {
                        std::future_status s =
                            apply([](auto &&el) -> std::future_status { return el.wait_for(seconds(0)); }, v, idx);
                        return s;
                    }
                }
            }
        }

        /// \brief wait_until waits for a result to become available.
        /// It blocks until specified timeout_time has been reached or the result becomes available, whichever comes
        /// first
        template <class Clock, class Duration>
        std::future_status wait_until(const std::chrono::time_point<Clock, Duration> &timeout_time) const {
            auto now_time = std::chrono::system_clock::now();
            return now_time > timeout_time ? wait_for(std::chrono::seconds(0))
                                           : wait_for(timeout_time - std::chrono::system_clock::now());
        }

        /// \brief Allow move the underlying sequence somewhere else
        /// The when_all_future is left empty and should now be considered invalid.
        /// This is useful for the algorithm that merges two wait_all_future objects without
        /// forcing encapsulation of the merge function.
        sequence_type &&release() { return std::move(v); }

        /// \brief Request the stoppable futures to stop
        bool request_stop() noexcept {
            bool any_request = false;
            auto f_request_stop = [&](auto &&f) { any_request = any_request || f.request_stop(); };
            if constexpr (sequence_is_range) {
                std::for_each(v.begin(), v.end(), f_request_stop);
            } else {
                tuple_for_each(v, f_request_stop);
            }
            return any_request;
        }

      private:
        /// \brief Internal wait_all_future state
        sequence_type v;
    };

#ifndef FUTURES_DOXYGEN
    /// \brief Specialization explicitly setting when_all_future<T> as a type of future
    template <typename T> struct is_future<when_all_future<T>> : std::true_type {};
#endif

    namespace detail {
        /// \name when_all helper traits
        /// Useful traits for when all future
        ///@{

        /// \brief Check if type is a when_all_future as a type
        template <typename> struct is_when_all_future : std::false_type {};
        template <typename Sequence> struct is_when_all_future<when_all_future<Sequence>> : std::true_type {};

        /// \brief Check if type is a when_all_future as constant bool
        template <class T> constexpr bool is_when_all_future_v = is_when_all_future<T>::value;

        /// \brief Check if a type can be used in a future conjunction (when_all or operator&& for futures)
        template <class T>
        using is_valid_when_all_argument =
            std::disjunction<is_future<std::decay_t<T>>, std::is_invocable<std::decay_t<T>>>;
        template <class T> constexpr bool is_valid_when_all_argument_v = is_valid_when_all_argument<T>::value;

        /// \brief Trait to identify valid when_all inputs
        template <class...> struct are_valid_when_all_arguments : std::true_type {};
        template <class B1> struct are_valid_when_all_arguments<B1> : is_valid_when_all_argument<B1> {};
        template <class B1, class... Bn>
        struct are_valid_when_all_arguments<B1, Bn...>
            : std::conditional_t<is_valid_when_all_argument_v<B1>, are_valid_when_all_arguments<Bn...>,
                                 std::false_type> {};
        template <class... Args>
        constexpr bool are_valid_when_all_arguments_v = are_valid_when_all_arguments<Args...>::value;
        /// @}

        /// \name Helpers and traits for operator&& on futures, functions and when_all futures
        /// @{

        /// \brief Check if type is a when_all_future with tuples as a sequence type
        template <typename T, class Enable = void> struct is_when_all_tuple_future : std::false_type {};
        template <typename Sequence>
        struct is_when_all_tuple_future<when_all_future<Sequence>, std::enable_if_t<is_tuple_v<Sequence>>>
            : std::true_type {};
        template <class T> constexpr bool is_when_all_tuple_future_v = is_when_all_tuple_future<T>::value;

        /// \brief Check if all template parameters are when_all_future with tuples as a sequence type
        template <class...> struct are_when_all_tuple_futures : std::true_type {};
        template <class B1> struct are_when_all_tuple_futures<B1> : is_when_all_tuple_future<std::decay_t<B1>> {};
        template <class B1, class... Bn>
        struct are_when_all_tuple_futures<B1, Bn...>
            : std::conditional_t<is_when_all_tuple_future_v<std::decay_t<B1>>, are_when_all_tuple_futures<Bn...>,
                                 std::false_type> {};
        template <class... Args>
        constexpr bool are_when_all_tuple_futures_v = are_when_all_tuple_futures<Args...>::value;

        /// \brief Check if type is a when_all_future with a range as a sequence type
        template <typename T, class Enable = void> struct is_when_all_range_future : std::false_type {};
        template <typename Sequence>
        struct is_when_all_range_future<when_all_future<Sequence>, std::enable_if_t<futures::detail::range<Sequence>>>
            : std::true_type {};
        template <class T> constexpr bool is_when_all_range_future_v = is_when_all_range_future<T>::value;

        /// \brief Check if all template parameters are when_all_future with tuples as a sequence type
        template <class...> struct are_when_all_range_futures : std::true_type {};
        template <class B1> struct are_when_all_range_futures<B1> : is_when_all_range_future<B1> {};
        template <class B1, class... Bn>
        struct are_when_all_range_futures<B1, Bn...>
            : std::conditional_t<is_when_all_range_future_v<B1>, are_when_all_range_futures<Bn...>, std::false_type> {};
        template <class... Args>
        constexpr bool are_when_all_range_futures_v = are_when_all_range_futures<Args...>::value;

        /// \brief Constructs a when_all_future that is a concatenation of all when_all_futures in args
        /// It's important to be able to merge when_all_future objects because of operator&&
        /// When the user asks for f1 && f2 && f3, we want that to return a single future that
        /// waits for <f1,f2,f3> rather than a future that wait for two futures <f1,<f2,f3>>
        /// \note This function only participates in overload resolution if all types in
        /// std::decay_t<WhenAllFutures>... are specializations of when_all_future with a tuple sequence type
        /// \overload "Merging" a single when_all_future of tuples. Overload provided for symmetry.
        template <class WhenAllFuture, std::enable_if_t<is_when_all_tuple_future_v<WhenAllFuture>, int> = 0>
        decltype(auto) when_all_future_cat(WhenAllFuture &&arg0) {
            return std::forward<WhenAllFuture>(arg0);
        }

        /// \overload Merging a two when_all_future objects of tuples
        template <class WhenAllFuture1, class WhenAllFuture2,
                  std::enable_if_t<are_when_all_tuple_futures_v<WhenAllFuture1, WhenAllFuture2>, int> = 0>
        decltype(auto) when_all_future_cat(WhenAllFuture1 &&arg0, WhenAllFuture2 &&arg1) {
            auto s1 = std::move(std::forward<WhenAllFuture1>(arg0).release());
            auto s2 = std::move(std::forward<WhenAllFuture2>(arg1).release());
            auto s = std::tuple_cat(std::move(s1), std::move(s2));
            return when_all_future(std::move(s));
        }

        /// \overload Merging two+ when_all_future of tuples
        template <class WhenAllFuture1, class... WhenAllFutures,
                  std::enable_if_t<are_when_all_tuple_futures_v<WhenAllFuture1, WhenAllFutures...>, int> = 0>
        decltype(auto) when_all_future_cat(WhenAllFuture1 &&arg0, WhenAllFutures &&...args) {
            auto s1 = std::move(std::forward<WhenAllFuture1>(arg0).release());
            auto s2 = std::move(when_all_future_cat(std::forward<WhenAllFutures>(args)...).release());
            auto s = std::tuple_cat(std::move(s1), std::move(s2));
            return when_all_future(std::move(s));
        }

        // When making the tuple for when_all_future:
        // - futures need to be moved
        // - shared futures need to be copied
        // - lambdas need to be posted
        template <typename F> constexpr decltype(auto) move_share_or_post(F &&f) {
            if constexpr (is_shared_future_v<std::decay_t<F>>) {
                return std::forward<F>(f);
            } else if constexpr (is_future_v<std::decay_t<F>>) {
                return std::move(std::forward<F>(f));
            } else /* if constexpr (std::is_invocable_v<F>) */ {
                return futures::async(std::forward<F>(f));
            }
        }
        ///@}
    } // namespace detail

    /// \brief Create a future object that becomes ready when the range of input futures becomes ready
    ///
    /// This function does not participate in overload resolution unless InputIt's value type (i.e.,
    /// typename std::iterator_traits<InputIt>::value_type) is a std::future or
    /// std::shared_future.
    ///
    /// This overload uses a small vector for avoid further allocations for such a simple operation.
    ///
    /// \return Future object of type @ref when_all_future
    template <class InputIt
#ifndef FUTURES_DOXYGEN
              ,
              std::enable_if_t<detail::is_valid_when_all_argument_v<typename std::iterator_traits<InputIt>::value_type>,
                               int> = 0
#endif
              >
    when_all_future<futures::small_vector<to_future_t<typename std::iterator_traits<InputIt>::value_type>>>
    when_all(InputIt first, InputIt last) {
        // Infer types
        using input_type = std::decay_t<typename std::iterator_traits<InputIt>::value_type>;
        constexpr bool input_is_future = is_future_v<input_type>;
        constexpr bool input_is_invocable = std::is_invocable_v<input_type>;
        static_assert(input_is_future || input_is_invocable);
        using output_future_type = to_future_t<input_type>;
        using sequence_type = futures::small_vector<output_future_type>;
        constexpr bool output_is_shared = is_shared_future_v<output_future_type>;

        // Create sequence
        sequence_type v;
        v.reserve(std::distance(first, last));

        // Move or copy the future objects
        if constexpr (input_is_future) {
            if constexpr (output_is_shared) {
                std::copy(first, last, std::back_inserter(v));
            } else /* if constexpr (not output_is_shared) */ {
                std::move(first, last, std::back_inserter(v));
            }
        } else /* if constexpr (input_is_invocable) */ {
            static_assert(input_is_invocable);
            std::transform(first, last, std::back_inserter(v),
                           [](auto &&f) { return std::move(futures::async(std::forward<decltype(f)>(f))); });
        }

        return when_all_future<sequence_type>(std::move(v));
    }

    /// \brief Create a future object that becomes ready when the range of input futures becomes ready
    ///
    /// This function does not participate in overload resolution unless the range type @ref is_future.
    ///
    /// \return Future object of type @ref when_all_future
    template <class Range
#ifndef FUTURES_DOXYGEN
              ,
              std::enable_if_t<futures::detail::range<std::decay_t<Range>>, int> = 0
#endif
              >
    when_all_future<futures::small_vector<
        to_future_t<typename std::iterator_traits<typename std::decay_t<Range>::iterator>::value_type>>>
    when_all(Range &&r) {
        return when_all(std::begin(std::forward<Range>(r)), std::end(std::forward<Range>(r)));
    }

    /// \brief Create a future object that becomes ready when all of the input futures become ready
    ///
    /// This function does not participate in overload resolution unless every argument is either a (possibly
    /// cv-qualified) shared_future or a cv-unqualified future, as defined by the trait @ref is_future.
    ///
    /// \return Future object of type @ref when_all_future
    template <class... Futures
#ifndef FUTURES_DOXYGEN
              ,
              std::enable_if_t<detail::are_valid_when_all_arguments_v<Futures...>, int> = 0
#endif
              >
    when_all_future<std::tuple<to_future_t<Futures>...>> when_all(Futures &&...futures) {
        // Infer sequence type
        using sequence_type = std::tuple<to_future_t<Futures>...>;

        // Create sequence (and infer types as we go)
        sequence_type v = std::make_tuple((detail::move_share_or_post(futures))...);

        return when_all_future<sequence_type>(std::move(v));
    }

    /// \brief Operator to create a future object that becomes ready when all of the input futures are ready
    ///
    /// Cperator&& works for futures and functions (which are converted to futures with the default executor)
    /// If the future is a when_all_future itself, then it gets merged instead of becoming a child future
    /// of another when_all_future.
    ///
    /// When the user asks for f1 && f2 && f3, we want that to return a single future that
    /// waits for <f1,f2,f3> rather than a future that wait for two futures <f1,<f2,f3>>.
    ///
    /// This emulates the usual behavior we expect from other types with operator&&.
    ///
    /// Note that this default behaviour is different from when_all(...), which doesn't merge
    /// the when_all_future objects by default, because they are variadic functions and
    /// this intention can be controlled explicitly:
    /// - when_all(f1,f2,f3) -> <f1,f2,f3>
    /// - when_all(f1,when_all(f2,f3)) -> <f1,<f2,f3>>
    ///
    /// \return @ref when_all_future object that concatenates all futures
    template <
        class T1, class T2
#ifndef FUTURES_DOXYGEN
        ,
        std::enable_if_t<detail::is_valid_when_all_argument_v<T1> && detail::is_valid_when_all_argument_v<T2>, int> = 0
#endif
        >
    auto operator&&(T1 &&lhs, T2 &&rhs) {
        constexpr bool first_is_when_all = detail::is_when_all_future_v<T1>;
        constexpr bool second_is_when_all = detail::is_when_all_future_v<T2>;
        constexpr bool both_are_when_all = first_is_when_all && second_is_when_all;
        if constexpr (both_are_when_all) {
            // Merge when all futures with operator&&
            return detail::when_all_future_cat(std::forward<T1>(lhs), std::forward<T2>(rhs));
        } else {
            // At least one of the arguments is not a when_all_future.
            // Any such argument might be another future or a function which needs to become a future.
            // Thus, we need a function to maybe convert these functions to futures.
            auto maybe_make_future = [](auto &&f) {
                if constexpr (std::is_invocable_v<decltype(f)> && not is_future_v<decltype(f)>) {
                    // Convert to future with the default executor if not a future yet
                    return asio::post(make_default_executor(), asio::use_future(std::forward<decltype(f)>(f)));
                } else {
                    if constexpr (is_shared_future_v<decltype(f)>) {
                        return std::forward<decltype(f)>(f);
                    } else {
                        return std::move(std::forward<decltype(f)>(f));
                    }
                }
            };
            // Simplest case, join futures in a new when_all_future
            constexpr bool none_are_when_all = not first_is_when_all && not second_is_when_all;
            if constexpr (none_are_when_all) {
                return when_all(maybe_make_future(std::forward<T1>(lhs)), maybe_make_future(std::forward<T2>(rhs)));
            } else if constexpr (first_is_when_all) {
                // If one of them is a when_all_future, then we need to concatenate the results
                // rather than creating a child in the sequence. To concatenate them, the
                // one that is not a when_all_future needs to become one.
                return detail::when_all_future_cat(lhs, when_all(maybe_make_future(std::forward<T2>(rhs))));
            } else /* if constexpr (second_is_when_all) */ {
                return detail::when_all_future_cat(when_all(maybe_make_future(std::forward<T1>(lhs))), rhs);
            }
        }
    }

    /** @} */
} // namespace futures

#endif // FUTURES_WHEN_ALL_H

// #include <futures/adaptor/when_any.h>
//
// Created by Alan Freitas on 8/18/21.
//

#ifndef FUTURES_WHEN_ANY_H
#define FUTURES_WHEN_ANY_H

/// \file Implement the when_any functionality for futures and executors
/// The same rationale as when_all applies here
/// \see https://en.cppreference.com/w/cpp/experimental/when_any

// #include <array>

// #include <condition_variable>

#include <optional>
// #include <shared_mutex>


// #include <futures/algorithm/detail/traits/range/range/concepts.h>


// #include <futures/config/small_vector_include.h>


// #include <futures/adaptor/detail/traits/is_tuple.h>

// #include <futures/adaptor/detail/tuple_algorithm.h>

// #include <futures/adaptor/when_any_result.h>

// #include <futures/futures/async.h>

// #include <futures/futures/traits/to_future.h>


namespace futures {
    /** \addtogroup adaptors Adaptors
     *  @{
     */

    /// \brief Proxy future class referring to the result of a disjunction of futures from @ref when_any
    ///
    /// This class implements another future type to identify when one of the tasks is over.
    ///
    /// As with `when_all`, this class acts as a future that checks the results of other futures to
    /// avoid creating a real disjunction of futures that would need to be polling on another thread.
    ///
    /// If the user does want to poll on another thread, then this can be converted into a regular future
    /// type with async or std::async.
    ///
    /// Not-polling is easier to emulate for future conjunctions (when_all) because we can sleep on
    /// each task until they are all done, since we need all of them anyway.
    ///
    /// For disjunctions, we have few options:
    /// - If the input futures have lazy continuations:
    ///   - Attach continuations to notify when a task is over
    /// - If the input futures do not have lazy continuations:
    ///   - Polling in a busy loop until one of the futures is ready
    ///   - Polling with exponential backoffs until one of the futures is ready
    ///   - Launching n continuation tasks that set a promise when one of the futures is ready
    ///   - Hybrids, usually polling for short tasks and launching threads for other tasks
    /// - If the input futures are mixed in regards to lazy continuations:
    ///   - Mix the strategies above, depending on each input future
    ///
    /// If the thresholds for these strategies are reasonable, this should be efficient for futures with
    /// or without lazy continuations.
    ///
    template <class Sequence> class when_any_future {
      private:
        using sequence_type = Sequence;
        static constexpr bool sequence_is_range = futures::detail::range<sequence_type>;
        static constexpr bool sequence_is_tuple = is_tuple_v<sequence_type>;
        static_assert(sequence_is_range || sequence_is_tuple);

      public:
        /// \brief Default constructor.
        /// Constructs a when_any_future with no shared state. After construction, valid() == false
        when_any_future() noexcept = default;

        /// \brief Move a sequence of futures into the when_any_future constructor.
        /// The sequence is moved into this future object and the objects from which the
        /// sequence was created get invalidated.
        ///
        /// We immediately set up the notifiers for any input future that supports lazy
        /// continuations.
        explicit when_any_future(sequence_type &&v) noexcept
            : v(std::move(v)), thread_notifiers_set(false), ready_notified(false) {
            maybe_set_up_lazy_notifiers();
        }

        /// \brief Move constructor.
        /// Constructs a when_any_future with the shared state of other using move semantics.
        /// After construction, other.valid() == false
        ///
        /// This is a class that controls resources, and their behavior needs to be moved. However,
        /// unlike a vector, some notifier resources cannot be moved and might need to be recreated,
        /// because they expect the underlying futures to be in a given address to work.
        ///
        /// We cannot move the notifiers because these expect things to be notified at certain addresses.
        /// This means the notifiers in `other` have to be stopped and we have to be sure of that before
        /// its destructor gets called.
        ///
        /// There are two in operations here.
        /// - Asking the notifiers to stop and waiting
        ///   - This is what we need to do at the destructor because we can't destruct "this" until
        ///     we are sure no notifiers are going to try to notify this object
        /// - Asking the notifiers to stop
        ///   - This is what we need to do when moving, because we know we won't need these notifiers
        ///     anymore. When the moved object gets destructed, it will ensure its notifiers are stopped
        ///     and finish the task.
        when_any_future(when_any_future &&other) noexcept : thread_notifiers_set(false), ready_notified(false) {
            other.request_notifiers_stop();
            // we can only move v after stopping the notifiers, or they will keep trying to
            // access invalid future address before they stop
            v = std::move(other.v);
            // Set up our own lazy notifiers
            maybe_set_up_lazy_notifiers();
        }

        /// \brief when_any_future is not CopyConstructible
        when_any_future(const when_any_future &other) = delete;

        /// \brief Releases any shared state.
        ///
        /// - If the return object or provider holds the last reference to its shared state, the shared state is
        /// destroyed.
        /// - the return object or provider gives up its reference to its shared state
        ///
        /// This means we just need to let the internal futures destroy themselves, but we have to stop
        /// notifiers if we have any, because these notifiers might later try to set tokens in a future
        /// that no longer exists.
        ///
        ~when_any_future() { request_notifiers_stop_and_wait(); };

        /// \brief Assigns the contents of another future object.
        ///
        /// Releases any shared state and move-assigns the contents of other to *this.
        ///
        /// After the assignment, other.valid() == false and this->valid() will yield the same value as
        /// other.valid() before the assignment.
        ///
        when_any_future &operator=(when_any_future &&other) noexcept {
            v = std::move(other.v);
            other.request_notifiers_stop();
            // Set up our own lazy notifiers
            maybe_set_up_lazy_notifiers();
        }

        /// \brief Copy assigns the contents of another when_any_future object.
        ///
        /// when_any_future is not copy assignable.
        when_any_future &operator=(const when_any_future &other) = delete;

        /// \brief Wait until any future has a valid result and retrieves it
        ///
        /// It effectively calls wait() in order to wait for the result.
        /// This avoids replicating the logic behind continuations, polling, and notifiers.
        ///
        /// The behavior is undefined if valid() is false before the call to this function.
        /// Any shared state is released. valid() is false after a call to this method.
        /// The value v stored in the shared state, as std::move(v)
        when_any_result<sequence_type> get() {
            // Check if the sequence is valid
            if (!valid()) {
                throw std::future_error(std::future_errc::no_state);
            }
            // Wait for the complete sequence to be ready
            wait();
            // Set up a `when_any_result` and move results to it.
            when_any_result<sequence_type> r;
            r.index = get_ready_index();
            request_notifiers_stop_and_wait();
            r.tasks = std::move(v);
            return r;
        }

        /// \brief Checks if the future refers to a shared state
        ///
        /// This future is always valid() unless there are tasks and they are all invalid
        ///
        /// \see https://en.cppreference.com/w/cpp/experimental/when_any
        [[nodiscard]] bool valid() const noexcept {
            if constexpr (sequence_is_range) {
                if (v.empty()) {
                    return true;
                }
                return std::any_of(v.begin(), v.end(), [](auto &&f) { return f.valid(); });
            } else {
                if constexpr (std::tuple_size_v<sequence_type> == 0) {
                    return true;
                } else {
                    return tuple_any_of(v, [](auto &&f) { return f.valid(); });
                }
            }
        }

        /// \brief Blocks until the result becomes available.
        /// valid() == true after the call.
        /// The behavior is undefined if valid() == false before the call to this function
        void wait() const {
            // Check if the sequence is valid
            if (!valid()) {
                throw std::future_error(std::future_errc::no_state);
            }
            // Reuse the logic from wait_for here
            using const_version = std::true_type;
            using timeout_version = std::false_type;
            wait_for_common<const_version, timeout_version>(*this, std::chrono::seconds(0));
        }

        /// \overload mutable version which allows setting up notifiers which might not have been set yet
        void wait() {
            // Check if the sequence is valid
            if (!valid()) {
                throw std::future_error(std::future_errc::no_state);
            }
            // Reuse the logic from wait_for here
            using const_version = std::false_type;
            using timeout_version = std::false_type;
            wait_for_common<const_version, timeout_version>(*this, std::chrono::seconds(0));
        }

        /// \brief Waits for the result to become available.
        /// Blocks until specified timeout_duration has elapsed or the result becomes available, whichever comes
        /// first.
        /// Not-polling is easier to emulate for future conjunctions (when_all) because we can sleep on
        /// each task until they are all done, since we need all of them anyway.
        /// For disjunctions, we have few options:
        /// - Polling in a busy loop until one of the futures is ready
        /// - Polling in a less busy loop with exponential backoffs
        /// - Launching n continuation tasks that set a promise when one of the futures is ready
        /// - Hybrids, usually polling for short tasks and launching threads for other tasks
        /// If these parameters are reasonable, this should not be less efficient that futures with
        /// continuations, because we save on the creation of new tasks.
        /// However, the relationship between the parameters depend on:
        /// - The number of tasks (n)
        /// - The estimated time of completion for each task (assumes a time distribution)
        /// - The probably a given task is the first task to finish (>=1/n)
        /// - The cost of launching continuation tasks
        /// Because we don't have access to information about the estimated time for a given task to finish,
        /// we can ignore a less-busy loop as a general solution. Thus, we can come up with a hybrid algorithm
        /// for all cases:
        /// - If there's only one task, behave as when_all
        /// - If there are more tasks:
        ///   1) Initially poll in a busy loop for a while, because tasks might finish very sooner than we would need
        ///      to create a continuations.
        ///   2) Create continuation tasks after a threshold time
        /// \see https://en.m.wikipedia.org/wiki/Exponential_backoff
        template <class Rep, class Period>
        std::future_status wait_for(const std::chrono::duration<Rep, Period> &timeout_duration) const {
            using const_version = std::true_type;
            using timeout_version = std::true_type;
            return wait_for_common<const_version, timeout_version>(*this, timeout_duration);
        }

        /// \overload wait for might need to be mutable so we can set up the notifiers
        /// \note the const version will only wait for notifiers if these have already been set
        template <class Rep, class Period>
        std::future_status wait_for(const std::chrono::duration<Rep, Period> &timeout_duration) {
            using const_version = std::false_type;
            using timeout_version = std::true_type;
            return wait_for_common<const_version, timeout_version>(*this, timeout_duration);
        }

        /// \brief wait_until waits for a result to become available.
        /// It blocks until specified timeout_time has been reached or the result becomes available, whichever comes
        /// first
        template <class Clock, class Duration>
        std::future_status wait_until(const std::chrono::time_point<Clock, Duration> &timeout_time) const {
            auto now_time = std::chrono::system_clock::now();
            return now_time > timeout_time ? wait_for(std::chrono::seconds(0))
                                           : wait_for(timeout_time - std::chrono::system_clock::now());
        }

        /// \overload mutable version that allows setting notifiers
        template <class Clock, class Duration>
        std::future_status wait_until(const std::chrono::time_point<Clock, Duration> &timeout_time) {
            auto now_time = std::chrono::system_clock::now();
            return now_time > timeout_time ? wait_for(std::chrono::seconds(0))
                                           : wait_for(timeout_time - std::chrono::system_clock::now());
        }

        /// \brief Check if it's ready
        [[nodiscard]] bool is_ready() const {
            auto idx = get_ready_index();
            return idx != static_cast<size_t>(-1) || (size() == 0);
        }

        /// \brief Allow move the underlying sequence somewhere else
        /// The when_any_future is left empty and should now be considered invalid.
        /// This is useful for the algorithm that merges two wait_any_future objects without
        /// forcing encapsulation of the merge function.
        sequence_type &&release() {
            request_notifiers_stop();
            return std::move(v);
        }

      private:
        /// \brief Get index of the first internal future that is ready
        /// If no future is ready, this returns the sequence size as a sentinel
        template <class CheckLazyContinuables = std::true_type> [[nodiscard]] size_t get_ready_index() const {
            const auto eq_comp = [](auto &&f) {
                if constexpr (CheckLazyContinuables::value || (!is_lazy_continuable_v<std::decay_t<decltype(f)>>)) {
                    return ::futures::is_ready(std::forward<decltype(f)>(f));
                } else {
                    return false;
                }
            };
            size_t ready_index(-1);
            if constexpr (sequence_is_range) {
                auto it = std::find_if(v.begin(), v.end(), eq_comp);
                ready_index = it - v.begin();
            } else {
                ready_index = tuple_find_if(v, eq_comp);
            }
            if (ready_index == size()) {
                return static_cast<size_t>(-1);
            } else {
                return ready_index;
            }
        }

        /// \brief To common paths to wait for a future
        /// \tparam const_version std::true_type or std::false_type for version with or without setting up new notifiers
        /// \tparam timeout_version std::true_type or std::false_type for version with or without timeout
        /// \tparam Rep Common std::chrono Rep time representation
        /// \tparam Period Common std::chrono Period time representation
        /// \param f when_any_future on which we want to wait
        /// \param timeout_duration Max time we should wait for a result (if timeout version is std::true_type)
        /// \return Status of the future (std::future_status::ready if any is ready)
        template <class const_version, class timeout_version, class Rep = std::chrono::seconds::rep,
                  class Period = std::chrono::seconds::period>
        static std::future_status
        wait_for_common(std::conditional_t<const_version::value, std::add_const<when_any_future>, when_any_future> &f,
                        const std::chrono::duration<Rep, Period> &timeout_duration) {
            constexpr bool is_trivial_tuple = sequence_is_tuple && (compile_time_size() < 2);
            if constexpr (is_trivial_tuple) {
                if constexpr (0 == compile_time_size()) {
                    // Trivial tuple: empty -> ready()
                    return std::future_status::ready;
                } else /* if constexpr (1 == compile_time_size()) */ {
                    // Trivial tuple: one element -> get()
                    if constexpr (timeout_version::value) {
                        return std::get<0>(f.v).wait_for(timeout_duration);
                    } else {
                        std::get<0>(f.v).wait();
                        return std::future_status::ready;
                    }
                }
            } else {
                if constexpr (sequence_is_range) {
                    if (f.v.empty()) {
                        // Trivial range: empty -> ready()
                        return std::future_status::ready;
                    } else if (f.v.size() == 1) {
                        // Trivial range: one element -> get()
                        if constexpr (timeout_version::value) {
                            return f.v.begin()->wait_for(timeout_duration);
                        } else {
                            f.v.begin()->wait();
                            return std::future_status::ready;
                        }
                    }
                }

                // General case: we already know it's ready
                if (f.is_ready()) {
                    return std::future_status::ready;
                }

                // All future types have their own notifiers as continuations created when
                // this object starts.
                // Don't busy wait if thread notifiers are already set anyway.
                if (f.all_lazy_continuable() || f.thread_notifiers_set) {
                    return f.template notifier_wait_for<timeout_version>(timeout_duration);
                }

                // Choose busy or future wait for, depending on how much time we have to wait
                // and whether the notifiers have been set in a previous function call
                if constexpr (const_version::value) {
                    // We cannot set up notifiers in the busy version of this function even though
                    // this is encapsulated. Maybe the notifiers should be always mutable, like
                    // we usually do with mutexes, but better safe than sorry. This has a small
                    // impact on continuable futures through.
                    return f.template busy_wait_for<timeout_version>(timeout_duration);
                } else /* if not const_version::value */ {
                    // - Don't busy wait forever, even though it implements an exponential backoff
                    // - Don't create notifiers for very short tasks either
                    const std::chrono::seconds max_busy_time(5);
                    const bool no_time_to_setup_notifiers = timeout_duration < max_busy_time;
                    // - Don't create notifiers if there are more tasks than the hardware limit already:
                    //   - If there are more tasks, the probability of a ready task increases while the cost
                    //     of notifiers is higher.
                    const bool too_many_threads_already = f.size() >= hardware_concurrency();
                    const bool busy_wait_only = no_time_to_setup_notifiers || too_many_threads_already;
                    if (busy_wait_only) {
                        return f.template busy_wait_for<timeout_version>(timeout_duration);
                    } else {
                        std::future_status s = f.template busy_wait_for<timeout_version>(max_busy_time);
                        if (s != std::future_status::ready) {
                            f.maybe_set_up_thread_notifiers();
                            return f.template notifier_wait_for<timeout_version>(timeout_duration - max_busy_time);
                        } else {
                            return s;
                        }
                    }
                }
            }
        }

        /// \brief Get number of internal futures
        [[nodiscard]] constexpr size_t size() const {
            if constexpr (sequence_is_tuple) {
                return std::tuple_size_v<sequence_type>;
            } else {
                return v.size();
            }
        }

        /// \brief Get number of internal futures with lazy continuations
        [[nodiscard]] constexpr size_t lazy_continuable_size() const {
            if constexpr (sequence_is_tuple) {
                return std::tuple_size_v<sequence_type>;
                size_t count = 0;
                tuple_for_each(v, [&count](auto &&el) {
                    if constexpr (is_lazy_continuable_v<std::decay_t<decltype(el)>>) {
                        ++count;
                    }
                });
                return count;
            } else {
                if (is_lazy_continuable_v<std::decay_t<typename sequence_type::value_type>>) {
                    return v.size();
                } else {
                    return 0;
                }
            }
        }

        /// \brief Check if all internal types are lazy continuable
        [[nodiscard]] constexpr bool all_lazy_continuable() const { return lazy_continuable_size() == size(); }

        /// \brief Get size, if we know that at compile time
        [[nodiscard]] static constexpr size_t compile_time_size() {
            if constexpr (sequence_is_tuple) {
                return std::tuple_size_v<sequence_type>;
            } else {
                return 0;
            }
        }

        /// \brief Check if the i-th future is ready
        [[nodiscard]] bool is_ready(size_t index) const {
            if constexpr (!sequence_is_range) {
                return apply([](auto &&el) -> std::future_status { return el.wait_for(std::chrono::seconds(0)); }, v,
                             index) == std::future_status::ready;
            } else {
                return v[index].wait_for(std::chrono::seconds(0)) == std::future_status::ready;
            }
        }

        /// \brief Busy wait for a certain amount of time
        /// \see https://en.m.wikipedia.org/wiki/Exponential_backoff
        template <class timeout_version, class Rep = std::chrono::seconds::rep,
                  class Period = std::chrono::seconds::period>
        std::future_status
        busy_wait_for(const std::chrono::duration<Rep, Period> &timeout_duration = std::chrono::seconds(0)) const {
            // Check if the sequence is valid
            if (!valid()) {
                throw std::future_error(std::future_errc::no_state);
            }
            // Wait for on each thread, increasingly accounting for the time we waited from the total
            using duration_type = std::chrono::duration<Rep, Period>;
            using namespace std::chrono;
            // 1) Total time we've waited so far
            auto start_time = system_clock::now();
            duration_type total_elapsed = duration_cast<duration_type>(nanoseconds(0));
            // 2) The time we are currently waiting on each thread (something like a minimum slot time)
            nanoseconds each_wait_for(1);
            // 3) After how long we should start increasing the each_wait_for
            // Wait longer in the busy pool if we have more tasks to account for the lower individual end in less-busy
            const auto full_busy_timeout_duration = milliseconds(100) * size();
            // 4) The increase factor of each_wait_for (20%)
            using each_wait_for_increase_factor = std::ratio<5, 4>;
            // 5) The max time we should wait on each thread (at most the estimated time to create a thread)
            // Assumes 1000 threads can be created per second and split that with the number of tasks
            // so that finding the task would never take longer than creating a thread
            auto max_wait_for = duration_cast<nanoseconds>(microseconds(20)) / size();
            // 6) Index of the future we found to be ready
            auto ready_in_the_meanwhile_idx = size_t(-1);
            // 7) Function to identify if a given task is ready
            // We will later keep looping on this function until we run out of time
            auto equal_fn = [&](auto &&f) {
                // a) Don't wait on the ones with lazy continuations
                // We are going to wait on all of them at once later
                if constexpr (is_lazy_continuable_v<std::decay_t<decltype(f)>>) {
                    return false;
                }
                // b) update parameters of our heuristic _before_ checking the future
                const bool use_backoffs = total_elapsed > full_busy_timeout_duration;
                if (use_backoffs) {
                    if (each_wait_for > max_wait_for) {
                        each_wait_for = max_wait_for;
                    } else if (each_wait_for < max_wait_for) {
                        each_wait_for *= each_wait_for_increase_factor::num;
                        each_wait_for /= each_wait_for_increase_factor::den;
                        each_wait_for += nanoseconds(1);
                    }
                }
                // c) effectively check if this future is ready
                std::future_status s = f.wait_for(each_wait_for);
                // d) update time spent on futures
                auto total_elapsed = system_clock::now() - start_time;
                // e) if this one wasn't ready, and we are already using exponential backoff
                if (s != std::future_status::ready && use_backoffs) {
                    // do a single pass in other futures to make sure no other future got ready in the meanwhile
                    size_t ready_in_the_meanwhile_idx = get_ready_index();
                    if (ready_in_the_meanwhile_idx != size_t(-1)) {
                        return true;
                    }
                }
                // f) request function to break the loop if we found something that's ready or we timed out
                return s == std::future_status::ready || (timeout_version::value && total_elapsed > timeout_duration);
            };
            // 8) Loop through the futures trying to find a ready future with the function above
            do {
                // Check for a signal from lazy continuable futures all at once
                // The notifiers for lazy continuable futures are always set up
                if (lazy_continuable_size() != 0 && notifiers_started()) {
                    std::future_status s = wait_for_ready_notification<timeout_version>(each_wait_for);
                    if (s == std::future_status::ready) {
                        return s;
                    }
                }
                // Check if other futures aren't ready
                // Use a hack to "break" "for_each" algorithms with find_if
                auto idx = size_t(-1);
                if constexpr (sequence_is_range) {
                    idx = std::find_if(v.begin(), v.end(), equal_fn) - v.begin();
                } else {
                    idx = tuple_find_if(v, equal_fn);
                }
                const bool found_ready_future = idx != size();
                if (found_ready_future) {
                    size_t ready_found_idx =
                        ready_in_the_meanwhile_idx != size_t(-1) ? ready_in_the_meanwhile_idx : idx;
                    if constexpr (sequence_is_range) {
                        return (v.begin() + ready_found_idx)->wait_for(seconds(0));
                    } else {
                        return apply([](auto &&el) -> std::future_status { return el.wait_for(seconds(0)); }, v,
                                     ready_found_idx);
                    }
                }
            } while (!timeout_version::value || total_elapsed < timeout_duration);
            return std::future_status::timeout;
        }

        /// \brief Check if the notifiers have started wait on futures
        [[nodiscard]] bool notifiers_started() const {
            if constexpr (sequence_is_range) {
                return std::any_of(notifiers.begin(), notifiers.end(),
                                   [](const notifier &n) { return n.start_token.load(); });
            } else {
                return tuple_any_of(v, [](auto &&f) { return f.valid(); });
            }
        };

        /// \brief Launch continuation threads and wait for any of them instead
        ///
        /// This is the second alternative to busy waiting, but once we used this alternative,
        /// we already have paid the price to create these continuation futures and we shouldn't
        /// busy wait ever again, even in a new call to wait_for. Thus, the wait_any_future
        /// needs to keep track of these continuation tasks to ensure this doesn't happen.
        ///
        /// Once the notifiers are set, if using an unlimited number of threads, we would only need to
        /// wait without a more specific timeout here.
        ///
        /// However, if the notifiers could not be launched for some reason, this can lock our process
        /// in the somewhat rare condition that (i) the last function is running in the last available
        /// pool thread, and (ii) none of the notifiers got a chance to get into the pool. We are then
        /// waiting for notifiers that don't exist yet and we don't have access to that information.
        ///
        /// To avoid that from happening, we (i) perform a single busy pass in the underlying futures
        /// from time to time to ensure they are really still running, regardless of the notifiers,
        /// and (ii) create a start token, besides the cancel token, for the notifiers to indicate
        /// that they have really started waiting. Thus, we can always check condition (i) to ensure
        /// we don't already have the results before we start waiting for them, and (ii) to ensure
        /// we don't wait for notifiers that are not running yet.
        template <class timeout_version, class Rep = std::chrono::seconds::rep,
                  class Period = std::chrono::seconds::period>
        std::future_status
        notifier_wait_for(const std::chrono::duration<Rep, Period> &timeout_duration = std::chrono::seconds(0)) {
            // Check if that have started yet and do some busy waiting while they haven't
            if (!notifiers_started()) {
                std::chrono::microseconds current_busy_wait(20);
                std::chrono::seconds max_busy_wait(1);
                do {
                    std::future_status s;
                    if (timeout_duration < current_busy_wait) {
                        return busy_wait_for<timeout_version>(timeout_duration);
                    } else {
                        s = busy_wait_for<timeout_version>(current_busy_wait);
                        if (s == std::future_status::ready) {
                            return s;
                        } else {
                            current_busy_wait *= 3;
                            current_busy_wait /= 2;
                            if (current_busy_wait > max_busy_wait) {
                                current_busy_wait = max_busy_wait;
                            }
                        }
                    }
                } while (not notifiers_started());
            }

            // wait for ready_notified to be set to true by a notifier task
            return wait_for_ready_notification<timeout_version>(timeout_duration);
        }

        /// \brief Sleep and wait for ready_notified to be set to true by a notifier task
        template <class timeout_version, class Rep = std::chrono::seconds::rep,
                  class Period = std::chrono::seconds::period>
        std::future_status wait_for_ready_notification(
            const std::chrono::duration<Rep, Period> &timeout_duration = std::chrono::seconds(0)) const {
            // Create lock to be able to read/wait_for "ready_notified" with the condition variable
            std::unique_lock<std::mutex> lock(ready_notified_mutex);

            // Check if it isn't true yet. Already notified.
            if (ready_notified) {
                return std::future_status::ready;
            }

            // Wait to be notified by another thread
            if constexpr (timeout_version::value) {
                ready_notified_cv.wait_for(lock, timeout_duration, [this]() { return ready_notified; });
            } else {
                while (not ready_notified_cv.wait_for(lock, std::chrono::seconds(1),
                                                      [this]() { return ready_notified; })) {
                    if (is_ready()) {
                        return std::future_status::ready;
                    }
                }
            }

            // convert what we got into a future status
            return ready_notified ? std::future_status::ready : std::future_status::timeout;
        }

        /// \brief Stop the notifiers
        /// We might need to stop the notifiers if the when_any_future is being destroyed, or they might
        /// try to set a condition variable that no longer exists.
        ///
        /// This is something we also have to check when merging two when_any_future objects, because this
        /// creates a new future with a single notifier that needs to replace the old notifiers.
        ///
        /// In practice, all of that should happen very rarely, but things get ugly when it happens.
        ///
        void request_notifiers_stop_and_wait() {
            // Check if we have notifiers
            if (not thread_notifiers_set && not lazy_notifiers_set) {
                return;
            }

            // Set each cancel token to true (separately to cancel as soon as possible)
            std::for_each(notifiers.begin(), notifiers.end(), [](auto &&n) { n.cancel_token.store(true); });

            // Wait for each future<void> notifier
            // - We have to wait for them even if they haven't started, because we don't have that kind of control
            //   over the executor queue. They need to start running, see the cancel token and just give up.
            std::for_each(notifiers.begin(), notifiers.end(), [](auto &&n) {
                if (n.task.valid()) {
                    n.task.wait();
                }
            });

            thread_notifiers_set = false;
        }

        /// \brief Request stop but don't wait
        /// This is useful when moving the object, because we know we won't need the notifiers anymore but
        /// we don't want to waste time waiting for them yet.
        void request_notifiers_stop() {
            // Check if we have notifiers
            if (not thread_notifiers_set && not lazy_notifiers_set) {
                return;
            }

            // Set each cancel token to true (separately to cancel as soon as possible)
            std::for_each(notifiers.begin(), notifiers.end(), [](auto &&n) { n.cancel_token.store(true); });
        }

        void maybe_set_up_lazy_notifiers() { maybe_set_up_notifiers_common<std::true_type>(); }

        void maybe_set_up_thread_notifiers() { maybe_set_up_notifiers_common<std::false_type>(); }

        /// \brief Common functionality to setup notifiers
        ///
        /// The logic for setting notifiers for futures with and without lazy continuations is almost the
        /// same.
        ///
        /// The task is the same but the difference is:
        /// 1) the notification task is a continuation if the future supports continuations, and
        /// 2) the notification task goes into a new new thread if the future does not support continuations.
        ///
        /// @note Unfortunately, we need a new thread an not only a new task in some executor whenever the
        /// task doesn't support continuations because we cannot be sure there's room in the executor for
        /// the notification task.
        ///
        /// This might be counter intuitive, as one could assume there's going to be room for the notifications
        /// as soon as the ongoing tasks are running. However, there are a few situations where this might happen:
        ///
        /// 1) The current tasks we are waiting for have not been launched yet and the executor is busy with
        ///    tasks that need cancellation to stop
        /// 2) Some of the tasks we are waiting for are running and some are enqueued. The running tasks finish
        ///    but we don't hear about it because the enqueued tasks come before the notification.
        /// 3) All tasks we are waiting for have no support for continuations. The executor has no room for the
        ///    notifier because of some parallel tasks happening in the executor and we never hear about a future
        ///    getting ready.
        ///
        /// So although this is an edge case, we cannot assume there's room for the notifications in the
        /// executor.
        ///
        template <class SettingLazyContinuables> void maybe_set_up_notifiers_common() {
            constexpr bool setting_notifiers_as_continuations = SettingLazyContinuables::value;
            constexpr bool setting_notifiers_as_new_threads = !SettingLazyContinuables::value;

            // Never do that more than once. Also check
            if constexpr (setting_notifiers_as_new_threads) {
                if (thread_notifiers_set) {
                    return;
                }
                thread_notifiers_set = true;
            } else {
                if (lazy_notifiers_set) {
                    return;
                }
                lazy_notifiers_set = true;
            }

            // Initialize the variable the notifiers need to set
            // Any of the notifiers will set the same variable
            const bool init_ready = (setting_notifiers_as_continuations && (!thread_notifiers_set)) ||
                                    (setting_notifiers_as_new_threads && (!lazy_notifiers_set));
            if (init_ready) {
                ready_notified = false;
            }

            // Check if there are threads to set up
            const bool no_compatible_futures = (setting_notifiers_as_new_threads && all_lazy_continuable()) ||
                                               (setting_notifiers_as_continuations && lazy_continuable_size() == 0);
            if (no_compatible_futures) {
                return;
            }

            // Function that posts a notifier task to update our common variable that indicates if the task is ready
            auto launch_notifier_task = [&](auto &&future, std::atomic_bool &cancel_token,
                                            std::atomic_bool &start_token) {
                // Launch a task with access to the underlying when_any_future and a cancel token that asks it to stop
                // These threads need to be independent of any executor because the whole system might crash if
                // the executor has no room for them. So this is either a real continuation or a new thread.
                // Direct access to the when_any_future avoid another level of indirection, but makes things
                // a little harder to get right.

                // Create promise the notifier needs to fulfill
                // All we need to know is whether it's over
                promise<void> p;
                auto std_future = p.get_future<futures::future<void>>();
                auto notifier_task = [p = std::move(p), &future, &cancel_token, &start_token, this]() mutable {
                    // The very first thing we need to do is set the start token,
                    // so we never wait for notifiers that aren't running
                    start_token.store(true);

                    // Check if we haven't started too late
                    if (cancel_token.load()) {
                        p.set_value();
                        return;
                    }

                    // If future is ready at the start, just ensure wait_any_future knows about it already
                    // before setting up more state data for this task:
                    // - `is_ready` shouldn't fail in this case because the future is ready, but we haven't
                    //   called `get()` yet, so its state is valid or the whole when_any_future would be invalid
                    //   already.
                    // - We also have to ensure it's valid in case we've moved the when_any_future, and we
                    //   requested this to stop before we even got to this point. In this case, this task
                    //   is accessing the correct location, but we invalidated the future when moving it
                    //   we did so correctly, to avoid blocking unnecessarily when moving.
                    if (!future.valid() || ::futures::is_ready(future)) {
                        std::lock_guard lk(ready_notified_mutex);
                        if (not ready_notified) {
                            ready_notified = true;
                            ready_notified_cv.notify_one();
                        }
                        p.set_value();
                        return;
                    }

                    // Set the max waiting time for each wait_for operation
                    // This task might need to be cancelled, so we cannot wait on the future forever.
                    // So we `wait_for(max_waiting_time)` rather than `wait()` forever.
                    // There are two reasons for this:
                    // - The main when_any object is being destroyed when we found nothing yet.
                    // - Other tasks might have found the ready value, so this task can stop running.
                    // A number of heuristics can be used to adjust this time, but both conditions
                    // are supposed to be relatively rare.
                    const std::chrono::seconds max_waiting_time(1);

                    // Waits for the future to be ready, sleeping most of the time
                    while (future.wait_for(max_waiting_time) != std::future_status::ready) {
                        // But once in a while, check if:
                        // - the main when_any_future has requested this operation to stop
                        //   because it's being destructed (cheaper condition)
                        if (cancel_token.load()) {
                            p.set_value();
                            return;
                        }
                        // - any other tasks haven't set the ready condition yet,
                        // so we can terminate a continuation task we no longer need
                        std::lock_guard lk(ready_notified_mutex);
                        if (ready_notified) {
                            p.set_value();
                            return;
                        }
                    }

                    // We found out about a future that's ready: notify the when_any_future object
                    std::lock_guard lk(ready_notified_mutex);
                    if (not ready_notified) {
                        ready_notified = true;
                        // Notify any thread that might be waiting for this event
                        ready_notified_cv.notify_one();
                    }
                    p.set_value();
                };

                // Create a copiable handle for the notifier task
                auto notifier_task_ptr = std::make_shared<decltype(notifier_task)>(std::move(notifier_task));
                auto executor_handle = [notifier_task_ptr]() { (*notifier_task_ptr)(); };

                // Post the task appropriately
                using future_type = std::decay_t<decltype(future)>;
                // MSVC hack
                constexpr bool internal_lazy_continuable = is_lazy_continuable_v<future_type>;
                constexpr bool internal_setting_lazy = SettingLazyContinuables::value;
                constexpr bool internal_setting_thread = not SettingLazyContinuables::value;
                if constexpr (internal_setting_lazy && internal_lazy_continuable) {
                    // Execute notifier task inline whenever `future` is done
                    future.then(make_inline_executor(), executor_handle);
                } else if constexpr (internal_setting_thread && not internal_lazy_continuable) {
                    // Execute notifier task in a new thread because we don't have the executor context
                    // to be sure. We detach it here but can still control the cancel_token and the future.
                    // This is basically the same as calling std::async and ignoring its std::future because
                    // we already have one set up.
                    std::thread(executor_handle).detach();
                }
                // Return a future we can use to wait for the notifier and ensure it's done
                return std_future;
            };

            // Launch the notification task for each future
            if constexpr (sequence_is_range) {
                if constexpr (is_lazy_continuable_v<typename sequence_type::value_type> &&
                              setting_notifiers_as_new_threads) {
                    return;
                } else if constexpr (not is_lazy_continuable_v<typename sequence_type::value_type> &&
                                     setting_notifiers_as_continuations) {
                    return;
                } else {
                    // Ensure we have one notifier allocated for each task
                    notifiers.resize(size());
                    // For each future in v
                    for (size_t i = 0; i < size(); ++i) {
                        notifiers[i].cancel_token.store(false);
                        notifiers[i].start_token.store(false);
                        // Launch task with reference to this future and its tokens
                        notifiers[i].task =
                            std::move(launch_notifier_task(v[i], notifiers[i].cancel_token, notifiers[i].start_token));
                    }
                }
            } else {
                for_each_paired(v, notifiers, [&](auto &this_future, notifier &n) {
                    using future_type = std::decay_t<decltype(this_future)>;
                    constexpr bool current_is_lazy_continuable_v = is_lazy_continuable_v<future_type>;
                    constexpr bool internal_setting_thread = !SettingLazyContinuables::value;
                    constexpr bool internal_setting_lazy = SettingLazyContinuables::value;
                    if constexpr (current_is_lazy_continuable_v && internal_setting_thread) {
                        return;
                    } else if constexpr ((!current_is_lazy_continuable_v) && internal_setting_lazy) {
                        return;
                    } else {
                        n.cancel_token.store(false);
                        n.start_token.store(false);
                        future<void> tmp = launch_notifier_task(this_future, n.cancel_token, n.start_token);
                        n.task = std::move(tmp);
                    }
                });
            }
        }

      private:
        /// \name Helpers to infer the type for the notifiers
        ///
        /// The array of futures comes with an array of tokens that also allows us to cancel the notifiers
        /// We shouldn't need these tokens because we do expect the notifiers to deliver their promise
        /// before object destruction and we don't usually expect to merge when_any_futures for
        /// which we have started notifiers. This is still a requirement to make sure the notifier
        /// model works.
        /// We could probably use stop tokens instead of atomic_bool in C++20
        /// @{

        /// \brief Type that defines an internal when_any notifier task
        ///
        /// A notifier task notifies the when_any_future of any internal future that is ready.
        ///
        /// We use this notifier type instead of a std::pair because futures need to be moved and
        /// the atomic bools do not, but std::pair conservatively deletes the move constructor
        /// because of atomic_bool.
        struct notifier {
            /// A simple task that notifies us whenever the task is ready
            future<void> task{make_ready_future()};

            /// Cancel the notification task
            std::atomic_bool cancel_token{false};

            /// Notifies this task the notification task has started
            std::atomic_bool start_token{false};

            /// Construct a ready notifier
            notifier() = default;

            /// Construct a notifier from an existing future
            notifier(future<void> &&f, bool c, bool s) : task(std::move(f)), cancel_token(c), start_token(s) {}

            /// Move a notifier
            notifier(notifier &&rhs) noexcept
                : task(std::move(rhs.task)), cancel_token(rhs.cancel_token.load()),
                  start_token(rhs.start_token.load()) {}
        };

#ifdef FUTURES_USE_SMALL_VECTOR
        using notifier_vector = ::futures::small_vector<notifier>;
#else
        // Whenever small::vector in unavailable we use std::vector because boost small_vector
        // couldn't handle move-only notifiers
        using notifier_vector = ::std::vector<notifier>;
#endif
        using notifier_array = std::array<notifier, compile_time_size()>;

        using notifier_sequence_type = std::conditional_t<sequence_is_range, notifier_vector, notifier_array>;
        /// @}

      private:
        /// \brief Internal wait_any_future state
        sequence_type v;

        /// \name Variables for the notifiers to indicate if the future is ready
        /// They indicate if any underlying future has been identified as ready by an auxiliary thread or
        /// as a lazy continuation to an existing future type.
        /// @{
        notifier_sequence_type notifiers;
        bool thread_notifiers_set{false};
        bool lazy_notifiers_set{false};
        bool ready_notified{false};
        mutable std::mutex ready_notified_mutex;
        mutable std::condition_variable ready_notified_cv;
        /// @}
    };

#ifndef FUTURES_DOXYGEN
    /// \name Define when_any_future as a kind of future
    /// @{
    /// Specialization explicitly setting when_any_future<T> as a type of future
    template <typename T> struct is_future<when_any_future<T>> : std::true_type {};

    /// Specialization explicitly setting when_any_future<T> & as a type of future
    template <typename T> struct is_future<when_any_future<T> &> : std::true_type {};

    /// Specialization explicitly setting when_any_future<T> && as a type of future
    template <typename T> struct is_future<when_any_future<T> &&> : std::true_type {};

    /// Specialization explicitly setting const when_any_future<T> as a type of future
    template <typename T> struct is_future<const when_any_future<T>> : std::true_type {};

    /// Specialization explicitly setting const when_any_future<T> & as a type of future
    template <typename T> struct is_future<const when_any_future<T> &> : std::true_type {};
    /// @}
#endif

    namespace detail {
        /// \name Useful traits for when_any
        /// @{

        /// \brief Check if type is a when_any_future as a type
        template <typename> struct is_when_any_future : std::false_type {};
        template <typename Sequence> struct is_when_any_future<when_any_future<Sequence>> : std::true_type {};
        template <typename Sequence> struct is_when_any_future<const when_any_future<Sequence>> : std::true_type {};
        template <typename Sequence> struct is_when_any_future<when_any_future<Sequence> &> : std::true_type {};
        template <typename Sequence> struct is_when_any_future<when_any_future<Sequence> &&> : std::true_type {};
        template <typename Sequence> struct is_when_any_future<const when_any_future<Sequence> &> : std::true_type {};

        /// \brief Check if type is a when_any_future as constant bool
        template <class T> constexpr bool is_when_any_future_v = is_when_any_future<T>::value;

        /// \brief Check if a type can be used in a future disjunction (when_any or operator|| for futures)
        template <class T> using is_valid_when_any_argument = std::disjunction<is_future<T>, std::is_invocable<T>>;
        template <class T> constexpr bool is_valid_when_any_argument_v = is_valid_when_any_argument<T>::value;

        /// \brief Trait to identify valid when_any inputs
        template <class...> struct are_valid_when_any_arguments : std::true_type {};
        template <class B1> struct are_valid_when_any_arguments<B1> : is_valid_when_any_argument<B1> {};
        template <class B1, class... Bn>
        struct are_valid_when_any_arguments<B1, Bn...>
            : std::conditional_t<is_valid_when_any_argument_v<B1>, are_valid_when_any_arguments<Bn...>,
                                 std::false_type> {};
        template <class... Args>
        constexpr bool are_valid_when_any_arguments_v = are_valid_when_any_arguments<Args...>::value;

        /// \subsection Helpers for operator|| on futures, functions and when_any futures

        /// \brief Check if type is a when_any_future with tuples as a sequence type
        template <typename T, class Enable = void> struct is_when_any_tuple_future : std::false_type {};
        template <typename Sequence>
        struct is_when_any_tuple_future<when_any_future<Sequence>, std::enable_if_t<is_tuple_v<Sequence>>>
            : std::true_type {};
        template <class T> constexpr bool is_when_any_tuple_future_v = is_when_any_tuple_future<T>::value;

        /// \brief Check if all template parameters are when_any_future with tuples as a sequence type
        template <class...> struct are_when_any_tuple_futures : std::true_type {};
        template <class B1> struct are_when_any_tuple_futures<B1> : is_when_any_tuple_future<std::decay_t<B1>> {};
        template <class B1, class... Bn>
        struct are_when_any_tuple_futures<B1, Bn...>
            : std::conditional_t<is_when_any_tuple_future_v<std::decay_t<B1>>, are_when_any_tuple_futures<Bn...>,
                                 std::false_type> {};
        template <class... Args>
        constexpr bool are_when_any_tuple_futures_v = are_when_any_tuple_futures<Args...>::value;

        /// \brief Check if type is a when_any_future with a range as a sequence type
        template <typename T, class Enable = void> struct is_when_any_range_future : std::false_type {};
        template <typename Sequence>
        struct is_when_any_range_future<when_any_future<Sequence>, std::enable_if_t<futures::detail::range<Sequence>>>
            : std::true_type {};
        template <class T> constexpr bool is_when_any_range_future_v = is_when_any_range_future<T>::value;

        /// \brief Check if all template parameters are when_any_future with tuples as a sequence type
        template <class...> struct are_when_any_range_futures : std::true_type {};
        template <class B1> struct are_when_any_range_futures<B1> : is_when_any_range_future<B1> {};
        template <class B1, class... Bn>
        struct are_when_any_range_futures<B1, Bn...>
            : std::conditional_t<is_when_any_range_future_v<B1>, are_when_any_range_futures<Bn...>, std::false_type> {};
        template <class... Args>
        constexpr bool are_when_any_range_futures_v = are_when_any_range_futures<Args...>::value;

        /// \brief Constructs a when_any_future that is a concatenation of all when_any_futures in args
        /// It's important to be able to merge when_any_future objects because of operator||
        /// When the user asks for f1 && f2 && f3, we want that to return a single future that
        /// waits for <f1,f2,f3> rather than a future that wait for two futures <f1,<f2,f3>>
        /// \note This function only participates in overload resolution if all types in
        /// std::decay_t<WhenAllFutures>... are specializations of when_any_future with a tuple sequence type
        /// \overload "Merging" a single when_any_future of tuples. Overload provided for symmetry.
        template <class WhenAllFuture, std::enable_if_t<is_when_any_tuple_future_v<WhenAllFuture>, int> = 0>
        decltype(auto) when_any_future_cat(WhenAllFuture &&arg0) {
            return std::forward<WhenAllFuture>(arg0);
        }

        /// \overload Merging a two when_any_future objects of tuples
        template <class WhenAllFuture1, class WhenAllFuture2,
                  std::enable_if_t<are_when_any_tuple_futures_v<WhenAllFuture1, WhenAllFuture2>, int> = 0>
        decltype(auto) when_any_future_cat(WhenAllFuture1 &&arg0, WhenAllFuture2 &&arg1) {
            auto s1 = std::move(std::forward<WhenAllFuture1>(arg0).release());
            auto s2 = std::move(std::forward<WhenAllFuture2>(arg1).release());
            auto s = std::tuple_cat(std::move(s1), std::move(s2));
            return when_any_future(std::move(s));
        }

        /// \overload Merging two+ when_any_future of tuples
        template <class WhenAllFuture1, class... WhenAllFutures,
                  std::enable_if_t<are_when_any_tuple_futures_v<WhenAllFuture1, WhenAllFutures...>, int> = 0>
        decltype(auto) when_any_future_cat(WhenAllFuture1 &&arg0, WhenAllFutures &&...args) {
            auto s1 = std::move(std::forward<WhenAllFuture1>(arg0).release());
            auto s2 = std::move(when_any_future_cat(std::forward<WhenAllFutures>(args)...).release());
            auto s = std::tuple_cat(std::move(s1), std::move(s2));
            return when_any_future(std::move(s));
        }

        /// @}
    } // namespace detail

    /// \brief Create a future object that becomes ready when any of the futures in the range is ready
    ///
    /// This function does not participate in overload resolution unless InputIt's value type (i.e.,
    /// typename std::iterator_traits<InputIt>::value_type) @ref is_future .
    ///
    /// This overload uses a small vector to avoid further allocations for such a simple operation.
    ///
    /// \return @ref when_any_future with all future objects
    template <class InputIt
#ifndef FUTURES_DOXYGEN
              ,
              std::enable_if_t<detail::is_valid_when_any_argument_v<typename std::iterator_traits<InputIt>::value_type>,
                               int> = 0
#endif
              >
    when_any_future<::futures::small_vector<to_future_t<typename std::iterator_traits<InputIt>::value_type>>>
    when_any(InputIt first, InputIt last) {
        // Infer types
        using input_type = std::decay_t<typename std::iterator_traits<InputIt>::value_type>;
        constexpr bool input_is_future = is_future_v<input_type>;
        constexpr bool input_is_invocable = std::is_invocable_v<input_type>;
        static_assert(input_is_future || input_is_invocable);
        using output_future_type = to_future_t<input_type>;
        using sequence_type = ::futures::small_vector<output_future_type>;
        constexpr bool output_is_shared = is_shared_future_v<output_future_type>;

        // Create sequence
        sequence_type v;
        v.reserve(std::distance(first, last));

        // Move or copy the future objects
        if constexpr (input_is_future) {
            if constexpr (output_is_shared) {
                std::copy(first, last, std::back_inserter(v));
            } else /* if constexpr (input_is_future) */ {
                std::move(first, last, std::back_inserter(v));
            }
        } else /* if constexpr (input_is_invocable) */ {
            static_assert(input_is_invocable);
            std::transform(first, last, std::back_inserter(v),
                           [](auto &&f) { return ::futures::async(std::forward<decltype(f)>(f)); });
        }

        return when_any_future<sequence_type>(std::move(v));
    }

    /// \brief Create a future object that becomes ready when any of the futures in the range is ready
    ///
    /// This function does not participate in overload resolution unless every argument is either a (possibly
    /// cv-qualified) std::shared_future or a cv-unqualified std::future.
    ///
    /// \return @ref when_any_future with all future objects
    template <class Range
#ifndef FUTURES_DOXYGEN
              ,
              std::enable_if_t<futures::detail::range<std::decay_t<Range>>, int> = 0
#endif
              >
    when_any_future<::futures::small_vector<
        to_future_t<typename std::iterator_traits<typename std::decay_t<Range>::iterator>::value_type>>>
    when_any(Range &&r) {
        return when_any(std::begin(std::forward<Range>(r)), std::end(std::forward<Range>(r)));
    }

    /// \brief Create a future object that becomes ready when any of the input futures is ready
    ///
    /// This function does not participate in overload resolution unless every argument is either a (possibly
    /// cv-qualified) std::shared_future or a cv-unqualified std::future.
    ///
    /// \return @ref when_any_future with all future objects
    template <class... Futures
#ifndef FUTURES_DOXYGEN
              ,
              std::enable_if_t<detail::are_valid_when_any_arguments_v<Futures...>, int> = 0
#endif
              >
    when_any_future<std::tuple<to_future_t<Futures>...>> when_any(Futures &&...futures) {
        // Infer sequence type
        using sequence_type = std::tuple<to_future_t<Futures>...>;

        // When making the tuple for when_any_future:
        // - futures need to be moved
        // - shared futures need to be copied
        // - lambdas need to be posted
        [[maybe_unused]] constexpr auto move_share_or_post = [](auto &&f) {
            if constexpr (is_shared_future_v<decltype(f)>) {
                return std::forward<decltype(f)>(f);
            } else if constexpr (is_future_v<decltype(f)>) {
                return std::move(std::forward<decltype(f)>(f));
            } else /* if constexpr (std::is_invocable_v<decltype(f)>) */ {
                return ::futures::async(std::forward<decltype(f)>(f));
            }
        };

        // Create sequence (and infer types as we go)
        sequence_type v = std::make_tuple((move_share_or_post(futures))...);

        return when_any_future<sequence_type>(std::move(v));
    }

    /// \brief Operator to create a future object that becomes ready when any of the input futures is ready
    ///
    /// ready operator|| works for futures and functions (which are converted to futures with the default executor)
    /// If the future is a when_any_future itself, then it gets merged instead of becoming a child future
    /// of another when_any_future.
    ///
    /// When the user asks for f1 || f2 || f3, we want that to return a single future that
    /// waits for <f1 || f2 || f3> rather than a future that wait for two futures <f1 || <f2 || f3>>.
    ///
    /// This emulates the usual behavior we expect from other types with operator||.
    ///
    /// Note that this default behaviour is different from when_any(...), which doesn't merge
    /// the when_any_future objects by default, because they are variadic functions and
    /// this intention can be controlled explicitly:
    /// - when_any(f1,f2,f3) -> <f1 || f2 || f3>
    /// - when_any(f1,when_any(f2,f3)) -> <f1 || <f2 || f3>>
    template <
        class T1, class T2
#ifndef FUTURES_DOXYGEN
        ,
        std::enable_if_t<detail::is_valid_when_any_argument_v<T1> && detail::is_valid_when_any_argument_v<T2>, int> = 0
#endif
        >
    auto operator||(T1 &&lhs, T2 &&rhs) {
        constexpr bool first_is_when_any = detail::is_when_any_future_v<T1>;
        constexpr bool second_is_when_any = detail::is_when_any_future_v<T2>;
        constexpr bool both_are_when_any = first_is_when_any && second_is_when_any;
        if constexpr (both_are_when_any) {
            // Merge when all futures with operator||
            return detail::when_any_future_cat(std::forward<T1>(lhs), std::forward<T2>(rhs));
        } else {
            // At least one of the arguments is not a when_any_future.
            // Any such argument might be another future or a function which needs to become a future.
            // Thus, we need a function to maybe convert these functions to futures.
            auto maybe_make_future = [](auto &&f) {
                if constexpr (std::is_invocable_v<decltype(f)> && (!is_future_v<decltype(f)>)) {
                    // Convert to future with the default executor if not a future yet
                    return async(f);
                } else {
                    if constexpr (is_shared_future_v<decltype(f)>) {
                        return std::forward<decltype(f)>(f);
                    } else {
                        return std::move(std::forward<decltype(f)>(f));
                    }
                }
            };
            // Simplest case, join futures in a new when_any_future
            constexpr bool none_are_when_any = not first_is_when_any && not second_is_when_any;
            if constexpr (none_are_when_any) {
                return when_any(maybe_make_future(std::forward<T1>(lhs)), maybe_make_future(std::forward<T2>(rhs)));
            } else if constexpr (first_is_when_any) {
                // If one of them is a when_any_future, then we need to concatenate the results
                // rather than creating a child in the sequence. To concatenate them, the
                // one that is not a when_any_future needs to become one.
                return detail::when_any_future_cat(lhs, when_any(maybe_make_future(std::forward<T2>(rhs))));
            } else /* if constexpr (second_is_when_any) */ {
                return detail::when_any_future_cat(when_any(maybe_make_future(std::forward<T1>(lhs))), rhs);
            }
        }
    }

    /** @} */
} // namespace futures
#endif // FUTURES_WHEN_ANY_H


#endif // FUTURES_FUTURES_H
//
// Created by Alan Freitas on 8/24/21.
//

#ifndef FUTURES_ALGORITHM_H
#define FUTURES_ALGORITHM_H


// #include <futures/algorithm/traits/algorithm_traits.h>
//
// Created by Alan Freitas on 8/20/21.
//

#ifndef FUTURES_ALGORITHM_TRAITS_H
#define FUTURES_ALGORITHM_TRAITS_H

/// \file Identify traits for algorithms, like we do for other types
///
/// The traits help us generate auxiliary algorithm overloads
/// This is somewhat similar to the pattern of traits and algorithms for ranges and views
/// It allows us to get algorithm overloads for free, including default inference of the best execution policies
///
/// \see https://en.cppreference.com/w/cpp/ranges/transform_view
/// \see https://en.cppreference.com/w/cpp/ranges/view
///

#include <execution>

#ifdef __has_include
#if __has_include(<version>)
// #include <version>

#endif
#endif

// #include <futures/algorithm/detail/traits/range/range/concepts.h>


// #include <futures/algorithm/partitioner/partitioner.h>
#ifndef FUTURES_PARTITIONER_H
#define FUTURES_PARTITIONER_H

// #include <thread>


// #include <futures/adaptor/detail/traits/has_get.h>

// #include <futures/algorithm/detail/traits/range/range/concepts.h>


/// \file Default partitioners
/// A partitioner is a light callable object that takes a pair of iterators and returns the
/// middle of the sequence. In particular, it returns an iterator `middle` that forms a subrange
/// `first`/`middle` which the algorithm should solve inline before scheduling the subrange
/// `middle`/`last` in the executor.

namespace futures {
    /** \addtogroup algorithms Algorithms
     *  @{
     */

    /** \addtogroup partitioners Partitioners
     *  @{
     */

    /// \brief The halve partitioner always splits the sequence into two parts of roughly equal size
    ///
    /// The sequence is split up to a minimum grain size.
    /// As a concept, the result from the partitioner is considered a suggestion for parallelization.
    /// For algorithms such as for_each, a partitioner with a very small grain size might be appropriate
    /// if the operation is very expensive.
    /// Some algorithms, such as a binary search, might naturally adjust this suggestion so that the result
    /// makes sense.
    class halve_partitioner {
        std::size_t min_grain_size_;

      public:
        /// \brief Halve partition constructor
        /// \param min_grain_size_ Minimum grain size used to split ranges
        inline explicit halve_partitioner(std::size_t min_grain_size_) : min_grain_size_(min_grain_size_) {}

        /// \brief Split a range of elements
        /// \tparam I Iterator type
        /// \tparam S Sentinel type
        /// \param first First element in range
        /// \param last Last element in range
        /// \return Iterator to point where sequence should be split
        template <typename I, typename S> auto operator()(I first, S last) {
            std::size_t size = std::distance(first, last);
            return (size <= min_grain_size_) ? last : std::next(first, (size + 1) / 2);
        }
    };

    /// \brief A partitioner that splits the ranges until it identifies we are not moving to new threads.
    ///
    /// This partitioner splits the ranges until it identifies we are not moving to new threads.
    /// Apart from that, it behaves as a halve_partitioner, splitting the range up to a minimum grain size.
    class thread_partitioner {
        std::size_t min_grain_size_;
        std::size_t num_threads_;
        std::thread::id last_thread_id_{};

      public:
        explicit thread_partitioner(std::size_t min_grain_size)
            : min_grain_size_(min_grain_size),
              num_threads_(std::max(std::thread::hardware_concurrency(), static_cast<unsigned int>(1))) {}

        template <typename I, typename S> auto operator()(I first, S last) {
            if (num_threads_ <= 1) {
                return last;
            }
            std::thread::id current_thread_id = std::this_thread::get_id();
            const bool threads_changed = current_thread_id != last_thread_id_;
            if (threads_changed) {
                last_thread_id_ = current_thread_id;
                num_threads_ += 1;
                num_threads_ /= 2;
                std::size_t size = std::distance(first, last);
                return (size <= min_grain_size_) ? last : std::next(first, (size + 1) / 2);
            }
            return last;
        }
    };

    /// \brief Default partitioner used by parallel algorithms
    ///
    /// Its type and parameters might change
    using default_partitioner = thread_partitioner;

    /// \brief Determine a reasonable minimum grain size depending on the number of elements in a sequence
    inline std::size_t make_grain_size(std::size_t n) {
        return std::clamp(n / (8 * std::max(std::thread::hardware_concurrency(), static_cast<unsigned int>(1))),
                          size_t(1), size_t(2048));
    }

    /// \brief Create an instance of the default partitioner with a reasonable grain size for @ref n elements
    ///
    /// The default partitioner type and parameters might change
    inline default_partitioner make_default_partitioner(size_t n) { return default_partitioner(make_grain_size(n)); }

    /// \brief Create an instance of the default partitioner with a reasonable grain for the range @ref first , @ref
    /// last
    ///
    /// The default partitioner type and parameters might change
    template <class I, class S,
              std::enable_if_t<futures::detail::input_iterator<I> && futures::detail::sentinel_for<S, I>, int> = 0>
    default_partitioner make_default_partitioner(I first, S last) {
        return make_default_partitioner(std::distance(first, last));
    }

    /// \brief Create an instance of the default partitioner with a reasonable grain for the range @ref r
    ///
    /// The default partitioner type and parameters might change
    template <class R, std::enable_if_t<futures::detail::input_range<R>, int> = 0>
    default_partitioner make_default_partitioner(R &&r) {
        return make_default_partitioner(std::begin(r), std::end(r));
    }

    /// Determine if P is a valid partitioner for the iterator range [I,S]
    template <class T, class I, class S>
    using is_partitioner =
        std::conjunction<std::conditional_t<futures::detail::input_iterator<I>, std::true_type, std::false_type>,
                         std::conditional_t<futures::detail::input_iterator<S>, std::true_type, std::false_type>,
                         std::is_invocable<T, I, S>>;

    /// Determine if P is a valid partitioner for the iterator range [I,S]
    template <class T, class I, class S> constexpr bool is_partitioner_v = is_partitioner<T, I, S>::value;

    /// Determine if P is a valid partitioner for the range R
    template <class T, class R, typename = void> struct is_range_partitioner : std::false_type {};
    template <class T, class R>
    struct is_range_partitioner<T, R, std::enable_if_t<futures::detail::range<R>>>
        : is_partitioner<T, futures::detail::range_common_iterator_t<R>, futures::detail::range_common_iterator_t<R>> {
    };

    template <class T, class R> constexpr bool is_range_partitioner_v = is_range_partitioner<T, R>::value;

    /** @}*/ // \addtogroup partitioners Partitioners
    /** @}*/ // \addtogroup algorithms Algorithms
} // namespace futures

#endif // FUTURES_PARTITIONER_H
// #include <futures/executor/default_executor.h>

// #include <futures/executor/inline_executor.h>


namespace futures {
    /** \addtogroup algorithms Algorithms
     *  @{
     */

    /** \addtogroup execution-policies Execution Policies
     *  @{
     */

    /// Class representing a type for a sequenced_policy tag
    class sequenced_policy {};

    /// Class representing a type for a parallel_policy tag
    class parallel_policy {};

    /// Class representing a type for a parallel_unsequenced_policy tag
    class parallel_unsequenced_policy {};

    /// Class representing a type for an unsequenced_policy tag
    class unsequenced_policy {};

    /// @name Instances of the execution policy types

    /// \brief Tag used in algorithms for a sequenced_policy
    inline constexpr sequenced_policy seq{};

    /// \brief Tag used in algorithms for a parallel_policy
    inline constexpr parallel_policy par{};

    /// \brief Tag used in algorithms for a parallel_unsequenced_policy
    inline constexpr parallel_unsequenced_policy par_unseq{};

    /// \brief Tag used in algorithms for an unsequenced_policy
    inline constexpr unsequenced_policy unseq{};

    /// \brief Checks whether T is a standard or implementation-defined execution policy type.
    template <class T>
    struct is_execution_policy
        : std::disjunction<std::is_same<T, sequenced_policy>, std::is_same<T, parallel_policy>,
                           std::is_same<T, parallel_unsequenced_policy>, std::is_same<T, unsequenced_policy>> {};

    /// \brief Checks whether T is a standard or implementation-defined execution policy type.
    template <class T> inline constexpr bool is_execution_policy_v = is_execution_policy<T>::value;

    /// \brief Make an executor appropriate to a given policy and a pair of iterators
    /// This depends, of course, of the default executors we have available and
    template <class E, class I, class S
#ifndef FUTURES_DOXYGEN
              ,
              std::enable_if_t<!is_executor_v<E> && is_execution_policy_v<E> && futures::detail::input_iterator<I> &&
                                   futures::detail::sentinel_for<S, I>,
                               int> = 0
#endif
              >
    constexpr decltype(auto) make_policy_executor() {
        if constexpr (!std::is_same_v<E, sequenced_policy>) {
            return make_default_executor();
        } else {
            return make_inline_executor();
        }
    }

    /** @}*/

    /** \addtogroup algorithm-traits Algorithm Traits
     *  @{
     */

    namespace detail {

        /// \brief CRTP class with the overloads for classes that look for elements in a sequence with an unary function
        /// This includes algorithms such as for_each, any_of, all_of, ...
        template <class Derived> class unary_invoke_algorithm_functor {
          public:
            template <class E, class P, class I, class S, class Fun
#ifndef FUTURES_DOXYGEN
                      ,
                      std::enable_if_t<is_executor_v<E> && is_partitioner_v<P, I, S> &&
                                           futures::detail::input_iterator<I> && futures::detail::sentinel_for<S, I> &&
                                           futures::detail::indirectly_unary_invocable<Fun, I> &&
                                           std::is_copy_constructible_v<Fun>,
                                       int> = 0>
#endif
            decltype(auto) operator()(const E &ex, P p, I first, S last, Fun f) const {
                return Derived().run(ex, std::forward<P>(p), first, last, f);
            }

            /// \overload execution policy instead of executor
            /// we can't however, count on std::is_execution_policy being defined
            template <class E, class P, class I, class S, class Fun
#ifndef FUTURES_DOXYGEN
                      ,
                      std::enable_if_t<!is_executor_v<E> && is_execution_policy_v<E> && is_partitioner_v<P, I, S> &&
                                           futures::detail::input_iterator<I> && futures::detail::sentinel_for<S, I> &&
                                           futures::detail::indirectly_unary_invocable<Fun, I> &&
                                           std::is_copy_constructible_v<Fun>,
                                       int> = 0
#endif
                      >
            decltype(auto) operator()(const E &, P p, I first, S last, Fun f) const {
                return Derived().operator()(make_policy_executor<E, I, S>(), std::forward<P>(p), first, last, f);
            }

            /// \overload Ranges
            template <
                class E, class P, class R, class Fun
#ifndef FUTURES_DOXYGEN
                ,
                std::enable_if_t<(is_executor_v<E> || is_execution_policy_v<E>)&&is_range_partitioner_v<P, R> &&
                                     futures::detail::input_range<R> &&
                                     futures::detail::indirectly_unary_invocable<Fun, futures::detail::iterator_t<R>> &&
                                     std::is_copy_constructible_v<Fun>,
                                 int> = 0
#endif
                >
            decltype(auto) operator()(const E &ex, P p, R &&r, Fun f) const {
                return Derived().operator()(ex, std::forward<P>(p), std::begin(r), std::end(r), std::move(f));
            }

            /// \overload Iterators / default parallel executor
            template <class P, class I, class S, class Fun
#ifndef FUTURES_DOXYGEN
                      ,
                      std::enable_if_t<is_partitioner_v<P, I, S> && futures::detail::input_iterator<I> &&
                                           futures::detail::sentinel_for<S, I> &&
                                           futures::detail::indirectly_unary_invocable<Fun, I> &&
                                           std::is_copy_constructible_v<Fun>,
                                       int> = 0
#endif
                      >
            decltype(auto) operator()(P p, I first, S last, Fun f) const {
                return Derived().operator()(make_default_executor(), std::forward<P>(p), first, last, std::move(f));
            }

            /// \overload Ranges / default parallel executor
            template <
                class P, class R, class Fun
#ifndef FUTURES_DOXYGEN
                ,
                std::enable_if_t<is_range_partitioner_v<P, R> && futures::detail::input_range<R> &&
                                     futures::detail::indirectly_unary_invocable<Fun, futures::detail::iterator_t<R>> &&
                                     std::is_copy_constructible_v<Fun>,
                                 int> = 0
#endif
                >
            decltype(auto) operator()(P p, R &&r, Fun f) const {
                return Derived().operator()(make_default_executor(), std::forward<P>(p), std::begin(r), std::end(r),
                                            std::move(f));
            }

            /// \overload Iterators / default partitioner
            template <class E, class I, class S, class Fun
#ifndef FUTURES_DOXYGEN
                      ,
                      std::enable_if_t<
                          (is_executor_v<E> || is_execution_policy_v<E>)&&futures::detail::input_iterator<I> &&
                              futures::detail::sentinel_for<S, I> &&
                              futures::detail::indirectly_unary_invocable<Fun, I> && std::is_copy_constructible_v<Fun>,
                          int> = 0
#endif
                      >
            decltype(auto) operator()(const E &ex, I first, S last, Fun f) const {
                return Derived().operator()(ex, make_default_partitioner(first, last), first, last, std::move(f));
            }

            /// \overload Ranges / default partitioner
            template <
                class E, class R, class Fun
#ifndef FUTURES_DOXYGEN
                ,
                std::enable_if_t<(is_executor_v<E> || is_execution_policy_v<E>)&&futures::detail::input_range<R> &&
                                     futures::detail::indirectly_unary_invocable<Fun, futures::detail::iterator_t<R>> &&
                                     std::is_copy_constructible_v<Fun>,
                                 int> = 0
#endif
                >
            decltype(auto) operator()(const E &ex, R &&r, Fun f) const {
                return Derived().operator()(ex, make_default_partitioner(std::forward<R>(r)), std::begin(r),
                                            std::end(r), std::move(f));
            }

            /// \overload Iterators / default executor / default partitioner
            template <class I, class S, class Fun
#ifndef FUTURES_DOXYGEN
                      ,
                      std::enable_if_t<futures::detail::input_iterator<I> && futures::detail::sentinel_for<S, I> &&
                                           futures::detail::indirectly_unary_invocable<Fun, I> &&
                                           std::is_copy_constructible_v<Fun>,
                                       int> = 0
#endif
                      >
            decltype(auto) operator()(I first, S last, Fun f) const {
                return Derived().operator()(make_default_executor(), make_default_partitioner(first, last), first, last,
                                            std::move(f));
            }

            /// \overload Ranges / default executor / default partitioner
            template <
                class R, class Fun
#ifndef FUTURES_DOXYGEN
                ,
                std::enable_if_t<futures::detail::input_range<R> &&
                                     futures::detail::indirectly_unary_invocable<Fun, futures::detail::iterator_t<R>> &&
                                     std::is_copy_constructible_v<Fun>,
                                 int> = 0
#endif
                >
            decltype(auto) operator()(R &&r, Fun f) const {
                return Derived().operator()(make_default_executor(), make_default_partitioner(r), std::begin(r),
                                            std::end(r), std::move(f));
            }

            /// \brief Struct holding the overload for the full async variant of all algorithms
            ///
            /// The async object represents the exact same overloads for the algorithms, with only
            /// two relevant differences:
            /// 1) the first task in the algorithm should not happen in the inline executor
            /// 2) the function returns future<...> on which we can wait for the algorithm to finish
            struct async_functor {

            } async;
        };

        /// \brief CRTP class with the overloads for classes that look for elements in a sequence with an unary function
        /// This includes algorithms such as for_each, any_of, all_of, ...
        template <class Derived> class value_cmp_algorithm_functor {
          public:
            template <
                class E, class P, class I, class S, class T
#ifndef FUTURES_DOXYGEN
                ,
                std::enable_if_t<is_executor_v<E> && is_partitioner_v<P, I, S> && futures::detail::input_iterator<I> &&
                                     futures::detail::sentinel_for<S, I> &&
                                     futures::detail::indirectly_binary_invocable_<futures::detail::equal_to, T *, I>,
                                 int> = 0
#endif
                >
            decltype(auto) operator()(const E &ex, P p, I first, S last, T f) const {
                return Derived().run(ex, std::forward<P>(p), first, last, f);
            }

            /// \overload execution policy instead of executor
            /// we can't however, count on std::is_execution_policy being defined
            template <
                class E, class P, class I, class S, class T
#ifndef FUTURES_DOXYGEN
                ,
                std::enable_if_t<!is_executor_v<E> && is_execution_policy_v<E> && is_partitioner_v<P, I, S> &&
                                     futures::detail::input_iterator<I> && futures::detail::sentinel_for<S, I> &&
                                     futures::detail::indirectly_binary_invocable_<futures::detail::equal_to, T *, I>,
                                 int> = 0
#endif
                >
            decltype(auto) operator()(const E &, P p, I first, S last, T f) const {
                return Derived().operator()(make_policy_executor<E, I, S>(), std::forward<P>(p), first, last, f);
            }

            /// \overload Ranges
            template <class E, class P, class R, class T
#ifndef FUTURES_DOXYGEN
                      ,
                      std::enable_if_t<(is_executor_v<E> || is_execution_policy_v<E>)&&is_range_partitioner_v<P, R> &&
                                           futures::detail::input_range<R> &&
                                           futures::detail::indirectly_binary_invocable_<
                                               futures::detail::equal_to, T *, futures::detail::iterator_t<R>> &&
                                           std::is_copy_constructible_v<T>,
                                       int> = 0
#endif
                      >
            decltype(auto) operator()(const E &ex, P p, R &&r, T f) const {
                return Derived().operator()(ex, std::forward<P>(p), std::begin(r), std::end(r), std::move(f));
            }

            /// \overload Iterators / default parallel executor
            template <
                class P, class I, class S, class T
#ifndef FUTURES_DOXYGEN
                ,
                std::enable_if_t<is_partitioner_v<P, I, S> && futures::detail::input_iterator<I> &&
                                     futures::detail::sentinel_for<S, I> &&
                                     futures::detail::indirectly_binary_invocable_<futures::detail::equal_to, T *, I> &&
                                     std::is_copy_constructible_v<T>,
                                 int> = 0
#endif
                >
            decltype(auto) operator()(P p, I first, S last, T f) const {
                return Derived().operator()(make_default_executor(), std::forward<P>(p), first, last, std::move(f));
            }

            /// \overload Ranges / default parallel executor
            template <class P, class R, class T
#ifndef FUTURES_DOXYGEN
                      ,
                      std::enable_if_t<is_range_partitioner_v<P, R> && futures::detail::input_range<R> &&
                                           futures::detail::indirectly_binary_invocable_<
                                               futures::detail::equal_to, T *, futures::detail::iterator_t<R>> &&
                                           std::is_copy_constructible_v<T>,
                                       int> = 0
#endif
                      >
            decltype(auto) operator()(P p, R &&r, T f) const {
                return Derived().operator()(make_default_executor(), std::forward<P>(p), std::begin(r), std::end(r),
                                            std::move(f));
            }

            /// \overload Iterators / default partitioner
            template <
                class E, class I, class S, class T
#ifndef FUTURES_DOXYGEN
                ,
                std::enable_if_t<(is_executor_v<E> || is_execution_policy_v<E>)&&futures::detail::input_iterator<I> &&
                                     futures::detail::sentinel_for<S, I> &&
                                     futures::detail::indirectly_binary_invocable_<futures::detail::equal_to, T *, I>,
                                 int> = 0
#endif
                >
            decltype(auto) operator()(const E &ex, I first, S last, T f) const {
                return Derived().operator()(ex, make_default_partitioner(first, last), first, last, std::move(f));
            }

            /// \overload Ranges / default partitioner
            template <
                class E, class R, class T
#ifndef FUTURES_DOXYGEN
                ,
                std::enable_if_t<(is_executor_v<E> || is_execution_policy_v<E>)&&futures::detail::input_range<R> &&
                                     futures::detail::indirectly_binary_invocable_<futures::detail::equal_to, T *,
                                                                                   futures::detail::iterator_t<R>> &&
                                     std::is_copy_constructible_v<T>,
                                 int> = 0
#endif
                >
            decltype(auto) operator()(const E &ex, R &&r, T f) const {
                return Derived().operator()(ex, make_default_partitioner(std::forward<R>(r)), std::begin(r),
                                            std::end(r), std::move(f));
            }

            /// \overload Iterators / default executor / default partitioner
            template <
                class I, class S, class T
#ifndef FUTURES_DOXYGEN
                ,
                std::enable_if_t<futures::detail::input_iterator<I> && futures::detail::sentinel_for<S, I> &&
                                     futures::detail::indirectly_binary_invocable_<futures::detail::equal_to, T *, I>,
                                 int> = 0
#endif
                >
            decltype(auto) operator()(I first, S last, T f) const {
                return Derived().operator()(make_default_executor(), make_default_partitioner(first, last), first, last,
                                            std::move(f));
            }

            /// \overload Ranges / default executor / default partitioner
            template <class R, class T
#ifndef FUTURES_DOXYGEN
                      ,
                      std::enable_if_t<futures::detail::input_range<R> &&
                                           futures::detail::indirectly_binary_invocable_<
                                               futures::detail::equal_to, T *, futures::detail::iterator_t<R>> &&
                                           std::is_copy_constructible_v<T>,
                                       int> = 0
#endif
                      >
            decltype(auto) operator()(R &&r, T f) const {
                return Derived().operator()(make_default_executor(), make_default_partitioner(r), std::begin(r),
                                            std::end(r), std::move(f));
            }
        };
    } // namespace detail
    /** @}*/
    /** @}*/
} // namespace futures

#endif // FUTURES_ALGORITHM_TRAITS_H


// #include <futures/algorithm/all_of.h>

//
// Created by Alan Freitas on 8/16/21.
//

#ifndef FUTURES_ALL_OF_H
#define FUTURES_ALL_OF_H

// #include <execution>

#include <variant>

// #include <futures/algorithm/detail/traits/range/range/concepts.h>


// #include <futures/algorithm/traits/algorithm_traits.h>

// #include <futures/algorithm/detail/try_async.h>
//
// Copyright (c) alandefreitas 12/3/21.
// See accompanying file LICENSE
//

#ifndef FUTURES_TRY_ASYNC_H
#define FUTURES_TRY_ASYNC_H

// #include <futures/futures/async.h>

// #include <futures/futures/detail/throw_exception.h>


namespace futures {
    /** \addtogroup futures Futures
     *  @{
     */

    /// \brief Attempts to schedule a function
    ///
    /// This function attempts to schedule a function, and returns 3 objects:
    /// - The future for the task itself
    /// - A future that indicates if the task got scheduled yet
    /// - A token for canceling the task
    ///
    /// This is mostly useful for recursive tasks, where there might not be room in the executor for
    /// a new task, as depending on recursive tasks for which there is no room is the executor might
    /// block execution.
    ///
    /// Although this is a general solution to allow any executor in the algorithms, executor traits
    /// to identify capacity in executor are much more desirable.
    ///
    template <typename Executor, typename Function, typename... Args
#ifndef FUTURES_DOXYGEN
              ,
              std::enable_if_t<detail::is_valid_async_input_v<Executor, Function, Args...>, int> = 0
#endif
              >
    decltype(auto) try_async(const Executor &ex, Function &&f, Args &&...args) {
        // Communication flags
        std::promise<void> started_token;
        std::future<void> started = started_token.get_future();
        stop_source cancel_source;

        // Wrap the task in a lambda that sets and checks the flags
        auto do_task = [p = std::move(started_token), cancel_token = cancel_source.get_token(),
                        f](Args &&...args) mutable {
            p.set_value();
            if (cancel_token.stop_requested()) {
                detail::throw_exception<std::runtime_error>("task cancelled");
            }
            return std::invoke(f, std::forward<Args>(args)...);
        };

        // Make it copy constructable
        auto do_task_ptr = std::make_shared<decltype(do_task)>(std::move(do_task));
        auto do_task_handle = [do_task_ptr](Args &&...args) { return (*do_task_ptr)(std::forward<Args>(args)...); };

        // Launch async
        using internal_result_type = std::decay_t<decltype(std::invoke(f, std::forward<Args>(args)...))>;
        cfuture<internal_result_type> rhs = async(ex, do_task_handle, std::forward<Args>(args)...);

        // Return future and tokens
        return std::make_tuple(std::move(rhs), std::move(started), cancel_source);
    }


    /** @} */
}


#endif // FUTURES_TRY_ASYNC_H

// #include <futures/algorithm/partitioner/partitioner.h>

// #include <futures/futures.h>


namespace futures {
    /** \addtogroup algorithms Algorithms
     *  @{
     */

    /// \brief Functor representing the overloads for the @ref all_of function
    class all_of_functor : public detail::unary_invoke_algorithm_functor<all_of_functor> {
      public:
        /// \brief Complete overload of the all_of algorithm
        /// \tparam E Executor type
        /// \tparam P Partitioner type
        /// \tparam I Iterator type
        /// \tparam S Sentinel iterator type
        /// \tparam Fun Function type
        /// \param ex Executor
        /// \param p Partitioner
        /// \param first Iterator to first element in the range
        /// \param last Iterator to (last + 1)-th element in the range
        /// \param f Function
        /// \brief function template \c all_of
        template <class E, class P, class I, class S, class Fun
#ifndef FUTURES_DOXYGEN
                  ,
                  std::enable_if_t<is_executor_v<E> && is_partitioner_v<P, I, S> &&
                                       futures::detail::input_iterator<I> && futures::detail::sentinel_for<S, I> &&
                                       futures::detail::indirectly_unary_invocable<Fun, I> &&
                                       std::is_copy_constructible_v<Fun>,
                                   int> = 0
#endif
                  >
        bool run(const E &ex, P p, I first, S last, Fun f) const {
            auto middle = p(first, last);
            if (middle == last || std::is_same_v<E, inline_executor> || futures::detail::forward_iterator<I>) {
                return std::all_of(first, last, f);
            }

            // Run all_of on rhs: [middle, last]
            auto [rhs, rhs_started, rhs_cancel] = try_async(ex, [=]() { return operator()(ex, p, middle, last, f); });

            // Run all_of on lhs: [first, middle]
            bool lhs = operator()(ex, p, first, middle, f);

            // Wait for rhs
            if (is_ready(rhs_started)) {
                return lhs && rhs.get();
            } else {
                rhs_cancel.request_stop();
                rhs.detach();
                if (!lhs) {
                    return false;
                } else {
                    return operator()(make_inline_executor(), p, middle, last, f);
                }
            }
        }
    };

    /// \brief Checks if a predicate is true for all the elements in a range
    inline constexpr all_of_functor all_of;

    /** @}*/ // \addtogroup algorithms Algorithms
} // namespace futures

#endif // FUTURES_ALL_OF_H

// #include <futures/algorithm/any_of.h>
//
// Created by Alan Freitas on 8/16/21.
//

#ifndef FUTURES_ANY_OF_H
#define FUTURES_ANY_OF_H

// #include <execution>

// #include <variant>


// #include <futures/algorithm/detail/traits/range/range/concepts.h>


// #include <futures/futures.h>

// #include <futures/algorithm/traits/algorithm_traits.h>

// #include <futures/algorithm/detail/try_async.h>

// #include <futures/algorithm/partitioner/partitioner.h>


namespace futures {
    /** \addtogroup algorithms Algorithms
     *  @{
     */

    /// \brief Functor representing the overloads for the @ref any_of function
    class any_of_functor : public detail::unary_invoke_algorithm_functor<any_of_functor> {
      public:
        /// \brief Complete overload of the any_of algorithm
        /// \tparam E Executor type
        /// \tparam P Partitioner type
        /// \tparam I Iterator type
        /// \tparam S Sentinel iterator type
        /// \tparam Fun Function type
        /// \param ex Executor
        /// \param p Partitioner
        /// \param first Iterator to first element in the range
        /// \param last Iterator to (last + 1)-th element in the range
        /// \param f Function
        /// \brief function template \c any_of
        template <class E, class P, class I, class S, class Fun
#ifndef FUTURES_DOXYGEN
                  ,
                  std::enable_if_t<is_executor_v<E> && is_partitioner_v<P, I, S> && futures::detail::input_iterator<I> &&
                                       futures::detail::sentinel_for<S, I> && futures::detail::indirectly_unary_invocable<Fun, I> &&
                                       std::is_copy_constructible_v<Fun>,
                                   int> = 0
#endif
                  >
        bool run(const E &ex, P p, I first, S last, Fun f) const {
            auto middle = p(first, last);
            if (middle == last || std::is_same_v<E, inline_executor> || futures::detail::forward_iterator<I>) {
                return std::any_of(first, last, f);
            }

            // Run any_of on rhs: [middle, last]
            auto [rhs, rhs_started, rhs_cancel] = try_async(ex, [=]() { return operator()(ex, p, middle, last, f); });

            // Run any_of on lhs: [first, middle]
            bool lhs = operator()(ex, p, first, middle, f);

            // Wait for rhs
            if (is_ready(rhs_started)) {
                return lhs || rhs.get();
            } else {
                rhs_cancel.request_stop();
                rhs.detach();
                if (lhs) {
                    return true;
                } else {
                    return operator()(make_inline_executor(), p, middle, last, f);
                }
            }
        }
    };

    /// \brief Checks if a predicate is true for any of the elements in a range
    inline constexpr any_of_functor any_of;

    /** @}*/ // \addtogroup algorithms Algorithms
} // namespace futures

#endif // FUTURES_ANY_OF_H

// #include <futures/algorithm/none_of.h>
//
// Created by Alan Freitas on 8/16/21.
//

#ifndef FUTURES_NONE_OF_H
#define FUTURES_NONE_OF_H

// #include <execution>

// #include <variant>


// #include <futures/algorithm/detail/traits/range/range/concepts.h>


// #include <futures/futures.h>

// #include <futures/algorithm/traits/algorithm_traits.h>

// #include <futures/algorithm/detail/try_async.h>

// #include <futures/algorithm/partitioner/partitioner.h>


namespace futures {
    /** \addtogroup algorithms Algorithms
     *  @{
     */

    /// \brief Functor representing the overloads for the @ref none_of function
    class none_of_functor : public detail::unary_invoke_algorithm_functor<none_of_functor> {
      public:
        /// \brief Complete overload of the none_of algorithm
        /// \tparam E Executor type
        /// \tparam P Partitioner type
        /// \tparam I Iterator type
        /// \tparam S Sentinel iterator type
        /// \tparam Fun Function type
        /// \param ex Executor
        /// \param p Partitioner
        /// \param first Iterator to first element in the range
        /// \param last Iterator to (last + 1)-th element in the range
        /// \param f Function
        /// \brief function template \c none_of
        template <class E, class P, class I, class S, class Fun,
                  std::enable_if_t<is_executor_v<E> && is_partitioner_v<P, I, S> && futures::detail::input_iterator<I> &&
                                       futures::detail::sentinel_for<S, I> && futures::detail::indirectly_unary_invocable<Fun, I> &&
                                       std::is_copy_constructible_v<Fun>,
                                   int> = 0>
        bool run(const E &ex, P p, I first, S last, Fun f) const {
            auto middle = p(first, last);
            if (middle == last || std::is_same_v<E, inline_executor> || futures::detail::forward_iterator<I>) {
                return std::none_of(first, last, f);
            }

            // Run none_of on rhs: [middle, last]
            auto [rhs, rhs_started, rhs_cancel] = try_async(ex, [=]() { return operator()(ex, p, middle, last, f); });

            // Run none_of on lhs: [first, middle]
            bool lhs = operator()(ex, p, first, middle, f);

            // Wait for rhs
            if (is_ready(rhs_started)) {
                return lhs && rhs.get();
            } else {
                rhs_cancel.request_stop();
                rhs.detach();
                if (!lhs) {
                    return false;
                } else {
                    return operator()(make_inline_executor(), p, middle, last, f);
                }
            }
        }
    };

    /// \brief Checks if a predicate is true for none of the elements in a range
    inline constexpr none_of_functor none_of;

    /** @}*/ // \addtogroup algorithms Algorithms
} // namespace futures

#endif // FUTURES_NONE_OF_H

// #include <futures/algorithm/for_each.h>
//
// Created by Alan Freitas on 8/16/21.
//

#ifndef FUTURES_FOR_EACH_H
#define FUTURES_FOR_EACH_H

// #include <execution>

// #include <variant>


// #include <futures/algorithm/partitioner/partitioner.h>

// #include <futures/algorithm/traits/algorithm_traits.h>

// #include <futures/algorithm/detail/traits/range/range/concepts.h>

// #include <futures/algorithm/detail/try_async.h>

// #include <futures/futures.h>

// #include <futures/futures/detail/empty_base.h>


namespace futures {
    /** \addtogroup algorithms Algorithms
     *  @{
     */

    /// \brief Functor representing the overloads for the @ref for_each function
    class for_each_functor : public detail::unary_invoke_algorithm_functor<for_each_functor> {
      public:
        // Let only unary_invoke_algorithm_functor access the primary sort function template
        friend detail::unary_invoke_algorithm_functor<for_each_functor>;

      private:
        /// \brief Internal class that takes care of the sorting tasks and its incomplete tasks
        ///
        /// If we could make sure no executors would ever block, recursion wouldn't be a problem, and
        /// we wouldn't need this class. In fact, this is what most related libraries do, counting on
        /// the executor to be some kind of work stealing thread pool.
        ///
        /// However, we cannot count on that, or these algorithms wouldn't work for many executors in which
        /// we are interested, such as an io_context or a thread pool that doesn't steel work (like asio's).
        /// So we need to separate the process of launching the tasks from the process of waiting for them.
        /// Fortunately, we can count that most executors wouldn't need this blocking procedure very often,
        /// because that's what usually make them useful executors. We also assume that, unlike in the other
        /// applications, the cost of this reading lock is trivial compared to the cost of the whole
        /// procedure.
        ///
        template <class Executor> class sorter : public detail::maybe_empty<Executor> {
          public:
            explicit sorter(const Executor &ex) : detail::maybe_empty<Executor>(ex) {}

            /// \brief Get executor from the base class as a function for convenience
            const Executor &ex() const { return detail::maybe_empty<Executor>::get(); }

            /// \brief Get executor from the base class as a function for convenience
            Executor &ex() { return detail::maybe_empty<Executor>::get(); }

            template <class P, class I, class S, class Fun> void launch_sort_tasks(P p, I first, S last, Fun f) {
                auto middle = p(first, last);
                const bool too_small = middle == last;
                constexpr bool cannot_parallelize =
                    std::is_same_v<Executor, inline_executor> || futures::detail::forward_iterator<I>;
                if (too_small || cannot_parallelize) {
                    std::for_each(first, last, f);
                } else {
                    // Run for_each on rhs: [middle, last]
                    cfuture<void> rhs_task =
                        futures::async(ex(), [this, p, middle, last, f] { launch_sort_tasks(p, middle, last, f); });

                    // Run for_each on lhs: [first, middle]
                    launch_sort_tasks(p, first, middle, f);

                    // When lhs is ready, we check on rhs
                    if (!is_ready(rhs_task)) {
                        // Put rhs_task on the list of tasks we need to await later
                        // This ensures we only deal with the task queue if we really need to
                        std::unique_lock write_lock(tasks_mutex_);
                        tasks_.emplace_back(std::move(rhs_task));
                    }
                }
            }

            /// \brief Wait for all tasks to finish
            ///
            /// This might sound like it should be as simple as a when_all(tasks_).
            /// However, while we wait for some tasks here, the running tasks might be enqueuing more tasks,
            /// so we still need a read lock here. The number of times this happens and the relative cost
            /// of this operation should still be negligible, compared to other applications.
            ///
            /// \return `true` if we had to wait for any tasks
            bool wait_for_sort_tasks() {
                tasks_mutex_.lock_shared();
                bool waited_any = false;
                while (!tasks_.empty()) {
                    tasks_mutex_.unlock_shared();
                    tasks_mutex_.lock();
                    futures::small_vector<futures::cfuture<void>> stolen_tasks(std::make_move_iterator(tasks_.begin()),
                                                                               std::make_move_iterator(tasks_.end()));
                    tasks_.clear();
                    tasks_mutex_.unlock();
                    when_all(stolen_tasks).wait();
                    waited_any = true;
                }
                return waited_any;
            }

            template <class P, class I, class S, class Fun> void sort(P p, I first, S last, Fun f) {
                launch_sort_tasks(p, first, last, f);
                wait_for_sort_tasks();
            }

          private:
            futures::small_vector<futures::cfuture<void>> tasks_;
            std::shared_mutex tasks_mutex_;
        };

        /// \brief Complete overload of the for_each algorithm
        /// \tparam E Executor type
        /// \tparam P Partitioner type
        /// \tparam I Iterator type
        /// \tparam S Sentinel iterator type
        /// \tparam Fun Function type
        /// \param ex Executor
        /// \param p Partitioner
        /// \param first Iterator to first element in the range
        /// \param last Iterator to (last + 1)-th element in the range
        /// \param f Function
        /// \brief function template \c for_each
        template <class FullAsync = std::false_type, class E, class P, class I, class S, class Fun
#ifndef FUTURES_DOXYGEN
                  ,
                  std::enable_if_t<is_executor_v<E> && is_partitioner_v<P, I, S> &&
                                       futures::detail::input_iterator<I> && futures::detail::sentinel_for<S, I> &&
                                       futures::detail::indirectly_unary_invocable<Fun, I> &&
                                       std::is_copy_constructible_v<Fun>,
                                   int> = 0
#endif
                  >
        auto run(const E &ex, P p, I first, S last, Fun f) const {
            if constexpr (FullAsync::value) {
                // If full async, launching the tasks and solving small tasks also happen asynchronously
                return async(ex, [ex, p, first, last, f]() { sorter<E>(ex).sort(p, first, last, f); });
            } else {
                // Else, we try to solve small tasks and launching other tasks if it's worth splitting the problem
                sorter<E>(ex).sort(p, first, last, f);
            }
        }
    };

    /// \brief Applies a function to a range of elements
    inline constexpr for_each_functor for_each;

    /** @}*/ // \addtogroup algorithms Algorithms
} // namespace futures

#endif // FUTURES_FOR_EACH_H

// #include <futures/algorithm/find.h>
//
// Created by Alan Freitas on 8/16/21.
//

#ifndef FUTURES_FIND_H
#define FUTURES_FIND_H

// #include <execution>

// #include <variant>


// #include <futures/algorithm/detail/traits/range/range/concepts.h>


// #include <futures/futures.h>

// #include <futures/algorithm/traits/algorithm_traits.h>

// #include <futures/algorithm/detail/try_async.h>

// #include <futures/algorithm/partitioner/partitioner.h>


namespace futures {
    /** \addtogroup algorithms Algorithms
     *  @{
     */

    /// \brief Functor representing the overloads for the @ref find function
    class find_functor : public detail::value_cmp_algorithm_functor<find_functor> {
      public:
        /// \brief Complete overload of the find algorithm
        /// \tparam E Executor type
        /// \tparam P Partitioner type
        /// \tparam I Iterator type
        /// \tparam S Sentinel iterator type
        /// \tparam T Value to compare
        /// \param ex Executor
        /// \param p Partitioner
        /// \param first Iterator to first element in the range
        /// \param last Iterator to (last + 1)-th element in the range
        /// \param value Value
        /// \brief function template \c find
        template <class E, class P, class I, class S, class T,
                  std::enable_if_t<is_executor_v<E> && is_partitioner_v<P, I, S> && futures::detail::input_iterator<I> &&
                                       futures::detail::sentinel_for<S, I> &&
                                       futures::detail::indirectly_binary_invocable_<futures::detail::equal_to, T *, I>,
                                   int> = 0>
        I run(const E &ex, P p, I first, S last, const T &v) const {
            auto middle = p(first, last);
            if (middle == last || std::is_same_v<E, inline_executor> || futures::detail::forward_iterator<I>) {
                return std::find(first, last, v);
            }

            // Run find on rhs: [middle, last]
            auto [rhs, rhs_started, rhs_cancel] = try_async(ex, [=]() { return operator()(ex, p, middle, last, v); });

            // Run find on lhs: [first, middle]
            I lhs = operator()(ex, p, first, middle, v);

            // Wait for rhs
            if (is_ready(rhs_started)) {
                rhs.wait();
                if (lhs != middle) {
                    return lhs;
                } else {
                    return rhs.get();
                }
            } else {
                rhs_cancel.request_stop();
                rhs.detach();
                if (lhs != middle) {
                    return lhs;
                } else {
                    return operator()(make_inline_executor(), p, middle, last, v);
                }
            }
        }
    };

    /// \brief Finds the first element equal to another element
    inline constexpr find_functor find;

    /** @}*/ // \addtogroup algorithms Algorithms
} // namespace futures

#endif // FUTURES_FIND_H

// #include <futures/algorithm/find_if.h>
//
// Created by Alan Freitas on 8/16/21.
//

#ifndef FUTURES_FIND_IF_H
#define FUTURES_FIND_IF_H

// #include <execution>

// #include <variant>


// #include <futures/algorithm/detail/traits/range/range/concepts.h>


// #include <futures/futures.h>

// #include <futures/algorithm/traits/algorithm_traits.h>

// #include <futures/algorithm/detail/try_async.h>

// #include <futures/algorithm/partitioner/partitioner.h>


namespace futures {
    /** \addtogroup algorithms Algorithms
     *  @{
     */

    /// \brief Functor representing the overloads for the @ref find_if function
    class find_if_functor : public detail::unary_invoke_algorithm_functor<find_if_functor> {
      public:
        /// \brief Complete overload of the find_if algorithm
        /// \tparam E Executor type
        /// \tparam P Partitioner type
        /// \tparam I Iterator type
        /// \tparam S Sentinel iterator type
        /// \tparam Fun Function type
        /// \param ex Executor
        /// \param p Partitioner
        /// \param first Iterator to first element in the range
        /// \param last Iterator to (last + 1)-th element in the range
        /// \param f Function
        /// \brief function template \c find_if
        template <class E, class P, class I, class S, class Fun,
                  std::enable_if_t<is_executor_v<E> && is_partitioner_v<P, I, S> && futures::detail::input_iterator<I> &&
                                       futures::detail::sentinel_for<S, I> && futures::detail::indirectly_unary_invocable<Fun, I> &&
                                       std::is_copy_constructible_v<Fun>,
                                   int> = 0>
        I run(const E &ex, P p, I first, S last, Fun f) const {
            auto middle = p(first, last);
            if (middle == last || std::is_same_v<E, inline_executor> || futures::detail::forward_iterator<I>) {
                return std::find_if(first, last, f);
            }

            // Run find_if on rhs: [middle, last]
            auto [rhs, rhs_started, rhs_cancel] = try_async(ex, [=]() { return operator()(ex, p, middle, last, f); });

            // Run find_if on lhs: [first, middle]
            I lhs = operator()(ex, p, first, middle, f);

            // Wait for rhs
            if (is_ready(rhs_started)) {
                rhs.wait();
                if (lhs != middle) {
                    return lhs;
                } else {
                    return rhs.get();
                }
            } else {
                rhs_cancel.request_stop();
                rhs.detach();
                if (lhs != middle) {
                    return lhs;
                } else {
                    return operator()(make_inline_executor(), p, middle, last, f);
                }
            }
        }
    };

    /// \brief Finds the first element satisfying specific criteria
    inline constexpr find_if_functor find_if;

    /** @}*/ // \addtogroup algorithms Algorithms
} // namespace futures

#endif // FUTURES_FIND_IF_H

// #include <futures/algorithm/find_if_not.h>
//
// Created by Alan Freitas on 8/16/21.
//

#ifndef FUTURES_FIND_IF_NOT_H
#define FUTURES_FIND_IF_NOT_H

// #include <execution>

// #include <variant>


// #include <futures/algorithm/detail/traits/range/range/concepts.h>


// #include <futures/futures.h>

// #include <futures/algorithm/traits/algorithm_traits.h>

// #include <futures/algorithm/detail/try_async.h>

// #include <futures/algorithm/partitioner/partitioner.h>


namespace futures {
    /** \addtogroup algorithms Algorithms
     *  @{
     */

    /// \brief Functor representing the overloads for the @ref find_if_not function
    class find_if_not_functor : public detail::unary_invoke_algorithm_functor<find_if_not_functor> {
      public:
        /// \brief Complete overload of the find_if_not algorithm
        /// \tparam E Executor type
        /// \tparam P Partitioner type
        /// \tparam I Iterator type
        /// \tparam S Sentinel iterator type
        /// \tparam Fun Function type
        /// \param ex Executor
        /// \param p Partitioner
        /// \param first Iterator to first element in the range
        /// \param last Iterator to (last + 1)-th element in the range
        /// \param f Function
        /// \brief function template \c find_if_not
        template <class E, class P, class I, class S, class Fun,
                  std::enable_if_t<is_executor_v<E> && is_partitioner_v<P, I, S> && futures::detail::input_iterator<I> &&
                                       futures::detail::sentinel_for<S, I> && futures::detail::indirectly_unary_invocable<Fun, I> &&
                                       std::is_copy_constructible_v<Fun>,
                                   int> = 0>
        I run(const E &ex, P p, I first, S last, Fun f) const {
            auto middle = p(first, last);
            if (middle == last || std::is_same_v<E, inline_executor> || futures::detail::forward_iterator<I>) {
                return std::find_if_not(first, last, f);
            }

            // Run find_if_not on rhs: [middle, last]
            auto [rhs, rhs_started, rhs_cancel] = try_async(ex, [=]() { return operator()(ex, p, middle, last, f); });

            // Run find_if_not on lhs: [first, middle]
            I lhs = operator()(ex, p, first, middle, f);

            // Wait for rhs
            if (is_ready(rhs_started)) {
                rhs.wait();
                if (lhs != middle) {
                    return lhs;
                } else {
                    return rhs.get();
                }
            } else {
                rhs_cancel.request_stop();
                rhs.detach();
                if (lhs != middle) {
                    return lhs;
                } else {
                    return operator()(make_inline_executor(), p, middle, last, f);
                }
            }
        }
    };

    /// \brief Finds the first element not satisfying specific criteria
    inline constexpr find_if_not_functor find_if_not;

    /** @}*/ // \addtogroup algorithms Algorithms
} // namespace futures

#endif // FUTURES_FIND_IF_NOT_H

// #include <futures/algorithm/count.h>
//
// Created by Alan Freitas on 8/16/21.
//

#ifndef FUTURES_COUNT_H
#define FUTURES_COUNT_H

// #include <execution>

// #include <variant>


// #include <futures/algorithm/detail/traits/range/range/concepts.h>


// #include <futures/futures.h>

// #include <futures/algorithm/traits/algorithm_traits.h>

// #include <futures/algorithm/detail/try_async.h>

// #include <futures/algorithm/partitioner/partitioner.h>


namespace futures {
    /** \addtogroup algorithms Algorithms
     *  @{
     */

    /// \brief Functor representing the overloads for the @ref count function
    class count_functor : public detail::value_cmp_algorithm_functor<count_functor> {
      public:
        /// \brief Complete overload of the count algorithm
        /// \tparam E Executor type
        /// \tparam P Partitioner type
        /// \tparam I Iterator type
        /// \tparam S Sentinel iterator type
        /// \tparam T Value to compare
        /// \param ex Executor
        /// \param p Partitioner
        /// \param first Iterator to first element in the range
        /// \param last Iterator to (last + 1)-th element in the range
        /// \param value Value
        /// \brief function template \c count
        template <class E, class P, class I, class S, class T,
                  std::enable_if_t<is_executor_v<E> && is_partitioner_v<P, I, S> && futures::detail::input_iterator<I> &&
                                       futures::detail::sentinel_for<S, I> &&
                                       futures::detail::indirectly_binary_invocable_<futures::detail::equal_to, T*, I>,
                                   int> = 0>
        futures::detail::iter_difference_t<I> run(const E &ex, P p, I first, S last, T v) const {
            auto middle = p(first, last);
            if (middle == last || std::is_same_v<E, inline_executor> || futures::detail::forward_iterator<I>) {
                return std::count(first, last, v);
            }

            // Run count on rhs: [middle, last]
            auto [rhs, rhs_started, rhs_cancel] = try_async(ex, [=]() { return operator()(ex, p, middle, last, v); });

            // Run count on lhs: [first, middle]
            bool lhs = operator()(ex, p, first, middle, v);

            // Wait for rhs
            if (is_ready(rhs_started)) {
                return lhs + rhs.get();
            } else {
                rhs_cancel.request_stop();
                rhs.detach();
                return lhs + operator()(make_inline_executor(), p, middle, last, v);
            }
        }
    };

    /// \brief Returns the number of elements matching an element
    inline constexpr count_functor count;

    /** @}*/ // \addtogroup algorithms Algorithms
} // namespace futures

#endif // FUTURES_COUNT_H

// #include <futures/algorithm/count_if.h>
//
// Created by Alan Freitas on 8/16/21.
//

#ifndef FUTURES_COUNT_IF_H
#define FUTURES_COUNT_IF_H

// #include <execution>

// #include <variant>


// #include <futures/algorithm/detail/traits/range/range/concepts.h>


// #include <futures/futures.h>

// #include <futures/algorithm/traits/algorithm_traits.h>

// #include <futures/algorithm/detail/try_async.h>

// #include <futures/algorithm/partitioner/partitioner.h>


namespace futures {
    /** \addtogroup algorithms Algorithms
     *  @{
     */

    /// \brief Functor representing the overloads for the @ref count_if function
    class count_if_functor : public detail::unary_invoke_algorithm_functor<count_if_functor> {
      public:
        /// \brief Complete overload of the count_if algorithm
        /// \tparam E Executor type
        /// \tparam P Partitioner type
        /// \tparam I Iterator type
        /// \tparam S Sentinel iterator type
        /// \tparam Fun Function type
        /// \param ex Executor
        /// \param p Partitioner
        /// \param first Iterator to first element in the range
        /// \param last Iterator to (last + 1)-th element in the range
        /// \param f Function
        /// \brief function template \c count_if
        template <class E, class P, class I, class S, class Fun,
                  std::enable_if_t<is_executor_v<E> && is_partitioner_v<P, I, S> && futures::detail::input_iterator<I> &&
                                       futures::detail::sentinel_for<S, I> && futures::detail::indirectly_unary_invocable<Fun, I> &&
                                       std::is_copy_constructible_v<Fun>,
                                   int> = 0>
        futures::detail::iter_difference_t<I> run(const E &ex, P p, I first, S last, Fun f) const {
            auto middle = p(first, last);
            if (middle == last || std::is_same_v<E, inline_executor> || futures::detail::forward_iterator<I>) {
                return std::count_if(first, last, f);
            }

            // Run count_if on rhs: [middle, last]
            auto [rhs, rhs_started, rhs_cancel] = try_async(ex, [=]() { return operator()(ex, p, middle, last, f); });

            // Run count_if on lhs: [first, middle]
            bool lhs = operator()(ex, p, first, middle, f);

            // Wait for rhs
            if (is_ready(rhs_started)) {
                return lhs + rhs.get();
            } else {
                rhs_cancel.request_stop();
                rhs.detach();
                return lhs + operator()(make_inline_executor(), p, middle, last, f);
            }
        }
    };

    /// \brief Returns the number of elements satisfying specific criteria
    inline constexpr count_if_functor count_if;

    /** @}*/ // \addtogroup algorithms Algorithms
} // namespace futures

#endif // FUTURES_COUNT_IF_H

// #include <futures/algorithm/reduce.h>
//
// Created by Alan Freitas on 8/16/21.
//

#ifndef FUTURES_REDUCE_H
#define FUTURES_REDUCE_H

// #include <execution>

// #include <variant>

#include <numeric>

// #include <futures/algorithm/detail/traits/range/range/concepts.h>


// #include <futures/futures.h>

// #include <futures/algorithm/traits/algorithm_traits.h>

// #include <futures/algorithm/detail/try_async.h>

// #include <futures/algorithm/partitioner/partitioner.h>


namespace futures {
    /** \addtogroup algorithms Algorithms
     *  @{
     */

    /// \brief Functor representing the overloads for the @ref reduce function
    class reduce_functor {
      public:
        /// \brief Complete overload of the reduce algorithm
        /// The reduce algorithm is equivalent to a version std::accumulate where the binary operation
        /// is applied out of order.
        /// \tparam E Executor type
        /// \tparam P Partitioner type
        /// \tparam I Iterator type
        /// \tparam S Sentinel iterator type
        /// \tparam Fun Function type
        /// \param ex Executor
        /// \param p Partitioner
        /// \param first Iterator to first element in the range
        /// \param last Iterator to (last + 1)-th element in the range
        /// \param i Initial value for the reduction
        /// \param f Function
        template <
            class E, class P, class I, class S, class T, class Fun = std::plus<>,
            std::enable_if_t<is_executor_v<E> && is_partitioner_v<P, I, S> && futures::detail::input_iterator<I> &&
                                 futures::detail::sentinel_for<S, I> && std::is_same_v<futures::detail::iter_value_t<I>, T> &&
                                 futures::detail::indirectly_binary_invocable_<Fun, I, I> && std::is_copy_constructible_v<Fun>,
                             int> = 0>
        T operator()(const E &ex, P p, I first, S last, T i, Fun f = std::plus<>()) const {
            auto middle = p(first, last);
            if (middle == last || std::is_same_v<E, inline_executor> || futures::detail::forward_iterator<I>) {
                return std::reduce(first, last, i, f);
            }

            // Run reduce on rhs: [middle, last]
            auto [rhs, rhs_started, rhs_cancel] = try_async(ex, [=]() {
                return operator()(ex, p, middle, last, i, f);
            });

            // Run reduce on lhs: [first, middle]
            T lhs = operator()(ex, p, first, middle, i, f);

            // Wait for rhs
            if (is_ready(rhs_started)) {
                return f(lhs, rhs.get());
            } else {
                rhs_cancel.request_stop();
                rhs.detach();
                T i_rhs = operator()(make_inline_executor(), p, middle, last, i, f);
                return f(lhs, i_rhs);
            }
        }

        /// \overload default init value
        template <class E, class P, class I, class S, class Fun = std::plus<>,
                  std::enable_if_t<is_executor_v<E> && is_partitioner_v<P, I, S> && futures::detail::input_iterator<I> &&
                                       futures::detail::sentinel_for<S, I> && futures::detail::indirectly_binary_invocable_<Fun, I, I> &&
                                       std::is_copy_constructible_v<Fun>,
                                   int> = 0>
        futures::detail::iter_value_t<I> operator()(const E &ex, P p, I first, S last, Fun f = std::plus<>()) const {
            if (first != last) {
                return operator()(ex, std::forward<P>(p), std::next(first), last, *first, f);
            } else {
                return futures::detail::iter_value_t<I>{};
            }
        }

        /// \overload execution policy instead of executor
        template <
            class E, class P, class I, class S, class T, class Fun = std::plus<>,
            std::enable_if_t<not is_executor_v<E> && is_execution_policy_v<E> && is_partitioner_v<P, I, S> &&
                                 futures::detail::input_iterator<I> && futures::detail::sentinel_for<S, I> &&
                                 std::is_same_v<futures::detail::iter_value_t<I>, T> &&
                                 futures::detail::indirectly_binary_invocable_<Fun, I, I> && std::is_copy_constructible_v<Fun>,
                             int> = 0>
        T operator()(const E &, P p, I first, S last, T i, Fun f = std::plus<>()) const {
            return operator()(make_policy_executor<E, I, S>(), std::forward<P>(p), first, last, i, f);
        }

        /// \overload execution policy instead of executor / default init value
        template <
            class E, class P, class I, class S, class Fun = std::plus<>,
            std::enable_if_t<not is_executor_v<E> && is_execution_policy_v<E> && is_partitioner_v<P, I, S> &&
                                 futures::detail::input_iterator<I> && futures::detail::sentinel_for<S, I> &&
                                 futures::detail::indirectly_binary_invocable_<Fun, I, I> && std::is_copy_constructible_v<Fun>,
                             int> = 0>
        futures::detail::iter_value_t<I> operator()(const E &, P p, I first, S last, Fun f = std::plus<>()) const {
            return operator()(make_policy_executor<E, I, S>(), std::forward<P>(p), first, last, f);
        }

        /// \overload Ranges
        template <class E, class P, class R, class T, class Fun = std::plus<>,
                  std::enable_if_t<
                      (is_executor_v<E> || is_execution_policy_v<E>)&&is_range_partitioner_v<P, R> &&
                          futures::detail::input_range<R> && std::is_same_v<futures::detail::range_value_t<R>, T> &&
                          futures::detail::indirectly_binary_invocable_<Fun, futures::detail::iterator_t<R>, futures::detail::iterator_t<R>> &&
                          std::is_copy_constructible_v<Fun>,
                      int> = 0>
        T operator()(const E &ex, P p, R &&r, T i, Fun f = std::plus<>()) const {
            return operator()(ex, std::forward<P>(p), std::begin(r), std::end(r), i, std::move(f));
        }

        /// \overload Ranges / default init value
        template <class E, class P, class R, class Fun = std::plus<>,
                  std::enable_if_t<
                      (is_executor_v<E> || is_execution_policy_v<E>)&&is_range_partitioner_v<P, R> &&
                          futures::detail::input_range<R> &&
                          futures::detail::indirectly_binary_invocable_<Fun, futures::detail::iterator_t<R>, futures::detail::iterator_t<R>> &&
                          std::is_copy_constructible_v<Fun>,
                      int> = 0>
        futures::detail::range_value_t<R> operator()(const E &ex, P p, R &&r, Fun f = std::plus<>()) const {
            return operator()(ex, std::forward<P>(p), std::begin(r), std::end(r), std::move(f));
        }

        /// \overload Iterators / default parallel executor
        template <
            class P, class I, class S, class T, class Fun = std::plus<>,
            std::enable_if_t<is_partitioner_v<P, I, S> && futures::detail::input_iterator<I> && futures::detail::sentinel_for<S, I> &&
                                 std::is_same_v<futures::detail::iter_value_t<I>, T> &&
                                 futures::detail::indirectly_binary_invocable_<Fun, I, I> && std::is_copy_constructible_v<Fun>,
                             int> = 0>
        T operator()(P p, I first, S last, T i, Fun f = std::plus<>()) const {
            return operator()(make_default_executor(), std::forward<P>(p), first, last, i, std::move(f));
        }

        /// \overload Iterators / default parallel executor / default init value
        template <
            class P, class I, class S, class Fun = std::plus<>,
            std::enable_if_t<is_partitioner_v<P, I, S> && futures::detail::input_iterator<I> && futures::detail::sentinel_for<S, I> &&
                                 futures::detail::indirectly_binary_invocable_<Fun, I, I> && std::is_copy_constructible_v<Fun>,
                             int> = 0>
        futures::detail::iter_value_t<I> operator()(P p, I first, S last, Fun f = std::plus<>()) const {
            return operator()(make_default_executor(), std::forward<P>(p), first, last, std::move(f));
        }

        /// \overload Ranges / default parallel executor
        template <
            class P, class R, class T, class Fun = std::plus<>,
            std::enable_if_t<
                is_range_partitioner_v<P, R> && futures::detail::input_range<R> && std::is_same_v<futures::detail::range_value_t<R>, T> &&
                    futures::detail::indirectly_binary_invocable_<Fun, futures::detail::iterator_t<R>, futures::detail::iterator_t<R>> &&
                    std::is_copy_constructible_v<Fun>,
                int> = 0>
        T operator()(P p, R &&r, T i, Fun f = std::plus<>()) const {
            return operator()(make_default_executor(), std::forward<P>(p), std::begin(r), std::end(r), i, std::move(f));
        }

        /// \overload Ranges / default parallel executor / default init value
        template <class P, class R, class Fun = std::plus<>,
                  std::enable_if_t<
                      is_range_partitioner_v<P, R> && futures::detail::input_range<R> &&
                          futures::detail::indirectly_binary_invocable_<Fun, futures::detail::iterator_t<R>, futures::detail::iterator_t<R>> &&
                          std::is_copy_constructible_v<Fun>,
                      int> = 0>
        futures::detail::range_value_t<R> operator()(P p, R &&r, Fun f = std::plus<>()) const {
            return operator()(make_default_executor(), std::forward<P>(p), std::begin(r), std::end(r), std::move(f));
        }

        /// \overload Iterators / default partitioner
        template <
            class E, class I, class S, class T, class Fun = std::plus<>,
            std::enable_if_t<(is_executor_v<E> || is_execution_policy_v<E>)&&futures::detail::input_iterator<I> &&
                                 futures::detail::sentinel_for<S, I> && std::is_same_v<futures::detail::iter_value_t<I>, T> &&
                                 futures::detail::indirectly_binary_invocable_<Fun, I, I> && std::is_copy_constructible_v<Fun>,
                             int> = 0>
        T operator()(const E &ex, I first, S last, T i, Fun f = std::plus<>()) const {
            return operator()(ex, make_default_partitioner(first, last), first, last, i, std::move(f));
        }

        /// \overload Iterators / default partitioner / default init value
        template <class E, class I, class S, class Fun = std::plus<>,
                  std::enable_if_t<(is_executor_v<E> || is_execution_policy_v<E>)&&futures::detail::input_iterator<I> &&
                                       futures::detail::sentinel_for<S, I> && futures::detail::indirectly_binary_invocable_<Fun, I, I> &&
                                       std::is_copy_constructible_v<Fun>,
                                   int> = 0>
        futures::detail::iter_value_t<I> operator()(const E &ex, I first, S last, Fun f = std::plus<>()) const {
            return operator()(ex, make_default_partitioner(first, last), first, last, std::move(f));
        }

        /// \overload Ranges / default partitioner
        template <class E, class R, class T, class Fun = std::plus<>,
                  std::enable_if_t<
                      (is_executor_v<E> || is_execution_policy_v<E>)&&futures::detail::input_range<R> &&
                          std::is_same_v<futures::detail::range_value_t<R>, T> &&
                          futures::detail::indirectly_binary_invocable_<Fun, futures::detail::iterator_t<R>, futures::detail::iterator_t<R>> &&
                          std::is_copy_constructible_v<Fun>,
                      int> = 0>
        T operator()(const E &ex, R &&r, T i, Fun f = std::plus<>()) const {
            return operator()(ex, make_default_partitioner(std::forward<R>(r)), std::begin(r), std::end(r), i, std::move(f));
        }

        /// \overload Ranges / default partitioner / default init value
        template <class E, class R, class Fun = std::plus<>,
                  std::enable_if_t<
                      (is_executor_v<E> || is_execution_policy_v<E>)&&futures::detail::input_range<R> &&
                          futures::detail::indirectly_binary_invocable_<Fun, futures::detail::iterator_t<R>, futures::detail::iterator_t<R>> &&
                          std::is_copy_constructible_v<Fun>,
                      int> = 0>
        futures::detail::range_value_t<R> operator()(const E &ex, R &&r, Fun f = std::plus<>()) const {
            return operator()(ex, make_default_partitioner(std::forward<R>(r)), std::begin(r), std::end(r), std::move(f));
        }

        /// \overload Iterators / default executor / default partitioner
        template <
            class I, class S, class T, class Fun = std::plus<>,
            std::enable_if_t<futures::detail::input_iterator<I> && futures::detail::sentinel_for<S, I> &&
                                 std::is_same_v<futures::detail::iter_value_t<I>, T> &&
                                 futures::detail::indirectly_binary_invocable_<Fun, I, I> && std::is_copy_constructible_v<Fun>,
                             int> = 0>
        T operator()(I first, S last, T i, Fun f = std::plus<>()) const {
            return operator()(make_default_executor(), make_default_partitioner(first, last), first, last, i, std::move(f));
        }

        /// \overload Iterators / default executor / default partitioner / default init value
        template <
            class I, class S, class Fun = std::plus<>,
            std::enable_if_t<futures::detail::input_iterator<I> && futures::detail::sentinel_for<S, I> &&
                                 futures::detail::indirectly_binary_invocable_<Fun, I, I> && std::is_copy_constructible_v<Fun>,
                             int> = 0>
        futures::detail::iter_value_t<I> operator()(I first, S last, Fun f = std::plus<>()) const {
            return operator()(make_default_executor(), make_default_partitioner(first, last), first, last, std::move(f));
        }

        /// \overload Ranges / default executor / default partitioner
        template <class R, class T, class Fun = std::plus<>,
                  std::enable_if_t<
                      futures::detail::input_range<R> && std::is_same_v<futures::detail::range_value_t<R>, T> &&
                          futures::detail::indirectly_binary_invocable_<Fun, futures::detail::iterator_t<R>, futures::detail::iterator_t<R>> &&
                          std::is_copy_constructible_v<Fun>,
                      int> = 0>
        T operator()(R &&r, T i, Fun f = std::plus<>()) const {
            return operator()(make_default_executor(), make_default_partitioner(r), std::begin(r), std::end(r), i,
                       std::move(f));
        }

        /// \overload Ranges / default executor / default partitioner / default init value
        template <class R, class Fun = std::plus<>,
                  std::enable_if_t<
                      futures::detail::input_range<R> &&
                          futures::detail::indirectly_binary_invocable_<Fun, futures::detail::iterator_t<R>, futures::detail::iterator_t<R>> &&
                          std::is_copy_constructible_v<Fun>,
                      int> = 0>
        futures::detail::range_value_t<R> operator()(R &&r, Fun f = std::plus<>()) const {
            return operator()(make_default_executor(), make_default_partitioner(r), std::begin(r), std::end(r), std::move(f));
        }
    };

    /// \brief Sums up (or accumulate with a custom function) a range of elements, except out of order
    inline constexpr reduce_functor reduce;

    /** @}*/ // \addtogroup algorithms Algorithms
} // namespace futures

#endif // FUTURES_REDUCE_H



#endif // FUTURES_ALGORITHM_H
