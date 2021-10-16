//
// Created by Alan Freitas on 8/21/21.
//

#ifndef CPP_MANIFEST_BASIC_FUTURE_H
#define CPP_MANIFEST_BASIC_FUTURE_H

#include <functional>
#include <shared_mutex>
#include <utility>

#include <small/vector.h>

#include "stop_token.h"
#include "traits/is_executor_then_continuation.h"
#include "traits/is_executor_then_function.h"

namespace futures {
    /// \section Forward declarations for base classes
    namespace detail {
        template <class T, class Shared, class LazyContinuable, class Stoppable> class basic_future;
    }

    /// \brief A simple std::future
    ///
    /// This is what a std::async returns
    template <class T> using future = std::future<T>;

    /// \brief A future type with stop tokens
    ///
    /// This class is like a version of jthread for futures
    ///
    /// It's a quite common use case that we need a way to cancel futures and
    /// jfuture provides us with an even better way to do that.
    ///
    /// @ref async is adapted to return a jcfuture whenever the first callable argument
    /// is a stop token.
    template <class T> using jfuture = detail::basic_future<T, std::false_type, std::false_type, std::true_type>;

    /// \brief A future type with lazy continuations
    ///
    /// This is what a @ref futures::async returns when the first function parameter is not a @ref futures::stop_token
    template <class T> using cfuture = detail::basic_future<T, std::false_type, std::true_type, std::false_type>;

    /// \brief A future type with lazy continuations and stop tokens
    ///
    /// This is what a @ref futures::async returns when the first function parameter is a @ref futures::stop_token
    template <class T> using jcfuture = detail::basic_future<T, std::false_type, std::true_type, std::true_type>;

    /// \brief A future type with lazy continuations and stop tokens
    ///
    /// This is what a @ref futures::async returns when the first function parameter is a @ref futures::stop_token
    ///
    /// Same as @ref jcfuture
    template <class T> using cjfuture = jcfuture<T>;

    /// \brief A simple std::shared_future
    ///
    /// This is what a std::future::share() returns
    template <class T> using shared_future = std::shared_future<T>;

    /// \brief A shared future type with stop tokens
    ///
    /// This is what a @ref futures::jfuture::share() returns
    template <class T> using shared_jfuture = detail::basic_future<T, std::true_type, std::false_type, std::true_type>;

    /// \brief A shared future type with lazy continuations
    ///
    /// This is what a @ref futures::cfuture::share() returns
    template <class T> using shared_cfuture = detail::basic_future<T, std::true_type, std::true_type, std::false_type>;

    /// \brief A shared future type with lazy continuations and stop tokens
    ///
    /// This is what a @ref futures::jcfuture::share() returns
    template <class T> using shared_jcfuture = detail::basic_future<T, std::true_type, std::true_type, std::true_type>;

    /// \brief A shared future type with lazy continuations and stop tokens
    ///
    /// This is what a @ref futures::cjfuture::share() returns
    template <class T> using shared_cjfuture = shared_jcfuture<T>;

    namespace detail {
        /// \section Helpers to declare internal_async a friend
        /// These helpers also help us deduce what kind of types we will return from `async` and `then`

        /// Return T::type, or void if it doesn't exist, to avoid errors in std::conditional
        template <class, class = void> struct type_member_or_void { using type = void; };
        template <class T> struct type_member_or_void<T, std::void_t<typename T::type>> {
            using type = typename T::type;
        };
        template <class T> using type_member_or_void_t = typename type_member_or_void<T>::type;

        /// The value_type in a future from this function
        template <typename Function>
        using async_result_of =
            std::conditional_t<std::is_invocable_v<std::decay_t<Function>, stop_token>,
                               type_member_or_void_t<std::invoke_result<std::decay_t<Function>, stop_token>>,
                               type_member_or_void_t<std::invoke_result<std::decay_t<Function>>>>;

        /// \brief The result future of calling async with a function (jcfuture or cfuture)
        /// Whenever we call async, we return a future with lazy continuations by default because
        /// we don't know if the user will need `then` (even though then also works for future types
        /// without lazy continuations.
        /// Also, when the function expects a stop token, we return a jfuture.
        template <typename Function>
        using async_future_result_of =
            std::conditional_t<std::is_invocable_v<std::decay_t<Function>, stop_token>,
                               jcfuture<async_result_of<Function>>, cfuture<async_result_of<Function>>>;

