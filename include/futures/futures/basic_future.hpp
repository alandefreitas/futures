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
#include <futures/adaptor/detail/make_continuation_state.hpp>
#include <futures/adaptor/detail/unwrap_and_continue.hpp>
#include <futures/detail/exception/throw_exception.hpp>
#include <futures/detail/utility/empty_base.hpp>
#include <futures/detail/utility/maybe_copyable.hpp>
#include <futures/futures/detail/continuations_source.hpp>
#include <futures/futures/detail/operation_state.hpp>
#include <futures/futures/detail/share_if_not_shared.hpp>
#include <futures/futures/detail/traits/append_future_option.hpp>
#include <futures/futures/detail/traits/is_executor_then_function.hpp>
#include <futures/futures/detail/traits/is_type_template_in_args.hpp>
#include <futures/futures/detail/traits/remove_future_option.hpp>
#include <functional>
#include <utility>
#include <shared_mutex>

namespace futures {
    /** @addtogroup futures Futures
     *
     * \brief Basic future types and functions
     *
     * The futures library provides components to create and launch futures,
     * objects representing data that might not be available yet.
     *  @{
     */

    /** @addtogroup future-types Future types
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

    /// A basic future type with custom features
    /**
     * Note that these classes only provide the capabilities of tracking these
     * features, such as continuations.
     *
     * Setting up these capabilities (creating tokens or setting main future to
     * run continuations) needs to be done when the future is created in a
     * function such as @ref async by creating the appropriate state for each
     * feature.
     *
     * All this behavior is already encapsulated in the @ref async function.
     *
     * @tparam T Type of element
     * @tparam Shared `std::true_value` if this future is shared
     * @tparam LazyContinuable `std::true_value` if this future supports
     * continuations
     * @tparam Stoppable `std::true_value` if this future
     * contains a stop token
     */
    template <class T, class Options = future_options<>>
    class basic_future
        : private detail::maybe_copyable<Options::is_shared>
        , private detail::conditional_base<!Options::is_always_detached, bool>
    {
    private:
        // join base represents the logical boolean indicating if we should
        // join at destruction
        using join_base = detail::
            conditional_base<!Options::is_always_detached, bool>;

        // the initial join value
        typename join_base::value_type
        initial_join_value() {
            if constexpr (!Options::is_always_detached) {
                return true;
            } else {
                return detail::empty_value;
            }
        }

        // remove shared_opt from operation state options
        using operation_state_options = detail::
            remove_future_option_t<shared_opt, Options>;

        // determine type of inline operation state
        template <bool inline_op_state>
        struct operation_state_type_impl
        {
            using type = std::conditional_t<
                inline_op_state,
                // Type with complete information about the task
                detail::deferred_operation_state<T, operation_state_options>,
                // Type to be shared, task is erased
                detail::operation_state<T, operation_state_options>>;
        };

        static constexpr bool inline_op_state = Options::is_always_deferred
                                                && !Options::is_shared;

        using operation_state_type = typename operation_state_type_impl<
            inline_op_state>::type;

        // determine type of the shared state, whenever it needs to be shared
        using shared_state_type = detail::
            shared_state<T, operation_state_options>;

        // the base operation state implementation type, where the notification
        // handle lives
        using operation_state_base = detail::operation_state_base<
            Options::is_always_deferred>;

        // the type of notification handles in the shared state
        using notify_when_ready_handle = typename operation_state_base::
            notify_when_ready_handle;

        // the type
        using unique_or_shared_state = std::conditional_t<
            inline_op_state,
            operation_state_type,
            shared_state_type>;

        // Types allowed to convert the shared state into a new future
        template <class U, class O>
        friend class basic_future;

        // Types allowed to access the shared state constructor
        template <class U, class O>
        friend class ::futures::promise_base;

        template <class U, class O>
        friend class ::futures::promise;

        template <typename Signature, class Opts>
        friend class ::futures::packaged_task;

        friend struct detail::async_future_scheduler;

        friend struct detail::internal_then_functor;

        friend class detail::waiter_for_any;

        /// Construct from a shared operation state
        /**
         * This constructor is private because we need to ensure the launching
         * function appropriately sets this std::future handling these traits.
         *
         * This is mostly used by the async and other launching functions.
         *
         * @param s Future shared state
         */
        explicit basic_future(const shared_state_type &s) noexcept
            : detail::maybe_copyable<Options::is_shared>{},
              join_base(initial_join_value()), state_{ std::move(s) } {}

        /// Construct from an inline operation state
        /**
         * This constructor is private because we need to ensure the launching
         * function appropriately sets this std::future handling these traits.
         *
         * This is mostly used by the async and other launching functions.
         *
         * @param op Future operation state
         */
        explicit basic_future(operation_state_type &&op) noexcept
            : detail::maybe_copyable<Options::is_shared>{},
              join_base(initial_join_value()), state_{ std::move(op) } {}

    public:
        /// @name Public types
        /// @{

        using value_type = T;

        /// @}

        /// @name Constructors
        /// @{

        /// Constructs the basic_future
        /**
         * The default constructor creates an invalid future with no shared
         * state.
         *
         * Null shared state. Properties inherited from base classes.
         */
        basic_future() noexcept
            : detail::maybe_copyable<Options::is_shared>{},
              join_base(initial_join_value()), state_{ nullptr } {};

        /// Copy constructor for shared futures only.
        /**
         * Inherited from base classes.
         *
         * @note The copy constructor only participates in overload resolution
         * if `other` is shared
         *
         * @param other Another future used as source to initialize the shared
         * state
         */
        basic_future(const basic_future &other)
            : detail::maybe_copyable<Options::is_shared>{ other },
              join_base{ other.join_base::get() }, state_{ other.state_ } {
            static_assert(
                Options::is_shared,
                "Copy constructor is only available for shared futures");
        }

#ifndef FUTURES_DOXYGEN
    private:
        // This makes a non-eligible template instances have two copy
        // constructors, which makes it not copy constructible according
        // to std::copy_constructible_v<...>.
        // This is the least intrusive way to achieve a conditional
        // copy constructor that fails std::is_copy_constructible before
        // C++20.
        struct moo
        {};
        basic_future(
            std::conditional_t<!Options::is_shared, const basic_future &, moo>,
            moo = moo());
    public:
#endif

        /// Move constructor.
        /**
         * Inherited from base classes.
         */
        basic_future(basic_future &&other) noexcept
            : detail::maybe_copyable<Options::is_shared>{},
              join_base{ std::move(other.join_base::get()) },
              state_{ std::move(other.state_) } {
            if constexpr (!inline_op_state) {
                other.state_.reset();
            }
            if constexpr (!Options::is_always_detached) {
                other.detach();
            }
        }

        /// Destructor
        /**
         * The shared pointer will take care of decrementing the reference
         * counter of the shared state, but we still take care of the special
         * options:
         * - We let stoppable futures set the stop token and wait.
         * - We run the continuations if possible
         */
        ~basic_future() {
            if constexpr (Options::is_stoppable && !Options::is_shared) {
                if (valid() && !is_ready()) {
                    get_stop_source().request_stop();
                }
            }
            wait_if_last();
        }

        /// Copy assignment for shared futures only.
        /**
         * Inherited from base classes.
         */
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
            if constexpr (std::is_same_v<
                              join_base,
                              typename basic_future<U, O>::join_base>) {
                join_base::get() = other.join_base::get();
            } else if constexpr (
                std::is_same_v<typename join_base::value_type, bool>) {
                join_base::get() = false;
            }
            state_ = other.state_; // Make it point to the same shared state
            other.detach();        // Detach other to ensure it won't block at
                                   // destruction
            return *this;
        }

