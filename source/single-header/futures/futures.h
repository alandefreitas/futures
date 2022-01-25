//
// Copyright (c) 2021 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
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
/// However, we use std::future and the ASIO proposed standard executors
/// (P0443r13, P1348r0, and P1393r0) to allow for better interoperability with
/// the C++ standard.
/// - the async function can accept any standard executor
/// - the async function will use a reasonable default thread pool when no
/// executor is provided
/// - future-concepts allows for new future classes to extend functionality
/// while reusing algorithms
/// - a cancellable future class is provided for more sensitive use cases
/// - the API can be updated as the standard gets updated
/// - the standard algorithms are reimplemented with a preference for parallel
/// operations
///
/// This interoperability comes at a price for continuations, as we might need
/// to poll for when_all/when_any/then events, because std::future does not have
/// internal continuations.
///
/// Although we attempt to replicate these features without recreating the
/// future class with internal continuations, we use a number of heuristics to
/// avoid polling for when_all/when_any/then:
/// - we allow for other future-like classes to be implemented through a
/// future-concept and provide these
///   functionalities at a lower cost whenever we can
/// - `when_all` (or operator&&) returns a when_all_future class, which does not
/// create a new std::future at all
///    and can check directly if futures are ready
/// - `when_any` (or operator||) returns a when_any_future class, which
/// implements a number of heuristics to avoid
///    polling, limit polling time, increased pooling intervals, and only
///    launching the necessary continuation futures for long tasks. (although
///    when_all always takes longer than when_any, when_any involves a number of
///    heuristics that influence its performance)
/// - `then` (or operator>>) returns a new future object that sleeps while the
/// previous future isn't ready
/// - when the standard supports that, this approach based on concepts also
/// serve as extension points to allow
///   for these proxy classes to change their behavior to some other algorithm
///   that makes more sense for futures that support continuations,
///   cancellation, progress, queries, .... More interestingly, the concepts
///   allow for all these possible future types to interoperate.
///
/// \see https://en.cppreference.com/w/cpp/experimental/concurrency
/// \see https://think-async.com/Asio/asio-1.18.2/doc/asio/std_executors.html
/// \see https://github.com/Amanieu/asyncplusplus

// Future classes
// #include <futures/futures/async.h>
#ifndef FUTURES_ASYNC_H
#define FUTURES_ASYNC_H

// #include <futures/executor/inline_executor.h>
#ifndef FUTURES_INLINE_EXECUTOR_H
#define FUTURES_INLINE_EXECUTOR_H

// #include <futures/config/asio_include.h>
#ifndef FUTURES_ASIO_INCLUDE_H
#define FUTURES_ASIO_INCLUDE_H

/// \file
/// Indirectly includes asio or boost.asio
///
/// Whenever including <asio.hpp>, we include this file instead.
/// This ensures the logic of including asio or boost::asio is consistent and
/// that we never include both.
///
/// Because this is not a networking library, at this point, we only depend on
/// the asio execution concepts (which we can forward-declare) and its
/// thread-pool, which is not very advanced. So this means we might be able to
/// remove boost asio as a dependency at some point and, because the small
/// vector library is also not mandatory, we can make this library free of
/// dependencies.
///

#ifdef _WIN32
#    include <SDKDDKVer.h>
#endif

/*
 * Check what versions of asio are available.
 *
 * We use __has_include<...> as a first alternative. If this fails,
 * we use some common assumptions.
 *
 */
#if defined __has_include
#    if __has_include(<asio.hpp>)
#        define FUTURES_HAS_ASIO
#    endif
#endif

#if defined __has_include
#    if __has_include(<boost/asio.hpp>)
#        define FUTURES_HAS_BOOST_ASIO
#    endif
#endif

// Recur to simple assumptions when not available.
#if !defined(FUTURES_HAS_BOOST_ASIO) && !defined(FUTURES_HAS_ASIO)
#    if FUTURES_PREFER_BOOST_DEPENDENCIES
#        define FUTURES_HAS_BOOST_ASIO
#    elif FUTURES_PREFER_STANDALONE_DEPENDENCIES
#        define FUTURES_HAS_ASIO
#    elif BOOST_CXX_VERSION
#        define FUTURES_HAS_BOOST_ASIO
#    elif FUTURES_STANDALONE
#        define FUTURES_HAS_ASIO
#    else
#        define FUTURES_HAS_BOOST_ASIO
#    endif
#endif

/*
 * Decide what version of asio to use
 */

// If the standalone is available, this is what we assume the user usually
// prefers, since it's more specific
#if defined(FUTURES_HAS_ASIO)           \
    && !(                               \
        defined(FUTURES_HAS_BOOST_ASIO) \
        && defined(FUTURES_PREFER_BOOST_DEPENDENCIES))
#    define FUTURES_USE_ASIO
#    include <asio.hpp>
#else
#    define FUTURES_USE_BOOST_ASIO
#    include <boost/asio.hpp>
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
} // namespace futures

#endif // FUTURES_ASIO_INCLUDE_H

// #include <futures/executor/is_executor.h>
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
    /// Future and previous executor models can be considered here, as long as
    /// their interface is the same as asio or we implement their respective
    /// traits to make @ref async work properly.
    template <typename T>
    using is_executor = asio::is_executor<T>;

    /// \brief Determine if type is an executor
    template <typename T>
    constexpr bool is_executor_v = is_executor<T>::value;

    /** @} */ // \addtogroup executors Executors
} // namespace futures

#endif // FUTURES_IS_EXECUTOR_H


namespace futures {
    /** \addtogroup executors Executors
     *  @{
     */

    /// \brief A minimal executor that runs anything in the local thread in the
    /// default context
    ///
    /// Although simple, it needs to meet the executor requirements:
    /// - Executor concept
    /// - Ability to query the execution context
    ///     - Result being derived from execution_context
    /// - The execute function
    /// \see https://think-async.com/Asio/asio-1.18.2/doc/asio/std_executors.html
    struct inline_executor
    {
        asio::execution_context *context_{ nullptr };

        constexpr bool
        operator==(const inline_executor &other) const noexcept {
            return context_ == other.context_;
        }

        constexpr bool
        operator!=(const inline_executor &other) const noexcept {
            return !(*this == other);
        }

        [[nodiscard]] constexpr asio::execution_context &
        query(asio::execution::context_t) const noexcept {
            return *context_;
        }

        static constexpr asio::execution::blocking_t::never_t
        query(asio::execution::blocking_t) noexcept {
            return asio::execution::blocking_t::never;
        }

        template <class F>
        void
        execute(F f) const {
            f();
        }
    };

    /// \brief Get the inline execution context
    asio::execution_context &
    inline_execution_context() {
        static asio::execution_context context;
        return context;
    }

    /// \brief Make an inline executor object
    inline_executor
    make_inline_executor() {
        asio::execution_context &ctx = inline_execution_context();
        return inline_executor{ &ctx };
    }

    /** @} */ // \addtogroup executors Executors
} // namespace futures

#ifdef FUTURES_USE_BOOST_ASIO
namespace boost {
#endif
    namespace asio {
        /// \brief Ensure asio and our internal functions see inline_executor as
        /// an executor
        ///
        /// This traits ensures asio and our internal functions see
        /// inline_executor as an executor, as asio traits don't always work.
        ///
        /// This is quite a workaround until things don't improve with our
        /// executor traits.
        ///
        /// Ideally, we would have our own executor traits and let asio pick up
        /// from those.
        ///
        template <>
        class is_executor<futures::inline_executor> : public std::true_type
        {};

        namespace traits {
#if !defined(ASIO_HAS_DEDUCED_EXECUTE_MEMBER_TRAIT)
            template <typename F>
            struct execute_member<futures::inline_executor, F>
            {
                static constexpr bool is_valid = true;
                static constexpr bool is_noexcept = true;
                typedef void result_type;
            };

#endif // !defined(ASIO_HAS_DEDUCED_EXECUTE_MEMBER_TRAIT)
#if !defined(ASIO_HAS_DEDUCED_EQUALITY_COMPARABLE_TRAIT)
            template <>
            struct equality_comparable<futures::inline_executor>
            {
                static constexpr bool is_valid = true;
                static constexpr bool is_noexcept = true;
            };

#endif // !defined(ASIO_HAS_DEDUCED_EQUALITY_COMPARABLE_TRAIT)
#if !defined(ASIO_HAS_DEDUCED_QUERY_MEMBER_TRAIT)
            template <>
            struct query_member<
                futures::inline_executor,
                asio::execution::context_t>
            {
                static constexpr bool is_valid = true;
                static constexpr bool is_noexcept = true;
                typedef asio::execution_context &result_type;
            };

#endif // !defined(ASIO_HAS_DEDUCED_QUERY_MEMBER_TRAIT)
#if !defined(ASIO_HAS_DEDUCED_QUERY_STATIC_CONSTEXPR_MEMBER_TRAIT)
            template <typename Property>
            struct query_static_constexpr_member<
                futures::inline_executor,
                Property,
                typename enable_if<std::is_convertible<
                    Property,
                    asio::execution::blocking_t>::value>::type>
            {
                static constexpr bool is_valid = true;
                static constexpr bool is_noexcept = true;
                typedef asio::execution::blocking_t::never_t result_type;
                static constexpr result_type
                value() noexcept {
                    return result_type();
                }
            };

#endif // !defined(ASIO_HAS_DEDUCED_QUERY_STATIC_CONSTEXPR_MEMBER_TRAIT)

        } // namespace traits
    }     // namespace asio
#ifdef FUTURES_USE_BOOST_ASIO
}
#endif

#endif // FUTURES_INLINE_EXECUTOR_H

// #include <futures/futures/await.h>
#ifndef FUTURES_AWAIT_H
#define FUTURES_AWAIT_H

// #include <futures/futures/traits/is_future.h>
#ifndef FUTURES_IS_FUTURE_H
#define FUTURES_IS_FUTURE_H

#include <future>
#include <type_traits>

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
    template <typename>
    struct is_future : std::false_type
    {};

    /// \brief Customization point to determine if a type is a future type
    /// (specialization for std::future<T>)
    template <typename T>
    struct is_future<std::future<T>> : std::true_type
    {};

    /// \brief Customization point to determine if a type is a future type
    /// (specialization for std::shared_future<T>)
    template <typename T>
    struct is_future<std::shared_future<T>> : std::true_type
    {};

    /// \brief Customization point to determine if a type is a future type as a
    /// bool value
    template <class T>
    constexpr bool is_future_v = is_future<T>::value;

    /// \brief Customization point to determine if a type is a shared future type
    template <typename>
    struct has_ready_notifier : std::false_type
    {};

    /// \brief Customization point to determine if a type is a shared future type
    template <class T>
    constexpr bool has_ready_notifier_v = has_ready_notifier<T>::value;

    /// \brief Customization point to determine if a type is a shared future type
    template <typename>
    struct is_shared_future : std::false_type
    {};

    /// \brief Customization point to determine if a type is a shared future
    /// type (specialization for std::shared_future<T>)
    template <typename T>
    struct is_shared_future<std::shared_future<T>> : std::true_type
    {};

    /// \brief Customization point to determine if a type is a shared future type
    template <class T>
    constexpr bool is_shared_future_v = is_shared_future<T>::value;

    /// \brief Customization point to define future as supporting lazy
    /// continuations
    template <typename>
    struct is_lazy_continuable : std::false_type
    {};

    /// \brief Customization point to define future as supporting lazy
    /// continuations
    template <class T>
    constexpr bool is_lazy_continuable_v = is_lazy_continuable<T>::value;

    /// \brief Customization point to define future as stoppable
    template <typename>
    struct is_stoppable : std::false_type
    {};

    /// \brief Customization point to define future as stoppable
    template <class T>
    constexpr bool is_stoppable_v = is_stoppable<T>::value;

    /// \brief Customization point to define future having a common stop token
    template <typename>
    struct has_stop_token : std::false_type
    {};

    /// \brief Customization point to define future having a common stop token
    template <class T>
    constexpr bool has_stop_token_v = has_stop_token<T>::value;

    /** @} */ // \addtogroup future-traits Future Traits
    /** @} */ // \addtogroup futures Futures
} // namespace futures

#endif // FUTURES_IS_FUTURE_H

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

    /// \brief Very simple syntax sugar for types that pass the @ref is_future
    /// concept
    ///
    /// This syntax is most useful for cases where we are immediately requesting
    /// the future result.
    ///
    /// The function also makes the syntax optionally a little closer to
    /// languages such as javascript.
    ///
    /// \tparam Future A future type
    ///
    /// \return The result of the future object
    template <
        typename Future
#ifndef FUTURES_DOXYGEN
        ,
        std::enable_if_t<is_future_v<std::decay_t<Future>>, int> = 0
#endif
        >
    decltype(auto)
    await(Future &&f) {
        return f.get();
    }

    /** @} */
    /** @} */
} // namespace futures

#endif // FUTURES_AWAIT_H

// #include <futures/futures/launch.h>
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
     * This module defines functions for conveniently launching asynchronous
     * tasks and policies to determine how executors should handle these tasks.
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

    /// \brief Specifies the launch policy for a task executed by the @ref
    /// futures::async function
    ///
    /// std::async creates a new thread for each asynchronous operation, which
    /// usually entails in only two execution policies: new thread or inline.
    /// Because futures::async use executors, there are many more policies and
    /// ways to use these executors beyond yes/no.
    ///
    /// Most of the time, we want the executor/post policy for executors. So as
    /// the @ref async function also accepts executors directly, this option can
    /// often be ignored, and is here mostly here for compatibility with the
    /// std::async.
    ///
    /// When only the policy is provided, async will try to generate the proper
    /// executor for that policy. When the executor and the policy is provided,
    /// we might only have some conflict for the deferred policy, which does not
    /// use an executor in std::launch. In the context of executors, the
    /// deferred policy means the function is only posted to the executor when
    /// its result is requested.
    enum class launch
    {
        /// no policy
        none = 0b0000'0000,
        /// execute on a new thread regardless of executors (same as
        /// std::async::async)
        new_thread = 0b0000'0001,
        /// execute on a new thread regardless of executors (same as
        /// std::async::async)
        async = 0b0000'0001,
        /// execute on the calling thread when result is requested (same as
        /// std::async::deferred)
        deferred = 0b0000'0010,
        /// execute on the calling thread when result is requested (same as
        /// std::async::deferred)
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
    constexpr launch
    operator&(launch x, launch y) {
        return static_cast<launch>(static_cast<int>(x) & static_cast<int>(y));
    }

    /// \brief operator | for launch policies
    ///
    /// \param x left-hand side operand
    /// \param y right-hand side operand
    /// \return A launch policy that attempts to satisfy any of the policies
    constexpr launch
    operator|(launch x, launch y) {
        return static_cast<launch>(static_cast<int>(x) | static_cast<int>(y));
    }

    /// \brief operator ^ for launch policies
    ///
    /// \param x left-hand side operand
    /// \param y right-hand side operand
    /// \return A launch policy that attempts to satisfy any policy set in only
    /// one of them
    constexpr launch
    operator^(launch x, launch y) {
        return static_cast<launch>(static_cast<int>(x) ^ static_cast<int>(y));
    }

    /// \brief operator ~ for launch policies
    ///
    /// \param x left-hand side operand
    /// \return A launch policy that attempts to satisfy the opposite of the
    /// policies set
    constexpr launch
    operator~(launch x) {
        return static_cast<launch>(~static_cast<int>(x));
    }

    /// \brief operator &= for launch policies
    ///
    /// \param x left-hand side operand
    /// \param y right-hand side operand
    /// \return A reference to `x`
    constexpr launch &
    operator&=(launch &x, launch y) {
        return x = x & y;
    }

    /// \brief operator |= for launch policies
    ///
    /// \param x left-hand side operand
    /// \param y right-hand side operand
    /// \return A reference to `x`
    constexpr launch &
    operator|=(launch &x, launch y) {
        return x = x | y;
    }

    /// \brief operator ^= for launch policies
    ///
    /// \param x left-hand side operand
    /// \param y right-hand side operand
    /// \return A reference to `x`
    constexpr launch &
    operator^=(launch &x, launch y) {
        return x = x ^ y;
    }
    /** @} */
    /** @} */
    /** @} */
} // namespace futures

#endif // FUTURES_LAUNCH_H

// #include <futures/futures/detail/empty_base.h>
#ifndef FUTURES_EMPTY_BASE_H
#define FUTURES_EMPTY_BASE_H

namespace futures::detail {
    /** \addtogroup futures Futures
     *  @{
     */

    /// \brief A convenience struct to refer to an empty type whenever we need
    /// one
    struct empty_value_type
    {};

    /// \brief A convenience struct to refer to an empty value whenever we need
    /// one
    inline constexpr empty_value_type empty_value = empty_value_type();

    /// \brief Represents a potentially empty base class for empty base class
    /// optimization
    ///
    /// We use the name maybe_empty for the base class that might be empty and
    /// empty_value_type / empty_value for the values we know to be empty.
    ///
    /// \tparam T The type represented by the base class
    /// \tparam BaseIndex An index to differentiate base classes in the same
    /// derived class. This might be important to ensure the same base class
    /// isn't inherited twice. \tparam E Indicates whether we should really
    /// instantiate the class (true when the class is not empty)
    template <class T, unsigned BaseIndex = 0, bool E = std::is_empty_v<T>>
    class maybe_empty
    {
    public:
        /// \brief The type this base class is effectively represent
        using value_type = T;

        /// \brief Initialize this potentially empty base with the specified
        /// values This will initialize the vlalue with the default constructor
        /// T(args...)
        template <class... Args>
        explicit maybe_empty(Args &&...args)
            : value_(std::forward<Args>(args)...) {}

        /// \brief Get the effective value this class represents
        /// This returns the underlying value represented here
        const T &
        get() const noexcept {
            return value_;
        }

        /// \brief Get the effective value this class represents
        /// This returns the underlying value represented here
        T &
        get() noexcept {
            return value_;
        }

    private:
        /// \brief The effective value representation when the value is not empty
        T value_;
    };

    /// \brief Represents a potentially empty base class, when it's effectively
    /// not empty
    ///
    /// \tparam T The type represented by the base class
    /// \tparam BaseIndex An index to differentiate base classes in the same
    /// derived class
    template <class T, unsigned BaseIndex>
    class maybe_empty<T, BaseIndex, true> : public T
    {
    public:
        /// \brief The type this base class is effectively represent
        using value_type = T;

        /// \brief Initialize this potentially empty base with the specified
        /// values This won't initialize any values but it will call the
        /// appropriate constructor T() in case we need its behaviour
        template <class... Args>
        explicit maybe_empty(Args &&...args) : T(std::forward<Args>(args)...) {}

        /// \brief Get the effective value this class represents
        /// Although the element takes no space, we can return a reference to
        /// whatever it represents so we can access its underlying functions
        const T &
        get() const noexcept {
            return *this;
        }

        /// \brief Get the effective value this class represents
        /// Although the element takes no space, we can return a reference to
        /// whatever it represents so we can access its underlying functions
        T &
        get() noexcept {
            return *this;
        }
    };

    /** @} */
} // namespace futures::detail

#endif // FUTURES_EMPTY_BASE_H

// #include <futures/futures/detail/traits/async_result_of.h>
#ifndef FUTURES_ASYNC_RESULT_OF_H
#define FUTURES_ASYNC_RESULT_OF_H

// #include <futures/futures/basic_future.h>
#ifndef FUTURES_BASIC_FUTURE_H
#define FUTURES_BASIC_FUTURE_H

// #include <futures/executor/default_executor.h>
#ifndef FUTURES_DEFAULT_EXECUTOR_H
#define FUTURES_DEFAULT_EXECUTOR_H

// #include <futures/config/asio_include.h>

// #include <futures/executor/is_executor.h>


namespace futures {
    /** \addtogroup executors Executors
     *  @{
     */

    /// \brief A version of hardware_concurrency that always returns at least 1
    ///
    /// This function is a safer version of hardware_concurrency that always
    /// returns at least 1 to represent the current context when the value is
    /// not computable.
    ///
    /// - It never returns 0, 1 is returned instead.
    /// - It is guaranteed to remain constant for the duration of the program.
    ///
    /// \see
    /// https://en.cppreference.com/w/cpp/thread/thread/hardware_concurrency
    ///
    /// \return Number of concurrent threads supported. If the value is not
    /// well-defined or not computable, returns 1.
    std::size_t
    hardware_concurrency() noexcept;
    inline std::size_t
    hardware_concurrency() noexcept {
        // Cache the value because calculating it may be expensive
        static std::size_t value = std::thread::hardware_concurrency();

        // Always return at least 1 core
        return std::max(static_cast<std::size_t>(1), value);
    }

    /// \brief The default execution context for async operations, unless
    /// otherwise stated
    ///
    /// Unless an executor is explicitly provided, this is the executor we use
    /// for async operations.
    ///
    /// This is the ASIO thread pool execution context with a default number of
    /// threads. However, the default execution context (and its type) might
    /// change in other versions of this library if something more general comes
    /// along. As the standard for executors gets adopted, libraries are likely
    /// to provide better implementations.
    ///
    /// Also note that executors might not allow work-stealing. This needs to be
    /// taken into account when implementing algorithms with recursive tasks.
    /// One common options is to use `try_async` for recursive tasks.
    ///
    /// Also note that, in the executors notation, the pool is an execution
    /// context but not an executor:
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

    /// \brief Default executor type as a constant trait for future_base
    /// functions
    using default_executor_type = default_execution_context_type::executor_type;

    /// \brief Create an instance of the default execution context
    ///
    /// \return Reference to the default execution context for @ref async
    inline default_execution_context_type &
    default_execution_context() {
#ifdef FUTURES_DEFAULT_THREAD_POOL_SIZE
        const std::size_t default_thread_pool_size
            = FUTURES_DEFAULT_THREAD_POOL_SIZE;
#else
        const std::size_t default_thread_pool_size = hardware_concurrency();
#endif
        static asio::thread_pool pool(default_thread_pool_size);
        return pool;
    }

    /// \brief Create an Asio thread pool executor for the default thread pool
    ///
    /// In the executors notation:
    /// - Executor: set of rules governing where, when and how to run a function
    /// object
    ///   - A thread pool is an execution context for which we can create
    ///   executors pointing to the pool.
    ///   - The executor rule for the default thread pool executor is to run
    ///   function objects in the pool
    ///     and nowhere else.
    ///
    /// An executor is:
    /// - Lightweight and copyable (just references and pointers to the
    /// execution context).
    /// - May be long or short lived.
    /// - May be customized on a fine-grained basis, such as exception behavior,
    /// and order
    ///
    /// There might be many executor types associated with with the same
    /// execution context.
    ///
    /// \return Executor handle to the default execution context
    inline default_execution_context_type::executor_type
    make_default_executor() {
        asio::thread_pool &pool = default_execution_context();
        return pool.executor();
    }

    /** @} */ // \addtogroup executors Executors
} // namespace futures

#endif // FUTURES_DEFAULT_EXECUTOR_H

// #include <futures/futures/stop_token.h>
#ifndef FUTURES_STOP_TOKEN_H
#define FUTURES_STOP_TOKEN_H

/// \file
///
/// This header contains is an adapted version of std::stop_token for futures
/// rather than threads.
///
/// The main difference in this implementation is 1) the reference counter does
/// not distinguish between/// tokens and sources, and 2) there is no
/// stop_callback.
///
/// The API was initially adapted from Baker Josuttis' reference implementation
/// for C++20: \see https://github.com/josuttis/jthread
///
/// Although the jfuture class is obviously different from std::jthread, this
/// stop_token is not different from std::stop_token. The main goal here is just
/// to provide a stop source in C++17. In the future, we might replace this with
/// an alias to a C++20 std::stop_token.

#include <atomic>
#include <thread>
#include <utility>
// #include <type_traits>


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
    } // namespace detail

    class stop_source;

    /// \brief Empty struct to initialize a @ref stop_source without a shared
    /// stop state
    struct nostopstate_t
    {
        explicit nostopstate_t() = default;
    };

    /// \brief Empty struct to initialize a @ref stop_source without a shared
    /// stop state
    inline constexpr nostopstate_t nostopstate{};

    /// \brief Token to check if a stop request has been made
    ///
    /// The stop_token class provides the means to check if a stop request has
    /// been made or can be made, for its associated std::stop_source object. It
    /// is essentially a thread-safe "view" of the associated stop-state.
    class stop_token
    {
    public:
        /// \name Constructors
        /// @{

        /// \brief Constructs an empty stop_token with no associated stop-state
        ///
        /// \post stop_possible() and stop_requested() are both false
        ///
        /// \param other another stop_token object to construct this stop_token
        /// object
        stop_token() noexcept = default;

        /// \brief Copy constructor.
        ///
        /// Constructs a stop_token whose associated stop-state is the same as
        /// that of other.
        ///
        /// \post *this and other share the same associated stop-state and
        /// compare equal
        ///
        /// \param other another stop_token object to construct this stop_token
        /// object
        stop_token(const stop_token &other) noexcept = default;

        /// \brief Move constructor.
        ///
        /// Constructs a stop_token whose associated stop-state is the same as
        /// that of other; other is left empty
        ///
        /// \post *this has other's previously associated stop-state, and
        /// other.stop_possible() is false
        ///
        /// \param other another stop_token object to construct this stop_token
        /// object
        stop_token(stop_token &&other) noexcept
            : shared_state_(std::exchange(other.shared_state_, nullptr)) {}

        /// \brief Destroys the stop_token object.
        ///
        /// \post If *this has associated stop-state, releases ownership of it.
        ///
        ~stop_token() = default;

        /// \brief Copy-assigns the associated stop-state of other to that of
        /// *this
        ///
        /// Equivalent to stop_token(other).swap(*this)
        ///
        /// \param other Another stop_token object to share the stop-state with
        /// to or acquire the stop-state from
        stop_token &
        operator=(const stop_token &other) noexcept {
            if (shared_state_ != other.shared_state_) {
                stop_token tmp{ other };
                swap(tmp);
            }
            return *this;
        }

        /// \brief Move-assigns the associated stop-state of other to that of
        /// *this
        ///
        /// After the assignment, *this contains the previous associated
        /// stop-state of other, and other has no associated stop-state
        ///
        /// Equivalent to stop_token(std::move(other)).swap(*this)
        ///
        /// \param other Another stop_token object to share the stop-state with
        /// to or acquire the stop-state from
        stop_token &
        operator=(stop_token &&other) noexcept {
            if (this != &other) {
                stop_token tmp{ std::move(other) };
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
        void
        swap(stop_token &other) noexcept {
            std::swap(shared_state_, other.shared_state_);
        }

        /// @}

        /// \name Observers
        /// @{

        /// \brief Checks whether the associated stop-state has been requested
        /// to stop
        ///
        /// Checks if the stop_token object has associated stop-state and that
        /// state has received a stop request. A default constructed stop_token
        /// has no associated stop-state, and thus has not had stop requested
        ///
        /// \return true if the stop_token object has associated stop-state and
        /// it received a stop request, false otherwise.
        [[nodiscard]] bool
        stop_requested() const noexcept {
            return (shared_state_ != nullptr)
                   && shared_state_->load(std::memory_order_relaxed);
        }

        /// \brief Checks whether associated stop-state can be requested to stop
        ///
        /// Checks if the stop_token object has associated stop-state, and that
        /// state either has already had a stop requested or it has associated
        /// std::stop_source object(s).
        ///
        /// A default constructed stop_token has no associated `stop-state`, and
        /// thus cannot be stopped. the associated stop-state for which no
        /// std::stop_source object(s) exist can also not be stopped if such a
        /// request has not already been made.
        ///
        /// \note If the stop_token object has associated stop-state and a stop
        /// request has already been made, this function still returns true.
        ///
        /// \return false if the stop_token object has no associated stop-state,
        /// or it did not yet receive a stop request and there are no associated
        /// std::stop_source object(s); true otherwise
        [[nodiscard]] bool
        stop_possible() const noexcept {
            return (shared_state_ != nullptr)
                   && (shared_state_->load(std::memory_order_relaxed)
                       || (shared_state_.use_count() > 1));
        }

        /// @}

        /// \name Non-member functions
        /// @{

        /// \brief compares two std::stop_token objects
        ///
        /// This function is not visible to ordinary unqualified or qualified
        /// lookup, and can only be found by argument-dependent lookup when
        /// std::stop_token is an associated class of the arguments.
        ///
        /// \param a stop_tokens to compare
        /// \param b stop_tokens to compare
        ///
        /// \return true if lhs and rhs have the same associated stop-state, or
        /// both have no associated stop-state, otherwise false
        [[nodiscard]] friend bool
        operator==(const stop_token &a, const stop_token &b) noexcept {
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
        [[nodiscard]] friend bool
        operator!=(const stop_token &a, const stop_token &b) noexcept {
            return a.shared_state_ != b.shared_state_;
        }

        /// @}

    private:
        friend class stop_source;

        /// \brief Constructor that allows the stop_source to construct the
        /// stop_token directly from the stop state
        ///
        /// \param state State for the new token
        explicit stop_token(detail::shared_stop_state state) noexcept
            : shared_state_(std::move(state)) {}

        /// \brief Shared pointer to an atomic bool indicating if an external
        /// procedure should stop
        detail::shared_stop_state shared_state_{ nullptr };
    };

    /// \brief Object used to issue a stop request
    ///
    /// The stop_source class provides the means to issue a stop request, such
    /// as for std::jthread cancellation. A stop request made for one
    /// stop_source object is visible to all stop_sources and std::stop_tokens
    /// of the same associated stop-state; any std::stop_callback(s) registered
    /// for associated std::stop_token(s) will be invoked, and any
    /// std::condition_variable_any objects waiting on associated
    /// std::stop_token(s) will be awoken.
    class stop_source
    {
    public:
        /// \name Constructors
        /// @{

        /// \brief Constructs a stop_source with new stop-state
        ///
        /// \post stop_possible() is true and stop_requested() is false
        stop_source()
            : shared_state_(std::make_shared<std::atomic_bool>(false)) {}

        /// \brief Constructs an empty stop_source with no associated stop-state
        ///
        /// \post stop_possible() and stop_requested() are both false
        explicit stop_source(nostopstate_t) noexcept {};

        /// \brief Copy constructor
        ///
        /// Constructs a stop_source whose associated stop-state is the same as
        /// that of other.
        ///
        /// \post *this and other share the same associated stop-state and
        /// compare equal
        ///
        /// \param other another stop_source object to construct this
        /// stop_source object with
        stop_source(const stop_source &other) noexcept = default;

        /// \brief Move constructor
        ///
        /// Constructs a stop_source whose associated stop-state is the same as
        /// that of other; other is left empty
        ///
        /// \post *this has other's previously associated stop-state, and
        /// other.stop_possible() is false
        ///
        /// \param other another stop_source object to construct this
        /// stop_source object with
        stop_source(stop_source &&other) noexcept
            : shared_state_(std::exchange(other.shared_state_, nullptr)) {}

        /// \brief Destroys the stop_source object.
        ///
        /// If *this has associated stop-state, releases ownership of it.
        ~stop_source() = default;

        /// \brief Copy-assigns the stop-state of other
        ///
        /// Equivalent to stop_source(other).swap(*this)
        ///
        /// \param other another stop_source object acquire the stop-state from
        stop_source &
        operator=(stop_source &&other) noexcept {
            stop_source tmp{ std::move(other) };
            swap(tmp);
            return *this;
        }

        /// \brief Move-assigns the stop-state of other
        ///
        /// Equivalent to stop_source(std::move(other)).swap(*this)
        ///
        /// \post After the assignment, *this contains the previous stop-state
        /// of other, and other has no stop-state
        ///
        /// \param other another stop_source object to share the stop-state with
        stop_source &
        operator=(const stop_source &other) noexcept {
            if (shared_state_ != other.shared_state_) {
                stop_source tmp{ other };
                swap(tmp);
            }
            return *this;
        }

        /// @}

        /// \name Modifiers
        /// @{

        /// \brief Makes a stop request for the associated stop-state, if any
        ///
        /// Issues a stop request to the stop-state, if the stop_source object
        /// has a stop-state, and it has not yet already had stop requested.
        ///
        /// The determination is made atomically, and if stop was requested, the
        /// stop-state is atomically updated to avoid race conditions, such
        /// that:
        ///
        /// - stop_requested() and stop_possible() can be concurrently invoked
        /// on other stop_tokens and stop_sources of the same stop-state
        /// - request_stop() can be concurrently invoked on other stop_source
        /// objects, and only one will actually perform the stop request.
        ///
        /// \return true if the stop_source object has a stop-state and this
        /// invocation made a stop request (the underlying atomic value was
        /// successfully changed), otherwise false
        bool
        request_stop() noexcept {
            if (shared_state_ != nullptr) {
                bool expected = false;
                return shared_state_->compare_exchange_strong(
                    expected,
                    true,
                    std::memory_order_relaxed);
            }
            return false;
        }

        /// \brief Swaps two stop_source objects
        /// \param other stop_source to exchange the contents with
        void
        swap(stop_source &other) noexcept {
            std::swap(shared_state_, other.shared_state_);
        }

        /// @}

        /// \name Non-member functions
        /// @{

        /// \brief Returns a stop_token for the associated stop-state
        ///
        /// Returns a stop_token object associated with the stop_source's
        /// stop-state, if the stop_source has stop-state, otherwise returns a
        /// default-constructed (empty) stop_token.
        ///
        /// \return A stop_token object, which will be empty if
        /// this->stop_possible() == false
        [[nodiscard]] stop_token
        get_token() const noexcept {
            return stop_token{ shared_state_ };
        }

        /// \brief Checks whether the associated stop-state has been requested
        /// to stop
        ///
        /// Checks if the stop_source object has a stop-state and that state has
        /// received a stop request.
        ///
        /// \return true if the stop_token object has a stop-state, and it has
        /// received a stop request, false otherwise
        [[nodiscard]] bool
        stop_requested() const noexcept {
            return (shared_state_ != nullptr)
                   && shared_state_->load(std::memory_order_relaxed);
        }

        /// \brief Checks whether associated stop-state can be requested to stop
        ///
        /// Checks if the stop_source object has a stop-state.
        ///
        /// \note If the stop_source object has a stop-state and a stop request
        /// has already been made, this function still returns true.
        ///
        /// \return true if the stop_source object has a stop-state, otherwise
        /// false
        [[nodiscard]] bool
        stop_possible() const noexcept {
            return shared_state_ != nullptr;
        }

        /// @}

        /// \name Non-member functions
        /// @{

        [[nodiscard]] friend bool
        operator==(const stop_source &a, const stop_source &b) noexcept {
            return a.shared_state_ == b.shared_state_;
        }
        [[nodiscard]] friend bool
        operator!=(const stop_source &a, const stop_source &b) noexcept {
            return a.shared_state_ != b.shared_state_;
        }

        /// @}

    private:
        /// \brief Shared pointer to an atomic bool indicating if an external
        /// procedure should stop
        detail::shared_stop_state shared_state_{ nullptr };
    };

    /** @} */ // \addtogroup cancellation Cancellation
    /** @} */ // \addtogroup futures Futures
} // namespace futures

#endif // FUTURES_STOP_TOKEN_H

// #include <futures/futures/traits/is_executor_then_function.h>
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
} // namespace futures

namespace futures::detail {
    /// \brief Check if types are an executor then a function
    /// The function should be invocable with the given args, the executor be an
    /// executor, and vice-versa
    template <class E, class F, typename... Args>
    using is_executor_then_function = std::conjunction<
        asio::is_executor<E>,
        std::negation<asio::is_executor<F>>,
        std::negation<std::is_invocable<E, Args...>>,
        std::is_invocable<F, Args...>>;

    template <class E, class F, typename... Args>
    constexpr bool is_executor_then_function_v
        = is_executor_then_function<E, F, Args...>::value;

    template <class E, class F, typename... Args>
    using is_executor_then_stoppable_function = std::conjunction<
        asio::is_executor<E>,
        std::negation<asio::is_executor<F>>,
        std::negation<std::is_invocable<E, stop_token, Args...>>,
        std::is_invocable<F, stop_token, Args...>>;

    template <class E, class F, typename... Args>
    constexpr bool is_executor_then_stoppable_function_v
        = is_executor_then_stoppable_function<E, F, Args...>::value;

    template <class F, typename... Args>
    using is_invocable_non_executor = std::conjunction<
        std::negation<asio::is_executor<F>>,
        std::is_invocable<F, Args...>>;

    template <class F, typename... Args>
    constexpr bool is_invocable_non_executor_v
        = is_invocable_non_executor<F, Args...>::value;

    template <class F, typename... Args>
    using is_stoppable_invocable_non_executor = std::conjunction<
        std::negation<asio::is_executor<F>>,
        std::is_invocable<F, stop_token, Args...>>;

    template <class F, typename... Args>
    constexpr bool is_stoppable_invocable_non_executor_v
        = is_stoppable_invocable_non_executor<F, Args...>::value;

    template <class F, typename... Args>
    using is_async_input_non_executor = std::disjunction<
        is_invocable_non_executor<F, Args...>,
        is_stoppable_invocable_non_executor<F, Args...>>;

    template <class F, typename... Args>
    constexpr bool is_async_input_non_executor_v
        = is_async_input_non_executor<F, Args...>::value;
    /** @} */ // \addtogroup future-traits Future Traits
    /** @} */ // \addtogroup futures Futures
} // namespace futures::detail

#endif // FUTURES_IS_EXECUTOR_THEN_FUNCTION_H

// #include <futures/futures/traits/is_future.h>

// #include <futures/futures/detail/continuations_source.h>
#ifndef FUTURES_CONTINUATIONS_SOURCE_H
#define FUTURES_CONTINUATIONS_SOURCE_H

// #include <futures/futures/detail/small_vector.h>
#ifndef FUTURES_SMALL_VECTOR_H
#define FUTURES_SMALL_VECTOR_H

// #include <futures/algorithm/traits/is_input_iterator.h>
#ifndef FUTURES_ALGORITHM_TRAITS_IS_INPUT_ITERATOR_H
#define FUTURES_ALGORITHM_TRAITS_IS_INPUT_ITERATOR_H

// #include <futures/algorithm/traits/has_iterator_traits_value_type.h>
#ifndef FUTURES_ALGORITHM_TRAITS_HAS_ITERATOR_TRAITS_VALUE_TYPE_H
#define FUTURES_ALGORITHM_TRAITS_HAS_ITERATOR_TRAITS_VALUE_TYPE_H

// #include <type_traits>

#include <iterator>

namespace futures {
    /** \addtogroup algorithms Algorithms
     *  @{
     */

    /** \addtogroup traits Traits
     *  @{
     */


    /** \brief A C++17 type trait equivalent to the C++20
     * "has-iterator-traits-value-type" concept
     */
#ifdef FUTURES_DOXYGEN
    template <class T>
    using has_iterator_traits_value_type = __see_below__;
#else
    template <class T, class = void>
    struct has_iterator_traits_value_type : std::false_type
    {};

    template <class T>
    struct has_iterator_traits_value_type<
        T,
        std::void_t<typename std::iterator_traits<T>::value_type>>
        : std::true_type
    {};
#endif
    template <class T>
    bool constexpr has_iterator_traits_value_type_v
        = has_iterator_traits_value_type<T>::value;
    /** @}*/
    /** @}*/

} // namespace futures

#endif // FUTURES_ALGORITHM_TRAITS_HAS_ITERATOR_TRAITS_VALUE_TYPE_H

// #include <futures/algorithm/traits/is_indirectly_readable.h>
#ifndef FUTURES_ALGORITHM_TRAITS_IS_INDIRECTLY_READABLE_H
#define FUTURES_ALGORITHM_TRAITS_IS_INDIRECTLY_READABLE_H

// #include <futures/algorithm/traits/iter_reference.h>
#ifndef FUTURES_ALGORITHM_TRAITS_ITER_REFERENCE_H
#define FUTURES_ALGORITHM_TRAITS_ITER_REFERENCE_H

// #include <futures/algorithm/traits/iter_value.h>
#ifndef FUTURES_ALGORITHM_TRAITS_ITER_VALUE_H
#define FUTURES_ALGORITHM_TRAITS_ITER_VALUE_H

// #include <futures/algorithm/traits/has_element_type.h>
#ifndef FUTURES_ALGORITHM_TRAITS_HAS_ELEMENT_TYPE_H
#define FUTURES_ALGORITHM_TRAITS_HAS_ELEMENT_TYPE_H

// #include <type_traits>


namespace futures {
    /** \addtogroup algorithms Algorithms
     *  @{
     */

    /** \addtogroup traits Traits
     *  @{
     */

    /** \brief A C++17 type trait equivalent to the C++20 has-member-element-type
     * concept
     */
#ifdef FUTURES_DOXYGEN
    template <class T>
    using has_element_type = __see_below__;
#else
    template <class T, class = void>
    struct has_element_type : std::false_type
    {};

    template <class T>
    struct has_element_type<T, std::void_t<typename T::element_type>>
        : std::true_type
    {};
#endif
    template <class T>
    bool constexpr has_element_type_v = has_element_type<T>::value;
    /** @}*/
    /** @}*/

} // namespace futures

#endif // FUTURES_ALGORITHM_TRAITS_HAS_ELEMENT_TYPE_H

// #include <futures/algorithm/traits/has_iterator_traits_value_type.h>

// #include <futures/algorithm/traits/has_value_type.h>
#ifndef FUTURES_ALGORITHM_TRAITS_HAS_VALUE_TYPE_H
#define FUTURES_ALGORITHM_TRAITS_HAS_VALUE_TYPE_H

// #include <type_traits>


namespace futures {
    /** \addtogroup algorithms Algorithms
     *  @{
     */

    /** \addtogroup traits Traits
     *  @{
     */


    /** \brief A C++17 type trait equivalent to the C++20 has-member-value-type concept
     */
#ifdef FUTURES_DOXYGEN
    template <class T>
    using has_value_type = __see_below__;
#else
    template <class T, class = void>
    struct has_value_type : std::false_type
    {};

    template <class T>
    struct has_value_type<T, std::void_t<typename T::value_type>>
        : std::true_type
    {};
#endif
    template <class T>
    bool constexpr has_value_type_v = has_value_type<T>::value;
    /** @}*/
    /** @}*/

} // namespace futures

#endif // FUTURES_ALGORITHM_TRAITS_HAS_VALUE_TYPE_H

// #include <futures/algorithm/traits/remove_cvref.h>
#ifndef FUTURES_ALGORITHM_TRAITS_REMOVE_CVREF_H
#define FUTURES_ALGORITHM_TRAITS_REMOVE_CVREF_H

// #include <type_traits>


namespace futures {
    /** \addtogroup algorithms Algorithms
     *  @{
     */

    /** \addtogroup traits Traits
     *  @{
     */
    /** \brief A C++17 type trait equivalent to the C++20 remove_cvref
     * concept
     */
#ifdef FUTURES_DOXYGEN
    template <class T>
    using remove_cvref = __see_below__;
#else
    template <class T>
    struct remove_cvref
    {
        using type = std::remove_cv_t<std::remove_reference_t<T>>;
    };
#endif

    template <class T>
    using remove_cvref_t = typename remove_cvref<T>::type;

    /** @}*/
    /** @}*/

} // namespace futures

#endif // FUTURES_ALGORITHM_TRAITS_REMOVE_CVREF_H

// #include <iterator>

// #include <type_traits>


namespace futures {
    /** \addtogroup algorithms Algorithms
     *  @{
     */

    /** \addtogroup traits Traits
     *  @{
     */


    /** \brief A C++17 type trait equivalent to the C++20 iter_value
     * concept
     */
#ifdef FUTURES_DOXYGEN
    template <class T>
    using iter_value = __see_below__;
#else
    template <class T, class = void>
    struct iter_value
    {};

    template <class T>
    struct iter_value<
        T,
        std::enable_if_t<has_iterator_traits_value_type_v<remove_cvref_t<T>>>>
    {
        using type = typename std::iterator_traits<
            remove_cvref_t<T>>::value_type;
    };

    template <class T>
    struct iter_value<
        T,
        std::enable_if_t<
            !has_iterator_traits_value_type_v<
                remove_cvref_t<T>> && std::is_pointer_v<T>>>
    {
        using type = decltype(*std::declval<std::remove_cv_t<T>>());
    };

    template <class T>
    struct iter_value<
        T,
        std::enable_if_t<
            !has_iterator_traits_value_type_v<remove_cvref_t<
                T>> && !std::is_pointer_v<T> && std::is_array_v<T>>>
    {
        using type = std::remove_cv_t<std::remove_extent_t<T>>;
    };

    template <class T>
    struct iter_value<
        T,
        std::enable_if_t<
            !has_iterator_traits_value_type_v<remove_cvref_t<
                T>> && !std::is_pointer_v<T> && !std::is_array_v<T> && std::is_const_v<T>>>
    {
        using type = typename iter_value<std::remove_const_t<T>>::type;
    };

    template <class T>
    struct iter_value<
        T,
        std::enable_if_t<
            !has_iterator_traits_value_type_v<remove_cvref_t<
                T>> && !std::is_pointer_v<T> && !std::is_array_v<T> && !std::is_const_v<T> && has_value_type_v<T>>>
    {
        using type = typename T::value_type;
    };

    template <class T>
    struct iter_value<
        T,
        std::enable_if_t<
            !has_iterator_traits_value_type_v<remove_cvref_t<
                T>> && !std::is_pointer_v<T> && !std::is_array_v<T> && !std::is_const_v<T> && !has_value_type_v<T> && has_element_type_v<T>>>
    {
        using type = typename T::element_type;
    };

#endif
    template <class T>
    using iter_value_t = typename iter_value<T>::type;

    /** @}*/
    /** @}*/

} // namespace futures

#endif // FUTURES_ALGORITHM_TRAITS_ITER_VALUE_H

// #include <iterator>

// #include <type_traits>


namespace futures {
    /** \addtogroup algorithms Algorithms
     *  @{
     */

    /** \addtogroup traits Traits
     *  @{
     */


    /** \brief A C++17 type trait equivalent to the C++20 iter_reference
     * concept
     */
#ifdef FUTURES_DOXYGEN
    template <class T>
    using iter_reference = __see_below__;
#else
    template <class T, class = void>
    struct iter_reference
    {};

    template <class T>
    struct iter_reference<T, std::void_t<iter_value_t<T>>>
    {
        using type = std::add_lvalue_reference<iter_value_t<T>>;
    };

#endif
    template <class T>
    using iter_reference_t = typename iter_reference<T>::type;

    /** @}*/
    /** @}*/

} // namespace futures

#endif // FUTURES_ALGORITHM_TRAITS_ITER_REFERENCE_H

// #include <futures/algorithm/traits/iter_rvalue_reference.h>
#ifndef FUTURES_ALGORITHM_TRAITS_ITER_RVALUE_REFERENCE_H
#define FUTURES_ALGORITHM_TRAITS_ITER_RVALUE_REFERENCE_H

// #include <futures/algorithm/traits/iter_value.h>

// #include <iterator>

// #include <type_traits>


namespace futures {
    /** \addtogroup algorithms Algorithms
     *  @{
     */

    /** \addtogroup traits Traits
     *  @{
     */


    /** \brief A C++17 type trait equivalent to the C++20 iter_rvalue_reference
     * concept
     */
#ifdef FUTURES_DOXYGEN
    template <class T>
    using iter_rvalue_reference = __see_below__;
#else
    template <class T, class = void>
    struct iter_rvalue_reference
    {};

    template <class T>
    struct iter_rvalue_reference<T, std::void_t<iter_value_t<T>>>
    {
        using type = std::add_rvalue_reference<iter_value_t<T>>;
    };
#endif
    template <class T>
    using iter_rvalue_reference_t = typename iter_rvalue_reference<T>::type;

    /** @}*/
    /** @}*/

} // namespace futures

#endif // FUTURES_ALGORITHM_TRAITS_ITER_RVALUE_REFERENCE_H

// #include <futures/algorithm/traits/iter_value.h>

// #include <type_traits>


namespace futures {
    /** \addtogroup algorithms Algorithms
     *  @{
     */

    /** \addtogroup traits Traits
     *  @{
     */


    /** \brief A C++17 type trait equivalent to the C++20 indirectly_readable
     * concept
     */
#ifdef FUTURES_DOXYGEN
    template <class T>
    using is_indirectly_readable = __see_below__;
#else
    template <class T, class = void>
    struct is_indirectly_readable : std::false_type
    {};

    template <class T>
    struct is_indirectly_readable<
        T,
        std::void_t<
            iter_value_t<T>,
            iter_reference_t<T>,
            iter_rvalue_reference_t<T>,
            decltype(*std::declval<T>())>> : std::true_type
    {};
#endif
    template <class T>
    bool constexpr is_indirectly_readable_v = is_indirectly_readable<T>::value;
    /** @}*/
    /** @}*/

} // namespace futures

#endif // FUTURES_ALGORITHM_TRAITS_IS_INDIRECTLY_READABLE_H

// #include <futures/algorithm/traits/is_input_or_output_iterator.h>
#ifndef FUTURES_ALGORITHM_TRAITS_IS_INPUT_OR_OUTPUT_ITERATOR_H
#define FUTURES_ALGORITHM_TRAITS_IS_INPUT_OR_OUTPUT_ITERATOR_H

// #include <type_traits>


namespace futures {
    /** \addtogroup algorithms Algorithms
     *  @{
     */

    /** \addtogroup traits Traits
     *  @{
     */


    /** \brief A C++17 type trait equivalent to the C++20 input_or_output_iterator
     * concept
     */
#ifdef FUTURES_DOXYGEN
    template <class T>
    using is_input_or_output_iterator = __see_below__;
#else
    template <class T, class = void>
    struct is_input_or_output_iterator : std::false_type
    {};

    template <class T>
    struct is_input_or_output_iterator<
        T,
        std::void_t<decltype(*std::declval<T>())>> : std::true_type
    {};
#endif
    template <class T>
    bool constexpr is_input_or_output_iterator_v = is_input_or_output_iterator<
        T>::value;

    /** @}*/
    /** @}*/

} // namespace futures

#endif // FUTURES_ALGORITHM_TRAITS_IS_INPUT_OR_OUTPUT_ITERATOR_H

// #include <type_traits>


namespace futures {
    /** \addtogroup algorithms Algorithms
     *  @{
     */

    /** \addtogroup traits Traits
     *  @{
     */


    /** \brief A C++17 type trait equivalent to the C++20 input_iterator concept
     */
#ifdef FUTURES_DOXYGEN
    template <class T>
    using is_input_iterator = __see_below__;
#else
    template <class T>
    struct is_input_iterator
        : std::conjunction<
              is_input_or_output_iterator<T>,
              is_indirectly_readable<T>>
    {};
#endif
    template <class T>
    bool constexpr is_input_iterator_v = is_input_iterator<T>::value;
    /** @}*/
    /** @}*/

} // namespace futures

#endif // FUTURES_ALGORITHM_TRAITS_IS_INPUT_ITERATOR_H

// #include <futures/algorithm/traits/is_range.h>
#ifndef FUTURES_ALGORITHM_TRAITS_IS_RANGE_H
#define FUTURES_ALGORITHM_TRAITS_IS_RANGE_H

// #include <type_traits>


namespace futures {
    /** \addtogroup algorithms Algorithms
     *  @{
     */

    /** \addtogroup traits Traits
     *  @{
     */


    /** \brief A C++17 type trait equivalent to the C++20 range concept
     */
#ifdef FUTURES_DOXYGEN
    template <class T>
    using is_range = __see_below__;
#else
    template <class T, class = void>
    struct is_range : std::false_type
    {};

    template <class T>
    struct is_range<
        T,
        std::void_t<
            // clang-format off
            decltype(*begin(std::declval<T>())),
            decltype(*end(std::declval<T>()))
            // clang-format on
            >> : std::true_type
    {};
#endif
    template <class T>
    bool constexpr is_range_v = is_range<T>::value;

    /** @}*/
    /** @}*/

} // namespace futures

#endif // FUTURES_ALGORITHM_TRAITS_IS_RANGE_H

// #include <futures/futures/detail/empty_base.h>

// #include <futures/futures/detail/scope_guard.h>
#ifndef FUTURES_SCOPE_GUARD_H
#define FUTURES_SCOPE_GUARD_H

// #include <futures/futures/detail/throw_exception.h>
#ifndef FUTURES_THROW_EXCEPTION_H
#define FUTURES_THROW_EXCEPTION_H

namespace futures::detail {
    /** \addtogroup futures::detail Futures
     *  @{
     */

    /// \brief Throw an exception but terminate if we can't throw
    template <typename Ex>
    [[noreturn]] void
    throw_exception(Ex &&ex) {
#ifndef FUTURES_DISABLE_EXCEPTIONS
        throw static_cast<Ex &&>(ex);
#else
        (void) ex;
        std::terminate();
#endif
    }

    /// \brief Construct and throw an exception but terminate otherwise
    template <typename Ex, typename... Args>
    [[noreturn]] void
    throw_exception(Args &&...args) {
        throw_exception(Ex(std::forward<Args>(args)...));
    }

    /// \brief Throw an exception but terminate if we can't throw
    template <typename ThrowFn, typename CatchFn>
    void
    catch_exception(ThrowFn &&thrower, CatchFn &&catcher) {
#ifndef FUTURES_DISABLE_EXCEPTIONS
        try {
            return static_cast<ThrowFn &&>(thrower)();
        }
        catch (std::exception &) {
            return static_cast<CatchFn &&>(catcher)();
        }
#else
        return static_cast<ThrowFn &&>(thrower)();
#endif
    }

} // namespace futures::detail
#endif // FUTURES_THROW_EXCEPTION_H

#include <functional>
#include <iostream>
// #include <utility>


namespace futures {
    namespace detail {
        /// \brief The common functions in a scope guard
        /// \note Adapted from folly / abseil
        class scope_guard_impl_base
        {
        public:
            /// \brief Tell the scope guard not to run the function
            void
            dismiss() noexcept {
                dismissed_ = true;
            }

            /// \brief Tell the scope guard to run the function again
            void
            rehire() noexcept {
                dismissed_ = false;
            }

        protected:
            /// Create the guard
            explicit scope_guard_impl_base(bool dismissed = false) noexcept
                : dismissed_(dismissed) {}

            /// Terminate if we have an exception
            inline static void
            terminate() noexcept {
                // Ensure the availability of std::cerr
                std::ios_base::Init ioInit;
                std::cerr << "This program will now terminate because a "
                             "scope_guard callback "
                             "threw an \nexception.\n";
                std::rethrow_exception(std::current_exception());
            }

            static scope_guard_impl_base
            make_empty_scope_guard() noexcept {
                return scope_guard_impl_base{};
            }

            template <typename T>
            static const T &
            as_const(const T &t) noexcept {
                return t;
            }

            bool dismissed_;
        };

        /// \brief A scope guard that calls a function when being destructed
        /// unless told otherwise
        template <typename FunctionType, bool InvokeNoexcept>
        class scope_guard_impl : public scope_guard_impl_base
        {
        public:
            explicit scope_guard_impl(FunctionType &fn) noexcept(
                std::is_nothrow_copy_constructible<FunctionType>::value)
                : scope_guard_impl(
                    as_const(fn),
                    makeFailsafe(
                        std::is_nothrow_copy_constructible<FunctionType>{},
                        &fn)) {}

            explicit scope_guard_impl(const FunctionType &fn) noexcept(
                std::is_nothrow_copy_constructible<FunctionType>::value)
                : scope_guard_impl(
                    fn,
                    makeFailsafe(
                        std::is_nothrow_copy_constructible<FunctionType>{},
                        &fn)) {}

            explicit scope_guard_impl(FunctionType &&fn) noexcept(
                std::is_nothrow_move_constructible<FunctionType>::value)
                : scope_guard_impl(
                    std::move_if_noexcept(fn),
                    makeFailsafe(
                        std::is_nothrow_move_constructible<FunctionType>{},
                        &fn)) {}

            /// A tag for a dismissed scope guard
            struct scope_guard_dismissed
            {};

            explicit scope_guard_impl(
                FunctionType &&fn,
                scope_guard_dismissed) noexcept(std::
                                                    is_nothrow_move_constructible<
                                                        FunctionType>::value)
                // No need for failsafe in this case, as the guard is dismissed.
                : scope_guard_impl_base{ true },
                  function_(std::forward<FunctionType>(fn)) {}

            scope_guard_impl(scope_guard_impl &&other) noexcept(
                std::is_nothrow_move_constructible<FunctionType>::value)
                : function_(std::move_if_noexcept(other.function_)) {
                // If the above line attempts a copy and the copy throws, other
                // is left owning the cleanup action and will execute it (or
                // not) depending on the value of other.dismissed_. The
                // following lines only execute if the move/copy succeeded, in
                // which case *this assumes ownership of the cleanup action and
                // dismisses other.
                dismissed_ = std::exchange(other.dismissed_, true);
            }

            ~scope_guard_impl() noexcept(InvokeNoexcept) {
                if (!dismissed_) {
                    execute();
                }
            }

        private:
            static scope_guard_impl_base
            makeFailsafe(std::true_type, const void *) noexcept {
                return make_empty_scope_guard();
            }

            template <typename Fn>
            static auto
            makeFailsafe(std::false_type, Fn *fn) noexcept
                -> scope_guard_impl<decltype(std::ref(*fn)), InvokeNoexcept> {
                return scope_guard_impl<decltype(std::ref(*fn)), InvokeNoexcept>{
                    std::ref(*fn)
                };
            }

            template <typename Fn>
            explicit scope_guard_impl(Fn &&fn, scope_guard_impl_base &&failsafe)
                : scope_guard_impl_base{}, function_(std::forward<Fn>(fn)) {
                failsafe.dismiss();
            }

            void *operator new(std::size_t) = delete;

            void
            execute() noexcept(InvokeNoexcept) {
                if (InvokeNoexcept) {
                    using R = decltype(function_());
                    auto catcher_word = reinterpret_cast<uintptr_t>(&terminate);
                    auto catcher = reinterpret_cast<R (*)()>(catcher_word);
                    catch_exception(function_, catcher);
                } else {
                    function_();
                }
            }

            FunctionType function_;
        };

        /// A decayed type scope guard
        template <typename F, bool InvokeNoExcept>
        using scope_guard_impl_decay
            = scope_guard_impl<typename std::decay<F>::type, InvokeNoExcept>;

    } // namespace detail

    /// \brief The default scope guard alias for a function
    template <class F>
    using scope_guard = detail::scope_guard_impl_decay<F, true>;

    /// \brief Make a scope guard with a function
    template <typename F>
    [[nodiscard]] scope_guard<F>
    make_guard(F &&f) noexcept(noexcept(scope_guard<F>(static_cast<F &&>(f)))) {
        return scope_guard<F>(static_cast<F &&>(f));
    }
} // namespace futures

#endif // FUTURES_SCOPE_GUARD_H

// #include <futures/futures/detail/throw_exception.h>

#include <cassert>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <new>
#include <ratio>

namespace futures::detail {
    /// \brief Vector of elements with a buffer for small vectors
    ///
    /// A vector optimized for the case when it's small.
    ///
    /// - Inline allocation for small number of elements
    /// - Custom expected size
    /// - Special treatment of relocatable types
    ///   - Relocatable types are defined by default for POD types and aggregate
    ///   types of PODs
    ///   - Some types might be dynamically relocatable (don't have internal
    ///   pointers all the time)
    /// - Better grow factors
    /// - Consider the cache line size in allocations
    /// (https://github.com/NickStrupat/CacheLineSize)
    ///
    /// When there are less elements than a given threshold,
    /// the elements are kept in a stack buffer for small vectors.
    /// Otherwise, it works as usual. This makes cache locality
    /// even better.
    ///
    /// This is what is usually used as a data structure for
    /// small vector. However, if you are 100% sure you will never
    /// need more than N elements, a max_size_vector is a more
    /// appropriate container.
    ///
    /// At worst, a std::vector<T> would take sizeof(std::vector<T>)
    /// bytes. So a small vector with sizeof(std::vector<T>)/sizeof(T)
    /// elements is a very conservative default because you would have
    /// using sizeof(std::vector<T>) bytes on the stack anyway.
    /// For instance:
    /// - sizeof(std::vector<int>)/sizeof(int) == 6
    /// - sizeof(std::vector<int>)/sizeof(int) == 24
    /// - sizeof(std::vector<float>)/sizeof(float) == 6
    ///
    /// This works well as a conservative default for fundamental data
    /// structures, but this will result in 1 for most other data
    /// structures. Thus, we also use a minimum of 5 elements for
    /// this default because even the smallest vectors are probably
    /// going to have at least 5 elements.
    ///
    /// This implementation was used folly, abseil, and LLVM as a reference.
    ///
    /// \tparam T Array type
    /// \tparam N Array maximum expected size
    template <
        class T,
        size_t N
        = std::max(std::size_t(5), (sizeof(T *) + sizeof(size_t)) / sizeof(T)),
        class Allocator = std::allocator<T>,
        class AllowHeap = std::true_type,
        class SizeType = size_t,
        class GrowthFactor = std::ratio<3, 2>>
    class small_vector : public maybe_empty<Allocator>
    {
    public:
        /// \name Common container types
        using size_type = SizeType;
        using value_type = T;
        using allocator_type = Allocator;
        using reference = value_type &;
        using const_reference = const value_type &;
        using difference_type = ptrdiff_t;
        using pointer = value_type *;
        using const_pointer = const value_type *;
        using iterator = pointer;
        using const_iterator = const_pointer;
        using reverse_iterator = std::reverse_iterator<iterator>;
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;

        static_assert(
            std::is_unsigned_v<size_t>,
            "Size type should be an unsigned integral type");

        static_assert(
            (std::is_same_v<typename allocator_type::value_type, value_type>),
            "Allocator::value_type must be same type as value_type");

        /// \name Rule of five constructors

        /// \brief Destructor
        /// Deallocate memory if it's not inline
        ~small_vector() {
            if constexpr (!std::is_trivially_destructible_v<T>) {
                for (auto &t: *this) {
                    (&t)->~value_type();
                }
            }
            free_heap();
        }

        /// \brief Copy constructor
        small_vector(const small_vector &rhs)
            : maybe_empty<Allocator>(
                std::allocator_traits<allocator_type>::
                    select_on_container_copy_construction(
                        rhs.maybe_empty<Allocator>::get())) {
            if constexpr (should_copy_inline) {
                if (rhs.is_inline()) {
                    copy_inline_trivial(rhs);
                    return;
                }
            }

            auto n = rhs.size();
            make_size(n);
            if constexpr (std::is_trivially_copy_constructible_v<value_type>) {
                std::memcpy(
                    (void *) begin(),
                    (void *) rhs.begin(),
                    n * sizeof(T));
            } else {
                auto rollback = make_guard([this] { free_heap(); });
                std::uninitialized_copy(rhs.begin(), rhs.begin() + n, begin());
                rollback.dismiss();
            }
            this->set_internal_size(n);
        }

        /// \brief Move constructor
        small_vector(small_vector &&rhs) noexcept
            : maybe_empty<Allocator>(
                std::allocator_traits<
                    Allocator>::propagate_on_container_move_assignment::value ?
                    std::move(rhs.maybe_empty<Allocator>::get()) :
                    rhs.get()) {
            if (rhs.is_external()) {
                this->data_.heap_storage_.pointer_ = rhs.data_.heap_storage_
                                                         .pointer_;
                rhs.data_.heap_storage_.pointer_ = nullptr;
                size_ = rhs.size_;
                rhs.size_ = 0;
                this->data_.set_capacity(rhs.data_.get_capacity());
                rhs.data_.set_capacity(0);
            } else {
                if (should_copy_inline) {
                    copy_inline_trivial(rhs);
                    rhs.size_ = 0;
                } else {
                    auto n = rhs.size();
                    if constexpr (std::is_trivially_move_constructible_v<
                                      value_type>) {
                        std::memcpy(
                            (void *) begin(),
                            (void *) rhs.begin(),
                            n * sizeof(T));
                    } else {
                        std::uninitialized_copy(
                            std::make_move_iterator(rhs.begin()),
                            std::make_move_iterator(rhs.end()),
                            begin());
                    }
                    this->set_internal_size(n);
                    rhs.clear();
                }
            }
        }

        /// \brief Copy assignment
        small_vector &
        operator=(const small_vector &rhs) {
            if (this == &rhs) {
                return *this;
            }
            if constexpr (std::allocator_traits<allocator_type>::
                              propagate_on_container_copy_assignment::value)
            {
                maybe_empty<Allocator>::get() = rhs.alloc_;
            }

            if constexpr (should_copy_inline) {
                if (this->is_inline() && rhs.is_inline()) {
                    // cheap copy the inline buffer
                    copy_inline_trivial(rhs);
                    return *this;
                }
            }

            if (rhs.size() < capacity()) {
                // rhs fits in lhs capacity
                const size_t n = rhs.size();
                if constexpr (std::is_trivially_copy_assignable_v<value_type>) {
                    std::memcpy(
                        (void *) begin(),
                        (void *) rhs.begin(),
                        n * sizeof(T));
                } else {
                    partially_uninitialized_copy(rhs.begin(), n, begin(), size());
                }
                this->set_internal_size(n);
            } else {
                // rhs does not fit in lhs current capacity
                assign(rhs.begin(), rhs.end());
            }

            return *this;
        }

        /// \brief Move assignment
        small_vector &
        operator=(small_vector &&rhs) noexcept(
            std::is_nothrow_move_constructible_v<
                value_type> && (allocator_type::propagate_on_container_move_assignment::value || allocator_type::is_always_equal::value)) {
            if constexpr (!std::allocator_traits<allocator_type>::
                              propagate_on_container_move_assignment::value)
            {
                assign(
                    std::make_move_iterator(rhs.begin()),
                    std::make_move_iterator(rhs.end()));
            } else {
                if constexpr (std::is_empty_v<allocator_type>) {
                    maybe_empty<Allocator>::get() = rhs.maybe_empty<
                        Allocator>::get();
                } else {
                    maybe_empty<Allocator>::get() = (std::move(rhs.alloc_));
                }
                move_internals(std::move(rhs));
            }
            return *this;
        }

    private:
        void
        move_internals(small_vector &&rhs) {
            if (this != &rhs) {
                if (this->is_external() || rhs.is_external()) {
                    reset();
                }
                if (rhs.is_inline()) {
                    if (should_copy_inline) {
                        copy_inline_trivial(rhs);
                        rhs.size_ = 0;
                    } else {
                        const size_t n = rhs.size();
                        if constexpr (std::is_trivially_move_assignable_v<
                                          value_type>) {
                            std::memcpy(
                                (void *) data_.buffer(),
                                (void *) rhs.data_.buffer(),
                                n * sizeof(T));
                        } else {
                            partially_uninitialized_copy(
                                std::make_move_iterator(rhs.data_.buffer()),
                                n,
                                this->data_.buffer(),
                                size());
                        }
                        this->set_internal_size(n);
                        rhs.clear();
                    }
                } else {
                    // rhs is external
                    this->data_.heap_storage_.pointer_ = rhs.data_.heap_storage_
                                                             .pointer_;
                    rhs.data_.heap_storage_.pointer_ = nullptr;
                    // this was already reset above, so it's empty and
                    // internal.
                    size_ = rhs.size_;
                    rhs.size_ = 0;
                    this->data_.set_capacity(rhs.data_.get_capacity());
                    rhs.data_.set_capacity(0);
                }
            }
        }

        /// \name Lambda constructor from which we can construct
        template <
            typename InitFunc,
            std::enable_if_t<std::is_invocable_v<InitFunc, void *>, int> = 0>
        small_vector(size_type n, InitFunc &&func, const allocator_type &alloc)
            : maybe_empty<Allocator>() {
            maybe_empty<Allocator>::get() = (alloc);
            make_size(n);
            assert(size() == 0);
            this->increment_internal_size(n);
            {
                auto rollback = make_guard([&] { free_heap(); });
                populate_mem_forward(data(), n, std::forward<InitFunc>(func));
                rollback.dismiss();
            }
            assert(invariants());
        }

    public /* constructors */:
        /// \name Initialization constructors

        /// \brief Construct empty small array
        /// Create the default allocator
        constexpr small_vector() noexcept(
            std::is_nothrow_default_constructible_v<allocator_type>)
            : small_vector(allocator_type()) {}

        /// \brief Construct small vector with a given allocator
        constexpr explicit small_vector(const allocator_type &alloc)
            : small_vector(0, alloc) {}

        /// \brief Construct small array with size n
        /// Any of the n values should be constructed with new (p) value_type();
        constexpr explicit small_vector(
            size_type n,
            const allocator_type &alloc = allocator_type())
            : small_vector(
                n,
                [&](void *p) { new (p) value_type(); },
                alloc) {}

        /// \brief Construct small array with size n and fill with single value
        constexpr small_vector(
            size_type n,
            const value_type &value,
            const allocator_type &alloc = allocator_type())
            : small_vector(
                n,
                [&](void *p) { new (p) value_type(value); },
                alloc) {}

        /// \brief Construct small array from a pair of iterators
        template <
            class Iterator,
            std::enable_if_t<is_input_iterator_v<Iterator>, int> = 0>
        constexpr small_vector(
            Iterator first,
            Iterator last,
            const allocator_type &alloc = allocator_type()) {
            maybe_empty<Allocator>::get() = alloc;
            // Handle input iterators
            constexpr bool is_input_iterator = std::is_same_v<
                typename std::iterator_traits<Iterator>::iterator_category,
                std::input_iterator_tag>;
            if (is_input_iterator) {
                while (first != last) {
                    emplace_back(*first++);
                }
                return;
            }

            // Handle inline small_vector
            size_type distance = std::distance(first, last);
            if (distance <= num_inline_elements) {
                this->increment_internal_size(distance);
                populate_mem_forward(data_.buffer(), distance, [&](void *p) {
                    new (p) value_type(*first++);
                });
                return;
            }

            // Handle external small_vector
            make_size(distance);
            this->increment_internal_size(distance);
            {
                auto rollback = make_guard([&] { free_heap(); });
                populate_mem_forward(data_.heap(), distance, [&](void *p) {
                    new (p) value_type(*first++);
                });
                rollback.dismiss();
            }

            assert(invariants());
        }

        /// \brief Construct small array from initializer list
        constexpr small_vector(
            std::initializer_list<value_type> il,
            const allocator_type &alloc = allocator_type())
            : small_vector(il.begin(), il.end(), alloc) {}

        /// \brief Construct small array from a range
        /// This range might also be a std::vector
        template <class Range, std::enable_if_t<is_range_v<Range>, int> = 0>
        constexpr explicit small_vector(
            Range &&r,
            const allocator_type &alloc = allocator_type())
            : small_vector(r.begin(), r.end(), alloc) {}

        /// \brief Assign small array from initializer list
        constexpr small_vector &
        operator=(std::initializer_list<value_type> il) {
            assign(il.begin(), il.end());
            return *this;
        }

        /// \brief Assign small array from iterators
        template <
            class Iterator,
            std::enable_if_t<is_input_iterator_v<Iterator>, int> = 0>
        constexpr void
        assign(Iterator first, value_type last) {
            clear();
            insert(end(), first, last);
        }

        /// \brief Assign small array from size and fill with value
        constexpr void
        assign(size_type n, const value_type &u) {
            clear();
            insert(end(), n, u);
        }

        /// \brief Assign small array from initializer list
        constexpr void
        assign(std::initializer_list<value_type> il) {
            assign(il.begin(), il.end());
        }

        /// \brief Fill small array with value u
        constexpr void
        fill(const T &u) {
            std::fill(begin(), end(), u);
            assert(invariants());
        }

        /// \brief Swap the contents of two small arrays
        constexpr void
        swap(small_vector &rhs) noexcept(
            std::is_nothrow_move_constructible_v<value_type>
                &&std::is_nothrow_swappable_v<value_type>) {
            // Allow ADL on swap for our value_type.
            using std::swap;

            const bool both_external = this->is_external() && rhs.is_external();
            if (both_external) {
                std::swap(size_, rhs.size_);
                auto *tmp = data_.heap_storage_.pointer_;
                data_.heap_storage_.pointer_ = rhs.data_.heap_storage_.pointer_;
                rhs.data_.heap_storage_.pointer_ = tmp;
                const auto capacity_ = this->data_.get_capacity();
                this->set_capacity(rhs.data_.get_capacity());
                rhs.data_.set_capacity(capacity_);
                return;
            }

            bool both_inline = this->is_inline() && rhs.is_inline();
            if (both_inline) {
                auto &old_small = size() < rhs.size() ? *this : rhs;
                auto &old_large = size() < rhs.size() ? rhs : *this;

                // Swap all elements up of the smaller one
                for (size_type i = 0; i < old_small.size(); ++i) {
                    swap(old_small[i], old_large[i]);
                }

                // Move elements from the larger one
                size_type i = old_small.size();
                const size_type ci = i;
                {
                    // If it fails, destruct the old large values we haven't
                    // moved
                    auto rollback = make_guard([&] {
                        old_small.set_internal_size(i);
                        if constexpr (!std::is_trivially_destructible_v<T>) {
                            for (; i < old_large.size(); ++i) {
                                old_large[i].~value_type();
                            }
                        }
                        old_large.set_internal_size(ci);
                    });
                    // Move elements from the larger small_vector to the small
                    // one
                    for (; i < old_large.size(); ++i) {
                        auto element_address = (old_small.begin() + i);
                        new (element_address)
                            value_type(std::move(old_large[i]));
                        if constexpr (!std::is_trivially_destructible_v<T>) {
                            old_large[i].~value_type();
                        }
                    }
                    rollback.dismiss();
                }
                old_small.set_internal_size(i);
                old_large.set_internal_size(ci);
                return;
            }

            // swap an external with an internal
            auto &old_external = rhs.is_external() ? rhs : *this;
            auto &old_internal = rhs.is_external() ? *this : rhs;
            auto old_external_capacity = old_external.capacity();
            auto old_external_heap = old_external.data_.heap_storage_.pointer_;

            // Store a pointer to the old external / new internal buffer
            auto old_external_buffer = old_external.data_.buffer();

            // Move elements from old internal to old external
            size_type i = 0;
            {
                // In case anything goes wrong
                auto rollback = make_guard([&] {
                    if constexpr (!std::is_trivially_destructible_v<T>) {
                        // Destroy elements from old external
                        for (size_type kill = 0; kill < i; ++kill) {
                            old_external_buffer[kill].~value_type();
                        }
                        // Destroy elements from old internal
                        for (; i < old_internal.size(); ++i) {
                            old_internal[i].~value_type();
                        }
                    }
                    // Reset sizes
                    old_internal.size_ = 0;
                    old_external.data_.heap_storage_.pointer_
                        = old_external_heap;
                    old_external.set_capacity(old_external_capacity);
                });
                // Move elements from old internal to old external buffer
                for (; i < old_internal.size(); ++i) {
                    new (&old_external_buffer[i])
                        value_type(std::move(old_internal[i]));
                    if constexpr (!std::is_trivially_destructible_v<T>) {
                        old_internal[i].~value_type();
                    }
                }
                rollback.dismiss();
            }

            // Adjust pointers
            old_internal.data_.heap_storage_.pointer_ = old_external_heap;
            std::swap(size_, rhs.size_);
            old_internal.set_capacity(old_external_capacity);

            // Change allocators
            const bool should_swap = std::allocator_traits<
                allocator_type>::propagate_on_container_swap::value;
            if constexpr (should_swap) {
                swap_allocator(rhs.alloc_);
            }

            assert(invariants());
        }

    public /* iterators */:
        /// \brief Get iterator to first element
        constexpr iterator
        begin() noexcept {
            return iterator(data());
        }

        /// \brief Get constant iterator to first element[[nodiscard]]
        constexpr const_iterator
        begin() const noexcept {
            return const_iterator(data());
        }

        /// \brief Get iterator to last element
        constexpr iterator
        end() noexcept {
            return iterator(data() + size());
        }

        /// \brief Get constant iterator to last element
        constexpr const_iterator
        end() const noexcept {
            return const_iterator(data() + size());
        }

        /// \brief Get iterator to first element in reverse order
        constexpr reverse_iterator
        rbegin() noexcept {
            return std::reverse_iterator<iterator>(end());
        }

        /// \brief Get constant iterator to first element in reverse order
        constexpr const_reverse_iterator
        rbegin() const noexcept {
            return std::reverse_iterator<const_iterator>(end());
        }

        /// \brief Get iterator to last element in reverse order
        constexpr reverse_iterator
        rend() noexcept {
            return std::reverse_iterator<iterator>(begin());
        }

        /// \brief Get constant iterator to last element in reverse order
        constexpr const_reverse_iterator
        rend() const noexcept {
            return std::reverse_iterator<const_iterator>(begin());
        }

        /// \brief Get constant iterator to first element
        constexpr const_iterator
        cbegin() const noexcept {
            return const_iterator(begin());
        }

        /// \brief Get constant iterator to last element
        constexpr const_iterator
        cend() const noexcept {
            return cbegin() + size();
        }

        /// \brief Get constant iterator to first element in reverse order
        constexpr const_reverse_iterator
        crbegin() const noexcept {
            return std::reverse_iterator<const_iterator>(cend());
        }

        /// \brief Get constant iterator to last element in reverse order
        constexpr const_reverse_iterator
        crend() const noexcept {
            return std::reverse_iterator<const_iterator>(cbegin());
        }

    public /* capacity */:
        /// \brief Get small array size
        [[nodiscard]] constexpr size_type
        size() const noexcept {
            return get_unmasked_size();
        }

        /// \brief Get small array max size
        [[nodiscard]] constexpr size_type
        max_size() const noexcept {
            if constexpr (!should_use_heap) {
                return static_cast<size_type>(num_inline_elements);
            } else {
                constexpr size_type max_with_mask = size_type(clear_size_mask);
                return std::min<size_type>(
                    std::allocator_traits<allocator_type>::max_size(
                        maybe_empty<Allocator>::get()),
                    max_with_mask);
            }
        }

        /// \brief Get small array capacity (same as max_size())
        [[nodiscard]] constexpr size_type
        capacity() const noexcept {
            if constexpr (!should_use_heap) {
                return num_inline_elements;
            } else {
                if (is_external()) {
                    return data_.get_capacity();
                } else {
                    return num_inline_elements;
                }
            }
        }

        /// \brief Check if small array is empty
        [[nodiscard]] constexpr bool
        empty() const noexcept {
            return !size();
        }

        /// \brief Check if small_vector is inline
        [[nodiscard]] constexpr bool
        is_inline() const {
            return !is_external();
        }

        /// \brief Reserve space for n elements
        /// We concentrate the logic to switch the variant types in these
        /// functions
        void
        reserve(size_type n) {
            make_size(n);
        }

        /// \brief Reserve space for n elements
        /// We concentrate the logic to switch the variant types in these
        /// functions The behaviour of this function might change
        void
        reserve() {
            shrink_to_fit();
        }

        /// \brief Shrink internal array to fit only the current elements
        /// We concentrate the logic to switch the variant types in these
        /// functions
        void
        shrink_to_fit() noexcept {
            if (this->is_inline()) {
                return;
            }
            small_vector tmp(begin(), end());
            tmp.swap(*this);
        }

    public /* element access */:
        /// \brief Get reference to n-th element in small array
        constexpr reference
        operator[](size_type n) {
            assert(
                n < size() && "small_vector::operator[] index out of bounds");
            return *(begin() + n);
        }

        /// \brief Get constant reference to n-th element in small array
        constexpr const_reference
        operator[](size_type n) const {
            assert(
                n < size() && "small_vector::operator[] index out of bounds");
            return *(begin() + n);
        }

        /// \brief Check bounds and get reference to n-th element in small array
        constexpr reference
        at(size_type n) {
            if (n >= size()) {
                throw_exception<std::out_of_range>(
                    "at: cannot access element after small_vector::size()");
            }
            return (*this)[n];
        }

        /// \brief Check bounds and get constant reference to n-th element in
        /// small array
        constexpr const_reference
        at(size_type n) const {
            if (n >= size()) {
                throw_exception<std::out_of_range>(
                    "at const: cannot access element after "
                    "small_vector::size()");
            }
            return (*this)[n];
        }

        /// \brief Get reference to first element in small array
        constexpr reference
        front() {
            assert(!empty() && "front() called for empty small array");
            return *begin();
        }

        /// \brief Get constant reference to first element in small array
        constexpr const_reference
        front() const {
            assert(!empty() && "front() called for empty small array");
            return *begin();
        }

        /// \brief Get reference to last element in small array
        constexpr reference
        back() {
            assert(!empty() && "back() called for empty small array");
            return *(end() - 1);
        }

        /// \brief Get constant reference to last element in small array
        constexpr const_reference
        back() const {
            assert(!empty() && "back() called for empty small array");
            return *(end() - 1);
        }

        /// \brief Get reference to internal pointer to small array data
        constexpr T *
        data() noexcept {
            return this->is_external() ? data_.heap() : data_.buffer();
        }

        /// \brief Get constant reference to internal pointer to small array data
        constexpr const T *
        data() const noexcept {
            return this->is_external() ? data_.heap() : data_.buffer();
        }

    public /* modifiers */:
        /// \brief Copy element to end of small array
        constexpr void
        push_back(const value_type &v) {
            emplace_back(v);
        }

        /// \brief Move element to end of small array
        constexpr void
        push_back(value_type &&v) {
            emplace_back(std::move(v));
        }

        /// \brief Emplace element to end of small array
        template <class... Args>
        constexpr reference
        emplace_back(Args &&...args) {
            // Handle inline small_vector
            if (size_ < num_inline_elements) {
                new (data_.buffer() + size_)
                    value_type(std::forward<Args>(args)...);
                this->increment_internal_size(1);
                return *(data_.buffer() + size_);
            } else {
                if constexpr (!should_use_heap) {
                    throw_exception<std::length_error>(
                        "emplace_back: max_size exceeded in "
                        "small_small_vector");
                } else {
                    // Handle external small_vectors
                    size_type old_size = size();
                    size_type old_capacity = capacity();
                    const bool needs_to_grow = old_capacity == old_size;
                    if (needs_to_grow) {
                        // Internal small_vector
                        make_size(
                            old_size + 1,
                            [&](void *p) {
                            new (p) value_type(std::forward<Args>(args)...);
                            },
                            old_size);
                    } else {
                        // External small_vector
                        new (data_.heap() + old_size)
                            value_type(std::forward<Args>(args)...);
                    }
                    this->increment_internal_size(1);
                    return *(data_.heap() + old_size);
                }
            }
        }

        /// \brief Remove element from end of small array
        constexpr void
        pop_back() {
            destroy_and_downsize(size() - 1);
        }

        /// \brief Emplace element to a position in small array
        template <class... Args>
        constexpr iterator
        emplace(const_iterator position, Args &&...args) {
            // Special case
            const bool is_emplace_back = position == cend();
            if (is_emplace_back) {
                emplace_back(std::forward<Args>(args)...);
                return end() - 1;
            }

            // Construct and insert
            return insert(position, value_type(std::forward<Args>(args)...));
        }

        /// \brief Copy element to a position in small array
        constexpr iterator
        insert(const_iterator position, const value_type &x) {
            return insert(position, 1, x);
        }

        /// \brief Move element to a position in small array
        constexpr iterator
        insert(const_iterator position, value_type &&x) {
            iterator p = unconst(position);
            const bool is_push_back = p == end();
            if (is_push_back) {
                push_back(std::move(x));
                return end() - 1;
            }

            auto offset = p - begin();
            auto old_size = size();
            const bool must_grow = capacity() == old_size;
            if (must_grow) {
                make_size(
                    old_size + 1,
                    [&x](void *ptr) { new (ptr) value_type(std::move(x)); },
                    offset);
                this->increment_internal_size(1);
            } else {
                shift_right_and_construct(
                    data() + offset,
                    data() + old_size,
                    data() + old_size + 1,
                    [&]() mutable -> value_type && { return std::move(x); });
                this->increment_internal_size(1);
            }
            return begin() + offset;
        }

        /// \brief Copy n elements to a position in small array
        constexpr iterator
        insert(const_iterator position, size_type n, const value_type &x) {
            auto offset = position - begin();
            auto old_size = size();
            make_size(old_size + n);
            shift_right_and_construct(
                data() + offset,
                data() + old_size,
                data() + old_size + n,
                [&]() mutable -> value_type const & { return x; });
            this->increment_internal_size(n);
            return begin() + offset;
        }

        /// \brief Copy element range stating at a position in small array
        template <
            class Iterator,
            std::enable_if_t<is_input_iterator_v<Iterator>, int> = 0>
        constexpr iterator
        insert(const_iterator position, Iterator first, value_type last) {
            // Handle input iterators
            using category = typename std::iterator_traits<
                Iterator>::iterator_category;
            using it_ref = typename std::iterator_traits<Iterator>::reference;
            if constexpr (std::is_same_v<category, std::input_iterator_tag>) {
                auto offset = position - begin();
                while (first != last) {
                    position = insert(position, *first++);
                    ++position;
                }
                return begin() + offset;
            }

            // Other iterator types
            auto const distance = std::distance(first, last);
            auto const offset = position - begin();
            auto old_size = size();
            assert(distance >= 0);
            assert(offset >= 0);
            make_size(old_size + distance);
            shift_right_and_construct(
                data() + offset,
                data() + old_size,
                data() + old_size + distance,
                [&, in = last]() mutable -> it_ref { return *--in; });
            this->increment_internal_size(distance);
            return begin() + offset;
        }

        /// \brief Copy elements from initializer list to a position in small
        /// array
        constexpr iterator
        insert(const_iterator position, std::initializer_list<value_type> il) {
            return insert(position, il.begin(), il.end());
        }

        /// \brief Erase element at a position in small array
        constexpr iterator
        erase(const_iterator position) {
            return erase(position, std::next(position));
        }

        /// \brief Erase range of elements in the small array
        constexpr iterator
        erase(const_iterator first, const_iterator last) {
            if (first == last) {
                return unconst(first);
            }
            if constexpr (is_relocatable_v<value_type> && using_std_allocator) {
                // Directly destroy elements before mem moving
                if constexpr (!std::is_trivially_destructible_v<T>) {
                    for (auto it = first; it != last; ++it) {
                        it->~value_type();
                    }
                }
                // Move elements directly in memory
                const auto n_erase = last - first;
                const auto n_after_erase = cend() - last;
                if (n_erase >= n_after_erase) {
                    std::memcpy(
                        (void *) first,
                        (void *) last,
                        (cend() - last) * sizeof(T));
                } else {
                    std::memmove(
                        (void *) first,
                        (void *) last,
                        (cend() - last) * sizeof(T));
                }
            } else {
                // Move elements in memory
                std::move(unconst(last), end(), unconst(first));
            }
            // Directly set internal size. Elements are already destroyed.
            set_internal_size(size() - std::distance(first, last));
            return unconst(first);
        }

        /// \brief Clear elements in the small array
        constexpr void
        clear() noexcept {
            destroy_and_downsize(0);
        }

        /// \brief Resize the small array
        /// If we are using a small_vector to store the data, resize will not
        /// move the data back to the stack even if the new size is less
        /// than the small_vector capacity
        constexpr void
        resize(size_type n) {
            if (n <= size()) {
                destroy_and_downsize(n);
                return;
            }
            auto extra = n - size();
            make_size(n);
            populate_mem_forward((begin() + size()), extra, [&](void *p) {
                new (p) value_type();
            });
            this->increment_internal_size(extra);
        }

        /// \brief Resize and fill the small array
        constexpr void
        resize(size_type n, const value_type &v) {
            if (n < size()) {
                erase(begin() + n, end());
                return;
            }
            auto extra = n - size();
            make_size(n);
            populate_mem_forward((begin() + size()), extra, [&](void *p) {
                new (p) value_type(v);
            });
            this->increment_internal_size(extra);
        }

    private:
        /// \brief Check if small array invariants are ok
        [[nodiscard]] constexpr bool
        invariants() const {
            if (size() > capacity()) {
                return false;
            }
            if (begin() > end()) {
                return false;
            }
            if (end() > begin() + capacity()) {
                return false;
            }
            return true;
        }

        /// \name Compile-time inferences

        /// \brief Size of a pointer
        static constexpr size_t pointer_size = sizeof(value_type *);

        /// \brief Size of a size type
        static constexpr size_t size_type_size = sizeof(size_type);

        /// \brief Size of new for the heap pointers
        static constexpr size_t heap_storage_size = pointer_size
                                                    + size_type_size;

        /// \brief Size of a value
        static constexpr size_t value_size = sizeof(value_type);

        /// \brief How many values we can store in the space allocated for the
        /// heap
        static constexpr size_t inline_values_per_heap = heap_storage_size
                                                         / value_size;

        /// \brief Min reasonable inline
        /// It's reasonable that:
        /// - less than can fit is the heap pointers is a waste
        /// - 0 is doesn't make sense
        /// - 1 is no reason to create a vector
        static constexpr size_t min_reasonable_inline_elements = 2;

        /// \brief Pointers per value
        static constexpr size_t min_inline_elements = std::
            max(inline_values_per_heap, min_reasonable_inline_elements);

        /// \brief Pointers per value
        static constexpr auto requested_inline_size = N;

        /// \brief Number of elements we should inline
        /// If the requested number is below the number we can fit in the
        /// pointer itself, use that instead
        static constexpr std::size_t num_inline_elements = std::
            max(min_inline_elements, requested_inline_size);

        /// \brief The type of the raw array we would use to store inline vectors
        using raw_value_type_array = value_type[num_inline_elements];

        /// \brief Type we would use for inline storage if we do
        using inline_storage_data_type = typename std::aligned_storage_t<
            sizeof(raw_value_type_array),
            alignof(raw_value_type_array)>;

        /// \brief True if inline storage would always be empty
        static constexpr bool inline_storage_empty
            = sizeof(value_type) * num_inline_elements == 0;

        /// \brief Final inline storage type
        /// This is the inline storage data type or (very rarely) just a pointer
        using inline_storage_type = typename std::conditional_t<
            !inline_storage_empty,
            inline_storage_data_type,
            pointer>;

        /// \brief An assumption about the size of a cache line
        /// \note Clang unfortunately defines
        /// __cpp_lib_hardware_interference_size without defining
        /// hardware_constructive_interference_size.
        static constexpr std::size_t cache_line_size =
#if defined(__cpp_lib_hardware_interference_size) && !defined(__clang__)
            std::hardware_constructive_interference_size;
#else
            2 * sizeof(std::max_align_t);
#endif

        /// \brief True if we should just copy the inline storage
        static constexpr bool should_copy_inline
            = std::is_trivially_copyable_v<
                  value_type> && sizeof(inline_storage_type) <= cache_line_size / 2;

        /// \brief True if we are using the std::allocator
        static constexpr bool using_std_allocator = std::
            is_same<allocator_type, std::allocator<value_type>>::value;

        /// \brief An empty type when we want to remove a member from the class
        using empty_type = std::tuple<>;

        /// \brief Type we use to internally store the allocator
        /// We don't really represent the std::allocator in the class because it
        /// has no members We can just recreate it at get_alloc()
        using internal_allocator_type = std::
            conditional_t<using_std_allocator, empty_type, allocator_type>;

        template <class U>
        struct is_relocatable
            : std::conjunction<
                  std::is_trivially_copy_constructible<U>,
                  std::is_trivially_copy_assignable<U>,
                  std::is_trivially_destructible<U>>
        {};

        template <class U>
        static constexpr bool is_relocatable_v = is_relocatable<U>::value;

        /// \brief Use memcpy to copy items
        /// If type is relocatable, we just use memcpy
        static constexpr bool relocate_use_memcpy = is_relocatable<T>::value
                                                    && using_std_allocator;


        /// \brief Whether this vector should also use the heap
        static constexpr bool should_use_heap = AllowHeap::value;

        /// \brief A mask with the most significant bit of the size type
        static size_type constexpr size_type_most_significant_bit_mask
            = size_type(1) << (sizeof(size_type) * 8 - 1);

        /// \brief A mask to identify if the array is external
        static size_type constexpr is_external_mask
            = should_use_heap ? size_type_most_significant_bit_mask : 0;

        /// \brief Identify if we are using the heap
        /// A mask to extract the size bits identifying whether the current
        /// vector it's external The two most significant digits can tell us if
        /// something is external / allocated in the heap
        [[nodiscard]] constexpr bool
        is_external() const {
            return is_external_mask & size_;
        }

        /// \brief Set the flag indicating the vector is external
        void
        set_external(bool b) {
            if (b) {
                size_ |= is_external_mask;
            } else {
                size_ &= ~is_external_mask;
            }
        }

        /// \brief Set the capacity of the external vector
        void
        set_capacity(size_type new_capacity) {
            assert(this->is_external());
            assert(new_capacity < std::numeric_limits<size_type>::max());
            data_.set_capacity(new_capacity);
        }

        /// \brief Return the max size of this vector
        /// We remove the final digit we use to identify the use of the heap
        static size_type constexpr clear_size_mask = ~is_external_mask;

        /// \brief Set the size variable
        /// This sets the size and maintains the inline bit
        void
        set_internal_size(std::size_t sz) {
            assert(sz <= clear_size_mask);
            size_ = (is_external_mask & size_) | size_type(sz);
        }

        /// \brief Get the size value without the mask
        [[nodiscard]] constexpr size_t
        get_unmasked_size() const {
            return size_ & clear_size_mask;
        }

        /// \brief Free the vector heap
        void
        free_heap() {
            if (is_external()) {
                auto alloc_instance = maybe_empty<Allocator>::get();
                std::allocator_traits<allocator_type>::
                    deallocate(alloc_instance, data(), capacity());
                data_.heap_storage_.pointer_ = nullptr;
            }
        }

        /// \brief Copy the inline storage from rhs vector when type is
        /// trivially copyable
        template <
            class T2 = value_type,
            std::enable_if_t<std::is_trivially_copyable_v<T2>, int> = 0>
        void
        copy_inline_trivial(small_vector const &rhs) {
            // Copy the whole buffer to maintain the loop with fixed size
            std::copy(
                rhs.data_.buffer(),
                rhs.data_.buffer() + num_inline_elements,
                data_.buffer());
            this->set_internal_size(rhs.size());
        }

        /// \brief Copy the inline storage from rhs vector when type is not
        /// trivially copyable
        template <
            class T2 = value_type,
            std::enable_if_t<!std::is_trivially_copyable_v<T2>, int> = 0>
        void
        copy_inline_trivial(small_vector const &) {
            throw std::logic_error(
                "Attempting to trivially copy not trivially copyable type");
        }

        /// \brief Make it empty and with no heap
        void
        reset() {
            clear();
            free_heap();
            size_ = 0;
        }

        /// \brief Change the size to a new size
        void
        make_size(size_type new_size) {
            if (new_size <= capacity()) {
                return;
            }
            make_size_internal<std::false_type>(
                new_size,
                [](void *) {
                throw_exception<std::logic_error>(
                    "Should not emplace when changing size");
                },
                0);
        }

        /// \brief Change the size and emplace the elements as we go
        template <typename EmplaceFunc>
        void
        make_size(
            size_type new_size,
            EmplaceFunc &&emplace_function,
            size_type new_emplaced_size) {
            assert(size() == capacity());
            make_size_internal<std::true_type>(
                new_size,
                std::forward<EmplaceFunc>(emplace_function),
                new_emplaced_size);
        }

        /// \brief Change the current size to new size and emplace the elements
        /// This will heapify the vector if it's inline
        template <typename InsertVersion, typename EmplaceFunc>
        void
        make_size_internal(
            size_type new_size,
            EmplaceFunc &&emplace_func,
            size_type new_emplaced_size) {
            // Invariants
            if (new_size > max_size()) {
                throw_exception<std::length_error>(
                    "make_size: max_size exceeded in small_vector");
            } else {
                if constexpr (!should_use_heap) {
                    return;
                } else {
                    // New heap pointer
                    size_type new_capacity = std::
                        max(new_size, compute_new_size());
                    auto alloc_instance = maybe_empty<Allocator>::get();
                    value_type *new_heap_ptr = std::allocator_traits<
                        allocator_type>::allocate(alloc_instance, new_capacity);

                    // Copy data
                    {
                        auto rollback = make_guard([&] {
                            std::allocator_traits<allocator_type>::deallocate(
                                alloc_instance,
                                new_heap_ptr,
                                new_capacity);
                        });
                        if constexpr (InsertVersion::value) {
                            // move the begin()/end() range to the
                            // new_heap_ptr/new_emplaced_size range and insert
                            // the new element
                            this->move_to_uninitialized_emplace(
                                begin(),
                                end(),
                                new_heap_ptr,
                                new_emplaced_size,
                                std::forward<EmplaceFunc>(emplace_func));
                        } else {
                            // move the begin()/end() range to the range
                            // starting at new_heap_ptr
                            this->move_to_uninitialized(
                                begin(),
                                end(),
                                new_heap_ptr);
                        }
                        rollback.dismiss();
                    }

                    // Destruct values we already copied
                    if constexpr (!std::is_trivially_destructible_v<T>) {
                        for (auto &val: *this) {
                            val.~value_type();
                        }
                    }

                    // Free the old heap
                    free_heap();

                    // Store the new pointer
                    data_.heap_storage_.pointer_ = new_heap_ptr;
                    this->set_external(true);
                    this->set_capacity(new_capacity);
                }
            }
        }

        /// \brief Move from begin/end range to uninitialized range out/pos and
        /// call the emplace function at pos
        template <class EmplaceFunc>
        void
        move_to_uninitialized_emplace(
            value_type *begin,
            value_type *end,
            value_type *out,
            size_type pos,
            EmplaceFunc &&emplace_func) {
            // Emplace the element at the position
            emplace_func(out + pos);

            {
                // Move old elements to the left of the new one
                auto rollback = make_guard([&] { //
                    out[pos].~T();
                });
                this->move_to_uninitialized(begin, begin + pos, out);
                rollback.dismiss();
            }

            {
                // move old elements to the right of the new one
                auto rollback = make_guard([&] {
                    for (size_type i = 0; i <= pos; ++i) {
                        out[i].~T();
                    }
                });
                if (begin + pos < end) {
                    this->move_to_uninitialized(begin + pos, end, out + pos + 1);
                }
                rollback.dismiss();
            }
        }

        /// \brief Move the range first/last to uninitialized memory starting at
        /// out
        template <
            class T2 = value_type,
            std::enable_if_t<!std::is_trivially_copyable_v<T2>, int> = 0>
        void
        move_to_uninitialized(T *first, T *last, T *out) {
            std::size_t idx = 0;
            {
                // Call destructor for all elements we emplaced correctly if
                // something fails
                auto rollback = make_guard([&] {
                    for (std::size_t i = 0; i < idx; ++i) {
                        out[i].~T();
                    }
                });
                // Move elements
                for (; first != last; ++first, ++idx) {
                    new (&out[idx]) T(std::move(*first));
                }
                rollback.dismiss();
            }
        }

        /// \brief Move the range first/last to uninitialized memory starting at
        /// out
        template <
            class T2 = value_type,
            std::enable_if_t<std::is_trivially_copyable_v<T2>, int> = 0>
        void
        move_to_uninitialized(T *first, T *last, T *out) {
            // Just memmove all the data
            std::memmove(
                static_cast<void *>(out),
                static_cast<void const *>(first),
                (last - first) * sizeof *first);
        }

        /// \brief Compute the new size this vector should have after growing
        /// This is a growth factor of 1.5
        constexpr size_type
        compute_new_size() const {
            const auto old_capacity = capacity();
            // Set the initial capacity
            if (old_capacity == 0) {
                return std::max(64 / sizeof(value_type), size_type(5));
            }
            // Blocks larger than or equal to 4096 bytes can be expanded in place
            constexpr size_t min_in_place_expansion = 4096;
            const bool cannot_be_in_place = old_capacity
                                            < min_in_place_expansion / sizeof(T);
            if (cannot_be_in_place) {
                return capacity() * 2;
            }
            const bool very_very_large_already = capacity()
                                                 > 4096 * 32 / sizeof(T);
            if (very_very_large_already) {
                return capacity() * 2;
            }
            // Apply usual growth factor
            return std::min(
                (static_cast<size_type>(GrowthFactor::num) * capacity())
                        / static_cast<size_type>(GrowthFactor::den)
                    + 1,
                max_size());
        }

        /// \brief Copy some elements as initialized and some as uninitialized
        template <class Iterator1, class Iterator2>
        static void
        partially_uninitialized_copy(
            Iterator1 from,
            size_t from_size,
            Iterator2 to,
            size_t to_size) {
            const size_t min_size = std::min(from_size, to_size);
            std::copy(from, from + min_size, to);
            if (from_size > to_size) {
                std::uninitialized_copy(
                    from + min_size,
                    from + from_size,
                    to + min_size);
            } else {
                for (auto it = to + min_size; it != to + to_size; ++it) {
                    using Value = typename std::decay<decltype(*it)>::type;
                    it->~Value();
                }
            }
        }

        /// \brief A std::memcpy implementation that ensures the values are
        /// moved byte by byte This is what std::memcpy usually does, but some
        /// compilers implement it differently
        static void
        byte_copy(void *destination, void *source, size_t n) {
            char *char_source = (char *) source;
            char *char_destination = (char *) destination;
            std::copy(char_source, char_source + n, char_destination);
        }

        /// \brief Move elements to the right a construct at the new location
        template <
            class Construct,
            class T2 = value_type,
            std::enable_if_t<!std::is_trivially_copyable_v<T2>, int> = 0>
        void
        shift_right_and_construct(
            T *const first,
            T *const last,
            T *const new_last,
            Construct &&create) {
            // Input is same as output
            if (last == new_last) {
                return;
            }

            // Input and output for the elements moving
            T *out = new_last;
            T *in = last;
            {
                // In case anything goes wrong
                auto rollback = make_guard([&] {
                    // Destroy the out/last range
                    if (out < last) {
                        out = last - 1;
                    }
                    for (auto it = out + 1; it != new_last; ++it) {
                        it->~T();
                    }
                });

                // Move elements from "in" to uninitialized "out"
                while (in != first && out > last) {
                    // Out must be decremented before an exception can be thrown
                    // so that the rollback guard knows where to start.
                    --out;
                    --in;
                    if constexpr (
                        is_relocatable_v<value_type> && using_std_allocator) {
                        byte_copy(out, in, sizeof(value_type));
                    } else {
                        new (out) T(std::move(*in));
                    }
                }

                // Move elements from "in" to initialized "out"
                while (in != first) {
                    --out;
                    --in;
                    if constexpr (
                        is_relocatable_v<value_type> && using_std_allocator) {
                        byte_copy(out, in, sizeof(value_type));
                    } else {
                        *out = std::move(*in);
                    }
                }

                // Construct elements in uninitialized "out"
                while (out > last) {
                    --out;
                    new (out) T(create());
                }

                // Construct elements in initialized "out"
                while (out != first) {
                    --out;
                    if constexpr (
                        is_relocatable_v<value_type> && using_std_allocator) {
                        new (out) T(create());
                    } else {
                        *out = create();
                    }
                }

                rollback.dismiss();
            }
        }

        /// \brief Shirt the range [first, last] to [new_first, new_last] and
        /// fill the range [first, new_first] with the `create` function
        template <
            class Construct,
            class T2 = value_type,
            std::enable_if_t<std::is_trivially_copyable_v<T2>, int> = 0>
        void
        shift_right_and_construct(
            T *const first,
            T *const last,
            T *const new_last,
            Construct &&create) {
            // Move elements backward from [first, last] to [new_first, new_last]
            std::move_backward(first, last, new_last);

            // Create elements in the range [first, new_first] backward
            const auto n_create = new_last - last;
            if (n_create > 0) {
                T *create_out = first + n_create;
                do {
                    --create_out;
                    *create_out = create();
                }
                while (create_out != first);
            }
        }

        /// \brief Populate memory with the given function
        template <class Function>
        void
        populate_mem_forward(
            value_type *mem,
            std::size_t n,
            Function const &op) {
            std::size_t idx = 0;
            {
                auto rollback = make_guard([&] {
                    for (std::size_t i = 0; i < idx; ++i) {
                        mem[i].~T();
                    }
                });
                for (size_t i = 0; i < n; ++i) {
                    op(&mem[idx]);
                    ++idx;
                }
                rollback.dismiss();
            }
        }

        /// \brief Increment the internal size
        void
        increment_internal_size(std::size_t n) {
            assert(get_unmasked_size() + n <= max_size());
            size_ += size_type(n);
        }

        /// \brief Destroy elements after n and decrement the internal size
        /// If elements are relocatable and we are erasing elements, we should
        /// directly destroy the appropriate elements and call set_internal_size
        void
        destroy_and_downsize(std::size_t n) {
            assert(n <= size());
            // Destroy extra elements
            if constexpr (!std::is_trivially_destructible_v<T>) {
                for (auto it = (begin() + n); it != end(); ++it) {
                    it->~value_type();
                }
            }
            // Set internal size
            this->set_internal_size(n);
        }

        /// Unconst an iterator
        static iterator
        unconst(const_iterator it) {
            return iterator(const_cast<value_type *>(it));
        }

    private:
        /// \brief A pointer to the heap and the capacity of the array in the
        /// heap This is usual storage we use when the vector is not inline.
        /// This class doesn't handle allocations directly though.
        struct heap_storage_type
        {
            value_type *pointer_{ nullptr };
            size_type capacity_;
            size_type
            get_capacity() const {
                return capacity_;
            }
            void
            set_capacity(size_type c) {
                capacity_ = c;
            }
        };

        /// \brief Data type used to represent the vector
        /// This is a union that might be storing a inline or heap array at a
        /// given time
        union data_type
        {
            /// \brief Storage when the vector is inline
            inline_storage_type inline_storage_;

            /// \brief Storage when the vector is in the heap
            heap_storage_type heap_storage_;

            /// \brief By default, we have a heap element pointing to nullptr
            /// (size == 0)
            explicit data_type() {
                heap_storage_.pointer_ = nullptr;
                heap_storage_.capacity_ = 0;
            }

            /// \brief Get a pointer to the buffer if it's inline
            value_type *
            buffer() noexcept {
                void *vp = &inline_storage_;
                return static_cast<value_type *>(vp);
            }

            /// \brief Get a const pointer to the buffer if it's inline
            value_type const *
            buffer() const noexcept {
                return const_cast<data_type *>(this)->buffer();
            }

            /// \brief Get a pointer to the array if it's not inline
            value_type *
            heap() noexcept {
                return heap_storage_.pointer_;
            }

            /// \brief Get a const pointer to the array if it's not inline
            value_type const *
            heap() const noexcept {
                return heap_storage_.pointer_;
            }

            /// \brief Get the current allocated capacity if it's not inline
            size_type
            get_capacity() const {
                return heap_storage_.get_capacity();
            }

            /// \brief Set the current allocated capacity if it's not inline
            void
            set_capacity(size_type c) {
                heap_storage_.set_capacity(c);
            }
        };

        /// \brief Internal array or vector
        data_type data_{};

        /// \brief The number of elements in the storage
        size_type size_{ 0 };
    };

    /// Type deduction
    template <class T, class... U>
    small_vector(T, U...) -> small_vector<T, 1 + sizeof...(U)>;

    /// \brief operator== for small arrays
    template <class T, size_t N, class A, class H, class S>
    constexpr bool
    operator==(
        const small_vector<T, N, A, H, S> &x,
        const small_vector<T, N, A, H, S> &y) {
        return std::equal(x.begin(), x.end(), y.begin(), y.end());
    }

    /// \brief operator!= for small arrays
    template <class T, size_t N, class A, class H, class S>
    constexpr bool
    operator!=(
        const small_vector<T, N, A, H, S> &x,
        const small_vector<T, N, A, H, S> &y) {
        return !(x == y);
    }

    /// \brief operator< for small arrays
    template <class T, size_t N, class A, class H, class S>
    constexpr bool
    operator<(
        const small_vector<T, N, A, H, S> &x,
        const small_vector<T, N, A, H, S> &y) {
        return std::
            lexicographical_compare(x.begin(), x.end(), y.begin(), y.end());
    }

    /// \brief operator> for small arrays
    template <class T, size_t N, class A, class H, class S>
    constexpr bool
    operator>(
        const small_vector<T, N, A, H, S> &x,
        const small_vector<T, N, A, H, S> &y) {
        return y < x;
    }

    /// \brief operator<= for small arrays
    template <class T, size_t N, class A, class H, class S>
    constexpr bool
    operator<=(
        const small_vector<T, N, A, H, S> &x,
        const small_vector<T, N, A, H, S> &y) {
        return !(y < x);
    }

    /// \brief operator>= for small arrays
    template <class T, size_t N, class A, class H, class S>
    constexpr bool
    operator>=(
        const small_vector<T, N, A, H, S> &x,
        const small_vector<T, N, A, H, S> &y) {
        return !(x < y);
    }

    /// \brief swap the contents of two small arrays
    template <class T, size_t N, class A, class H, class S>
    void
    swap(
        small_vector<T, N, A, H, S> &x,
        small_vector<T, N, A, H, S> &y) noexcept(noexcept(x.swap(y))) {
        x.swap(y);
    }

    /// \brief Create a small_vector from a raw array
    /// This is similar to std::to_array
    template <
        class T,
        size_t N_INPUT,
        size_t N_OUTPUT = std::max(
            std::max(std::size_t(5), (sizeof(T *) + sizeof(size_t)) / sizeof(T)),
            N_INPUT)>
    constexpr small_vector<std::remove_cv_t<T>, N_OUTPUT>
    to_small_vector(T (&a)[N_INPUT]) {
        return small_vector<std::remove_cv_t<T>, N_OUTPUT>(a, a + N_INPUT);
    }

    /// \brief Create a small_vector from a raw array
    /// This is similar to std::to_array
    template <
        class T,
        size_t N_INPUT,
        size_t N_OUTPUT = std::max(
            std::max(std::size_t(5), (sizeof(T *) + sizeof(size_t)) / sizeof(T)),
            N_INPUT)>
    constexpr small_vector<std::remove_cv_t<T>, N_OUTPUT>
    to_small_vector(T(&&a)[N_INPUT]) {
        return small_vector<std::remove_cv_t<T>, N_OUTPUT>(a, a + N_INPUT);
    }

    template <
        class T,
        size_t N
        = std::max((sizeof(std::vector<T>) * 2) / sizeof(T), std::size_t(5)),
        class Allocator = std::allocator<T>,
        class SizeType = size_t>
    using max_size_small_vector
        = small_vector<T, N, Allocator, std::false_type, SizeType>;

    template <
        class T,
        size_t N
        = std::max((sizeof(std::vector<T>) * 2) / sizeof(T), std::size_t(5)),
        class Allocator = std::allocator<T>,
        class SizeType = size_t>
    using inline_small_vector
        = small_vector<T, N, Allocator, std::true_type, SizeType>;

} // namespace futures::detail

#endif // FUTURES_SMALL_VECTOR_H

#include <memory>
#include <shared_mutex>

namespace futures::detail {
    /** \addtogroup futures Futures
     *  @{
     */

    /// \brief The continuation state as a small thread safe container that
    /// holds continuation functions for a future
    ///
    /// The whole logic here is very similar to that of stop_tokens. There is a
    /// source, a state, and a token.
    ///
    /// This is very limited as a container because there are not many
    /// operations we need to do with the continuation state. We need to be able
    /// to attach continuations (then), and run all continuations with a single
    /// shared lock.
    ///
    /// Like the stop_state, a continuation state might be shared between shared
    /// futures. Once one of the futures has run the continuations, the state is
    /// considered done.
    ///
    /// The continuation state needs to be atomic because it's also a shared
    /// state. Especially when the future is shared, many threads might be
    /// trying to attach new continuations to this future type, and the main
    /// future callback needs to wait for it.
    class continuations_state
    {
    public:
        /// \name Public Types
        /// @{

        /// \brief Type of a continuation callback
        /// This is a callback function that posts the next task to an executor.
        /// We cannot ensure the tasks go to the same executor.
        /// This needs to be type erased because there are many types of
        /// callables that might become a continuation here.
        using continuation_type = std::function<void()>;

        /// \brief Continuation ptr
        /// The callbacks are stored pointers because their addresses cannot
        /// lose stability when the future is moved or shared
        using continuation_ptr = std::unique_ptr<continuation_type>;

        /// \brief The continuation vector
        /// We use a small vector because of the common case when there few
        /// continuations per task
        using continuation_vector = detail::small_vector<continuation_ptr>;

        /// @}

        /// \name Constructors
        /// @{

        /// \brief Default constructor
        continuations_state() = default;

        /// \brief Copy constructor
        continuations_state(const continuations_state &) = delete;

        /// \brief Destructor - Run continuations if they have not run yet
        ~continuations_state() {
            request_run();
        }

        /// \brief Copy assignment
        continuations_state &
        operator=(const continuations_state &)
            = delete;

        /// @}

        /// \name Non-modifying
        /// @{

        /// \brief Get number of continuations
        [[nodiscard]] size_t
        size() const {
            std::shared_lock lock(continuations_mutex_);
            return continuations_.size();
        }

        /// \brief Get the i-th continuation
        /// The return reference is safe (in context) because the continuation
        /// vector has stability
        continuation_type &
        operator[](size_t index) const {
            std::shared_lock lock(continuations_mutex_);
            return continuations_.at(index).operator*();
        }
        /// @}

        /// \name Modifying
        /// @{

        /// \brief Emplace a new continuation
        /// Use executor ex if more continuations are not possible
        template <class Executor>
        bool
        emplace_back(const Executor &ex, continuation_type &&fn) {
            std::unique_lock lock(continuations_mutex_);
            if (is_run_possible()) {
                continuations_.emplace_back(
                    std::make_unique<continuation_type>(std::move(fn)));
                return true;
            } else {
                // When the shared state currently associated with *this is
                // ready, the continuation is called on an unspecified thread of
                // execution
                asio::post(ex, asio::use_future(std::move(fn)));
                return false;
            }
        }

        /// \brief Check if some source asked already asked for the
        /// continuations to run
        bool
        is_run_requested() const {
            std::shared_lock lock(run_requested_mutex_);
            return run_requested_;
        }

        /// \brief Check if some source asked already asked for the
        /// continuations to run
        bool
        is_run_possible() const {
            return !is_run_requested();
        }

        /// \brief Run all continuations
        bool
        request_run() {
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
            for (auto &continuation: continuations_) {
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
        bool run_requested_{ false };
        mutable std::shared_mutex continuations_mutex_;
        mutable std::shared_mutex run_requested_mutex_;
    };

    /// Unit type intended for use as a placeholder in continuations_source
    /// non-default constructor
    struct nocontinuationsstate_t
    {
        explicit nocontinuationsstate_t() = default;
    };

    /// This is a constant object instance of stdnocontinuationsstate_t for use
    /// in constructing an empty continuations_source, as a placeholder value in
    /// the non-default constructor
    inline constexpr nocontinuationsstate_t nocontinuationsstate{};

    /// \brief Token the future object uses to emplace continuations
    class continuations_token
    {
    public:
        /// \brief Constructs an empty continuations_token with no associated
        /// continuations-state
        continuations_token() noexcept : state_(nullptr) {}

        /// \brief Constructs a continuations_token whose associated
        /// continuations-state is the same as that of other
        continuations_token(
            const continuations_token &other) noexcept = default;

        /// \brief Constructs a continuations_token whose associated
        /// continuations-state is the same as that of other; other is left empty
        continuations_token(continuations_token &&other) noexcept = default;

        /// \brief Copy-assigns the associated continuations-state of other to
        /// that of *this
        continuations_token &
        operator=(const continuations_token &other) noexcept = default;

        /// \brief Move-assigns the associated continuations-state of other to
        /// that of *this
        continuations_token &
        operator=(continuations_token &&other) noexcept = default;

        /// \brief Exchanges the associated continuations-state of *this and
        /// other.
        void
        swap(continuations_token &other) noexcept {
            std::swap(state_, other.state_);
        }

        /// \brief Checks if the continuations_token object has associated
        /// continuations-state and that state has received a run request
        [[nodiscard]] bool
        run_requested() const noexcept {
            return (state_ != nullptr) && state_->is_run_requested();
        }

        /// \brief Checks if the continuations_token object has associated
        /// continuations-state, and that state either has already had a run
        /// requested or it has associated continuations_source object(s)
        [[nodiscard]] bool
        run_possible() const noexcept {
            return (state_ != nullptr) && (!state_->is_run_requested());
        }

        /// \brief compares two std::run_token objects
        [[nodiscard]] friend bool
        operator==(
            const continuations_token &lhs,
            const continuations_token &rhs) noexcept {
            return lhs.state_ == rhs.state_;
        }

        [[nodiscard]] friend bool
        operator!=(
            const continuations_token &lhs,
            const continuations_token &rhs) noexcept {
            return lhs.state_ != rhs.state_;
        }

    private:
        friend class continuations_source;

        /// \brief Create token from state
        explicit continuations_token(
            std::shared_ptr<continuations_state> state) noexcept
            : state_(std::move(state)) {}

        /// \brief The state
        std::shared_ptr<continuations_state> state_;
    };

    /// \brief The continuations_source class provides the means to issue a
    /// request to run the future continuations
    class continuations_source
    {
    public:
        /// \brief Constructs a continuations_source with new continuations-state
        continuations_source()
            : state_(std::make_shared<continuations_state>()){};

        /// \brief Constructs an empty continuations_source with no associated
        /// continuations-state.
        explicit continuations_source(nocontinuationsstate_t) noexcept
            : state_{ nullptr } {}

        /// \brief Copy constructor.
        /// Constructs a continuations_source whose associated
        /// continuations-state is the same as that of other.
        continuations_source(
            const continuations_source &other) noexcept = default;

        /// \brief Move constructor.
        /// Constructs a continuations_source whose associated
        /// continuations-state is the same as that of other; other is left
        /// empty.
        continuations_source(continuations_source &&other) noexcept = default;

        /// \brief Copy-assigns the continuations-state of other to that of *this
        continuations_source &
        operator=(const continuations_source &other) noexcept = default;

        /// \brief Move-assigns the continuations-state of other to that of *this
        continuations_source &
        operator=(continuations_source &&other) noexcept = default;

        /// \brief Run all continuations
        /// The return reference is safe because the continuation vector has
        /// stability
        bool
        request_run() const {
            if (state_ != nullptr) {
                return state_->request_run();
            }
            return false;
        }

        /// \brief Run all continuations
        /// The return reference is safe because the continuation vector has
        /// stability
        template <class Executor>
        bool
        emplace_continuation(
            const Executor &ex,
            continuations_state::continuation_type &&fn) {
            if (state_ != nullptr) {
                return state_->emplace_back(ex, std::move(fn));
            }
            return false;
        }

        /// \brief Exchanges the continuations-state of *this and other.
        void
        swap(continuations_source &other) noexcept {
            std::swap(state_, other.state_);
        }

        /// \brief Get a token to this object
        /// Returns a continuations_token object associated with the
        /// continuations_source's continuations-state, if the
        /// continuations_source has continuations-state; otherwise returns a
        /// default-constructed (empty) continuations_token.
        [[nodiscard]] continuations_token
        get_token() const noexcept {
            return continuations_token(state_);
        }

        /// \brief Checks if the continuations_source object has a
        /// continuations-state and that state has received a run request.
        [[nodiscard]] bool
        run_requested() const noexcept {
            return state_ != nullptr && state_->is_run_requested();
        }

        /// \brief Checks if the continuations_source object has a
        /// continuations-state.
        [[nodiscard]] bool
        run_possible() const noexcept {
            return state_ != nullptr;
        }

        /// \brief Compares two continuations_source values
        [[nodiscard]] friend bool
        operator==(
            const continuations_source &a,
            const continuations_source &b) noexcept {
            return a.state_ == b.state_;
        }

        /// \brief Compares two continuations_source values
        [[nodiscard]] friend bool
        operator!=(
            const continuations_source &a,
            const continuations_source &b) noexcept {
            return a.state_ != b.state_;
        }

    private:
        std::shared_ptr<continuations_state> state_;
    };

    /** @} */
} // namespace futures::detail

#endif // FUTURES_CONTINUATIONS_SOURCE_H

// #include <futures/futures/detail/shared_state.h>
#ifndef FUTURES_SHARED_STATE_H
#define FUTURES_SHARED_STATE_H

// #include <futures/futures/future_error.h>
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
    class futures_error : public std::system_error
    {
    public:
        /// \brief Construct underlying system error with a specified error code
        /// \param ec Error code
        explicit futures_error(std::error_code ec) : std::system_error{ ec } {}

        /// \brief Construct underlying system error with a specified error code
        /// and literal string message \param ec Error code \param what_arg
        /// Error string
        futures_error(std::error_code ec, const char *what_arg)
            : std::system_error{ ec, what_arg } {}

        /// \brief Construct underlying system error with a specified error code
        /// and std::string message \param ec Error code \param what_arg Error
        /// string
        futures_error(std::error_code ec, std::string const &what_arg)
            : std::system_error{ ec, what_arg } {}

        /// \brief Destructor
        ~futures_error() override = default;
    };

    /// \brief Error codes for futures
    enum class future_errc
    {
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
    inline std::error_category const &
    future_category() noexcept;

    /// \brief Class representing the common error category properties for
    /// future errors
    class future_error_category : public std::error_category
    {
    public:
        /// \brief Name for future_error_category errors
        [[nodiscard]] const char *
        name() const noexcept override {
            return "future";
        }

        /// \brief Generate error condition
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
            default:
                return std::error_condition{ ev, *this };
            }
        }

        /// \brief Check error condition
        [[nodiscard]] bool
        equivalent(std::error_code const &code, int condition)
            const noexcept override {
            return *this == code.category()
                   && static_cast<int>(
                          default_error_condition(code.value()).value())
                          == condition;
        }

        /// \brief Generate message
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
            }
            return std::string{ "unspecified future_errc value\n" };
        }
    };

    /// \brief Function to return a common reference to a global future error
    /// category
    inline std::error_category const &
    future_category() noexcept {
        static future_error_category cat;
        return cat;
    }

    /// \brief Class for errors with specific future types or their
    /// dependencies, such as promises
    class future_error : public futures_error
    {
    public:
        /// \brief Construct underlying futures error with a specified error
        /// code \param ec Error code
        explicit future_error(std::error_code ec) : futures_error{ ec } {}
    };

    inline std::error_code
    make_error_code(future_errc code) {
        return std::error_code{
            static_cast<int>(code),
            futures::future_category()
        };
    }

    /// \brief Class for errors when a promise is not delivered properly
    class broken_promise : public future_error
    {
    public:
        /// \brief Construct underlying future error with a specified error code
        /// \param ec Error code
        broken_promise()
            : future_error{ make_error_code(future_errc::broken_promise) } {}
    };

    /// \brief Class for errors when a promise is not delivered properly
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

    /** @} */
    /** @} */
} // namespace futures

#endif // FUTURES_FUTURE_ERROR_H

// #include <futures/futures/detail/relocker.h>
#ifndef FUTURES_RELOCKER_H
#define FUTURES_RELOCKER_H

#include <mutex>

namespace futures::detail {
    /** \addtogroup futures Futures
     *  @{
     */
    /// \brief An object that temporarily unlocks a lock
    struct relocker
    {
        /// \brief The underlying lock
        std::unique_lock<std::mutex> &lock_;

        /// \brief Construct a relocker
        ///
        /// A relocker keeps a temporary reference to the lock and
        /// immediately unlocks it
        ///
        /// \param lk Reference to underlying lock
        explicit relocker(std::unique_lock<std::mutex> &lk) : lock_(lk) {
            lock_.unlock();
        }

        /// \brief Copy constructor is deleted
        relocker(const relocker &) = delete;
        relocker(relocker &&other) noexcept = delete;

        /// \brief Copy assignment is deleted
        relocker &
        operator=(const relocker &)
            = delete;
        relocker &
        operator=(relocker &&other) noexcept = delete;

        /// \brief Destroy the relocker
        ///
        /// The relocker locks the underlying lock when it's done
        ~relocker() {
            if (!lock_.owns_lock()) {
                lock_.lock();
            }
        }

        /// \brief Lock the underlying lock
        void
        lock() {
            if (!lock_.owns_lock()) {
                lock_.lock();
            }
        }
    };
    /** @} */
} // namespace futures::detail

#endif // FUTURES_RELOCKER_H

// #include <futures/futures/detail/small_vector.h>

// #include <atomic>

// #include <future>

#include <condition_variable>

namespace futures {
    /** \addtogroup futures Futures
     *  @{
     */

    namespace detail {
        /// \brief Members functions and objects common to all shared state
        /// object types (bool ready and exception ptr)
        ///
        /// Shared states for asynchronous operations contain an element of a
        /// given type and an exception.
        ///
        /// All objects such as futures and promises have shared states and
        /// inherit from this class to synchronize their access to their common
        /// shared state.
        class shared_state_base
        {
        public:
            /// \brief A list of waiters: condition variables to notify any
            /// object waiting for this shared state to be ready
            using waiter_list = detail::small_vector<
                std::condition_variable_any *>;

            /// \brief A handle to notify an external context about this state
            /// being ready
            using notify_when_ready_handle = waiter_list::iterator;

            /// \brief A default constructed shared state data
            shared_state_base() = default;

            /// \brief Cannot copy construct the shared state data
            ///
            /// We cannot copy construct the shared state data because its
            /// parent class holds synchronization primitives
            shared_state_base(const shared_state_base &) = delete;

            /// \brief Virtual shared state data destructor
            ///
            /// Virtual to make it inheritable
            virtual ~shared_state_base() = default;

            /// \brief Cannot copy assign the shared state data
            ///
            /// We cannot copy assign the shared state data because this parent
            /// class holds synchronization primitives
            shared_state_base &
            operator=(const shared_state_base &)
                = delete;

            /// \brief Indicate to the shared state the state is ready in the
            /// derived class
            ///
            /// This operation marks the ready_ flags and warns any future
            /// waiting on it. This overload is meant to be used by derived
            /// classes that might need to use another mutex for this operation
            void
            set_ready() noexcept {
                status prev = status_.exchange(
                    status::ready,
                    std::memory_order_release);
                if (prev == status::waiting) {
                    std::atomic_thread_fence(std::memory_order_acquire);
                    auto lk = create_wait_lock();
                    waiter_.notify_all();
                    for (auto &&external_waiter: external_waiters_) {
                        external_waiter->notify_all();
                    }
                }
            }

            /// \brief Check if state is ready
            ///
            /// This overload uses the default global mutex for synchronization
            bool
            is_ready() const {
                return status_.load(std::memory_order_acquire) == status::ready;
            }

            /// \brief Check if state is ready without locking
            ///
            /// Although the parent shared state class doesn't store the state,
            /// we know it's ready with state because it's the only way it's
            /// ready unless it has an exception.
            bool
            succeeded() const {
                return is_ready() && !except_;
            }

            /// \brief Set shared state to an exception
            ///
            /// This sets the exception value and marks the shared state as
            /// ready. If we try to set an exception on a shared state that's
            /// ready, we throw an exception. This overload is meant to be used
            /// by derived classes that might need to use another mutex for this
            /// operation.
            ///
            void
            set_exception(std::exception_ptr except) {
                if (status_.load(std::memory_order_acquire) == status::ready) {
                    throw promise_already_satisfied();
                }
                // ready_ already protects except_ because only one thread
                // should call this function
                except_ = std::move(except);
                set_ready();
            }

            /// \brief Get the shared state when it's an exception
            ///
            /// This overload uses the default global mutex for synchronization
            std::exception_ptr
            get_exception_ptr() const {
                if (status_.load(std::memory_order_acquire) != status::ready) {
                    throw promise_uninitialized();
                }
                return except_;
            }

            /// \brief Rethrow the exception we have stored
            void
            throw_internal_exception() const {
                std::rethrow_exception(get_exception_ptr());
            }

            /// \brief Indicate to the shared state its owner has been destroyed
            ///
            /// If owner has been destroyed before the shared state is ready,
            /// this means a promise has been broken and the shared state should
            /// store an exception. This overload is meant to be used by derived
            /// classes that might need to use another mutex for this operation
            void
            signal_promise_destroyed() {
                if (status_.load(std::memory_order_acquire) != status::ready) {
                    set_exception(std::make_exception_ptr(broken_promise()));
                }
            }

            /// \brief Check if state is ready without locking
            bool
            failed() const {
                return is_ready() && except_ != nullptr;
            }

            /// \brief Wait for shared state to become ready
            ///
            /// This overload uses the default global mutex for synchronization
            void
            wait() const {
                status prev = status::initial;
                status_.compare_exchange_strong(prev, status::waiting);
                if (prev != status::ready) {
                    auto lk = create_wait_lock();
                    waiter_.wait(lk, [this]() {
                        return (
                            status_.load(std::memory_order_acquire)
                            == status::ready);
                    });
                }
            }

            /// \brief Wait for shared state to become ready
            ///
            /// This function uses the condition variable waiters to wait for
            /// this shared state to be marked as ready This overload is meant
            /// to be used by derived classes that might need to use another
            /// mutex for this operation
            ///
            void
            wait(std::unique_lock<std::mutex> &lk) const {
                status prev = status::initial;
                status_.compare_exchange_strong(prev, status::waiting);
                if (prev != status::ready) {
                    assert(lk.owns_lock());
                    waiter_.wait(lk, [this]() {
                        return (
                            status_.load(std::memory_order_acquire)
                            == status::ready);
                    });
                }
            }

            /// \brief Wait for a specified duration for the shared state to
            /// become ready
            ///
            /// This overload uses the default global mutex for synchronization
            template <typename Rep, typename Period>
            std::future_status
            wait_for(std::chrono::duration<Rep, Period> const &timeout_duration)
                const {
                status prev = status::initial;
                status_.compare_exchange_strong(prev, status::waiting);
                if (prev != status::ready) {
                    auto lk = create_wait_lock();
                    if (waiter_.wait_for(lk, timeout_duration, [this]() {
                            return status_.load(std::memory_order_acquire)
                                   == status::ready;
                        }))
                    {
                        return std::future_status::ready;
                    } else {
                        return std::future_status::timeout;
                    }
                }
                return std::future_status::ready;
            }

            /// \brief Wait for a specified duration for the shared state to
            /// become ready
            ///
            /// This function uses the condition variable waiters to wait for
            /// this shared state to be marked as ready for a specified
            /// duration. This overload is meant to be used by derived classes
            /// that might need to use another mutex for this operation
            template <typename Rep, typename Period>
            std::future_status
            wait_for(
                std::unique_lock<std::mutex> &lk,
                std::chrono::duration<Rep, Period> const &timeout_duration)
                const {
                status prev = status::initial;
                status_.compare_exchange_strong(prev, status::waiting);
                if (prev != status::ready) {
                    assert(lk.owns_lock());
                    if (waiter_.wait_for(lk, timeout_duration, [this]() {
                            return status_.load(std::memory_order_acquire)
                                   == status::ready;
                        }))
                    {
                        return std::future_status::ready;
                    } else {
                        return std::future_status::timeout;
                    }
                }
                return std::future_status::ready;
            }

            /// \brief Wait until a specified time point for the shared state to
            /// become ready
            ///
            /// This overload uses the default global mutex for synchronization
            template <typename Clock, typename Duration>
            std::future_status
            wait_until(std::chrono::time_point<Clock, Duration> const
                           &timeout_time) const {
                status prev = status::initial;
                status_.compare_exchange_strong(prev, status::waiting);
                if (prev != status::ready) {
                    auto lk = create_wait_lock();
                    if (waiter_.wait_until(lk, timeout_time, [this]() {
                            return status_.load(std::memory_order_acquire)
                                   == status::ready;
                        }))
                    {
                        return std::future_status::ready;
                    } else {
                        return std::future_status::timeout;
                    }
                }
                return std::future_status::ready;
            }

            /// \brief Wait until a specified time point for the shared state to
            /// become ready This function uses the condition variable waiters
            /// to wait for this shared state to be marked as ready until a
            /// specified time point. This overload is meant to be used by
            /// derived classes that might need to use another mutex for this
            /// operation
            template <typename Clock, typename Duration>
            std::future_status
            wait_until(
                std::unique_lock<std::mutex> &lk,
                std::chrono::time_point<Clock, Duration> const &timeout_time)
                const {
                status prev = status::initial;
                status_.compare_exchange_strong(prev, status::waiting);
                if (prev != status::ready) {
                    assert(lk.owns_lock());
                    if (waiter_.wait_until(lk, timeout_time, [this]() {
                            return status_.load(std::memory_order_acquire)
                                   == status::ready;
                        }))
                    {
                        return std::future_status::ready;
                    } else {
                        return std::future_status::timeout;
                    }
                }
                return std::future_status::ready;
            }

            /// \brief Call the internal callback function, if any
            void
            do_callback(std::unique_lock<std::mutex> &lk) const {
                if (callback_
                    && !(
                        status_.load(std::memory_order_acquire)
                        == status::ready)) {
                    std::function<void()> local_callback = callback_;
                    relocker relock(lk);
                    local_callback();
                }
            }

            /// \brief Include a condition variable in the list of waiters we
            /// need to notify when the state is ready
            notify_when_ready_handle
            notify_when_ready(std::condition_variable_any &cv) {
                status expected = status::initial;
                status_.compare_exchange_strong(expected, status::waiting);
                auto lk = create_wait_lock();
                do_callback(lk);
                return external_waiters_.insert(external_waiters_.end(), &cv);
            }

            /// \brief Include a condition variable in the list of waiters we
            /// need to notify when the state is ready
            notify_when_ready_handle
            notify_when_ready(
                [[maybe_unused]] std::unique_lock<std::mutex> &lk,
                std::condition_variable_any &cv) {
                status expected = status::initial;
                status_.compare_exchange_strong(expected, status::waiting);
                assert(lk.owns_lock());
                do_callback(lk);
                return external_waiters_.insert(external_waiters_.end(), &cv);
            }

            /// \brief Remove condition variable from list of condition
            /// variables we need to warn about this state
            void
            unnotify_when_ready(notify_when_ready_handle it) {
                status expected = status::initial;
                status_.compare_exchange_strong(expected, status::waiting);
                auto lk = create_wait_lock();
                external_waiters_.erase(it);
            }

            /// \brief Remove condition variable from list of condition
            /// variables we need to warn about this state
            void
            unnotify_when_ready(
                [[maybe_unused]] std::unique_lock<std::mutex> &lk,
                notify_when_ready_handle it) {
                status expected = status::initial;
                status_.compare_exchange_strong(expected, status::waiting);
                assert(lk.owns_lock());
                external_waiters_.erase(it);
            }

            /// \brief Get a reference to the mutex in the shared state
            std::mutex &
            mutex() {
                return waiters_mutex_;
            }

            /// \brief Generate unique lock for the shared state
            ///
            /// This lock can be used for any operations on the state that might
            /// need to be protected/
            std::unique_lock<std::mutex>
            create_wait_lock() const {
                return std::unique_lock{ waiters_mutex_ };
            }

        private:
            /// \brief The current status of this shared state
            enum status : uint8_t
            {
                initial,
                waiting,
                ready,
            };

            /// \brief Indicates if the shared state is already set
            ///
            /// There are three states possible: nothing, waiting, ready
            mutable std::atomic<status> status_{ status::initial };

            /// \brief Pointer to an exception, when the shared_state fails
            ///
            /// std::exception_ptr does not need to be atomic because
            /// the status variable is guarding it.
            ///
            std::exception_ptr except_{ nullptr };

            /// \brief Condition variable to notify any object waiting for this
            /// shared state to be ready
            ///
            /// This is the object we use to be able to block until the shared
            /// state is ready. Although the shared state is lock-free, users
            /// might still need to wait for results to be ready. One future
            /// thread calls `waiter_.wait(...)` to block while the promise
            /// thread calls `waiter_.notify_all(...)`. In C++20, atomic
            /// variables have `wait()` functions we could use to replace this
            /// with `ready_.wait(...)`
            ///
            mutable std::condition_variable waiter_{};

            /// \brief List of external condition variables also waiting for
            /// this shared state to be ready
            ///
            /// While the internal waiter is intended for
            ///
            waiter_list external_waiters_;

            /// \brief Callback function we should call when waiting for the
            /// shared state
            std::function<void()> callback_;

            /// \brief Mutex for threads that want to wait on the result
            ///
            /// While the shared state is lock-free, it also includes a mutex
            /// that can be used for communication between futures, such as
            /// waiter futures.
            ///
            /// This is used when one thread intents to wait for the result
            /// of a future. The mutex is not used for lazy futures or if
            /// the result is ready before the waiting operation.
            ///
            /// These functions should be used directly by users very often.
            ///
            mutable std::mutex waiters_mutex_{};
        };

        /// \brief Determine the type we should use to store a shared state
        /// internally
        ///
        /// We usually need uninitialized storage for a given type, since the
        /// shared state needs to be in control of constructors and destructors.
        ///
        /// For trivial types, we can directly store the value.
        ///
        /// When the shared state is a reference, we store pointers internally.
        ///
        template <typename R, class Enable = void>
        struct shared_state_storage
        {
            using type = std::aligned_storage_t<sizeof(R), alignof(R)>;
        };

        template <typename R>
        struct shared_state_storage<R, std::enable_if_t<std::is_trivial_v<R>>>
        {
            using type = R;
        };

        template <typename R>
        using shared_state_storage_t = typename shared_state_storage<R>::type;
    } // namespace detail

    /// \brief Shared state specialization for regular variables
    ///
    /// This class stores the data for a shared state holding an element of type
    /// T
    ///
    /// The data is stored with uninitialized storage because this will only
    /// need to be initialized when the state becomes ready. This ensures the
    /// shared state works for all types and avoids wasting operations on a
    /// constructor we might not use.
    ///
    /// However, initialized storage might be more useful and easier to debug
    /// for trivial types.
    ///
    template <typename R>
    class shared_state : public detail::shared_state_base
    {
    public:
        /// \brief Default construct a shared state
        ///
        /// This will construct the state with uninitialized storage for R
        ///
        shared_state() = default;

        /// \brief Deleted copy constructor
        ///
        /// The copy constructor does not make sense for shared states as they
        /// should be shared. Besides, copying state that might be uninitialized
        /// is a bad idea.
        ///
        shared_state(shared_state const &) = delete;

        /// \brief Deleted copy assignment operator
        ///
        /// These functions do not make sense for shared states as they are
        /// shared
        shared_state &
        operator=(shared_state const &)
            = delete;

        /// \brief Destructor
        ///
        /// Destruct the shared object R manually if the state is ready with a
        /// value
        ///
        ~shared_state() override {
            if constexpr (!std::is_trivial_v<R>) {
                if (succeeded()) {
                    reinterpret_cast<R *>(std::addressof(storage_))->~R();
                }
            }
        }

        /// \brief Set the value of the shared state to a copy of value
        ///
        /// This function locks the shared state and makes a copy of the value
        /// into the storage.
        void
        set_value(const R &value) {
            auto lk = create_wait_lock();
            set_value(value, lk);
        }

        /// \brief Set the value of the shared state by moving a value
        ///
        /// This function locks the shared state and moves the value to the state
        void
        set_value(R &&value) {
            auto lk = create_wait_lock();
            set_value(std::move(value), lk);
        }

        /// \brief Get the value of the shared state
        ///
        /// \return Reference to the state as a reference to R
        R &
        get() {
            auto lk = create_wait_lock();
            return get(lk);
        }

    private:
        /// \brief Set value of the shared state in the storage by copying the
        /// value
        ///
        /// \param value The value for the shared state
        /// \param lk Custom mutex lock
        void
        set_value(R const &value, std::unique_lock<std::mutex> &lk) {
            assert(lk.owns_lock());
            if (is_ready()) {
                throw promise_already_satisfied{};
            }
            ::new (static_cast<void *>(std::addressof(storage_))) R(value);
            detail::relocker relk(lk);
            set_ready();
        }

        /// \brief Set value of the shared state in the storage by moving the
        /// value
        ///
        /// \param value The rvalue for the shared state
        /// \param lk Custom mutex lock
        void
        set_value(R &&value, std::unique_lock<std::mutex> &lk) {
            assert(lk.owns_lock());
            if (is_ready()) {
                throw promise_already_satisfied{};
            }
            ::new (static_cast<void *>(std::addressof(storage_)))
                R(std::move(value));
            detail::relocker relk(lk);
            set_ready();
        }

        /// \brief Get value of the shared state
        /// This function wait for the shared state to become ready and returns
        /// its value \param lk Custom mutex lock \return Shared state as a
        /// reference to R
        R &
        get(std::unique_lock<std::mutex> &lk) {
            assert(lk.owns_lock());
            wait(lk);
            if (failed()) {
                throw_internal_exception();
            }
            return *reinterpret_cast<R *>(std::addressof(storage_));
        }

        /// \brief Aligned opaque storage for an element of type R
        ///
        /// This needs to be aligned_storage_t because the value might not be
        /// initialized yet
        detail::shared_state_storage_t<R> storage_{};
    };

    /// \brief Shared state specialization for references
    ///
    /// This class stores the data for a shared state holding a reference to an
    /// element of type T These shared states need to store a pointer to R
    /// internally
    template <typename R>
    class shared_state<R &> : public detail::shared_state_base
    {
    public:
        /// \brief Default construct a shared state
        ///
        /// This will construct the state with uninitialized storage for R
        shared_state() = default;

        /// \brief Deleted copy constructor
        ///
        /// These functions do not make sense for shared states as they are
        /// shared
        shared_state(shared_state const &) = delete;

        /// \brief Destructor
        ///
        /// Destruct the shared object R if the state is ready with a value
        ~shared_state() override = default;

        /// \brief Deleted copy assignment
        ///
        /// These functions do not make sense for shared states as they are
        /// shared
        shared_state &
        operator=(shared_state const &)
            = delete;

        /// \brief Set the value of the shared state by storing the address of
        /// value R in its internal state
        ///
        /// Shared states to references internally store a pointer to the value
        /// instead fo copying it. This function locks the shared state and
        /// moves the value to the state
        void
        set_value(std::enable_if_t<!std::is_same_v<R, void>, R> &value) {
            auto lk = create_wait_lock();
            set_value(value, lk);
        }

        /// \brief Get the value of the shared state as a reference to the
        /// internal pointer \return Reference to the state as a reference to R
        R &
        get() {
            auto lk = create_wait_lock();
            return get(lk);
        }

    private:
        /// \brief Set reference of the shared state to the address of a value R
        /// \param value The value for the shared state
        /// \param lk Custom mutex lock
        void
        set_value(R &value, std::unique_lock<std::mutex> &lk) {
            assert(lk.owns_lock());
            if (is_ready()) {
                throw promise_already_satisfied();
            }
            value_ = std::addressof(value);
            detail::relocker relk(lk);
            set_ready();
        }

        /// \brief Get reference to the shared value
        /// \return Reference to the state as a reference to R
        R &
        get(std::unique_lock<std::mutex> &lk) {
            assert(lk.owns_lock());
            wait(lk);
            if (failed()) {
                throw_internal_exception();
            }
            return *value_;
        }

        /// \brief Pointer to an element of type R representing the reference
        /// This needs to be aligned_storage_t because the value might not be
        /// initialized yet
        R *value_{ nullptr };
    };

    /// \brief Shared state specialization for a void shared state
    ///
    /// A void shared state needs to synchronize waiting, but it does not need
    /// to store anything.
    template <>
    class shared_state<void> : public detail::shared_state_base
    {
    public:
        /// \brief Default construct a shared state
        ///
        /// This will construct the state with uninitialized storage for R
        shared_state() = default;

        /// \brief Deleted copy constructor
        ///
        /// These functions do not make sense for shared states as they are
        /// shared
        shared_state(shared_state const &) = delete;

        /// \brief Destructor
        ///
        /// Destruct the shared object R if the state is ready with a value
        ~shared_state() override = default;

        /// \brief Deleted copy assignment
        ///
        /// These functions do not make sense for shared states as they are
        /// shared
        shared_state &
        operator=(shared_state const &)
            = delete;

        /// \brief Set the value of the void shared state without any input as
        /// "not-error"
        ///
        /// This function locks the shared state and moves the value to the state
        void
        set_value() {
            auto lk = create_wait_lock();
            set_value(lk);
        }

        /// \brief Get the value of the shared state by waiting and checking for
        /// exceptions
        ///
        /// \return Reference to the state as a reference to R
        void
        get() {
            auto lk = create_wait_lock();
            get(lk);
        }

    private:
        /// \brief Set value of the shared state with no inputs as "no-error"
        ///
        /// \param lk Custom mutex lock
        void
        set_value(std::unique_lock<std::mutex> &lk) {
            assert(lk.owns_lock());
            if (is_ready()) {
                throw promise_already_satisfied();
            }
            detail::relocker relk(lk);
            set_ready();
        }

        /// \brief Get the value of the shared state by waiting and checking for
        /// exceptions
        void
        get(std::unique_lock<std::mutex> &lk) const {
            assert(lk.owns_lock());
            wait(lk);
            if (failed()) {
                throw_internal_exception();
            }
        }
    };

    /** @} */ // \addtogroup futures Futures
} // namespace futures

#endif // FUTURES_SHARED_STATE_H

// #include <futures/futures/detail/throw_exception.h>

// #include <functional>

// #include <utility>

// #include <shared_mutex>


namespace futures {
    /** \addtogroup futures Futures
     *
     * \brief Basic future types and functions
     *
     * The futures library provides components to create and launch futures,
     * objects representing data that might not be available yet.
     *  @{
     */

    /** \addtogroup future-types Future types
     *
     * \brief Basic future types
     *
     * This module defines the @ref basic_future template class, which can be
     * used to define futures with a number of extensions.
     *
     *  @{
     */

    // Fwd-declaration
    template <class T, class Shared, class LazyContinuable, class Stoppable>
    class basic_future;

    /// \brief A simple future type similar to `std::future`
    template <class T>
    using future
        = basic_future<T, std::false_type, std::false_type, std::false_type>;

    /// \brief A future type with stop tokens
    ///
    /// This class is like a version of jthread for futures
    ///
    /// It's a quite common use case that we need a way to cancel futures and
    /// jfuture provides us with an even better way to do that.
    ///
    /// @ref async is adapted to return a jcfuture whenever:
    /// 1. the callable receives 1 more argument than the caller provides @ref
    /// async
    /// 2. the first callable argument is a stop token
    template <class T>
    using jfuture
        = basic_future<T, std::false_type, std::false_type, std::true_type>;

    /// \brief A future type with lazy continuations
    ///
    /// This is what a @ref futures::async returns when the first function
    /// parameter is not a @ref futures::stop_token
    ///
    template <class T>
    using cfuture
        = basic_future<T, std::false_type, std::true_type, std::false_type>;

    /// \brief A future type with lazy continuations and stop tokens
    ///
    /// This is what a @ref futures::async returns when the first function
    /// parameter is a @ref futures::stop_token
    template <class T>
    using jcfuture
        = basic_future<T, std::false_type, std::true_type, std::true_type>;

    /// \brief A future type with lazy continuations and stop tokens
    /// Same as @ref jcfuture
    template <class T>
    using cjfuture = jcfuture<T>;

    /// \brief A simple std::shared_future
    ///
    /// This is what a futures::future::share() returns
    template <class T>
    using shared_future
        = basic_future<T, std::true_type, std::false_type, std::false_type>;

    /// \brief A shared future type with stop tokens
    ///
    /// This is what a @ref futures::jfuture::share() returns
    template <class T>
    using shared_jfuture
        = basic_future<T, std::true_type, std::false_type, std::true_type>;

    /// \brief A shared future type with lazy continuations
    ///
    /// This is what a @ref futures::cfuture::share() returns
    template <class T>
    using shared_cfuture
        = basic_future<T, std::true_type, std::true_type, std::false_type>;

    /// \brief A shared future type with lazy continuations and stop tokens
    ///
    /// This is what a @ref futures::jcfuture::share() returns
    template <class T>
    using shared_jcfuture
        = basic_future<T, std::true_type, std::true_type, std::true_type>;

    /// \brief A shared future type with lazy continuations and stop tokens
    ///
    /// \note Same as @ref shared_jcfuture
    template <class T>
    using shared_cjfuture = shared_jcfuture<T>;

    namespace detail {
        /// \name Helpers to declare internal_async a friend
        /// These helpers also help us deduce what kind of types we will return
        /// from `async` and `then`
        /// @{

        /// @}

        // Fwd-declare
        template <typename R>
        class promise_base;
        struct async_future_scheduler;
        struct internal_then_functor;

        /// \class Enable a future class that automatically waits on
        /// destruction, and can be cancelled/stopped Besides a regular future,
        /// this class also carries a stop_source representing a should-stop
        /// state. The callable should receive a stop_token which represents a
        /// "view" of this stop state. The user can request the future to stop
        /// with request_stop, without directly interacting with the
        /// stop_source. jfuture, cfuture, and jcfuture share a lot of code.
        template <class Derived, class T>
        class enable_stop_token
        {
        public:
            enable_stop_token() = default;
            enable_stop_token(const enable_stop_token &c) = default;
            /// \brief Move construct/assign helper for the
            /// When the basic future is being moved, the stop source gets
            /// copied instead of moved, because we only want to move the future
            /// state, but we don't want to invalidate the stop token of a
            /// function that is already running. The user could copy the source
            /// before moving the future.
            enable_stop_token(enable_stop_token &&c) noexcept
                : stop_source_(c.stop_source_){};

            enable_stop_token &
            operator=(const enable_stop_token &c)
                = default;
            enable_stop_token &
            operator=(enable_stop_token &&c) noexcept {
                stop_source_ = c.stop_source_;
                return *this;
            };

            bool
            request_stop() noexcept {
                return get_stop_source().request_stop();
            }

            [[nodiscard]] stop_source
            get_stop_source() const noexcept {
                return stop_source_;
            }

            [[nodiscard]] stop_token
            get_stop_token() const noexcept {
                return stop_source_.get_token();
            }

        public:
            /// \brief Stop source for started future
            /// Unlike threads, futures need to be default constructed without
            /// nostopstate because the std::future might be set later in this
            /// object and it needs to be created with a reference to this
            /// already existing stop source.
            stop_source stop_source_{ nostopstate };
        };

        template <class Derived, class T>
        class disable_stop_token
        {
        public:
            disable_stop_token() = default;
            disable_stop_token(const disable_stop_token &c) = default;
            disable_stop_token(disable_stop_token &&c) noexcept = default;
            disable_stop_token &
            operator=(const disable_stop_token &c)
                = default;
            disable_stop_token &
            operator=(disable_stop_token &&c) noexcept = default;
        };

        template <typename Enable, typename Derived, typename T>
        using stop_token_base = std::conditional_t<
            Enable::value,
            enable_stop_token<Derived, T>,
            disable_stop_token<Derived, T>>;

        /// \brief Enable lazy continuations for a future type
        /// When async starts this future type, its internal lambda function
        /// needs to be is programmed to continue and run all attached
        /// continuations in the same executor even after the original callable
        /// is over.
        template <class Derived, class T>
        class enable_lazy_continuations
        {
        public:
            enable_lazy_continuations() = default;
            enable_lazy_continuations(const enable_lazy_continuations &c)
                = default;
            enable_lazy_continuations &
            operator=(const enable_lazy_continuations &c)
                = default;
            enable_lazy_continuations(
                enable_lazy_continuations &&c) noexcept = default;
            enable_lazy_continuations &
            operator=(enable_lazy_continuations &&c) noexcept = default;

            /// Default Constructors: whenever the state is moved or copied, the
            /// default constructor will copy or move the internal shared ptr
            /// pointing to this common state of continuations.

            bool
            then(continuations_state::continuation_type &&fn) {
                return then(
                    make_default_executor(),
                    asio::use_future(std::move(fn)));
            }

            /// \brief Emplace a function to the shared vector of continuations
            /// If the function is ready, use the given executor instead of
            /// executing inline with the previous executor.
            template <class Executor>
            bool
            then(
                const Executor &ex,
                continuations_state::continuation_type &&fn) {
                if (!static_cast<Derived *>(this)->valid()) {
                    throw std::future_error(std::future_errc::no_state);
                }
                if (!static_cast<Derived *>(this)->is_ready()
                    && continuations_source_.run_possible())
                {
                    return continuations_source_
                        .emplace_continuation(ex, std::move(fn));
                } else {
                    // When the shared state currently associated with *this is
                    // ready, the continuation is called on an unspecified
                    // thread of execution
                    asio::post(ex, asio::use_future(std::move(fn)));
                    return false;
                }
            }

        public:
            /// \brief Internal shared continuation state
            /// The continuation state needs to be in a shared pointer because
            /// of it's shared. Shared futures and internal functions accessing
            /// this state need be have access to the same state for
            /// continuations. The pointer also helps with stability issues when
            /// the futures are moved or shared.
            continuations_source continuations_source_;
        };

        template <class Derived, class T>
        class disable_lazy_continuations
        {
        public:
            disable_lazy_continuations() = default;
            disable_lazy_continuations(const disable_lazy_continuations &c)
                = default;
            disable_lazy_continuations &
            operator=(const disable_lazy_continuations &c)
                = default;
            disable_lazy_continuations(
                disable_lazy_continuations &&c) noexcept = default;
            disable_lazy_continuations &
            operator=(disable_lazy_continuations &&c) noexcept = default;
        };

        /// \subsection Convenience aliases to refer to base classes
        template <typename Enable, typename Derived, typename T>
        using lazy_continuations_base = std::conditional_t<
            Enable::value,
            enable_lazy_continuations<Derived, T>,
            disable_lazy_continuations<Derived, T>>;
    } // namespace detail

#ifndef FUTURES_DOXYGEN
    // fwd-declare
    template <typename Signature>
    class packaged_task;
#endif

    /// \brief A basic future type with custom features
    ///
    /// Note that these classes only provide the capabilities of tracking these
    /// features, such as continuations.
    ///
    /// Setting up these capabilities (creating tokens or setting main future to
    /// run continuations) needs to be done when the future is created in a
    /// function such as @ref async by creating the appropriate state for each
    /// feature.
    ///
    /// All this behavior is already encapsulated in the @ref async function.
    ///
    /// \tparam T Type of element
    /// \tparam Shared `std::true_value` if this future is shared
    /// \tparam LazyContinuable `std::true_value` if this future supports
    /// continuations \tparam Stoppable `std::true_value` if this future
    /// contains a stop token
    ///
    template <class T, class Shared, class LazyContinuable, class Stoppable>
    class
#if defined(__clang__) && !defined(__apple_build_version__)
        [[clang::preferred_name(jfuture<T>),
          clang::preferred_name(cfuture<T>),
          clang::preferred_name(jcfuture<T>),
          clang::preferred_name(shared_jfuture<T>),
          clang::preferred_name(shared_cfuture<T>),
          clang::preferred_name(shared_jcfuture<T>)]]
#endif
        basic_future
#ifndef FUTURES_DOXYGEN
        : public detail::lazy_continuations_base<
              LazyContinuable,
              basic_future<T, Shared, LazyContinuable, Stoppable>,
              T>
        , public detail::stop_token_base<
              Stoppable,
              basic_future<T, Shared, LazyContinuable, Stoppable>,
              T>
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
        using lazy_continuations_base = detail::lazy_continuations_base<
            LazyContinuable,
            basic_future<T, Shared, LazyContinuable, Stoppable>,
            T>;
        using stop_token_base = detail::stop_token_base<
            Stoppable,
            basic_future<T, Shared, LazyContinuable, Stoppable>,
            T>;

        friend lazy_continuations_base;
        friend stop_token_base;

        using notify_when_ready_handle = detail::shared_state_base::
            notify_when_ready_handle;
#endif

        /// @}

        /// \name Shared state counterparts
        /// @{

        // Other shared state types can access the shared state constructor
        // directly
        friend class detail::promise_base<T>;

        template <typename Signature>
        friend class packaged_task;

#ifndef FUTURES_DOXYGEN
        // The shared variant is always a friend
        using basic_future_shared_version_t
            = basic_future<T, std::true_type, LazyContinuable, Stoppable>;
        friend basic_future_shared_version_t;

        using basic_future_unique_version_t
            = basic_future<T, std::false_type, LazyContinuable, Stoppable>;
        friend basic_future_unique_version_t;
#endif

        friend struct detail::async_future_scheduler;
        friend struct detail::internal_then_functor;

        /// @}

        /// \name Constructors
        /// @{

        /// \brief Constructs the basic_future
        ///
        /// The default constructor creates an invalid future with no shared
        /// state.
        ///
        /// Null shared state. Properties inherited from base classes.
        basic_future() noexcept
            : lazy_continuations_base(), // No continuations at constructions,
                                         // but callbacks should be set
              stop_token_base(), // Stop token false, but stop token parameter
                                 // should be set
              state_{ nullptr } {}

        /// \brief Construct from a pointer to the shared state
        ///
        /// This constructor is private because we need to ensure the launching
        /// function appropriately sets this std::future handling these traits
        /// This is a function for async.
        ///
        /// \param s Future shared state
        explicit basic_future(
            const std::shared_ptr<shared_state<T>> &s) noexcept
            : lazy_continuations_base(), // No continuations at constructions,
                                         // but callbacks should be set
              stop_token_base(), // Stop token false, but stop token parameter
                                 // should be set
              state_{ std::move(s) } {}

        /// \brief Copy constructor for shared futures only.
        ///
        /// Inherited from base classes.
        ///
        /// \note The copy constructor only participates in overload resolution
        /// if `other` is shared
        ///
        /// \param other Another future used as source to initialize the shared
        /// state
        basic_future(const basic_future &other)
            : lazy_continuations_base(other), // Copy reference to continuations
              stop_token_base(other),         // Copy reference to stop state
              state_{ other.state_ } {
            static_assert(
                is_shared_v,
                "Copy constructor is only available for shared futures");
        }

        /// \brief Move constructor.
        ///
        /// Inherited from base classes.
        basic_future(basic_future && other) noexcept
            : lazy_continuations_base(
                std::move(other)),               // Get control of continuations
              stop_token_base(std::move(other)), // Move stop state
              join_{ other.join_ }, state_{ other.state_ } {
            other.state_.reset();
        }

        /// \brief Destructor
        ///
        /// The shared pointer will take care of decrementing the reference
        /// counter of the shared state, but we still take care of the special
        /// options:
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
                detail::continuations_source &cs = lazy_continuations_base::
                    continuations_source_;
                if (cs.run_possible()) {
                    cs.request_run();
                }
            }
        }

        /// \brief Copy assignment for shared futures only.
        ///
        /// Inherited from base classes.
        basic_future &operator=(const basic_future &other) {
            static_assert(
                is_shared_v,
                "Copy assignment is only available for shared futures");
            if (&other == this) {
                return *this;
            }
            wait_if_last(); // If this is the last shared future waiting for
                            // previous result, we wait
            lazy_continuations_base::operator=(
                other); // Copy reference to continuations
            stop_token_base::operator=(other); // Copy reference to stop state
            join_ = other.join_;
            state_ = other.state_; // Make it point to the same shared state
            other.detach();        // Detach other to ensure it won't block at
                                   // destruction
            return *this;
        }

        /// \brief Move assignment.
        ///
        /// Inherited from base classes.
        basic_future &operator=(basic_future &&other) noexcept {
            if (&other == this) {
                return *this;
            }
            wait_if_last(); // If this is the last shared future waiting for
                            // previous result, we wait
            lazy_continuations_base::operator=(
                std::move(other)); // Get control of continuations
            stop_token_base::operator=(std::move(other)); // Move stop state
            join_ = other.join_;
            state_ = other.state_; // Make it point to the same shared state
            other.state_.reset();
            other.detach(); // Detach other to ensure it won't block at
                            // destruction
            return *this;
        }
        /// @}

#if FUTURES_DOXYGEN
        /// \brief Emplace a function to the shared vector of continuations
        ///
        /// If properly setup (by async), this future holds the result from a
        /// function that runs these continuations after the main promise is
        /// fulfilled. However, if this future is already ready, we can just run
        /// the continuation right away.
        ///
        /// \note This function only participates in overload resolution if the
        /// future supports continuations
        ///
        /// \return True if the contination was emplaced without the using the
        /// default executor
        bool then(continuations_state::continuation_type && fn);

        /// \brief Emplace a function to the shared vector of continuations
        ///
        /// If the function is ready, use the given executor instead of
        /// executing inline with the previous executor.
        ///
        /// \note This function only participates in overload resolution if the
        /// future supports continuations
        ///
        /// \tparam Executor
        /// \param ex
        /// \param fn
        /// \return
        template <class Executor>
        bool
        then(const Executor &ex, continuations_state::continuation_type &&fn);

        /// \brief Request the future to stop whatever task it's running
        ///
        /// \note This function only participates in overload resolution if the
        /// future supports stop tokens
        ///
        /// \return Whether the request was made
        bool request_stop() noexcept;

        /// \brief Get this future's stop source
        ///
        /// \note This function only participates in overload resolution if the
        /// future supports stop tokens
        ///
        /// \return The stop source
        [[nodiscard]] stop_source get_stop_source() const noexcept;

        /// \brief Get this future's stop token
        ///
        /// \note This function only participates in overload resolution if the
        /// future supports stop tokens
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
                std::shared_ptr<shared_state<T>> tmp;
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
        /// If the current type is shared, the new object is equivalent to a
        /// copy.
        ///
        /// \return A shared variant of this future
        basic_future_shared_version_t share() {
            if (!valid()) {
                throw future_uninitialized{};
            }
            basic_future_shared_version_t res{
                is_shared_v ? state_ : std::move(state_)
            };
            res.join_ = std::exchange(join_, is_shared_v && join_);
            if constexpr (is_lazy_continuable_v) {
                res.continuations_source_ = this->continuations_source_;
            }
            if constexpr (is_stoppable_v) {
                res.stop_source_ = this->stop_source_;
            }
            return res;
        }

        /// \brief Get exception pointer without throwing exception
        ///
        /// This extends std::future so that we can always check if the future
        /// threw an exception
        std::exception_ptr get_exception_ptr() {
            if (!valid()) {
                throw future_uninitialized{};
            }
            return state_->get_exception_ptr();
        }

        /// \brief Checks if the future refers to a shared state
        [[nodiscard]] bool valid() const {
            return nullptr != state_.get();
        }

        /// \brief Blocks until the result becomes available.
        void wait() const {
            if (!valid()) {
                throw future_uninitialized{};
            }
            state_->wait();
        }

        /// \brief Waits for the result to become available.
        template <class Rep, class Period>
        [[nodiscard]] std::future_status wait_for(
            const std::chrono::duration<Rep, Period> &timeout_duration) const {
            if (!valid()) {
                throw future_uninitialized{};
            }
            return state_->wait_for(timeout_duration);
        }

        /// \brief Waits for the result to become available.
        template <class Clock, class Duration>
        std::future_status wait_until(
            const std::chrono::time_point<Clock, Duration> &timeout_time)
            const {
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
        void detach() {
            join_ = false;
        }

        /// \brief Notify this condition variable when the future is ready
        notify_when_ready_handle notify_when_ready(
            std::condition_variable_any & cv) {
            if (!state_) {
                detail::throw_exception<future_uninitialized>();
            }
            return state_->notify_when_ready(cv);
        }

        /// \brief Cancel request to notify this condition variable when the
        /// future is ready
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
        void set_stop_source(const stop_source &ss) noexcept {
            stop_token_base::stop_source_ = ss;
        }
        void set_continuations_source(
            const detail::continuations_source &cs) noexcept {
            lazy_continuations_base::continuations_source_ = cs;
        }
        detail::continuations_source get_continuations_source() const noexcept {
            return lazy_continuations_base::continuations_source_;
        }

        /// @}

        /// \name Members
        /// @{
        bool join_{ true };

        /// \brief Pointer to shared state
        std::shared_ptr<shared_state<T>> state_{};
        /// @}
    };

#ifndef FUTURES_DOXYGEN
    /// \name Define basic_future as a kind of future
    /// @{
    template <typename... Args>
    struct is_future<basic_future<Args...>> : std::true_type
    {};
    template <typename... Args>
    struct is_future<basic_future<Args...> &> : std::true_type
    {};
    template <typename... Args>
    struct is_future<basic_future<Args...> &&> : std::true_type
    {};
    template <typename... Args>
    struct is_future<const basic_future<Args...>> : std::true_type
    {};
    template <typename... Args>
    struct is_future<const basic_future<Args...> &> : std::true_type
    {};
    /// @}

    /// \name Define basic_future as a kind of future
    /// @{
    template <typename... Args>
    struct has_ready_notifier<basic_future<Args...>> : std::true_type
    {};
    template <typename... Args>
    struct has_ready_notifier<basic_future<Args...> &> : std::true_type
    {};
    template <typename... Args>
    struct has_ready_notifier<basic_future<Args...> &&> : std::true_type
    {};
    template <typename... Args>
    struct has_ready_notifier<const basic_future<Args...>> : std::true_type
    {};
    template <typename... Args>
    struct has_ready_notifier<const basic_future<Args...> &> : std::true_type
    {};
    /// @}

    /// \name Define basic_futures as supporting lazy continuations
    /// @{
    template <class T, class SH, class L, class ST>
    struct is_shared_future<basic_future<T, SH, L, ST>> : SH
    {};
    template <class T, class SH, class L, class ST>
    struct is_shared_future<basic_future<T, SH, L, ST> &> : SH
    {};
    template <class T, class SH, class L, class ST>
    struct is_shared_future<basic_future<T, SH, L, ST> &&> : SH
    {};
    template <class T, class SH, class L, class ST>
    struct is_shared_future<const basic_future<T, SH, L, ST>> : SH
    {};
    template <class T, class SH, class L, class ST>
    struct is_shared_future<const basic_future<T, SH, L, ST> &> : SH
    {};
    /// @}

    /// \name Define basic_futures as supporting lazy continuations
    /// @{
    template <class T, class SH, class L, class ST>
    struct is_lazy_continuable<basic_future<T, SH, L, ST>> : L
    {};
    template <class T, class SH, class L, class ST>
    struct is_lazy_continuable<basic_future<T, SH, L, ST> &> : L
    {};
    template <class T, class SH, class L, class ST>
    struct is_lazy_continuable<basic_future<T, SH, L, ST> &&> : L
    {};
    template <class T, class SH, class L, class ST>
    struct is_lazy_continuable<const basic_future<T, SH, L, ST>> : L
    {};
    template <class T, class SH, class L, class ST>
    struct is_lazy_continuable<const basic_future<T, SH, L, ST> &> : L
    {};
    /// @}

    /// \name Define basic_futures as having a stop token
    /// @{
    template <class T, class S, class L, class Stoppable>
    struct has_stop_token<basic_future<T, S, L, Stoppable>> : Stoppable
    {};
    template <class T, class S, class L, class Stoppable>
    struct has_stop_token<basic_future<T, S, L, Stoppable> &> : Stoppable
    {};
    template <class T, class S, class L, class Stoppable>
    struct has_stop_token<basic_future<T, S, L, Stoppable> &&> : Stoppable
    {};
    template <class T, class S, class L, class Stoppable>
    struct has_stop_token<const basic_future<T, S, L, Stoppable>> : Stoppable
    {};
    template <class T, class S, class L, class Stoppable>
    struct has_stop_token<const basic_future<T, S, L, Stoppable> &> : Stoppable
    {};
    /// @}

    /// \name Define basic_futures as being stoppable (not the same as having a
    /// stop token for other future types)
    /// @{
    template <class T, class S, class L, class Stoppable>
    struct is_stoppable<basic_future<T, S, L, Stoppable>> : Stoppable
    {};
    template <class T, class S, class L, class Stoppable>
    struct is_stoppable<basic_future<T, S, L, Stoppable> &> : Stoppable
    {};
    template <class T, class S, class L, class Stoppable>
    struct is_stoppable<basic_future<T, S, L, Stoppable> &&> : Stoppable
    {};
    template <class T, class S, class L, class Stoppable>
    struct is_stoppable<const basic_future<T, S, L, Stoppable>> : Stoppable
    {};
    template <class T, class S, class L, class Stoppable>
    struct is_stoppable<const basic_future<T, S, L, Stoppable> &> : Stoppable
    {};
/** @} */
#endif

    /** @} */
    /** @} */
} // namespace futures

#endif // FUTURES_BASIC_FUTURE_H

// #include <futures/futures/detail/traits/async_result_value_type.h>
#ifndef FUTURES_ASYNC_RESULT_VALUE_TYPE_H
#define FUTURES_ASYNC_RESULT_VALUE_TYPE_H

// #include <futures/futures/stop_token.h>

// #include <futures/futures/detail/traits/type_member_or_void.h>
#ifndef FUTURES_TYPE_MEMBER_OR_VOID_H
#define FUTURES_TYPE_MEMBER_OR_VOID_H

// #include <type_traits>


namespace futures::detail {
    /** \addtogroup futures Futures
     *  @{
     */

    /// \brief Return T::type or void as a placeholder if T::type doesn't exist
    /// This class is meant to avoid errors in std::conditional
    template <class, class = void>
    struct type_member_or_void
    {
        using type = void;
    };
    template <class T>
    struct type_member_or_void<T, std::void_t<typename T::type>>
    {
        using type = typename T::type;
    };
    template <class T>
    using type_member_or_void_t = typename type_member_or_void<T>::type;

    /** @} */
} // namespace futures::detail

#endif // FUTURES_TYPE_MEMBER_OR_VOID_H


namespace futures::detail {
    /** \addtogroup futures Futures
     *  @{
     */

    /// \brief The return type of a callable given to futures::async with the
    /// Args... This is the value type of the future object returned by async.
    /// In typical implementations this is usually the same as
    /// result_of_t<Function, Args...>. However, our implementation is a little
    /// different as the stop_token is provided by the async function and is
    /// thus not a part of Args, so both paths need to be considered.
    template <typename Function, typename... Args>
    using async_result_value_type = std::conditional<
        std::is_invocable_v<std::decay_t<Function>, stop_token, Args...>,
        type_member_or_void_t<
            std::invoke_result<std::decay_t<Function>, stop_token, Args...>>,
        type_member_or_void_t<
            std::invoke_result<std::decay_t<Function>, Args...>>>;

    template <typename Function, typename... Args>
    using async_result_value_type_t =
        typename async_result_value_type<Function, Args...>::type;

    /** @} */
} // namespace futures::detail


#endif // FUTURES_ASYNC_RESULT_VALUE_TYPE_H


namespace futures::detail {
    /** \addtogroup futures Futures
     *  @{
     */

    /// \brief The future type that results from calling async with a function
    /// <Function, Args...> This is the future type returned by async. In
    /// typical implementations this is usually the same as
    /// future<result_of_t<Function, Args...>>. However, our implementation is a
    /// little different as the stop_token is provided by the async function and
    /// can thus influence the resulting future type, so both paths need to be
    /// considered. Whenever we call async, we return a future with lazy
    /// continuations by default because we don't know if the user will need
    /// efficient continuations. Also, when the function expects a stop token,
    /// we return a jfuture.
    template <typename Function, typename... Args>
    using async_result_of = std::conditional<
        std::is_invocable_v<std::decay_t<Function>, stop_token, Args...>,
        jcfuture<async_result_value_type_t<Function, Args...>>,
        cfuture<async_result_value_type_t<Function, Args...>>>;

    template <typename Function, typename... Args>
    using async_result_of_t = typename async_result_of<Function, Args...>::type;

    /** @} */
} // namespace futures::detail

#endif // FUTURES_ASYNC_RESULT_OF_H


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
     * This module contains function we can use to launch and schedule tasks. If
     * possible, tasks should be scheduled lazily instead of launched eagerly to
     * avoid a race between the task and its dependencies.
     *
     * When tasks are scheduled eagerly, the function @ref async provides an
     * alternatives to launch tasks on specific executors instead of creating a
     * new thread for each asynchronous task.
     *
     *  @{
     */

    namespace detail {
        enum class schedule_future_policy
        {
            /// \brief Internal tag indicating the executor should post the
            /// execution
            post,
            /// \brief Internal tag indicating the executor should dispatch the
            /// execution
            dispatch,
            /// \brief Internal tag indicating the executor should defer the
            /// execution
            defer,
        };

        /// \brief A single trait to validate and constraint futures::async
        /// input types
        template <class Executor, class Function, typename... Args>
        using is_valid_async_input = std::disjunction<
            is_executor_then_function<Executor, Function, Args...>,
            is_executor_then_stoppable_function<Executor, Function, Args...>>;

        /// \brief A single trait to validate and constraint futures::async
        /// input types
        template <class Executor, class Function, typename... Args>
        constexpr bool is_valid_async_input_v
            = is_valid_async_input<Executor, Function, Args...>::value;

        /// \brief Create a new stop source for the new shared state
        template <bool expects_stop_token>
        auto
        create_stop_source() {
            if constexpr (expects_stop_token) {
                stop_source ss;
                return std::make_pair(ss, ss.get_token());
            } else {
                return std::make_pair(empty_value, empty_value);
            }
        }

        /// This function is defined as a functor to facilitate friendship in
        /// basic_future
        struct async_future_scheduler
        {
            /// This is a functor to fulfill a promise in a packaged task
            /// Handle to fulfill promise. Asio requires us to create a handle
            /// because callables need to be *copy constructable*. Continuations
            /// also require us to create an extra handle because we need to run
            /// them after the function is over.
            template <class StopToken, class Function, class Task, class... Args>
            class promise_fulfill_handle
                : public maybe_empty<StopToken>
                , // stop token for stopping the process is represented in base
                  // class
                  public maybe_empty<std::tuple<
                      Args...>> // arguments bound to the function to fulfill
                                // the promise also have empty base opt
            {
            public:
                promise_fulfill_handle(
                    Task &&pt,
                    std::tuple<Args...> &&args,
                    continuations_source cs,
                    const StopToken &st)
                    : maybe_empty<StopToken>(st),
                      maybe_empty<std::tuple<Args...>>(std::move(args)),
                      pt_(std::forward<Task>(pt)),
                      continuations_(std::move(cs)) {}

                void
                operator()() {
                    // Fulfill promise
                    if constexpr (
                        std::is_invocable_v<Function, StopToken, Args...>) {
                        std::apply(
                            pt_,
                            std::tuple_cat(
                                std::make_tuple(token()),
                                std::move(args())));
                    } else {
                        std::apply(pt_, std::move(args()));
                    }
                    // Run future continuations
                    continuations_.request_run();
                }

                /// \brief Get stop token from the base class as function for
                /// convenience
                const StopToken &
                token() const {
                    return maybe_empty<StopToken>::get();
                }

                /// \brief Get stop token from the base class as function for
                /// convenience
                StopToken &
                token() {
                    return maybe_empty<StopToken>::get();
                }

                /// \brief Get args from the base class as function for
                /// convenience
                const std::tuple<Args...> &
                args() const {
                    return maybe_empty<std::tuple<Args...>>::get();
                }

                /// \brief Get args from the base class as function for
                /// convenience
                std::tuple<Args...> &
                args() {
                    return maybe_empty<std::tuple<Args...>>::get();
                }

            private:
                /// \brief Task we need to fulfill the promise and its shared
                /// state
                Task pt_;

                /// \brief Continuation source for next futures
                continuations_source continuations_;
            };

            /// \brief Schedule the function in the executor
            /// This is the internal function async uses to finally schedule the
            /// function after setting the default parameters and converting
            /// policies into scheduling strategies.
            template <
                typename Executor,
                typename Function,
                typename... Args
#ifndef FUTURES_DOXYGEN
                ,
                std::enable_if_t<
                    is_valid_async_input_v<Executor, Function, Args...>,
                    int> = 0
#endif
                >
            async_result_of_t<Function, Args...>
            operator()(
                schedule_future_policy policy,
                const Executor &ex,
                Function &&f,
                Args &&...args) const {
                using future_value_type
                    = async_result_value_type_t<Function, Args...>;
                using future_type = async_result_of_t<Function, Args...>;

                // Shared sources
                constexpr bool expects_stop_token = std::
                    is_invocable_v<Function, stop_token, Args...>;
                auto [s_source, s_token] = create_stop_source<
                    expects_stop_token>();
                continuations_source cs;

                // Set up shared state
                using packaged_task_type = std::conditional_t<
                    expects_stop_token,
                    packaged_task<
                        future_value_type(stop_token, std::decay_t<Args>...)>,
                    packaged_task<future_value_type(std::decay_t<Args>...)>>;
                packaged_task_type pt{ std::forward<Function>(f) };
                future_type result{ pt.template get_future<future_type>() };
                result.set_continuations_source(cs);
                if constexpr (expects_stop_token) {
                    result.set_stop_source(s_source);
                }
                promise_fulfill_handle<
                    std::decay_t<decltype(s_token)>,
                    Function,
                    packaged_task_type,
                    Args...>
                    fulfill_promise(
                        std::move(pt),
                        std::make_tuple(std::forward<Args>(args)...),
                        cs,
                        s_token);

                // Fire-and-forget: Post a handle running the complete function
                // to the executor
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

    /// \brief Launch an asynchronous task with the specified executor and
    /// policy
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
    template <
        typename Executor,
        typename Function,
        typename... Args
#ifndef FUTURES_DOXYGEN
        ,
        std::enable_if_t<
            detail::is_valid_async_input_v<Executor, Function, Args...>,
            int> = 0
#endif
        >
#ifndef FUTURES_DOXYGEN
    decltype(auto)
#else
    __implementation_defined__
#endif
    async(launch policy, const Executor &ex, Function &&f, Args &&...args) {
        // Unwrap policies
        const bool new_thread_policy = (policy & launch::new_thread)
                                       == launch::new_thread;
        const bool deferred_policy = (policy & launch::deferred)
                                     == launch::deferred;
        const bool inline_now_policy = (policy & launch::inline_now)
                                       == launch::inline_now;
        const bool executor_policy = (policy & launch::executor)
                                     == launch::executor;
        const bool executor_now_policy = (policy & launch::executor_now)
                                         == launch::executor_now;
        const bool executor_later_policy = (policy & launch::executor_later)
                                           == launch::executor_later;

        // Define executor
        const bool use_default_executor = executor_policy && executor_now_policy
                                          && executor_later_policy;
        const bool use_new_thread_executor = (!use_default_executor)
                                             && new_thread_policy;
        const bool use_inline_later_executor = (!use_default_executor)
                                               && deferred_policy;
        const bool use_inline_executor = (!use_default_executor)
                                         && inline_now_policy;
        const bool no_executor_defined = !(
            use_default_executor || use_new_thread_executor
            || use_inline_later_executor || use_inline_executor);

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

        return detail::schedule_future(
            schedule_policy,
            ex,
            std::forward<Function>(f),
            std::forward<Args>(args)...);
    }

    /// \brief Launch a task with a custom executor instead of policies.
    ///
    /// This version of the async function will always use the specified
    /// executor instead of creating a new thread.
    ///
    /// If no executor is provided, then the function is run in a default
    /// executor created from the default thread pool.
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
    template <
        typename Executor,
        typename Function,
        typename... Args
#ifndef FUTURES_DOXYGEN
        ,
        std::enable_if_t<
            detail::is_valid_async_input_v<Executor, Function, Args...>,
            int> = 0
#endif
        >
#ifndef FUTURES_DOXYGEN
    detail::async_result_of_t<Function, Args...>
#else
    __implementation_defined__
#endif
    async(const Executor &ex, Function &&f, Args &&...args) {
        return async(
            launch::async,
            ex,
            std::forward<Function>(f),
            std::forward<Args>(args)...);
    }

    /// \brief Launch an async function according to the specified policy with
    /// the default executor
    ///
    /// \tparam Function A callable object
    /// \tparam Args Arguments for the Function
    ///
    /// \param policy Launch policy
    /// \param f Function to execute
    /// \param args Function arguments
    ///
    /// \return A future object with the function results
    template <
        typename Function,
        typename... Args
#ifndef FUTURES_DOXYGEN
        ,
        std::enable_if_t<
            detail::is_async_input_non_executor_v<Function, Args...>,
            int> = 0
#endif
        >
#ifndef FUTURES_DOXYGEN
    detail::async_result_of_t<Function, Args...>
#else
    __implementation_defined__
#endif
    async(launch policy, Function &&f, Args &&...args) {
        return async(
            policy,
            make_default_executor(),
            std::forward<Function>(f),
            std::forward<Args>(args)...);
    }

    /// \brief Launch an async function with the default executor of type @ref
    /// default_executor_type
    ///
    /// \tparam Executor Executor from an execution context
    /// \tparam Function A callable object
    /// \tparam Args Arguments for the Function
    ///
    /// \param f Function to execute
    /// \param args Function arguments
    ///
    /// \return A future object with the function results
    template <
        typename Function,
        typename... Args
#ifndef FUTURES_DOXYGEN
        ,
        std::enable_if_t<
            detail::is_async_input_non_executor_v<Function, Args...>,
            int> = 0
#endif
        >
#ifndef FUTURES_DOXYGEN
    detail::async_result_of_t<Function, Args...>
#else
    __implementation_defined__
#endif
    async(Function &&f, Args &&...args) {
        return async(
            launch::async,
            ::futures::make_default_executor(),
            std::forward<Function>(f),
            std::forward<Args>(args)...);
    }

    /** @} */
    /** @} */
    /** @} */
} // namespace futures

#endif // FUTURES_ASYNC_H

// #include <futures/futures/await.h>

// #include <futures/futures/basic_future.h>

// #include <futures/futures/packaged_task.h>
#ifndef FUTURES_PACKAGED_TASK_H
#define FUTURES_PACKAGED_TASK_H

// #include <futures/futures/basic_future.h>

// #include <futures/futures/detail/shared_task.h>
#ifndef FUTURES_SHARED_TASK_H
#define FUTURES_SHARED_TASK_H

// #include <futures/futures/detail/empty_base.h>

// #include <futures/futures/detail/shared_state.h>

// #include <futures/futures/detail/to_address.h>
#ifndef FUTURES_TO_ADDRESS_H
#define FUTURES_TO_ADDRESS_H

/// \file
/// Replicate the C++20 to_address functionality in C++17

// #include <memory>


namespace futures::detail {
    /// \brief Obtain the address represented by p without forming a reference
    /// to the object pointed to by p This is the "fancy pointer" overload: If
    /// the expression std::pointer_traits<Ptr>::to_address(p) is well-formed,
    /// returns the result of that expression. Otherwise, returns
    /// std::to_address(p.operator->()). \tparam T Element type \param v Element
    /// pointer \return Element address
    template <class T>
    constexpr T *
    to_address(T *v) noexcept {
        return v;
    }

    /// \brief Obtain the address represented by p without forming a reference
    /// to the object pointed to by p This is the "raw pointer overload": If T
    /// is a function type, the program is ill-formed. Otherwise, returns p
    /// unmodified. \tparam T Element type \param v Raw pointer \return Element
    /// address
    template <class T>
    inline typename std::pointer_traits<T>::element_type *
    to_address(const T &v) noexcept {
        return to_address(v.operator->());
    }
} // namespace futures::detail

#endif // FUTURES_TO_ADDRESS_H


namespace futures::detail {
    /** \addtogroup futures Futures
     *  @{
     */

    /// \brief Members common to shared tasks
    ///
    /// While the main purpose of shared_state_base is to differentiate the
    /// versions of `set_value`, the main purpose of this task base class is to
    /// nullify the function type and allocator from the concrete task
    /// implementation in the final packaged task.
    ///
    /// \tparam R Type returned by the task callable
    /// \tparam Args Argument types to run the task callable
    template <typename R, typename... Args>
    class shared_task_base : public shared_state<R>
    {
    public:
        /// \brief Virtual task destructor
        virtual ~shared_task_base() = default;

        /// \brief Virtual function to run the task with its Args
        /// \param args Arguments
        virtual void
        run(Args &&...args)
            = 0;

        /// \brief Reset the state
        ///
        /// This function returns a new pointer to this shared task where we
        /// reallocate everything
        ///
        /// \return New pointer to a shared_task
        virtual std::shared_ptr<shared_task_base>
        reset() = 0;
    };

    /// \brief A shared task object, that also stores the function to create the
    /// shared state
    ///
    /// A shared_task extends the shared state with a task. A task is an
    /// extension of and analogous with shared states. The main difference is
    /// that tasks also define a function that specify how to create the state,
    /// with the `run` function.
    ///
    /// In practice, a shared_task are to a packaged_task what a shared state is
    /// to a promise.
    ///
    /// \tparam R Type returned by the task callable
    /// \tparam Args Argument types to run the task callable
    template <typename Fn, typename Allocator, typename R, typename... Args>
    class shared_task
        : public shared_task_base<R, Args...>
#ifndef FUTURES_DOXYGEN
        , public maybe_empty<Fn>
        , public maybe_empty<
              /* allocator_type */ typename std::allocator_traits<Allocator>::
                  template rebind_alloc<shared_task<Fn, Allocator, R, Args...>>>
#endif
    {
    public:
        /// \brief Allocator used to allocate this task object type
        using allocator_type = typename std::allocator_traits<
            Allocator>::template rebind_alloc<shared_task>;

        /// \brief Construct a task object for the specified allocator and
        /// function, copying the function
        shared_task(const allocator_type &alloc, const Fn &fn)
            : shared_task_base<R, Args...>{}, maybe_empty<Fn>{ fn },
              maybe_empty<allocator_type>{ alloc } {}

        /// \brief Construct a task object for the specified allocator and
        /// function, moving the function
        shared_task(const allocator_type &alloc, Fn &&fn)
            : shared_task_base<R, Args...>{}, maybe_empty<Fn>{ std::move(fn) },
              maybe_empty<allocator_type>{ alloc } {}

        /// \brief No copy constructor
        shared_task(shared_task const &) = delete;

        /// \brief No copy assignment
        shared_task &
        operator=(shared_task const &)
            = delete;

        /// \brief Virtual shared task destructor
        virtual ~shared_task() = default;

        /// \brief Run the task function with the given arguments and use the
        /// result to set the shared state value \param args Arguments
        void
        run(Args &&...args) final {
            try {
                if constexpr (std::is_same_v<R, void>) {
                    std::apply(
                        fn(),
                        std::make_tuple(std::forward<Args>(args)...));
                    this->set_value();
                } else {
                    this->set_value(std::apply(
                        fn(),
                        std::make_tuple(std::forward<Args>(args)...)));
                }
            }
            catch (...) {
                this->set_exception(std::current_exception());
            }
        }

        /// \brief Reallocate and reconstruct a task object
        ///
        /// This constructs a task object of same type from scratch.
        typename std::shared_ptr<shared_task_base<R, Args...>>
        reset() final {
            return std::allocate_shared<
                shared_task>(alloc(), alloc(), std::move(fn()));
        }

    private:
        /// @name Maybe-empty internal members
        /// @{

        /// \brief Internal function object representing the task function
        const Fn &
        fn() const {
            return maybe_empty<Fn>::get();
        }

        /// \brief Internal function object representing the task function
        Fn &
        fn() {
            return maybe_empty<Fn>::get();
        }

        /// \brief Internal function object representing the task function
        const allocator_type &
        alloc() const {
            return maybe_empty<allocator_type>::get();
        }

        /// \brief Internal function object representing the task function
        allocator_type &
        alloc() {
            return maybe_empty<allocator_type>::get();
        }

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
    template <typename Signature>
    class packaged_task;
#endif

    /// \brief A packaged task that sets a shared state when done
    ///
    /// A packaged task holds a task to be executed and a shared state for its
    /// result.
    ///
    /// It's very similar to a promise where the shared state is replaced by a
    /// shared task.
    ///
    /// \tparam R Return type
    /// \tparam Args Task arguments
#ifndef FUTURES_DOXYGEN
    template <typename R, typename... Args>
#else
    template <typename Signature>
#endif
    class packaged_task<R(Args...)>
    {
    public:
        /// \brief Constructs a std::packaged_task object with no task and no
        /// shared state
        packaged_task() = default;

        /// \brief Construct a packaged task from a function with the default
        /// std allocator
        ///
        /// \par Constraints
        /// This constructor only participates in overload resolution if Fn is
        /// not a packaged task itself.
        ///
        /// \tparam Fn Function type
        /// \param fn The callable target to execute
        template <
            typename Fn
#ifndef FUTURES_DOXYGEN
            ,
            typename = std::enable_if_t<
                !std::is_base_of_v<packaged_task, typename std::decay_t<Fn>>>
#endif
            >
        explicit packaged_task(Fn &&fn)
            : packaged_task{ std::allocator_arg,
                             std::allocator<packaged_task>{},
                             std::forward<Fn>(fn) } {
        }

        /// \brief Constructs a std::packaged_task object with a shared state
        /// and a copy of the task
        ///
        /// This function constructs a std::packaged_task object with a shared
        /// state and a copy of the task, initialized with std::forward<Fn>(fn).
        /// It uses the provided allocator to allocate memory necessary to store
        /// the task.
        ///
        /// \par Constraints
        /// This constructor does not participate in overload resolution if
        /// std::decay<Fn>::type is the same type as
        /// std::packaged_task<R(ArgTypes...)>.
        ///
        /// \tparam Fn Function type
        /// \tparam Allocator Allocator type
        /// \param alloc The allocator to use when storing the task
        /// \param fn The callable target to execute
        template <
            typename Fn,
            typename Allocator
#ifndef FUTURES_DOXYGEN
            ,
            typename = std::enable_if_t<
                !std::is_base_of_v<packaged_task, typename std::decay_t<Fn>>>
#endif
            >
        explicit packaged_task(
            std::allocator_arg_t,
            const Allocator &alloc,
            Fn &&fn) {
            task_ = std::allocate_shared<
                detail::shared_task<std::decay_t<Fn>, Allocator, R, Args...>>(
                alloc,
                alloc,
                std::forward<Fn>(fn));
        }

        /// \brief The copy constructor is deleted, std::packaged_task is
        /// move-only.
        packaged_task(packaged_task const &) = delete;

        /// \brief Constructs a std::packaged_task with the shared state and
        /// task formerly owned by other
        packaged_task(packaged_task &&other) noexcept
            : future_retrieved_{ other.future_retrieved_ },
              task_{ std::move(other.task_) } {
            other.future_retrieved_ = false;
        }

        /// \brief The copy assignment is deleted, std::packaged_task is
        /// move-only.
        packaged_task &
        operator=(packaged_task const &)
            = delete;

        /// \brief Assigns a std::packaged_task with the shared state and task
        /// formerly owned by other
        packaged_task &
        operator=(packaged_task &&other) noexcept {
            if (this != &other) {
                packaged_task tmp{ std::move(other) };
                swap(tmp);
            }
            return *this;
        }

        /// \brief Destructs the task object
        ~packaged_task() {
            if (task_ && future_retrieved_) {
                task_->signal_promise_destroyed();
            }
        }

        /// \brief Checks if the task object has a valid function
        ///
        /// \return true if *this has a shared state, false otherwise
        [[nodiscard]] bool
        valid() const noexcept {
            return task_ != nullptr;
        }

        /// \brief Swaps two task objects
        ///
        /// This function exchanges the shared states and stored tasks of *this
        /// and other
        ///
        /// \param other packaged task whose state to swap with
        void
        swap(packaged_task &other) noexcept {
            std::swap(future_retrieved_, other.future_retrieved_);
            task_.swap(other.task_);
        }

        /// \brief Returns a future object associated with the promised result
        ///
        /// This function constructs a future object that shares its state with
        /// this promise Because this library handles more than a single future
        /// type, the future type we want is a template parameter. This function
        /// expects future type constructors to accept pointers to shared states.
        template <class Future = cfuture<R>>
        Future
        get_future() {
            if (future_retrieved_) {
                throw future_already_retrieved{};
            }
            if (!valid()) {
                throw packaged_task_uninitialized{};
            }
            future_retrieved_ = true;
            return Future{ std::static_pointer_cast<shared_state<R>>(task_) };
        }

        /// \brief Executes the function and set the shared state
        ///
        /// Calls the stored task with args as the arguments. The return value
        /// of the task or any exceptions thrown are stored in the shared state
        /// The shared state is made ready and any threads waiting for this are
        /// unblocked.
        ///
        /// \param args the parameters to pass on invocation of the stored task
        void
        operator()(Args... args) {
            if (!valid()) {
                throw packaged_task_uninitialized{};
            }
            task_->run(std::forward<Args>(args)...);
        }

        /// \brief Resets the shared state abandoning any stored results of
        /// previous executions
        ///
        /// Resets the state abandoning the results of previous executions. A
        /// new shared state is constructed. Equivalent to *this =
        /// packaged_task(std::move(f)), where f is the stored task.
        void
        reset() {
            if (!valid()) {
                throw packaged_task_uninitialized{};
            }
            task_ = task_->reset();
            future_retrieved_ = false;
        }

    private:
        /// \brief True if the corresponding future has already been retrieved
        bool future_retrieved_{ false };

        /// \brief The function this task should execute
        std::shared_ptr<detail::shared_task_base<R, Args...>> task_{};
    };

    /// \brief Specializes the std::swap algorithm
    template <typename Signature>
    void
    swap(packaged_task<Signature> &l, packaged_task<Signature> &r) noexcept {
        l.swap(r);
    }

    /** @} */
    /** @} */
} // namespace futures

#endif // FUTURES_PACKAGED_TASK_H

// #include <futures/futures/promise.h>
#ifndef FUTURES_PROMISE_H
#define FUTURES_PROMISE_H

// #include <futures/futures/basic_future.h>

// #include <futures/futures/detail/empty_base.h>

// #include <futures/futures/detail/shared_state.h>

// #include <futures/futures/detail/to_address.h>

// #include <memory>


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
    /// This includes a pointer to the corresponding shared_state for the future
    /// and the functions to manage the promise.
    ///
    /// The specific promise specialization will only differ by their set_value
    /// functions.
    ///
    template <typename R>
    class promise_base
    {
    public:
        /// \brief Create the base promise with std::allocator
        ///
        /// Use std::allocator_arg tag to dispatch and select allocator aware
        /// constructor
        promise_base()
            : promise_base{ std::allocator_arg,
                            std::allocator<promise_base>{} } {}

        /// \brief Create a base promise setting the shared state with the
        /// specified allocator
        ///
        /// This function allocates memory for and allocates an initial
        /// promise_shared_state (the future value) with the specified
        /// allocator. This object is stored in the internal intrusive pointer
        /// as the future shared state.
        template <typename Allocator>
        promise_base(std::allocator_arg_t, Allocator alloc)
            : shared_state_(std::allocate_shared<shared_state<R>>(alloc)) {}

        /// \brief No copy constructor
        promise_base(promise_base const &) = delete;

        /// \brief Move constructor
        promise_base(promise_base &&other) noexcept
            : obtained_{ other.obtained_ },
              shared_state_{ std::move(other.shared_state_) } {
            other.obtained_ = false;
        }

        /// \brief No copy assignment
        promise_base &
        operator=(promise_base const &)
            = delete;

        /// \brief Move assignment
        promise_base &
        operator=(promise_base &&other) noexcept {
            if (this != &other) {
                promise_base tmp{ std::move(other) };
                swap(tmp);
            }
            return *this;
        }

        /// \brief Destructor
        ///
        /// This promise owns the shared state, so we need to warn the shared
        /// state when it's destroyed.
        virtual ~promise_base() {
            if (shared_state_ && obtained_) {
                shared_state_->signal_promise_destroyed();
            }
        }

        /// \brief Gets a future that shares its state with this promise
        ///
        /// This function constructs a future object that shares its state with
        /// this promise. Because this library handles more than a single future
        /// type, the future type we want is a template parameter.
        ///
        /// This function expects future type constructors to accept pointers to
        /// shared states.
        template <class Future = futures::cfuture<R>>
        Future
        get_future() {
            if (obtained_) {
                throw future_already_retrieved{};
            }
            if (!shared_state_) {
                throw promise_uninitialized{};
            }
            obtained_ = true;
            return Future{ shared_state_ };
        }

        /// \brief Set the promise result as an exception
        /// \note The set_value operation is only available at the concrete
        /// derived class, where we know the class type
        void
        set_exception(std::exception_ptr p) {
            if (!shared_state_) {
                throw promise_uninitialized{};
            }
            shared_state_->set_exception(p);
        }

        /// \brief Set the promise result as an exception
        template <
            typename E
#ifndef FUTURES_DOXYGEN
            ,
            std::enable_if_t<std::is_base_of_v<std::exception, E>, int> = 0
#endif
            >
        void
        set_exception(E e) {
            set_exception(std::make_exception_ptr(e));
        }

    protected:
        /// \brief Swap the value of two promises
        void
        swap(promise_base &other) noexcept {
            std::swap(obtained_, other.obtained_);
            shared_state_.swap(other.shared_state_);
        }

        /// \brief Intrusive pointer to the future corresponding to this promise
        constexpr std::shared_ptr<shared_state<R>> &
        get_shared_state() {
            return shared_state_;
        };

    private:
        /// \brief True if the future has already obtained the shared state
        bool obtained_{ false };

        /// \brief Intrusive pointer to the future corresponding to this promise
        std::shared_ptr<shared_state<R>> shared_state_{};
    };

    /// \brief A shared state that will later be acquired by a future type
    ///
    /// The difference between the promise specializations is only in how they
    /// handle their set_value functions.
    ///
    /// \tparam R The shared state type
    template <typename R>
    class promise : public promise_base<R>
    {
    public:
        /// \brief Create the promise for type R
        using promise_base<R>::promise_base;

        /// \brief Copy and set the promise value so it can be obtained by the
        /// future \param value lvalue reference to the shared state value
        void
        set_value(R const &value) {
            if (!promise_base<R>::get_shared_state()) {
                throw promise_uninitialized{};
            }
            promise_base<R>::get_shared_state()->set_value(value);
        }

        /// \brief Move and set the promise value so it can be obtained by the
        /// future \param value rvalue reference to the shared state value
        void
        set_value(R &&value) {
            if (!promise_base<R>::get_shared_state()) {
                throw promise_uninitialized{};
            }
            promise_base<R>::get_shared_state()->set_value(std::move(value));
        }

        /// \brief Swap the value of two promises
        void
        swap(promise &other) noexcept {
            promise_base<R>::swap(other);
        }
    };

    /// \brief A shared state that will later be acquired by a future type
    template <typename R>
    class promise<R &> : public promise_base<R &>
    {
    public:
        /// \brief Create the promise for type R&
        using promise_base<R &>::promise_base;

        /// \brief Set the promise value so it can be obtained by the future
        void
        set_value(R &value) {
            if (!promise_base<R &>::get_shared_state()) {
                throw promise_uninitialized{};
            }
            promise_base<R &>::get_shared_state()->set_value(value);
        }

        /// \brief Swap the value of two promises
        void
        swap(promise &other) noexcept {
            promise_base<R &>::swap(other);
        }
    };

    /// \brief A shared state that will later be acquired by a future type
    template <>
    class promise<void> : public promise_base<void>
    {
    public:
        /// \brief Create the promise for type void
        using promise_base<void>::promise_base;

        /// \brief Set the promise value, so it can be obtained by the future
        void
        set_value() { // NOLINT(readability-make-member-function-const)
            if (!promise_base<void>::get_shared_state()) {
                throw promise_uninitialized{};
            }
            promise_base<void>::get_shared_state()->set_value();
        }

        /// \brief Swap the value of two promises
        void
        swap(promise &other) noexcept {
            promise_base<void>::swap(other);
        }
    };

    /// \brief Swap the value of two promises
    template <typename R>
    void
    swap(promise<R> &l, promise<R> &r) noexcept {
        l.swap(r);
    }

    /** @} */
    /** @} */
} // namespace futures

#endif // FUTURES_PROMISE_H

// #include <futures/futures/wait_for_all.h>
#ifndef FUTURES_WAIT_FOR_ALL_H
#define FUTURES_WAIT_FOR_ALL_H

// #include <futures/algorithm/traits/iter_value.h>

// #include <futures/algorithm/traits/is_range.h>

// #include <futures/algorithm/traits/range_value.h>
#ifndef FUTURES_ALGORITHM_TRAITS_RANGE_VALUE_H
#define FUTURES_ALGORITHM_TRAITS_RANGE_VALUE_H

// #include <futures/algorithm/traits/is_range.h>

// #include <futures/algorithm/traits/iter_value.h>

// #include <futures/algorithm/traits/iterator.h>
#ifndef FUTURES_ALGORITHM_TRAITS_ITERATOR_T_H
#define FUTURES_ALGORITHM_TRAITS_ITERATOR_T_H

// #include <futures/algorithm/traits/has_element_type.h>

// #include <futures/algorithm/traits/has_iterator_traits_value_type.h>

// #include <futures/algorithm/traits/has_value_type.h>

// #include <futures/algorithm/traits/remove_cvref.h>

// #include <iterator>

// #include <type_traits>


namespace futures {
    /** \addtogroup algorithms Algorithms
     *  @{
     */

    /** \addtogroup traits Traits
     *  @{
     */


    /** \brief A C++17 type trait equivalent to the C++20 iterator_t
     * concept
     */
#ifdef FUTURES_DOXYGEN
    template <class T>
    using iterator = __see_below__;
#else
    template <class T, class = void>
    struct iterator
    {};

    template <class T>
    struct iterator<T, std::void_t<decltype(begin(std::declval<T&>()))>>
    {
        using type = decltype(begin(std::declval<T&>()));
    };
#endif
    template <class T>
    using iterator_t = typename iterator<T>::type;

    /** @}*/
    /** @}*/

} // namespace futures

#endif // FUTURES_ALGORITHM_TRAITS_ITERATOR_T_H

// #include <iterator>

// #include <type_traits>


namespace futures {
    /** \addtogroup algorithms Algorithms
     *  @{
     */

    /** \addtogroup traits Traits
     *  @{
     */


    /** \brief A C++17 type trait equivalent to the C++20 range_value
     * concept
     */
#ifdef FUTURES_DOXYGEN
    template <class R>
    using range_value = __see_below__;
#else
    template <class R, class = void>
    struct range_value
    {};

    template <class R>
    struct range_value<R, std::enable_if_t<is_range_v<R>>>
    {
        using type = iter_value_t<iterator_t<R>>;
    };
#endif
    template <class R>
    using range_value_t = typename range_value<R>::type;

    /** @}*/
    /** @}*/

} // namespace futures

#endif // FUTURES_ALGORITHM_TRAITS_RANGE_VALUE_H

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
    /// This function waits for all futures in the range [`first`, `last`) to be
    /// ready. It simply waits iteratively for each of the futures to be ready.
    ///
    /// \note This function is adapted from boost::wait_for_all
    ///
    /// \see
    /// [boost.thread
    /// wait_for_all](https://www.boost.org/doc/libs/1_78_0/doc/html/thread/synchronization.html#thread.synchronization.futures.reference.wait_for_all)
    ///
    /// \tparam Iterator Iterator type in a range of futures
    /// \param first Iterator to the first element in the range
    /// \param last Iterator to one past the last element in the range
    template <
        typename Iterator
#ifndef FUTURES_DOXYGEN
        ,
        typename std::
            enable_if_t<is_future_v<iter_value_t<Iterator>>, int> = 0
#endif
        >
    void
    wait_for_all(Iterator first, Iterator last) {
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
    /// [boost.thread
    /// wait_for_all](https://www.boost.org/doc/libs/1_78_0/doc/html/thread/synchronization.html#thread.synchronization.futures.reference.wait_for_all)
    ///
    /// \tparam Range A range of futures type
    /// \param r Range of futures
    template <
        typename Range
#ifndef FUTURES_DOXYGEN
        ,
        typename std::enable_if_t<
            is_range_v<Range> && is_future_v<range_value_t<Range>>,
            int> = 0
#endif
        >
    void
    wait_for_all(Range &&r) {
        using std::begin;
        wait_for_all(begin(r), end(r));
    }

    /// \brief Wait for a sequence of futures to be ready
    ///
    /// This function waits for all specified futures `fs`... to be ready.
    ///
    /// It creates a compile-time fixed-size data structure to store references
    /// to all of the futures and then waits for each of the futures to be
    /// ready.
    ///
    /// \note This function is adapted from boost::wait_for_all
    ///
    /// \see
    /// [boost.thread
    /// wait_for_all](https://www.boost.org/doc/libs/1_78_0/doc/html/thread/synchronization.html#thread.synchronization.futures.reference.wait_for_all)
    ///
    /// \tparam Fs A list of future types
    /// \param fs A list of future objects
    template <
        typename... Fs
#ifndef FUTURES_DOXYGEN
        ,
        typename std::enable_if_t<
            std::conjunction_v<is_future<std::decay_t<Fs>>...>,
            int> = 0
#endif
        >
    void
    wait_for_all(Fs &&...fs) {
        (fs.wait(), ...);
    }

    /** @} */
    /** @} */
} // namespace futures

#endif // FUTURES_WAIT_FOR_ALL_H

// #include <futures/futures/wait_for_any.h>
#ifndef FUTURES_WAIT_FOR_ANY_H
#define FUTURES_WAIT_FOR_ANY_H

// #include <futures/algorithm/traits/iter_value.h>

// #include <futures/algorithm/traits/is_range.h>

// #include <futures/algorithm/traits/range_value.h>

// #include <futures/futures/traits/is_future.h>

// #include <futures/futures/detail/waiter_for_any.h>
#ifndef FUTURES_WAITER_FOR_ANY_H
#define FUTURES_WAITER_FOR_ANY_H

// #include <futures/futures/detail/lock.h>
#ifndef FUTURES_LOCK_H
#define FUTURES_LOCK_H

// #include <futures/algorithm/traits/is_input_iterator.h>


namespace futures::detail {
    /** \addtogroup futures Futures
     *  @{
     */

    /// \brief Try to lock range of mutexes in a way that all of them should
    /// work
    ///
    /// Calls try_lock() on each of the Lockable objects in the supplied range.
    /// If any of the calls to try_lock() returns false then all locks acquired
    /// are released and an iterator referencing the failed lock is returned.
    ///
    /// If any of the try_lock() operations on the supplied Lockable objects
    /// throws an exception any locks acquired by the function will be released
    /// before the function exits.
    ///
    /// \throws exception Any exceptions thrown by calling try_lock() on the
    /// supplied Lockable objects
    ///
    /// \post All the supplied Lockable objects are locked by the calling
    /// thread.
    ///
    /// \see
    /// https://www.boost.org/doc/libs/1_78_0/doc/html/thread/synchronization.html#thread.synchronization.lock_functions.try_lock_range
    ///
    /// \tparam Iterator Range iterator type
    /// \param first Iterator to first mutex in the range
    /// \param last Iterator to one past the last mutex in the range
    /// \return Iterator to first element that could *not* be locked, or `end`
    /// if all the supplied Lockable objects are now locked
    template <
        typename Iterator,
        std::enable_if_t<is_input_iterator_v<Iterator>, int> = 0>
    Iterator
    try_lock(Iterator first, Iterator last) {
        using lock_type = typename std::iterator_traits<Iterator>::value_type;

        // Handle trivial cases
        if (const bool empty_range = first == last; empty_range) {
            return last;
        } else if (const bool single_element = std::next(first) == last;
                   single_element) {
            if (first->try_lock()) {
                return last;
            } else {
                return first;
            }
        }

        // General cases: Try to lock first and already return if fails
        std::unique_lock<lock_type> guard_first(*first, std::try_to_lock);
        if (const bool locking_failed = !guard_first.owns_lock();
            locking_failed) {
            return first;
        }

        // While first is locked by guard_first, try to lock the other elements
        // in the range
        const Iterator failed_mutex_it = try_lock(std::next(first), last);
        if (const bool none_failed = failed_mutex_it == last; none_failed) {
            // Break the association of the associated mutex (i.e. don't unlock
            // at destruction)
            guard_first.release();
        }
        return failed_mutex_it;
    }

    /// \brief Lock range of mutexes in a way that avoids deadlock
    ///
    /// Locks the Lockable objects in the range [`first`, `last`) supplied as
    /// arguments in an unspecified and indeterminate order in a way that avoids
    /// deadlock. It is safe to call this function concurrently from multiple
    /// threads for any set of mutexes (or other lockable objects) in any order
    /// without risk of deadlock. If any of the lock() or try_lock() operations
    /// on the supplied Lockable objects throws an exception any locks acquired
    /// by the function will be released before the function exits.
    ///
    /// \throws exception Any exceptions thrown by calling lock() or try_lock()
    /// on the supplied Lockable objects
    ///
    /// \post All the supplied Lockable objects are locked by the calling
    /// thread.
    ///
    /// \see
    /// https://www.boost.org/doc/libs/1_78_0/doc/html/thread/synchronization.html#thread.synchronization.lock_functions
    ///
    /// \tparam Iterator Range iterator type
    /// \param first Iterator to first mutex in the range
    /// \param last Iterator to one past the last mutex in the range
    template <
        typename Iterator,
        std::enable_if_t<is_input_iterator_v<Iterator>, int> = 0>
    void
    lock(Iterator first, Iterator last) {
        using lock_type = typename std::iterator_traits<Iterator>::value_type;

        /// \brief Auxiliary lock guard for a range of mutexes recursively using
        /// this lock function
        struct range_lock_guard
        {
            /// Iterator to first locked mutex in the range
            Iterator begin;

            /// Iterator to one past last locked mutex in the range
            Iterator end;

            /// \brief Construct a lock guard for a range of mutexes
            range_lock_guard(Iterator first, Iterator last)
                : begin(first), end(last) {
                // The range lock guard recursively calls the same lock function
                // we use here
                futures::detail::lock(begin, end);
            }

            range_lock_guard(const range_lock_guard &) = delete;
            range_lock_guard &
            operator=(const range_lock_guard &)
                = delete;

            range_lock_guard(range_lock_guard &&other) noexcept
                : begin(std::exchange(other.begin, Iterator{})),
                  end(std::exchange(other.end, Iterator{})) {}

            range_lock_guard &
            operator=(range_lock_guard &&other) noexcept {
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
            void
            release() {
                begin = end;
            }
        };

        // Handle trivial cases
        if (const bool empty_range = first == last; empty_range) {
            return;
        } else if (const bool single_element = std::next(first) == last;
                   single_element) {
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
            // A deferred lock assumes the algorithm might lock the first lock
            // later
            std::unique_lock<lock_type> first_lock(*first, std::defer_lock);
            if (currently_using_first_strategy) {
                // First strategy: Lock first, then _try_ to lock the others
                first_lock.lock();
                const Iterator failed_lock_it = try_lock(next, last);
                if (const bool no_lock_failed = failed_lock_it == last;
                    no_lock_failed) {
                    // !SUCCESS!
                    // Breaks the association of the associated mutex (i.e.
                    // don't unlock first_lock)
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
                    if (const bool all_locked = failed_lock == next; all_locked)
                    {
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

// #include <utility>


namespace futures::detail {
    /** \addtogroup futures Futures
     *  @{
     */

    /// \brief Helper class to set signals and wait for any future in a sequence
    /// of futures to become ready
    class waiter_for_any
    {
    public:
        /// \brief Construct a waiter_for_any watching zero futures
        waiter_for_any() = default;

        /// \brief Destruct the waiter
        ///
        /// If the waiter is destroyed before we wait for a result, we disable
        /// the future notifications
        ///
        ~waiter_for_any() {
            for (auto const &waiter: waiters_) {
                waiter.disable_notification();
            }
        }

        /// \brief Construct a waiter_for_any that waits for one of the futures
        /// in a range of futures
        template <typename Iterator>
        waiter_for_any(Iterator first, Iterator last) {
            for (Iterator current = first; current != last; ++current) {
                add(*current);
            }
        }

        waiter_for_any(const waiter_for_any &) = delete;
        waiter_for_any(waiter_for_any &&) = delete;
        waiter_for_any &
        operator=(const waiter_for_any &)
            = delete;
        waiter_for_any &
        operator=(waiter_for_any &&)
            = delete;

        /// \brief Watch the specified future
        template <typename F>
        void
        add(F &f) {
            if constexpr (has_ready_notifier_v<std::decay_t<F>>) {
                if (f.valid()) {
                    registered_waiter
                        waiter(f, f.notify_when_ready(cv), future_count);
                    try {
                        waiters_.push_back(waiter);
                    }
                    catch (...) {
                        f.unnotify_when_ready(waiter.handle);
                        throw;
                    }
                    ++future_count;
                }
            } else {
                // The future has no ready-notifier, so we create a future to
                // poll until it can notify us This is the future we wait for
                // instead
                poller_futures_.emplace_back(futures::async([f = &f]() {
                    f->wait();
                }));
                add(poller_futures_.back());
            }
        }

        /// \brief Watch the specified futures in the parameter pack
        template <typename F1, typename... Fs>
        void
        add(F1 &&f1, Fs &&...fs) {
            add(std::forward<F1>(f1));
            add(std::forward<Fs>(fs)...);
        }

        /// \brief Wait for one of the futures to notify it got ready
        std::size_t
        wait() {
            registered_waiter_range_lock lk(waiters_);
            std::size_t ready_idx;
            cv.wait(lk, [this, &ready_idx]() {
                for (auto const &waiter: waiters_) {
                    if (waiter.is_ready()) {
                        ready_idx = waiter.index;
                        return true;
                    }
                }
                return false;
            });
            return ready_idx;
        }

    private:
        /// \brief Type of handle in the future object used to notify completion
        using notify_when_ready_handle = typename detail::shared_state_base::
            notify_when_ready_handle;

        /// \brief Helper class to store information about each of the futures
        /// we are waiting for
        ///
        /// Because the waiter can be associated with futures of different
        /// types, this class also nullifies the operations necessary to check
        /// the state of the future object.
        ///
        struct registered_waiter
        {
            /// \brief Mutex associated with a future we are watching
            std::mutex *future_mutex_;

            /// \brief Callback to disable notifications
            std::function<void(notify_when_ready_handle)>
                disable_notification_callback;

            /// \brief Callback to disable notifications
            std::function<bool()> is_ready_callback;

            /// \brief Handler to the resource that will notify us when the
            /// future is ready
            ///
            /// In the shared state, this usually represents a pointer to the
            /// condition variable
            ///
            notify_when_ready_handle handle;

            /// \brief Index to this future in the underlying range
            std::size_t index;

            /// \brief Construct a registered waiter to be enqueued in the main
            /// waiter
            template <class Future>
            registered_waiter(
                Future &a_future,
                const notify_when_ready_handle &handle_,
                std::size_t index_)
                : future_mutex_(&a_future.mutex()),
                  disable_notification_callback(
                      [future_ = &a_future](notify_when_ready_handle h)
                          -> void { future_->unnotify_when_ready(h); }),
                  is_ready_callback([future_ = &a_future]() -> bool {
                      return future_->is_ready();
                  }),
                  handle(handle_), index(index_) {}

            /// \brief Get the mutex associated with the future we are watching
            [[nodiscard]] std::mutex &
            mutex() const {
                return *future_mutex_;
            }

            /// \brief Disable notification when the future is ready
            void
            disable_notification() const {
                disable_notification_callback(handle);
            }

            /// \brief Check if underlying future is ready
            [[nodiscard]] bool
            is_ready() const {
                return is_ready_callback();
            }
        };

        /// \brief Helper class to lock all futures
        struct registered_waiter_range_lock
        {
            /// \brief Type for a vector of locks
            using lock_vector = std::vector<std::unique_lock<std::mutex>>;

            /// \brief Type for a shared vector of locks
            using shared_lock_vector = std::shared_ptr<lock_vector>;

            /// \brief Number of futures locked
            std::size_t count{ 0 };

            /// \brief Locks for each future in the range
            shared_lock_vector locks;

            /// \brief Create a lock for each future in the specified vector of
            /// registered waiters
            template <typename WaiterIterator>
            explicit registered_waiter_range_lock(
                WaiterIterator first_waiter,
                WaiterIterator last_waiter)
                : count(std::distance(first_waiter, last_waiter)),
                  locks(std::make_shared<lock_vector>(count)) {
                WaiterIterator waiter_it = first_waiter;
                std::size_t lock_idx = 0;
                while (waiter_it != last_waiter) {
                    (*locks)[lock_idx] = (std::unique_lock<std::mutex>(
                        waiter_it->mutex()));
                    ++waiter_it;
                    ++lock_idx;
                }
            }

            template <typename Range>
            explicit registered_waiter_range_lock(Range &&r)
                : registered_waiter_range_lock(
                    std::forward<Range>(r).begin(),
                    std::forward<Range>(r).end()) {}


            /// \brief Lock all future mutexes in the range
            void
            lock() const {
                futures::detail::lock(locks->begin(), locks->end());
            }

            /// \brief Unlock all future mutexes in the range
            void
            unlock() const {
                for (size_t i = 0; i < count; ++i) {
                    (*locks)[i].unlock();
                }
            }
        };

        /// \brief Condition variable to warn about any ready future
        std::condition_variable_any cv;

        /// \brief Waiters with information about each future and notification
        /// handlers
        std::vector<registered_waiter> waiters_;

        /// \brief Number of futures in this range
        std::size_t future_count{ 0 };

        /// \brief Poller futures
        ///
        /// Futures that support notifications wrapping future types that don't
        ///
        small_vector<cfuture<void>> poller_futures_{};
    };

    /** @} */
} // namespace futures::detail

#endif // FUTURES_WAITER_FOR_ANY_H

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
    /// This function waits for any future in the range [`first`, `last`) to be
    /// ready.
    ///
    /// Unlike @ref wait_for_all, this function requires special data structures
    /// to allow that to happen without blocking.
    ///
    /// \note This function is adapted from `boost::wait_for_any`
    ///
    /// \see
    /// [boost.thread
    /// wait_for_any](https://www.boost.org/doc/libs/1_78_0/doc/html/thread/synchronization.html#thread.synchronization.futures.reference.wait_for_any)
    ///
    /// \tparam Iterator Iterator type in a range of futures
    /// \param first Iterator to the first element in the range
    /// \param last Iterator to one past the last element in the range
    /// \return Iterator to the first future that got ready
    template <
        typename Iterator
#ifndef FUTURES_DOXYGEN
        ,
        typename std::
            enable_if_t<is_future_v<iter_value_t<Iterator>>, int> = 0
#endif
        >
    Iterator
    wait_for_any(Iterator first, Iterator last) {
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
    /// This function requires special data structures to allow that to happen
    /// without blocking.
    ///
    /// \note This function is adapted from `boost::wait_for_any`
    ///
    /// \see
    /// [boost.thread
    /// wait_for_any](https://www.boost.org/doc/libs/1_78_0/doc/html/thread/synchronization.html#thread.synchronization.futures.reference.wait_for_any)
    ///
    /// \tparam Iterator A range of futures type
    /// \param r Range of futures
    /// \return Iterator to the first future that got ready
    template <
        typename Range
#ifndef FUTURES_DOXYGEN
        ,
        typename std::enable_if_t<
            is_range_v<Range> && is_future_v<range_value_t<Range>>,
            int> = 0
#endif
        >
    iterator_t<Range>
    wait_for_any(Range &&r) {
        return wait_for_any(std::begin(r), std::end(r));
    }

    /// \brief Wait for any future in a sequence to be ready
    ///
    /// This function waits for all specified futures `fs`... to be ready.
    ///
    /// \note This function is adapted from `boost::wait_for_any`
    ///
    /// \see
    /// [boost.thread
    /// wait_for_any](https://www.boost.org/doc/libs/1_78_0/doc/html/thread/synchronization.html#thread.synchronization.futures.reference.wait_for_any)
    ///
    /// \tparam Fs A list of future types
    /// \param fs A list of future objects
    /// \return Index of the first future that got ready
    template <
        typename... Fs
#ifndef FUTURES_DOXYGEN
        ,
        typename std::enable_if_t<
            std::conjunction_v<is_future<std::decay_t<Fs>>...>,
            int> = 0
#endif
        >
    std::size_t
    wait_for_any(Fs &&...fs) {
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


// Adaptors
// #include <futures/adaptor/ready_future.h>
#ifndef FUTURES_READY_FUTURE_H
#define FUTURES_READY_FUTURE_H

// #include <futures/futures/basic_future.h>

// #include <futures/futures/promise.h>

// #include <futures/futures/traits/future_return.h>
#ifndef FUTURES_FUTURE_RETURN_H
#define FUTURES_FUTURE_RETURN_H

// #include <futures/adaptor/detail/traits/is_reference_wrapper.h>
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
    template <typename>
    struct is_reference_wrapper : std::false_type
    {};

    template <class T>
    struct is_reference_wrapper<std::reference_wrapper<T>> : std::true_type
    {};

    template <class T>
    constexpr bool is_reference_wrapper_v = is_reference_wrapper<T>::value;
    /** @} */ // \addtogroup future-traits Future Traits
    /** @} */ // \addtogroup futures Futures
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
    using future_return = std::conditional<
        is_reference_wrapper_v<std::decay_t<T>>,
        T &,
        std::decay_t<T>>;

    /// Determine the type to be stored and returned by a future object
    template <class T>
    using future_return_t = typename future_return<T>::type;

    /** @} */ // \addtogroup future-traits Future Traits
    /** @} */ // \addtogroup futures Futures
} // namespace futures

#endif // FUTURES_FUTURE_RETURN_H

// #include <futures/futures/traits/is_future.h>

// #include <futures/futures/detail/traits/has_is_ready.h>
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
    /// This is what we use to identify the return type of a future type
    /// candidate However, this doesn't mean the type is a future in the terms
    /// of the is_future concept
    template <typename T, typename = void>
    struct has_is_ready : std::false_type
    {};

    template <typename T>
    struct has_is_ready<T, std::void_t<decltype(std::declval<T>().is_ready())>>
        : std::is_same<bool, decltype(std::declval<T>().is_ready())>
    {};

    template <typename T>
    constexpr bool has_is_ready_v = has_is_ready<T>::value;

    /** @} */
    /** @} */
} // namespace futures::detail

#endif // FUTURES_HAS_IS_READY_H

// #include <future>


namespace futures {
    /** \addtogroup adaptors Adaptors
     *  @{
     */

    /// \brief Check if a future is ready
    ///
    /// Although basic_future has its more efficient is_ready function, this
    /// free function allows us to query other futures that don't implement
    /// is_ready, such as std::future.
    template <
        typename Future
#ifndef FUTURES_DOXYGEN
        ,
        std::enable_if_t<is_future_v<std::decay_t<Future>>, int> = 0
#endif
        >
    bool
    is_ready(Future &&f) {
        assert(
            f.valid()
            && "Undefined behaviour. Checking if an invalid future is ready.");
        if constexpr (detail::has_is_ready_v<Future>) {
            return f.is_ready();
        } else {
            return f.wait_for(std::chrono::seconds(0))
                   == std::future_status::ready;
        }
    }

    /// \brief Make a placeholder future object that is ready
    ///
    /// \see https://en.cppreference.com/w/cpp/experimental/make_ready_future
    ///
    /// \return A future associated with the shared state that is created.
    template <typename T, typename Future = future<typename std::decay_t<T>>>
    Future
    make_ready_future(T &&value) {
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
    template <typename T, typename Future = future<T &>>
    Future
    make_ready_future(std::reference_wrapper<T> value) {
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
    template <typename Future = future<void>>
    Future
    make_ready_future() {
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
    template <typename T>
    cfuture<typename std::decay<T>>
    make_ready_cfuture(T &&value) {
        return make_ready_future<T, cfuture<typename std::decay<T>>>(
            std::forward<T>(value));
    }

    /// \brief Make a placeholder @ref cfuture object that is ready
    template <typename T>
    cfuture<T &>
    make_ready_cfuture(std::reference_wrapper<T> value) {
        return make_ready_future<T, cfuture<T &>>(value);
    }

    /// \brief Make a placeholder void @ref cfuture object that is ready
    ///
    /// \see https://en.cppreference.com/w/cpp/experimental/make_ready_future
    ///
    /// \return A cfuture associated with the shared state that is created.
    inline cfuture<void>
    make_ready_cfuture() {
        return make_ready_future<cfuture<void>>();
    }

    /// \brief Make a placeholder @ref jcfuture object that is ready
    ///
    /// \see https://en.cppreference.com/w/cpp/experimental/make_ready_future
    ///
    /// \return A cfuture associated with the shared state that is created.
    template <typename T>
    jcfuture<typename std::decay<T>>
    make_ready_jcfuture(T &&value) {
        return make_ready_future<T, jcfuture<typename std::decay<T>>>(
            std::forward<T>(value));
    }

    /// \brief Make a placeholder @ref cfuture object that is ready
    template <typename T>
    jcfuture<T &>
    make_ready_jcfuture(std::reference_wrapper<T> value) {
        return make_ready_future<T, jcfuture<T &>>(value);
    }

    /// \brief Make a placeholder void @ref jcfuture object that is ready
    ///
    /// \see https://en.cppreference.com/w/cpp/experimental/make_ready_future
    ///
    /// \return A cfuture associated with the shared state that is created.
    inline jcfuture<void>
    make_ready_jcfuture() {
        return make_ready_future<jcfuture<void>>();
    }

    /// \brief Make a placeholder future object that is ready with an exception
    /// from an exception ptr
    ///
    /// \see
    /// https://en.cppreference.com/w/cpp/experimental/make_exceptional_future
    ///
    /// \return A future associated with the shared state that is created.
    template <typename T, typename Future = future<T>>
    future<T>
    make_exceptional_future(std::exception_ptr ex) {
        promise<T> p;
        p.set_exception(ex);
        return p.template get_future<Future>();
    }

    /// \brief Make a placeholder future object that is ready with from any
    /// exception
    ///
    /// \see
    /// https://en.cppreference.com/w/cpp/experimental/make_exceptional_future
    ///
    /// \return A future associated with the shared state that is created.
    template <class T, typename Future = future<T>, class E>
    future<T>
    make_exceptional_future(E ex) {
        promise<T> p;
        p.set_exception(std::make_exception_ptr(ex));
        return p.template get_future<Future>();
    }
    /** @} */
} // namespace futures

#endif // FUTURES_READY_FUTURE_H

// #include <futures/adaptor/then.h>
#ifndef FUTURES_THEN_H
#define FUTURES_THEN_H

// #include <futures/adaptor/detail/continuation_unwrap.h>
#ifndef FUTURES_CONTINUATION_UNWRAP_H
#define FUTURES_CONTINUATION_UNWRAP_H

// #include <futures/algorithm/traits/is_range.h>

// #include <futures/algorithm/traits/range_value.h>

// #include <futures/futures/basic_future.h>

// #include <futures/futures/traits/is_future.h>

// #include <futures/futures/traits/unwrap_future.h>
#ifndef FUTURES_UNWRAP_FUTURE_H
#define FUTURES_UNWRAP_FUTURE_H

// #include <futures/futures/traits/is_future.h>

// #include <futures/adaptor/detail/traits/has_get.h>
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
        /// This is what we use to identify the return type of a future type
        /// candidate However, this doesn't mean the type is a future in the
        /// terms of the is_future concept
        template <typename T, typename = void>
        struct has_get : std::false_type
        {};

        template <typename T>
        struct has_get<T, std::void_t<decltype(std::declval<T>().get())>>
            : std::true_type
        {};
    }         // namespace detail
    /** @} */ // \addtogroup future-traits Future Traits
    /** @} */ // \addtogroup futures Futures
} // namespace futures

#endif // FUTURES_HAS_GET_H


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
    template <typename T, class Enable = void>
    struct unwrap_future
    {
        using type = void;
    };

    /// \brief Determine type a future object holds (specialization for types
    /// that implement `get()`)
    ///
    /// Template for types that implement ::get()
    ///
    /// \note Not to be confused with continuation unwrapping
    template <typename Future>
    struct unwrap_future<
        Future,
        std::enable_if_t<detail::has_get<std::decay_t<Future>>::value>>
    {
        using type = std::decay_t<
            decltype(std::declval<std::decay_t<Future>>().get())>;
    };

    /// \brief Determine type a future object holds
    ///
    /// \note Not to be confused with continuation unwrapping
    template <class T>
    using unwrap_future_t = typename unwrap_future<T>::type;

    /** @} */ // \addtogroup future-traits Future Traits
    /** @} */ // \addtogroup futures Futures
} // namespace futures

#endif // FUTURES_UNWRAP_FUTURE_H

// #include <futures/adaptor/detail/move_or_copy.h>
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
    /// Create another future with the state of the before future (usually for a
    /// continuation function). This state should be copied to the new callback
    /// function. Shared futures can be copied. Normal futures should be moved.
    /// \return The moved future or the shared future
    template <class Future>
    constexpr decltype(auto)
    move_or_copy(Future &&before) {
        if constexpr (is_shared_future_v<Future>) {
            return std::forward<Future>(before);
        } else {
            return std::move(std::forward<Future>(before));
        }
    }

    /** @} */ // \addtogroup future-traits Future Traits
    /** @} */ // \addtogroup futures Futures
} // namespace futures::detail
#endif // FUTURES_MOVE_OR_COPY_H

// #include <futures/adaptor/detail/traits/is_callable.h>
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
    template <typename T>
    struct is_callable
    {
    private:
        typedef char (&yes)[1];
        typedef char (&no)[2];

        struct Fallback
        {
            void
            operator()();
        };
        struct Derived
            : T
            , Fallback
        {};

        template <typename U, U>
        struct Check;

        template <typename>
        static yes
        test(...);

        template <typename C>
        static no
        test(Check<void (Fallback::*)(), &C::operator()> *);

    public:
        static const bool value = sizeof(test<Derived>(0)) == sizeof(yes);
    };

    template <typename T>
    constexpr bool is_callable_v = is_callable<T>::value;

    /** @} */ // \addtogroup future-traits Future Traits
    /** @} */ // \addtogroup futures Futures
} // namespace futures

#endif // FUTURES_IS_CALLABLE_H

// #include <futures/adaptor/detail/traits/is_single_type_tuple.h>
#ifndef FUTURES_TUPLE_TYPE_IS_SINGLE_TYPE_TUPLE_H
#define FUTURES_TUPLE_TYPE_IS_SINGLE_TYPE_TUPLE_H

// #include <futures/adaptor/detail/traits/is_tuple.h>
#ifndef FUTURES_IS_TUPLE_H
#define FUTURES_IS_TUPLE_H

#include <tuple>
// #include <type_traits>


namespace futures {
    /** \addtogroup futures Futures
     *  @{
     */
    /** \addtogroup future-traits Future Traits
     *  @{
     */

    /// Check if type is a tuple
    template <typename>
    struct is_tuple : std::false_type
    {};

    template <typename... Args>
    struct is_tuple<std::tuple<Args...>> : std::true_type
    {};
    template <typename... Args>
    struct is_tuple<const std::tuple<Args...>> : std::true_type
    {};
    template <typename... Args>
    struct is_tuple<std::tuple<Args...> &> : std::true_type
    {};
    template <typename... Args>
    struct is_tuple<std::tuple<Args...> &&> : std::true_type
    {};
    template <typename... Args>
    struct is_tuple<const std::tuple<Args...> &> : std::true_type
    {};

    template <class T>
    constexpr bool is_tuple_v = is_tuple<T>::value;
    /** @} */ // \addtogroup future-traits Future Traits
    /** @} */ // \addtogroup futures Futures
} // namespace futures

#endif // FUTURES_IS_TUPLE_H

// #include <tuple>

// #include <type_traits>


namespace futures::detail {
    /// \brief Check if all types in a tuple match a predicate
    template <class L>
    struct is_single_type_tuple : is_tuple<L>
    {};

    template <class T1>
    struct is_single_type_tuple<std::tuple<T1>> : std::true_type
    {};

    template <class T1, class T2>
    struct is_single_type_tuple<std::tuple<T1, T2>> : std::is_same<T1, T2>
    {};

    template <class T1, class T2, class... Tn>
    struct is_single_type_tuple<std::tuple<T1, T2, Tn...>>
        : std::bool_constant<
              std::is_same_v<
                  T1,
                  T2> && is_single_type_tuple<std::tuple<T2, Tn...>>::value>
    {};

    template <class L>
    constexpr bool is_single_type_tuple_v = is_single_type_tuple<L>::value;

} // namespace futures::detail

#endif // FUTURES_TUPLE_TYPE_IS_SINGLE_TYPE_TUPLE_H

// #include <futures/adaptor/detail/traits/is_tuple.h>

// #include <futures/adaptor/detail/traits/is_tuple_invocable.h>
#ifndef FUTURES_IS_TUPLE_INVOCABLE_H
#define FUTURES_IS_TUPLE_INVOCABLE_H

// #include <tuple>

// #include <type_traits>


namespace futures::detail {
    /** \addtogroup futures Futures
     *  @{
     */

    /// \brief Check if a function can be invoked with the elements of a tuple
    /// as arguments, as in std::apply
    template <typename Function, typename Tuple>
    struct is_tuple_invocable : std::false_type
    {};

    template <typename Function, class... Args>
    struct is_tuple_invocable<Function, std::tuple<Args...>>
        : std::is_invocable<Function, Args...>
    {};

    template <typename Function, typename Tuple>
    constexpr bool is_tuple_invocable_v = is_tuple_invocable<Function, Tuple>::
        value;

    /** @} */
} // namespace futures::detail

#endif // FUTURES_IS_TUPLE_INVOCABLE_H

// #include <futures/adaptor/detail/traits/is_when_any_result.h>
#ifndef FUTURES_IS_WHEN_ANY_RESULT_H
#define FUTURES_IS_WHEN_ANY_RESULT_H

// #include <futures/adaptor/when_any_result.h>
#ifndef FUTURES_WHEN_ANY_RESULT_H
#define FUTURES_WHEN_ANY_RESULT_H

namespace futures {
    /** \addtogroup adaptors Adaptors
     *  @{
     */

    /// \brief Result type for when_any_future objects
    ///
    /// This is defined in a separate file because many other concepts depend on
    /// this definition, especially the inferences for unwrapping `then`
    /// continuations, regardless of the when_any algorithm.
    template <typename Sequence>
    struct when_any_result
    {
        using size_type = std::size_t;
        using sequence_type = Sequence;

        size_type index{ static_cast<size_type>(-1) };
        sequence_type tasks;
    };


    /** @} */
} // namespace futures

#endif // FUTURES_WHEN_ANY_RESULT_H

// #include <type_traits>


namespace futures::detail {
    /** \addtogroup futures Futures
     *  @{
     */

    /// Check if type is a when_any_result
    template <typename>
    struct is_when_any_result : std::false_type
    {};
    template <typename Sequence>
    struct is_when_any_result<when_any_result<Sequence>> : std::true_type
    {};
    template <typename Sequence>
    struct is_when_any_result<const when_any_result<Sequence>> : std::true_type
    {};
    template <typename Sequence>
    struct is_when_any_result<when_any_result<Sequence> &> : std::true_type
    {};
    template <typename Sequence>
    struct is_when_any_result<when_any_result<Sequence> &&> : std::true_type
    {};
    template <typename Sequence>
    struct is_when_any_result<const when_any_result<Sequence> &>
        : std::true_type
    {};
    template <class T>
    constexpr bool is_when_any_result_v = is_when_any_result<T>::value;

    /** @} */
} // namespace futures::detail


#endif // FUTURES_IS_WHEN_ANY_RESULT_H

// #include <futures/adaptor/detail/traits/tuple_type_all_of.h>
#ifndef FUTURES_TUPLE_TYPE_ALL_OF_H
#define FUTURES_TUPLE_TYPE_ALL_OF_H

// #include <futures/adaptor/detail/traits/is_tuple.h>

// #include <tuple>

// #include <type_traits>


namespace futures::detail {
    /// \brief Check if all types in a tuple match a predicate
    template <class T, template <class...> class P>
    struct tuple_type_all_of : is_tuple<T>
    {};

    template <class T1, template <class...> class P>
    struct tuple_type_all_of<std::tuple<T1>, P> : P<T1>
    {};

    template <class T1, class... Tn, template <class...> class P>
    struct tuple_type_all_of<std::tuple<T1, Tn...>, P>
        : std::bool_constant<
              P<T1>::value && tuple_type_all_of<std::tuple<Tn...>, P>::value>
    {};

    template <class L, template <class...> class P>
    constexpr bool tuple_type_all_of_v = tuple_type_all_of<L, P>::value;

} // namespace futures::detail

#endif // FUTURES_TUPLE_TYPE_ALL_OF_H

// #include <futures/adaptor/detail/traits/tuple_type_concat.h>
#ifndef FUTURES_TUPLE_TYPE_CONCAT_H
#define FUTURES_TUPLE_TYPE_CONCAT_H

// #include <tuple>


namespace futures::detail {
    /// \brief Concatenate type lists
    /// The detail functions related to type lists assume we use std::tuple for
    /// all type lists
    template <class...>
    struct tuple_type_concat
    {
        using type = std::tuple<>;
    };

    template <class T1>
    struct tuple_type_concat<T1>
    {
        using type = T1;
    };

    template <class... First, class... Second>
    struct tuple_type_concat<std::tuple<First...>, std::tuple<Second...>>
    {
        using type = std::tuple<First..., Second...>;
    };

    template <class T1, class... Tn>
    struct tuple_type_concat<T1, Tn...>
    {
        using type = typename tuple_type_concat<
            T1,
            typename tuple_type_concat<Tn...>::type>::type;
    };

    template <class... Tn>
    using tuple_type_concat_t = typename tuple_type_concat<Tn...>::type;
} // namespace futures::detail

#endif // FUTURES_TUPLE_TYPE_CONCAT_H

// #include <futures/adaptor/detail/traits/tuple_type_transform.h>
#ifndef FUTURES_TUPLE_TYPE_TRANSFORM_H
#define FUTURES_TUPLE_TYPE_TRANSFORM_H

// #include <tuple>

// #include <type_traits>


namespace futures::detail {
    /// \brief Transform all types in a tuple
    template <class L, template <class...> class P>
    struct tuple_type_transform
    {
        using type = std::tuple<>;
    };

    template <class... Tn, template <class...> class P>
    struct tuple_type_transform<std::tuple<Tn...>, P>
    {
        using type = std::tuple<typename P<Tn>::type...>;
    };

    template <class L, template <class...> class P>
    using tuple_type_transform_t = typename tuple_type_transform<L, P>::type;

} // namespace futures::detail

#endif // FUTURES_TUPLE_TYPE_TRANSFORM_H

// #include <futures/adaptor/detail/traits/type_member_or.h>
#ifndef FUTURES_TYPE_MEMBER_OR_H
#define FUTURES_TYPE_MEMBER_OR_H

// #include <type_traits>


namespace futures::detail {
    /** \addtogroup futures Futures
     *  @{
     */

    /// \brief Return T::type or a second type as a placeholder if T::type
    /// doesn't exist This class is meant to avoid errors in std::conditional
    template <class, class Placeholder = void, class = void>
    struct type_member_or
    {
        using type = Placeholder;
    };

    template <class T, class Placeholder>
    struct type_member_or<T, Placeholder, std::void_t<typename T::type>>
    {
        using type = typename T::type;
    };

    template <class T, class Placeholder>
    using type_member_or_t = typename type_member_or<T>::type;

    /** @} */
} // namespace futures::detail

#endif // FUTURES_TYPE_MEMBER_OR_H

// #include <futures/adaptor/detail/tuple_algorithm.h>
#ifndef FUTURES_TUPLE_ALGORITHM_H
#define FUTURES_TUPLE_ALGORITHM_H

// #include <futures/futures/detail/throw_exception.h>

// #include <tuple>


namespace futures {
    /** \addtogroup futures Futures
     *  @{
     */

    namespace detail {
        template <class Function, class... Args, std::size_t... Is>
        static void
        for_each_impl(
            const std::tuple<Args...> &t,
            Function &&fn,
            std::index_sequence<Is...>) {
            (fn(std::get<Is>(t)), ...);
        }
    } // namespace detail

    /// \brief tuple_for_each for tuples
    template <class Function, class... Args>
    static void
    tuple_for_each(const std::tuple<Args...> &t, Function &&fn) {
        detail::for_each_impl(
            t,
            std::forward<Function>(fn),
            std::index_sequence_for<Args...>{});
    }

    namespace detail {
        template <
            class Function,
            class... Args1,
            class... Args2,
            std::size_t... Is>
        static void
        for_each_paired_impl(
            std::tuple<Args1...> &t1,
            std::tuple<Args2...> &t2,
            Function &&fn,
            std::index_sequence<Is...>) {
            (fn(std::get<Is>(t1), std::get<Is>(t2)), ...);
        }
    } // namespace detail

    /// \brief for_each_paired for paired tuples of same size
    template <class Function, class... Args1, class... Args2>
    static void
    for_each_paired(
        std::tuple<Args1...> &t1,
        std::tuple<Args2...> &t2,
        Function &&fn) {
        static_assert(
            std::tuple_size_v<std::tuple<
                Args1...>> == std::tuple_size_v<std::tuple<Args2...>>);
        detail::for_each_paired_impl(
            t1,
            t2,
            std::forward<Function>(fn),
            std::index_sequence_for<Args1...>{});
    }

    namespace detail {
        template <
            class Function,
            class... Args1,
            class T,
            size_t N,
            std::size_t... Is>
        static void
        for_each_paired_impl(
            std::tuple<Args1...> &t,
            std::array<T, N> &a,
            Function &&fn,
            std::index_sequence<Is...>) {
            (fn(std::get<Is>(t), a[Is]), ...);
        }
    } // namespace detail

    /// \brief for_each_paired for paired tuples and arrays of same size
    template <class Function, class... Args1, class T, size_t N>
    static void
    for_each_paired(
        std::tuple<Args1...> &t,
        std::array<T, N> &a,
        Function &&fn) {
        static_assert(std::tuple_size_v<std::tuple<Args1...>> == N);
        detail::for_each_paired_impl(
            t,
            a,
            std::forward<Function>(fn),
            std::index_sequence_for<Args1...>{});
    }

    /// \brief find_if for tuples
    template <class Function, size_t t_idx = 0, class... Args>
    static size_t
    tuple_find_if(const std::tuple<Args...> &t, Function &&fn) {
        if constexpr (t_idx == std::tuple_size_v<std::decay_t<decltype(t)>>) {
            return t_idx;
        } else {
            if (fn(std::get<t_idx>(t))) {
                return t_idx;
            }
            return tuple_find_if<Function, t_idx + 1, Args...>(
                t,
                std::forward<Function>(fn));
        }
    }

    namespace detail {
        template <class Function, class... Args, std::size_t... Is>
        static bool
        all_of_impl(
            const std::tuple<Args...> &t,
            Function &&fn,
            std::index_sequence<Is...>) {
            return (fn(std::get<Is>(t)) && ...);
        }
    } // namespace detail

    /// \brief all_of for tuples
    template <class Function, class... Args>
    static bool
    tuple_all_of(const std::tuple<Args...> &t, Function &&fn) {
        return detail::all_of_impl(
            t,
            std::forward<Function>(fn),
            std::index_sequence_for<Args...>{});
    }

    namespace detail {
        template <class Function, class... Args, std::size_t... Is>
        static bool
        any_of_impl(
            const std::tuple<Args...> &t,
            Function &&fn,
            std::index_sequence<Is...>) {
            return (fn(std::get<Is>(t)) || ...);
        }
    } // namespace detail

    /// \brief any_of for tuples
    template <class Function, class... Args>
    static bool
    tuple_any_of(const std::tuple<Args...> &t, Function &&fn) {
        return detail::any_of_impl(
            t,
            std::forward<Function>(fn),
            std::index_sequence_for<Args...>{});
    }

    /// \brief Apply a function to a single tuple element at runtime
    /// The function must, of course, be valid for all tuple elements
    template <
        class Function,
        class Tuple,
        size_t current_tuple_idx = 0,
        std::enable_if_t<
            is_callable_v<
                Function> && is_tuple_v<Tuple> && (current_tuple_idx < std::tuple_size_v<std::decay_t<Tuple>>),
            int> = 0>

    constexpr static auto
    apply(Function &&fn, Tuple &&t, std::size_t idx) {
        assert(idx < std::tuple_size_v<std::decay_t<Tuple>>);
        if (current_tuple_idx == idx) {
            return fn(std::get<current_tuple_idx>(t));
        } else if constexpr (
            current_tuple_idx + 1 < std::tuple_size_v<std::decay_t<Tuple>>)
        {
            return apply<Function, Tuple, current_tuple_idx + 1>(
                std::forward<Function>(fn),
                std::forward<Tuple>(t),
                idx);
        } else {
            detail::throw_exception<std::out_of_range>(
                "apply:: tuple idx out of range");
        }
    }

    /// \brief Return the i-th element from a tuple whose types are the same
    /// The return expression function must, of course, be valid for all tuple
    /// elements
    template <
        class Tuple,
        size_t current_tuple_idx = 0,
        std::enable_if_t<
            is_tuple_v<
                Tuple> && (current_tuple_idx < std::tuple_size_v<std::decay_t<Tuple>>),
            int> = 0>

    constexpr static decltype(auto)
    get(Tuple &&t, std::size_t idx) {
        assert(idx < std::tuple_size_v<std::decay_t<Tuple>>);
        if (current_tuple_idx == idx) {
            return std::get<current_tuple_idx>(t);
        } else if constexpr (
            current_tuple_idx + 1 < std::tuple_size_v<std::decay_t<Tuple>>)
        {
            return get<Tuple, current_tuple_idx + 1>(
                std::forward<Tuple>(t),
                idx);
        } else {
            detail::throw_exception<std::out_of_range>(
                "get:: tuple idx out of range");
        }
    }

    /// \brief Return the i-th element from a tuple with a transformation
    /// function whose return is always the same The return expression function
    /// must, of course, be valid for all tuple elements
    template <
        class Tuple,
        size_t current_tuple_idx = 0,
        class TransformFn,
        std::enable_if_t<
            is_tuple_v<
                Tuple> && (current_tuple_idx < std::tuple_size_v<std::decay_t<Tuple>>),
            int> = 0>

    constexpr static decltype(auto)
    get(Tuple &&t, std::size_t idx, TransformFn &&transform) {
        assert(idx < std::tuple_size_v<std::decay_t<Tuple>>);
        if (current_tuple_idx == idx) {
            return transform(std::get<current_tuple_idx>(t));
        } else if constexpr (
            current_tuple_idx + 1 < std::tuple_size_v<std::decay_t<Tuple>>)
        {
            return get<
                Tuple,
                current_tuple_idx + 1>(std::forward<Tuple>(t), idx, transform);
        } else {
            detail::throw_exception<std::out_of_range>(
                "get:: tuple idx out of range");
        }
    }

    namespace detail {
        template <class F, class FT, class Tuple, std::size_t... I>
        constexpr decltype(auto)
        transform_and_apply_impl(
            F &&f,
            FT &&ft,
            Tuple &&t,
            std::index_sequence<I...>) {
            return std::invoke(
                std::forward<F>(f),
                ft(std::get<I>(std::forward<Tuple>(t)))...);
        }
    } // namespace detail

    template <class F, class FT, class Tuple>
    constexpr decltype(auto)
    transform_and_apply(F &&f, FT &&ft, Tuple &&t) {
        return detail::transform_and_apply_impl(
            std::forward<F>(f),
            std::forward<FT>(ft),
            std::forward<Tuple>(t),
            std::make_index_sequence<
                std::tuple_size_v<std::remove_reference_t<Tuple>>>{});
    }

    namespace detail {
        /// The tuple type after we filtered it with a template template
        /// predicate
        template <template <typename> typename UnaryPredicate, typename Tuple>
        struct filtered_tuple_type;

        /// The tuple type after we filtered it with a template template
        /// predicate
        template <template <typename> typename UnaryPredicate, typename... Ts>
        struct filtered_tuple_type<UnaryPredicate, std::tuple<Ts...>>
        {
            /// If this element has to be kept, returns `std::tuple<Ts>`
            /// Otherwise returns `std::tuple<>`
            template <class E>
            using t_filtered_tuple_type_impl = std::conditional_t<
                UnaryPredicate<E>::value,
                std::tuple<E>,
                std::tuple<>>;

            /// Determines the type that would be returned by `std::tuple_cat`
            ///  if it were called with instances of the types reported by
            ///  t_filtered_tuple_type_impl for each element
            using type = decltype(std::tuple_cat(
                std::declval<t_filtered_tuple_type_impl<Ts>>()...));
        };

        /// The tuple type after we filtered it with a template template
        /// predicate
        template <template <typename> typename UnaryPredicate, typename Tuple>
        struct transformed_tuple;

        /// The tuple type after we filtered it with a template template
        /// predicate
        template <template <typename> typename UnaryPredicate, typename... Ts>
        struct transformed_tuple<UnaryPredicate, std::tuple<Ts...>>
        {
            /// If this element has to be kept, returns `std::tuple<Ts>`
            /// Otherwise returns `std::tuple<>`
            template <class E>
            using transformed_tuple_element_type = typename UnaryPredicate<
                E>::type;

            /// Determines the type that would be returned by `std::tuple_cat`
            ///  if it were called with instances of the types reported by
            ///  transformed_tuple_element_type for each element
            using type = decltype(std::tuple_cat(
                std::declval<transformed_tuple_element_type<Ts>>()...));
        };
    } // namespace detail

    /// \brief Filter tuple elements based on their types
    template <template <typename> typename UnaryPredicate, typename... Ts>
    constexpr typename detail::
        filtered_tuple_type<UnaryPredicate, std::tuple<Ts...>>::type
        filter_if(const std::tuple<Ts...> &tup) {
        return std::apply(
            [](auto... tuple_value) {
            return std::tuple_cat(
                std::conditional_t<
                    UnaryPredicate<decltype(tuple_value)>::value,
                    std::tuple<decltype(tuple_value)>,
                    std::tuple<>>{}...);
            },
            tup);
    }

    /// \brief Remove tuple elements based on their types
    template <template <typename> typename UnaryPredicate, typename... Ts>
    constexpr typename detail::
        filtered_tuple_type<UnaryPredicate, std::tuple<Ts...>>::type
        remove_if(const std::tuple<Ts...> &tup) {
        return std::apply(
            [](auto... tuple_value) {
            return std::tuple_cat(
                std::conditional_t<
                    not UnaryPredicate<decltype(tuple_value)>::value,
                    std::tuple<decltype(tuple_value)>,
                    std::tuple<>>{}...);
            },
            tup);
    }

    /// \brief Transform tuple elements based on their types
    template <template <typename> typename UnaryPredicate, typename... Ts>
    constexpr typename detail::
        transformed_tuple<UnaryPredicate, std::tuple<Ts...>>::type
        transform(const std::tuple<Ts...> &tup) {
        return std::apply(
            [](auto... tuple_value) {
            return std::tuple_cat(
                std::tuple<typename UnaryPredicate<decltype(tuple_value)>::type>{
                    tuple_value }...);
            },
            tup);
    }

    /** @} */ // \addtogroup futures Futures
} // namespace futures

#endif // FUTURES_TUPLE_ALGORITHM_H

// #include <futures/futures/detail/small_vector.h>

// #include <futures/futures/detail/traits/type_member_or_void.h>


namespace futures::detail {
    /** \addtogroup futures Futures
     *  @{
     */

    struct unwrapping_failure_t
    {};

    /// \brief Get the element type of a when any result object
    /// This is a very specific helper trait we need
    template <typename T, class Enable = void>
    struct range_or_tuple_element_type
    {};

    template <typename Sequence>
    struct range_or_tuple_element_type<
        Sequence,
        std::enable_if_t<is_range_v<Sequence>>>
    {
        using type = range_value_t<Sequence>;
    };

    template <typename Sequence>
    struct range_or_tuple_element_type<
        Sequence,
        std::enable_if_t<is_tuple_v<Sequence>>>
    {
        using type = std::tuple_element_t<0, Sequence>;
    };

    template <class T>
    using range_or_tuple_element_type_t = typename range_or_tuple_element_type<
        T>::type;

    /// \brief Unwrap the results from `before` future object and give them to
    /// `continuation`
    ///
    /// This function unfortunately has very high cyclomatic complexity because
    /// it's the only way we can concatenate so many `if constexpr` without
    /// negating all previous conditions.
    ///
    /// \param before_future The antecedent future to be unwrapped
    /// \param continuation The continuation function
    /// \param args Arguments we send to the function before the unwrapped
    /// result (stop_token or <empty>) \return The continuation result
    template <
        class Future,
        typename Function,
        typename... PrefixArgs,
        std::enable_if_t<is_future_v<std::decay_t<Future>>, int> = 0>
    decltype(auto)
    unwrap_and_continue(
        Future &&before_future,
        Function &&continuation,
        PrefixArgs &&...prefix_args) {
        // Types we might use in continuation
        using value_type = unwrap_future_t<Future>;
        using lvalue_type = std::add_lvalue_reference_t<value_type>;
        using rvalue_type = std::add_rvalue_reference_t<value_type>;

        // What kind of unwrapping is the continuation invocable with
        constexpr bool no_unwrap = std::
            is_invocable_v<Function, PrefixArgs..., Future>;
        constexpr bool no_input = std::is_invocable_v<Function, PrefixArgs...>;
        constexpr bool value_unwrap = std::
            is_invocable_v<Function, PrefixArgs..., value_type>;
        constexpr bool lvalue_unwrap = std::
            is_invocable_v<Function, PrefixArgs..., lvalue_type>;
        constexpr bool rvalue_unwrap = std::
            is_invocable_v<Function, PrefixArgs..., rvalue_type>;
        constexpr bool double_unwrap
            = is_future_v<std::decay_t<
                  value_type>> && std::is_invocable_v<Function, PrefixArgs..., unwrap_future_t<value_type>>;
        constexpr bool is_tuple = is_tuple_v<value_type>;
        constexpr bool is_range = is_range_v<value_type>;

        // 5 main unwrapping paths: (no unwrap, no input, single future,
        // when_all, when_any)
        constexpr bool direct_unwrap = value_unwrap || lvalue_unwrap
                                       || rvalue_unwrap || double_unwrap;
        constexpr bool sequence_unwrap = is_tuple || is_range_v<value_type>;
        constexpr bool when_any_unwrap = is_when_any_result_v<value_type>;

        constexpr auto fail = []() {
            // Could not unwrap, return unwrapping_failure_t to indicate we
            // couldn't unwrap the continuation The function still needs to be
            // well-formed because other templates depend on it
            detail::throw_exception<std::logic_error>(
                "Continuation unwrapping not possible");
            return unwrapping_failure_t{};
        };

        // Common continuations for basic_future
        if constexpr (no_unwrap) {
            return continuation(
                std::forward<PrefixArgs>(prefix_args)...,
                detail::move_or_copy(before_future));
        } else if constexpr (no_input) {
            before_future.get();
            return continuation(std::forward<PrefixArgs>(prefix_args)...);
        } else if constexpr (direct_unwrap) {
            value_type prev_state = before_future.get();
            if constexpr (value_unwrap) {
                return continuation(
                    std::forward<PrefixArgs>(prefix_args)...,
                    std::move(prev_state));
            } else if constexpr (lvalue_unwrap) {
                return continuation(
                    std::forward<PrefixArgs>(prefix_args)...,
                    prev_state);
            } else if constexpr (rvalue_unwrap) {
                return continuation(
                    std::forward<PrefixArgs>(prefix_args)...,
                    std::move(prev_state));
            } else if constexpr (double_unwrap) {
                return continuation(
                    std::forward<PrefixArgs>(prefix_args)...,
                    prev_state.get());
            } else {
                return fail();
            }
        } else if constexpr (sequence_unwrap || when_any_unwrap) {
            using prefix_as_tuple = std::tuple<PrefixArgs...>;
            if constexpr (sequence_unwrap && is_tuple) {
                constexpr bool tuple_explode = is_tuple_invocable_v<
                    Function,
                    tuple_type_concat_t<prefix_as_tuple, value_type>>;
                constexpr bool is_future_tuple
                    = tuple_type_all_of_v<std::decay_t<value_type>, is_future>;
                if constexpr (tuple_explode) {
                    // future<tuple<future<T1>, future<T2>, ...>> ->
                    // function(future<T1>, future<T2>, ...)
                    return std::apply(
                        continuation,
                        std::tuple_cat(
                            std::make_tuple(
                                std::forward<PrefixArgs>(prefix_args)...),
                            before_future.get()));
                } else if constexpr (is_future_tuple) {
                    // future<tuple<future<T1>, future<T2>, ...>> ->
                    // function(T1, T2, ...)
                    using unwrapped_elements
                        = tuple_type_transform_t<value_type, unwrap_future>;
                    constexpr bool tuple_explode_unwrap = is_tuple_invocable_v<
                        Function,
                        tuple_type_concat_t<prefix_as_tuple, unwrapped_elements>>;
                    if constexpr (tuple_explode_unwrap) {
                        return transform_and_apply(
                            continuation,
                            [](auto &&el) {
                            if constexpr (!is_future_v<
                                              std::decay_t<decltype(el)>>) {
                                return el;
                            } else {
                                return el.get();
                            }
                            },
                            std::tuple_cat(
                                std::make_tuple(
                                    std::forward<PrefixArgs>(prefix_args)...),
                                before_future.get()));
                    } else {
                        return fail();
                    }
                } else {
                    return fail();
                }
            } else if constexpr (sequence_unwrap && is_range) {
                // when_all vector<future<T>> ->
                // function(futures::small_vector<T>)
                using range_value_t = range_value_t<value_type>;
                constexpr bool is_range_of_futures = is_future_v<
                    std::decay_t<range_value_t>>;
                using continuation_vector = detail::small_vector<
                    unwrap_future_t<range_value_t>>;
                using lvalue_continuation_vector = std::add_lvalue_reference_t<
                    continuation_vector>;
                constexpr bool vector_unwrap
                    = is_range_of_futures
                      && (std::is_invocable_v<
                              Function,
                              PrefixArgs...,
                              continuation_vector> || std::is_invocable_v<Function, PrefixArgs..., lvalue_continuation_vector>);
                if constexpr (vector_unwrap) {
                    value_type futures_vector = before_future.get();
                    using future_vector_value_type = typename value_type::
                        value_type;
                    using unwrap_vector_value_type = unwrap_future_t<
                        future_vector_value_type>;
                    using unwrap_vector_type = detail::small_vector<
                        unwrap_vector_value_type>;
                    unwrap_vector_type continuation_values;
                    std::transform(
                        futures_vector.begin(),
                        futures_vector.end(),
                        std::back_inserter(continuation_values),
                        [](future_vector_value_type &f) { return f.get(); });
                    return continuation(
                        std::forward<PrefixArgs>(prefix_args)...,
                        continuation_values);
                } else {
                    return fail();
                }
            } else if constexpr (when_any_unwrap) {
                // Common continuations for when_any futures
                // when_any<tuple<future<T1>, future<T2>, ...>> ->
                // function(size_t, tuple<future<T1>, future<T2>, ...>)
                using when_any_index = typename value_type::size_type;
                using when_any_sequence = typename value_type::sequence_type;
                using when_any_members_as_tuple = std::
                    tuple<when_any_index, when_any_sequence>;
                constexpr bool when_any_split = is_tuple_invocable_v<
                    Function,
                    tuple_type_concat_t<
                        prefix_as_tuple,
                        when_any_members_as_tuple>>;

                // when_any<tuple<future<>,...>> -> function(size_t, future<T1>,
                // future<T2>, ...)
                constexpr bool when_any_explode = []() {
                    if constexpr (is_tuple_v<when_any_sequence>) {
                        return is_tuple_invocable_v<
                            Function,
                            tuple_type_concat_t<
                                prefix_as_tuple,
                                std::tuple<when_any_index>,
                                when_any_sequence>>;
                    } else {
                        return false;
                    }
                }();

                // when_any_result<tuple<future<T>, future<T>, ...>> ->
                // continuation(future<T>)
                constexpr bool when_any_same_type
                    = is_range_v<
                          when_any_sequence> || is_single_type_tuple_v<when_any_sequence>;
                using when_any_element_type = range_or_tuple_element_type_t<
                    when_any_sequence>;
                constexpr bool when_any_element
                    = when_any_same_type
                      && is_tuple_invocable_v<
                          Function,
                          tuple_type_concat_t<
                              prefix_as_tuple,
                              std::tuple<when_any_element_type>>>;

                // when_any_result<tuple<future<T>, future<T>, ...>> ->
                // continuation(T)
                constexpr bool when_any_unwrap_element
                    = when_any_same_type
                      && is_tuple_invocable_v<
                          Function,
                          tuple_type_concat_t<
                              prefix_as_tuple,
                              std::tuple<
                                  unwrap_future_t<when_any_element_type>>>>;

                auto w = before_future.get();
                if constexpr (when_any_split) {
                    return std::apply(
                        continuation,
                        std::make_tuple(
                            std::forward<PrefixArgs>(prefix_args)...,
                            w.index,
                            std::move(w.tasks)));
                } else if constexpr (when_any_explode) {
                    return std::apply(
                        continuation,
                        std::tuple_cat(
                            std::make_tuple(
                                std::forward<PrefixArgs>(prefix_args)...,
                                w.index),
                            std::move(w.tasks)));
                } else if constexpr (when_any_element || when_any_unwrap_element)
                {
                    constexpr auto get_nth_future = [](auto &when_any_f) {
                        if constexpr (is_tuple_v<when_any_sequence>) {
                            return std::move(futures::get(
                                std::move(when_any_f.tasks),
                                when_any_f.index));
                        } else {
                            return std::move(
                                when_any_f.tasks[when_any_f.index]);
                        }
                    };
                    auto nth_future = get_nth_future(w);
                    if constexpr (when_any_element) {
                        return continuation(
                            std::forward<PrefixArgs>(prefix_args)...,
                            std::move(nth_future));
                    } else if constexpr (when_any_unwrap_element) {
                        return continuation(
                            std::forward<PrefixArgs>(prefix_args)...,
                            std::move(nth_future.get()));
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

    /// \brief Find the result of unwrap and continue or return
    /// unwrapping_failure_t if expression is not well-formed
    template <class Future, class Function, class = void>
    struct result_of_unwrap
    {
        using type = unwrapping_failure_t;
    };

    template <class Future, class Function>
    struct result_of_unwrap<
        Future,
        Function,
        std::void_t<decltype(unwrap_and_continue(
            std::declval<Future>(),
            std::declval<Function>()))>>
    {
        using type = decltype(unwrap_and_continue(
            std::declval<Future>(),
            std::declval<Function>()));
    };

    template <class Future, class Function>
    using result_of_unwrap_t = typename result_of_unwrap<Future, Function>::type;

    /// \brief Find the result of unwrap and continue with token or return
    /// unwrapping_failure_t otherwise
    template <class Future, class Function, class = void>
    struct result_of_unwrap_with_token
    {
        using type = unwrapping_failure_t;
    };

    template <class Future, class Function>
    struct result_of_unwrap_with_token<
        Future,
        Function,
        std::void_t<decltype(unwrap_and_continue(
            std::declval<Future>(),
            std::declval<Function>(),
            std::declval<stop_token>()))>>
    {
        using type = decltype(unwrap_and_continue(
            std::declval<Future>(),
            std::declval<Function>(),
            std::declval<stop_token>()));
    };

    template <class Future, class Function>
    using result_of_unwrap_with_token_t =
        typename result_of_unwrap_with_token<Future, Function>::type;

    template <typename Function, typename Future>
    struct unwrap_traits
    {
        // The return type of unwrap and continue function
        using unwrap_result_no_token_type = result_of_unwrap_t<Future, Function>;
        using unwrap_result_with_token_type
            = result_of_unwrap_with_token_t<Future, Function>;

        // Whether the continuation expects a token
        static constexpr bool is_valid_without_stop_token = !std::is_same_v<
            unwrap_result_no_token_type,
            unwrapping_failure_t>;
        static constexpr bool is_valid_with_stop_token = !std::is_same_v<
            unwrap_result_with_token_type,
            unwrapping_failure_t>;

        // Whether the continuation is valid
        static constexpr bool is_valid = is_valid_without_stop_token
                                         || is_valid_with_stop_token;

        // The result type of unwrap and continue for the valid version, with or
        // without token
        using result_value_type = std::conditional_t<
            is_valid_with_stop_token,
            unwrap_result_with_token_type,
            unwrap_result_no_token_type>;

        // Stop token for the continuation function
        constexpr static bool continuation_expects_stop_token
            = is_valid_with_stop_token;

        // Check if the stop token should be inherited from previous future
        constexpr static bool previous_future_has_stop_token = has_stop_token_v<
            Future>;
        constexpr static bool previous_future_is_shared = is_shared_future_v<
            Future>;
        constexpr static bool inherit_stop_token
            = previous_future_has_stop_token && (!previous_future_is_shared);

        // Continuation future should have stop token
        constexpr static bool after_has_stop_token = is_valid_with_stop_token
                                                     || inherit_stop_token;

        // The result type of unwrap and continue for the valid version, with or
        // without token
        using result_future_type = std::conditional_t<
            after_has_stop_token,
            jcfuture<result_value_type>,
            cfuture<result_value_type>>;
    };

    /// \brief The result we get from the `then` function
    /// - If after function expects a stop token:
    ///   - If previous future is stoppable and not-shared: return jcfuture with
    ///   shared stop source
    ///   - Otherwise:                                      return jcfuture with
    ///   new stop source
    /// - If after function does not expect a stop token:
    ///   - If previous future is stoppable and not-shared: return jcfuture with
    ///   shared stop source
    ///   - Otherwise:                                      return cfuture with
    ///   no stop source
    template <typename Function, typename Future>
    struct result_of_then
    {
        using type = typename unwrap_traits<Function, Future>::
            result_future_type;
    };

    template <typename Function, typename Future>
    using result_of_then_t = typename result_of_then<Function, Future>::type;

    /// \brief A trait to validate whether a Function can be continuation to a
    /// future
    template <class Function, class Future>
    using is_valid_continuation = std::bool_constant<
        unwrap_traits<Function, Future>::is_valid>;

    template <class Function, class Future>
    constexpr bool is_valid_continuation_v
        = is_valid_continuation<Function, Future>::value;

    // Wrap implementation in empty struct to facilitate friends
    struct internal_then_functor
    {
        /// \brief Make an appropriate stop source for the continuation
        template <typename Function, class Future>
        static stop_source
        make_continuation_stop_source(const Future &before, const Function &) {
            using traits = unwrap_traits<Function, Future>;
            if constexpr (traits::after_has_stop_token) {
                if constexpr (
                    traits::inherit_stop_token
                    && (!traits::continuation_expects_stop_token))
                {
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
        template <class Future>
        static continuations_source
        copy_continuations_source(const Future &before) {
            constexpr bool before_is_lazy_continuable = is_lazy_continuable_v<
                Future>;
            if constexpr (before_is_lazy_continuable) {
                return before.get_continuations_source();
            } else {
                return continuations_source(nocontinuationsstate);
            }
        }

        /// \brief Create a tuple with the arguments for unwrap and continue
        template <typename Function, class Future>
        static decltype(auto)
        make_unwrap_args_tuple(
            Future &&before_future,
            Function &&continuation_function,
            stop_token st) {
            using traits = unwrap_traits<Function, Future>;
            if constexpr (!traits::is_valid_with_stop_token) {
                return std::make_tuple(
                    std::forward<Future>(before_future),
                    std::forward<Function>(continuation_function));
            } else {
                return std::make_tuple(
                    std::forward<Future>(before_future),
                    std::forward<Function>(continuation_function),
                    st);
            }
        }

        template <
            typename Executor,
            typename Function,
            class Future
#ifndef FUTURES_DOXYGEN
            ,
            std::enable_if_t<
                is_executor_v<std::decay_t<
                    Executor>> && !is_executor_v<std::decay_t<Function>> && !is_executor_v<std::decay_t<Future>> && is_future_v<std::decay_t<Future>> && is_valid_continuation_v<std::decay_t<Function>, std::decay_t<Future>>,
                int> = 0
#endif
            >
        result_of_then_t<Function, Future>
        operator()(const Executor &ex, Future &&before, Function &&after)
            const {
            using traits = unwrap_traits<Function, Future>;

            // Shared sources
            stop_source ss = make_continuation_stop_source(before, after);
            detail::continuations_source after_continuations;
            continuations_source before_cs = copy_continuations_source(before);

            // Set up shared state (packaged task contains unwrap and continue
            // instead of after)
            promise<typename traits::result_value_type> p;
            typename traits::result_future_type result{
                p.template get_future<typename traits::result_future_type>()
            };
            result.set_continuations_source(after_continuations);
            if constexpr (traits::after_has_stop_token) {
                result.set_stop_source(ss);
            }
            // Set the complete executor task, using result to fulfill the
            // promise, and running continuations
            auto fulfill_promise =
                [p = std::move(p), // task and shared state
                 before_future = detail::move_or_copy(
                     std::forward<Future>(before)), // the previous future
                 continuation = std::forward<Function>(
                     after),            // the continuation function
                 after_continuations,   // continuation source for after
                 token = ss.get_token() // maybe shared stop token
            ]() mutable {
                try {
                    if constexpr (std::is_same_v<
                                      typename traits::result_value_type,
                                      void>) {
                        if constexpr (traits::is_valid_with_stop_token) {
                            detail::unwrap_and_continue(
                                before_future,
                                continuation,
                                token);
                            p.set_value();
                        } else {
                            detail::unwrap_and_continue(
                                before_future,
                                continuation);
                            p.set_value();
                        }
                    } else {
                        if constexpr (traits::is_valid_with_stop_token) {
                            p.set_value(detail::unwrap_and_continue(
                                before_future,
                                continuation,
                                token));
                        } else {
                            p.set_value(detail::unwrap_and_continue(
                                before_future,
                                continuation));
                        }
                    }
                }
                catch (...) {
                    p.set_exception(std::current_exception());
                }
                after_continuations.request_run();
            };

            // Move function to shared location because executors require
            // handles to be copy constructable
            auto fulfill_promise_ptr = std::make_shared<
                decltype(fulfill_promise)>(std::move(fulfill_promise));
            auto copyable_handle = [fulfill_promise_ptr]() {
                (*fulfill_promise_ptr)();
            };

            // Fire-and-forget: Post a handle running the complete continuation
            // function to the executor
            if constexpr (is_lazy_continuable_v<Future>) {
                // Attach continuation to previous future.
                // - Continuation is posted when previous is done
                // - Continuation is posted immediately if previous is already
                // done
                before_cs.emplace_continuation(
                    ex,
                    [h = std::move(copyable_handle), ex]() {
                    asio::post(ex, std::move(h));
                    });
            } else {
                // We defer the task in the executor because the input doesn't
                // have lazy continuations. The executor will take care of this
                // running later, so we don't need polling.
                asio::defer(ex, std::move(copyable_handle));
            }

            return result;
        }
    };
    constexpr internal_then_functor internal_then;

    /** @} */
} // namespace futures::detail

#endif // FUTURES_CONTINUATION_UNWRAP_H

// #include <future>

#include <version>

namespace futures {
    /** \addtogroup adaptors Adaptors
     *
     * \brief Functions to create new futures from existing functions.
     *
     * This module defines functions we can use to create new futures from
     * existing futures. Future adaptors are future types of whose values are
     * dependant on the condition of other future objects.
     *
     *  @{
     */

    /// \brief Schedule a continuation function to a future
    ///
    /// This creates a continuation that gets executed when the before future is
    /// over. The continuation needs to be invocable with the return type of the
    /// previous future.
    ///
    /// This function works for all kinds of futures but behavior depends on the
    /// input:
    /// - If previous future is continuable, attach the function to the
    /// continuation list
    /// - If previous future is not continuable (such as std::future), post to
    /// execution with deferred policy In both cases, the result becomes a
    /// cfuture or jcfuture.
    ///
    /// Stop tokens are also propagated:
    /// - If after function expects a stop token:
    ///   - If previous future is stoppable and not-shared: return jcfuture with
    ///   shared stop source
    ///   - Otherwise:                                      return jcfuture with
    ///   new stop source
    /// - If after function does not expect a stop token:
    ///   - If previous future is stoppable and not-shared: return jcfuture with
    ///   shared stop source
    ///   - Otherwise:                                      return cfuture with
    ///   no stop source
    ///
    /// \return A continuation to the before future
    template <
        typename Executor,
        typename Function,
        class Future
#ifndef FUTURES_DOXYGEN
        ,
        std::enable_if_t<
            is_executor_v<std::decay_t<
                Executor>> && !is_executor_v<std::decay_t<Function>> && !is_executor_v<std::decay_t<Future>> && is_future_v<std::decay_t<Future>> && detail::is_valid_continuation_v<std::decay_t<Function>, std::decay_t<Future>>,
            int> = 0
#endif
        >
    decltype(auto)
    then(const Executor &ex, Future &&before, Function &&after) {
        return detail::internal_then(
            ex,
            std::forward<Future>(before),
            std::forward<Function>(after));
    }

    /// \brief Schedule a continuation function to a future, allow an executor
    /// as second parameter
    ///
    /// \see @ref then
    template <
        class Future,
        typename Executor,
        typename Function
#ifndef FUTURES_DOXYGEN
        ,
        std::enable_if_t<
            is_executor_v<std::decay_t<
                Executor>> && !is_executor_v<std::decay_t<Function>> && !is_executor_v<std::decay_t<Future>> && is_future_v<std::decay_t<Future>> && detail::is_valid_continuation_v<std::decay_t<Function>, std::decay_t<Future>>,
            int> = 0
#endif
        >
    decltype(auto)
    then(Future &&before, const Executor &ex, Function &&after) {
        return then(
            ex,
            std::forward<Future>(before),
            std::forward<Function>(after));
    }

    /// \brief Schedule a continuation function to a future with the default
    /// executor
    ///
    /// \return A continuation to the before future
    ///
    /// \see @ref then
    template <
        class Future,
        typename Function
#ifndef FUTURES_DOXYGEN
        ,
        std::enable_if_t<
            !is_executor_v<std::decay_t<
                Function>> && !is_executor_v<std::decay_t<Future>> && is_future_v<std::decay_t<Future>> && detail::is_valid_continuation_v<std::decay_t<Function>, std::decay_t<Future>>,
            int> = 0
#endif
        >
    decltype(auto)
    then(Future &&before, Function &&after) {
        return then(
            ::futures::make_default_executor(),
            std::forward<Future>(before),
            std::forward<Function>(after));
    }

    /// \brief Operator to schedule a continuation function to a future
    ///
    /// \return A continuation to the before future
    template <
        class Future,
        typename Function
#ifndef FUTURES_DOXYGEN
        ,
        std::enable_if_t<
            !is_executor_v<std::decay_t<
                Function>> && !is_executor_v<std::decay_t<Future>> && is_future_v<std::decay_t<Future>> && detail::is_valid_continuation_v<std::decay_t<Function>, std::decay_t<Future>>,
            int> = 0
#endif
        >
    auto
    operator>>(Future &&before, Function &&after) {
        return then(std::forward<Future>(before), std::forward<Function>(after));
    }

    /// \brief Schedule a continuation function to a future with a custom
    /// executor
    ///
    /// \return A continuation to the before future
    template <
        class Executor,
        class Future,
        typename Function
#ifndef FUTURES_DOXYGEN
        ,
        std::enable_if_t<
            is_executor_v<std::decay_t<
                Executor>> && !is_executor_v<std::decay_t<Function>> && !is_executor_v<std::decay_t<Future>> && is_future_v<std::decay_t<Future>> && detail::is_valid_continuation_v<std::decay_t<Function>, std::decay_t<Future>>,
            int> = 0
#endif
        >
    auto
    operator>>(
        Future &&before,
        std::pair<const Executor &, Function &> &&after) {
        return then(
            after.first,
            std::forward<Future>(before),
            std::forward<Function>(after.second));
    }

    /// \brief Create a proxy pair to schedule a continuation function to a
    /// future with a custom executor
    ///
    /// For this operation, we needed an operator with higher precedence than
    /// operator>> Our options are: +, -, *, /, %, &, !, ~. Although + seems
    /// like an obvious choice, % is the one that leads to less conflict with
    /// other functions.
    ///
    /// \return A proxy pair to schedule execution
    template <
        class Executor,
        typename Function,
        typename... Args
#ifndef FUTURES_DOXYGEN
        ,
        std::enable_if_t<
            is_executor_v<std::decay_t<
                Executor>> && !is_executor_v<std::decay_t<Function>> && !is_callable_v<std::decay_t<Executor>> && is_callable_v<std::decay_t<Function>>,
            int> = 0
#endif
        >
    auto
    operator%(const Executor &ex, Function &&after) {
        return std::make_pair(std::cref(ex), std::ref(after));
    }

    /** @} */
} // namespace futures

#endif // FUTURES_THEN_H

// #include <futures/adaptor/when_all.h>
#ifndef FUTURES_WHEN_ALL_H
#define FUTURES_WHEN_ALL_H

/// \file Implement the when_all functionality for futures and executors
///
/// Because all tasks need to be done to achieve the result, the algorithm
/// doesn't depend much on the properties of the underlying futures. The thread
/// that is awaiting just needs sleep and await for each of the internal
/// futures.
///
/// The usual approach, without our future concepts, like in returning another
/// std::future, is to start another polling thread, which sets a promise when
/// all other futures are ready. If the futures support lazy continuations,
/// these promises can be set from the previous objects. However, this has an
/// obvious cost for such a trivial operation, given that the solutions is
/// already available in the underlying futures.
///
/// Instead, we implement one more future type `when_all_future` that can query
/// if the futures are ready and waits for them to be ready whenever get() is
/// called. This proxy object can then be converted to a regular future if the
/// user needs to.
///
/// This has a disadvantage over futures with lazy continuations because we
/// might need to schedule another task if we need notifications from this
/// future. However, we avoid scheduling another task right now, so this is, at
/// worst, as good as the common approach of wrapping it into another existing
/// future type.
///
/// If the input futures are not shared, they are moved into `when_all_future`
/// and are invalidated, as usual. The `when_all_future` cannot be shared.

// #include <futures/futures/traits/to_future.h>
#ifndef FUTURES_TO_FUTURE_H
#define FUTURES_TO_FUTURE_H

namespace futures {
    /// \brief Trait to convert input type to its proper future type
    ///
    /// - Futures become their decayed versions
    /// - Lambdas become futures
    ///
    /// The primary template handles non-future types
    template <typename T, class Enable = void>
    struct to_future
    {
        using type = void;
    };

    /// \brief Trait to convert input type to its proper future type
    /// (specialization for future types)
    ///
    /// - Futures become their decayed versions
    /// - Lambdas become futures
    ///
    /// The primary template handles non-future types
    template <typename Future>
    struct to_future<Future, std::enable_if_t<is_future_v<std::decay_t<Future>>>>
    {
        using type = std::decay_t<Future>;
    };

    /// \brief Trait to convert input type to its proper future type
    /// (specialization for functions)
    ///
    /// - Futures become their decayed versions
    /// - Lambdas become futures
    ///
    /// The primary template handles non-future types
    template <typename Lambda>
    struct to_future<
        Lambda,
        std::enable_if_t<std::is_invocable_v<std::decay_t<Lambda>>>>
    {
        using type = futures::cfuture<
            std::invoke_result_t<std::decay_t<Lambda>>>;
    };

    template <class T>
    using to_future_t = typename to_future<T>::type;
} // namespace futures

#endif // FUTURES_TO_FUTURE_H

// #include <futures/algorithm/traits/is_range.h>

// #include <futures/adaptor/detail/traits/is_tuple.h>

// #include <futures/adaptor/detail/tuple_algorithm.h>

// #include <futures/futures/detail/small_vector.h>


namespace futures {
    /** \addtogroup adaptors Adaptors
     *  @{
     */

    /// \brief Proxy future class referring to the result of a conjunction of
    /// futures from @ref when_all
    ///
    /// This class implements the behavior of the `when_all` operation as
    /// another future type, which can handle heterogeneous future objects.
    ///
    /// This future type logically checks the results of other futures in place
    /// to avoid creating a real conjunction of futures that would need to be
    /// polling (or be a lazy continuation) on another thread.
    ///
    /// If the user does want to poll on another thread, then this can be
    /// converted into a cfuture as usual with async. If the other future holds
    /// the when_all_state as part of its state, then it can become another
    /// future.
    template <class Sequence>
    class when_all_future
    {
    private:
        using sequence_type = Sequence;
        using corresponding_future_type = std::future<sequence_type>;
        static constexpr bool sequence_is_range = is_range_v<
            sequence_type>;
        static constexpr bool sequence_is_tuple = is_tuple_v<sequence_type>;
        static_assert(sequence_is_range || sequence_is_tuple);

    public:
        /// \brief Default constructor.
        /// Constructs a when_all_future with no shared state. After
        /// construction, valid() == false
        when_all_future() noexcept = default;

        /// \brief Move a sequence of futures into the when_all_future
        /// constructor. The sequence is moved into this future object and the
        /// objects from which the sequence was created get invalidated
        explicit when_all_future(sequence_type &&v) noexcept
            : v(std::move(v)) {}

        /// \brief Move constructor.
        /// Constructs a when_all_future with the shared state of other using
        /// move semantics. After construction, other.valid() == false
        when_all_future(when_all_future &&other) noexcept
            : v(std::move(other.v)) {}

        /// \brief when_all_future is not CopyConstructible
        when_all_future(const when_all_future &other) = delete;

        /// \brief Releases any shared state.
        /// - If the return object or provider holds the last reference to its
        /// shared state, the shared state is destroyed
        /// - the return object or provider gives up its reference to its shared
        /// state This means we just need to let the internal futures destroy
        /// themselves
        ~when_all_future() = default;

        /// \brief Assigns the contents of another future object.
        /// Releases any shared state and move-assigns the contents of other to
        /// *this. After the assignment, other.valid() == false and
        /// this->valid() will yield the same value as other.valid() before the
        /// assignment.
        when_all_future &
        operator=(when_all_future &&other) noexcept {
            v = std::move(other.v);
        }

        /// \brief Assigns the contents of another future object.
        /// when_all_future is not CopyAssignable.
        when_all_future &
        operator=(const when_all_future &other)
            = delete;

        /// \brief Wait until all futures have a valid result and retrieves it
        /// It effectively calls wait() in order to wait for the result.
        /// The behavior is undefined if valid() is false before the call to
        /// this function. Any shared state is released. valid() is false after
        /// a call to this method. The value v stored in the shared state, as
        /// std::move(v)
        sequence_type
        get() {
            // Check if the sequence is valid
            if (!valid()) {
                throw std::future_error(std::future_errc::no_state);
            }
            // Wait for the complete sequence to be ready
            wait();
            // Move results
            return std::move(v);
        }

        /// \brief Checks if the future refers to a shared state
        [[nodiscard]] bool
        valid() const noexcept {
            if constexpr (sequence_is_range) {
                return std::all_of(v.begin(), v.end(), [](auto &&f) {
                    return f.valid();
                });
            } else {
                return tuple_all_of(v, [](auto &&f) { return f.valid(); });
            }
        }

        /// \brief Blocks until the result becomes available.
        /// valid() == true after the call.
        /// The behavior is undefined if valid() == false before the call to
        /// this function
        void
        wait() const {
            // Check if the sequence is valid
            if (!valid()) {
                throw std::future_error(std::future_errc::no_state);
            }
            if constexpr (sequence_is_range) {
                std::for_each(v.begin(), v.end(), [](auto &&f) { f.wait(); });
            } else {
                tuple_for_each(v, [](auto &&f) { f.wait(); });
            }
        }

        /// \brief Waits for the result to become available.
        /// Blocks until specified timeout_duration has elapsed or the result
        /// becomes available, whichever comes first.
        template <class Rep, class Period>
        [[nodiscard]] std::future_status
        wait_for(
            const std::chrono::duration<Rep, Period> &timeout_duration) const {
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
                if (!valid()) {
                    throw std::future_error(std::future_errc::no_state);
                }
                using duration_type = std::chrono::duration<Rep, Period>;
                using namespace std::chrono;
                auto start_time = system_clock::now();
                duration_type total_elapsed = duration_cast<duration_type>(
                    nanoseconds(0));
                auto equal_fn = [&](auto &&f) {
                    std::future_status s = f.wait_for(
                        timeout_duration - total_elapsed);
                    total_elapsed = duration_cast<duration_type>(
                        system_clock::now() - start_time);
                    const bool when_all_impossible
                        = s != std::future_status::ready;
                    return when_all_impossible
                           || total_elapsed > timeout_duration;
                };
                if constexpr (sequence_is_range) {
                    // Use a hack to "break" for_each loops with find_if
                    auto it = std::find_if(v.begin(), v.end(), equal_fn);
                    return (it == v.end()) ?
                               std::future_status::ready :
                               it->wait_for(seconds(0));
                } else {
                    auto idx = tuple_find_if(v, equal_fn);
                    if (idx == std::tuple_size<sequence_type>()) {
                        return std::future_status::ready;
                    } else {
                        std::future_status s = apply(
                            [](auto &&el) -> std::future_status {
                                return el.wait_for(seconds(0));
                            },
                            v,
                            idx);
                        return s;
                    }
                }
            }
        }

        /// \brief wait_until waits for a result to become available.
        /// It blocks until specified timeout_time has been reached or the
        /// result becomes available, whichever comes first
        template <class Clock, class Duration>
        std::future_status
        wait_until(const std::chrono::time_point<Clock, Duration> &timeout_time)
            const {
            auto now_time = std::chrono::system_clock::now();
            return now_time > timeout_time ?
                       wait_for(std::chrono::seconds(0)) :
                       wait_for(
                           timeout_time - std::chrono::system_clock::now());
        }

        /// \brief Allow move the underlying sequence somewhere else
        /// The when_all_future is left empty and should now be considered
        /// invalid. This is useful for the algorithm that merges two
        /// wait_all_future objects without forcing encapsulation of the merge
        /// function.
        sequence_type &&
        release() {
            return std::move(v);
        }

        /// \brief Request the stoppable futures to stop
        bool
        request_stop() noexcept {
            bool any_request = false;
            auto f_request_stop = [&](auto &&f) {
                any_request = any_request || f.request_stop();
            };
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
    /// \brief Specialization explicitly setting when_all_future<T> as a type of
    /// future
    template <typename T>
    struct is_future<when_all_future<T>> : std::true_type
    {};
#endif

    namespace detail {
        /// \name when_all helper traits
        /// Useful traits for when all future
        ///@{

        /// \brief Check if type is a when_all_future as a type
        template <typename>
        struct is_when_all_future : std::false_type
        {};
        template <typename Sequence>
        struct is_when_all_future<when_all_future<Sequence>> : std::true_type
        {};

        /// \brief Check if type is a when_all_future as constant bool
        template <class T>
        constexpr bool is_when_all_future_v = is_when_all_future<T>::value;

        /// \brief Check if a type can be used in a future conjunction (when_all
        /// or operator&& for futures)
        template <class T>
        using is_valid_when_all_argument = std::disjunction<
            is_future<std::decay_t<T>>,
            std::is_invocable<std::decay_t<T>>>;
        template <class T>
        constexpr bool is_valid_when_all_argument_v
            = is_valid_when_all_argument<T>::value;

        /// \brief Trait to identify valid when_all inputs
        template <class...>
        struct are_valid_when_all_arguments : std::true_type
        {};
        template <class B1>
        struct are_valid_when_all_arguments<B1> : is_valid_when_all_argument<B1>
        {};
        template <class B1, class... Bn>
        struct are_valid_when_all_arguments<B1, Bn...>
            : std::conditional_t<
                  is_valid_when_all_argument_v<B1>,
                  are_valid_when_all_arguments<Bn...>,
                  std::false_type>
        {};
        template <class... Args>
        constexpr bool are_valid_when_all_arguments_v
            = are_valid_when_all_arguments<Args...>::value;
        /// @}

        /// \name Helpers and traits for operator&& on futures, functions and
        /// when_all futures
        /// @{

        /// \brief Check if type is a when_all_future with tuples as a sequence
        /// type
        template <typename T, class Enable = void>
        struct is_when_all_tuple_future : std::false_type
        {};
        template <typename Sequence>
        struct is_when_all_tuple_future<
            when_all_future<Sequence>,
            std::enable_if_t<is_tuple_v<Sequence>>> : std::true_type
        {};
        template <class T>
        constexpr bool is_when_all_tuple_future_v = is_when_all_tuple_future<
            T>::value;

        /// \brief Check if all template parameters are when_all_future with
        /// tuples as a sequence type
        template <class...>
        struct are_when_all_tuple_futures : std::true_type
        {};
        template <class B1>
        struct are_when_all_tuple_futures<B1>
            : is_when_all_tuple_future<std::decay_t<B1>>
        {};
        template <class B1, class... Bn>
        struct are_when_all_tuple_futures<B1, Bn...>
            : std::conditional_t<
                  is_when_all_tuple_future_v<std::decay_t<B1>>,
                  are_when_all_tuple_futures<Bn...>,
                  std::false_type>
        {};
        template <class... Args>
        constexpr bool are_when_all_tuple_futures_v
            = are_when_all_tuple_futures<Args...>::value;

        /// \brief Check if type is a when_all_future with a range as a sequence
        /// type
        template <typename T, class Enable = void>
        struct is_when_all_range_future : std::false_type
        {};
        template <typename Sequence>
        struct is_when_all_range_future<
            when_all_future<Sequence>,
            std::enable_if_t<is_range_v<Sequence>>> : std::true_type
        {};
        template <class T>
        constexpr bool is_when_all_range_future_v = is_when_all_range_future<
            T>::value;

        /// \brief Check if all template parameters are when_all_future with
        /// tuples as a sequence type
        template <class...>
        struct are_when_all_range_futures : std::true_type
        {};
        template <class B1>
        struct are_when_all_range_futures<B1> : is_when_all_range_future<B1>
        {};
        template <class B1, class... Bn>
        struct are_when_all_range_futures<B1, Bn...>
            : std::conditional_t<
                  is_when_all_range_future_v<B1>,
                  are_when_all_range_futures<Bn...>,
                  std::false_type>
        {};
        template <class... Args>
        constexpr bool are_when_all_range_futures_v
            = are_when_all_range_futures<Args...>::value;

        /// \brief Constructs a when_all_future that is a concatenation of all
        /// when_all_futures in args It's important to be able to merge
        /// when_all_future objects because of operator&& When the user asks for
        /// f1 && f2 && f3, we want that to return a single future that waits
        /// for <f1,f2,f3> rather than a future that wait for two futures
        /// <f1,<f2,f3>> \note This function only participates in overload
        /// resolution if all types in std::decay_t<WhenAllFutures>... are
        /// specializations of when_all_future with a tuple sequence type
        /// \overload "Merging" a single when_all_future of tuples. Overload
        /// provided for symmetry.
        template <
            class WhenAllFuture,
            std::enable_if_t<is_when_all_tuple_future_v<WhenAllFuture>, int> = 0>
        decltype(auto)
        when_all_future_cat(WhenAllFuture &&arg0) {
            return std::forward<WhenAllFuture>(arg0);
        }

        /// \overload Merging a two when_all_future objects of tuples
        template <
            class WhenAllFuture1,
            class WhenAllFuture2,
            std::enable_if_t<
                are_when_all_tuple_futures_v<WhenAllFuture1, WhenAllFuture2>,
                int> = 0>
        decltype(auto)
        when_all_future_cat(WhenAllFuture1 &&arg0, WhenAllFuture2 &&arg1) {
            auto s1 = std::move(std::forward<WhenAllFuture1>(arg0).release());
            auto s2 = std::move(std::forward<WhenAllFuture2>(arg1).release());
            auto s = std::tuple_cat(std::move(s1), std::move(s2));
            return when_all_future(std::move(s));
        }

        /// \overload Merging two+ when_all_future of tuples
        template <
            class WhenAllFuture1,
            class... WhenAllFutures,
            std::enable_if_t<
                are_when_all_tuple_futures_v<WhenAllFuture1, WhenAllFutures...>,
                int> = 0>
        decltype(auto)
        when_all_future_cat(WhenAllFuture1 &&arg0, WhenAllFutures &&...args) {
            auto s1 = std::move(std::forward<WhenAllFuture1>(arg0).release());
            auto s2 = std::move(
                when_all_future_cat(std::forward<WhenAllFutures>(args)...)
                    .release());
            auto s = std::tuple_cat(std::move(s1), std::move(s2));
            return when_all_future(std::move(s));
        }

        // When making the tuple for when_all_future:
        // - futures need to be moved
        // - shared futures need to be copied
        // - lambdas need to be posted
        template <typename F>
        constexpr decltype(auto)
        move_share_or_post(F &&f) {
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

    /// \brief Create a future object that becomes ready when the range of input
    /// futures becomes ready
    ///
    /// This function does not participate in overload resolution unless
    /// InputIt's value type (i.e., typename
    /// std::iterator_traits<InputIt>::value_type) is a std::future or
    /// std::shared_future.
    ///
    /// This overload uses a small vector for avoid further allocations for such
    /// a simple operation.
    ///
    /// \return Future object of type @ref when_all_future
    template <
        class InputIt
#ifndef FUTURES_DOXYGEN
        ,
        std::enable_if_t<
            detail::is_valid_when_all_argument_v<
                typename std::iterator_traits<InputIt>::value_type>,
            int> = 0
#endif
        >
    when_all_future<detail::small_vector<
        to_future_t<typename std::iterator_traits<InputIt>::value_type>>>
    when_all(InputIt first, InputIt last) {
        // Infer types
        using input_type = std::decay_t<
            typename std::iterator_traits<InputIt>::value_type>;
        constexpr bool input_is_future = is_future_v<input_type>;
        constexpr bool input_is_invocable = std::is_invocable_v<input_type>;
        static_assert(input_is_future || input_is_invocable);
        using output_future_type = to_future_t<input_type>;
        using sequence_type = detail::small_vector<output_future_type>;
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
            std::transform(first, last, std::back_inserter(v), [](auto &&f) {
                return std::move(futures::async(std::forward<decltype(f)>(f)));
            });
        }

        return when_all_future<sequence_type>(std::move(v));
    }

    /// \brief Create a future object that becomes ready when the range of input
    /// futures becomes ready
    ///
    /// This function does not participate in overload resolution unless the
    /// range type @ref is_future.
    ///
    /// \return Future object of type @ref when_all_future
    template <
        class Range
#ifndef FUTURES_DOXYGEN
        ,
        std::enable_if_t<is_range_v<std::decay_t<Range>>, int> = 0
#endif
        >
    when_all_future<
        detail::small_vector<to_future_t<typename std::iterator_traits<
            typename std::decay_t<Range>::iterator>::value_type>>>
    when_all(Range &&r) {
        return when_all(
            std::begin(std::forward<Range>(r)),
            std::end(std::forward<Range>(r)));
    }

    /// \brief Create a future object that becomes ready when all of the input
    /// futures become ready
    ///
    /// This function does not participate in overload resolution unless every
    /// argument is either a (possibly cv-qualified) shared_future or a
    /// cv-unqualified future, as defined by the trait @ref is_future.
    ///
    /// \return Future object of type @ref when_all_future
    template <
        class... Futures
#ifndef FUTURES_DOXYGEN
        ,
        std::enable_if_t<
            detail::are_valid_when_all_arguments_v<Futures...>,
            int> = 0
#endif
        >
    when_all_future<std::tuple<to_future_t<Futures>...>>
    when_all(Futures &&...futures) {
        // Infer sequence type
        using sequence_type = std::tuple<to_future_t<Futures>...>;

        // Create sequence (and infer types as we go)
        sequence_type v = std::make_tuple(
            (detail::move_share_or_post(futures))...);

        return when_all_future<sequence_type>(std::move(v));
    }

    /// \brief Operator to create a future object that becomes ready when all of
    /// the input futures are ready
    ///
    /// Cperator&& works for futures and functions (which are converted to
    /// futures with the default executor) If the future is a when_all_future
    /// itself, then it gets merged instead of becoming a child future of
    /// another when_all_future.
    ///
    /// When the user asks for f1 && f2 && f3, we want that to return a single
    /// future that waits for <f1,f2,f3> rather than a future that wait for two
    /// futures <f1,<f2,f3>>.
    ///
    /// This emulates the usual behavior we expect from other types with
    /// operator&&.
    ///
    /// Note that this default behaviour is different from when_all(...), which
    /// doesn't merge the when_all_future objects by default, because they are
    /// variadic functions and this intention can be controlled explicitly:
    /// - when_all(f1,f2,f3) -> <f1,f2,f3>
    /// - when_all(f1,when_all(f2,f3)) -> <f1,<f2,f3>>
    ///
    /// \return @ref when_all_future object that concatenates all futures
    template <
        class T1,
        class T2
#ifndef FUTURES_DOXYGEN
        ,
        std::enable_if_t<
            detail::is_valid_when_all_argument_v<
                T1> && detail::is_valid_when_all_argument_v<T2>,
            int> = 0
#endif
        >
    auto
    operator&&(T1 &&lhs, T2 &&rhs) {
        constexpr bool first_is_when_all = detail::is_when_all_future_v<T1>;
        constexpr bool second_is_when_all = detail::is_when_all_future_v<T2>;
        constexpr bool both_are_when_all = first_is_when_all
                                           && second_is_when_all;
        if constexpr (both_are_when_all) {
            // Merge when all futures with operator&&
            return detail::when_all_future_cat(
                std::forward<T1>(lhs),
                std::forward<T2>(rhs));
        } else {
            // At least one of the arguments is not a when_all_future.
            // Any such argument might be another future or a function which
            // needs to become a future. Thus, we need a function to maybe
            // convert these functions to futures.
            auto maybe_make_future = [](auto &&f) {
                if constexpr (
                    std::is_invocable_v<
                        decltype(f)> && !is_future_v<decltype(f)>) {
                    // Convert to future with the default executor if not a
                    // future yet
                    return asio::post(
                        make_default_executor(),
                        asio::use_future(std::forward<decltype(f)>(f)));
                } else {
                    if constexpr (is_shared_future_v<decltype(f)>) {
                        return std::forward<decltype(f)>(f);
                    } else {
                        return std::move(std::forward<decltype(f)>(f));
                    }
                }
            };
            // Simplest case, join futures in a new when_all_future
            constexpr bool none_are_when_all = !first_is_when_all
                                               && !second_is_when_all;
            if constexpr (none_are_when_all) {
                return when_all(
                    maybe_make_future(std::forward<T1>(lhs)),
                    maybe_make_future(std::forward<T2>(rhs)));
            } else if constexpr (first_is_when_all) {
                // If one of them is a when_all_future, then we need to
                // concatenate the results rather than creating a child in the
                // sequence. To concatenate them, the one that is not a
                // when_all_future needs to become one.
                return detail::when_all_future_cat(
                    lhs,
                    when_all(maybe_make_future(std::forward<T2>(rhs))));
            } else /* if constexpr (second_is_when_all) */ {
                return detail::when_all_future_cat(
                    when_all(maybe_make_future(std::forward<T1>(lhs))),
                    rhs);
            }
        }
    }

    /** @} */
} // namespace futures

#endif // FUTURES_WHEN_ALL_H

// #include <futures/adaptor/when_any.h>
#ifndef FUTURES_WHEN_ANY_H
#define FUTURES_WHEN_ANY_H

/// \file Implement the when_any functionality for futures and executors
/// The same rationale as when_all applies here
/// \see https://en.cppreference.com/w/cpp/experimental/when_any

// #include <futures/adaptor/when_any_result.h>

// #include <futures/algorithm/traits/is_range.h>

// #include <futures/futures/async.h>

// #include <futures/futures/traits/to_future.h>

// #include <futures/adaptor/detail/traits/is_tuple.h>

// #include <futures/adaptor/detail/tuple_algorithm.h>

// #include <futures/futures/detail/small_vector.h>

#include <array>
#include <optional>
// #include <condition_variable>

// #include <shared_mutex>


namespace futures {
    /** \addtogroup adaptors Adaptors
     *  @{
     */

    /// \brief Proxy future class referring to the result of a disjunction of
    /// futures from @ref when_any
    ///
    /// This class implements another future type to identify when one of the
    /// tasks is over.
    ///
    /// As with `when_all`, this class acts as a future that checks the results
    /// of other futures to avoid creating a real disjunction of futures that
    /// would need to be polling on another thread.
    ///
    /// If the user does want to poll on another thread, then this can be
    /// converted into a regular future type with async or std::async.
    ///
    /// Not-polling is easier to emulate for future conjunctions (when_all)
    /// because we can sleep on each task until they are all done, since we need
    /// all of them anyway.
    ///
    /// For disjunctions, we have few options:
    /// - If the input futures have lazy continuations:
    ///   - Attach continuations to notify when a task is over
    /// - If the input futures do not have lazy continuations:
    ///   - Polling in a busy loop until one of the futures is ready
    ///   - Polling with exponential backoffs until one of the futures is ready
    ///   - Launching n continuation tasks that set a promise when one of the
    ///   futures is ready
    ///   - Hybrids, usually polling for short tasks and launching threads for
    ///   other tasks
    /// - If the input futures are mixed in regards to lazy continuations:
    ///   - Mix the strategies above, depending on each input future
    ///
    /// If the thresholds for these strategies are reasonable, this should be
    /// efficient for futures with or without lazy continuations.
    ///
    template <class Sequence>
    class when_any_future
    {
    private:
        using sequence_type = Sequence;
        static constexpr bool sequence_is_range = is_range_v<sequence_type>;
        static constexpr bool sequence_is_tuple = is_tuple_v<sequence_type>;
        static_assert(sequence_is_range || sequence_is_tuple);

    public:
        /// \brief Default constructor.
        /// Constructs a when_any_future with no shared state. After
        /// construction, valid() == false
        when_any_future() noexcept = default;

        /// \brief Move a sequence of futures into the when_any_future
        /// constructor. The sequence is moved into this future object and the
        /// objects from which the sequence was created get invalidated.
        ///
        /// We immediately set up the notifiers for any input future that
        /// supports lazy continuations.
        explicit when_any_future(sequence_type &&v) noexcept
            : v(std::move(v)), thread_notifiers_set(false),
              ready_notified(false) {
            maybe_set_up_lazy_notifiers();
        }

        /// \brief Move constructor.
        /// Constructs a when_any_future with the shared state of other using
        /// move semantics. After construction, other.valid() == false
        ///
        /// This is a class that controls resources, and their behavior needs to
        /// be moved. However, unlike a vector, some notifier resources cannot
        /// be moved and might need to be recreated, because they expect the
        /// underlying futures to be in a given address to work.
        ///
        /// We cannot move the notifiers because these expect things to be
        /// notified at certain addresses. This means the notifiers in `other`
        /// have to be stopped and we have to be sure of that before its
        /// destructor gets called.
        ///
        /// There are two in operations here.
        /// - Asking the notifiers to stop and waiting
        ///   - This is what we need to do at the destructor because we can't
        ///   destruct "this" until
        ///     we are sure no notifiers are going to try to notify this object
        /// - Asking the notifiers to stop
        ///   - This is what we need to do when moving, because we know we won't
        ///   need these notifiers
        ///     anymore. When the moved object gets destructed, it will ensure
        ///     its notifiers are stopped and finish the task.
        when_any_future(when_any_future &&other) noexcept
            : thread_notifiers_set(false), ready_notified(false) {
            other.request_notifiers_stop();
            // we can only move v after stopping the notifiers, or they will
            // keep trying to access invalid future address before they stop
            v = std::move(other.v);
            // Set up our own lazy notifiers
            maybe_set_up_lazy_notifiers();
        }

        /// \brief when_any_future is not CopyConstructible
        when_any_future(const when_any_future &other) = delete;

        /// \brief Releases any shared state.
        ///
        /// - If the return object or provider holds the last reference to its
        /// shared state, the shared state is destroyed.
        /// - the return object or provider gives up its reference to its shared
        /// state
        ///
        /// This means we just need to let the internal futures destroy
        /// themselves, but we have to stop notifiers if we have any, because
        /// these notifiers might later try to set tokens in a future that no
        /// longer exists.
        ///
        ~when_any_future() {
            request_notifiers_stop_and_wait();
        };

        /// \brief Assigns the contents of another future object.
        ///
        /// Releases any shared state and move-assigns the contents of other to
        /// *this.
        ///
        /// After the assignment, other.valid() == false and this->valid() will
        /// yield the same value as other.valid() before the assignment.
        ///
        when_any_future &
        operator=(when_any_future &&other) noexcept {
            v = std::move(other.v);
            other.request_notifiers_stop();
            // Set up our own lazy notifiers
            maybe_set_up_lazy_notifiers();
        }

        /// \brief Copy assigns the contents of another when_any_future object.
        ///
        /// when_any_future is not copy assignable.
        when_any_future &
        operator=(const when_any_future &other)
            = delete;

        /// \brief Wait until any future has a valid result and retrieves it
        ///
        /// It effectively calls wait() in order to wait for the result.
        /// This avoids replicating the logic behind continuations, polling, and
        /// notifiers.
        ///
        /// The behavior is undefined if valid() is false before the call to
        /// this function. Any shared state is released. valid() is false after
        /// a call to this method. The value v stored in the shared state, as
        /// std::move(v)
        when_any_result<sequence_type>
        get() {
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
        /// This future is always valid() unless there are tasks and they are
        /// all invalid
        ///
        /// \see https://en.cppreference.com/w/cpp/experimental/when_any
        [[nodiscard]] bool
        valid() const noexcept {
            if constexpr (sequence_is_range) {
                if (v.empty()) {
                    return true;
                }
                return std::any_of(v.begin(), v.end(), [](auto &&f) {
                    return f.valid();
                });
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
        /// The behavior is undefined if valid() == false before the call to
        /// this function
        void
        wait() const {
            // Check if the sequence is valid
            if (!valid()) {
                throw std::future_error(std::future_errc::no_state);
            }
            // Reuse the logic from wait_for here
            using const_version = std::true_type;
            using timeout_version = std::false_type;
            wait_for_common<const_version, timeout_version>(
                *this,
                std::chrono::seconds(0));
        }

        /// \overload mutable version which allows setting up notifiers which
        /// might not have been set yet
        void
        wait() {
            // Check if the sequence is valid
            if (!valid()) {
                throw std::future_error(std::future_errc::no_state);
            }
            // Reuse the logic from wait_for here
            using const_version = std::false_type;
            using timeout_version = std::false_type;
            wait_for_common<const_version, timeout_version>(
                *this,
                std::chrono::seconds(0));
        }

        /// \brief Waits for the result to become available.
        /// Blocks until specified timeout_duration has elapsed or the result
        /// becomes available, whichever comes first. Not-polling is easier to
        /// emulate for future conjunctions (when_all) because we can sleep on
        /// each task until they are all done, since we need all of them anyway.
        /// For disjunctions, we have few options:
        /// - Polling in a busy loop until one of the futures is ready
        /// - Polling in a less busy loop with exponential backoffs
        /// - Launching n continuation tasks that set a promise when one of the
        /// futures is ready
        /// - Hybrids, usually polling for short tasks and launching threads for
        /// other tasks If these parameters are reasonable, this should not be
        /// less efficient that futures with continuations, because we save on
        /// the creation of new tasks. However, the relationship between the
        /// parameters depend on:
        /// - The number of tasks (n)
        /// - The estimated time of completion for each task (assumes a time
        /// distribution)
        /// - The probably a given task is the first task to finish (>=1/n)
        /// - The cost of launching continuation tasks
        /// Because we don't have access to information about the estimated time
        /// for a given task to finish, we can ignore a less-busy loop as a
        /// general solution. Thus, we can come up with a hybrid algorithm for
        /// all cases:
        /// - If there's only one task, behave as when_all
        /// - If there are more tasks:
        ///   1) Initially poll in a busy loop for a while, because tasks might
        ///   finish very sooner than we would need
        ///      to create a continuations.
        ///   2) Create continuation tasks after a threshold time
        /// \see https://en.m.wikipedia.org/wiki/Exponential_backoff
        template <class Rep, class Period>
        std::future_status
        wait_for(
            const std::chrono::duration<Rep, Period> &timeout_duration) const {
            using const_version = std::true_type;
            using timeout_version = std::true_type;
            return wait_for_common<const_version, timeout_version>(
                *this,
                timeout_duration);
        }

        /// \overload wait for might need to be mutable so we can set up the
        /// notifiers \note the const version will only wait for notifiers if
        /// these have already been set
        template <class Rep, class Period>
        std::future_status
        wait_for(const std::chrono::duration<Rep, Period> &timeout_duration) {
            using const_version = std::false_type;
            using timeout_version = std::true_type;
            return wait_for_common<const_version, timeout_version>(
                *this,
                timeout_duration);
        }

        /// \brief wait_until waits for a result to become available.
        /// It blocks until specified timeout_time has been reached or the
        /// result becomes available, whichever comes first
        template <class Clock, class Duration>
        std::future_status
        wait_until(const std::chrono::time_point<Clock, Duration> &timeout_time)
            const {
            auto now_time = std::chrono::system_clock::now();
            return now_time > timeout_time ?
                       wait_for(std::chrono::seconds(0)) :
                       wait_for(
                           timeout_time - std::chrono::system_clock::now());
        }

        /// \overload mutable version that allows setting notifiers
        template <class Clock, class Duration>
        std::future_status
        wait_until(
            const std::chrono::time_point<Clock, Duration> &timeout_time) {
            auto now_time = std::chrono::system_clock::now();
            return now_time > timeout_time ?
                       wait_for(std::chrono::seconds(0)) :
                       wait_for(
                           timeout_time - std::chrono::system_clock::now());
        }

        /// \brief Check if it's ready
        [[nodiscard]] bool
        is_ready() const {
            auto idx = get_ready_index();
            return idx != static_cast<size_t>(-1) || (size() == 0);
        }

        /// \brief Allow move the underlying sequence somewhere else
        /// The when_any_future is left empty and should now be considered
        /// invalid. This is useful for the algorithm that merges two
        /// wait_any_future objects without forcing encapsulation of the merge
        /// function.
        sequence_type &&
        release() {
            request_notifiers_stop();
            return std::move(v);
        }

    private:
        /// \brief Get index of the first internal future that is ready
        /// If no future is ready, this returns the sequence size as a sentinel
        template <class CheckLazyContinuables = std::true_type>
        [[nodiscard]] size_t
        get_ready_index() const {
            const auto eq_comp = [](auto &&f) {
                if constexpr (
                    CheckLazyContinuables::value
                    || (!is_lazy_continuable_v<std::decay_t<decltype(f)>>) )
                {
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
        /// \tparam const_version std::true_type or std::false_type for version
        /// with or without setting up new notifiers \tparam timeout_version
        /// std::true_type or std::false_type for version with or without
        /// timeout \tparam Rep Common std::chrono Rep time representation
        /// \tparam Period Common std::chrono Period time representation \param
        /// f when_any_future on which we want to wait \param timeout_duration
        /// Max time we should wait for a result (if timeout version is
        /// std::true_type) \return Status of the future
        /// (std::future_status::ready if any is ready)
        template <
            class const_version,
            class timeout_version,
            class Rep = std::chrono::seconds::rep,
            class Period = std::chrono::seconds::period>
        static std::future_status
        wait_for_common(
            std::conditional_t<
                const_version::value,
                std::add_const<when_any_future>,
                when_any_future> &f,
            const std::chrono::duration<Rep, Period> &timeout_duration) {
            constexpr bool is_trivial_tuple = sequence_is_tuple
                                              && (compile_time_size() < 2);
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

                // All future types have their own notifiers as continuations
                // created when this object starts. Don't busy wait if thread
                // notifiers are already set anyway.
                if (f.all_lazy_continuable() || f.thread_notifiers_set) {
                    return f.template notifier_wait_for<timeout_version>(
                        timeout_duration);
                }

                // Choose busy or future wait for, depending on how much time we
                // have to wait and whether the notifiers have been set in a
                // previous function call
                if constexpr (const_version::value) {
                    // We cannot set up notifiers in the busy version of this
                    // function even though this is encapsulated. Maybe the
                    // notifiers should be always mutable, like we usually do
                    // with mutexes, but better safe than sorry. This has a
                    // small impact on continuable futures through.
                    return f.template busy_wait_for<timeout_version>(
                        timeout_duration);
                } else /* if not const_version::value */ {
                    // - Don't busy wait forever, even though it implements an
                    // exponential backoff
                    // - Don't create notifiers for very short tasks either
                    const std::chrono::seconds max_busy_time(5);
                    const bool no_time_to_setup_notifiers = timeout_duration
                                                            < max_busy_time;
                    // - Don't create notifiers if there are more tasks than the
                    // hardware limit already:
                    //   - If there are more tasks, the probability of a ready
                    //   task increases while the cost
                    //     of notifiers is higher.
                    const bool too_many_threads_already
                        = f.size() >= hardware_concurrency();
                    const bool busy_wait_only = no_time_to_setup_notifiers
                                                || too_many_threads_already;
                    if (busy_wait_only) {
                        return f.template busy_wait_for<timeout_version>(
                            timeout_duration);
                    } else {
                        std::future_status s = f.template busy_wait_for<
                            timeout_version>(max_busy_time);
                        if (s != std::future_status::ready) {
                            f.maybe_set_up_thread_notifiers();
                            return f
                                .template notifier_wait_for<timeout_version>(
                                    timeout_duration - max_busy_time);
                        } else {
                            return s;
                        }
                    }
                }
            }
        }

        /// \brief Get number of internal futures
        [[nodiscard]] constexpr size_t
        size() const {
            if constexpr (sequence_is_tuple) {
                return std::tuple_size_v<sequence_type>;
            } else {
                return v.size();
            }
        }

        /// \brief Get number of internal futures with lazy continuations
        [[nodiscard]] constexpr size_t
        lazy_continuable_size() const {
            if constexpr (sequence_is_tuple) {
                return std::tuple_size_v<sequence_type>;
                size_t count = 0;
                tuple_for_each(v, [&count](auto &&el) {
                    if constexpr (is_lazy_continuable_v<
                                      std::decay_t<decltype(el)>>) {
                        ++count;
                    }
                });
                return count;
            } else {
                if (is_lazy_continuable_v<
                        std::decay_t<typename sequence_type::value_type>>) {
                    return v.size();
                } else {
                    return 0;
                }
            }
        }

        /// \brief Check if all internal types are lazy continuable
        [[nodiscard]] constexpr bool
        all_lazy_continuable() const {
            return lazy_continuable_size() == size();
        }

        /// \brief Get size, if we know that at compile time
        [[nodiscard]] static constexpr size_t
        compile_time_size() {
            if constexpr (sequence_is_tuple) {
                return std::tuple_size_v<sequence_type>;
            } else {
                return 0;
            }
        }

        /// \brief Check if the i-th future is ready
        [[nodiscard]] bool
        is_ready(size_t index) const {
            if constexpr (!sequence_is_range) {
                return apply(
                           [](auto &&el) -> std::future_status {
                               return el.wait_for(std::chrono::seconds(0));
                           },
                           v,
                           index)
                       == std::future_status::ready;
            } else {
                return v[index].wait_for(std::chrono::seconds(0))
                       == std::future_status::ready;
            }
        }

        /// \brief Busy wait for a certain amount of time
        /// \see https://en.m.wikipedia.org/wiki/Exponential_backoff
        template <
            class timeout_version,
            class Rep = std::chrono::seconds::rep,
            class Period = std::chrono::seconds::period>
        std::future_status
        busy_wait_for(
            const std::chrono::duration<Rep, Period> &timeout_duration
            = std::chrono::seconds(0)) const {
            // Check if the sequence is valid
            if (!valid()) {
                throw std::future_error(std::future_errc::no_state);
            }
            // Wait for on each thread, increasingly accounting for the time we
            // waited from the total
            using duration_type = std::chrono::duration<Rep, Period>;
            using namespace std::chrono;
            // 1) Total time we've waited so far
            auto start_time = system_clock::now();
            duration_type total_elapsed = duration_cast<duration_type>(
                nanoseconds(0));
            // 2) The time we are currently waiting on each thread (something
            // like a minimum slot time)
            nanoseconds each_wait_for(1);
            // 3) After how long we should start increasing the each_wait_for
            // Wait longer in the busy pool if we have more tasks to account for
            // the lower individual end in less-busy
            const auto full_busy_timeout_duration = milliseconds(100) * size();
            // 4) The increase factor of each_wait_for (20%)
            using each_wait_for_increase_factor = std::ratio<5, 4>;
            // 5) The max time we should wait on each thread (at most the
            // estimated time to create a thread) Assumes 1000 threads can be
            // created per second and split that with the number of tasks so
            // that finding the task would never take longer than creating a
            // thread
            auto max_wait_for = duration_cast<nanoseconds>(microseconds(20))
                                / size();
            // 6) Index of the future we found to be ready
            auto ready_in_the_meanwhile_idx = size_t(-1);
            // 7) Function to identify if a given task is ready
            // We will later keep looping on this function until we run out of
            // time
            auto equal_fn = [&](auto &&f) {
                // a) Don't wait on the ones with lazy continuations
                // We are going to wait on all of them at once later
                if constexpr (is_lazy_continuable_v<std::decay_t<decltype(f)>>)
                {
                    return false;
                }
                // b) update parameters of our heuristic _before_ checking the
                // future
                const bool use_backoffs = total_elapsed
                                          > full_busy_timeout_duration;
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
                // e) if this one wasn't ready, and we are already using
                // exponential backoff
                if (s != std::future_status::ready && use_backoffs) {
                    // do a single pass in other futures to make sure no other
                    // future got ready in the meanwhile
                    size_t ready_in_the_meanwhile_idx = get_ready_index();
                    if (ready_in_the_meanwhile_idx != size_t(-1)) {
                        return true;
                    }
                }
                // f) request function to break the loop if we found something
                // that's ready or we timed out
                return s == std::future_status::ready
                       || (timeout_version::value
                           && total_elapsed > timeout_duration);
            };
            // 8) Loop through the futures trying to find a ready future with
            // the function above
            do {
                // Check for a signal from lazy continuable futures all at once
                // The notifiers for lazy continuable futures are always set up
                if (lazy_continuable_size() != 0 && notifiers_started()) {
                    std::future_status s = wait_for_ready_notification<
                        timeout_version>(each_wait_for);
                    if (s == std::future_status::ready) {
                        return s;
                    }
                }
                // Check if other futures aren't ready
                // Use a hack to "break" "for_each" algorithms with find_if
                auto idx = size_t(-1);
                if constexpr (sequence_is_range) {
                    idx = std::find_if(v.begin(), v.end(), equal_fn)
                          - v.begin();
                } else {
                    idx = tuple_find_if(v, equal_fn);
                }
                const bool found_ready_future = idx != size();
                if (found_ready_future) {
                    size_t ready_found_idx
                        = ready_in_the_meanwhile_idx != size_t(-1) ?
                              ready_in_the_meanwhile_idx :
                              idx;
                    if constexpr (sequence_is_range) {
                        return (v.begin() + ready_found_idx)
                            ->wait_for(seconds(0));
                    } else {
                        return apply(
                            [](auto &&el) -> std::future_status {
                                return el.wait_for(seconds(0));
                            },
                            v,
                            ready_found_idx);
                    }
                }
            }
            while (!timeout_version::value || total_elapsed < timeout_duration);
            return std::future_status::timeout;
        }

        /// \brief Check if the notifiers have started wait on futures
        [[nodiscard]] bool
        notifiers_started() const {
            if constexpr (sequence_is_range) {
                return std::any_of(
                    notifiers.begin(),
                    notifiers.end(),
                    [](const notifier &n) { return n.start_token.load(); });
            } else {
                return tuple_any_of(v, [](auto &&f) { return f.valid(); });
            }
        };

        /// \brief Launch continuation threads and wait for any of them instead
        ///
        /// This is the second alternative to busy waiting, but once we used
        /// this alternative, we already have paid the price to create these
        /// continuation futures and we shouldn't busy wait ever again, even in
        /// a new call to wait_for. Thus, the wait_any_future needs to keep
        /// track of these continuation tasks to ensure this doesn't happen.
        ///
        /// Once the notifiers are set, if using an unlimited number of threads,
        /// we would only need to wait without a more specific timeout here.
        ///
        /// However, if the notifiers could not be launched for some reason,
        /// this can lock our process in the somewhat rare condition that (i)
        /// the last function is running in the last available pool thread, and
        /// (ii) none of the notifiers got a chance to get into the pool. We are
        /// then waiting for notifiers that don't exist yet and we don't have
        /// access to that information.
        ///
        /// To avoid that from happening, we (i) perform a single busy pass in
        /// the underlying futures from time to time to ensure they are really
        /// still running, regardless of the notifiers, and (ii) create a start
        /// token, besides the cancel token, for the notifiers to indicate that
        /// they have really started waiting. Thus, we can always check
        /// condition (i) to ensure we don't already have the results before we
        /// start waiting for them, and (ii) to ensure we don't wait for
        /// notifiers that are not running yet.
        template <
            class timeout_version,
            class Rep = std::chrono::seconds::rep,
            class Period = std::chrono::seconds::period>
        std::future_status
        notifier_wait_for(
            const std::chrono::duration<Rep, Period> &timeout_duration
            = std::chrono::seconds(0)) {
            // Check if that have started yet and do some busy waiting while
            // they haven't
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
                }
                while (!notifiers_started());
            }

            // wait for ready_notified to be set to true by a notifier task
            return wait_for_ready_notification<timeout_version>(
                timeout_duration);
        }

        /// \brief Sleep and wait for ready_notified to be set to true by a
        /// notifier task
        template <
            class timeout_version,
            class Rep = std::chrono::seconds::rep,
            class Period = std::chrono::seconds::period>
        std::future_status
        wait_for_ready_notification(
            const std::chrono::duration<Rep, Period> &timeout_duration
            = std::chrono::seconds(0)) const {
            // Create lock to be able to read/wait_for "ready_notified" with the
            // condition variable
            std::unique_lock<std::mutex> lock(ready_notified_mutex);

            // Check if it isn't true yet. Already notified.
            if (ready_notified) {
                return std::future_status::ready;
            }

            // Wait to be notified by another thread
            if constexpr (timeout_version::value) {
                ready_notified_cv.wait_for(lock, timeout_duration, [this]() {
                    return ready_notified;
                });
            } else {
                while (!ready_notified_cv
                            .wait_for(lock, std::chrono::seconds(1), [this]() {
                                return ready_notified;
                            }))
                {
                    if (is_ready()) {
                        return std::future_status::ready;
                    }
                }
            }

            // convert what we got into a future status
            return ready_notified ? std::future_status::ready :
                                    std::future_status::timeout;
        }

        /// \brief Stop the notifiers
        /// We might need to stop the notifiers if the when_any_future is being
        /// destroyed, or they might try to set a condition variable that no
        /// longer exists.
        ///
        /// This is something we also have to check when merging two
        /// when_any_future objects, because this creates a new future with a
        /// single notifier that needs to replace the old notifiers.
        ///
        /// In practice, all of that should happen very rarely, but things get
        /// ugly when it happens.
        ///
        void
        request_notifiers_stop_and_wait() {
            // Check if we have notifiers
            if (!thread_notifiers_set && !lazy_notifiers_set) {
                return;
            }

            // Set each cancel token to true (separately to cancel as soon as
            // possible)
            std::for_each(notifiers.begin(), notifiers.end(), [](auto &&n) {
                n.cancel_token.store(true);
            });

            // Wait for each future<void> notifier
            // - We have to wait for them even if they haven't started, because
            // we don't have that kind of control
            //   over the executor queue. They need to start running, see the
            //   cancel token and just give up.
            std::for_each(notifiers.begin(), notifiers.end(), [](auto &&n) {
                if (n.task.valid()) {
                    n.task.wait();
                }
            });

            thread_notifiers_set = false;
        }

        /// \brief Request stop but don't wait
        /// This is useful when moving the object, because we know we won't need
        /// the notifiers anymore but we don't want to waste time waiting for
        /// them yet.
        void
        request_notifiers_stop() {
            // Check if we have notifiers
            if (!thread_notifiers_set && !lazy_notifiers_set) {
                return;
            }

            // Set each cancel token to true (separately to cancel as soon as
            // possible)
            std::for_each(notifiers.begin(), notifiers.end(), [](auto &&n) {
                n.cancel_token.store(true);
            });
        }

        void
        maybe_set_up_lazy_notifiers() {
            maybe_set_up_notifiers_common<std::true_type>();
        }

        void
        maybe_set_up_thread_notifiers() {
            maybe_set_up_notifiers_common<std::false_type>();
        }

        /// \brief Common functionality to setup notifiers
        ///
        /// The logic for setting notifiers for futures with and without lazy
        /// continuations is almost the same.
        ///
        /// The task is the same but the difference is:
        /// 1) the notification task is a continuation if the future supports
        /// continuations, and 2) the notification task goes into a new new
        /// thread if the future does not support continuations.
        ///
        /// @note Unfortunately, we need a new thread an not only a new task in
        /// some executor whenever the task doesn't support continuations
        /// because we cannot be sure there's room in the executor for the
        /// notification task.
        ///
        /// This might be counter intuitive, as one could assume there's going
        /// to be room for the notifications as soon as the ongoing tasks are
        /// running. However, there are a few situations where this might
        /// happen:
        ///
        /// 1) The current tasks we are waiting for have not been launched yet
        /// and the executor is busy with
        ///    tasks that need cancellation to stop
        /// 2) Some of the tasks we are waiting for are running and some are
        /// enqueued. The running tasks finish
        ///    but we don't hear about it because the enqueued tasks come before
        ///    the notification.
        /// 3) All tasks we are waiting for have no support for continuations.
        /// The executor has no room for the
        ///    notifier because of some parallel tasks happening in the executor
        ///    and we never hear about a future getting ready.
        ///
        /// So although this is an edge case, we cannot assume there's room for
        /// the notifications in the executor.
        ///
        template <class SettingLazyContinuables>
        void
        maybe_set_up_notifiers_common() {
            constexpr bool setting_notifiers_as_continuations
                = SettingLazyContinuables::value;
            constexpr bool setting_notifiers_as_new_threads
                = !SettingLazyContinuables::value;

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
            const bool init_ready = (setting_notifiers_as_continuations
                                     && (!thread_notifiers_set))
                                    || (setting_notifiers_as_new_threads
                                        && (!lazy_notifiers_set));
            if (init_ready) {
                ready_notified = false;
            }

            // Check if there are threads to set up
            const bool no_compatible_futures
                = (setting_notifiers_as_new_threads && all_lazy_continuable())
                  || (setting_notifiers_as_continuations
                      && lazy_continuable_size() == 0);
            if (no_compatible_futures) {
                return;
            }

            // Function that posts a notifier task to update our common variable
            // that indicates if the task is ready
            auto launch_notifier_task =
                [&](auto &&future,
                    std::atomic_bool &cancel_token,
                    std::atomic_bool &start_token) {
                // Launch a task with access to the underlying when_any_future
                // and a cancel token that asks it to stop These threads need to
                // be independent of any executor because the whole system might
                // crash if the executor has no room for them. So this is either
                // a real continuation or a new thread. Direct access to the
                // when_any_future avoid another level of indirection, but makes
                // things a little harder to get right.

                // Create promise the notifier needs to fulfill
                // All we need to know is whether it's over
                promise<void> p;
                auto std_future = p.get_future<futures::future<void>>();
                auto notifier_task =
                    [p = std::move(p),
                     &future,
                     &cancel_token,
                     &start_token,
                     this]() mutable {
                    // The very first thing we need to do is set the start
                    // token, so we never wait for notifiers that aren't running
                    start_token.store(true);

                    // Check if we haven't started too late
                    if (cancel_token.load()) {
                        p.set_value();
                        return;
                    }

                    // If future is ready at the start, just ensure
                    // wait_any_future knows about it already before setting up
                    // more state data for this task:
                    // - `is_ready` shouldn't fail in this case because the
                    // future is ready, but we haven't
                    //   called `get()` yet, so its state is valid or the whole
                    //   when_any_future would be invalid already.
                    // - We also have to ensure it's valid in case we've moved
                    // the when_any_future, and we
                    //   requested this to stop before we even got to this
                    //   point. In this case, this task is accessing the correct
                    //   location, but we invalidated the future when moving it
                    //   we did so correctly, to avoid blocking unnecessarily
                    //   when moving.
                    if (!future.valid() || ::futures::is_ready(future)) {
                        std::lock_guard lk(ready_notified_mutex);
                        if (!ready_notified) {
                            ready_notified = true;
                            ready_notified_cv.notify_one();
                        }
                        p.set_value();
                        return;
                    }

                    // Set the max waiting time for each wait_for operation
                    // This task might need to be cancelled, so we cannot wait
                    // on the future forever. So we `wait_for(max_waiting_time)`
                    // rather than `wait()` forever. There are two reasons for
                    // this:
                    // - The main when_any object is being destroyed when we
                    // found nothing yet.
                    // - Other tasks might have found the ready value, so this
                    // task can stop running. A number of heuristics can be used
                    // to adjust this time, but both conditions are supposed to
                    // be relatively rare.
                    const std::chrono::seconds max_waiting_time(1);

                    // Waits for the future to be ready, sleeping most of the
                    // time
                    while (future.wait_for(max_waiting_time)
                           != std::future_status::ready) {
                        // But once in a while, check if:
                        // - the main when_any_future has requested this
                        // operation to stop
                        //   because it's being destructed (cheaper condition)
                        if (cancel_token.load()) {
                            p.set_value();
                            return;
                        }
                        // - any other tasks haven't set the ready condition
                        // yet, so we can terminate a continuation task we no
                        // longer need
                        std::lock_guard lk(ready_notified_mutex);
                        if (ready_notified) {
                            p.set_value();
                            return;
                        }
                    }

                    // We found out about a future that's ready: notify the
                    // when_any_future object
                    std::lock_guard lk(ready_notified_mutex);
                    if (!ready_notified) {
                        ready_notified = true;
                        // Notify any thread that might be waiting for this event
                        ready_notified_cv.notify_one();
                    }
                    p.set_value();
                };

                // Create a copiable handle for the notifier task
                auto notifier_task_ptr = std::make_shared<
                    decltype(notifier_task)>(std::move(notifier_task));
                auto executor_handle = [notifier_task_ptr]() {
                    (*notifier_task_ptr)();
                };

                // Post the task appropriately
                using future_type = std::decay_t<decltype(future)>;
                // MSVC hack
                constexpr bool internal_lazy_continuable
                    = is_lazy_continuable_v<future_type>;
                constexpr bool internal_setting_lazy = SettingLazyContinuables::
                    value;
                constexpr bool internal_setting_thread
                    = !SettingLazyContinuables::value;
                if constexpr (internal_setting_lazy && internal_lazy_continuable)
                {
                    // Execute notifier task inline whenever `future` is done
                    future.then(make_inline_executor(), executor_handle);
                } else if constexpr (
                    internal_setting_thread && !internal_lazy_continuable) {
                    // Execute notifier task in a new thread because we don't
                    // have the executor context to be sure. We detach it here
                    // but can still control the cancel_token and the future.
                    // This is basically the same as calling std::async and
                    // ignoring its std::future because we already have one set
                    // up.
                    std::thread(executor_handle).detach();
                }
                // Return a future we can use to wait for the notifier and
                // ensure it's done
                return std_future;
            };

            // Launch the notification task for each future
            if constexpr (sequence_is_range) {
                if constexpr (
                    is_lazy_continuable_v<
                        typename sequence_type::
                            value_type> && setting_notifiers_as_new_threads)
                {
                    return;
                } else if constexpr (
                    !is_lazy_continuable_v<
                        typename sequence_type::
                            value_type> && setting_notifiers_as_continuations)
                {
                    return;
                } else {
                    // Ensure we have one notifier allocated for each task
                    notifiers.resize(size());
                    // For each future in v
                    for (size_t i = 0; i < size(); ++i) {
                        notifiers[i].cancel_token.store(false);
                        notifiers[i].start_token.store(false);
                        // Launch task with reference to this future and its
                        // tokens
                        notifiers[i].task = std::move(launch_notifier_task(
                            v[i],
                            notifiers[i].cancel_token,
                            notifiers[i].start_token));
                    }
                }
            } else {
                for_each_paired(
                    v,
                    notifiers,
                    [&](auto &this_future, notifier &n) {
                    using future_type = std::decay_t<decltype(this_future)>;
                    constexpr bool current_is_lazy_continuable_v
                        = is_lazy_continuable_v<future_type>;
                    constexpr bool internal_setting_thread
                        = !SettingLazyContinuables::value;
                    constexpr bool internal_setting_lazy
                        = SettingLazyContinuables::value;
                    if constexpr (
                        current_is_lazy_continuable_v
                        && internal_setting_thread) {
                        return;
                    } else if constexpr (
                        (!current_is_lazy_continuable_v)
                        && internal_setting_lazy) {
                        return;
                    } else {
                        n.cancel_token.store(false);
                        n.start_token.store(false);
                        future<void> tmp = launch_notifier_task(
                            this_future,
                            n.cancel_token,
                            n.start_token);
                        n.task = std::move(tmp);
                    }
                    });
            }
        }

    private:
        /// \name Helpers to infer the type for the notifiers
        ///
        /// The array of futures comes with an array of tokens that also allows
        /// us to cancel the notifiers We shouldn't need these tokens because we
        /// do expect the notifiers to deliver their promise before object
        /// destruction and we don't usually expect to merge when_any_futures
        /// for which we have started notifiers. This is still a requirement to
        /// make sure the notifier model works. We could probably use stop
        /// tokens instead of atomic_bool in C++20
        /// @{

        /// \brief Type that defines an internal when_any notifier task
        ///
        /// A notifier task notifies the when_any_future of any internal future
        /// that is ready.
        ///
        /// We use this notifier type instead of a std::pair because futures
        /// need to be moved and the atomic bools do not, but std::pair
        /// conservatively deletes the move constructor because of atomic_bool.
        struct notifier
        {
            /// A simple task that notifies us whenever the task is ready
            future<void> task{ make_ready_future() };

            /// Cancel the notification task
            std::atomic_bool cancel_token{ false };

            /// Notifies this task the notification task has started
            std::atomic_bool start_token{ false };

            /// Construct a ready notifier
            notifier() = default;

            /// Construct a notifier from an existing future
            notifier(future<void> &&f, bool c, bool s)
                : task(std::move(f)), cancel_token(c), start_token(s) {}

            /// Move a notifier
            notifier(notifier &&rhs) noexcept
                : task(std::move(rhs.task)),
                  cancel_token(rhs.cancel_token.load()),
                  start_token(rhs.start_token.load()) {}
        };

#ifdef FUTURES_USE_SMALL_VECTOR
        using notifier_vector = ::futures::small_vector<notifier>;
#else
        // Whenever small::vector in unavailable we use std::vector because
        // boost small_vector couldn't handle move-only notifiers
        using notifier_vector = ::std::vector<notifier>;
#endif
        using notifier_array = std::array<notifier, compile_time_size()>;

        using notifier_sequence_type = std::
            conditional_t<sequence_is_range, notifier_vector, notifier_array>;
        /// @}

    private:
        /// \brief Internal wait_any_future state
        sequence_type v;

        /// \name Variables for the notifiers to indicate if the future is ready
        /// They indicate if any underlying future has been identified as ready
        /// by an auxiliary thread or as a lazy continuation to an existing
        /// future type.
        /// @{
        notifier_sequence_type notifiers;
        bool thread_notifiers_set{ false };
        bool lazy_notifiers_set{ false };
        bool ready_notified{ false };
        mutable std::mutex ready_notified_mutex;
        mutable std::condition_variable ready_notified_cv;
        /// @}
    };

#ifndef FUTURES_DOXYGEN
    /// \name Define when_any_future as a kind of future
    /// @{
    /// Specialization explicitly setting when_any_future<T> as a type of future
    template <typename T>
    struct is_future<when_any_future<T>> : std::true_type
    {};

    /// Specialization explicitly setting when_any_future<T> & as a type of
    /// future
    template <typename T>
    struct is_future<when_any_future<T> &> : std::true_type
    {};

    /// Specialization explicitly setting when_any_future<T> && as a type of
    /// future
    template <typename T>
    struct is_future<when_any_future<T> &&> : std::true_type
    {};

    /// Specialization explicitly setting const when_any_future<T> as a type of
    /// future
    template <typename T>
    struct is_future<const when_any_future<T>> : std::true_type
    {};

    /// Specialization explicitly setting const when_any_future<T> & as a type
    /// of future
    template <typename T>
    struct is_future<const when_any_future<T> &> : std::true_type
    {};
    /// @}
#endif

    namespace detail {
        /// \name Useful traits for when_any
        /// @{

        /// \brief Check if type is a when_any_future as a type
        template <typename>
        struct is_when_any_future : std::false_type
        {};
        template <typename Sequence>
        struct is_when_any_future<when_any_future<Sequence>> : std::true_type
        {};
        template <typename Sequence>
        struct is_when_any_future<const when_any_future<Sequence>>
            : std::true_type
        {};
        template <typename Sequence>
        struct is_when_any_future<when_any_future<Sequence> &> : std::true_type
        {};
        template <typename Sequence>
        struct is_when_any_future<when_any_future<Sequence> &&> : std::true_type
        {};
        template <typename Sequence>
        struct is_when_any_future<const when_any_future<Sequence> &>
            : std::true_type
        {};

        /// \brief Check if type is a when_any_future as constant bool
        template <class T>
        constexpr bool is_when_any_future_v = is_when_any_future<T>::value;

        /// \brief Check if a type can be used in a future disjunction (when_any
        /// or operator|| for futures)
        template <class T>
        using is_valid_when_any_argument = std::
            disjunction<is_future<T>, std::is_invocable<T>>;
        template <class T>
        constexpr bool is_valid_when_any_argument_v
            = is_valid_when_any_argument<T>::value;

        /// \brief Trait to identify valid when_any inputs
        template <class...>
        struct are_valid_when_any_arguments : std::true_type
        {};
        template <class B1>
        struct are_valid_when_any_arguments<B1> : is_valid_when_any_argument<B1>
        {};
        template <class B1, class... Bn>
        struct are_valid_when_any_arguments<B1, Bn...>
            : std::conditional_t<
                  is_valid_when_any_argument_v<B1>,
                  are_valid_when_any_arguments<Bn...>,
                  std::false_type>
        {};
        template <class... Args>
        constexpr bool are_valid_when_any_arguments_v
            = are_valid_when_any_arguments<Args...>::value;

        /// \subsection Helpers for operator|| on futures, functions and
        /// when_any futures

        /// \brief Check if type is a when_any_future with tuples as a sequence
        /// type
        template <typename T, class Enable = void>
        struct is_when_any_tuple_future : std::false_type
        {};
        template <typename Sequence>
        struct is_when_any_tuple_future<
            when_any_future<Sequence>,
            std::enable_if_t<is_tuple_v<Sequence>>> : std::true_type
        {};
        template <class T>
        constexpr bool is_when_any_tuple_future_v = is_when_any_tuple_future<
            T>::value;

        /// \brief Check if all template parameters are when_any_future with
        /// tuples as a sequence type
        template <class...>
        struct are_when_any_tuple_futures : std::true_type
        {};
        template <class B1>
        struct are_when_any_tuple_futures<B1>
            : is_when_any_tuple_future<std::decay_t<B1>>
        {};
        template <class B1, class... Bn>
        struct are_when_any_tuple_futures<B1, Bn...>
            : std::conditional_t<
                  is_when_any_tuple_future_v<std::decay_t<B1>>,
                  are_when_any_tuple_futures<Bn...>,
                  std::false_type>
        {};
        template <class... Args>
        constexpr bool are_when_any_tuple_futures_v
            = are_when_any_tuple_futures<Args...>::value;

        /// \brief Check if type is a when_any_future with a range as a sequence
        /// type
        template <typename T, class Enable = void>
        struct is_when_any_range_future : std::false_type
        {};
        template <typename Sequence>
        struct is_when_any_range_future<
            when_any_future<Sequence>,
            std::enable_if_t<is_range_v<Sequence>>> : std::true_type
        {};
        template <class T>
        constexpr bool is_when_any_range_future_v = is_when_any_range_future<
            T>::value;

        /// \brief Check if all template parameters are when_any_future with
        /// tuples as a sequence type
        template <class...>
        struct are_when_any_range_futures : std::true_type
        {};
        template <class B1>
        struct are_when_any_range_futures<B1> : is_when_any_range_future<B1>
        {};
        template <class B1, class... Bn>
        struct are_when_any_range_futures<B1, Bn...>
            : std::conditional_t<
                  is_when_any_range_future_v<B1>,
                  are_when_any_range_futures<Bn...>,
                  std::false_type>
        {};
        template <class... Args>
        constexpr bool are_when_any_range_futures_v
            = are_when_any_range_futures<Args...>::value;

        /// \brief Constructs a when_any_future that is a concatenation of all
        /// when_any_futures in args It's important to be able to merge
        /// when_any_future objects because of operator|| When the user asks for
        /// f1 && f2 && f3, we want that to return a single future that waits
        /// for <f1,f2,f3> rather than a future that wait for two futures
        /// <f1,<f2,f3>> \note This function only participates in overload
        /// resolution if all types in std::decay_t<WhenAllFutures>... are
        /// specializations of when_any_future with a tuple sequence type
        /// \overload "Merging" a single when_any_future of tuples. Overload
        /// provided for symmetry.
        template <
            class WhenAllFuture,
            std::enable_if_t<is_when_any_tuple_future_v<WhenAllFuture>, int> = 0>
        decltype(auto)
        when_any_future_cat(WhenAllFuture &&arg0) {
            return std::forward<WhenAllFuture>(arg0);
        }

        /// \overload Merging a two when_any_future objects of tuples
        template <
            class WhenAllFuture1,
            class WhenAllFuture2,
            std::enable_if_t<
                are_when_any_tuple_futures_v<WhenAllFuture1, WhenAllFuture2>,
                int> = 0>
        decltype(auto)
        when_any_future_cat(WhenAllFuture1 &&arg0, WhenAllFuture2 &&arg1) {
            auto s1 = std::move(std::forward<WhenAllFuture1>(arg0).release());
            auto s2 = std::move(std::forward<WhenAllFuture2>(arg1).release());
            auto s = std::tuple_cat(std::move(s1), std::move(s2));
            return when_any_future(std::move(s));
        }

        /// \overload Merging two+ when_any_future of tuples
        template <
            class WhenAllFuture1,
            class... WhenAllFutures,
            std::enable_if_t<
                are_when_any_tuple_futures_v<WhenAllFuture1, WhenAllFutures...>,
                int> = 0>
        decltype(auto)
        when_any_future_cat(WhenAllFuture1 &&arg0, WhenAllFutures &&...args) {
            auto s1 = std::move(std::forward<WhenAllFuture1>(arg0).release());
            auto s2 = std::move(
                when_any_future_cat(std::forward<WhenAllFutures>(args)...)
                    .release());
            auto s = std::tuple_cat(std::move(s1), std::move(s2));
            return when_any_future(std::move(s));
        }

        /// @}
    } // namespace detail

    /// \brief Create a future object that becomes ready when any of the futures
    /// in the range is ready
    ///
    /// This function does not participate in overload resolution unless
    /// InputIt's value type (i.e., typename
    /// std::iterator_traits<InputIt>::value_type) @ref is_future .
    ///
    /// This overload uses a small vector to avoid further allocations for such
    /// a simple operation.
    ///
    /// \return @ref when_any_future with all future objects
    template <
        class InputIt
#ifndef FUTURES_DOXYGEN
        ,
        std::enable_if_t<
            detail::is_valid_when_any_argument_v<
                typename std::iterator_traits<InputIt>::value_type>,
            int> = 0
#endif
        >
    when_any_future<detail::small_vector<
        to_future_t<typename std::iterator_traits<InputIt>::value_type>>>
    when_any(InputIt first, InputIt last) {
        // Infer types
        using input_type = std::decay_t<
            typename std::iterator_traits<InputIt>::value_type>;
        constexpr bool input_is_future = is_future_v<input_type>;
        constexpr bool input_is_invocable = std::is_invocable_v<input_type>;
        static_assert(input_is_future || input_is_invocable);
        using output_future_type = to_future_t<input_type>;
        using sequence_type = detail::small_vector<output_future_type>;
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
            std::transform(first, last, std::back_inserter(v), [](auto &&f) {
                return ::futures::async(std::forward<decltype(f)>(f));
            });
        }

        return when_any_future<sequence_type>(std::move(v));
    }

    /// \brief Create a future object that becomes ready when any of the futures
    /// in the range is ready
    ///
    /// This function does not participate in overload resolution unless every
    /// argument is either a (possibly cv-qualified) std::shared_future or a
    /// cv-unqualified std::future.
    ///
    /// \return @ref when_any_future with all future objects
    template <
        class Range
#ifndef FUTURES_DOXYGEN
        ,
        std::enable_if_t<is_range_v<std::decay_t<Range>>, int> = 0
#endif
        >
    when_any_future<
        detail::small_vector<to_future_t<typename std::iterator_traits<
            typename std::decay_t<Range>::iterator>::value_type>>>
    when_any(Range &&r) {
        return when_any(
            std::begin(std::forward<Range>(r)),
            std::end(std::forward<Range>(r)));
    }

    /// \brief Create a future object that becomes ready when any of the input
    /// futures is ready
    ///
    /// This function does not participate in overload resolution unless every
    /// argument is either a (possibly cv-qualified) std::shared_future or a
    /// cv-unqualified std::future.
    ///
    /// \return @ref when_any_future with all future objects
    template <
        class... Futures
#ifndef FUTURES_DOXYGEN
        ,
        std::enable_if_t<
            detail::are_valid_when_any_arguments_v<Futures...>,
            int> = 0
#endif
        >
    when_any_future<std::tuple<to_future_t<Futures>...>>
    when_any(Futures &&...futures) {
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

    /// \brief Operator to create a future object that becomes ready when any of
    /// the input futures is ready
    ///
    /// ready operator|| works for futures and functions (which are converted to
    /// futures with the default executor) If the future is a when_any_future
    /// itself, then it gets merged instead of becoming a child future of
    /// another when_any_future.
    ///
    /// When the user asks for f1 || f2 || f3, we want that to return a single
    /// future that waits for <f1 || f2 || f3> rather than a future that wait
    /// for two futures <f1 || <f2 || f3>>.
    ///
    /// This emulates the usual behavior we expect from other types with
    /// operator||.
    ///
    /// Note that this default behaviour is different from when_any(...), which
    /// doesn't merge the when_any_future objects by default, because they are
    /// variadic functions and this intention can be controlled explicitly:
    /// - when_any(f1,f2,f3) -> <f1 || f2 || f3>
    /// - when_any(f1,when_any(f2,f3)) -> <f1 || <f2 || f3>>
    template <
        class T1,
        class T2
#ifndef FUTURES_DOXYGEN
        ,
        std::enable_if_t<
            detail::is_valid_when_any_argument_v<
                T1> && detail::is_valid_when_any_argument_v<T2>,
            int> = 0
#endif
        >
    auto
    operator||(T1 &&lhs, T2 &&rhs) {
        constexpr bool first_is_when_any = detail::is_when_any_future_v<T1>;
        constexpr bool second_is_when_any = detail::is_when_any_future_v<T2>;
        constexpr bool both_are_when_any = first_is_when_any
                                           && second_is_when_any;
        if constexpr (both_are_when_any) {
            // Merge when all futures with operator||
            return detail::when_any_future_cat(
                std::forward<T1>(lhs),
                std::forward<T2>(rhs));
        } else {
            // At least one of the arguments is not a when_any_future.
            // Any such argument might be another future or a function which
            // needs to become a future. Thus, we need a function to maybe
            // convert these functions to futures.
            auto maybe_make_future = [](auto &&f) {
                if constexpr (
                    std::is_invocable_v<
                        decltype(f)> && (!is_future_v<decltype(f)>) ) {
                    // Convert to future with the default executor if not a
                    // future yet
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
            constexpr bool none_are_when_any = !first_is_when_any
                                               && !second_is_when_any;
            if constexpr (none_are_when_any) {
                return when_any(
                    maybe_make_future(std::forward<T1>(lhs)),
                    maybe_make_future(std::forward<T2>(rhs)));
            } else if constexpr (first_is_when_any) {
                // If one of them is a when_any_future, then we need to
                // concatenate the results rather than creating a child in the
                // sequence. To concatenate them, the one that is not a
                // when_any_future needs to become one.
                return detail::when_any_future_cat(
                    lhs,
                    when_any(maybe_make_future(std::forward<T2>(rhs))));
            } else /* if constexpr (second_is_when_any) */ {
                return detail::when_any_future_cat(
                    when_any(maybe_make_future(std::forward<T1>(lhs))),
                    rhs);
            }
        }
    }

    /** @} */
} // namespace futures
#endif // FUTURES_WHEN_ANY_H


#endif // FUTURES_FUTURES_H
#ifndef FUTURES_ALGORITHM_H
#define FUTURES_ALGORITHM_H

// #include <futures/algorithm/all_of.h>
#ifndef FUTURES_ALL_OF_H
#define FUTURES_ALL_OF_H

// #include <futures/algorithm/partitioner/partitioner.h>
#ifndef FUTURES_PARTITIONER_H
#define FUTURES_PARTITIONER_H

// #include <futures/algorithm/traits/is_input_iterator.h>

// #include <futures/algorithm/traits/is_input_range.h>
#ifndef FUTURES_ALGORITHM_TRAITS_IS_INPUT_RANGE_H
#define FUTURES_ALGORITHM_TRAITS_IS_INPUT_RANGE_H

// #include <futures/algorithm/traits/is_input_iterator.h>

// #include <futures/algorithm/traits/is_range.h>

// #include <futures/algorithm/traits/iterator.h>

// #include <type_traits>


namespace futures {
    /** \addtogroup algorithms Algorithms
     *  @{
     */

    /** \addtogroup traits Traits
     *  @{
     */


    /** \brief A C++17 type trait equivalent to the C++20 input_range concept
     */
#ifdef FUTURES_DOXYGEN
    template <class T>
    using is_input_range = __see_below__;
#else
    template <class T, class = void>
    struct is_input_range : std::false_type
    {};

    template <class T>
    struct is_input_range<T, std::void_t<iterator_t<T>>>
        : std::conjunction<
              // clang-format off
              is_range<T>,
              is_input_iterator<iterator_t<T>>
              // clang-format off
            >
    {};
#endif
    template <class T>
    bool constexpr is_input_range_v = is_input_range<T>::value;

    /** @}*/
    /** @}*/
} // namespace futures

#endif // FUTURES_ALGORITHM_TRAITS_IS_INPUT_RANGE_H

// #include <futures/algorithm/traits/is_range.h>

// #include <futures/algorithm/traits/is_sentinel_for.h>
#ifndef FUTURES_ALGORITHM_TRAITS_IS_SENTINEL_FOR_H
#define FUTURES_ALGORITHM_TRAITS_IS_SENTINEL_FOR_H

// #include <futures/algorithm/traits/is_input_or_output_iterator.h>

// #include <futures/algorithm/traits/is_semiregular.h>
#ifndef FUTURES_ALGORITHM_TRAITS_IS_SEMIREGULAR_H
#define FUTURES_ALGORITHM_TRAITS_IS_SEMIREGULAR_H

// #include <futures/algorithm/traits/is_copyable.h>
#ifndef FUTURES_ALGORITHM_TRAITS_IS_COPYABLE_H
#define FUTURES_ALGORITHM_TRAITS_IS_COPYABLE_H

// #include <futures/algorithm/traits/is_movable.h>
#ifndef FUTURES_ALGORITHM_TRAITS_IS_MOVABLE_H
#define FUTURES_ALGORITHM_TRAITS_IS_MOVABLE_H

// #include <futures/algorithm/traits/is_move_constructible.h>
#ifndef FUTURES_ALGORITHM_TRAITS_IS_MOVE_CONSTRUCTIBLE_H
#define FUTURES_ALGORITHM_TRAITS_IS_MOVE_CONSTRUCTIBLE_H

// #include <futures/algorithm/traits/is_constructible_from.h>
#ifndef FUTURES_ALGORITHM_TRAITS_IS_CONSTRUCTIBLE_FROM_H
#define FUTURES_ALGORITHM_TRAITS_IS_CONSTRUCTIBLE_FROM_H

// #include <type_traits>


namespace futures {
    /** \addtogroup algorithms Algorithms
     *  @{
     */

    /** \addtogroup traits Traits
     *  @{
     */


    /** \brief A C++17 type trait equivalent to the C++20 constructible_from
     * concept
     */
#ifdef FUTURES_DOXYGEN
    template <class T, class... Args>
    using is_constructible_from = __see_below__;
#else
    template <class T, class... Args>
    struct is_constructible_from
        : std::conjunction<
              std::is_destructible<T>,
              std::is_constructible<T, Args...>>
    {};
#endif
    template <class T, class... Args>
    bool constexpr is_constructible_from_v = is_constructible_from<T>::value;
    /** @}*/
    /** @}*/

} // namespace futures

#endif // FUTURES_ALGORITHM_TRAITS_IS_CONSTRUCTIBLE_FROM_H

// #include <futures/algorithm/traits/is_convertible_to.h>
#ifndef FUTURES_ALGORITHM_TRAITS_IS_CONVERTIBLE_TO_H
#define FUTURES_ALGORITHM_TRAITS_IS_CONVERTIBLE_TO_H

// #include <type_traits>


namespace futures {
    /** \addtogroup algorithms Algorithms
     *  @{
     */

    /** \addtogroup traits Traits
     *  @{
     */


    /** \brief A C++17 type trait equivalent to the C++20 convertible_to
     * concept
     */
#ifdef FUTURES_DOXYGEN
    template <class From, class To>
    using is_convertible_to = __see_below__;
#else
    template <class From, class To, class = void>
    struct is_convertible_to : std::false_type
    {};

    template <class From, class To>
    struct is_convertible_to<
        From, To,
        std::void_t<
            // clang-format off
            std::enable_if_t<std::is_convertible_v<From, To>>,
            decltype(static_cast<To>(std::declval<From>()))
            // clang-format on
            >> : std::true_type
    {};
#endif
    template <class From, class To>
    bool constexpr is_convertible_to_v = is_convertible_to<From, To>::value;
    /** @}*/
    /** @}*/

} // namespace futures

#endif // FUTURES_ALGORITHM_TRAITS_IS_CONVERTIBLE_TO_H

// #include <type_traits>


namespace futures {
    /** \addtogroup algorithms Algorithms
     *  @{
     */

    /** \addtogroup traits Traits
     *  @{
     */


    /** \brief A C++17 type trait equivalent to the C++20 move_constructible
     * concept
     */
#ifdef FUTURES_DOXYGEN
    template <class T>
    using is_move_constructible = __see_below__;
#else
    template <class T, class = void>
    struct is_move_constructible : std::false_type
    {};

    template <class T>
    struct is_move_constructible<
        T,
        std::enable_if_t<
            // clang-format off
            is_constructible_from_v<T, T> &&
            is_convertible_to_v<T, T>
            // clang-format on
            >> : std::true_type
    {};
#endif
    template <class T>
    bool constexpr is_move_constructible_v = is_move_constructible<T>::value;

    /** @}*/
    /** @}*/

} // namespace futures

#endif // FUTURES_ALGORITHM_TRAITS_IS_MOVE_CONSTRUCTIBLE_H

// #include <futures/algorithm/traits/is_assignable_from.h>
#ifndef FUTURES_ALGORITHM_TRAITS_IS_ASSIGNABLE_FROM_H
#define FUTURES_ALGORITHM_TRAITS_IS_ASSIGNABLE_FROM_H

// #include <futures/algorithm/traits/is_constructible_from.h>

// #include <futures/algorithm/traits/is_convertible_to.h>

// #include <type_traits>


namespace futures {
    /** \addtogroup algorithms Algorithms
     *  @{
     */

    /** \addtogroup traits Traits
     *  @{
     */


    /** \brief A C++17 type trait equivalent to the C++20 assignable_from
     * concept
     */
#ifdef FUTURES_DOXYGEN
    template <class LHS, class RHS>
    using is_assignable_from = __see_below__;
#else
    template <class LHS, class RHS, class = void>
    struct is_assignable_from : std::false_type
    {};

    template <class LHS, class RHS>
    struct is_assignable_from<
        LHS, RHS,
        std::enable_if_t<
            // clang-format off
            std::is_lvalue_reference_v<LHS> &&
            std::is_same_v<decltype(std::declval<LHS>() = std::declval<RHS&&>()), LHS>
            // clang-format on
            >> : std::true_type
    {};
#endif
    template <class LHS, class RHS>
    bool constexpr is_assignable_from_v = is_assignable_from<LHS, RHS>::value;
    /** @}*/
    /** @}*/

} // namespace futures

#endif // FUTURES_ALGORITHM_TRAITS_IS_ASSIGNABLE_FROM_H

// #include <futures/algorithm/traits/is_swappable.h>
#ifndef FUTURES_ALGORITHM_TRAITS_IS_SWAPPABLE_H
#define FUTURES_ALGORITHM_TRAITS_IS_SWAPPABLE_H

// #include <type_traits>


namespace futures {
    /** \addtogroup algorithms Algorithms
     *  @{
     */

    /** \addtogroup traits Traits
     *  @{
     */


    /** \brief A C++17 type trait equivalent to the C++20 swappable concept
     */
#ifdef FUTURES_DOXYGEN
    template <class T>
    using is_swappable = __see_below__;
#else
    template <class T, class = void>
    struct is_swappable : std::false_type
    {};

    template <class T>
    struct is_swappable<
        T,
        std::void_t<
            // clang-format off
            decltype(swap(std::declval<T&>(), std::declval<T&>()))
            // clang-format on
            >> : std::true_type
    {};
#endif
    template <class T>
    bool constexpr is_swappable_v = is_swappable<T>::value;

    /** @}*/
    /** @}*/

} // namespace futures

#endif // FUTURES_ALGORITHM_TRAITS_IS_SWAPPABLE_H

// #include <type_traits>


namespace futures {
    /** \addtogroup algorithms Algorithms
     *  @{
     */

    /** \addtogroup traits Traits
     *  @{
     */


    /** \brief A C++17 type trait equivalent to the C++20 movable
     * concept
     */
#ifdef FUTURES_DOXYGEN
    template <class T>
    using is_movable = __see_below__;
#else
    template <class T, class = void>
    struct is_movable : std::false_type
    {};

    template <class T>
    struct is_movable<
        T,
        std::enable_if_t<
            // clang-format off
            std::is_object_v<T> &&
            is_move_constructible_v<T> &&
            is_assignable_from_v<T&, T> &&
            is_swappable_v<T>
            // clang-format on
            >> : std::true_type
    {};
#endif
    template <class T>
    bool constexpr is_movable_v = is_movable<T>::value;

    /** @}*/
    /** @}*/

} // namespace futures

#endif // FUTURES_ALGORITHM_TRAITS_IS_MOVABLE_H

// #include <futures/algorithm/traits/is_assignable_from.h>

// #include <type_traits>


namespace futures {
    /** \addtogroup algorithms Algorithms
     *  @{
     */

    /** \addtogroup traits Traits
     *  @{
     */


    /** \brief A C++17 type trait equivalent to the C++20 copyable
     * concept
     */
#ifdef FUTURES_DOXYGEN
    template <class T>
    using is_copyable = __see_below__;
#else
    template <class T, class = void>
    struct is_copyable : std::false_type
    {};

    template <class T>
    struct is_copyable<
        T,
        std::enable_if_t<
            // clang-format off
            std::is_copy_constructible_v<T> &&
            is_movable_v<T> &&
            is_assignable_from_v<T&, T&> &&
            is_assignable_from_v<T&, const T&> &&
            is_assignable_from_v<T&, const T>
            // clang-format on
            >> : std::true_type
    {};
#endif
    template <class T>
    bool constexpr is_copyable_v = is_copyable<T>::value;
    /** @}*/
    /** @}*/

} // namespace futures

#endif // FUTURES_ALGORITHM_TRAITS_IS_COPYABLE_H

// #include <futures/algorithm/traits/is_default_initializable.h>
#ifndef FUTURES_ALGORITHM_TRAITS_IS_DEFAULT_INITIALIZABLE_H
#define FUTURES_ALGORITHM_TRAITS_IS_DEFAULT_INITIALIZABLE_H

// #include <futures/algorithm/traits/is_constructible_from.h>

// #include <type_traits>


namespace futures {
    /** \addtogroup algorithms Algorithms
     *  @{
     */

    /** \addtogroup traits Traits
     *  @{
     */


    /** \brief A C++17 type trait equivalent to the C++20 default_initializable
     * concept
     */
#ifdef FUTURES_DOXYGEN
    template <class T>
    using is_default_initializable = __see_below__;
#else
    template <class T, class = void>
    struct is_default_initializable : std::false_type
    {};

    template <class T>
    struct is_default_initializable<
        T,
        std::void_t<
            // clang-format off
            std::enable_if_t<is_constructible_from_v<T>>,
            decltype(T{})
            // clang-format on
            >> : std::true_type
    {};
#endif
    template <class T>
    bool constexpr is_default_initializable_v = is_default_initializable<T>::value;
    /** @}*/
    /** @}*/

} // namespace futures

#endif // FUTURES_ALGORITHM_TRAITS_IS_DEFAULT_INITIALIZABLE_H

// #include <type_traits>


namespace futures {
    /** \addtogroup algorithms Algorithms
     *  @{
     */

    /** \addtogroup traits Traits
     *  @{
     */


    /** \brief A C++17 type trait equivalent to the C++20 semiregular
     * concept
     */
#ifdef FUTURES_DOXYGEN
    template <class T>
    using is_semiregular = __see_below__;
#else
    template <class T, class = void>
    struct is_semiregular : std::false_type
    {};

    template <class T>
    struct is_semiregular<
        T,
        std::enable_if_t<
            // clang-format off
            is_copyable_v<T> &&
            is_default_initializable_v<T>
            // clang-format on
            >> : std::true_type
    {};
#endif
    template <class T>
    bool constexpr is_semiregular_v = is_semiregular<T>::value;

    /** @}*/
    /** @}*/

} // namespace futures

#endif // FUTURES_ALGORITHM_TRAITS_IS_SEMIREGULAR_H

// #include <type_traits>


namespace futures {
    /** \addtogroup algorithms Algorithms
     *  @{
     */

    /** \addtogroup traits Traits
     *  @{
     */


    /** \brief A C++17 type trait equivalent to the C++20 sentinel_for
     * concept
     */
#ifdef FUTURES_DOXYGEN
    template <class S, class I>
    using is_sentinel_for = __see_below__;
#else
    template <class S, class I, class = void>
    struct is_sentinel_for : std::false_type
    {};

    template <class S, class I>
    struct is_sentinel_for<
        S,
        I,
        std::enable_if_t<
            // clang-format off
            is_semiregular_v<S> &&
            is_input_or_output_iterator_v<I>
            // clang-format on
            >> : std::true_type
    {};
#endif
    template <class S, class I>
    bool constexpr is_sentinel_for_v = is_sentinel_for<S, I>::value;

    /** @}*/
    /** @}*/

} // namespace futures

#endif // FUTURES_ALGORITHM_TRAITS_IS_SENTINEL_FOR_H

// #include <futures/executor/default_executor.h>

// #include <futures/adaptor/detail/traits/has_get.h>

#include <algorithm>
// #include <thread>


/// \file Default partitioners
/// A partitioner is a light callable object that takes a pair of iterators and
/// returns the middle of the sequence. In particular, it returns an iterator
/// `middle` that forms a subrange `first`/`middle` which the algorithm should
/// solve inline before scheduling the subrange `middle`/`last` in the executor.

namespace futures {
    /** \addtogroup algorithms Algorithms
     *  @{
     */

    /** \addtogroup partitioners Partitioners
     *  @{
     */

    /// \brief The halve partitioner always splits the sequence into two parts
    /// of roughly equal size
    ///
    /// The sequence is split up to a minimum grain size.
    /// As a concept, the result from the partitioner is considered a suggestion
    /// for parallelization. For algorithms such as for_each, a partitioner with
    /// a very small grain size might be appropriate if the operation is very
    /// expensive. Some algorithms, such as a binary search, might naturally
    /// adjust this suggestion so that the result makes sense.
    class halve_partitioner
    {
        std::size_t min_grain_size_;

    public:
        /// \brief Halve partition constructor
        /// \param min_grain_size_ Minimum grain size used to split ranges
        inline explicit halve_partitioner(std::size_t min_grain_size_)
            : min_grain_size_(min_grain_size_) {}

        /// \brief Split a range of elements
        /// \tparam I Iterator type
        /// \tparam S Sentinel type
        /// \param first First element in range
        /// \param last Last element in range
        /// \return Iterator to point where sequence should be split
        template <typename I, typename S>
        auto
        operator()(I first, S last) {
            std::size_t size = std::distance(first, last);
            return (size <= min_grain_size_) ?
                       last :
                       std::next(first, (size + 1) / 2);
        }
    };

    /// \brief A partitioner that splits the ranges until it identifies we are
    /// not moving to new threads.
    ///
    /// This partitioner splits the ranges until it identifies we are not moving
    /// to new threads. Apart from that, it behaves as a halve_partitioner,
    /// splitting the range up to a minimum grain size.
    class thread_partitioner
    {
        std::size_t min_grain_size_;
        std::size_t num_threads_{hardware_concurrency()};
        std::thread::id last_thread_id_{};

    public:
        explicit thread_partitioner(std::size_t min_grain_size)
            : min_grain_size_(min_grain_size) {}

        template <typename I, typename S>
        auto
        operator()(I first, S last) {
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
                return (size <= min_grain_size_) ?
                           last :
                           std::next(first, (size + 1) / 2);
            }
            return last;
        }
    };

    /// \brief Default partitioner used by parallel algorithms
    ///
    /// Its type and parameters might change
    using default_partitioner = thread_partitioner;

    /// \brief Determine a reasonable minimum grain size depending on the number
    /// of elements in a sequence
    inline std::size_t
    make_grain_size(std::size_t n) {
        return std::clamp(
            n
                / (8
                   * std::
                       max(std::thread::hardware_concurrency(),
                           static_cast<unsigned int>(1))),
            size_t(1),
            size_t(2048));
    }

    /// \brief Create an instance of the default partitioner with a reasonable
    /// grain size for @ref n elements
    ///
    /// The default partitioner type and parameters might change
    inline default_partitioner
    make_default_partitioner(size_t n) {
        return default_partitioner(make_grain_size(n));
    }

    /// \brief Create an instance of the default partitioner with a reasonable
    /// grain for the range @ref first , @ref last
    ///
    /// The default partitioner type and parameters might change
    template <
        class I,
        class S,
        std::enable_if_t<
            is_input_iterator_v<I> && is_sentinel_for_v<S, I>,
            int> = 0>
    default_partitioner
    make_default_partitioner(I first, S last) {
        return make_default_partitioner(std::distance(first, last));
    }

    /// \brief Create an instance of the default partitioner with a reasonable
    /// grain for the range @ref r
    ///
    /// The default partitioner type and parameters might change
    template <
        class R,
        std::enable_if_t<is_input_range_v<R>, int> = 0>
    default_partitioner
    make_default_partitioner(R &&r) {
        return make_default_partitioner(std::begin(r), std::end(r));
    }

    /// Determine if P is a valid partitioner for the iterator range [I,S]
    template <class T, class I, class S>
    using is_partitioner = std::conjunction<
        std::conditional_t<
            is_input_iterator_v<I>,
            std::true_type,
            std::false_type>,
        std::conditional_t<
            is_input_iterator_v<S>,
            std::true_type,
            std::false_type>,
        std::is_invocable<T, I, S>>;

    /// Determine if P is a valid partitioner for the iterator range [I,S]
    template <class T, class I, class S>
    constexpr bool is_partitioner_v = is_partitioner<T, I, S>::value;

    /// Determine if P is a valid partitioner for the range R
    template <class T, class R, typename = void>
    struct is_range_partitioner : std::false_type
    {};

    template <class T, class R>
    struct is_range_partitioner<T, R, std::enable_if_t<is_range_v<R>>>
        : is_partitioner<
              T,
              iterator_t<R>,
              iterator_t<R>>
    {};

    template <class T, class R>
    constexpr bool is_range_partitioner_v = is_range_partitioner<T, R>::value;

    /** @}*/ // \addtogroup partitioners Partitioners
    /** @}*/ // \addtogroup algorithms Algorithms
} // namespace futures

#endif // FUTURES_PARTITIONER_H
// #include <futures/algorithm/traits/is_forward_iterator.h>
#ifndef FUTURES_ALGORITHM_TRAITS_IS_FORWARD_ITERATOR_H
#define FUTURES_ALGORITHM_TRAITS_IS_FORWARD_ITERATOR_H

// #include <futures/algorithm/traits/is_input_iterator.h>

// #include <futures/algorithm/traits/is_sentinel_for.h>

// #include <futures/algorithm/traits/iter_concept.h>
#ifndef FUTURES_ALGORITHM_TRAITS_ITER_CONCEPT_H
#define FUTURES_ALGORITHM_TRAITS_ITER_CONCEPT_H

// #include <futures/algorithm/traits/has_iterator_traits_iterator_concept.h>
#ifndef FUTURES_ALGORITHM_TRAITS_HAS_ITERATOR_TRAITS_ITERATOR_CONCEPT_H
#define FUTURES_ALGORITHM_TRAITS_HAS_ITERATOR_TRAITS_ITERATOR_CONCEPT_H

// #include <type_traits>

// #include <iterator>


namespace futures {
    /** \addtogroup algorithms Algorithms
     *  @{
     */

    /** \addtogroup traits Traits
     *  @{
     */


    /** \brief A C++17 type trait equivalent to the C++20
     * "has-iterator-traits-iterator-concept" concept
     */
#ifdef FUTURES_DOXYGEN
    template <class T>
    using has_iterator_traits_iterator_concept = __see_below__;
#else
    template <class T, class = void>
    struct has_iterator_traits_iterator_concept : std::false_type
    {};

    template <class T>
    struct has_iterator_traits_iterator_concept<
        T,
        std::void_t<typename std::iterator_traits<T>::iterator_concept>>
        : std::true_type
    {};
#endif
    template <class T>
    bool constexpr has_iterator_traits_iterator_concept_v
        = has_iterator_traits_iterator_concept<T>::value;
    /** @}*/
    /** @}*/

} // namespace futures

#endif // FUTURES_ALGORITHM_TRAITS_HAS_ITERATOR_TRAITS_ITERATOR_CONCEPT_H

// #include <futures/algorithm/traits/has_iterator_traits_iterator_category.h>
#ifndef FUTURES_ALGORITHM_TRAITS_HAS_ITERATOR_TRAITS_ITERATOR_CATEGORY_H
#define FUTURES_ALGORITHM_TRAITS_HAS_ITERATOR_TRAITS_ITERATOR_CATEGORY_H

// #include <type_traits>

// #include <iterator>


namespace futures {
    /** \addtogroup algorithms Algorithms
     *  @{
     */

    /** \addtogroup traits Traits
     *  @{
     */


    /** \brief A C++17 type trait equivalent to the C++20
     * "has-iterator-traits-iterator-category" concept
     */
#ifdef FUTURES_DOXYGEN
    template <class T>
    using has_iterator_traits_iterator_category = __see_below__;
#else
    template <class T, class = void>
    struct has_iterator_traits_iterator_category : std::false_type
    {};

    template <class T>
    struct has_iterator_traits_iterator_category<
        T,
        std::void_t<typename std::iterator_traits<T>::iterator_category>>
        : std::true_type
    {};
#endif
    template <class T>
    bool constexpr has_iterator_traits_iterator_category_v
        = has_iterator_traits_iterator_category<T>::value;
    /** @}*/
    /** @}*/

} // namespace futures

#endif // FUTURES_ALGORITHM_TRAITS_HAS_ITERATOR_TRAITS_ITERATOR_CATEGORY_H

// #include <futures/algorithm/traits/remove_cvref.h>

// #include <iterator>

// #include <type_traits>


namespace futures {
    /** \addtogroup algorithms Algorithms
     *  @{
     */

    /** \addtogroup traits Traits
     *  @{
     */


    /** \brief A C++17 type trait equivalent to the C++20 iter_concept
     * concept
     */
#ifdef FUTURES_DOXYGEN
    template <class T>
    using iter_concept = __see_below__;
#else
    template <class T, class = void>
    struct iter_concept
    {};

    template <class T>
    struct iter_concept<
        T,
        std::enable_if_t<
            // clang-format off
            has_iterator_traits_iterator_concept_v<remove_cvref_t<T>>
            // clang-format on
        >>
    {
        using type = typename std::iterator_traits<
            remove_cvref_t<T>>::iterator_concept;
    };

    template <class T>
    struct iter_concept<
        T,
        std::enable_if_t<
            // clang-format off
            !has_iterator_traits_iterator_concept_v<remove_cvref_t<T>> &&
            has_iterator_traits_iterator_category_v<remove_cvref_t<T>>
            // clang-format on
        >>
    {
        using type = typename std::iterator_traits<
            remove_cvref_t<T>>::iterator_category;
    };

    template <class T>
    struct iter_concept<
        T,
        std::enable_if_t<
            // clang-format off
            !has_iterator_traits_iterator_concept_v<remove_cvref_t<T>> &&
            !has_iterator_traits_iterator_category_v<remove_cvref_t<T>>
            // clang-format on
        >>
    {
        using type = std::random_access_iterator_tag;
    };
#endif
    template <class T>
    using iter_concept_t = typename iter_concept<T>::type;

    /** @}*/
    /** @}*/

} // namespace futures

#endif // FUTURES_ALGORITHM_TRAITS_ITER_CONCEPT_H

// #include <futures/algorithm/traits/is_derived_from.h>
#ifndef FUTURES_ALGORITHM_TRAITS_IS_DERIVED_FROM_H
#define FUTURES_ALGORITHM_TRAITS_IS_DERIVED_FROM_H

// #include <futures/algorithm/traits/iter_reference.h>

// #include <futures/algorithm/traits/iter_rvalue_reference.h>

// #include <futures/algorithm/traits/iter_value.h>

// #include <type_traits>


namespace futures {
    /** \addtogroup algorithms Algorithms
     *  @{
     */

    /** \addtogroup traits Traits
     *  @{
     */


    /** \brief A C++17 type trait equivalent to the C++20 derived_from concept
     */
#ifdef FUTURES_DOXYGEN
    template <class T>
    using is_derived_from = __see_below__;
#else
    template <class Derived, class Base>
    struct is_derived_from
        : std::conjunction<
              std::is_base_of<Base, Derived>,
              std::is_convertible<const volatile Derived*, const volatile Base*>>
    {};
#endif
    template <class Derived, class Base>
    bool constexpr is_derived_from_v = is_derived_from<Derived, Base>::value;
    /** @}*/
    /** @}*/

} // namespace futures

#endif // FUTURES_ALGORITHM_TRAITS_IS_DERIVED_FROM_H

// #include <futures/algorithm/traits/is_incrementable.h>
#ifndef FUTURES_ALGORITHM_TRAITS_IS_INCREMENTABLE_H
#define FUTURES_ALGORITHM_TRAITS_IS_INCREMENTABLE_H

// #include <futures/algorithm/traits/is_regular.h>
#ifndef FUTURES_ALGORITHM_TRAITS_IS_REGULAR_H
#define FUTURES_ALGORITHM_TRAITS_IS_REGULAR_H

// #include <futures/algorithm/traits/is_equality_comparable.h>
#ifndef FUTURES_ALGORITHM_TRAITS_IS_EQUALITY_COMPARABLE_H
#define FUTURES_ALGORITHM_TRAITS_IS_EQUALITY_COMPARABLE_H

// #include <type_traits>


namespace futures {
    /** \addtogroup algorithms Algorithms
     *  @{
     */

    /** \addtogroup traits Traits
     *  @{
     */


    /** \brief A C++17 type trait equivalent to the C++20 equality_comparable
     * concept
     */
#ifdef FUTURES_DOXYGEN
    template <class T>
    using is_equality_comparable = __see_below__;
#else
    template <class T, class = void>
    struct is_equality_comparable : std::false_type
    {};

    template <class T>
    struct is_equality_comparable<
        T,
        std::void_t<
            // clang-format off
            decltype(std::declval<const std::remove_reference_t<T>&>() == std::declval<const std::remove_reference_t<T>&>()),
            decltype(std::declval<const std::remove_reference_t<T>&>() != std::declval<const std::remove_reference_t<T>&>())
            // clang-format on
            >> : std::true_type
    {};
#endif
    template <class T>
    bool constexpr is_equality_comparable_v = is_equality_comparable<T>::value;
    /** @}*/
    /** @}*/

} // namespace futures

#endif // FUTURES_ALGORITHM_TRAITS_IS_EQUALITY_COMPARABLE_H

// #include <futures/algorithm/traits/is_semiregular.h>

// #include <type_traits>


namespace futures {
    /** \addtogroup algorithms Algorithms
     *  @{
     */

    /** \addtogroup traits Traits
     *  @{
     */


    /** \brief A C++17 type trait equivalent to the C++20 regular
     * concept
     */
#ifdef FUTURES_DOXYGEN
    template <class T>
    using is_regular = __see_below__;
#else
    template <class T, class = void>
    struct is_regular : std::false_type
    {};

    template <class T>
    struct is_regular<
        T,
        std::enable_if_t<
            // clang-format off
            is_semiregular_v<T> &&
            is_equality_comparable_v<T>
            // clang-format on
            >> : std::true_type
    {};
#endif
    template <class T>
    bool constexpr is_regular_v = is_regular<T>::value;

    /** @}*/
    /** @}*/

} // namespace futures

#endif // FUTURES_ALGORITHM_TRAITS_IS_REGULAR_H

// #include <futures/algorithm/traits/is_weakly_incrementable.h>
#ifndef FUTURES_ALGORITHM_TRAITS_IS_WEAKLY_INCREMENTABLE_H
#define FUTURES_ALGORITHM_TRAITS_IS_WEAKLY_INCREMENTABLE_H

// #include <futures/algorithm/traits/is_movable.h>

// #include <futures/algorithm/traits/iter_difference.h>
#ifndef FUTURES_ALGORITHM_TRAITS_ITER_DIFFERENCE_H
#define FUTURES_ALGORITHM_TRAITS_ITER_DIFFERENCE_H

// #include <futures/algorithm/traits/has_iterator_traits_difference_type.h>
#ifndef FUTURES_ALGORITHM_TRAITS_HAS_ITERATOR_TRAITS_DIFFERENCE_TYPE_H
#define FUTURES_ALGORITHM_TRAITS_HAS_ITERATOR_TRAITS_DIFFERENCE_TYPE_H

// #include <type_traits>

// #include <iterator>


namespace futures {
    /** \addtogroup algorithms Algorithms
     *  @{
     */

    /** \addtogroup traits Traits
     *  @{
     */


    /** \brief A C++17 type trait equivalent to the C++20
     * "has-iterator-traits-difference-type" concept
     */
#ifdef FUTURES_DOXYGEN
    template <class T>
    using has_iterator_traits_difference_type = __see_below__;
#else
    template <class T, class = void>
    struct has_iterator_traits_difference_type : std::false_type
    {};

    template <class T>
    struct has_iterator_traits_difference_type<
        T,
        std::void_t<typename std::iterator_traits<T>::difference_type>>
        : std::true_type
    {};
#endif
    template <class T>
    bool constexpr has_iterator_traits_difference_type_v
        = has_iterator_traits_difference_type<T>::value;
    /** @}*/
    /** @}*/

} // namespace futures

#endif // FUTURES_ALGORITHM_TRAITS_HAS_ITERATOR_TRAITS_DIFFERENCE_TYPE_H

// #include <futures/algorithm/traits/remove_cvref.h>

// #include <futures/algorithm/traits/has_difference_type.h>
#ifndef FUTURES_ALGORITHM_TRAITS_HAS_DIFFERENCE_TYPE_H
#define FUTURES_ALGORITHM_TRAITS_HAS_DIFFERENCE_TYPE_H

// #include <type_traits>


namespace futures {
    /** \addtogroup algorithms Algorithms
     *  @{
     */

    /** \addtogroup traits Traits
     *  @{
     */

    /** \brief A C++17 type trait equivalent to the C++20 has-member-difference-type
     * concept
     */
#ifdef FUTURES_DOXYGEN
    template <class T>
    using has_difference_type = __see_below__;
#else
    template <class T, class = void>
    struct has_difference_type : std::false_type
    {};

    template <class T>
    struct has_difference_type<T, std::void_t<typename T::value_type>>
        : std::true_type
    {};
#endif
    template <class T>
    bool constexpr has_difference_type_v = has_difference_type<T>::value;
    /** @}*/
    /** @}*/
} // namespace futures

#endif // FUTURES_ALGORITHM_TRAITS_HAS_DIFFERENCE_TYPE_H

// #include <futures/algorithm/traits/is_subtractable.h>
#ifndef FUTURES_ALGORITHM_TRAITS_IS_SUBTRACTABLE_H
#define FUTURES_ALGORITHM_TRAITS_IS_SUBTRACTABLE_H

// #include <type_traits>


namespace futures {
    /** \addtogroup algorithms Algorithms
     *  @{
     */

    /** \addtogroup traits Traits
     *  @{
     */


    /** \brief A C++17 type trait equivalent to the C++20 subtractable
     * concept
     */
#ifdef FUTURES_DOXYGEN
    template <class T>
    using is_subtractable = __see_below__;
#else
    template <class T, class = void>
    struct is_subtractable : std::false_type
    {};

    template <class T>
    struct is_subtractable<
        T,
        std::void_t<
            // clang-format off
            decltype(std::declval<const std::remove_reference_t<T>&>() - std::declval<const std::remove_reference_t<T>&>())
            // clang-format on
            >>
        : std::is_integral<
              decltype(std::declval<const std::remove_reference_t<T>&>() - std::declval<const std::remove_reference_t<T>&>())>
    {};
#endif
    template <class T>
    bool constexpr is_subtractable_v = is_subtractable<T>::value;

    /** @}*/
    /** @}*/

} // namespace futures

#endif // FUTURES_ALGORITHM_TRAITS_IS_SUBTRACTABLE_H

// #include <iterator>

// #include <type_traits>


namespace futures {
    /** \addtogroup algorithms Algorithms
     *  @{
     */

    /** \addtogroup traits Traits
     *  @{
     */


    /** \brief A C++17 type trait equivalent to the C++20 iter_difference
     * concept
     */
#ifdef FUTURES_DOXYGEN
    template <class T>
    using iter_difference = __see_below__;
#else
    template <class T, class = void>
    struct iter_difference
    {};

    template <class T>
    struct iter_difference<
        T,
        std::enable_if_t<
            // clang-format off
            has_iterator_traits_difference_type_v<remove_cvref_t<T>>
            // clang-format on
            >>
    {
        using type = typename std::iterator_traits<
            remove_cvref_t<T>>::difference_type;
    };

    template <class T>
    struct iter_difference<
        T,
        std::enable_if_t<
            // clang-format off
            !has_iterator_traits_difference_type_v<remove_cvref_t<T>> &&
            std::is_pointer_v<T>
            // clang-format on
            >>
    {
        using type = std::ptrdiff_t;
    };

    template <class T>
    struct iter_difference<
        T,
        std::enable_if_t<
            // clang-format off
            !has_iterator_traits_difference_type_v<remove_cvref_t<T>> &&
            !std::is_pointer_v<T> &&
            std::is_const_v<T>
            // clang-format on
            >>
    {
        using type = typename iter_difference<std::remove_const_t<T>>::type;
    };

    template <class T>
    struct iter_difference<
        T,
        std::enable_if_t<
            // clang-format off
            !has_iterator_traits_difference_type_v<remove_cvref_t<T>> &&
            !std::is_pointer_v<T> &&
            !std::is_const_v<T> &&
            has_iterator_traits_difference_type_v<remove_cvref_t<T>>
            // clang-format on
            >>
    {
        using type = typename T::difference_type;
    };

    template <class T>
    struct iter_difference<
        T,
        std::enable_if_t<
            // clang-format off
            !has_iterator_traits_difference_type_v<remove_cvref_t<T>> &&
            !std::is_pointer_v<T> &&
            !std::is_const_v<T> &&
            !has_iterator_traits_difference_type_v<remove_cvref_t<T>> &&
            is_subtractable_v<remove_cvref_t<T>>
            // clang-format on
            >>
    {
        using type = std::make_signed_t<decltype(std::declval<T>() - std::declval<T>())>;
    };
#endif
    template <class T>
    using iter_difference_t = typename iter_difference<T>::type;

    /** @}*/
    /** @}*/
} // namespace futures

#endif // FUTURES_ALGORITHM_TRAITS_ITER_DIFFERENCE_H

// #include <type_traits>


namespace futures {
    /** \addtogroup algorithms Algorithms
     *  @{
     */

    /** \addtogroup traits Traits
     *  @{
     */


    /** \brief A C++17 type trait equivalent to the C++20 weakly_incrementable concept
     */
#ifdef FUTURES_DOXYGEN
    template <class I>
    using is_weakly_incrementable = __see_below__;
#else
    template <class I, class = void>
    struct is_weakly_incrementable : std::false_type
    {};

    template <class I>
    struct is_weakly_incrementable<
        I,
        std::void_t<
            // clang-format off
            decltype(std::declval<I>()++),
            decltype(++std::declval<I>()),
            iter_difference_t<I>
            // clang-format on
            >>
        : std::conjunction<
              // clang-format off
              is_movable<I>,
              std::is_same<decltype(++std::declval<I>()), I&>
              // clang-format on
              >
    {};
#endif
    template <class I>
    bool constexpr is_weakly_incrementable_v = is_weakly_incrementable<I>::value;

    /** @}*/
    /** @}*/

} // namespace futures

#endif // FUTURES_ALGORITHM_TRAITS_IS_WEAKLY_INCREMENTABLE_H

// #include <type_traits>


namespace futures {
    /** \addtogroup algorithms Algorithms
     *  @{
     */

    /** \addtogroup traits Traits
     *  @{
     */


    /** \brief A C++17 type trait equivalent to the C++20 incrementable concept
     */
#ifdef FUTURES_DOXYGEN
    template <class I>
    using is_incrementable = __see_below__;
#else
    template <class I, class = void>
    struct is_incrementable : std::false_type
    {};

    template <class I>
    struct is_incrementable<
        I,
        std::void_t<
            // clang-format off
            decltype(std::declval<I>()++)
            // clang-format on
            >>
        : std::conjunction<
              // clang-format off
              is_regular<I>,
              is_weakly_incrementable<I>,
              std::is_same<decltype(std::declval<I>()++), I>
              // clang-format on
              >
    {};
#endif
    template <class I>
    bool constexpr is_incrementable_v = is_incrementable<I>::value;
    /** @}*/
    /** @}*/

} // namespace futures

#endif // FUTURES_ALGORITHM_TRAITS_IS_INCREMENTABLE_H

// #include <type_traits>


namespace futures {
    /** \addtogroup algorithms Algorithms
     *  @{
     */

    /** \addtogroup traits Traits
     *  @{
     */


    /** \brief A C++17 type trait equivalent to the C++20 forward_iterator
     * concept
     */
#ifdef FUTURES_DOXYGEN
    template <class I>
    using is_forward_iterator = __see_below__;
#else
    template <class I, class = void>
    struct is_forward_iterator : std::false_type
    {};

    template <class I>
    struct is_forward_iterator<
        I,
        std::enable_if_t<
            // clang-format off
            is_input_iterator_v<I> &&
            is_derived_from_v<iter_concept_t<I>, std::forward_iterator_tag> &&
            is_incrementable_v<I> &&
            is_sentinel_for_v<I, I>
            // clang-format on
            >> : std::true_type
    {};
#endif
    template <class I>
    bool constexpr is_forward_iterator_v = is_forward_iterator<I>::value;
    /** @}*/
    /** @}*/

} // namespace futures

#endif // FUTURES_ALGORITHM_TRAITS_IS_FORWARD_ITERATOR_H

// #include <futures/algorithm/traits/is_range.h>

// #include <futures/algorithm/traits/is_sentinel_for.h>

// #include <futures/algorithm/traits/unary_invoke_algorithm.h>
#ifndef FUTURES_ALGORITHM_TRAITS_UNARY_INVOKE_ALGORITHM_H
#define FUTURES_ALGORITHM_TRAITS_UNARY_INVOKE_ALGORITHM_H

/// \file Identify traits for algorithms, like we do for other types
///
/// The traits help us generate auxiliary algorithm overloads
/// This is somewhat similar to the pattern of traits and algorithms for ranges
/// and views It allows us to get algorithm overloads for free, including
/// default inference of the best execution policies
///
/// \see https://en.cppreference.com/w/cpp/ranges/transform_view
/// \see https://en.cppreference.com/w/cpp/ranges/view
///

// #include <futures/algorithm/partitioner/partitioner.h>

// #include <futures/algorithm/policies.h>
#ifndef FUTURES_ALGORITHM_POLICIES_H
#define FUTURES_ALGORITHM_POLICIES_H

/// \file Identify traits for algorithms, like we do for other types
///
/// The traits help us generate auxiliary algorithm overloads
/// This is somewhat similar to the pattern of traits and algorithms for ranges
/// and views It allows us to get algorithm overloads for free, including
/// default inference of the best execution policies
///
/// \see https://en.cppreference.com/w/cpp/ranges/transform_view
/// \see https://en.cppreference.com/w/cpp/ranges/view
///

#include <execution>
// #include <futures/algorithm/partitioner/partitioner.h>

// #include <futures/executor/default_executor.h>

// #include <futures/executor/inline_executor.h>


#ifdef __has_include
#    if __has_include(<version>)
// #include <version>

#    endif
#endif

namespace futures {
    /** \addtogroup algorithms Algorithms
     *  @{
     */

    /** \addtogroup execution-policies Execution Policies
     *  @{
     */

    /// Class representing a type for a sequenced_policy tag
    class sequenced_policy
    {};

    /// Class representing a type for a parallel_policy tag
    class parallel_policy
    {};

    /// Class representing a type for a parallel_unsequenced_policy tag
    class parallel_unsequenced_policy
    {};

    /// Class representing a type for an unsequenced_policy tag
    class unsequenced_policy
    {};

    /// @name Instances of the execution policy types

    /// \brief Tag used in algorithms for a sequenced_policy
    inline constexpr sequenced_policy seq{};

    /// \brief Tag used in algorithms for a parallel_policy
    inline constexpr parallel_policy par{};

    /// \brief Tag used in algorithms for a parallel_unsequenced_policy
    inline constexpr parallel_unsequenced_policy par_unseq{};

    /// \brief Tag used in algorithms for an unsequenced_policy
    inline constexpr unsequenced_policy unseq{};

    /// \brief Checks whether T is a standard or implementation-defined
    /// execution policy type.
    template <class T>
    struct is_execution_policy
        : std::disjunction<
              std::is_same<T, sequenced_policy>,
              std::is_same<T, parallel_policy>,
              std::is_same<T, parallel_unsequenced_policy>,
              std::is_same<T, unsequenced_policy>>
    {};

    /// \brief Checks whether T is a standard or implementation-defined
    /// execution policy type.
    template <class T>
    inline constexpr bool is_execution_policy_v = is_execution_policy<T>::value;

    /// \brief Make an executor appropriate to a given policy and a pair of
    /// iterators This depends, of course, of the default executors we have
    /// available and
    template <
        class E,
        class I,
        class S
#ifndef FUTURES_DOXYGEN
        ,
        std::enable_if_t<
            !is_executor_v<
                E> && is_execution_policy_v<E> && is_input_iterator_v<I> && is_sentinel_for_v<S, I>,
            int> = 0
#endif
        >
    constexpr decltype(auto)
    make_policy_executor() {
        if constexpr (!std::is_same_v<E, sequenced_policy>) {
            return make_default_executor();
        } else {
            return make_inline_executor();
        }
    }

    /** @}*/
    /** @}*/
} // namespace futures

#endif // FUTURES_ALGORITHM_POLICIES_H

// #include <futures/algorithm/traits/is_indirectly_unary_invocable.h>
#ifndef FUTURES_ALGORITHM_TRAITS_IS_INDIRECTLY_UNARY_INVOCABLE_H
#define FUTURES_ALGORITHM_TRAITS_IS_INDIRECTLY_UNARY_INVOCABLE_H

// #include <futures/algorithm/traits/is_indirectly_readable.h>

// #include <futures/algorithm/traits/is_convertible_to.h>

// #include <futures/algorithm/traits/iter_value.h>

// #include <type_traits>


namespace futures {
    /** \addtogroup algorithms Algorithms
     *  @{
     */

    /** \addtogroup traits Traits
     *  @{
     */


    /** \brief A C++17 type trait equivalent to the C++20 indirectly_unary_invocable
     * concept
     */
#ifdef FUTURES_DOXYGEN
    template <class F, class I>
    using is_indirectly_unary_invocable = __see_below__;
#else
    template <class F, class I, class = void>
    struct is_indirectly_unary_invocable : std::false_type
    {};

    template <class F, class I>
    struct is_indirectly_unary_invocable<
        F, I,
        std::enable_if_t<
            // clang-format off
            is_indirectly_readable_v<I> &&
            std::is_copy_constructible_v<F> &&
            std::is_invocable_v<F&, iter_value_t<I>&>
            // clang-format on
            >> : std::true_type
    {};
#endif
    template <class F, class I>
    bool constexpr is_indirectly_unary_invocable_v = is_indirectly_unary_invocable<F, I>::value;
    /** @}*/
    /** @}*/

} // namespace futures

#endif // FUTURES_ALGORITHM_TRAITS_IS_INDIRECTLY_UNARY_INVOCABLE_H

// #include <futures/algorithm/traits/is_input_range.h>

// #include <futures/executor/default_executor.h>

// #include <futures/executor/inline_executor.h>

// #include <execution>


namespace futures {
    /** \addtogroup algorithms Algorithms
     *  @{
     */

    /** \addtogroup algorithm-traits Algorithm Traits
     *  @{
     */

    /// \brief CRTP class with the overloads for classes that look for
    /// elements in a sequence with an unary function This includes
    /// algorithms such as for_each, any_of, all_of, ...
    template <class Derived>
    class unary_invoke_algorithm_functor
    {
    public:
        template <
            class E,
            class P,
            class I,
            class S,
            class Fun
#ifndef FUTURES_DOXYGEN
            ,
            std::enable_if_t<
                // clang-format off
                is_executor_v<E> &&
                is_partitioner_v<P, I, S> &&
                is_input_iterator_v<I> &&
                is_sentinel_for_v<S, I> &&
                is_indirectly_unary_invocable_v<Fun, I> &&
                std::is_copy_constructible_v<Fun>
                // clang-format on
                ,
                int> = 0>
#endif
        decltype(auto)
        operator()(const E &ex, P p, I first, S last, Fun f) const {
            return Derived().run(ex, p, first, last, f);
        }

        /// \overload execution policy instead of executor
        /// we can't however, count on std::is_execution_policy being defined
        template <
            class E,
            class P,
            class I,
            class S,
            class Fun
#ifndef FUTURES_DOXYGEN
            ,
            std::enable_if_t<
                // clang-format off
                !is_executor_v<E> &&
                is_execution_policy_v<E> &&
                is_partitioner_v<P, I, S> &&
                is_input_iterator_v<I> &&
                is_sentinel_for_v<S, I> &&
                is_indirectly_unary_invocable_v<Fun, I> &&
                std::is_copy_constructible_v<Fun>
                // clang-format on
                ,
                int> = 0
#endif
            >
        decltype(auto)
        operator()(const E &, P p, I first, S last, Fun f) const {
            return Derived()
                .run(make_policy_executor<E, I, S>(), p, first, last, f);
        }

        /// \overload Ranges
        template <
            class E,
            class P,
            class R,
            class Fun
#ifndef FUTURES_DOXYGEN
            ,
            std::enable_if_t<
                // clang-format off
                (is_executor_v<E> || is_execution_policy_v<E>) &&
                is_range_partitioner_v<P, R> &&
                is_input_range_v<R> &&
                is_indirectly_unary_invocable_v<Fun, iterator_t<R>> &&
                std::is_copy_constructible_v<Fun>,
                // clang-format on
                int> = 0
#endif
            >
        decltype(auto)
        operator()(const E &ex, P p, R &&r, Fun f) const {
            return operator()(ex, p, std::begin(r), std::end(r), std::move(f));
        }

        /// \overload Iterators / default parallel executor
        template <
            class P,
            class I,
            class S,
            class Fun
#ifndef FUTURES_DOXYGEN
            ,
            std::enable_if_t<
                // clang-format off
                is_partitioner_v<P, I,S> &&
                is_input_iterator_v<I> &&
                is_sentinel_for_v<S, I> &&
                is_indirectly_unary_invocable_v<Fun, I> &&
                std::is_copy_constructible_v<Fun>,
                // clang-format off
                int> = 0
#endif
            >
        decltype(auto)
        operator()(P p, I first, S last, Fun f) const {
            return Derived().
            run(make_default_executor(), p, first, last, std::move(f));
        }

        /// \overload Ranges / default parallel executor
        template <
            class P,
            class R,
            class Fun
#ifndef FUTURES_DOXYGEN
            ,
            std::enable_if_t<
                // clang-format off
                is_range_partitioner_v<P,R> &&
                is_input_range_v<R> &&
                is_indirectly_unary_invocable_v<Fun, iterator_t<R>> &&
                std::is_copy_constructible_v<Fun>,
                // clang-format on
                int> = 0
#endif
            >
        decltype(auto)
        operator()(P p, R &&r, Fun f) const {
            return Derived().operator()(
                make_default_executor(),
                p,
                std::begin(r),
                std::end(r),
                std::move(f));
        }

        /// \overload Iterators / default partitioner
        template <
            class E,
            class I,
            class S,
            class Fun
#ifndef FUTURES_DOXYGEN
            ,
            std::enable_if_t<
                // clang-format off
                (is_executor_v<E> || is_execution_policy_v<E>) &&
                is_input_iterator_v<I> &&
                is_sentinel_for_v<S, I> &&
                is_indirectly_unary_invocable_v<Fun, I> &&
                std::is_copy_constructible_v<Fun>,
                // clang-format on
                int> = 0
#endif
            >
        decltype(auto)
        operator()(const E &ex, I first, S last, Fun f) const {
            return Derived().operator()(
                ex,
                make_default_partitioner(first, last),
                first,
                last,
                std::move(f));
        }

        /// \overload Ranges / default partitioner
        template <
            class E,
            class R,
            class Fun
#ifndef FUTURES_DOXYGEN
            ,
            std::enable_if_t<
                // clang-format off
                (is_executor_v<E> || is_execution_policy_v<E>) &&
                is_input_range_v<R> &&
                is_indirectly_unary_invocable_v<Fun, iterator_t<R>> &&
                std::is_copy_constructible_v<Fun>,
                // clang-format on
                int> = 0
#endif
            >
        decltype(auto)
        operator()(const E &ex, R &&r, Fun f) const {
            return Derived().operator()(
                ex,
                make_default_partitioner(std::forward<R>(r)),
                std::begin(r),
                std::end(r),
                std::move(f));
        }

        /// \overload Iterators / default executor / default partitioner
        template <
            class I,
            class S,
            class Fun
#ifndef FUTURES_DOXYGEN
            ,
            std::enable_if_t<
                // clang-format off
                is_input_iterator_v<I> &&
                is_sentinel_for_v<S, I> &&
                is_indirectly_unary_invocable_v<Fun, I> &&
                std::is_copy_constructible_v<Fun>,
                // clang-format on
                int> = 0
#endif
            >
        decltype(auto)
        operator()(I first, S last, Fun f) const {
            return Derived().operator()(
                make_default_executor(),
                make_default_partitioner(first, last),
                first,
                last,
                std::move(f));
        }

        /// \overload Ranges / default executor / default partitioner
        template <
            class R,
            class Fun
#ifndef FUTURES_DOXYGEN
            ,
            std::enable_if_t<
                // clang-format off
                is_input_range_v<R> &&
                is_indirectly_unary_invocable_v<Fun, iterator_t<R>> &&
                std::is_copy_constructible_v<Fun>,
                // clang-format on
                int> = 0
#endif
            >
        decltype(auto)
        operator()(R &&r, Fun f) const {
            return Derived().operator()(
                make_default_executor(),
                make_default_partitioner(r),
                std::begin(r),
                std::end(r),
                std::move(f));
        }
    };

    /** @}*/
    /** @}*/
} // namespace futures

#endif // FUTURES_ALGORITHM_TRAITS_UNARY_INVOKE_ALGORITHM_H

// #include <futures/futures.h>

// #include <futures/algorithm/detail/try_async.h>
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
    /// This is mostly useful for recursive tasks, where there might not be room
    /// in the executor for a new task, as depending on recursive tasks for
    /// which there is no room is the executor might block execution.
    ///
    /// Although this is a general solution to allow any executor in the
    /// algorithms, executor traits to identify capacity in executor are much
    /// more desirable.
    ///
    template <
        typename Executor,
        typename Function,
        typename... Args
#ifndef FUTURES_DOXYGEN
        ,
        std::enable_if_t<
            detail::is_valid_async_input_v<Executor, Function, Args...>,
            int> = 0
#endif
        >
    decltype(auto)
    try_async(const Executor &ex, Function &&f, Args &&...args) {
        // Communication flags
        std::promise<void> started_token;
        std::future<void> started = started_token.get_future();
        stop_source cancel_source;

        // Wrap the task in a lambda that sets and checks the flags
        auto do_task =
            [p = std::move(started_token),
             cancel_token = cancel_source.get_token(),
             f](Args &&...args) mutable {
            p.set_value();
            if (cancel_token.stop_requested()) {
                detail::throw_exception<std::runtime_error>("task cancelled");
            }
            return std::invoke(f, std::forward<Args>(args)...);
        };

        // Make it copy constructable
        auto do_task_ptr = std::make_shared<decltype(do_task)>(
            std::move(do_task));
        auto do_task_handle = [do_task_ptr](Args &&...args) {
            return (*do_task_ptr)(std::forward<Args>(args)...);
        };

        // Launch async
        using internal_result_type = std::decay_t<
            decltype(std::invoke(f, std::forward<Args>(args)...))>;
        cfuture<internal_result_type>
            rhs = async(ex, do_task_handle, std::forward<Args>(args)...);

        // Return future and tokens
        return std::
            make_tuple(std::move(rhs), std::move(started), cancel_source);
    }


    /** @} */
} // namespace futures


#endif // FUTURES_TRY_ASYNC_H

// #include <execution>

#include <variant>

namespace futures {
    /** \addtogroup algorithms Algorithms
     *  @{
     */

    /** \addtogroup functions Functions
     *  @{
     */

    /// \brief Functor representing the overloads for the @ref all_of function
    class all_of_functor : public unary_invoke_algorithm_functor<all_of_functor>
    {
        friend unary_invoke_algorithm_functor<all_of_functor>;

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
        template <
            class E,
            class P,
            class I,
            class S,
            class Fun
#ifndef FUTURES_DOXYGEN
            ,
            std::enable_if_t<
                // clang-format off
                is_executor_v<E> &&
                is_partitioner_v<P, I, S> &&
                is_input_iterator_v<I> &&
                is_sentinel_for_v<S, I> &&
                is_indirectly_unary_invocable_v<Fun, I> &&
                std::is_copy_constructible_v<Fun>
                // clang-format on
                ,
                int> = 0
#endif
            >
        bool
        run(const E &ex, P p, I first, S last, Fun f) const {
            auto middle = p(first, last);
            if (bool always_sequential
                = std::is_same_v<E, inline_executor> || is_forward_iterator_v<I>;
                always_sequential || middle == last)
            {
                return std::all_of(first, last, f);
            }

            // Run all_of on rhs: [middle, last]
            auto [rhs, rhs_started, rhs_cancel]
                = try_async(ex, [ex, p, middle, last, f, this]() {
                      return operator()(ex, p, middle, last, f);
                  });

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
                    return
                    operator()(make_inline_executor(), p, middle, last, f);
                }
            }
        }
    };

    /// \brief Checks if a predicate is true for all the elements in a range
    inline constexpr all_of_functor all_of;

    /** @}*/
    /** @}*/
} // namespace futures

#endif // FUTURES_ALL_OF_H

// #include <futures/algorithm/any_of.h>
#ifndef FUTURES_ANY_OF_H
#define FUTURES_ANY_OF_H

// #include <futures/algorithm/partitioner/partitioner.h>

// #include <futures/algorithm/traits/is_forward_iterator.h>

// #include <futures/algorithm/traits/unary_invoke_algorithm.h>

// #include <futures/futures.h>

// #include <futures/algorithm/detail/try_async.h>

// #include <execution>

// #include <variant>


namespace futures {
    /** \addtogroup algorithms Algorithms
     *  @{
     */
    /** \addtogroup functions Functions
     *  @{
     */

    /// \brief Functor representing the overloads for the @ref any_of function
    class any_of_functor : public unary_invoke_algorithm_functor<any_of_functor>
    {
        friend unary_invoke_algorithm_functor<any_of_functor>;

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
        template <
            class E,
            class P,
            class I,
            class S,
            class Fun
#ifndef FUTURES_DOXYGEN
            ,
            std::enable_if_t<
                // clang-format off
                is_executor_v<E> &&
                is_partitioner_v<P, I, S> &&
                is_input_iterator_v<I> &&
                is_sentinel_for_v<S, I> &&
                is_indirectly_unary_invocable_v<Fun, I> &&
                std::is_copy_constructible_v<Fun>,
                // clang-format on
                int> = 0
#endif
            >
        bool
        run(const E &ex, P p, I first, S last, Fun f) const {
            auto middle = p(first, last);
            if (bool is_always_sequential
                = std::is_same_v<E, inline_executor> || is_forward_iterator_v<I>;
                is_always_sequential || middle == last)
            {
                return std::any_of(first, last, f);
            }

            // Run any_of on rhs: [middle, last]
            auto [rhs, rhs_started, rhs_cancel]
                = try_async(ex, [ex, p, middle, last, f, this]() {
                      return operator()(ex, p, middle, last, f);
                  });

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
                    return
                    operator()(make_inline_executor(), p, middle, last, f);
                }
            }
        }
    };

    /// \brief Checks if a predicate is true for any of the elements in a range
    inline constexpr any_of_functor any_of;

    /** @}*/
    /** @}*/
} // namespace futures

#endif // FUTURES_ANY_OF_H

// #include <futures/algorithm/count.h>
#ifndef FUTURES_COUNT_H
#define FUTURES_COUNT_H

// #include <futures/algorithm/comparisons/equal_to.h>
#ifndef FUTURES_ALGORITHM_COMPARISONS_EQUAL_TO_H
#define FUTURES_ALGORITHM_COMPARISONS_EQUAL_TO_H

// #include <futures/algorithm/traits/is_equality_comparable_with.h>
#ifndef FUTURES_ALGORITHM_TRAITS_IS_EQUALITY_COMPARABLE_WITH_H
#define FUTURES_ALGORITHM_TRAITS_IS_EQUALITY_COMPARABLE_WITH_H

// #include <futures/algorithm/traits/is_equality_comparable.h>

// #include <futures/algorithm/traits/is_weakly_equality_comparable.h>
#ifndef FUTURES_ALGORITHM_TRAITS_IS_WEAKLY_EQUALITY_COMPARABLE_H
#define FUTURES_ALGORITHM_TRAITS_IS_WEAKLY_EQUALITY_COMPARABLE_H

// #include <type_traits>


namespace futures {
    /** \addtogroup algorithms Algorithms
     *  @{
     */

    /** \addtogroup traits Traits
     *  @{
     */


    /** \brief A C++17 type trait equivalent to the C++20 weakly_equality_comparable
     * concept
     */
#ifdef FUTURES_DOXYGEN
    template <class T, class U>
    using is_weakly_equality_comparable = __see_below__;
#else
    template <class T, class U, class = void>
    struct is_weakly_equality_comparable : std::false_type
    {};

    template <class T, class U>
    struct is_weakly_equality_comparable<
        T,
        U,
        std::void_t<
            // clang-format off
            decltype(std::declval<const std::remove_reference_t<T>&>() == std::declval<const std::remove_reference_t<U>&>()),
            decltype(std::declval<const std::remove_reference_t<T>&>() != std::declval<const std::remove_reference_t<U>&>()),
            decltype(std::declval<const std::remove_reference_t<U>&>() == std::declval<const std::remove_reference_t<T>&>()),
            decltype(std::declval<const std::remove_reference_t<U>&>() != std::declval<const std::remove_reference_t<T>&>())
            // clang-format on
            >> : std::true_type
    {};
#endif
    template <class T, class U>
    bool constexpr is_weakly_equality_comparable_v
        = is_weakly_equality_comparable<T, U>::value;

    /** @}*/
    /** @}*/

} // namespace futures

#endif // FUTURES_ALGORITHM_TRAITS_IS_WEAKLY_EQUALITY_COMPARABLE_H

// #include <type_traits>


namespace futures {
    /** \addtogroup algorithms Algorithms
     *  @{
     */

    /** \addtogroup traits Traits
     *  @{
     */


    /** \brief A C++17 type trait equivalent to the C++20 equality_comparable_with
     * concept
     */
#ifdef FUTURES_DOXYGEN
    template <class T, class U>
    using is_equality_comparable_with = __see_below__;
#else
    template <class T, class U, class = void>
    struct is_equality_comparable_with : std::false_type
    {};

    template <class T, class U>
    struct is_equality_comparable_with<
        T,
        U,
        std::enable_if_t<
            // clang-format off
            is_equality_comparable_v<T> &&
            is_equality_comparable_v<U> &&
            is_weakly_equality_comparable_v<T,U>
            // clang-format on
            >> : std::true_type
    {};
#endif
    template <class T, class U>
    bool constexpr is_equality_comparable_with_v
        = is_equality_comparable_with<T, U>::value;
    /** @}*/
    /** @}*/

} // namespace futures

#endif // FUTURES_ALGORITHM_TRAITS_IS_EQUALITY_COMPARABLE_WITH_H

// #include <utility>

// #include <type_traits>


namespace futures {
    /** A C++17 functor equivalent to the C++20 std::ranges::equal_to
     */
    struct equal_to
    {
        template <
            typename T,
            typename U
#ifndef FUTURES_DOXYGEN
            ,
            std::enable_if_t<is_equality_comparable_with_v<T, U>, int> = 0
#endif
            >
        constexpr bool
        operator()(T &&t, U &&u) const {
            return std::forward<T>(t) == std::forward<U>(u);
        }
        using is_transparent = void;
    };

} // namespace futures

#endif // FUTURES_ALGORITHM_COMPARISONS_EQUAL_TO_H

// #include <futures/algorithm/partitioner/partitioner.h>

// #include <futures/algorithm/traits/is_forward_iterator.h>

// #include <futures/algorithm/traits/is_indirectly_binary_invocable.h>
#ifndef FUTURES_ALGORITHM_TRAITS_IS_INDIRECTLY_BINARY_INVOCABLE_H
#define FUTURES_ALGORITHM_TRAITS_IS_INDIRECTLY_BINARY_INVOCABLE_H

// #include <futures/algorithm/traits/is_indirectly_readable.h>

// #include <futures/algorithm/traits/is_convertible_to.h>

// #include <futures/algorithm/traits/iter_value.h>

// #include <type_traits>


namespace futures {
    /** \addtogroup algorithms Algorithms
     *  @{
     */

    /** \addtogroup traits Traits
     *  @{
     */


    /** \brief A C++17 type trait equivalent to the C++20 indirectly_binary_invocable
     * concept
     */
#ifdef FUTURES_DOXYGEN
    template <class F, class I1, class I2>
    using is_indirectly_binary_invocable = __see_below__;
#else
    template <class F, class I1, class I2, class = void>
    struct is_indirectly_binary_invocable : std::false_type
    {};

    template <class F, class I1, class I2>
    struct is_indirectly_binary_invocable<
        F, I1, I2,
        std::enable_if_t<
            // clang-format off
            is_indirectly_readable_v<I1> &&
            is_indirectly_readable_v<I2> &&
            std::is_copy_constructible_v<F> &&
            std::is_invocable_v<F&, iter_value_t<I1>&, iter_value_t<I2>&>
            // clang-format on
            >> : std::true_type
    {};
#endif
    template <class F, class I1, class I2>
    bool constexpr is_indirectly_binary_invocable_v = is_indirectly_binary_invocable<F, I1, I2>::value;
    /** @}*/
    /** @}*/

} // namespace futures

#endif // FUTURES_ALGORITHM_TRAITS_IS_INDIRECTLY_BINARY_INVOCABLE_H

// #include <futures/algorithm/traits/iter_difference.h>

// #include <futures/algorithm/traits/value_cmp_algorithm.h>
#ifndef FUTURES_ALGORITHM_VALUE_CMP_ALGORITHM_H
#define FUTURES_ALGORITHM_VALUE_CMP_ALGORITHM_H

/// \file Identify traits for algorithms, like we do for other types
///
/// The traits help us generate auxiliary algorithm overloads
/// This is somewhat similar to the pattern of traits and algorithms for ranges
/// and views It allows us to get algorithm overloads for free, including
/// default inference of the best execution policies
///
/// \see https://en.cppreference.com/w/cpp/ranges/transform_view
/// \see https://en.cppreference.com/w/cpp/ranges/view
///

// #include <futures/algorithm/comparisons/equal_to.h>

// #include <futures/algorithm/partitioner/partitioner.h>

// #include <futures/algorithm/policies.h>

// #include <futures/algorithm/traits/is_indirectly_binary_invocable.h>

// #include <futures/executor/default_executor.h>

// #include <futures/executor/inline_executor.h>

// #include <execution>


namespace futures {
    /** \addtogroup algorithms Algorithms
     *  @{
     */

    /** \addtogroup algorithm-traits Algorithm Traits
     *  @{
     */

    /// \brief CRTP class with the overloads for classes that look for
    /// elements in a sequence with an unary function This includes
    /// algorithms such as for_each, any_of, all_of, ...
    template <class Derived>
    class value_cmp_algorithm_functor
    {
    public:
        template <
            class E,
            class P,
            class I,
            class S,
            class T
#ifndef FUTURES_DOXYGEN
            ,
            std::enable_if_t<
                // clang-format off
                is_executor_v<E> &&
                is_partitioner_v<P, I, S> &&
                is_input_iterator_v<I> &&
                is_sentinel_for_v<S, I> &&
                is_indirectly_binary_invocable_v<equal_to, T *, I>,
                // clang-format on
                int> = 0
#endif
            >
        decltype(auto)
        operator()(const E &ex, P p, I first, S last, T f) const {
            return Derived().run(ex, p, first, last, f);
        }

        /// \overload execution policy instead of executor
        /// we can't however, count on std::is_execution_policy being defined
        template <
            class E,
            class P,
            class I,
            class S,
            class T
#ifndef FUTURES_DOXYGEN
            ,
            std::enable_if_t<
                // clang-format off
                !is_executor_v<E> &&
                is_execution_policy_v<E> &&
                is_partitioner_v<P, I, S> &&
                is_input_iterator_v<I> &&
                is_sentinel_for_v<S, I> &&
                is_indirectly_binary_invocable_v<equal_to, T *, I>,
                // clang-format on
                int> = 0
#endif
            >
        decltype(auto)
        operator()(const E &, P p, I first, S last, T f) const {
            return Derived()
                .run(make_policy_executor<E, I, S>(), p, first, last, f);
        }

        /// \overload Ranges
        template <
            class E,
            class P,
            class R,
            class T
#ifndef FUTURES_DOXYGEN
            ,
            std::enable_if_t<
                // clang-format off
                (is_executor_v<E> || is_execution_policy_v<E>) &&
                is_range_partitioner_v<P, R> &&
                is_input_range_v<R> &&
                is_indirectly_binary_invocable_v<equal_to, T *, iterator_t<R>> &&
                std::is_copy_constructible_v<T>,
                // clang-format on
                int> = 0
#endif
            >
        decltype(auto)
        operator()(const E &ex, P p, R &&r, T f) const {
            return Derived()
                .run(ex, p, std::begin(r), std::end(r), std::move(f));
        }

        /// \overload Iterators / default parallel executor
        template <
            class P,
            class I,
            class S,
            class T
#ifndef FUTURES_DOXYGEN
            ,
            std::enable_if_t<
                // clang-format off
                is_partitioner_v<P, I, S> &&
                is_input_iterator_v<I> &&
                is_sentinel_for_v<S, I> &&
                is_indirectly_binary_invocable_v<equal_to, T *, I> &&
                std::is_copy_constructible_v<T>,
                // clang-format on
                int> = 0
#endif
            >
        decltype(auto)
        operator()(P p, I first, S last, T f) const {
            return Derived()
                .run(make_default_executor(), p, first, last, std::move(f));
        }

        /// \overload Ranges / default parallel executor
        template <
            class P,
            class R,
            class T
#ifndef FUTURES_DOXYGEN
            ,
            std::enable_if_t<
                // clang-format off
                is_range_partitioner_v<P, R> &&
                is_input_range_v<R> &&
                is_indirectly_binary_invocable_v<equal_to, T *, iterator_t<R>> &&
                std::is_copy_constructible_v<T>,
                // clang-format on
                int> = 0
#endif
            >
        decltype(auto)
        operator()(P p, R &&r, T f) const {
            return Derived().run(
                make_default_executor(),
                p,
                std::begin(r),
                std::end(r),
                std::move(f));
        }

        /// \overload Iterators / default partitioner
        template <
            class E,
            class I,
            class S,
            class T
#ifndef FUTURES_DOXYGEN
            ,
            std::enable_if_t<
                // clang-format off
                (is_executor_v<E> || is_execution_policy_v<E>) &&
                is_input_iterator_v<I> &&
                is_sentinel_for_v<S, I> &&
                is_indirectly_binary_invocable_v<equal_to, T *, I>,
                // clang-format on
                int> = 0
#endif
            >
        decltype(auto)
        operator()(const E &ex, I first, S last, T f) const {
            return operator()(
                ex,
                make_default_partitioner(first, last),
                first,
                last,
                std::move(f));
        }

        /// \overload Ranges / default partitioner
        template <
            class E,
            class R,
            class T
#ifndef FUTURES_DOXYGEN
            ,
            std::enable_if_t<
                // clang-format off
                (is_executor_v<E> || is_execution_policy_v<E>) &&
                is_input_range_v<R> &&
                is_indirectly_binary_invocable_v<equal_to, T *, iterator_t<R>> &&
                std::is_copy_constructible_v<T>,
                // clang-format on
                int> = 0
#endif
            >
        decltype(auto)
        operator()(const E &ex, R &&r, T f) const {
            return operator()(
                ex,
                make_default_partitioner(std::forward<R>(r)),
                std::begin(r),
                std::end(r),
                std::move(f));
        }

        /// \overload Iterators / default executor / default partitioner
        template <
            class I,
            class S,
            class T
#ifndef FUTURES_DOXYGEN
            ,
            std::enable_if_t<
                // clang-format off
                is_input_iterator_v<
                    I> &&
                is_sentinel_for_v<S, I> &&
                is_indirectly_binary_invocable_v<equal_to, T *, I>,
                // clang-format on
                int> = 0
#endif
            >
        decltype(auto)
        operator()(I first, S last, T f) const {
            return Derived().run(
                make_default_executor(),
                make_default_partitioner(first, last),
                first,
                last,
                std::move(f));
        }

        /// \overload Ranges / default executor / default partitioner
        template <
            class R,
            class T
#ifndef FUTURES_DOXYGEN
            ,
            std::enable_if_t<
                // clang-format off
                is_input_range_v<R> &&
                is_indirectly_binary_invocable_v<equal_to, T *, iterator_t<R>> &&
                std::is_copy_constructible_v<T>,
                // clang-format on
                int> = 0
#endif
            >
        decltype(auto)
        operator()(R &&r, T f) const {
            return Derived().run(
                make_default_executor(),
                make_default_partitioner(r),
                std::begin(r),
                std::end(r),
                std::move(f));
        }
    };
    /** @}*/
    /** @}*/
} // namespace futures

#endif // FUTURES_ALGORITHM_VALUE_CMP_ALGORITHM_H

// #include <futures/futures.h>

// #include <futures/algorithm/detail/try_async.h>

// #include <execution>

// #include <variant>


namespace futures {
    /** \addtogroup algorithms Algorithms
     *  @{
     */

    /** \addtogroup functions Functions
     *  @{
     */


    /// \brief Functor representing the overloads for the @ref count function
    class count_functor : public value_cmp_algorithm_functor<count_functor>
    {
        friend value_cmp_algorithm_functor<count_functor>;

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
        template <
            class E,
            class P,
            class I,
            class S,
            class T
#ifndef FUTURES_DOXYGEN
            ,
            std::enable_if_t<
                // clang-format off
                is_executor_v<E> &&
                is_partitioner_v<P, I, S> &&
                is_input_iterator_v<I> &&
                is_sentinel_for_v<S, I> &&
                is_indirectly_binary_invocable_v<equal_to, T *, I>
                // clang-format on
                ,
                int> = 0
#endif
            >
        iter_difference_t<I>
        run(const E &ex, P p, I first, S last, T v) const {
            auto middle = p(first, last);
            if (middle == last
                || std::
                    is_same_v<E, inline_executor> || is_forward_iterator_v<I>)
            {
                return std::count(first, last, v);
            }

            // Run count on rhs: [middle, last]
            auto [rhs, rhs_started, rhs_cancel]
                = try_async(ex, [ex, p, middle, last, v, this]() {
                      return operator()(ex, p, middle, last, v);
                  });

            // Run count on lhs: [first, middle]
            auto lhs = operator()(ex, p, first, middle, v);

            // Wait for rhs
            if (is_ready(rhs_started)) {
                return lhs + rhs.get();
            } else {
                rhs_cancel.request_stop();
                rhs.detach();
                return lhs
                       + operator()(make_inline_executor(), p, middle, last, v);
            }
        }
    };

    /// \brief Returns the number of elements matching an element
    inline constexpr count_functor count;

    /** @}*/
    /** @}*/
} // namespace futures

#endif // FUTURES_COUNT_H

// #include <futures/algorithm/count_if.h>
#ifndef FUTURES_COUNT_IF_H
#define FUTURES_COUNT_IF_H

// #include <futures/algorithm/partitioner/partitioner.h>

// #include <futures/algorithm/traits/unary_invoke_algorithm.h>

// #include <futures/futures.h>

// #include <futures/algorithm/detail/try_async.h>

// #include <execution>

// #include <variant>


namespace futures {
    /** \addtogroup algorithms Algorithms
     *  @{
     */

    /** \addtogroup functions Functions
     *  @{
     */

    /// \brief Functor representing the overloads for the @ref count_if function
    class count_if_functor
        : public unary_invoke_algorithm_functor<count_if_functor>
    {
        friend unary_invoke_algorithm_functor<count_if_functor>;

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
        template <
            class E,
            class P,
            class I,
            class S,
            class Fun
#ifndef FUTURES_DOXYGEN
            ,
            std::enable_if_t<
                // clang-format off
                is_executor_v<E> &&
                is_partitioner_v<P, I, S> &&
                is_input_iterator_v<I> &&
                is_sentinel_for_v<S, I> &&
                is_indirectly_unary_invocable_v<Fun, I> &&
                std::is_copy_constructible_v<Fun>
                // clang-format on
                ,
                int> = 0
#endif
            >
        iter_difference_t<I>
        run(const E &ex, P p, I first, S last, Fun f) const {
            auto middle = p(first, last);
            if (bool always_sequential
                = std::is_same_v<E, inline_executor> || is_forward_iterator_v<I>;
                always_sequential || middle == last)
            {
                return std::count_if(first, last, f);
            }

            // Run count_if on rhs: [middle, last]
            auto [rhs, rhs_started, rhs_cancel]
                = try_async(ex, [ex, p, middle, last, f, this]() {
                      return operator()(ex, p, middle, last, f);
                  });

            // Run count_if on lhs: [first, middle]
            auto lhs = operator()(ex, p, first, middle, f);

            // Wait for rhs
            if (is_ready(rhs_started)) {
                return lhs + rhs.get();
            } else {
                rhs_cancel.request_stop();
                rhs.detach();
                return lhs
                       + operator()(make_inline_executor(), p, middle, last, f);
            }
        }
    };

    /// \brief Returns the number of elements satisfying specific criteria
    inline constexpr count_if_functor count_if;

    /** @}*/
    /** @}*/
} // namespace futures

#endif // FUTURES_COUNT_IF_H

// #include <futures/algorithm/find.h>
#ifndef FUTURES_FIND_H
#define FUTURES_FIND_H

// #include <futures/algorithm/comparisons/equal_to.h>

// #include <futures/algorithm/partitioner/partitioner.h>

// #include <futures/algorithm/traits/is_indirectly_binary_invocable.h>

// #include <futures/algorithm/traits/value_cmp_algorithm.h>

// #include <futures/futures.h>

// #include <futures/algorithm/detail/try_async.h>

// #include <execution>

// #include <variant>


namespace futures {
    /** \addtogroup algorithms Algorithms
     *  @{
     */

    /** \addtogroup functions Functions
     *  @{
     */

    /// \brief Functor representing the overloads for the @ref find function
    class find_functor : public value_cmp_algorithm_functor<find_functor>
    {
        friend value_cmp_algorithm_functor<find_functor>;

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
        template <
            class E,
            class P,
            class I,
            class S,
            class T
#ifndef FUTURES_DOXYGEN
            ,
            std::enable_if_t<
                // clang-format off
                is_executor_v<E> &&
                is_partitioner_v<P, I, S> &&
                is_input_iterator_v<I> &&
                is_sentinel_for_v<S, I> &&
                is_indirectly_binary_invocable_v<equal_to, T *, I>
                // clang-format on
                ,
                int> = 0
#endif
            >
        I
        run(const E &ex, P p, I first, S last, const T &v) const {
            auto middle = p(first, last);
            if (middle == last
                || std::is_same_v<
                    E,
                    inline_executor> || is_forward_iterator_v<I>)
            {
                return std::find(first, last, v);
            }

            // Run find on rhs: [middle, last]
            auto [rhs, rhs_started, rhs_cancel]
                = try_async(ex, [ex, p, middle, last, v, this]() {
                      return operator()(ex, p, middle, last, v);
                  });

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
                    return
                    operator()(make_inline_executor(), p, middle, last, v);
                }
            }
        }
    };

    /// \brief Finds the first element equal to another element
    inline constexpr find_functor find;

    /** @}*/
    /** @}*/
} // namespace futures

#endif // FUTURES_FIND_H

// #include <futures/algorithm/find_if.h>
#ifndef FUTURES_FIND_IF_H
#define FUTURES_FIND_IF_H

// #include <futures/algorithm/partitioner/partitioner.h>

// #include <futures/algorithm/traits/unary_invoke_algorithm.h>

// #include <futures/futures.h>

// #include <futures/algorithm/detail/try_async.h>

// #include <execution>

// #include <variant>


namespace futures {
    /** \addtogroup algorithms Algorithms
     *  @{
     */

    /** \addtogroup functions Functions
     *  @{
     */

    /// \brief Functor representing the overloads for the @ref find_if function
    class find_if_functor
        : public unary_invoke_algorithm_functor<find_if_functor>
    {
        friend unary_invoke_algorithm_functor<find_if_functor>;

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
        template <
            class E,
            class P,
            class I,
            class S,
            class Fun
#ifndef FUTURES_DOXYGEN
            ,
            std::enable_if_t<
                // clang-format off
                is_executor_v<E> &&
                is_partitioner_v<P, I, S> &&
                is_input_iterator_v<I> &&
                is_sentinel_for_v<S, I> &&
                is_indirectly_unary_invocable_v<Fun, I> &&
                std::is_copy_constructible_v<Fun>
                // clang-format on
                ,
                int> = 0
#endif
            >
        I
        run(const E &ex, P p, I first, S last, Fun f) const {
            auto middle = p(first, last);
            if (middle == last
                || std::is_same_v<
                    E,
                    inline_executor> || is_forward_iterator_v<I>)
            {
                return std::find_if(first, last, f);
            }

            // Run find_if on rhs: [middle, last]
            auto [rhs, rhs_started, rhs_cancel]
                = try_async(ex, [ex, p, middle, last, f, this]() {
                      return operator()(ex, p, middle, last, f);
                  });

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
                    return
                    operator()(make_inline_executor(), p, middle, last, f);
                }
            }
        }
    };

    /// \brief Finds the first element satisfying specific criteria
    inline constexpr find_if_functor find_if;

    /** @}*/
    /** @}*/
} // namespace futures

#endif // FUTURES_FIND_IF_H

// #include <futures/algorithm/find_if_not.h>
#ifndef FUTURES_FIND_IF_NOT_H
#define FUTURES_FIND_IF_NOT_H

// #include <futures/algorithm/partitioner/partitioner.h>

// #include <futures/algorithm/traits/unary_invoke_algorithm.h>

// #include <futures/futures.h>

// #include <futures/algorithm/detail/try_async.h>

// #include <execution>

// #include <variant>


namespace futures {
    /** \addtogroup algorithms Algorithms
     *  @{
     */
    /** \addtogroup functions Functions
     *  @{
     */


    /// \brief Functor representing the overloads for the @ref find_if_not
    /// function
    class find_if_not_functor
        : public unary_invoke_algorithm_functor<find_if_not_functor>
    {
        friend unary_invoke_algorithm_functor<find_if_not_functor>;

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
        template <
            class E,
            class P,
            class I,
            class S,
            class Fun
#ifndef FUTURES_DOXYGEN
            ,
            std::enable_if_t<
                // clang-format off
                is_executor_v<E> &&
                is_partitioner_v<P, I, S> &&
                is_input_iterator_v<I> &&
                is_sentinel_for_v<S, I> &&
                is_indirectly_unary_invocable_v<Fun, I> &&
                std::is_copy_constructible_v<Fun>
                // clang-format on
                ,
                int> = 0
#endif
            >
        I
        run(const E &ex, P p, I first, S last, Fun f) const {
            auto middle = p(first, last);
            if (middle == last
                || std::is_same_v<
                    E,
                    inline_executor> || is_forward_iterator_v<I>)
            {
                return std::find_if_not(first, last, f);
            }

            // Run find_if_not on rhs: [middle, last]
            auto [rhs, rhs_started, rhs_cancel]
                = try_async(ex, [ex, p, middle, last, f, this]() {
                      return operator()(ex, p, middle, last, f);
                  });

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
                    return
                    operator()(make_inline_executor(), p, middle, last, f);
                }
            }
        }
    };

    /// \brief Finds the first element not satisfying specific criteria
    inline constexpr find_if_not_functor find_if_not;

    /** @}*/
    /** @}*/
} // namespace futures

#endif // FUTURES_FIND_IF_NOT_H

// #include <futures/algorithm/for_each.h>
#ifndef FUTURES_FOR_EACH_H
#define FUTURES_FOR_EACH_H

// #include <futures/algorithm/partitioner/partitioner.h>

// #include <futures/algorithm/traits/unary_invoke_algorithm.h>

// #include <futures/futures.h>

// #include <futures/algorithm/detail/try_async.h>

// #include <futures/futures/detail/empty_base.h>

// #include <execution>

// #include <variant>


namespace futures {
    /** \addtogroup algorithms Algorithms
     *  @{
     */
    /** \addtogroup functions Functions
     *  @{
     */

    /// \brief Functor representing the overloads for the @ref for_each function
    class for_each_functor
        : public unary_invoke_algorithm_functor<for_each_functor>
    {
        friend unary_invoke_algorithm_functor<for_each_functor>;

        /// \brief Internal class that takes care of the sorting tasks and its
        /// incomplete tasks
        ///
        /// If we could make sure no executors would ever block, recursion
        /// wouldn't be a problem, and we wouldn't need this class. In fact,
        /// this is what most related libraries do, counting on the executor to
        /// be some kind of work stealing thread pool.
        ///
        /// However, we cannot count on that, or these algorithms wouldn't work
        /// for many executors in which we are interested, such as an io_context
        /// or a thread pool that doesn't steel work (like asio's). So we need
        /// to separate the process of launching the tasks from the process of
        /// waiting for them. Fortunately, we can count that most executors
        /// wouldn't need this blocking procedure very often, because that's
        /// what usually make them useful executors. We also assume that, unlike
        /// in the other applications, the cost of this reading lock is trivial
        /// compared to the cost of the whole procedure.
        ///
        template <class Executor>
        class sorter : public detail::maybe_empty<Executor>
        {
        public:
            explicit sorter(const Executor &ex)
                : detail::maybe_empty<Executor>(ex) {}

            /// \brief Get executor from the base class as a function for
            /// convenience
            const Executor &
            ex() const {
                return detail::maybe_empty<Executor>::get();
            }

            /// \brief Get executor from the base class as a function for
            /// convenience
            Executor &
            ex() {
                return detail::maybe_empty<Executor>::get();
            }

            template <class P, class I, class S, class Fun>
            void
            launch_sort_tasks(P p, I first, S last, Fun f) {
                auto middle = p(first, last);
                const bool too_small = middle == last;
                constexpr bool cannot_parallelize
                    = std::is_same_v<
                          Executor,
                          inline_executor> || is_forward_iterator_v<I>;
                if (too_small || cannot_parallelize) {
                    std::for_each(first, last, f);
                } else {
                    // Run for_each on rhs: [middle, last]
                    cfuture<void> rhs_task = futures::
                        async(ex(), [this, p, middle, last, f] {
                            launch_sort_tasks(p, middle, last, f);
                        });

                    // Run for_each on lhs: [first, middle]
                    launch_sort_tasks(p, first, middle, f);

                    // When lhs is ready, we check on rhs
                    if (!is_ready(rhs_task)) {
                        // Put rhs_task on the list of tasks we need to await
                        // later This ensures we only deal with the task queue
                        // if we really need to
                        std::unique_lock write_lock(tasks_mutex_);
                        tasks_.emplace_back(std::move(rhs_task));
                    }
                }
            }

            /// \brief Wait for all tasks to finish
            ///
            /// This might sound like it should be as simple as a
            /// when_all(tasks_). However, while we wait for some tasks here,
            /// the running tasks might be enqueuing more tasks, so we still
            /// need a read lock here. The number of times this happens and the
            /// relative cost of this operation should still be negligible,
            /// compared to other applications.
            ///
            /// \return `true` if we had to wait for any tasks
            bool
            wait_for_sort_tasks() {
                tasks_mutex_.lock_shared();
                bool waited_any = false;
                while (!tasks_.empty()) {
                    tasks_mutex_.unlock_shared();
                    tasks_mutex_.lock();
                    detail::small_vector<futures::cfuture<void>> stolen_tasks(
                        std::make_move_iterator(tasks_.begin()),
                        std::make_move_iterator(tasks_.end()));
                    tasks_.clear();
                    tasks_mutex_.unlock();
                    when_all(stolen_tasks).wait();
                    waited_any = true;
                }
                return waited_any;
            }

            template <class P, class I, class S, class Fun>
            void
            sort(P p, I first, S last, Fun f) {
                launch_sort_tasks(p, first, last, f);
                wait_for_sort_tasks();
            }

        private:
            detail::small_vector<futures::cfuture<void>> tasks_;
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
        template <
            class FullAsync = std::false_type,
            class E,
            class P,
            class I,
            class S,
            class Fun
#ifndef FUTURES_DOXYGEN
            ,
            std::enable_if_t<
                is_executor_v<
                    E> && is_partitioner_v<P, I, S> && is_input_iterator_v<I> && is_sentinel_for_v<S, I> && is_indirectly_unary_invocable_v<Fun, I> && std::is_copy_constructible_v<Fun>,
                int> = 0
#endif
            >
        auto
        run(const E &ex, P p, I first, S last, Fun f) const {
            if constexpr (FullAsync::value) {
                // If full async, launching the tasks and solving small tasks
                // also happen asynchronously
                return async(ex, [ex, p, first, last, f]() {
                    sorter<E>(ex).sort(p, first, last, f);
                });
            } else {
                // Else, we try to solve small tasks and launching other tasks
                // if it's worth splitting the problem
                sorter<E>(ex).sort(p, first, last, f);
            }
        }
    };

    /// \brief Applies a function to a range of elements
    inline constexpr for_each_functor for_each;

    /** @}*/
    /** @}*/
} // namespace futures

#endif // FUTURES_FOR_EACH_H

// #include <futures/algorithm/none_of.h>
#ifndef FUTURES_NONE_OF_H
#define FUTURES_NONE_OF_H

// #include <futures/algorithm/partitioner/partitioner.h>

// #include <futures/algorithm/traits/unary_invoke_algorithm.h>

// #include <futures/futures.h>

// #include <futures/algorithm/detail/try_async.h>

// #include <execution>

// #include <variant>


namespace futures {
    /** \addtogroup algorithms Algorithms
     *  @{
     */
    /** \addtogroup functions Functions
     *  @{
     */


    /// \brief Functor representing the overloads for the @ref none_of function
    class none_of_functor
        : public unary_invoke_algorithm_functor<none_of_functor>
    {
        friend unary_invoke_algorithm_functor<none_of_functor>;

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
        template <
            class E,
            class P,
            class I,
            class S,
            class Fun
#ifndef FUTURES_DOXYGEN
            ,
            std::enable_if_t<
                // clang-format off
                is_executor_v<E> &&
                is_partitioner_v<P, I, S> &&
                is_input_iterator_v<I> &&
                is_sentinel_for_v<S, I> &&
                is_indirectly_unary_invocable_v<Fun, I> &&
                std::is_copy_constructible_v<Fun>
                // clang-format on
                ,
                int> = 0
#endif
            >
        bool
        run(const E &ex, P p, I first, S last, Fun f) const {
            auto middle = p(first, last);
            if (middle == last
                || std::is_same_v<
                    E,
                    inline_executor> || is_forward_iterator_v<I>)
            {
                return std::none_of(first, last, f);
            }

            // Run none_of on rhs: [middle, last]
            auto [rhs, rhs_started, rhs_cancel]
                = try_async(ex, [ex, p, middle, last, f, this]() {
                      return operator()(ex, p, middle, last, f);
                  });

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
                    return
                    operator()(make_inline_executor(), p, middle, last, f);
                }
            }
        }
    };

    /// \brief Checks if a predicate is true for none of the elements in a range
    inline constexpr none_of_functor none_of;

    /** @}*/
    /** @}*/
} // namespace futures

#endif // FUTURES_NONE_OF_H

// #include <futures/algorithm/reduce.h>
#ifndef FUTURES_REDUCE_H
#define FUTURES_REDUCE_H

// #include <futures/algorithm/partitioner/partitioner.h>

// #include <futures/algorithm/traits/binary_invoke_algorithm.h>
#ifndef FUTURES_ALGORITHM_TRAITS_BINARY_INVOKE_ALGORITHM_H
#define FUTURES_ALGORITHM_TRAITS_BINARY_INVOKE_ALGORITHM_H

// #include <futures/algorithm/partitioner/partitioner.h>

// #include <futures/algorithm/policies.h>

// #include <futures/algorithm/traits/is_indirectly_binary_invocable.h>

// #include <futures/algorithm/traits/is_input_range.h>

// #include <futures/algorithm/traits/range_value.h>

// #include <futures/futures.h>

// #include <futures/algorithm/detail/try_async.h>

// #include <execution>

#include <numeric>
// #include <variant>


namespace futures {
    /** \addtogroup algorithms Algorithms
     *  @{
     */

    /** \addtogroup algorithm-traits Algorithm Traits
     *  @{
     */

    /// \brief Binary algorithm overloads
    ///
    /// CRTP class with the overloads for classes that aggregate
    /// elements in a sequence with an binary function. This includes
    /// algorithms such as reduce and accumulate.
    template <class Derived>
    class binary_invoke_algorithm_functor
    {
    public:
        template <
            class E,
            class P,
            class I,
            class S,
            class T,
            class Fun = std::plus<>
#ifndef FUTURES_DOXYGEN
            ,
            std::enable_if_t<
                // clang-format off
                is_executor_v<E> &&
                is_partitioner_v<P, I, S> &&
                is_input_iterator_v<I> &&
                is_sentinel_for_v<S, I> &&
                std::is_same_v<iter_value_t<I>, T> &&
                is_indirectly_binary_invocable_v<Fun, I, I> &&
                std::is_copy_constructible_v<Fun>
                // clang-format on
                ,
                int> = 0
#endif
            >
        decltype(auto)
        operator()(const E &ex, P p, I first, S last, T i, Fun f = std::plus<>())
            const {
            return Derived().run(ex, p, first, last, i, f);
        }

        /// \overload default init value
        template <
            class E,
            class P,
            class I,
            class S,
            class Fun = std::plus<>
#ifndef FUTURES_DOXYGEN
            ,
            std::enable_if_t<
                // clang-format off
                is_executor_v<E> &&
                is_partitioner_v<P, I, S> &&
                is_input_iterator_v<I> &&
                is_sentinel_for_v<S, I> &&
                is_indirectly_binary_invocable_v<Fun, I, I> &&
                std::is_copy_constructible_v<Fun>
                // clang-format on
                ,
                int> = 0
#endif
            >
        decltype(auto)
        operator()(const E &ex, P p, I first, S last, Fun f = std::plus<>())
            const {
            if (first != last) {
                return Derived().run(ex, p, std::next(first), last, *first, f);
            } else {
                return iter_value_t<I>{};
            }
        }

        /// \overload execution policy instead of executor
        template <
            class E,
            class P,
            class I,
            class S,
            class T,
            class Fun = std::plus<>
#ifndef FUTURES_DOXYGEN
            ,
            std::enable_if_t<
                // clang-format off
                !is_executor_v<E> &&
                is_execution_policy_v<E> &&
                is_partitioner_v<P, I, S> &&
                is_input_iterator_v<I> &&
                is_sentinel_for_v<S, I> &&
                std::is_same_v<iter_value_t<I>, T> &&
                is_indirectly_binary_invocable_v<Fun, I, I> &&
                std::is_copy_constructible_v<Fun>
                // clang-format on
                ,
                int> = 0
#endif
            >
        decltype(auto)
        operator()(const E &, P p, I first, S last, T i, Fun f = std::plus<>())
            const {
            return Derived().
            operator()(make_policy_executor<E, I, S>(), p, first, last, i, f);
        }

        /// \overload execution policy instead of executor / default init value
        template <
            class E,
            class P,
            class I,
            class S,
            class Fun = std::plus<>
#ifndef FUTURES_DOXYGEN
            ,
            std::enable_if_t<
                // clang-format off
                !is_executor_v<E> &&
                is_execution_policy_v<E> &&
                is_partitioner_v<P, I, S> &&
                is_input_iterator_v<I> &&
                is_sentinel_for_v<S, I> &&
                is_indirectly_binary_invocable_v<Fun, I, I> &&
                std::is_copy_constructible_v<Fun>
                // clang-format on
                ,
                int> = 0
#endif
            >
        decltype(auto)
        operator()(const E &, P p, I first, S last, Fun f = std::plus<>())
            const {
            return
            operator()(make_policy_executor<E, I, S>(), p, first, last, f);
        }

        /// \overload Ranges
        template <
            class E,
            class P,
            class R,
            class T,
            class Fun = std::plus<>
#ifndef FUTURES_DOXYGEN
            ,
            std::enable_if_t<
                // clang-format off
                (is_executor_v<E> || is_execution_policy_v<E>) &&
                is_range_partitioner_v<P,R> &&
                is_input_range_v<R> &&
                std::is_same_v<range_value_t<R>, T> &&
                is_indirectly_binary_invocable_v<Fun, iterator_t<R>, iterator_t<R>> &&
                std::is_copy_constructible_v<Fun>
                // clang-format on
                ,
                int> = 0
#endif
            >
        decltype(auto)
        operator()(const E &ex, P p, R &&r, T i, Fun f = std::plus<>()) const {
            return Derived().
            operator()(ex, p, std::begin(r), std::end(r), i, std::move(f));
        }

        /// \overload Ranges / default init value
        template <
            class E,
            class P,
            class R,
            class Fun = std::plus<>
#ifndef FUTURES_DOXYGEN
            ,
            std::enable_if_t<
                // clang-format off
                (is_executor_v<E> || is_execution_policy_v<E>) &&
                is_range_partitioner_v<P,R> &&
                is_input_range_v<R> &&
                is_indirectly_binary_invocable_v<Fun, iterator_t<R>, iterator_t<R>> &&
                std::is_copy_constructible_v<Fun>
                // clang-format on
                ,
                int> = 0
#endif
            >
        decltype(auto)
        operator()(const E &ex, P p, R &&r, Fun f = std::plus<>()) const {
            return operator()(ex, p, std::begin(r), std::end(r), std::move(f));
        }

        /// \overload Iterators / default parallel executor
        template <
            class P,
            class I,
            class S,
            class T,
            class Fun = std::plus<>
#ifndef FUTURES_DOXYGEN
            ,
            std::enable_if_t<
                // clang-format on
                is_partitioner_v<
                    P,
                    I,
                    S> && is_input_iterator_v<I> && is_sentinel_for_v<S, I> && std::is_same_v<iter_value_t<I>, T> && is_indirectly_binary_invocable_v<Fun, I, I> && std::is_copy_constructible_v<Fun>
                // clang-format off
                , int> = 0
#endif
            >
        decltype(auto)
        operator()(P p, I first, S last, T i, Fun f = std::plus<>()) const {
            return
            Derived().run(make_default_executor(), p, first, last, i, std::move(f));
        }

        /// \overload Iterators / default parallel executor / default init value
        template <
            class P,
            class I,
            class S,
            class Fun = std::plus<>
#ifndef FUTURES_DOXYGEN
            ,
            std::enable_if_t<
                // clang-format off
                is_partitioner_v<P,I,S> &&
                is_input_iterator_v<I> &&
                is_sentinel_for_v<S, I> &&
                is_indirectly_binary_invocable_v<Fun, I, I> &&
                std::is_copy_constructible_v<Fun>
                // clang-format on
                ,
                int> = 0
#endif
            >
        decltype(auto)
        operator()(P p, I first, S last, Fun f = std::plus<>()) const {
            return
            operator()(make_default_executor(), p, first, last, std::move(f));
        }

        /// \overload Ranges / default parallel executor
        template <
            class P,
            class R,
            class T,
            class Fun = std::plus<>
#ifndef FUTURES_DOXYGEN
            ,
            std::enable_if_t<
                // clang-format off
                is_range_partitioner_v<P,R> &&
                is_input_range_v<R> &&
                std::is_same_v<range_value_t<R>, T> &&
                is_indirectly_binary_invocable_v<Fun, iterator_t<R>, iterator_t<R>> &&
                std::is_copy_constructible_v<Fun>
                // clang-format on
                ,
                int> = 0
#endif
            >
        decltype(auto)
        operator()(P p, R &&r, T i, Fun f = std::plus<>()) const {
            return Derived().run(
                make_default_executor(),
                p,
                std::begin(r),
                std::end(r),
                i,
                std::move(f));
        }

        /// \overload Ranges / default parallel executor / default init value
        template <
            class P,
            class R,
            class Fun = std::plus<>
#ifndef FUTURES_DOXYGEN
            ,
            std::enable_if_t<
                // clang-format off
                is_range_partitioner_v<P, R> &&
                is_input_range_v<R> &&
                is_indirectly_binary_invocable_v<Fun, iterator_t<R>, iterator_t<R>> &&
                std::is_copy_constructible_v<Fun>
                // clang-format on
                ,
                int> = 0
#endif
            >
        decltype(auto)
        operator()(P p, R &&r, Fun f = std::plus<>()) const {
            return operator()(
                make_default_executor(),
                p,
                std::begin(r),
                std::end(r),
                std::move(f));
        }

        /// \overload Iterators / default partitioner
        template <
            class E,
            class I,
            class S,
            class T,
            class Fun = std::plus<>
#ifndef FUTURES_DOXYGEN
            ,
            std::enable_if_t<
                // clang-format off
                (is_executor_v<E> || is_execution_policy_v<E>) &&
                is_input_iterator_v<I> &&
                is_sentinel_for_v<S, I> &&
                std::is_same_v<iter_value_t<I>, T> &&
                is_indirectly_binary_invocable_v<Fun, I, I> &&
                std::is_copy_constructible_v<Fun>
                // clang-format on
                ,
                int> = 0
#endif
            >
        decltype(auto)
        operator()(const E &ex, I first, S last, T i, Fun f = std::plus<>())
            const {
            return operator()(
                ex,
                make_default_partitioner(first, last),
                first,
                last,
                i,
                std::move(f));
        }

        /// \overload Iterators / default partitioner / default init value
        template <
            class E,
            class I,
            class S,
            class Fun = std::plus<>
#ifndef FUTURES_DOXYGEN
            ,
            std::enable_if_t<
                // clang-format off
                (is_executor_v<E> || is_execution_policy_v<E>) &&
                is_input_iterator_v<I> &&
                is_sentinel_for_v<S, I> &&
                is_indirectly_binary_invocable_v<Fun, I, I> &&
                std::is_copy_constructible_v<Fun>
                // clang-format on
                ,
                int> = 0
#endif
            >
        decltype(auto)
        operator()(const E &ex, I first, S last, Fun f = std::plus<>()) const {
            return operator()(
                ex,
                make_default_partitioner(first, last),
                first,
                last,
                std::move(f));
        }

        /// \overload Ranges / default partitioner
        template <
            class E,
            class R,
            class T,
            class Fun = std::plus<>
#ifndef FUTURES_DOXYGEN
            ,
            std::enable_if_t<
                // clang-format off
                (is_executor_v<E> || is_execution_policy_v<E>) &&
                is_input_range_v<R> &&
                std::is_same_v<range_value_t<R>, T> &&
                is_indirectly_binary_invocable_v<Fun, iterator_t<R>, iterator_t<R>> &&
                std::is_copy_constructible_v<Fun>
                // clang-format on
                ,
                int> = 0
#endif
            >
        decltype(auto)
        operator()(const E &ex, R &&r, T i, Fun f = std::plus<>()) const {
            return operator()(
                ex,
                make_default_partitioner(std::forward<R>(r)),
                std::begin(r),
                std::end(r),
                i,
                std::move(f));
        }

        /// \overload Ranges / default partitioner / default init value
        template <
            class E,
            class R,
            class Fun = std::plus<>
#ifndef FUTURES_DOXYGEN
            ,
            std::enable_if_t<
                // clang-format off
                (is_executor_v<E> || is_execution_policy_v<E>) &&
                is_input_range_v<R> &&
                is_indirectly_binary_invocable_v<Fun, iterator_t<R>, iterator_t<R>> &&
                std::is_copy_constructible_v<Fun>
                // clang-format on
                ,
                int> = 0
#endif
            >
        decltype(auto)
        operator()(const E &ex, R &&r, Fun f = std::plus<>()) const {
            return operator()(
                ex,
                make_default_partitioner(std::forward<R>(r)),
                std::begin(r),
                std::end(r),
                std::move(f));
        }

        /// \overload Iterators / default executor / default partitioner
        template <
            class I,
            class S,
            class T,
            class Fun = std::plus<>
#ifndef FUTURES_DOXYGEN
            ,
            std::enable_if_t<
                // clang-format off
                is_input_iterator_v<I> &&
                is_sentinel_for_v<S, I> &&
                std::is_same_v<iter_value_t<I>, T> &&
                is_indirectly_binary_invocable_v<Fun, I, I> &&
                std::is_copy_constructible_v<Fun>
                // clang-format on
                ,
                int> = 0
#endif
            >
        decltype(auto)
        operator()(I first, S last, T i, Fun f = std::plus<>()) const {
            return Derived().run(
                make_default_executor(),
                make_default_partitioner(first, last),
                first,
                last,
                i,
                std::move(f));
        }

        /// \overload Iterators / default executor / default partitioner /
        /// default init value
        template <
            class I,
            class S,
            class Fun = std::plus<>
#ifndef FUTURES_DOXYGEN
            ,
            std::enable_if_t<
                // clang-format off
                is_input_iterator_v<I> &&
                is_sentinel_for_v<S, I> &&
                is_indirectly_binary_invocable_v<Fun, I, I> &&
                std::is_copy_constructible_v<Fun>
                // clang-format on
                ,
                int> = 0
#endif
            >
        decltype(auto)
        operator()(I first, S last, Fun f = std::plus<>()) const {
            return operator()(
                make_default_executor(),
                make_default_partitioner(first, last),
                first,
                last,
                std::move(f));
        }

        /// \overload Ranges / default executor / default partitioner
        template <
            class R,
            class T,
            class Fun = std::plus<>
#ifndef FUTURES_DOXYGEN
            ,
            std::enable_if_t<
                // clang-format off
                is_input_range_v<R> &&
                std::is_same_v<range_value_t<R>, T> &&
                is_indirectly_binary_invocable_v<Fun, iterator_t<R>, iterator_t<R>> &&
                std::is_copy_constructible_v<Fun>
                // clang-format on
                ,
                int> = 0
#endif
            >
        decltype(auto)
        operator()(R &&r, T i, Fun f = std::plus<>()) const {
            return Derived().run(
                make_default_executor(),
                make_default_partitioner(r),
                std::begin(r),
                std::end(r),
                i,
                std::move(f));
        }

        /// \overload Ranges / default executor / default partitioner / default
        /// init value
        template <
            class R,
            class Fun = std::plus<>
#ifndef FUTURES_DOXYGEN
            ,
            std::enable_if_t<
                // clang-format off
                is_input_range_v<R> &&
                is_indirectly_binary_invocable_v<Fun, iterator_t<R>, iterator_t<R>> &&
                std::is_copy_constructible_v<Fun>
                // clang-format on
                ,
                int> = 0
#endif
            >
        decltype(auto)
        operator()(R &&r, Fun f = std::plus<>()) const {
            return operator()(
                make_default_executor(),
                make_default_partitioner(r),
                std::begin(r),
                std::end(r),
                std::move(f));
        }
    };

    /** @}*/
    /** @}*/
} // namespace futures

#endif // FUTURES_ALGORITHM_TRAITS_BINARY_INVOKE_ALGORITHM_H

// #include <futures/futures.h>

// #include <futures/algorithm/detail/try_async.h>

// #include <execution>

// #include <numeric>

// #include <variant>


namespace futures {
    /** \addtogroup algorithms Algorithms
     *  @{
     */
    /** \addtogroup functions Functions
     *  @{
     */


    /// \brief Functor representing the overloads for the @ref reduce function
    class reduce_functor
        : public binary_invoke_algorithm_functor<reduce_functor>
    {
        friend binary_invoke_algorithm_functor<reduce_functor>;

        /// \brief Complete overload of the reduce algorithm
        ///
        /// The reduce algorithm is equivalent to a version std::accumulate
        /// where the binary operation is applied out of order. \tparam E
        /// Executor type \tparam P Partitioner type \tparam I Iterator type
        /// \tparam S Sentinel iterator type
        /// \tparam Fun Function type
        /// \param ex Executor
        /// \param p Partitioner
        /// \param first Iterator to first element in the range
        /// \param last Iterator to (last + 1)-th element in the range
        /// \param i Initial value for the reduction
        /// \param f Function
        template <
            class E,
            class P,
            class I,
            class S,
            class T,
            class Fun = std::plus<>
#ifndef FUTURES_DOXYGEN
            ,
            std::enable_if_t<
                // clang-format off
                is_executor_v<E> &&
                is_partitioner_v<P, I, S> &&
                is_input_iterator_v<I> &&
                is_sentinel_for_v<S, I> &&
                std::is_same_v<iter_value_t<I>, T> &&
                is_indirectly_binary_invocable_v<Fun, I, I> &&
                std::is_copy_constructible_v<Fun>,
                // clang-format on
                int> = 0
#endif
            >
        T
        run(const E &ex, P p, I first, S last, T i, Fun f = std::plus<>())
            const {
            auto middle = p(first, last);
            if (middle == last
                || std::is_same_v<
                    E,
                    inline_executor> || is_forward_iterator_v<I>)
            {
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
                T i_rhs =
                operator()(make_inline_executor(), p, middle, last, i, f);
                return f(lhs, i_rhs);
            }
        }
    };

    /// \brief Sums up (or accumulate with a custom function) a range of
    /// elements, except out of order
    inline constexpr reduce_functor reduce;

    /** @}*/
    /** @}*/
} // namespace futures

#endif // FUTURES_REDUCE_H


#endif // FUTURES_ALGORITHM_H