        /// \brief The result we get from the `then` function
        /// - If after function expects a stop token:
        ///   - If previous future is stoppable and not-shared: return jcfuture with shared stop source
        ///   - Otherwise:                                      return jcfuture with new stop source
        /// - If after function does not expect a stop token:
        ///   - If previous future is stoppable and not-shared: return jcfuture with shared stop source
        ///   - Otherwise:                                      return cfuture with no stop source
        template <typename Function, typename Future> struct then_result_of {
          private:
            constexpr static bool continuation_expects_stop_token =
                is_future_continuation_v<std::decay_t<Function>, Future, stop_token>;
            constexpr static bool previous_future_has_stop_token = has_stop_token_v<Future>;
            constexpr static bool previous_future_is_shared = is_shared_future_v<Future>;
            constexpr static bool inherit_stop_token = previous_future_has_stop_token && not previous_future_is_shared;
            constexpr static bool after_has_stop_token = continuation_expects_stop_token || inherit_stop_token;
            using decay_function = std::decay_t<Function>;
            using argtuple = std::conditional_t<continuation_expects_stop_token, std::tuple<stop_token>, std::tuple<>>;
            using continuation_result = continuation_result<decay_function, Future, argtuple>;
            using result_value_type = type_member_or_void_t<continuation_result>;

          public:
            using type =
                std::conditional_t<after_has_stop_token, jcfuture<result_value_type>, cfuture<result_value_type>>;
        };

        template <typename Function, typename Future>
        using then_result_of_t = typename then_result_of<Function, Future>::type;

        /// \class Enable a future class a whose future is shared
        /// Shared futures can move copied and their state can be retrieved more than once
        template <class Derived, class T> class enable_shared {
          protected:
            /// \brief Storage for the underlying std::future
            /// We store the std::future directly to maintain compatibility with other functions while not being
            /// able to access the underlying state of this future.
            /// We use a shared_ptr rather than the future directly because we can't directly access the number of
            /// references to the state, and we need those to know when the last shared future is being destroyed.
            /// The state of a basic_future, even a shared basic_future, is encapsulated and not expected to be
            /// shared with other future types. Nevertheless, if the is state is shared, at worst, this object
            /// might at most wait for a task to complete ("join" the future) sooner than it had to, unless it's
            /// detached.
            using internal_future_type = std::shared_ptr<std::shared_future<T>>;

          public:
            enable_shared() = default;
            enable_shared(const enable_shared &c) = default;
            enable_shared &operator=(const enable_shared &c) = default;
            enable_shared(enable_shared &&c) noexcept = default;
            enable_shared &operator=(enable_shared &&c) noexcept = default;

          protected:
            explicit enable_shared(const internal_future_type &f) : future_(f){};
            explicit enable_shared(internal_future_type &&f) : future_(std::move(f)){};

            internal_future_type future_;
        };

        template <class Derived, class T> class disable_shared {
          protected:
            using internal_future_type = std::unique_ptr<std::future<T>>;

            template <typename BasicFuture> struct basic_future_shared_version {
                using type = basic_future<T, std::true_type, typename BasicFuture::is_lazy_continuable,
                                          typename BasicFuture::is_stoppable>;
            };

            template <typename BT, typename SH, typename L, typename ST>
            struct basic_future_shared_version<basic_future<BT, SH, L, ST>> {
                using type = basic_future<T, std::true_type, L, ST>;
            };

            template <typename F> using basic_future_shared_version_t = typename basic_future_shared_version<F>::type;

          public:
            disable_shared() = default;
            disable_shared(const disable_shared &c) = delete;
            disable_shared &operator=(const disable_shared &c) = delete;
            disable_shared(disable_shared &&c) noexcept = default;
            disable_shared &operator=(disable_shared &&c) noexcept = default;

            /// \brief Create another future whose state is shared
            /// While future_.share() will return a std::shared_future, we need to
            /// move this std::shared_future into the corresponding derived class
            /// so that features aren't disabled:
            /// - jfuture -> shared_jfuture
            /// - cfuture -> shared_cfuture
            /// - jcfuture -> shared_jcfuture
            /// \return
            basic_future_shared_version_t<Derived> share() {
                static_assert(not Derived::is_shared_v);
                auto shared_std_future = std::move(future_->share());
                basic_future_shared_version_t<Derived> res(
                    std::make_shared<decltype(shared_std_future)>(shared_std_future));
                // Shares the source of continuations
                if constexpr (Derived::is_lazy_continuable::value) {
                    res.continuations_source_ = static_cast<Derived *>(this)->continuations_source_;
                }
                // Shares the source for the stop token
                if constexpr (Derived::is_stoppable::value) {
                    res.stop_source_ = static_cast<Derived *>(this)->stop_source_;
                }
                return res;
            }

          protected:
            disable_shared(const internal_future_type &f) = delete;
            disable_shared(internal_future_type &&f) : future_(std::move(f)){};

            internal_future_type future_;
        };

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
            enable_stop_token &operator=(const enable_stop_token &c) = default;
            /// \brief Move construct/assign helper for the
            /// When the basic future is being moved, the stop source gets copied instead of moved,
            /// because we only want to move the future state, but we don't want to invalidate the
            /// stop token of a function that is already running. The user could copy the source
            /// before moving the future.
            enable_stop_token(enable_stop_token &&c) noexcept : stop_source_(c.stop_source_){};
            enable_stop_token &operator=(enable_stop_token &&c) noexcept {
                stop_source_ = c.stop_source_;
                return *this;
            };