        /// Move assignment.
        /**
         * Inherited from base classes.
         */
        template <class U, class O>
        basic_future &
        operator=(basic_future<U, O> &&other) noexcept {
            if (&other == this) {
                return *this;
            }
            wait_if_last(); // If this is the last shared future waiting for
                            // previous result, we wait
            if constexpr (std::is_same_v<
                              join_base,
                              typename basic_future<U, O>::join_base>) {
                join_base::get() = other.join_base::get();
            } else if constexpr (
                std::is_same_v<typename join_base::value_type, bool>) {
                join_base::get() = false;
            }
            state_ = other.state_; // Make it point to the same shared state
            other.state_.reset();
            other.detach(); // Detach other to ensure it won't block at
                            // destruction
            return *this;
        }
        /// @}

        /// Emplace a function to the shared vector of continuations
        /**
         * If the function is ready, this functions uses the given executor
         * instead of executing with the previous executor.
         *
         * @note This function only participates in overload resolution if the
         * future supports continuations
         *
         * @tparam Executor Executor type
         * @tparam Fn Function type
         * @param ex An executor
         * @param fn A continuation function
         * @return The continuation future
         */
        template <
            class Executor,
            class Fn
#ifndef FUTURES_DOXYGEN
            ,
            bool U = (Options::is_continuable || Options::is_always_deferred),
            std::enable_if_t<
                U && U == (Options::is_continuable || Options::is_always_deferred),
                int> = 0
#endif
            >
        decltype(auto)
        then(const Executor &ex, Fn &&fn) {
            if (!valid()) {
                detail::throw_exception<std::future_error>(
                    std::future_errc::no_state);
            }

            // Determine next future options
            using traits = detail::
                continuation_traits<Executor, Fn, basic_future>;
            using next_value_type = typename traits::next_value_type;
            using next_future_options = typename traits::next_future_options;
            using next_future_type
                = basic_future<next_value_type, next_future_options>;

            if constexpr (Options::is_continuable) {
                // Create task for continuation future
                detail::continuations_source cont_source
                    = state_->get_continuations_source();

                detail::unwrap_and_continue_task<
                    std::decay_t<basic_future>,
                    std::decay_t<Fn>>
                    task{ detail::move_if_not_shared(*this),
                          std::forward<Fn>(fn) };

                // Create shared state for next future
                using operation_state_t = detail::
                    operation_state<next_value_type, next_future_options>;
                auto state = std::make_shared<operation_state_t>(ex);
                next_future_type fut(state);

                // Push task to set next state to this continuation list
                auto apply_fn =
                    [state = std::move(state),
                     task = std::move(task)]() mutable {
                    state->apply(std::move(task));
                };

                auto fn_shared_ptr = std::make_shared<decltype(apply_fn)>(
                    std::move(apply_fn));
                auto copyable_handle = [fn_shared_ptr]() {
                    (*fn_shared_ptr)();
                };

                cont_source.push(ex, copyable_handle);

                return fut;
            } else if constexpr (Options::is_always_deferred) {
                static_assert(is_always_deferred_v<basic_future>);
                static_assert(is_always_deferred_v<next_future_type>);

                // Create task for the continuation future
                detail::unwrap_and_continue_task<
                    std::decay_t<basic_future>,
                    std::decay_t<Fn>>
                    task{ detail::move_if_not_shared(*this),
                          std::forward<Fn>(fn) };

                // Create operation state for the next future
                if constexpr (!Options::is_shared) {
                    static_assert(!is_shared_future_v<basic_future>);
                    static_assert(!is_shared_future_v<next_future_type>);
                    using operation_state_t = detail::deferred_operation_state<
                        next_value_type,
                        next_future_options>;
                    operation_state_t state(ex, std::move(task));

                    next_future_type fut(std::move(state));
                    return fut;
                } else {
                    static_assert(is_shared_future_v<basic_future>);
                    static_assert(is_shared_future_v<next_future_type>);
                    using shared_state_t = detail::
                        operation_state<next_value_type, next_future_options>;
                    auto state = std::make_shared<
                        shared_state_t>(ex, std::move(task));
                    next_future_type fut(state);
                    return fut;
                }
            }
        }

