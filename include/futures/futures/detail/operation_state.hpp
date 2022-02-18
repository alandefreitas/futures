//
// Copyright (c) 2021 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_FUTURES_DETAIL_OPERATION_STATE_HPP
#define FUTURES_FUTURES_DETAIL_OPERATION_STATE_HPP

#include <futures/futures/future_error.hpp>
#include <futures/futures/future_options.hpp>
#include <futures/adaptor/detail/unwrap_and_continue.hpp>
#include <futures/detail/container/small_vector.hpp>
#include <futures/detail/thread/relocker.hpp>
#include <futures/futures/detail/continuations_source.hpp>
#include <futures/futures/detail/operation_state_storage.hpp>
#include <futures/futures/detail/traits/is_in_args.hpp>
#include <atomic>
#include <future>
#include <condition_variable>

namespace futures::detail {
    /** @addtogroup futures Futures
     *  @{
     */
    /// Members functions and objects common to all shared state
    /// object types (bool ready and exception ptr)
    ///
    /// Shared states for asynchronous operations contain an element of a
    /// given type and an exception.
    ///
    /// All objects such as futures and promises have shared states and
    /// inherit from this class to synchronize their access to their common
    /// shared state.
    template <bool is_always_deferred>
    class operation_state_base
    {
    private:
        /// A list of waiters: condition variables to notify any
        /// object waiting for this shared state to be ready
        using waiter_list = detail::small_vector<std::condition_variable_any *>;

    public:
        /// A handle to notify an external context about this state being ready
        using notify_when_ready_handle = waiter_list::iterator;

        /// Virtual shared state data destructor
        /**
         *  Virtual to make it inheritable.
         */
        virtual ~operation_state_base() = default;

        /// Constructor
        operation_state_base() = default;

        /// Delete copy constructor
        /**
         *  We should usually not copy construct the operation state data
         *  because its members, which include synchronization primitives,
         *  should be shared in eager futures.
         *
         *  In deferred futures, copying would still represent a logic error
         *  at a higher level. A shared deferred future will still use a
         *  shared operation state, while a unique deferred future is not
         *  allowed to be copied.
         */
        operation_state_base(const operation_state_base &) = delete;

        /// Move constructor
        /**
         * Moving the operation state is only valid before the task is running,
         * as it might happen with deferred futures.
         *
         * At this point, we can ignore the synchronization primitives and
         * let other object steal the contents while recreating the condition
         * variables.
         *
         * @param other
         */
        operation_state_base(operation_state_base &&other) noexcept
            : except_{ std::move(other.except_) },
              external_waiters_(std::move(other.external_waiters_)) {
            assert(!is_waiting());
            other.waiter_.notify_all();
            std::unique_lock lk(other.waiters_mutex_);
        };

        /// Cannot copy assign the shared state data
        /**
         * We cannot copy assign the shared state data because this base
         * class holds synchronization primitives
         */
        operation_state_base &
        operator=(const operation_state_base &)
            = delete;

        /// Indicate to the shared state the state is ready
        /**
         *  This operation marks the ready_ flags and warns any future
         *  waiting on it. This overload is meant to be used by derived
         *  classes that might need to use another mutex for this operation
         */
        void
        set_ready() noexcept {
            // notify all waiters
            auto notify_all = [this]() {
                auto lk = create_wait_lock();
                waiter_.notify_all();
                for (auto &&external_waiter: external_waiters_) {
                    external_waiter->notify_all();
                }
            };

            // set state to ready and notify all
            if constexpr (!is_always_deferred) {
                status prev = status_.exchange(
                    status::ready,
                    std::memory_order_release);
                if (prev == status::waiting) {
                    std::atomic_thread_fence(std::memory_order_acquire);
                    notify_all();
                }
            } else {
                status prev = std::exchange(status_, status::ready);
                if (prev == status::waiting) {
                    notify_all();
                }
            }
        }

        /// Check if operation state is ready
        bool
        is_ready() const {
            return is_status(status::ready);
        }

        /// Check if operation state is waiting
        bool
        is_waiting() const {
            return is_status(status::waiting);
        }

        /// Check if state is ready with no exception
        bool
        succeeded() const {
            return is_ready() && !except_;
        }

