//
// Copyright (c) 2021 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_FUTURES_BASIC_FUTURE_HPP
#define FUTURES_FUTURES_BASIC_FUTURE_HPP

#include <futures/executor/default_executor.hpp>
#include <futures/futures/future_options.hpp>
#include <futures/futures/stop_token.hpp>
#include <futures/futures/traits/is_future.hpp>
#include <futures/futures/detail/continuations_source.hpp>
#include <futures/detail/utility/empty_base.hpp>
#include <futures/futures/detail/shared_state.hpp>
#include <futures/detail/exception/throw_exception.hpp>
#include <futures/futures/detail/traits/append_future_option.hpp>
#include <futures/futures/detail/traits/is_executor_then_function.hpp>
#include <futures/futures/detail/traits/is_type_template_in_args.hpp>
#include <futures/futures/detail/traits/remove_future_option.hpp>
#include <functional>
#include <utility>
#include <shared_mutex>

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

#ifndef FUTURES_DOXYGEN
    // Fwd-declare basic_future friends
    namespace detail {
        template <bool>
        struct async_future_scheduler;
        struct internal_then_functor;
        class waiter_for_any;
    } // namespace detail
    template <class R, class Options>
    class promise_base;
    template <class U, class Options>
    class promise;
    template <class Signature, class Options>
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
    template <class T, class Options = future_options<>>
    class basic_future : private Options
    {
    private:
        // futures and shared futures use the same state type
        using shared_state_options = detail::
            remove_future_option_t<shared_opt, Options>;
        using shared_state_type = detail::shared_state<T, shared_state_options>;
        using shared_state_base = detail::shared_state_base<
            Options::is_deferred>;

        using notify_when_ready_handle = typename shared_state_base::
            notify_when_ready_handle;

        template <class U, class O>
        friend class basic_future;

        // Types allowed to access the shared state constructor
        template <class U, class O>
        friend class ::futures::promise_base;

        template <class U, class O>
        friend class ::futures::promise;

        template <typename Signature, class Opts>
        friend class ::futures::packaged_task;

        template <bool>
        friend struct detail::async_future_scheduler;

        friend struct detail::internal_then_functor;

        friend class detail::waiter_for_any;

        /// \brief Construct from a pointer to the shared state
        ///
        /// This constructor is private because we need to ensure the launching
        /// function appropriately sets this std::future handling these traits
        /// This is a function for async.
        ///
        /// \param s Future shared state
        explicit basic_future(
            const std::shared_ptr<shared_state_type> &s) noexcept
            : state_{ std::move(s) } {}

    public:
        /// \name Public types
        /// @{

        using value_type = T;

        /// @}

        /// \name Constructors
        /// @{

        /// \brief Constructs the basic_future
        ///
        /// The default constructor creates an invalid future with no shared
        /// state.
        ///
        /// Null shared state. Properties inherited from base classes.
        basic_future() noexcept = default;

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
            : join_{ other.join_ }, state_{ other.state_ } {
            static_assert(
                Options::is_shared,
                "Copy constructor is only available for shared futures");
        }

        /// \brief Move constructor.
        ///
        /// Inherited from base classes.
        basic_future(basic_future &&other) noexcept
            : join_{ other.join_ }, state_{ std::move(other.state_) } {
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
            if constexpr (Options::is_stoppable && !Options::is_shared) {
                if (valid() && !is_ready()) {
                    get_stop_source().request_stop();
                }
            }
            wait_if_last();
            if constexpr (Options::is_continuable) {
                if (valid()) {
                    auto &cs = state_->get_continuations_source();
                    if (cs.run_possible()) {
                        cs.request_run();
                    }
                }
            }
        }

        /// \brief Copy assignment for shared futures only.
        ///
        /// Inherited from base classes.
        template <class U, class O>
        basic_future &
        operator=(const basic_future<U, O> &other) {
            static_assert(
                Options::is_shared,
                "Copy assignment is only available for shared futures");
            if (&other == this) {
                return *this;
            }
            wait_if_last(); // If this is the last shared future waiting for
                            // previous result, we wait
            join_ = other.join_;
            state_ = other.state_; // Make it point to the same shared state
            other.detach();        // Detach other to ensure it won't block at
                                   // destruction
            return *this;
        }

        /// \brief Move assignment.
        ///
        /// Inherited from base classes.
        template <class U, class O>
        basic_future &
        operator=(basic_future<U, O> &&other) noexcept {
            if (&other == this) {
                return *this;
            }
            wait_if_last(); // If this is the last shared future waiting for
                            // previous result, we wait
            join_ = other.join_;
            state_ = other.state_; // Make it point to the same shared state
            other.state_.reset();
            other.detach(); // Detach other to ensure it won't block at
                            // destruction
            return *this;
        }
        /// @}

        /// \brief Emplace a function to the shared vector of continuations
        ///
        /// If the function is ready, this functions uses the given executor
        /// instead of executing with the previous executor.
        ///
        /// \note This function only participates in overload resolution if the
        /// future supports continuations
        ///
        /// \tparam Executor Executor type
        /// \tparam Fn Function type
        /// \param ex An executor
        /// \param fn A continuation function
        /// \return
        template <
            class Executor,
            class Fn
#ifndef FUTURES_DOXYGEN
            ,
            bool U = Options::is_continuable,
            std::enable_if_t<U && U == Options::is_continuable, int> = 0
#endif
            >
        bool
        then(const Executor &ex, Fn &&fn) {
            if (!valid()) {
                detail::throw_exception<std::future_error>(
                    std::future_errc::no_state);
            }
            if (!is_ready()
                && state_->get_continuations_source().run_possible()) {
                if constexpr (std::is_copy_constructible_v<Fn>) {
                    return state_->get_continuations_source()
                        .emplace_continuation(ex, std::forward<Fn>(fn));
                } else {
                    auto fn_shared_ptr = std::make_shared<Fn>(std::move(fn));
                    auto copyable_handle = [fn_shared_ptr]() {
                        (*fn_shared_ptr)();
                    };
                    return state_->get_continuations_source()
                        .emplace_continuation(ex, copyable_handle);
                }
            } else {
                asio::post(ex, asio::use_future(std::forward<Fn>(fn)));
                return false;
            }
        }

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
        template <
            class Fn
#ifndef FUTURES_DOXYGEN
            ,
            bool U = Options::is_continuable,
            std::enable_if_t<U && U == Options::is_continuable, int> = 0
#endif

            >
        bool
        then(Fn &&fn) {
            return then(
                make_default_executor(),
                asio::use_future(std::forward<Fn>(fn)));
        }

        /// \brief Request the future to stop whatever task it's running
        ///
        /// \note This function only participates in overload resolution if the
        /// future supports stop tokens
        ///
        /// \return Whether the request was made
        template <
#ifndef FUTURES_DOXYGEN
            bool U = Options::is_stoppable,
            std::enable_if_t<U && U == Options::is_stoppable, int> = 0
#endif
            >
        bool
        request_stop() noexcept {
            return state_->get_stop_source().request_stop();
        }

        /// \brief Get this future's stop source
        ///
        /// \note This function only participates in overload resolution if the
        /// future supports stop tokens
        ///
        /// \return The stop source
        template <
#ifndef FUTURES_DOXYGEN
            bool U = Options::is_stoppable,
            std::enable_if_t<U && U == Options::is_stoppable, int> = 0
#endif
            >
        [[nodiscard]] stop_source
        get_stop_source() const noexcept {
            return state_->get_stop_source();
        }

        /// \brief Get this future's stop token
        ///
        /// \note This function only participates in overload resolution if the
        /// future supports stop tokens
        ///
        /// \return The stop token
        template <
#ifndef FUTURES_DOXYGEN
            bool U = Options::is_stoppable,
            std::enable_if_t<U && U == Options::is_stoppable, int> = 0
#endif
            >
        [[nodiscard]] stop_token
        get_stop_token() const noexcept {
            return get_stop_source().get_token();
        }

        /// \brief Wait until all futures have a valid result and retrieves it
        ///
        /// The behaviour depends on shared_based.
        decltype(auto)
        get() {
            if (!valid()) {
                detail::throw_exception<future_uninitialized>();
            }
            if constexpr (Options::is_shared) {
                return state_->get();
            } else {
                std::shared_ptr<shared_state_type> tmp;
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
        basic_future<T, detail::append_future_option_t<shared_opt, Options>>
        share() {
            if (!valid()) {
                detail::throw_exception<future_uninitialized>();
            }
            using shared_options = detail::
                append_future_option_t<shared_opt, Options>;
            using shared_future_t = basic_future<T, shared_options>;
            shared_future_t res{
                Options::is_shared ? state_ : std::move(state_)
            };
            res.join_ = std::exchange(join_, Options::is_shared && join_);
            return res;
        }

        /// \brief Get exception pointer without throwing exception
        ///
        /// This extends std::future so that we can always check if the future
        /// threw an exception
        std::exception_ptr
        get_exception_ptr() {
            if (!valid()) {
                detail::throw_exception<future_uninitialized>();
            }
            return state_->get_exception_ptr();
        }

        /// \brief Checks if the future refers to a shared state
        [[nodiscard]] bool
        valid() const {
            return nullptr != state_.get();
        }

        /// \brief Blocks until the result becomes available.
        void
        wait() const {
            if (!valid()) {
                detail::throw_exception<future_uninitialized>();
            }
            state_->wait();
        }

        /// \brief Waits for the result to become available.
        template <class Rep, class Period>
        [[nodiscard]] std::future_status
        wait_for(
            const std::chrono::duration<Rep, Period> &timeout_duration) const {
            if (!valid()) {
                detail::throw_exception<future_uninitialized>();
            }
            return state_->wait_for(timeout_duration);
        }

        /// \brief Waits for the result to become available.
        template <class Clock, class Duration>
        std::future_status
        wait_until(const std::chrono::time_point<Clock, Duration> &timeout_time)
            const {
            if (!valid()) {
                detail::throw_exception<future_uninitialized>();
            }
            return state_->wait_until(timeout_time);
        }

        /// \brief Checks if the shared state is ready
        [[nodiscard]] bool
        is_ready() const {
            if (!valid()) {
                detail::throw_exception<std::future_error>(
                    std::future_errc::no_state);
            }
            return state_->is_ready();
        }

        /// \brief Tell this future not to join at destruction
        ///
        /// For safety, all futures join at destruction
        void
        detach() {
            join_ = false;
        }

        /// \brief Notify this condition variable when the future is ready
        notify_when_ready_handle
        notify_when_ready(std::condition_variable_any &cv) {
            if (!state_) {
                detail::throw_exception<future_uninitialized>();
            }
            return state_->notify_when_ready(cv);
        }

        /// \brief Cancel request to notify this condition variable when the
        /// future is ready
        void
        unnotify_when_ready(notify_when_ready_handle h) {
            if (!state_) {
                detail::throw_exception<future_uninitialized>();
            }
            return state_->unnotify_when_ready(h);
        }

    private:
        /// \name Private Functions
        /// @{

        /// \brief Get a reference to the mutex in the underlying shared state
        std::mutex &
        waiters_mutex() {
            if (!state_) {
                detail::throw_exception<future_uninitialized>();
            }
            return state_->waiters_mutex();
        }

        void
        wait_if_last() const {
            if (join_ && valid() && (!is_ready())) {
                if constexpr (!Options::is_shared) {
                    wait();
                } else /* constexpr */ {
                    if (1 == state_.use_count()) {
                        wait();
                    }
                }
            }
        }

        /// @}

        /// \name Members
        /// @{
        bool join_{ true };

        /// \brief Pointer to shared state
        std::shared_ptr<shared_state_type> state_{};
        /// @}
    };

#ifndef FUTURES_DOXYGEN
    /// \name Define all basic_futures as a kind of future
    /// @{
    template <typename... Args>
    struct is_future<basic_future<Args...>> : std::true_type
    {};
    /// @}

    /// \name Define all basic_futures as a future with a ready notifier
    /// @{
    template <typename... Args>
    struct has_ready_notifier<basic_future<Args...>> : std::true_type
    {};
    /// @}

    /// \name Define shared basic_futures as supporting shared values
    /// @{
    template <class T, class... Args>
    struct is_shared_future<basic_future<T, future_options<Args...>>>
        : detail::is_in_args<shared_opt, Args...>
    {};
    /// @}

    /// \name Define continuable basic_futures as supporting lazy continuations
    /// @{
    template <class T, class... Args>
    struct is_continuable<basic_future<T, future_options<Args...>>>
        : detail::is_in_args<continuable_opt, Args...>
    {};
    /// @}

    /// \name Define stoppable basic_futures as being stoppable
    ///
    /// Some futures might be stoppable without a stop token
    ///
    /// @{
    template <class T, class... Args>
    struct is_stoppable<basic_future<T, future_options<Args...>>>
        : detail::is_in_args<stoppable_opt, Args...>
    {};

    template <class T, class... Args>
    struct has_stop_token<basic_future<T, future_options<Args...>>>
        : detail::is_in_args<stoppable_opt, Args...>
    {};

    /// \name Define deferred basic_futures as being deferred
    /// @{
    template <class T, class... Args>
    struct is_deferred<basic_future<T, future_options<Args...>>>
        : detail::is_in_args<deferred_opt, Args...>
    {};
    /// @}
#endif

    /// \brief A simple future type similar to `std::future`
    template <class T>
    using future
        = basic_future<T, future_options<executor_opt<default_executor_type>>>;

    /// \brief A future that simply holds a ready value
    ///
    /// These futures have no associated executor
    template <class T>
    using vfuture = basic_future<T, future_options<>>;

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
    using jfuture = basic_future<
        T,
        future_options<executor_opt<default_executor_type>, stoppable_opt>>;

    /// \brief A future type with lazy continuations
    ///
    /// This is what a @ref futures::async returns when the first function
    /// parameter is not a @ref futures::stop_token
    ///
    template <class T>
    using cfuture = basic_future<
        T,
        future_options<executor_opt<default_executor_type>, continuable_opt>>;

    /// \brief A future type with lazy continuations and stop tokens
    ///
    /// This is what a @ref futures::async returns when the first function
    /// parameter is a @ref futures::stop_token
    template <class T>
    using jcfuture = basic_future<
        T,
        future_options<
            executor_opt<default_executor_type>,
            continuable_opt,
            stoppable_opt>>;

    /// \brief A simple std::shared_future
    ///
    /// This is what a futures::future::share() returns
    template <class T>
    using shared_future = basic_future<
        T,
        future_options<executor_opt<default_executor_type>, shared_opt>>;

    /// \brief A shared future type with stop tokens
    ///
    /// This is what a @ref futures::jfuture::share() returns
    template <class T>
    using shared_jfuture = basic_future<
        T,
        future_options<
            executor_opt<default_executor_type>,
            continuable_opt,
            stoppable_opt,
            shared_opt>>;

    /// \brief A shared future type with lazy continuations
    ///
    /// This is what a @ref futures::cfuture::share() returns
    template <class T>
    using shared_cfuture = basic_future<
        T,
        future_options<
            executor_opt<default_executor_type>,
            continuable_opt,
            shared_opt>>;

    /// \brief A shared future type with lazy continuations and stop tokens
    ///
    /// This is what a @ref futures::jcfuture::share() returns
    template <class T>
    using shared_jcfuture = basic_future<
        T,
        future_options<
            executor_opt<default_executor_type>,
            continuable_opt,
            stoppable_opt,
            shared_opt>>;

    /** @} */
    /** @} */
} // namespace futures

#endif // FUTURES_FUTURES_BASIC_FUTURE_HPP