        /// Emplace a function to the shared vector of continuations
        /**
         * If properly setup (by async), this future holds the result from a
         * function that runs these continuations after the main promise is
         * fulfilled. However, if this future is already ready, we can just run
         * the continuation right away.
         *
         * @note This function only participates in overload resolution if the
         * future supports continuations
         *
         * @return True if the contination was emplaced without the using the
         * default executor
         */
        template <
            class Fn
#ifndef FUTURES_DOXYGEN
            ,
            bool U = Options::is_continuable || Options::is_always_deferred,
            std::enable_if_t<
                U && U == (Options::is_continuable || Options::is_always_deferred),
                int> = 0
#endif

            >
        decltype(auto)
        then(Fn &&fn) {
            if constexpr (Options::has_executor) {
                return then(get_executor(), std::forward<Fn>(fn));
            } else {
                return then(make_default_executor(), std::forward<Fn>(fn));
            }
        }

        /// Get this future's continuations source
        /**
         * @note This function only participates in overload resolution if the
         * future supports continuations
         *
         * @return The continuations source
         */
        template <
#ifndef FUTURES_DOXYGEN
            bool U = Options::is_continuable,
            std::enable_if_t<U && U == Options::is_continuable, int> = 0
#endif
            >
        [[nodiscard]] decltype(auto)
        get_continuations_source() const noexcept {
            return state_->get_continuations_source();
        }