            bool request_stop() noexcept { return get_stop_source().request_stop(); }

            stop_source get_stop_source() noexcept { return stop_source_; }

            [[nodiscard]] stop_token get_stop_token() const noexcept { return stop_source_.get_token(); }

          protected:
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
            disable_stop_token &operator=(const disable_stop_token &c) = default;
            disable_stop_token(disable_stop_token &&c) noexcept = default;
            disable_stop_token &operator=(disable_stop_token &&c) noexcept = default;
        };

        /// \brief The continuation state as a small thread safe container with stability
        ///
        /// The whole logic here is very similar to stop_tokens.
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
            using continuation_vector = small::vector<continuation_ptr>;

          public:
            ~continuations_state() { request_run(); }

          public:
            /// \section Non-modifying

            /// \brief Get number of continuations
            size_t size() const {
                std::shared_lock lock(continuations_mutex_);
                return continuations_.size();
            }

            /// \brief Get the i-th continuation
            /// The return reference is safe (in context) because the continuation vector has stability
            continuation_type &operator[](size_t index) const {
                std::shared_lock lock(continuations_mutex_);
                return continuations_.at(index).operator*();
            }

            /// \section Modifying

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
            bool is_run_requested() {
                std::shared_lock lock(run_requested_mutex_);
                return run_requested_;
            }

            /// \brief Check if some source asked already asked for the continuations to run
            bool is_run_possible() { return not is_run_requested(); }

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
            [[nodiscard]] bool run_requested() const noexcept {
                return state_ != nullptr && state_->is_run_requested();
            }

            /// \brief Checks if the continuations_token object has associated continuations-state, and that state
            /// either has already had a run requested or it has associated continuations_source object(s)
            [[nodiscard]] bool run_possible() const noexcept {
                return state_ != nullptr && not state_->is_run_requested();
            }

            /// \brief compares two std::run_token objects
            [[nodiscard]] friend bool operator==(const continuations_token &lhs,
                                                 const continuations_token &rhs) noexcept {
                return lhs.state_ == rhs.state_;
            }

            [[nodiscard]] friend bool operator!=(const continuations_token &lhs,
                                                 const continuations_token &rhs) noexcept {
                return lhs.state_ != rhs.state_;
            }

          private:
            friend class continuations_source;
            explicit continuations_token(std::shared_ptr<continuations_state> state) noexcept
                : state_(std::move(state)) {}

          private:
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
            bool request_run() {
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
            [[nodiscard]] bool run_requested() const noexcept {
                return state_ != nullptr && state_->is_run_requested();
            }

            /// \brief Checks if the continuations_source object has a continuations-state.
            [[nodiscard]] bool run_possible() const noexcept { return state_ != nullptr; }

            /// \brief Compares two continuations_source values
            [[nodiscard]] friend bool operator==(const continuations_source &a,
                                                 const continuations_source &b) noexcept {
                return a.state_ == b.state_;
            }

            /// \brief Compares two continuations_source values
            [[nodiscard]] friend bool operator!=(const continuations_source &a,
                                                 const continuations_source &b) noexcept {
                return a.state_ != b.state_;
            }

          private:
            std::shared_ptr<continuations_state> state_;
        };

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

          protected:
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