        /// Set shared state to an exception
        /**
         *  This sets the exception value and marks the shared state as
         *  ready. If we try to set an exception on a shared state that's
         *  ready, we throw an exception. This overload is meant to be used
         *  by derived classes that might need to use another mutex for this
         *  operation.
         */
        void
        set_exception(std::exception_ptr except) {
            if (is_ready()) {
                detail::throw_exception<promise_already_satisfied>();
            }
            // ready_ already protects except_ because only one thread
            // should call this function
            except_ = std::move(except);
            set_ready();
        }

        /// Get the shared state when it's an exception
        /**
         *  This overload uses the default global mutex for synchronization
         */
        std::exception_ptr
        get_exception_ptr() const {
            if (!is_ready()) {
                detail::throw_exception<promise_uninitialized>();
            }
            return except_;
        }

        /// Rethrow the exception we have stored
        void
        throw_internal_exception() const {
            std::rethrow_exception(get_exception_ptr());
        }

        /// Indicate to the shared state its owner has been destroyed
        /**
         *  This allows us to set an error if the promise has been destroyed
         *  too early.
         *
         *  If owner has been destroyed before the shared state is ready,
         *  this means a promise has been broken and the shared state should
         *  store an exception. This overload is meant to be used by derived
         *  classes that might need to use another mutex for this operation
         */
        void
        signal_promise_destroyed() {
            if (!is_ready()) {
                set_exception(std::make_exception_ptr(broken_promise()));
            }
        }

        /// Check if state is ready without locking
        bool
        failed() const {
            return is_ready() && except_ != nullptr;
        }

        /// Wait for shared state to become ready
        /**
         *  This function uses the condition variable waiters to wait for
         *  this shared state to be marked as ready.
         *
         *  Atomic operations are used to ensure we only involve the
         *  waiting lock if the shared state is not ready yet.
         */
        void
        wait() {
            status expected = status::initial;
            compare_exchange_status(expected, status::waiting);
            if (expected != status::ready) {
                wait_for_parent();
                if (expected == status::initial) {
                    this->post_deferred();
                }
                auto lk = create_wait_lock();
                waiter_.wait(lk, [this]() { return is_ready(); });
            }
        }

        /// Wait for the shared state to become ready
        /**
         *  This function uses the condition variable waiters to wait for
         *  this shared state to be marked as ready for a specified
         *  duration.
         *
         *  This overload is meant to be used by derived classes
         *  that might need to use another mutex for this operation.
         *
         *  @tparam Rep An arithmetic type representing the number of ticks
         *  @tparam Period A std::ratio representing the tick period
         *  @param timeout_duration maximum duration to block for
         *  @return The state of the shared value
         */
        template <typename Rep, typename Period>
        std::future_status
        wait_for(std::chrono::duration<Rep, Period> const &timeout_duration) {
            status expected = status::initial;
            compare_exchange_status(expected, status::waiting);
            if (expected != status::ready) {
                wait_for_parent();
                if (expected == status::initial) {
                    this->post_deferred();
                }
                auto lk = create_wait_lock();
                if (waiter_.wait_for(lk, timeout_duration, [this]() {
                        return is_ready();
                    })) {
                    return std::future_status::ready;
                } else {
                    return std::future_status::timeout;
                }
            }
            return std::future_status::ready;
        }

        /// Wait for the shared state to become ready
        /**
         *  This function uses the condition variable waiters
         *  to wait for this shared state to be marked as ready until a
         *  specified time point. This overload is meant to be used by
         *  derived classes that might need to use another mutex for this
         *  operation
         *
         *  @tparam Clock The clock type
         *  @tparam Duration The duration type
         *  @param timeout_time maximum time point to block until
         *  @return The state of the shared value
         */
        template <typename Clock, typename Duration>
        std::future_status
        wait_until(
            std::chrono::time_point<Clock, Duration> const &timeout_time) {
            status expected = status::initial;
            compare_exchange_status(expected, status::waiting);
            if (expected != status::ready) {
                wait_for_parent();
                if (expected == status::initial) {
                    this->post_deferred();
                }
                auto lk = create_wait_lock();
                if (waiter_.wait_until(lk, timeout_time, [this]() {
                        return is_ready();
                    })) {
                    return std::future_status::ready;
                } else {
                    return std::future_status::timeout;
                }
            }
            return std::future_status::ready;
        }