        /// Request the future to stop whatever task it's running
        /**
         * @note This function only participates in overload resolution if the
         * future supports stop tokens
         *
         * @return Whether the request was made
         */
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

        /// Get this future's stop source
        /**
         * @note This function only participates in overload resolution if the
         * future supports stop tokens
         *
         * @return The stop source
         */
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

        /// Get this future's stop token
        /**
         * @note This function only participates in overload resolution if the
         * future supports stop tokens
         *
         * @return The stop token
         */
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

        /// Wait until all futures have a valid result and retrieves it
        /**
         * The behaviour depends on shared_based.
         */
        decltype(auto)
        get() {
            if (!valid()) {
                detail::throw_exception<future_uninitialized>();
            }
            if constexpr (Options::is_shared) {
                if constexpr (!inline_op_state) {
                    return state_->get();
                } else {
                    return state_.get();
                }
            } else {
                if constexpr (!inline_op_state) {
                    shared_state_type tmp;
                    tmp.swap(state_);
                    if constexpr (std::is_reference_v<T> || std::is_void_v<T>) {
                        return tmp->get();
                    } else {
                        return T(std::move(tmp->get()));
                    }
                } else {
                    if constexpr (std::is_reference_v<T> || std::is_void_v<T>) {
                        return state_.get();
                    } else {
                        return T(std::move(state_.get()));
                    }
                }
            }
        }


        /// Get exception pointer without throwing exception
        /**
         * This extends std::future so that we can always check if the future
         * threw an exception
         */
        std::exception_ptr
        get_exception_ptr() {
            if (!valid()) {
                detail::throw_exception<future_uninitialized>();
            }
            state_->wait();
            return state_->get_exception_ptr();
        }

        /// Create another future whose state is shared
        /**
         * Create a shared variant of the current future type.
         * If the current type is not shared, the object becomes invalid.
         * If the current type is shared, the new object is equivalent to a
         * copy.
         *
         * @return A shared variant of this future
         */
        basic_future<T, detail::append_future_option_t<shared_opt, Options>>
        share() {
            if (!valid()) {
                detail::throw_exception<future_uninitialized>();
            }
            using shared_options = detail::
                append_future_option_t<shared_opt, Options>;
            using shared_future_t = basic_future<T, shared_options>;
            // this op state might be inline
            // shared future is never inline
            if constexpr (inline_op_state) {
                auto shared_state = std::make_shared<operation_state_type>(
                    std::move(state_));
                auto erased_shared_state = std::dynamic_pointer_cast<
                    detail::operation_state<T, operation_state_options>>(
                    shared_state);
                shared_future_t res{ std::move(erased_shared_state) };
                res.join_base::get() = std::exchange(
                    join_base::get(),
                    Options::is_shared && join_base::get());
                return res;
            } else {
                shared_future_t res{
                    Options::is_shared ? state_ : std::move(state_)
                };
                res.join_base::get() = std::exchange(
                    join_base::get(),
                    Options::is_shared && join_base::get());
                return res;
            }
        }


        /// Checks if the future refers to a shared state
        [[nodiscard]] bool
        valid() const {
            if constexpr (!inline_op_state) {
                return nullptr != state_.get();
            } else {
                return true;
            }
        }

        /// Blocks until the result becomes available.
        void
        wait() const {
            if (!valid()) {
                detail::throw_exception<future_uninitialized>();
            }
            if constexpr (!inline_op_state) {
                state_->wait();
            } else {
                state_.wait();
            }
        }

        /// Waits for the result to become available.
        template <class Rep, class Period>
        std::future_status
        wait_for(
            const std::chrono::duration<Rep, Period> &timeout_duration) const {
            if (!valid()) {
                detail::throw_exception<future_uninitialized>();
            }
            return state_->wait_for(timeout_duration);
        }

        /// Waits for the result to become available.
        template <class Clock, class Duration>
        std::future_status
        wait_until(const std::chrono::time_point<Clock, Duration> &timeout_time)
            const {
            if (!valid()) {
                detail::throw_exception<future_uninitialized>();
            }
            return state_->wait_until(timeout_time);
        }