        /// \brief The common implementation for a future that might have lazy continuations and stop tokens
        /// Note that these classes only provide the capabilities of tracking these features
        /// Setting up these capabilities (creating tokens or setting main future to run continuations)
        /// needs to be done when the future is created.
        /// All this behavior is encapsulated in the async function.
        template <class T, class Shared, class LazyContinuable, class Stoppable>
        class [[clang::preferred_name(jfuture<T>), clang::preferred_name(cfuture<T>),
                clang::preferred_name(jcfuture<T>), clang::preferred_name(shared_jfuture<T>),
                clang::preferred_name(shared_cfuture<T>), clang::preferred_name(shared_jcfuture<T>)]] basic_future
            : public std::conditional_t<Shared::value,
                                        enable_shared<basic_future<T, Shared, LazyContinuable, Stoppable>, T>,
                                        disable_shared<basic_future<T, Shared, LazyContinuable, Stoppable>, T>>,
              public std::conditional_t<
                  LazyContinuable::value,
                  enable_lazy_continuations<basic_future<T, Shared, LazyContinuable, Stoppable>, T>,
                  disable_lazy_continuations<basic_future<T, Shared, LazyContinuable, Stoppable>, T>>,
              public std::conditional_t<Stoppable::value,
                                        enable_stop_token<basic_future<T, Shared, LazyContinuable, Stoppable>, T>,
                                        disable_stop_token<basic_future<T, Shared, LazyContinuable, Stoppable>, T>> {
          private:
            /// \section Private types

            /// \subsection Convenience aliases to refer to base classes
            using shared_base =
                std::conditional_t<Shared::value, enable_shared<basic_future<T, Shared, LazyContinuable, Stoppable>, T>,
                                   disable_shared<basic_future<T, Shared, LazyContinuable, Stoppable>, T>>;
            using lazy_continuations_base =
                std::conditional_t<LazyContinuable::value,
                                   enable_lazy_continuations<basic_future<T, Shared, LazyContinuable, Stoppable>, T>,
                                   disable_lazy_continuations<basic_future<T, Shared, LazyContinuable, Stoppable>, T>>;
            using stop_token_base =
                std::conditional_t<Stoppable::value,
                                   enable_stop_token<basic_future<T, Shared, LazyContinuable, Stoppable>, T>,
                                   disable_stop_token<basic_future<T, Shared, LazyContinuable, Stoppable>, T>>;

            using internal_future_type = typename shared_base::internal_future_type;

          public:
            /// \section Public types
            using value_type = T;
            using is_shared = Shared;
            using is_lazy_continuable = LazyContinuable;
            using is_stoppable = Stoppable;
            static constexpr bool is_shared_v = Shared::value;
            static constexpr bool is_lazy_continuable_v = LazyContinuable::value;
            static constexpr bool is_stoppable_v = Stoppable::value;

            /// \section Make async/then/when_all/when_any functions friends
            template <typename ScheduleMethod, typename Executor, typename Function,
                      std::enable_if_t<detail::is_executor_then_async_input_v<Executor, Function>, int>>
            friend async_future_result_of<Function> internal_launch_async(const Executor &ex, Function &&f);

            template <typename TExecutor, typename TFunction, class TFuture,
                      std::enable_if_t<is_executor_then_continuation_v<TExecutor, TFunction, TFuture>, int>>
            friend then_result_of_t<TFunction, TFuture> internal_then(const TExecutor &ex, TFuture &&before,
                                                                      TFunction &&after);

            template <typename U> friend cfuture<typename std::decay_t<U>> internal_make_ready_cfuture(U && value);
            template <typename U> friend cfuture<U &> internal_make_ready_cfuture(std::reference_wrapper<U> value);
            inline friend cfuture<void> internal_make_ready_cfuture();
            template <typename U> friend jcfuture<typename std::decay_t<U>> internal_make_ready_jcfuture(U && value);
            template <typename U> friend jcfuture<U &> internal_make_ready_jcfuture(std::reference_wrapper<U> value);
            inline friend jcfuture<void> internal_make_ready_jcfuture();

            /// \section Make base classes friends
            template <class D, class DT> friend class enable_shared;
            template <class D, class DT> friend class disable_shared;
            template <class D, class DT> friend class enable_stop_token;
            template <class D, class DT> friend class disable_stop_token;
            template <class D, class DT> friend class enable_lazy_continuations;
            template <class D, class DT> friend class disable_lazy_continuations;

          private:
            /// \section Private functions used by async/then/when_all/when_any to set up futures

            /// \brief Create a basic_future from an existing std::future
            /// This constructor is private because we need to ensure the launching
            /// function appropriately sets this std::future handling these traits
            /// This is a function for async.
            explicit basic_future(internal_future_type && f) noexcept
                : shared_base(std::move(f)), // Get the state from std::future, which should be set up properly
                  lazy_continuations_base(), // No continuations at constructions, but callbacks should be set
                  stop_token_base() {}       // Stop token false, but stop token parameter should be set

            /// \subsection Private getters and setters
            void set_future(internal_future_type && f) noexcept { shared_base::future_ = std::move(f); }
            void set_future(const internal_future_type &f) noexcept { shared_base::future_ = f; }
            internal_future_type &get_future() noexcept { return shared_base::future_; }
            void set_stop_source(const stop_source &ss) noexcept { stop_token_base::stop_source_ = ss; }
            void set_continuations_source(const continuations_source &cs) noexcept {
                lazy_continuations_base::continuations_source_ = cs;
            }
            continuations_source get_continuations_source() noexcept {
                return lazy_continuations_base::continuations_source_;
            }

          public:
            /// \section Constructors

            /// \brief Default constructor. Properties inherited from base classes.
            basic_future() noexcept
                : shared_base(),             // Future in an invalid state
                  lazy_continuations_base(), // No continuations at constructions, but callbacks should be set
                  stop_token_base() {}       // Stop token false, but stop token parameter should be set

            /// \brief Move constructor. Inherited from base classes.
            basic_future(basic_future && other) noexcept
                : shared_base(std::move(other)),             // Future steals the state
                  lazy_continuations_base(std::move(other)), // Get control of continuations
                  stop_token_base(std::move(other))          // Move stop state
            {}

            /// \brief Move assignment. Inherited from base classes.
            basic_future &operator=(basic_future &&other) noexcept {
                if (&other == this) {
                    return *this;
                }
                wait_if_last();
                shared_base::operator=(std::move(other));             // Future steals the state
                lazy_continuations_base::operator=(std::move(other)); // Get control of continuations
                stop_token_base::operator=(std::move(other));         // Move stop state
                other.detach();
                return *this;
            }

            /// \brief Copy constructor. Inherited from base classes.
            basic_future(const basic_future &other) noexcept
                : shared_base(other),             // Copies reference to state (if operator= is defined)
                  lazy_continuations_base(other), // Copy reference to continuations
                  stop_token_base(other)          // Copy reference to stop state
            {}

            /// \brief Copy assignment. Inherited from base classes.
            basic_future &operator=(const basic_future &other) noexcept {
                if (&other == this) {
                    return *this;
                }
                wait_if_last();
                shared_base::operator=(other);             // Copies reference to state (if operator= is defined)
                lazy_continuations_base::operator=(other); // Copy reference to continuations
                stop_token_base::operator=(other);         // Copy reference to stop state
                other.detach();
                return *this;
            }

            /// \brief Destructor
            /// We let stoppable set the stop token and wait.
            ~basic_future() {
                if constexpr (is_stoppable_v && not is_shared_v) {
                    if (valid() && not is_ready()) {
                        stop_token_base::request_stop();
                    }
                }
                wait_if_last();
                if constexpr (is_lazy_continuable_v) {
                    continuations_source &cs = lazy_continuations_base::continuations_source_;
                    if (cs.run_possible()) {
                        cs.request_run();
                    }
                }
            }

            /// \brief Wait until all futures have a valid result and retrieves it
            /// The behaviour depends on shared_based.
            decltype(auto) get() {
                if (not valid()) {
                    throw std::future_error(std::future_errc::no_state);
                }
                return shared_base::future_->get();
            }

            /// \brief Checks if the future refers to a shared state
            decltype(auto) valid() const { return shared_base::future_ && shared_base::future_->valid(); }

            /// \brief Blocks until the result becomes available.
            void wait() const {
                if (not valid()) {
                    throw std::future_error(std::future_errc::no_state);
                }
                shared_base::future_->wait(); }

            /// \brief Waits for the result to become available.
            template <class Rep, class Period>
            [[nodiscard]] std::future_status wait_for(const std::chrono::duration<Rep, Period> &timeout_duration)
                const {
                if (not valid()) {
                    throw std::future_error(std::future_errc::no_state);
                }
                return shared_base::future_->wait_for(timeout_duration);
            }

            /// \brief Waits for the result to become available.
            template <class Clock, class Duration>
            std::future_status wait_until(const std::chrono::time_point<Clock, Duration> &timeout_time) const {
                if (not valid()) {
                    throw std::future_error(std::future_errc::no_state);
                }
                return shared_base::future_->wait_for(timeout_time);
            }

            /// \brief Checks if the shared state is ready
            [[nodiscard]] bool is_ready() const {
                if (not valid()) {
                    throw std::future_error(std::future_errc::no_state);
                }
                return shared_base::future_->template wait_for(std::chrono::seconds(0)) == std::future_status::ready;
            }

            /// \brief Tell this future not to join at destruction
            /// For safety, all futures join at destruction
            void detach() { join_ = false; }

          private:
            /// \section Functions

            void wait_if_last() const {
                if (join_ && valid() && not is_ready()) {
                    if constexpr (not is_shared_v) {
                        wait();
                    } else {
                        if (shared_base::future_.use_count() == 1) {
                            wait();
                        }
                    }
                }
            }

          private:
            /// \section Members

            bool join_{true};
        };
    } // namespace detail

    /// \section Define basic_future as a kind of future
    template <typename... Args> struct is_future<detail::basic_future<Args...>> : std::true_type {};
    template <typename... Args> struct is_future<detail::basic_future<Args...> &> : std::true_type {};
    template <typename... Args> struct is_future<detail::basic_future<Args...> &&> : std::true_type {};
    template <typename... Args> struct is_future<const detail::basic_future<Args...>> : std::true_type {};
    template <typename... Args> struct is_future<const detail::basic_future<Args...> &> : std::true_type {};

    /// \section Define basic_futures as supporting lazy continuations
    template <class T, class SH, class L, class ST> struct is_shared_future<detail::basic_future<T, SH, L, ST>> : SH {};
    template <class T, class SH, class L, class ST>
    struct is_shared_future<detail::basic_future<T, SH, L, ST> &> : SH {};
    template <class T, class SH, class L, class ST>
    struct is_shared_future<detail::basic_future<T, SH, L, ST> &&> : SH {};
    template <class T, class SH, class L, class ST>
    struct is_shared_future<const detail::basic_future<T, SH, L, ST>> : SH {};
    template <class T, class SH, class L, class ST>
    struct is_shared_future<const detail::basic_future<T, SH, L, ST> &> : SH {};

    /// \section Define basic_futures as supporting lazy continuations
    template <class T, class SH, class L, class ST>
    struct is_lazy_continuable<detail::basic_future<T, SH, L, ST>> : L {};
    template <class T, class SH, class L, class ST>
    struct is_lazy_continuable<detail::basic_future<T, SH, L, ST> &> : L {};
    template <class T, class SH, class L, class ST>
    struct is_lazy_continuable<detail::basic_future<T, SH, L, ST> &&> : L {};
    template <class T, class SH, class L, class ST>
    struct is_lazy_continuable<const detail::basic_future<T, SH, L, ST>> : L {};
    template <class T, class SH, class L, class ST>
    struct is_lazy_continuable<const detail::basic_future<T, SH, L, ST> &> : L {};

    /// \section Define basic_futures as having a stop token
    template <class T, class S, class L, class Stoppable>
    struct has_stop_token<detail::basic_future<T, S, L, Stoppable>> : Stoppable {};
    template <class T, class S, class L, class Stoppable>
    struct has_stop_token<detail::basic_future<T, S, L, Stoppable> &> : Stoppable {};
    template <class T, class S, class L, class Stoppable>
    struct has_stop_token<detail::basic_future<T, S, L, Stoppable> &&> : Stoppable {};
    template <class T, class S, class L, class Stoppable>
    struct has_stop_token<const detail::basic_future<T, S, L, Stoppable>> : Stoppable {};
    template <class T, class S, class L, class Stoppable>
    struct has_stop_token<const detail::basic_future<T, S, L, Stoppable> &> : Stoppable {};

    /// \section Define basic_futures as being stoppable (not the same as having a stop token for other future types)
    template <class T, class S, class L, class Stoppable>
    struct is_stoppable<detail::basic_future<T, S, L, Stoppable>> : Stoppable {};
    template <class T, class S, class L, class Stoppable>
    struct is_stoppable<detail::basic_future<T, S, L, Stoppable> &> : Stoppable {};
    template <class T, class S, class L, class Stoppable>
    struct is_stoppable<detail::basic_future<T, S, L, Stoppable> &&> : Stoppable {};
    template <class T, class S, class L, class Stoppable>
    struct is_stoppable<const detail::basic_future<T, S, L, Stoppable>> : Stoppable {};
    template <class T, class S, class L, class Stoppable>
    struct is_stoppable<const detail::basic_future<T, S, L, Stoppable> &> : Stoppable {};

} // namespace futures

#endif // CPP_MANIFEST_BASIC_FUTURE_H