        /// Include a condition variable in the list of waiters
        /**
         *  These are external waiters we need to notify when the state is
         *  ready.
         *
         *  @param cv Reference to an external condition variable
         *
         *  @return Handle which can be used to unnotify when ready
         */
        notify_when_ready_handle
        notify_when_ready(std::condition_variable_any &cv) {
            status expected = status::initial;
            compare_exchange_status(expected, status::waiting);
            if (expected == status::initial) {
                this->post_deferred();
            }
            wait_for_parent();
            return external_waiters_.insert(external_waiters_.end(), &cv);
        }

        /// Remove condition variable from list of external waiters
        /**
         *  @param it External condition variable
         */
        void
        unnotify_when_ready(notify_when_ready_handle it) {
            auto lk = create_wait_lock();
            external_waiters_.erase(it);
        }

        /// Post a deferred function
        virtual void
        post_deferred() {
            // override in deferred futures only
        }

        /// Wait for parent operation
        virtual void
        wait_for_parent() {
            // override in deferred futures only
        }

        /// Get a reference to the mutex in the shared state
        std::mutex &
        waiters_mutex() {
            return waiters_mutex_;
        }

        /// Generate unique lock for the shared state
        ///
        /// This lock can be used for any operations on the state that might
        /// need to be protected/
        std::unique_lock<std::mutex>
        create_wait_lock() const {
            return std::unique_lock{ waiters_mutex_ };
        }

    private:
        /// The current status of this shared state
        enum status : uint8_t
        {
            /// Nothing happened yet
            initial,
            /// Someone is waiting for the result
            waiting,
            /// The state has been set and we notified everyone
            ready,
        };

        /// Indicates if the shared state is already set
        /**
         *  There are three states possible: initial, waiting, ready
         */
        mutable std::
            conditional_t<is_always_deferred, status, std::atomic<status>>
                status_{ status::initial };

        /// Check the status of the operation state
        bool
        is_status(status s) const {
            if constexpr (!is_always_deferred) {
                return status_.load(std::memory_order_acquire) == s;
            } else {
                return status_ == s;
            }
        }

        /// Compare status and exchange if it matches the previous value
        /**
         * This function:
         * 1) replaces the expected status variable with the old status
         * 2) replaces status with new_value if old value matches expected value
         * 3) return true if the value was changed
         *
         * @param expected Expected status and reference to store the old status
         * @param new_value New status value
         * @return True if the status has changed
         */
        bool
        compare_exchange_status(status &expected, status new_value) {
            if constexpr (!is_always_deferred) {
                return status_.compare_exchange_strong(expected, new_value);
            } else {
                bool match_expected = status_ == expected;
                expected = status_;
                if (match_expected) {
                    status_ = new_value;
                    return true;
                } else {
                    return false;
                }
            }
        }

        /// Pointer to an exception, when the shared_state fails
        /**
         *  std::exception_ptr does not need to be atomic because
         *  the status variable is guarding it.
         */
        std::exception_ptr except_{ nullptr };

        /// Condition variable to notify any object waiting for this state
        /**
         *  This is the object we use to be able to block until the shared
         *  state is ready. Although the shared state is lock-free, users
         *  might still need to wait for results to be ready. One future
         *  thread calls `waiter_.wait(...)` to block while the promise
         *  thread calls `waiter_.notify_all(...)`. In C++20, atomic
         *  variables have `wait()` functions we could use to replace this
         *  with `ready_.wait(...)`
         */
        mutable std::condition_variable waiter_{};

        /// List of external condition variables waiting for the state
        /**
         *  While the internal waiter is intended for
         */
        waiter_list external_waiters_;

        /// Mutex for threads that want to wait on the result
        /**
         *  While the basic state operations is lock-free, it also includes a
         *  mutex that can be used for communication between futures, such as
         *  waiter futures.
         *
         *  This is used when one thread intents to wait for the result
         *  of a future. The mutex is not used for lazy futures or if
         *  the result is ready before the waiting operation.
         *
         *  These functions should be used directly by users very often.
         */
        mutable std::mutex waiters_mutex_{};
    };