        /// Checks if the shared state is ready
        [[nodiscard]] bool
        is_ready() const {
            if (!valid()) {
                detail::throw_exception<std::future_error>(
                    std::future_errc::no_state);
            }
            if constexpr (!inline_op_state) {
                return state_->is_ready();
            } else {
                return state_.is_ready();
            }
        }

        /// Tell this future not to join at destruction
        /**
         * For safety, all futures join at destruction by default
         */
        void
        detach() {
            join_base::get() = false;
        }

        /// Notify this condition variable when the future is ready
        notify_when_ready_handle
        notify_when_ready(std::condition_variable_any &cv) {
            if constexpr (!inline_op_state) {
                if (!state_) {
                    detail::throw_exception<future_uninitialized>();
                }
                return state_->notify_when_ready(cv);
            } else {
                return state_.notify_when_ready(cv);
            }
        }

        /// Cancel request to notify this condition variable when the
        /// future is ready
        void
        unnotify_when_ready(notify_when_ready_handle h) {
            if constexpr (!inline_op_state) {
                if (!state_) {
                    detail::throw_exception<future_uninitialized>();
                }
                return state_->unnotify_when_ready(h);
            } else {
                return state_.unnotify_when_ready(h);
            }
        }

        /// Get the current executor for this task
        /**
         * @note This function only participates in overload resolution
         * if future_options has an associated executor
         *
         * @return The executor associated with this future instance
         */
        template <
#ifndef FUTURES_DOXYGEN
            bool U = Options::has_executor,
            std::enable_if_t<U && U == Options::has_executor, int> = 0
#endif
            >
        const typename Options::executor_t &
        get_executor() const {
            static_assert(Options::has_executor);
            if constexpr (!inline_op_state) {
                return state_->get_executor();
            } else {
                return state_.get_executor();
            }
        }

    private:
        /// @name Private Functions
        /// @{

        /// Get a reference to the mutex in the underlying shared state
        std::mutex &
        waiters_mutex() {
            if constexpr (!inline_op_state) {
                if (!state_) {
                    detail::throw_exception<future_uninitialized>();
                }
                return state_->waiters_mutex();
            } else {
                return state_.waiters_mutex();
            }
        }

