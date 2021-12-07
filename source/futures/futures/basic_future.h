//
// Created by Alan Freitas on 8/21/21.
//

#ifndef FUTURES_BASIC_FUTURE_H
#define FUTURES_BASIC_FUTURE_H

#include <functional>
#include <shared_mutex>
#include <utility>

#include <futures/executor/default_executor.h>

#include "futures/futures/traits/is_executor_then_function.h"
#include <futures/adaptor/detail/traits/is_executor_then_continuation.h>

#include <futures/futures/detail/continuations_source.h>
#include <futures/futures/detail/shared_state.h>
#include <futures/futures/stop_token.h>

namespace futures {
    /** \addtogroup futures Futures
     *  @{
     */

    /** \addtogroup future-types Future types
     *  @{
     */

    // Fwd-declaration
    template <class T, class Shared, class LazyContinuable, class Stoppable> class basic_future;

    /// \brief A simple future type similar to std::future
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

    /// @}

    namespace detail {
        /// \name Helpers to declare internal_async a friend
        /// These helpers also help us deduce what kind of types we will return from `async` and `then`
        /// @{

        /// @}

        // Fwd-declare
        template <typename R> struct promise_base;
        class async_future_scheduler;
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

            /// \brief Emplace a function to the shared vector of continuations
            /// If properly setup (by async), this future holds the result from a function that runs these
            /// continuations after the main promise is fulfilled.
            /// However, if this future is already ready, we can just run the continuation right away.
            /// \return True if the contination was emplaced without the using the default executor
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

    // fwd-declare
    template <typename Signature> class packaged_task;

    /// \brief The common implementation for a future that might have lazy continuations and stop tokens
    /// Note that these classes only provide the capabilities of tracking these features
    /// Setting up these capabilities (creating tokens or setting main future to run continuations)
    /// needs to be done when the future is created.
    /// All this behavior is encapsulated in the async function.
    template <class T, class Shared, class LazyContinuable, class Stoppable>
    class
#if defined(__clang__) && !defined(__apple_build_version__)
        [[clang::preferred_name(jfuture<T>), clang::preferred_name(cfuture<T>), clang::preferred_name(jcfuture<T>),
          clang::preferred_name(shared_jfuture<T>), clang::preferred_name(shared_cfuture<T>),
          clang::preferred_name(shared_jcfuture<T>)]]
#endif
        basic_future
        : public detail::lazy_continuations_base<LazyContinuable, basic_future<T, Shared, LazyContinuable, Stoppable>,
                                                 T>,
          public detail::stop_token_base<Stoppable, basic_future<T, Shared, LazyContinuable, Stoppable>, T> {
      private:
        /// \name Private types
        /// @{

        /// \brief Pointer to shared state
        using ptr_type = typename detail::shared_state<T>::ptr_type;
        /// @}

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
        using lazy_continuations_base =
            detail::lazy_continuations_base<LazyContinuable, basic_future<T, Shared, LazyContinuable, Stoppable>, T>;
        using stop_token_base =
            detail::stop_token_base<Stoppable, basic_future<T, Shared, LazyContinuable, Stoppable>, T>;
        friend lazy_continuations_base;
        friend stop_token_base;
        /// @}

        /// \name Shared state counterparts
        /// @{

        // Other shared state types can access the shared state constructor directly
        friend struct detail::promise_base<T>;

        template <typename Signature> friend class packaged_task;

        // The shared variant is always a friend
        using basic_future_shared_version_t = basic_future<T, std::true_type, LazyContinuable, Stoppable>;
        friend basic_future_shared_version_t;

        using basic_future_unique_version_t = basic_future<T, std::false_type, LazyContinuable, Stoppable>;
        friend basic_future_unique_version_t;

        friend class detail::async_future_scheduler;
        friend struct detail::internal_then_functor;

        /// @}

      private:
        /// \name Private functions used by async/then/when_all/when_any to set up futures
        /// @{

        /// \brief Construct from a pointer to the shared state
        /// This constructor is private because we need to ensure the launching
        /// function appropriately sets this std::future handling these traits
        /// This is a function for async.
        explicit basic_future(const ptr_type &p) noexcept
            : lazy_continuations_base(), // No continuations at constructions, but callbacks should be set
              stop_token_base(),         // Stop token false, but stop token parameter should be set
              state_{std::move(p)} {}

        /// \subsection Private getters and setters
        void set_stop_source(const stop_source &ss) noexcept { stop_token_base::stop_source_ = ss; }
        void set_continuations_source(const detail::continuations_source &cs) noexcept {
            lazy_continuations_base::continuations_source_ = cs;
        }
        detail::continuations_source get_continuations_source() const noexcept {
            return lazy_continuations_base::continuations_source_;
        }
        /// @}
      public:
        /// \name Constructors
        /// @{

        /// \brief Default constructor.
        /// Null shared state. Properties inherited from base classes.
        basic_future() noexcept
            : lazy_continuations_base(), // No continuations at constructions, but callbacks should be set
              stop_token_base(),         // Stop token false, but stop token parameter should be set
              state_{nullptr} {}

        /// \brief Copy constructor for shared futures only.
        /// Inherited from base classes.
        basic_future(const basic_future &other)
            : lazy_continuations_base(other), // Copy reference to continuations
              stop_token_base(other),         // Copy reference to stop state
              state_{other.state_} {
            static_assert(is_shared_v, "Copy constructor is only available for shared futures");
        }

        /// \brief Move constructor. Inherited from base classes.
        basic_future(basic_future && other) noexcept
            : lazy_continuations_base(std::move(other)), // Get control of continuations
              stop_token_base(std::move(other)),         // Move stop state
              join_{other.join_}, state_{other.state_} {
            other.state_.reset();
        }

        /// \brief Copy assignment for shared futures only.
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

        /// \brief Move assignment. Inherited from base classes.
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

        /// \brief Destructor
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

        /// \brief Wait until all futures have a valid result and retrieves it
        /// The behaviour depends on shared_based.
        decltype(auto) get() {
            if (!valid()) {
                throw future_uninitialized{};
            }
            if constexpr (is_shared_v) {
                return state_->get();
            } else {
                ptr_type tmp{};
                tmp.swap(state_);
                if constexpr (std::is_reference_v<T> || std::is_void_v<T>) {
                    return tmp->get();
                } else {
                    return T(std::move(tmp->get()));
                }
            }
        }

        /// \brief Create another future whose state is shared
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
        /// For safety, all futures join at destruction
        void detach() { join_ = false; }

      private:
        /// \name Private Functions
        /// @{

        void wait_if_last() const {
            if (join_ && valid() && (!is_ready())) {
                if constexpr (!is_shared_v) {
                    wait();
                } else /* constexpr */ {
                    if (1 == state_->use_count()) {
                        wait();
                    }
                }
            }
        }

        /// @}

      private:
        /// \name Members
        /// @{
        bool join_{true};

        /// \brief Pointer to shared state
        ptr_type state_{};
        /// @}
    };

    /// \name Define basic_future as a kind of future
    /// @{
    template <typename... Args> struct is_future<basic_future<Args...>> : std::true_type {};
    template <typename... Args> struct is_future<basic_future<Args...> &> : std::true_type {};
    template <typename... Args> struct is_future<basic_future<Args...> &&> : std::true_type {};
    template <typename... Args> struct is_future<const basic_future<Args...>> : std::true_type {};
    template <typename... Args> struct is_future<const basic_future<Args...> &> : std::true_type {};
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
    /** @} */
} // namespace futures

#endif // FUTURES_BASIC_FUTURE_H