    /// Operation state with its concrete storage
    /**
     * This class stores the data for a shared state holding an element of
     * type `R`, which might be a concrete type, a reference, or `void`.
     *
     * For most types, the data is stored as uninitialized storage because
     * this will only need to be initialized when the state becomes ready.
     * This ensures the shared state works for all types and avoids
     * wasting operations on a constructor we might not use.
     *
     * However, initialized storage is used for trivial types because
     * this involves no performance penalty. This also makes states
     * easier to debug.
     *
     * If the shared state returns a reference, the data is stored
     * internally as a pointer.
     *
     * A void shared state needs to synchronize waiting, but it does not
     * need to store anything.
     *
     * The storage also uses empty base optimization, which means it
     * takes no memory if the state is an empty type, such as `void` or
     * std::allocator.
     */
    template <class R, class Options>
    class operation_state
        // base state
        : public operation_state_base<Options::is_always_deferred>
        // enable shared pointers
        , public std::enable_shared_from_this<operation_state<R, Options>>
        // storage for the results
        , private operation_state_storage<R>
        // storage for the executor
        , private conditional_base<
              Options::has_executor,
              typename Options::executor_t,
              0>
        // storage for the continuations
        , private conditional_base<
              Options::is_continuable,
              continuations_source<Options::is_always_deferred>,
              1>
        // storage for the stop token
        , private conditional_base<Options::is_stoppable, stop_source, 2>
    {
    protected:
        static_assert(
            !Options::is_shared,
            "The underlying operation state cannot be shared");

        /// Operation state base type (without storage for the value)
        using operation_state_base_type = operation_state_base<
            Options::is_always_deferred>;

        /// Executor storage
        using executor_base = conditional_base<
            Options::has_executor,
            typename Options::executor_t,
            0>;

        using executor_type = typename executor_base::value_type;

        /// Continuations storage
        using continuations_base = conditional_base<
            Options::is_continuable,
            continuations_source<Options::is_always_deferred>,
            1>;

        using continuations_type = typename continuations_base::value_type;

        /// Stop source storage
        using stop_source_base
            = conditional_base<Options::is_stoppable, stop_source, 2>;

        using stop_source_type = typename stop_source_base::value_type;

        using stop_token_type = std::conditional_t<
            Options::is_stoppable,
            stop_token,
            detail::empty_value_type>;

    public:
        /// Destructor
        /**
         *  We might need to destroy the shared object R if the state is
         *  ready with a value.
         */
        ~operation_state() override {
            if constexpr (Options::is_continuable) {
                // The state might have been destroyed by the executor
                // by now. Use another shared reference to the
                // continuations.
                this->get_continuations_source().request_run();
            }
            if (this->succeeded()) {
                operation_state_storage<R>::destroy();
            }
        }

        /// Constructor
        /**
         *  This function will construct the state with storage for R.
         *
         *  This is often invalid because we cannot let it create an empty
         *  executor type. Nonetheless, this constructor is still useful for
         *  allocating pointers.
         *
         */
        operation_state() = default;

        /// Constructor for state with reference to executor
        /**
         * The executor allows us to emplace continuations on the
         * same executor by default.
         */
        explicit operation_state(const executor_type &ex) : executor_base(ex) {}

        /// Deleted copy constructor
        operation_state(operation_state const &) = delete;

        /// Move constructor
        operation_state(operation_state &&) noexcept = default;

        /// Deleted copy assignment operator
        operation_state &
        operator=(operation_state const &)
            = delete;

        /// Move assignment operator
        operation_state &
        operator=(operation_state &&)
            = default;

        /// Set the value of the shared state
        template <class... Args>
        void
        set_value(Args &&...args) {
            if (this->is_ready()) {
                detail::throw_exception<promise_already_satisfied>();
            }
            operation_state_storage<R>::set_value(std::forward<Args>(args)...);
            this->set_ready();
        }

        /// Set value with a callable and an argument list
        template <typename Fn, typename... Args>
        void
        apply(Fn &&fn, Args &&...args) {
            try {
                if constexpr (!Options::is_stoppable) {
                    if constexpr (std::is_void_v<R>) {
                        std::invoke(
                            std::forward<Fn>(fn),
                            std::forward<Args>(args)...);
                        this->set_value();
                    } else {
                        this->set_value(std::invoke(
                            std::forward<Fn>(fn),
                            std::forward<Args>(args)...));
                    }
                } else {
                    if constexpr (std::is_void_v<R>) {
                        std::invoke(
                            std::forward<Fn>(fn),
                            this->get_stop_token(),
                            std::forward<Args>(args)...);
                        this->set_value();
                    } else {
                        this->set_value(std::invoke(
                            std::forward<Fn>(fn),
                            this->get_stop_token(),
                            std::forward<Args>(args)...));
                    }
                }
            }
            catch (...) {
                this->set_exception(std::current_exception());
            }
            if constexpr (Options::is_continuable) {
                // The state might have been destroyed by the executor
                // by now. Use another shared reference to the
                // continuations.
                this->get_continuations_source().request_run();
            }
        }

        /// Set value with a callable and a tuple or arguments
        template <typename Fn, typename Tuple>
        void
        apply_tuple(Fn &&fn, Tuple &&targs) {
            return apply_tuple_impl(
                std::forward<Fn>(fn),
                std::forward<Tuple>(targs),
                std::make_index_sequence<
                    std::tuple_size_v<std::remove_reference_t<Tuple>>>{});
        }

        /// Get the value of the shared state
        /**
         * This function waits for the shared state to become ready and
         * returns its value.
         *
         * This function returns `R&` unless this is a shared state to
         * `void`.
         *
         * @return Reference to the state as a reference to R
         */
        std::add_lvalue_reference_t<R>
        get() {
            this->wait();
            if (this->failed()) {
                this->throw_internal_exception();
            }
            return operation_state_storage<R>::get();
        }

        /// Get executor associated with the operation
        const executor_type &
        get_executor() const noexcept {
            static_assert(Options::has_executor);
            return executor_base::get();
        }

        /// Create stop token associated with the operation
        stop_token
        get_stop_token() const noexcept {
            static_assert(Options::is_stoppable);
            return stop_source_base::get().get_token();
        }

        /// Create stop token associated with the operation
        stop_source &
        get_stop_source() const noexcept {
            static_assert(Options::is_stoppable);
            return stop_source_base::get();
        }

        /// Get operation stop source
        stop_source &
        get_stop_source() noexcept {
            static_assert(Options::is_stoppable);
            return stop_source_base::get();
        }

        /// Get continuations source
        typename continuations_base::value_type &
        get_continuations_source() const noexcept {
            static_assert(Options::is_continuable);
            return continuations_base::get();
        }

        /// Get continuations source
        typename continuations_base::value_type &
        get_continuations_source() noexcept {
            static_assert(Options::is_continuable);
            return continuations_base::get();
        }

    private:
        template <typename Fn, typename Tuple, std::size_t... I>
        void
        apply_tuple_impl(Fn &&fn, Tuple &&targs, std::index_sequence<I...>) {
            return apply(
                std::forward<Fn>(fn),
                std::get<I>(std::forward<Tuple>(targs))...);
        }
    };