        void
        wait_if_last() const {
            if (join_base::get() && valid() && (!is_ready())) {
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

        /// @name Members
        /// @{
        /// Pointer to shared state
        mutable unique_or_shared_state state_{};
        /// @}
    };

#ifndef FUTURES_DOXYGEN
    /// Define all basic_futures as a kind of future
    template <typename... Args>
    struct is_future<basic_future<Args...>> : std::true_type
    {};

    /// Define all basic_futures as a future with a ready notifier
    template <typename... Args>
    struct has_ready_notifier<basic_future<Args...>> : std::true_type
    {};

    /// Define shared basic_futures as supporting shared values
    template <class T, class... Args>
    struct is_shared_future<
        basic_future<T, detail::future_options_list<Args...>>>
        : detail::is_in_args<shared_opt, Args...>
    {};

    /// Define continuable basic_futures as supporting lazy continuations
    template <class T, class... Args>
    struct is_continuable<basic_future<T, detail::future_options_list<Args...>>>
        : detail::is_in_args<continuable_opt, Args...>
    {};

    /// Define stoppable basic_futures as being stoppable
    template <class T, class... Args>
    struct is_stoppable<basic_future<T, detail::future_options_list<Args...>>>
        : detail::is_in_args<stoppable_opt, Args...>
    {};

    /// Define stoppable basic_futures as having a stop token
    /**
     * Some futures might be stoppable without a stop token
     */
    template <class T, class... Args>
    struct has_stop_token<basic_future<T, detail::future_options_list<Args...>>>
        : detail::is_in_args<stoppable_opt, Args...>
    {};

    /// Define deferred basic_futures as being deferred
    template <class T, class... Args>
    struct is_always_deferred<
        basic_future<T, detail::future_options_list<Args...>>>
        : detail::is_in_args<always_deferred_opt, Args...>
    {};

    /// Define deferred basic_futures as having an executor
    template <class T, class... Args>
    struct has_executor<basic_future<T, detail::future_options_list<Args...>>>
        : detail::is_type_template_in_args<executor_opt, Args...>
    {};
#endif

    /// A simple future type similar to `std::future`
    /**
     * We should only use this future type for eager tasks that do not
     * expect continuations.
     */
    template <class T, class Executor = default_executor_type>
    using future = basic_future<T, future_options<executor_opt<Executor>>>;

    /// A future type with lazy continuations
    /**
     * Futures with lazy continuations contains a list of continuation tasks
     * to be launched once the main task is complete.
     *
     * This is what a @ref futures::async returns by default when the first
     * function parameter is not a @ref futures::stop_token.
     **/
    template <class T, class Executor = default_executor_type>
    using cfuture = basic_future<
        T,
        future_options<executor_opt<Executor>, continuable_opt>>;

    /// A future type with lazy continuations and stop tokens
    /**
     *  It's a quite common use case that we need a way to cancel futures and
     *  jcfuture provides us with an even better way to do that.
     *
     *  This is what @ref futures::async returns when the first function
     *  parameter is a @ref futures::stop_token
     */
    template <class T, class Executor = default_executor_type>
    using jcfuture = basic_future<
        T,
        future_options<executor_opt<Executor>, continuable_opt, stoppable_opt>>;

    /// A deferred future type
    /**
     * This is a future type whose main task will only be launched when we
     * wait for its results from another execution context.
     *
     * This is what the function @ref schedule returns when the first task
     * parameter is not a stop token.
     *
     * The state of this future stores the function to be run.
     *
     * This future type supports continuations without the continuation lists
     * of continuable futures.
     *
     */
    template <class T, class Executor = default_executor_type>
    using dfuture = basic_future<
        T,
        future_options<executor_opt<Executor>, always_deferred_opt>>;

    /// A deferred stoppable future type
    /**
     * This is a future type whose main task will only be launched when we
     * wait for its results from another execution context.
     *
     * Once the task is launched, it can be requested to stop through its
     * stop source.
     *
     * This is what the function @ref schedule returns when the first task
     * parameter is a stop token.
     */
    template <class T, class Executor = default_executor_type>
    using jdfuture = basic_future<
        T,
        future_options<executor_opt<Executor>, always_deferred_opt>>;

    /// A future that simply holds a ready value
    /**
     * This is the future type we use for constant values. This is the
     * future type we usually return from functions such as
     * @ref make_ready_future.
     *
     * These futures have no support for associated executors, continuations,
     * or deferred tasks.
     *
     * Like deferred futures, the operation state is stored inline.
     *
     */
    template <class T>
    using vfuture = basic_future<T, future_options<>>;

    /// A simple std::shared_future
    /**
     * This is what a futures::future::share() returns
     */
    template <class T, class Executor = default_executor_type>
    using shared_future
        = basic_future<T, future_options<executor_opt<Executor>, shared_opt>>;

    /// A shared future type with lazy continuations
    /**
     * This is what a @ref futures::cfuture::share() returns
     */
    template <class T, class Executor = default_executor_type>
    using shared_cfuture = basic_future<
        T,
        future_options<executor_opt<Executor>, continuable_opt, shared_opt>>;

    /// A shared future type with lazy continuations and stop tokens
    /**
     * This is what a @ref futures::jcfuture::share() returns
     */
    template <class T, class Executor = default_executor_type>
    using shared_jcfuture = basic_future<
        T,
        future_options<
            executor_opt<Executor>,
            continuable_opt,
            stoppable_opt,
            shared_opt>>;

    /// A shared future type with deferred task
    /**
     * This is what a @ref futures::dfuture::share() returns
     */
    template <class T, class Executor = default_executor_type>
    using shared_dfuture = basic_future<
        T,
        future_options<executor_opt<Executor>, always_deferred_opt, shared_opt>>;

    /// A shared future type with deferred task and stop token
    /**
     * This is what a @ref futures::jdfuture::share() returns
     */
    template <class T, class Executor = default_executor_type>
    using shared_jdfuture = basic_future<
        T,
        future_options<
            executor_opt<Executor>,
            stoppable_opt,
            always_deferred_opt,
            shared_opt>>;

    /// A shared future that simply holds a ready value
    template <class T>
    using shared_vfuture = basic_future<T, future_options<shared_opt>>;

    /** @} */
    /** @} */
} // namespace futures

#endif // FUTURES_FUTURES_BASIC_FUTURE_HPP
