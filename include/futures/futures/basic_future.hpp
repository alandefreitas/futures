//
// Copyright (c) 2021 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_BASIC_FUTURE_H
#define FUTURES_BASIC_FUTURE_H

#include <futures/executor/default_executor.hpp>
#include <futures/futures/stop_token.hpp>
#include <futures/futures/traits/is_executor_then_function.hpp>
#include <futures/futures/traits/is_future.hpp>
#include <futures/futures/detail/continuations_source.hpp>
#include <futures/futures/detail/empty_base.hpp>
#include <futures/futures/detail/shared_state.hpp>
#include <futures/futures/detail/throw_exception.hpp>
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
        template <typename R>
        class promise_base;
        struct async_future_scheduler;
        struct internal_then_functor;
    } // namespace detail
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
    class basic_future
#ifndef FUTURES_DOXYGEN
        : private detail::maybe_empty<
              std::conditional_t<
                  LazyContinuable::value,
                  detail::continuations_source,
                  detail::empty_value_type>,
              0>
        , private detail::maybe_empty<
              std::conditional_t<
                  Stoppable::value,
                  stop_source,
                  detail::empty_value_type>,
              1>
#endif
    {
    private:
        using lazy_continuations_base = detail::maybe_empty<
            std::conditional_t<
                LazyContinuable::value,
                detail::continuations_source,
                detail::empty_value_type>,
            0>;
        using stop_token_base = detail::maybe_empty<
            std::conditional_t<
                Stoppable::value,
                stop_source,
                detail::empty_value_type>,
            1>;
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
              stop_token_base([]() {
                  if constexpr (is_stoppable_v) {
                      return nostopstate;
                  } else {
                      return detail::empty_value_type{};
                  }
              }()), // Stop token false, but stop token
                    // parameter should be set
              state_{ nullptr } {}

        /// \brief Construct from a pointer to the shared state
        ///
        /// This constructor is private because we need to ensure the launching
        /// function appropriately sets this std::future handling these traits
        /// This is a function for async.
        ///
        /// \param s Future shared state
        explicit basic_future(
            const std::shared_ptr<detail::shared_state<T>> &s) noexcept
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
            : lazy_continuations_base(
                other.lazy_continuations_base::get()), // Copy reference to
                                                       // continuations
              stop_token_base(
                  other.stop_token_base::get()), // Copy reference to stop state
              state_{ other.state_ } {
            static_assert(
                is_shared_v,
                "Copy constructor is only available for shared futures");
        }

        /// \brief Move constructor.
        ///
        /// Inherited from base classes.
        basic_future(basic_future &&other) noexcept
            : lazy_continuations_base(std::move(
                other.lazy_continuations_base::get())),      // Get control of
                                                             // continuations
              stop_token_base(other.stop_token_base::get()), // Move stop state
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
                    get_stop_source().request_stop();
                }
            }
            wait_if_last();
            if constexpr (is_lazy_continuable_v) {
                detail::continuations_source &cs = lazy_continuations_base::
                    get();
                if (cs.run_possible()) {
                    cs.request_run();
                }
            }
        }

        /// \brief Copy assignment for shared futures only.
        ///
        /// Inherited from base classes.
        basic_future &
        operator=(const basic_future &other) {
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
        basic_future &
        operator=(basic_future &&other) noexcept {
            if (&other == this) {
                return *this;
            }
            wait_if_last(); // If this is the last shared future waiting for
                            // previous result, we wait
            lazy_continuations_base::operator=(
                std::move(other));             // Get control of continuations
            stop_token_base::operator=(other); // Move stop state
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
            bool U = is_lazy_continuable_v,
            std::enable_if_t<U && U == is_lazy_continuable_v, int> = 0
#endif
            >
        bool
        then(const Executor &ex, Fn &&fn) {
            if (!valid()) {
                detail::throw_exception<std::future_error>(
                    std::future_errc::no_state);
            }
            if (!is_ready() && lazy_continuations_base::get().run_possible()) {
                return lazy_continuations_base::get()
                    .emplace_continuation(ex, std::forward<Fn>(fn));
            } else {
                // When the shared state currently associated with *this is
                // ready, the continuation is called on an unspecified
                // thread of execution
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
            bool U = is_lazy_continuable_v,
            std::enable_if_t<U && U == is_lazy_continuable_v, int> = 0
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
            bool U = is_stoppable_v,
            std::enable_if_t<U && U == is_stoppable_v, int> = 0
#endif
            >
        bool
        request_stop() noexcept {
            return stop_token_base::get().request_stop();
        }

        /// \brief Get this future's stop source
        ///
        /// \note This function only participates in overload resolution if the
        /// future supports stop tokens
        ///
        /// \return The stop source
        template <
#ifndef FUTURES_DOXYGEN
            bool U = is_stoppable_v,
            std::enable_if_t<U && U == is_stoppable_v, int> = 0
#endif
            >
        [[nodiscard]] stop_source
        get_stop_source() const noexcept {
            return stop_token_base::get();
        }

        /// \brief Get this future's stop token
        ///
        /// \note This function only participates in overload resolution if the
        /// future supports stop tokens
        ///
        /// \return The stop token
        template <
#ifndef FUTURES_DOXYGEN
            bool U = is_stoppable_v,
            std::enable_if_t<U && U == is_stoppable_v, int> = 0
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
            if constexpr (is_shared_v) {
                return state_->get();
            } else {
                std::shared_ptr<detail::shared_state<T>> tmp;
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
        basic_future_shared_version_t
        share() {
            if (!valid()) {
                detail::throw_exception<future_uninitialized>();
            }
            basic_future_shared_version_t res{
                is_shared_v ? state_ : std::move(state_)
            };
            res.join_ = std::exchange(join_, is_shared_v && join_);
            if constexpr (is_lazy_continuable_v) {
                res.lazy_continuations_base::get()
                    = this->lazy_continuations_base::get();
            }
            if constexpr (is_stoppable_v) {
                res.stop_token_base::get() = this->stop_token_base::get();
            }
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

        /// \brief Get a reference to the mutex in the underlying shared state
        std::mutex &
        mutex() {
            if (!state_) {
                detail::throw_exception<future_uninitialized>();
            }
            return state_->waiters_mutex();
        }

    private:
        /// \name Private Functions
        /// @{

        void
        wait_if_last() const {
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
        void
        set_stop_source(const stop_source &ss) noexcept {
            stop_token_base::get() = ss;
        }
        void
        set_continuations_source(
            const detail::continuations_source &cs) noexcept {
            lazy_continuations_base::get() = cs;
        }
        detail::continuations_source
        get_continuations_source() const noexcept {
            return lazy_continuations_base::get();
        }

        /// @}

        /// \name Members
        /// @{
        bool join_{ true };

        /// \brief Pointer to shared state
        std::shared_ptr<detail::shared_state<T>> state_{};
        /// @}
    };

#ifndef FUTURES_DOXYGEN
    /// \name Define basic_future as a kind of future
    /// @{
    template <typename... Args>
    struct is_future<basic_future<Args...>> : std::true_type
    {};
    /// @}

    /// \name Define basic_future as a kind of future
    /// @{
    template <typename... Args>
    struct has_ready_notifier<basic_future<Args...>> : std::true_type
    {};
    /// @}

    /// \name Define basic_futures as supporting lazy continuations
    /// @{
    template <class T, class SH, class L, class ST>
    struct is_shared_future<basic_future<T, SH, L, ST>> : SH
    {};
    /// @}

    /// \name Define basic_futures as supporting lazy continuations
    /// @{
    template <class T, class SH, class L, class ST>
    struct is_lazy_continuable<basic_future<T, SH, L, ST>> : L
    {};
    /// @}

    /// \name Define basic_futures as having a stop token
    /// @{
    template <class T, class S, class L, class Stoppable>
    struct has_stop_token<basic_future<T, S, L, Stoppable>> : Stoppable
    {};
    /// @}

    /// \name Define basic_futures as being stoppable (not the same as having a
    /// stop token for other future types)
    /// @{
    template <class T, class S, class L, class Stoppable>
    struct is_stoppable<basic_future<T, S, L, Stoppable>> : Stoppable
    {};
/** @} */
#endif


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

    /** @} */
    /** @} */
} // namespace futures

#endif // FUTURES_BASIC_FUTURE_H