    /// A functor that binds function arguments for deferred futures
    /**
     * This function binds the function arguments to a function, generating
     * a named functor we can use in a deferred shared state.
     *
     * @tparam Fn Function type
     * @tparam Args Arguments
     */
    template <class Fn, class... Args>
    struct bind_deferred_state_args
    {
    public:
        template <class OtherFn, class... OtherArgs>
        explicit bind_deferred_state_args(OtherFn &&f, OtherArgs &&...args)
            : fn_(std::forward<OtherFn>(f)),
              args_(std::make_tuple(std::forward<OtherArgs>(args)...)) {}

        decltype(auto)
        operator()() {
            return std::apply(std::move(fn_), std::move(args_));
        }

    private:
        Fn fn_;
        std::tuple<std::decay_t<Args>...> args_;
    };

    /// An extension of shared state with storage for a deferred task
    /**
     *  This class provides the same functionality as shared state and
     *  extra storage for a deferred task.
     *
     *  Because futures keep references to a shared state, what this
     *  effectively achieves is type erasing the task type. Otherwise,
     *  we would need embed the task types in the futures.
     *
     *  For instance, this would make it impossible to have vectors of
     *  futures without wrapping tasks into a more limited std::function
     *  before for erasing the task type.
     */
    template <class R, class Options>
    class deferred_operation_state
        : public operation_state<R, Options>
        , private maybe_empty<typename Options::function_t, 3>
    {
    private:
        /// Deferred function storage
        using deferred_function_base
            = maybe_empty<typename Options::function_t, 3>;
        using deferred_function_type = typename deferred_function_base::
            value_type;

    public:
        /// Destructor
        /**
         *  We might need to destroy the shared object R if the state is
         *  ready with a value.
         */
        ~deferred_operation_state() override = default;

        /// Constructor
        /**
         *  This function will construct the state with storage for R.
         */
        deferred_operation_state() = default;

        /// Constructor from the deferred function
        /**
         *  The function accepts other function and args types so
         *  (i) we can forward the variables, and (ii) allow compatible
         *  types to be used here. Most of the time, these will be the
         *  same types as Fn and Args.
         */
        template <class Fn>
        explicit deferred_operation_state(
            const typename operation_state<R, Options>::executor_type &ex,
            Fn &&f)
            : operation_state<R, Options>(ex),
              deferred_function_base(std::forward<Fn>(f)) {}

        /// Constructor from the deferred function
        /**
         *  The function accepts other function and args types so
         *  (i) we can forward the variables, and (ii) allow
         compatible
         *  types to be used here. Most of the time, these will be the
         *  same types as Fn and Args.
         */
        template <class Fn, class... Args>
        explicit deferred_operation_state(
            const typename operation_state<R, Options>::executor_type &ex,
            Fn &&f,
            Args &&...args)
            : operation_state<R, Options>(ex),
              deferred_function_base(bind_deferred_state_args<Fn, Args...>(
                  std::forward<Fn>(f),
                  std::forward<Args>(args)...)) {}

        /// Deleted copy constructor
        /**
         *  The copy constructor does not make sense for shared states as
         *  they are meant to be shared in all of our use cases with
         *  promises and futures.
         *
         */
        deferred_operation_state(deferred_operation_state const &) = delete;

        /// Move constructor
        deferred_operation_state(
            deferred_operation_state &&) noexcept = default;

        /// Deleted copy assignment operator
        /**
         *  These functions do not make sense for shared states as they are
         *  meant to be shared.
         */
        deferred_operation_state &
        operator=(deferred_operation_state const &)
            = delete;

        /// Move assignment operator
        /**
         *  These functions do not make sense for shared states as they are
         *  meant to be shared.
         */
        deferred_operation_state &
        operator=(deferred_operation_state &&) noexcept = default;

        void
        post_deferred() override {
            // if this is a continuation, wait for tasks that come before
            if constexpr (Options::is_always_deferred) {
                if constexpr (Options::has_executor) {
                    asio::post(
                        operation_state<R, Options>::get_executor(),
                        [this]() {
                        // use shared_from_this to ensure the state lives while
                        // the deferred function is executed. Some futures might
                        // wait() until the shared state value is set but
                        // destroy the state before the continuations and
                        // related tasks are executed.
                        this->apply(
                            std::move(this->deferred_function_base::get()));
                        });
                } else {
                    this->apply(std::move(deferred_function_base::get()));
                }
            }
        }

        /// Wait for the parent operation state to be set
        /**
         * Deferred states that refer to continuations need to wait for the
         * parent task to finish before invoking the continuation task.
         *
         * This only happens when the operation state is storing a continuation.
         */
        void
        wait_for_parent() override {
            if constexpr (is_unwrap_and_continue_task<
                              deferred_function_type>::value) {
                deferred_function_base::get().before_.wait();
            }
        }

        /// Get the current value from this operation state
        using operation_state<R, Options>::get;

        void
        swap(deferred_operation_state &other) {
            std::swap(
                static_cast<operation_state<R, Options> &>(*this),
                static_cast<operation_state<R, Options> &>(other));
            std::swap(
                static_cast<deferred_function_base &>(*this),
                static_cast<deferred_function_base &>(other));
        }
    };

    /** @} */ // @addtogroup futures Futures
} // namespace futures::detail

#endif // FUTURES_FUTURES_DETAIL_OPERATION_STATE_HPP
